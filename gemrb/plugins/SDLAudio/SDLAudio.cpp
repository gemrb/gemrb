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

#include "AmbientMgr.h"
#include "GameData.h"
#include "Interface.h" // GetMusicMgr()
#include "MusicMgr.h"
#include "SoundMgr.h"

#include "Logging/Logging.h"

#include <SDL.h>
#include <SDL_mixer.h>

using namespace GemRB;

static void SetChannelPosition(const Point& listenerPos, const Point& p, int channel, float rolloff)
{
	int16_t angle = AngleFromPoints(listenerPos, p) * 180 / M_PI - 90;
	if (angle < 0) {
		angle += 360;
	}

	uint8_t distance = std::min(static_cast<int32_t>(Distance(listenerPos, p) / rolloff), 255);
	Mix_SetPosition(channel, angle, distance);
}

void SDLAudioSoundHandle::SetPos(const Point& p)
{
	if (sndRelative)
		return;

	Point pos = core->GetAudioDrv()->GetListenerPos();
	SetChannelPosition(pos, p, chunkChannel, AUDIO_DISTANCE_ROLLOFF_MOD);
}

bool SDLAudioSoundHandle::Playing()
{
	return (mixChunk && Mix_Playing(chunkChannel) && Mix_GetChunk(chunkChannel) == mixChunk);
}

void SDLAudioSoundHandle::Stop()
{
	// Mix_FadeOutChannel is not as aggressive sounding (especially when stopping spellcasting) as Mix_HaltChannel
	Mix_FadeOutChannel(chunkChannel, 500);
}

void SDLAudioSoundHandle::StopLooping()
{
	// No way to stop looping. Fading out instead..
	Mix_FadeOutChannel(chunkChannel, 1000);
}

SDLAudio::SDLAudio(void)
	: buffercache(BUFFER_CACHE_SIZE)
{
}

SDLAudio::~SDLAudio(void)
{
	// TODO
	Mix_HaltChannel(-1);
	delete ambim;
	Mix_HookMusic(NULL, NULL);
	FreeBuffers();
	Mix_ChannelFinished(NULL);

	SDL_QuitSubSystem(SDL_INIT_AUDIO);
}

bool SDLAudio::Init(void)
{
	if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0) {
		Log(ERROR, "SDLAudio", "InitSubSystem failed: {}", SDL_GetError());
		return false;
	}
	if (Mix_OpenAudio(22050, MIX_DEFAULT_FORMAT, 2, 512) < 0) {
		return false;
	}

	int result = Mix_AllocateChannels(MIXER_CHANNELS + AMBIENT_CHANNELS);
	if (result < 0) {
		Log(ERROR, "SDLAudio", "Unable to allocate mixing channels: {}\n", SDL_GetError());
		return false;
	}

	Mix_QuerySpec(&audio_rate, (Uint16*) &audio_format, &audio_channels);
	Mix_ReserveChannels(AMBIENT_CHANNELS + 1); // for speech and ambients
	ambim = new AmbientMgr();

	return true;
}

void SDLAudio::SetAudioStreamVolume(uint8_t* stream, int len, int volume)
{
	// since it's at max volume by default
	if (volume == MIX_MAX_VOLUME)
		return;
	uint8_t* mixData = new uint8_t[len];
	memcpy(mixData, stream, len * sizeof(uint8_t));
	memset(stream, 0, len); // mix audio data against silence
#if SDL_VERSION_ATLEAST(2, 0, 0)
	SDL_MixAudioFormat(stream, mixData, MIX_DEFAULT_FORMAT, len, MIX_MAX_VOLUME);
#else
	SDL_MixAudio(stream, mixData, len, MIX_MAX_VOLUME);
#endif
	delete[] mixData;
}

