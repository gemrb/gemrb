/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
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
 *
 */

#include "SDLAudio.h"

#include "ChannelManager.h"
#include "Interface.h"
#include "SoundMgr.h"

#include "Logging/Logging.h"

#include <SDL.h>
#include <chrono>

namespace GemRB {

// Don't increase too much since it will make the volume slider delay
static constexpr size_t RING_BUFFER_CHANNEL_SIZE = 16384;
static constexpr uint8_t RESERVED_CHANNELS = 1;

uint64_t SDLSoundSourceHandle::nextId = 0;
std::mutex MixCallbackState::mutex = {};
SDLSoundStreamSourceHandle* MixCallbackState::handle = nullptr;

static void SetChannelPosition(const AudioPoint& listenerPos, const AudioPlaybackConfig& config, int channel)
{
	auto p1 = static_cast<Point>(listenerPos);
	auto p2 = static_cast<Point>(config.position);

	int16_t angle = static_cast<int16_t>(AngleFromPoints(p1, p2) * 180 / M_PI + 90);
	// for unknown reasons, if the hardware reports only 2 channels, the angle needs to be shifted by 180°
	angle += SDLAudioBackend::audioChannels > 2 ? 0 : 180;

	uint8_t distance = std::min(255u * Distance(p1, p2) / config.muteDistance, 255u);
	Mix_SetPosition(channel, angle, distance);
}

SDLSoundBufferHandle::SDLSoundBufferHandle(Mix_Chunk* chunk, std::vector<char>&& buffer)
	: chunk(chunk), chunkBuffer(std::move(buffer)) {}

SDLSoundBufferHandle::~SDLSoundBufferHandle()
{
	Mix_FreeChunk(chunk);
}

bool SDLSoundBufferHandle::Disposable()
{
	bool playing = false;
	for (auto it = assignedChannels.begin(); it != assignedChannels.end();) {
		if (Mix_Playing(*it) && Mix_GetChunk(*it) == chunk) {
			playing = true;
			++it;
		} else {
			it = assignedChannels.erase(it);
		}
	}

	return playing;
}

void SDLSoundBufferHandle::AssignChannel(int channel)
{
	assignedChannels.insert(channel);
}

Mix_Chunk* SDLSoundBufferHandle::GetChunk()
{
	return chunk;
}

SDLSoundSourceHandle::SDLSoundSourceHandle(
	const AudioPlaybackConfig& config,
	PositionGetter positionGetter,
	int channel)
	: id(nextId++), config(config), positionGetter(positionGetter), channel(channel)
{
	if (channel != -1) {
		reserved = true;
	}
}

SDLSoundSourceHandle::~SDLSoundSourceHandle()
{
	SDLSoundSourceHandle::Stop();
}

uint64_t SDLSoundSourceHandle::GetID() const
{
	return id;
}

bool SDLSoundSourceHandle::Enqueue(Holder<SoundBufferHandle> handle)
{
	auto sdlHandle = std::dynamic_pointer_cast<SDLSoundBufferHandle>(handle);
	if (!sdlHandle) {
		return false;
	}

	auto chunk = sdlHandle->GetChunk();
	Mix_VolumeChunk(chunk, MIX_MAX_VOLUME * config.channelVolume / 100);

	if (!reserved && !Mix_Playing(channel) && CanOperateOnChannel()) {
		channel = -1;
	}

	auto nextChannel = ChannelManager::Request(*this, channel, chunk, config.loop);
	if (nextChannel == -1 && !reserved) {
		Log(ERROR, "SDLAudio", "Error playing channel! {}", Mix_GetError());
		return false;
	} else if (!reserved) {
		channel = nextChannel;
	}

	ConfigChannel();
	sdlHandle->AssignChannel(channel);

	return true;
}

bool SDLSoundSourceHandle::CanOperateOnChannel() const
{
	return reserved || (channel != -1 && ChannelManager::IsMyChannel(*this, channel));
}

bool SDLSoundSourceHandle::HasFinishedPlaying() const
{
	if (CanOperateOnChannel()) {
		return Mix_Playing(channel) == 0;
	}

	return true;
}

void SDLSoundSourceHandle::Reconfigure(const AudioPlaybackConfig& newConfig)
{
	config = newConfig;
	ConfigChannel();
}

void SDLSoundSourceHandle::ConfigChannel() const
{
	if (CanOperateOnChannel()) {
		Mix_Volume(channel, MIX_MAX_VOLUME * config.masterVolume / 100);
		if (config.spatial) {
			SetChannelPosition(positionGetter(), config, channel);
		}
	}
}

void SDLSoundSourceHandle::Stop()
{
	if (CanOperateOnChannel()) {
		Mix_HaltChannel(channel);
	}
}

void SDLSoundSourceHandle::StopLooping()
{
	if (CanOperateOnChannel()) {
		Mix_FadeOutChannel(channel, 1000);
		if (!reserved) {
			channel = -1;
		}
	}
}

void SDLSoundSourceHandle::SetPosition(const AudioPoint& position)
{
	config.position = position;
	if (CanOperateOnChannel() && config.spatial) {
		SetChannelPosition(positionGetter(), config, channel);
	}
}

void SDLSoundSourceHandle::SetVolume(int volume)
{
	if (CanOperateOnChannel()) {
		Mix_Volume(channel, MIX_MAX_VOLUME * volume / 100);
	}
}

std::mutex SDLSoundStreamSourceHandle::mixMutex = {};

SDLSoundStreamSourceHandle::SDLSoundStreamSourceHandle(size_t ringBufferSize)
	: ringBuffer { ringBufferSize }
{
	SDLSoundStreamSourceHandle::Reclaim();
}

SDLSoundStreamSourceHandle::~SDLSoundStreamSourceHandle()
{
	SDLSoundStreamSourceHandle::Stop();
}

bool SDLSoundStreamSourceHandle::Feed(const AudioBufferFormat&, const char* memory, size_t length)
{
	std::vector<char> convertBuffer(length, 0);

	// We can't use `Mix_VolumeMusic` since we don't use Mix to play files, and thus it internally
	// never reaches a playing-state to care about volume calls.
	if (volume == MIX_MAX_VOLUME) {
		std::copy(memory, memory + length, convertBuffer.begin());
	} else {
#if SDL_VERSION_ATLEAST(2, 0, 0)
		SDL_MixAudioFormat(reinterpret_cast<Uint8*>(convertBuffer.data()), reinterpret_cast<const Uint8*>(memory),
				   MIX_DEFAULT_FORMAT, length, volume);
#else
		SDL_MixAudio(reinterpret_cast<Uint8*>(convertBuffer.data()), reinterpret_cast<const Uint8*>(memory), length, volume);
#endif
	}

	SDL_AudioCVT cvt;
	SDL_BuildAudioCVT(&cvt,
			  AUDIO_S16SYS, 2, 22050,
			  SDLAudioBackend::audioFormat, SDLAudioBackend::audioChannels, SDLAudioBackend::audioRate);

	convertBuffer.resize(convertBuffer.size() * cvt.len_mult);
	cvt.buf = reinterpret_cast<Uint8*>(convertBuffer.data());
	cvt.len = length;
	SDL_ConvertAudio(&cvt);
	convertBuffer.resize(length * cvt.len_ratio);

	size_t offset = 0;
	do {
		auto written = ringBuffer.Fill(convertBuffer.data() + offset, convertBuffer.size() - offset);
		offset += written;

		if (offset < convertBuffer.size()) {
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
	} while (offset < convertBuffer.size() && fillWait);

	return fillWait;
}

bool SDLSoundStreamSourceHandle::HasProcessed()
{
	return true;
}

void SDLSoundStreamSourceHandle::Pause()
{
	Mix_PauseMusic();
}

void SDLSoundStreamSourceHandle::Resume()
{
	Mix_ResumeMusic();
}

void SDLSoundStreamSourceHandle::Reclaim()
{
	std::lock_guard<std::mutex> lock { mixMutex };

	fillWait = true;
	// The hook call isn't guarded by Mix' own locking
	{
		std::lock_guard<std::mutex> cbLock { MixCallbackState::mutex };
		if (MixCallbackState::handle != this) {
			MixCallbackState::handle = this;
			ringBuffer.Reset();
		}
	}
	Mix_HookMusic((void (*)(void*, Uint8*, int)) StreamCallback, nullptr);
}

void SDLSoundStreamSourceHandle::Stop()
{
	fillWait = false;
	{
		std::lock_guard<std::mutex> lock { MixCallbackState::mutex };
		MixCallbackState::handle = nullptr;
	}
	Mix_HaltMusic();
}

void SDLSoundStreamSourceHandle::SetVolume(int value)
{
	volume = MIX_MAX_VOLUME * value / 100;
}

void SDLSoundStreamSourceHandle::StreamCallback(const void*, uint8_t* dest, int len)
{
	std::lock_guard<std::mutex> lock { MixCallbackState::mutex };
	SDLSoundStreamSourceHandle* handle = MixCallbackState::handle;
	if (!handle) {
		return;
	}

	auto length = static_cast<size_t>(len);
	auto read = handle->ringBuffer.Consume(reinterpret_cast<char*>(dest), length);

	if (read < length) {
		std::fill(dest + read, dest + length, 0);
	}
}

SDLAudioBackend::~SDLAudioBackend()
{
	Mix_HaltChannel(-1);
	Mix_ChannelFinished(nullptr);
	Mix_HaltMusic();

	Mix_CloseAudio();
	SDL_QuitSubSystem(SDL_INIT_AUDIO);
}

int SDLAudioBackend::audioRate = 0;
unsigned short SDLAudioBackend::audioFormat = 0;
int SDLAudioBackend::audioChannels = 0;

bool SDLAudioBackend::Init()
{
	if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0) {
		Log(ERROR, "SDLAudio", "InitSubSystem failed: {}", SDL_GetError());
		return false;
	}
	if (Mix_OpenAudio(22050, MIX_DEFAULT_FORMAT, 2, 1024) < 0) {
		return false;
	}

	ChannelManager::Init(24, RESERVED_CHANNELS);
	Mix_QuerySpec(&audioRate, &audioFormat, &audioChannels);

	return true;
}

Holder<SoundSourceHandle> SDLAudioBackend::CreatePlaybackSource(const AudioPlaybackConfig& config, bool priority)
{
	Housekeeping();

	if (priority && reservedCounter == RESERVED_CHANNELS) {
		Log(ERROR, "SDLAudio", "Cannot issue any more reserved channels.");
		return {};
	}

	auto channel = priority ? reservedCounter++ : -1;
	auto handle =
		MakeHolder<SDLSoundSourceHandle>(
			config,
			[this]() -> const AudioPoint& { return this->GetListenerPosition(); },
			channel);
	issuedChannels.emplace_back(handle);

	return handle;
}

Holder<SoundStreamSourceHandle> SDLAudioBackend::CreateStreamable(const AudioPlaybackConfig&)
{
	return MakeHolder<SDLSoundStreamSourceHandle>(audioChannels * RING_BUFFER_CHANNEL_SIZE);
}

Holder<SoundBufferHandle> SDLAudioBackend::LoadSound(ResourceHolder<SoundMgr> resource, const AudioPlaybackConfig&)
{
	Mix_Chunk* chunk = nullptr;

	auto numSamples = resource->GetNumSamples();
	auto riffChans = resource->GetChannels();
	auto sampleRate = resource->GetSampleRate();

	std::vector<char> buffer {};
	buffer.resize(numSamples * 2);
	auto actualSamples = resource->read_samples(reinterpret_cast<short*>(buffer.data()), numSamples);
	if (actualSamples == 0) {
		return {};
	}

	SDL_AudioCVT cvt;
	SDL_BuildAudioCVT(&cvt, AUDIO_S16SYS, riffChans, sampleRate, audioFormat, audioChannels, audioRate);

	buffer.resize(actualSamples * 2 * cvt.len_mult);
	cvt.buf = reinterpret_cast<Uint8*>(buffer.data());
	cvt.len = actualSamples * 2;
	SDL_ConvertAudio(&cvt);

	buffer.resize(cvt.len * cvt.len_ratio);
	chunk = Mix_QuickLoad_RAW(reinterpret_cast<Uint8*>(buffer.data()), cvt.len * cvt.len_ratio);
	if (!chunk) {
		Log(ERROR, "SDLAudio", "Error loading chunk!");
		return {};
	}

	return MakeHolder<SDLSoundBufferHandle>(chunk, std::move(buffer));
}

const AudioPoint& SDLAudioBackend::GetListenerPosition() const
{
	return listenerPosition;
}

void SDLAudioBackend::SetListenerPosition(const AudioPoint& p)
{
	listenerPosition = p;

	for (auto channelPtr : issuedChannels) {
		if (auto channel = channelPtr.lock()) {
			channel->ConfigChannel();
		}
	}
}

void SDLAudioBackend::Housekeeping()
{
	for (auto it = issuedChannels.begin(); it != issuedChannels.end();) {
		if (!it->lock()) {
			it = issuedChannels.erase(it);
		} else {
			it++;
		}
	}
}

}

#include "plugindef.h"

GEMRB_PLUGIN(0x52C524E, "SDL Audio Driver")
PLUGIN_DRIVER(SDLAudioBackend, "SDLAudio")
END_PLUGIN()
