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

#include <SDL.h>
#include <SDL_mixer.h>
#include <cmath>

using namespace GemRB;

static void SetChannelPosition(int listenerXPos, int listenerYPos, int XPos, int YPos, int channel)
{
	int x = listenerXPos - XPos;
	int y = listenerYPos - YPos;
	int16_t angle = atan2(y, x) * 180 / M_PI - 90;
	if (angle < 0) {
		angle += 360;
	}
	uint8_t distance = std::min(static_cast<int32_t>(sqrt(x * x + y * y) / AUDIO_DISTANCE_ROLLOFF_MOD), 255);
	Mix_SetPosition(channel, angle, distance);
}

void SDLAudioSoundHandle::SetPos(int XPos, int YPos)
{
	if (sndRelative)
		return;

	int listenerXPos = 0;
	int listenerYPos = 0;
	core->GetAudioDrv()->GetListenerPos(listenerXPos, listenerYPos);
	SetChannelPosition(listenerXPos, listenerYPos, XPos, YPos, chunkChannel);
}

bool SDLAudioSoundHandle::Playing()
{
	return (mixChunk && Mix_Playing(chunkChannel) && Mix_GetChunk(chunkChannel) == mixChunk);
}

void SDLAudioSoundHandle::Stop()
{
	// Mix_FadeOutChannel is not as agressive sounding (especially when stopping spellcasting) as Mix_HaltChannel
	Mix_FadeOutChannel(chunkChannel, 500);
}

void SDLAudioSoundHandle::StopLooping()
{
	// No way to stop looping. Fading out instead..
	Mix_FadeOutChannel(chunkChannel, 1000);
}

SDLAudio::SDLAudio(void)
{
	ambim = new AmbientMgr();
	MusicPlaying = false;
	curr_buffer_offset = 0;
	audio_rate = audio_format = audio_channels = 0;
}

SDLAudio::~SDLAudio(void)
{
	// TODO
	Mix_HaltChannel(-1);
	clearBufferCache();
	delete ambim;
	Mix_HookMusic(NULL, NULL);
	FreeBuffers();
	Mix_ChannelFinished(NULL);
}

bool SDLAudio::Init(void)
{
	// TODO: we assume SDLVideo already got loaded
	if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0) {
		return false;
	}
#ifdef RPI
	if (Mix_OpenAudio(22050, MIX_DEFAULT_FORMAT, 2, 512) < 0) {
#else
	if (Mix_OpenAudio(22050, MIX_DEFAULT_FORMAT, 2, 8192) < 0) {
#endif
		return false;
	}

	int result = Mix_AllocateChannels(MIXER_CHANNELS);
	if (result < 0) {
		Log(ERROR, "SDLAudio", "Unable to allocate mixing channels: %s\n", SDL_GetError());
		return false;
	}

	Mix_QuerySpec(&audio_rate, (Uint16 *)&audio_format, &audio_channels);

	Mix_ReserveChannels(1); // for speech
	return true;
}

void SDLAudio::SetAudioStreamVolume(uint8_t *stream, int len, int volume)
{
	// since it's at max volume by default
	if (volume == MIX_MAX_VOLUME)
		return;
	uint8_t *mixData = new uint8_t[len];
	memcpy(mixData, stream, len * sizeof(uint8_t));
	memset(stream, 0, len); // mix audio data against silence
	SDL_MixAudio(stream, mixData, len, volume);
	delete[] mixData;
}

void SDLAudio::music_callback(void *udata, uint8_t *stream, int len)
{
	ieDword volume = 100;
	core->GetDictionary()->Lookup("Volume Music", volume);

	// No point of bothering if it's off anyway
	if (volume == 0) {
		return;
	}

	uint8_t *mixerStream = stream;
	int mixerLen = len;

	SDLAudio *driver = (SDLAudio *)udata;

	do {
		std::lock_guard<std::recursive_mutex> l(driver->MusicMutex);

		int num_samples = len / 2;
		int cnt = driver->MusicReader->read_samples(( short* ) stream, num_samples);

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
	} while(true);

	SetAudioStreamVolume(mixerStream, mixerLen, MIX_MAX_VOLUME * volume / 100);
}