void SDLAudio::music_callback(void* udata, uint8_t* stream, int len)
{
	ieDword volume = core->GetDictionary().Get("Volume Music", 100);

	// No point of bothering if it's off anyway
	if (volume == 0) {
		return;
	}

	uint8_t* mixerStream = stream;
	int mixerLen = len;

	SDLAudio* driver = (SDLAudio*) udata;

	do {
		std::lock_guard<std::recursive_mutex> l(driver->MusicMutex);

		int num_samples = len / 2;
		int cnt = driver->MusicReader->read_samples((short*) stream, num_samples);

		// Done?
		if (cnt == num_samples)
			break;

		// TODO: this shouldn't be in the callback (see also the openal thread)
		Log(MESSAGE, "SDLAudio", "Playing Next Music");
		core->GetMusicMgr()->PlayNext();

		stream = stream + (cnt * 2);
		len = len - (cnt * 2);

		if (!driver->MusicPlaying) {
			Log(MESSAGE, "SDLAudio", "No Other Music to play");
			memset(stream, 0, len);
			Mix_HookMusic(NULL, NULL);
			break;
		}
	} while (true);

	SetAudioStreamVolume(mixerStream, mixerLen, MIX_MAX_VOLUME * volume / 100);
}

Mix_Chunk* SDLAudio::loadSound(StringView ResRef, tick_t& time_length)
{
	Mix_Chunk* chunk = nullptr;

	if (ResRef.empty()) {
		return chunk;
	}

	auto entry = buffercache.Lookup(ResRef);
	if (entry != nullptr) {
		time_length = entry->Length;
		return entry->chunk;
	}

	ResourceHolder<SoundMgr> acm = gamedata->GetResourceHolder<SoundMgr>(ResRef);
	if (!acm) {
		Log(ERROR, "SDLAudio", "Failed acm load!");
		return chunk;
	}
	int cnt = acm->get_length();
	int riff_chans = acm->get_channels();
	int samplerate = acm->get_samplerate();
	// Use 16-bit word for memory allocation because read_samples takes a 16 bit alignment
	short* memory = (short*) malloc(cnt * 2);
	//multiply always with 2 because it is in 16 bits
	int cnt1 = acm->read_samples(memory, cnt) * 2;
	//Sound Length in milliseconds
	time_length = ((cnt / riff_chans) * 1000) / samplerate;

	// convert our buffer, if necessary
	SDL_AudioCVT cvt;
	SDL_BuildAudioCVT(&cvt, AUDIO_S16SYS, riff_chans, samplerate,
			  audio_format, audio_channels, audio_rate);
	cvt.buf = (Uint8*) malloc(cnt1 * cvt.len_mult);
	memcpy(cvt.buf, (char*) memory, cnt1);
	cvt.len = cnt1;
	SDL_ConvertAudio(&cvt);

	// free old buffer
	free(memory);

	// make SDL_mixer chunk
	chunk = Mix_QuickLoad_RAW(cvt.buf, cvt.len * cvt.len_ratio);
	if (!chunk) {
		Log(ERROR, "SDLAudio", "Error loading chunk!");
		free(cvt.buf);
		return chunk;
	}

	buffercache.SetAt(ResRef, chunk, time_length);

	return chunk;
}

Holder<SoundHandle> SDLAudio::Play(StringView ResRef, SFXChannel channel,
				   const Point& p, unsigned int flags, tick_t* length)
{
	Mix_Chunk* chunk;

	if (ResRef.empty()) {
		if (flags & GEM_SND_SPEECH) {
			Mix_HaltChannel(0);
		}
		return Holder<SoundHandle>();
	}

	int chan = -1;
	int loop = (flags & GEM_SND_LOOPING) ? -1 : 0;
	ieDword volume = 100;

	if (flags & GEM_SND_SPEECH) {
		chan = 0;
		loop = 0; // Speech ignores GEM_SND_LOOPING
		volume = core->GetDictionary().Get("Volume Voices", 100);
	} else {
		volume = core->GetDictionary().Get("Volume SFX", 100);
	}

	if (volume == 0) {
		return Holder<SoundHandle>();
	}

	tick_t time_length;
	chunk = loadSound(ResRef, time_length);
	if (chunk == nullptr) {
		return Holder<SoundHandle>();
	}

	if (length) {
		*length = time_length;
	}

	Mix_VolumeChunk(chunk, MIX_MAX_VOLUME * GetVolume(channel) / 100);

	chan = Mix_PlayChannel(chan, chunk, loop);
	if (chan < 0) {
		Log(ERROR, "SDLAudio", "Error playing channel!");
		return Holder<SoundHandle>();
	}
	Mix_Volume(chan, MIX_MAX_VOLUME * volume / 100);

	if (flags & GEM_SND_SPATIAL) {
		SetChannelPosition(listenerPos, p, chan, AUDIO_DISTANCE_ROLLOFF_MOD);
	}

	// TODO: we need something like static_ptr_cast
	return Holder<SoundHandle>(new SDLAudioSoundHandle(chunk, SFXChannel(chan), flags));
}

