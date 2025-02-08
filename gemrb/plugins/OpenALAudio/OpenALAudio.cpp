/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2025 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#include "OpenALAudio.h"

#include "Logging/Logging.h"

#ifdef HAVE_OPENAL_EFX_H
static LPALGENEFFECTS alGenEffects = NULL;
static LPALDELETEEFFECTS alDeleteEffects = NULL;
static LPALISEFFECT alIsEffect = NULL;
static LPALGENAUXILIARYEFFECTSLOTS alGenAuxiliaryEffectSlots = NULL;
static LPALDELETEAUXILIARYEFFECTSLOTS alDeleteAuxiliaryEffectSlots = NULL;
static LPALEFFECTI alEffecti = NULL;
static LPALEFFECTF alEffectf = NULL;
static LPALAUXILIARYEFFECTSLOTI alAuxiliaryEffectSloti = NULL;
#endif

namespace GemRB {

static bool hasEFX = false;
#ifdef HAVE_OPENAL_EFX_H
static ALuint efxEffectSlot = 0;
static ALuint efxEffect = 0;
#endif

static ALenum GetFormatEnum(int channels, int bits)
{
	switch (channels) {
		case 1:
			if (bits == 8)
				return AL_FORMAT_MONO8;
			else
				return AL_FORMAT_MONO16;

		case 2:
			if (bits == 8)
				return AL_FORMAT_STEREO8;
			else
				return AL_FORMAT_STEREO16;
	}

	return AL_FORMAT_MONO8;
}

// Be aware that alGetError may be thread-safe, or not: https://github.com/kcat/openal-soft/commit/0584aec5d43f52d762f335cca0d62513f48328bd
static bool CheckALError(const char* msg, LogLevel level)
{
	int error = alGetError();
	if (error != AL_NO_ERROR) {
		Log(level, "OpenAL", "{}: {:#x} - {}", msg, error, alGetString(error));
		return true;
	}
	return false;
}

static void ShowALCError(const char* msg, LogLevel level, ALCdevice* device)
{
	int error = alcGetError(device);
	if (error != AL_NO_ERROR) {
		Log(level, "OpenAL", "{}: {:#x}", msg, error);
	}
}

static bool QueueBuffer(ALuint source, ALuint buffer)
{
	alSourceQueueBuffers(source, 1, &buffer);
	if (CheckALError("Unable to queue buffer", ERROR)) {
		return false;
	}

	return true;
}

static void ConfigureSource(ALuint source, const AudioPlaybackConfig& config)
{
	std::array<ALfloat, 3> sourceVel = { 0.0f, 0.0f, 0.0f };
	std::array<ALfloat, 3> sourcePos = { 0.0f, 0.0f, 0.0f };

	if (config.spatial) {
		sourcePos[0] = float(config.position.x);
		sourcePos[1] = float(config.position.y);
		sourcePos[2] = float(config.position.z);
	}

	alSourcefv(source, AL_VELOCITY, sourceVel.data());
	alSourcei(source, AL_LOOPING, config.loop);
	alSourcef(source, AL_GAIN, 0.0001f * (config.channelVolume * config.masterVolume));
	alSourcei(source, AL_SOURCE_RELATIVE, !config.spatial);
	alSourcefv(source, AL_POSITION, sourcePos.data());
	alSourcei(source, AL_ROLLOFF_FACTOR, 0);

	if (config.spatial) {
		alSourcei(source, AL_ROLLOFF_FACTOR, 1);
		alSourcei(source, AL_MAX_DISTANCE, config.muteDistance);
		alSourcei(source, AL_REFERENCE_DISTANCE, 10);
	}

#ifdef HAVE_OPENAL_EFX_H
	if (config.efx && hasEFX) {
		alSource3i(source, AL_AUXILIARY_SEND_FILTER, efxEffectSlot, 0, 0);
	} else {
		alSource3i(source, AL_AUXILIARY_SEND_FILTER, 0, 0, 0);
	}
#endif
}

OpenALBufferHandle::OpenALBufferHandle(ALPair buffers)
	: buffers(buffers)
{}

OpenALBufferHandle::OpenALBufferHandle(OpenALBufferHandle&& other) noexcept
	: buffers(std::move(other.buffers))
{
	other.buffers = { 0, 0 };
}

OpenALBufferHandle::~OpenALBufferHandle()
{
	OpenALBufferHandle::Disposable();
}

bool OpenALBufferHandle::Disposable()
{
	std::array<ALuint, 2> bufferArray = { buffers.first, buffers.second };
	alDeleteBuffers(buffers.second != 0 ? 2 : 1, bufferArray.data());
	auto success = alGetError() == AL_NO_ERROR;
	if (success) {
		buffers = { 0, 0 };
	}

	return success;
}

ALPair OpenALBufferHandle::GetBuffers() const
{
	return buffers;
}

AudioBufferFormat OpenALBufferHandle::GetFormat() const
{
	ALint buffer;
	AudioBufferFormat format;
	alGetBufferi(buffers.first, AL_FREQUENCY, &buffer);
	format.sampleRate = buffer;
	alGetBufferi(buffers.first, AL_BITS, &buffer);
	format.bits = buffer;
	alGetBufferi(buffers.first, AL_CHANNELS, &buffer);
	format.channels = buffer;

	return format;
}

OpenALSourceHandle::OpenALSourceHandle(ALPair sources, const AudioPlaybackConfig& config)
	: sources(sources)
{
	OpenALSourceHandle::Reconfigure(config);
}

OpenALSourceHandle::OpenALSourceHandle(OpenALSourceHandle&& other) noexcept
	: sources(std::move(other.sources))
{
	other.sources = { 0, 0 };
}

OpenALSourceHandle::~OpenALSourceHandle()
{
	std::array<ALuint, 2> sourcesArray = { sources.first, sources.second };
	auto numSources = sources.second != 0 ? 2 : 1;

	alSourceStopv(numSources, sourcesArray.data());
	alSourcei(sources.first, AL_BUFFER, 0);
	if (numSources == 2) {
		alSourcei(sources.second, AL_BUFFER, 0);
	}

	alDeleteSources(numSources, sourcesArray.data());
}

bool OpenALSourceHandle::HasFinishedPlaying() const
{
	ALint state = 0;
	alGetSourcei(sources.first, AL_SOURCE_STATE, &state);

	return state == AL_STOPPED;
}

bool OpenALSourceHandle::Enqueue(Holder<SoundBufferHandle> handle)
{
	auto alHandle = std::dynamic_pointer_cast<OpenALBufferHandle>(handle);
	if (!alHandle) {
		return false;
	}

	// Must stop when enqueing different formats
	auto nextFormat = alHandle->GetFormat();
	if (lastFormat.bits != 0 && !(nextFormat == lastFormat)) {
		Stop();
	}
	lastFormat = nextFormat;

	auto buffers = alHandle->GetBuffers();
	alSourceQueueBuffers(sources.first, 1, &buffers.first);
	if (CheckALError("Unable to queue buffer", ERROR)) {
		return false;
	}

	if (sources.second != 0 && buffers.second != 0) {
		alSourceQueueBuffers(sources.second, 1, &buffers.second);
	}

	std::array<ALuint, 2> sourcesArray = { sources.first, sources.second };
	alSourcePlayv(sources.second != 0 ? 2 : 1, sourcesArray.data());

	return true;
}

void OpenALSourceHandle::Reconfigure(const AudioPlaybackConfig& config)
{
	channelVolume = config.channelVolume;
	ConfigureSource(sources.first, config);
	if (sources.second != 0) {
		ConfigureSource(sources.second, config);
	}
}


void OpenALSourceHandle::Stop()
{
	std::array<ALuint, 2> sourcesArray = { sources.first, sources.second };
	alSourceStopv(sources.second != 0 ? 2 : 1, sourcesArray.data());

	alSourcei(sources.first, AL_BUFFER, 0);
	if (sources.second != 0) {
		alSourcei(sources.second, AL_BUFFER, 0);
	}
}

void OpenALSourceHandle::StopLooping()
{
	alSourcei(sources.first, AL_LOOPING, 0);
	if (sources.second) {
		alSourcei(sources.second, AL_LOOPING, 0);
	}
}

void OpenALSourceHandle::SetPosition(const AudioPoint& point)
{
	std::array<ALfloat, 3> pos = { float(point.x), float(point.y), float(point.z) };

	alSourcefv(sources.first, AL_POSITION, pos.data());
	if (sources.second) {
		alSourcefv(sources.second, AL_POSITION, pos.data());
	}
}

void OpenALSourceHandle::SetPitch(int pitch)
{
	float fPitch = 0.01f * pitch;
	alSourcef(sources.first, AL_PITCH, fPitch);
	if (sources.second) {
		alSourcef(sources.second, AL_PITCH, fPitch);
	}
}

void OpenALSourceHandle::SetVolume(int volume)
{
	float fVolume = 0.0001f * volume * channelVolume;
	alSourcef(sources.first, AL_GAIN, fVolume);
	if (sources.second) {
		alSourcef(sources.second, AL_GAIN, fVolume);
	}
}

OpenALSoundStreamHandle::OpenALSoundStreamHandle(ALint source, int channelVolume)
	: source(source), channelVolume(channelVolume)
{}

OpenALSoundStreamHandle::OpenALSoundStreamHandle(OpenALSoundStreamHandle&& other) noexcept
	: source(other.source)
{
	other.source = 0;
}

OpenALSoundStreamHandle::~OpenALSoundStreamHandle()
{
	OpenALSoundStreamHandle::Stop();
	alDeleteSources(1, &source);
}

bool OpenALSoundStreamHandle::Feed(const AudioBufferFormat& format, const char* memory, size_t size)
{
	UnloadFinishedSourceBuffers();

	ALuint buffer;
	alGenBuffers(1, &buffer);
	if (CheckALError("Unable to create stream buffer", ERROR)) {
		return false;
	}

	alBufferData(buffer, GetFormatEnum(format.channels, format.bits), memory, size, format.sampleRate);
	if (CheckALError("Unable to buffer data", ERROR)) {
		alDeleteBuffers(1, &buffer);
		return false;
	}

	if (!QueueBuffer(source, buffer)) {
		alDeleteBuffers(1, &buffer);
		return false;
	}

	ALint state = 0;
	alGetSourcei(source, AL_SOURCE_STATE, &state);
	if (state != AL_PLAYING) {
		alSourcePlay(source);
	}

	return true;
}

bool OpenALSoundStreamHandle::HasProcessed()
{
	ALint processed = 0;
	alGetSourcei(source, AL_BUFFERS_PROCESSED, &processed);

	return processed > 0;
}

void OpenALSoundStreamHandle::Pause()
{
	alSourcePause(source);
}

void OpenALSoundStreamHandle::Resume()
{
	alSourcePlay(source);
}

void OpenALSoundStreamHandle::Stop()
{
	alSourceStop(source);
	UnloadFinishedSourceBuffers();
}

void OpenALSoundStreamHandle::SetVolume(int volume)
{
	float fVolume = 0.0001f * (volume * channelVolume);
	alSourcef(source, AL_GAIN, fVolume);
}

void OpenALSoundStreamHandle::UnloadFinishedSourceBuffers() const
{
	ALint processed = 0;
	alGetSourcei(source, AL_BUFFERS_PROCESSED, &processed);
	if (processed > 0) {
		std::vector<ALuint> buffers;
		buffers.resize(processed);

		alSourceUnqueueBuffers(source, processed, buffers.data());
#ifdef __APPLE__ // mac os x and iOS
		/* FIXME: hackish
			somebody with more knowledge than me could perhapps figure out
			why Apple's implementation of alSourceUnqueueBuffers seems to delay (threading thing?)
			and possible how better to deal with this.
		*/
		do {
			alDeleteBuffers(processed, buffers.data());
		} while (alGetError() != AL_NO_ERROR);
#else
		alDeleteBuffers(processed, buffers.data());
#endif
	}
}

bool OpenALBackend::Init()
{
	Log(MESSAGE, "OpenAL", "Initializing OpenAL driver");

	ALCdevice* device = alcOpenDevice(nullptr);
	if (device == nullptr) {
		Log(ERROR, "OpenAL", "Failed to acquire OpenAL device.");
		return false;
	}

	alContext = alcCreateContext(device, nullptr);
	if (alContext == nullptr) {
		ShowALCError("Failed to create context", ERROR, device);
		alcCloseDevice(device);
		return false;
	}

	if (!alcMakeContextCurrent(alContext)) {
		ShowALCError("Failed to select context", ERROR, device);
		alcDestroyContext(alContext);
		alcCloseDevice(device);
		return false;
	}

	InitEFX();
	if (!hasEFX) {
		Log(MESSAGE, "OpenAL", "EFX not available.");
	}

	alDistanceModel(AL_LINEAR_DISTANCE_CLAMPED);

	return true;
}

void OpenALBackend::InitEFX()
{
#ifdef HAVE_OPENAL_EFX_H
	ALCdevice* device = alcGetContextsDevice(alContext);
	ALCint auxSends = 0;

	if (AL_FALSE == alcIsExtensionPresent(device, "ALC_EXT_EFX")) {
		return;
	}

	alcGetIntegerv(device, ALC_MAX_AUXILIARY_SENDS, 1, &auxSends);

	if (auxSends < 1) {
		return;
	}

	alGenEffects = reinterpret_cast<LPALGENEFFECTS>(alGetProcAddress("alGenEffects"));
	alDeleteEffects = reinterpret_cast<LPALDELETEEFFECTS>(alGetProcAddress("alDeleteEffects"));
	alIsEffect = reinterpret_cast<LPALISEFFECT>(alGetProcAddress("alIsEffect"));
	alGenAuxiliaryEffectSlots = reinterpret_cast<LPALGENAUXILIARYEFFECTSLOTS>(alGetProcAddress("alGenAuxiliaryEffectSlots"));
	alDeleteAuxiliaryEffectSlots = reinterpret_cast<LPALDELETEAUXILIARYEFFECTSLOTS>(alGetProcAddress("alDeleteAuxiliaryEffectSlots"));
	alEffecti = reinterpret_cast<LPALEFFECTI>(alGetProcAddress("alEffecti"));
	alEffectf = reinterpret_cast<LPALEFFECTF>(alGetProcAddress("alEffectf"));
	alAuxiliaryEffectSloti = reinterpret_cast<LPALAUXILIARYEFFECTSLOTI>(alGetProcAddress("alAuxiliaryEffectSloti"));

	if (!alGenEffects || !alDeleteEffects || !alIsEffect) {
		return;
	}

	alGenAuxiliaryEffectSlots(1, &efxEffectSlot);

	if (AL_NO_ERROR != alGetError()) {
		return;
	}

	alGenEffects(1, &efxEffect);

	if (AL_NO_ERROR != alGetError()) {
		return;
	}

	if (alIsEffect(efxEffect)) {
		alEffecti(efxEffect, AL_EFFECT_TYPE, AL_EFFECT_REVERB);

		if (AL_NO_ERROR != alGetError()) {
			return;
		}

		alAuxiliaryEffectSloti(efxEffectSlot, AL_EFFECTSLOT_EFFECT, efxEffect);
		if (AL_NO_ERROR != alGetError()) {
			return;
		}

		hasEFX = true;
	}
#endif
}

OpenALBackend::~OpenALBackend()
{
#ifdef HAVE_OPENAL_EFX_H
	if (hasEFX) {
		alDeleteAuxiliaryEffectSlots(1, &efxEffectSlot);
		alDeleteEffects(1, &efxEffect);
	}
#endif

	alcMakeContextCurrent(nullptr);
	ALCdevice* device = alcGetContextsDevice(alContext);
	alcDestroyContext(alContext);
	alcCloseDevice(device);
}

Holder<SoundSourceHandle> OpenALBackend::CreatePlaybackSource(const AudioPlaybackConfig& config, bool)
{
	std::array<ALuint, 2> sourcesArray = { 0, 0 };
	alGenSources(config.spatial != 0 ? 2 : 1, sourcesArray.data());
	if (CheckALError("Error creating source", ERROR)) {
		return Holder<SoundSourceHandle>();
	}

	return MakeHolder<OpenALSourceHandle>(ALPair { sourcesArray[0], sourcesArray[1] }, config);
}

Holder<SoundStreamSourceHandle> OpenALBackend::CreateStreamable(const AudioPlaybackConfig& config, size_t)
{
	ALuint source = 0;
	alGenSources(1, &source);
	if (CheckALError("Error creating source", ERROR)) {
		return {};
	}

	ConfigureSource(source, config);

	return MakeHolder<OpenALSoundStreamHandle>(source, config.channelVolume);
}

Holder<SoundBufferHandle> OpenALBackend::LoadSound(
	ResourceHolder<SoundMgr> resource,
	const AudioPlaybackConfig& config)
{
	auto buffers = GetBuffers(resource, config.spatial);
	if (buffers.first == 0) {
		return Holder<SoundBufferHandle>();
	}

	return MakeHolder<OpenALBufferHandle>(buffers);
}

ALPair OpenALBackend::GetBuffers(ResourceHolder<SoundMgr> resource, bool spatial) const
{
	auto channels = resource->GetChannels();
	assert(channels <= 2);
	bool spatialStereo = channels > 1 && spatial;

	std::array<ALuint, 2> buffers = { 0, 0 };
	alGenBuffers(spatialStereo ? 2 : 1, buffers.data());
	if (CheckALError("Unable to create sound buffer", ERROR)) {
		return { 0, 0 };
	}

	auto sampleRate = resource->GetSampleRate();
	auto numSamples = resource->GetNumSamples();
	auto totalBytesPerChannel = numSamples * 2;

	// Positional sound doesn't work for stereo in all known implementations
	// so make two sources and play them in parallel: https://openal.org/pipermail/openal/2016-August/000527.html
	if (spatialStereo) {
		std::vector<char> channel1;
		std::vector<char> channel2;
		channel1.resize(totalBytesPerChannel);
		channel2.resize(totalBytesPerChannel);
		auto actualSamples = resource->ReadSamplesIntoChannels(channel1.data(), channel2.data(), numSamples);

		auto format = GetFormatEnum(1, 16);
		alBufferData(buffers[0], format, channel1.data(), actualSamples * 2, sampleRate);
		alBufferData(buffers[1], format, channel2.data(), actualSamples * 2, sampleRate);
	} else {
		std::vector<short> memory;
		memory.resize(totalBytesPerChannel / sizeof(short));

		// multiply always with 2 because it is in 16 bits
		auto count = resource->read_samples(memory.data(), numSamples) * 2;
		// it is always reading the stuff into 16 bits
		alBufferData(buffers[0], GetFormatEnum(channels, 16), memory.data(), count, sampleRate);
	}

	if (CheckALError("Unable to fill buffer", ERROR)) {
		alDeleteBuffers(spatialStereo ? 2 : 1, buffers.data());
		CheckALError("Error deleting buffer", WARNING);
		return { 0, 0 };
	}

	return ALPair { buffers[0], buffers[1] };
}

const AudioPoint& OpenALBackend::GetListenerPosition() const
{
	return listenerPosition;
}

void OpenALBackend::SetListenerPosition(const AudioPoint& p)
{
	listenerPosition = p;
	alListener3f(AL_POSITION, p.x, p.y, 200);
	alListenerf(AL_GAIN, 1.25f);
}

void OpenALBackend::SetReverbProperties(const MapReverbProperties& props)
{
	if (!hasEFX) {
		return;
	}

#ifdef HAVE_OPENAL_EFX_H
	alDeleteEffects(1, &efxEffect);
	alGenEffects(1, &efxEffect);

	if (!props.reverbDisabled) {
		alEffecti(efxEffect, AL_EFFECT_TYPE, AL_EFFECT_REVERB);

		alEffectf(efxEffect, AL_REVERB_DENSITY, props.reverbData.flDensity);
		alEffectf(efxEffect, AL_REVERB_DIFFUSION, props.reverbData.flDiffusion);
		alEffectf(efxEffect, AL_REVERB_GAIN, props.reverbData.flGain);
		alEffectf(efxEffect, AL_REVERB_GAINHF, props.reverbData.flGainHF);
		alEffectf(efxEffect, AL_REVERB_DECAY_TIME, props.reverbData.flDecayTime);
		alEffectf(efxEffect, AL_REVERB_DECAY_HFRATIO, props.reverbData.flDecayHFRatio);
		alEffectf(efxEffect, AL_REVERB_REFLECTIONS_GAIN, props.reverbData.flReflectionsGain);
		alEffectf(efxEffect, AL_REVERB_REFLECTIONS_DELAY, props.reverbData.flReflectionsDelay);
		alEffectf(efxEffect, AL_REVERB_LATE_REVERB_GAIN, props.reverbData.flLateReverbGain);
		alEffectf(efxEffect, AL_REVERB_LATE_REVERB_DELAY, props.reverbData.flLateReverbDelay);
		alEffectf(efxEffect, AL_REVERB_AIR_ABSORPTION_GAINHF, props.reverbData.flAirAbsorptionGainHF);
		alEffectf(efxEffect, AL_REVERB_ROOM_ROLLOFF_FACTOR, props.reverbData.flRoomRolloffFactor);
		alEffecti(efxEffect, AL_REVERB_DECAY_HFLIMIT, props.reverbData.iDecayHFLimit);
	} else {
		alEffecti(efxEffect, AL_EFFECT_TYPE, 0);
	}

	alAuxiliaryEffectSloti(efxEffectSlot, AL_EFFECTSLOT_EFFECT, efxEffect);
#else
	(void) props;
#endif
}

}

#include "plugindef.h"

GEMRB_PLUGIN(0x27DD67E0, "OpenAL Audio Driver")
PLUGIN_DRIVER(OpenALBackend, "openal")
END_PLUGIN()