bool SDLAudio::evictBuffer()
{
	// Note: this function assumes the caller holds bufferMutex

	// Room for optimization: this is O(n^2) in the number of buffers
	// at the tail that are used. It can be O(n) if LRUCache supports it.
	unsigned int n = 0;
	void *p;
	const char *k;
	bool res;

	while ((res = buffercache.getLRU(n, k, p)) == true && buffercache.GetCount() >= BUFFER_CACHE_SIZE) {
		CacheEntry *e = (CacheEntry*)p;
		bool chunkPlaying = false;
		int numChannels = Mix_AllocateChannels(-1);

		for (int i = 0; i < numChannels; ++i) {
			if (Mix_Playing(i) && Mix_GetChunk(i) == e->chunk) {
				chunkPlaying = true;
				break;
			}
		}

		if (chunkPlaying) {
			++n;
		} else {		
			//Mix_FreeChunk(e->chunk) fails to free anything here
			free(e->chunk->abuf);
			free(e->chunk);
			delete e;
			buffercache.Remove(k);
		}
	}

	return res;
}

void SDLAudio::clearBufferCache()
{
	// Room for optimization: any method of iterating over the buffers
	// would suffice. It doesn't have to be in LRU-order.
	void *p;
	const char *k;
	int n = 0;
	while (buffercache.getLRU(n, k, p)) {
		CacheEntry *e = (CacheEntry*)p;
		free(e->chunk->abuf);
		free(e->chunk);
		delete e;
		buffercache.Remove(k);
	}
}

Mix_Chunk* SDLAudio::loadSound(const char *ResRef, unsigned int &time_length)
{
	Mix_Chunk *chunk = nullptr;
	CacheEntry *e;
	void *p;

	if (!ResRef[0]) {
		return chunk;
	}

	if (buffercache.Lookup(ResRef, p)) {
		e = (CacheEntry*) p;
		time_length = e->Length;
		return e->chunk;
	}

	ResourceHolder<SoundMgr> acm = GetResourceHolder<SoundMgr>(ResRef);
	if (!acm) {
		print("failed acm load");
		return chunk;
	}
	int cnt = acm->get_length();
	int riff_chans = acm->get_channels();
	int samplerate = acm->get_samplerate();
	// Use 16-bit word for memory allocation because read_samples takes a 16 bit alignment
	short *memory = (short*) malloc(cnt*2);
	//multiply always with 2 because it is in 16 bits
	int cnt1 = acm->read_samples( memory, cnt ) * 2;
	//Sound Length in milliseconds
	time_length = ((cnt / riff_chans) * 1000) / samplerate;

	// convert our buffer, if necessary
	SDL_AudioCVT cvt;
	SDL_BuildAudioCVT(&cvt, AUDIO_S16SYS, riff_chans, samplerate,
			audio_format, audio_channels, audio_rate);
	cvt.buf = (Uint8*)malloc(cnt1*cvt.len_mult);
	memcpy(cvt.buf, (char*)memory, cnt1);
	cvt.len = cnt1;
	SDL_ConvertAudio(&cvt);

	// free old buffer
	free(memory);

	// make SDL_mixer chunk
	chunk = Mix_QuickLoad_RAW(cvt.buf, cvt.len*cvt.len_ratio);
	if (!chunk) {
		print("error loading chunk");
		free(cvt.buf);
		return chunk;
	}

	e = new CacheEntry;
	e->chunk = chunk;
	e->Length = time_length;

	if (buffercache.GetCount() >= BUFFER_CACHE_SIZE) {
		evictBuffer();
	}

	buffercache.SetAt(ResRef, (void*)e);

	return chunk;
}