bool SDLAudio::Pause()
{
	((AmbientMgr*) ambim)->Deactivate();
	return true;
}

bool SDLAudio::Resume()
{
	((AmbientMgr*) ambim)->Activate();
	return true;
}

int SDLAudio::CreateStream(ResourceHolder<SoundMgr> newMusic)
{
	std::lock_guard<std::recursive_mutex> l(MusicMutex);

	Log(MESSAGE, "SDLAudio", "SDLAudio setting new music");
	MusicReader = std::move(newMusic);

	return 0;
}

bool SDLAudio::Stop()
{
	MusicPlaying = false;
	Mix_HookMusic(NULL, NULL);
	return true;
}

bool SDLAudio::Play()
{
	std::lock_guard<std::recursive_mutex> l(MusicMutex);

	if (!MusicReader) {
		return false;
	}
	MusicPlaying = true;
	Mix_HookMusic((void (*)(void*, Uint8*, int)) music_callback, this);
	return true;
}

void SDLAudio::ResetMusics()
{
	MusicPlaying = false;
	Mix_HookMusic(NULL, NULL);
}

void SDLAudio::UpdateVolume(unsigned int flags)
{
	ieDword volume = 0;

	if (flags & GEM_SND_VOL_AMBIENTS) {
		volume = core->GetDictionary().Get("Volume Ambients", 0);
		((AmbientMgr*) ambim)->UpdateVolume(volume);
	}
}

bool SDLAudio::CanPlay()
{
	return true;
}

void SDLAudio::UpdateListenerPos(const Point& p)
{
	listenerPos = p;

	for (int i = 0; i < AMBIENT_CHANNELS; i++) {
		if (!ambientStreams[i].free && ambientStreams[i].point) {
			// channel i + 1, since ambient channels start at 1 (channel 0 is reserved for speech)
			SetChannelPosition(listenerPos, ambientStreams[i].streamPos, i + 1, AMBIENT_DISTANCE_ROLLOFF_MOD);
		}
	}
}

Point SDLAudio::GetListenerPos()
{
	return listenerPos;
}

void SDLAudio::buffer_callback(void* udata, uint8_t* stream, int len)
{
	// Check only movie volume, since ambiens aren't supported right now
	ieDword volume = core->GetDictionary().Get("Volume Movie", 100);

	// No point of bothering if it's off anyway
	if (volume == 0) {
		return;
	}

	uint8_t* mixerStream = stream;
	int mixerLen = len;

	SDLAudio* driver = (SDLAudio*) udata;
	unsigned int remaining = len;

	while (remaining && !driver->buffers.empty()) {
		std::lock_guard<std::recursive_mutex> l(driver->MusicMutex);

		unsigned int avail = driver->buffers[0].size - driver->curr_buffer_offset;
		if (avail > remaining) {
			// more data available in this buffer than we need
			avail = remaining;
			memcpy(stream, driver->buffers[0].buf + driver->curr_buffer_offset, avail);
			driver->curr_buffer_offset += avail;
		} else {
			// exhausted this buffer, move to the next one
			memcpy(stream, driver->buffers[0].buf + driver->curr_buffer_offset, avail);
			driver->curr_buffer_offset = 0;
			free(driver->buffers[0].buf);
			// TODO: inefficient
			driver->buffers.erase(driver->buffers.begin());
		}
		remaining -= avail;
		stream = stream + avail;
	}

	if (remaining > 0) {
		// underrun (out of buffers)
		memset(stream, 0, remaining);
	}
	SetAudioStreamVolume(mixerStream, mixerLen, MIX_MAX_VOLUME * volume / 100);
}