Holder<SoundHandle> SDLAudio::Play(const char* ResRef, unsigned int channel,
	int XPos, int YPos, unsigned int flags, unsigned int *length)
{
	Mix_Chunk *chunk;
	unsigned int time_length;

	if (!ResRef) {
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
		core->GetDictionary()->Lookup("Volume Voices", volume);
	} else {
		core->GetDictionary()->Lookup("Volume SFX", volume);
	}

	if (volume == 0) {
		return Holder<SoundHandle>();
	}

	chunk = loadSound(ResRef, time_length);
	if (chunk == nullptr) {
		return Holder<SoundHandle>();
	}

	if (length) {
		*length = time_length;
	}

	Mix_VolumeChunk(chunk, MIX_MAX_VOLUME * (GetVolume(channel) * volume / 10000.0f));

	chan = Mix_PlayChannel(chan, chunk, loop);
	if (chan < 0) {
		print("error playing channel");
		return Holder<SoundHandle>();
	}

	if (!(flags & GEM_SND_RELATIVE)) {
		SetChannelPosition(listenerPos.x, listenerPos.y, XPos, YPos, chan);
	}

	return new SDLAudioSoundHandle(chunk, chan, flags & GEM_SND_RELATIVE);
}

int SDLAudio::CreateStream(Holder<SoundMgr> newMusic)
{
	std::lock_guard<std::recursive_mutex> l(MusicMutex);

	print("SDLAudio setting new music");
	MusicReader = newMusic;

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
	Mix_HookMusic((void (*)(void*, Uint8*, int))music_callback, this);
	return true;
}

void SDLAudio::ResetMusics()
{
	MusicPlaying = false;
	Mix_HookMusic(NULL, NULL);
}

bool SDLAudio::CanPlay()
{
	return true;
}

void SDLAudio::UpdateListenerPos(int x, int y)
{
	listenerPos.x = x;
	listenerPos.y = y;
}

void SDLAudio::GetListenerPos(int& x, int& y)
{
	x = listenerPos.x;
	y = listenerPos.y;
}

void SDLAudio::buffer_callback(void *udata, uint8_t *stream, int len)
{
	ieDword volume = 100;
	// Check only movie volume, since ambiens aren't supported right now
	core->GetDictionary()->Lookup("Volume Movie", volume);

	// No point of bothering if it's off anyway
	if (volume == 0) {
		return;
	}

	uint8_t *mixerStream = stream;
	int mixerLen = len;

	SDLAudio *driver = (SDLAudio *)udata;
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

int SDLAudio::SetupNewStream(ieWord x, ieWord y, ieWord z,
			ieWord gain, bool point, int ambientRange)
{
	std::lock_guard<std::recursive_mutex> l(MusicMutex);

	if (ambientRange) {
		// TODO: ambient sounds
		return -1;
	}

	// TODO: maybe don't ignore these
	(void)x;
	(void)y;
	(void)z;
	(void)gain;
	(void)point;

	print("SDLAudio allocating stream");

	// TODO: buggy
	MusicPlaying = false;
	curr_buffer_offset = 0;
	Mix_HookMusic((void (*)(void*, Uint8*, int))buffer_callback, this);
	return 0;
}

int SDLAudio::QueueAmbient(int, const char*)
{
	// TODO: ambient sounds
	return -1;
}

bool SDLAudio::ReleaseStream(int stream, bool HardStop)
{
	if (stream != 0) {
		return false;
	}

	print("SDLAudio releasing stream");

	(void)HardStop;

	assert(!MusicPlaying);

	Mix_HookMusic(NULL, NULL);
	FreeBuffers();

	return true;
}

void SDLAudio::FreeBuffers()
{
	std::lock_guard<std::recursive_mutex> l(MusicMutex);
	for (unsigned int i = 0; i < buffers.size(); i++) {
		free(buffers[i].buf);
	}
	buffers.clear();
}

void SDLAudio::SetAmbientStreamVolume(int, int)
{
	// TODO: ambient sounds
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
			Log(ERROR, "SDLAudio", "Couldn't convert video stream! trying to convert %d bits, %d channels, %d rate",
				bits, channels, samplerate);
			return;
		}
		cvt.buf = (Uint8*)malloc(size*cvt.len_mult);
		memcpy(cvt.buf, memory, size);
		cvt.len = size;
		SDL_ConvertAudio(&cvt);

		d.size = cvt.len*cvt.len_ratio;
		d.buf = (char *)cvt.buf;
	} else {
		d.size = size;
		d.buf = (char *)malloc(d.size);
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