int SDLAudio::SetupNewStream(int x, int y, int z,
			     ieWord gain, bool point, int ambientRange)
{
	std::lock_guard<std::recursive_mutex> l(MusicMutex);

	if (ambientRange) {
		for (int i = 1; i <= AMBIENT_CHANNELS; i++) {
			if (ambientStreams[i - 1].free) {
				if (point) {
					Point ambientPos(x, y);
					SetChannelPosition(listenerPos, ambientPos, i, AMBIENT_DISTANCE_ROLLOFF_MOD);
				} else {
					Mix_SetPosition(i, 0, 0);
				}

				Mix_Volume(i, MIX_MAX_VOLUME * gain / 100);
				ambientStreams[i - 1].free = false;
				ambientStreams[i - 1].streamPos.x = x;
				ambientStreams[i - 1].streamPos.y = y;
				ambientStreams[i - 1].point = point;
				return i;
			}
		}
		return -1;
	}

	// TODO: maybe don't ignore these
	(void) z;

	Log(MESSAGE, "SDLAudio", "SDLAudio allocating stream...");

	// TODO: buggy
	MusicPlaying = false;
	curr_buffer_offset = 0;
	Mix_HookMusic((void (*)(void*, Uint8*, int)) buffer_callback, this);
	return 0;
}

tick_t SDLAudio::QueueAmbient(int stream, const ResRef& sound)
{
	if (stream <= 0 || stream > AMBIENT_CHANNELS) {
		return -1;
	}

	if (Mix_Playing(stream)) {
		Mix_HaltChannel(stream);
	}

	tick_t time_length;
	Mix_Chunk* chunk = loadSound(sound, time_length);

	if (chunk == nullptr) {
		return -1;
	}

	if (ambientStreams[stream - 1].point) {
		SetChannelPosition(listenerPos, ambientStreams[stream - 1].streamPos, stream, AMBIENT_DISTANCE_ROLLOFF_MOD);
	}
	Mix_PlayChannel(stream, chunk, 0);

	return time_length;
}

bool SDLAudio::ReleaseStream(int stream, bool HardStop)
{
	if (stream < 0) {
		return false;
	}

	if (stream > 0) {
		if (ambientStreams[stream - 1].free) {
			return false;
		}
		Mix_HaltChannel(stream);
		ambientStreams[stream - 1].free = true;
		return true;
	}

	Log(MESSAGE, "SDLAudio", "Releasing stream...");

	(void) HardStop;
	Stop();
	FreeBuffers();

	return true;
}

void SDLAudio::FreeBuffers()
{
	std::lock_guard<std::recursive_mutex> l(MusicMutex);
	for (const auto& buffer : buffers) {
		free(buffer.buf);
	}
	buffers.clear();
}

void SDLAudio::SetAmbientStreamVolume(int stream, int volume)
{
	Mix_Volume(stream, MIX_MAX_VOLUME * volume / 100);
}

void SDLAudio::SetAmbientStreamPitch(int, int)
{
	// TODO: ambient sounds
}

void SDLAudio::QueueBuffer(int stream, unsigned short bits,
			   int channels, short* memory, int size, int samplerate)
{
	if (stream != 0) {
		return;
	}

	assert(!MusicPlaying);

	BufferedData d;

	// convert our buffer, if necessary
	if (bits != 16 || channels != audio_channels || samplerate != audio_rate) {
		SDL_AudioCVT cvt;
		if (SDL_BuildAudioCVT(&cvt, (bits == 8 ? AUDIO_S8 : AUDIO_S16SYS), channels, samplerate,
				      audio_format, audio_channels, audio_rate) == 0) {
			Log(ERROR, "SDLAudio", "Couldn't convert video stream! trying to convert {} bits, {} channels, {} rate",
			    bits, channels, samplerate);
			return;
		}
		cvt.buf = (Uint8*) malloc(size * cvt.len_mult);
		memcpy(cvt.buf, memory, size);
		cvt.len = size;
		SDL_ConvertAudio(&cvt);

		d.size = cvt.len * cvt.len_ratio;
		d.buf = (char*) cvt.buf;
	} else {
		d.size = size;
		d.buf = (char*) malloc(d.size);
		memcpy(d.buf, memory, d.size);
	}

	MusicMutex.lock();
	buffers.push_back(d);
	MusicMutex.unlock();
}

#include "plugindef.h"

GEMRB_PLUGIN(0x52C524E, "SDL Audio Driver")
PLUGIN_DRIVER(SDLAudio, "SDLAudio")
END_PLUGIN()
