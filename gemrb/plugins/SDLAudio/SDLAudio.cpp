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

#include "win32def.h"

#include "AmbientMgr.h"
#include "GameData.h"
#include "Interface.h" // GetMusicMgr()
#include "MusicMgr.h"
#include "SoundMgr.h"

#include <SDL.h>
#include <SDL_mixer.h>

using namespace GemRB;

SDLAudio::SDLAudio(void)
{
	XPos = 0;
	YPos = 0;
	ambim = new AmbientMgr();
	MusicPlaying = false;
	OurMutex = NULL;
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
	SDL_DestroyMutex(OurMutex);
	Mix_ChannelFinished(NULL);
}

bool SDLAudio::Init(void)
{
	// TODO: we assume SDLVideo already got loaded
	if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0) {
		return false;
	}
	OurMutex = SDL_CreateMutex();
#ifdef RPI
	if (Mix_OpenAudio(22050, MIX_DEFAULT_FORMAT, 2, 512) < 0) {
#else
	if (Mix_OpenAudio(22050, MIX_DEFAULT_FORMAT, 2, 8192) < 0) {
#endif
		return false;
	}
	Mix_QuerySpec(&audio_rate, (Uint16 *)&audio_format, &audio_channels);

	Mix_ReserveChannels(1); // for speech
	return true;
}

void SDLAudio::music_callback(void *udata, unsigned short *stream, int len) {
	SDLAudio *driver = (SDLAudio *)udata;
	SDL_mutexP(driver->OurMutex);

	do {

		// TODO: conversion? mutexes? sanity checks? :)
		int num_samples = len / 2;
		int cnt = driver->MusicReader->read_samples(( short* ) stream, num_samples);

		// Done?
		if (cnt == num_samples)
			break;

		// TODO: this shouldn't be in the callback (see also the openal thread)
		Log(MESSAGE, "SDLAudio", "Playing Next Music");
		core->GetMusicMgr()->PlayNext();

		stream = stream + cnt;
		len = len - (cnt * 2);

		if (!driver->MusicPlaying) {
			Log(MESSAGE, "SDLAudio", "No Other Music to play");
			memset(stream, 0, len);
			Mix_HookMusic(NULL, NULL);
			break;
		}

	} while(true);

	SDL_mutexV(driver->OurMutex);
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

	SDL_LockAudio();

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

	SDL_UnlockAudio();

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

	// TODO: some panning
	(void)XPos;
	(void)YPos;

	// TODO: flags
	(void)flags;

	if (!ResRef) {
		if (flags & GEM_SND_SPEECH) {
			Mix_HaltChannel(0);
		}
		return Holder<SoundHandle>();
	}

	chunk = loadSound(ResRef, time_length);
	if (chunk == nullptr) {
		return Holder<SoundHandle>();
	}

	if (length) {
		*length = time_length;
	}

	Mix_VolumeChunk(chunk, MIX_MAX_VOLUME * GetVolume(channel) / 100);

	// play
	int chan = -1;
	if (flags & GEM_SND_SPEECH) {
		chan = 0;
	}
#ifndef VITA
	SDL_mutexP(OurMutex);
#endif
	chan = Mix_PlayChannel(chan, chunk, 0);
	if (chan < 0) {
#ifndef VITA
		SDL_mutexV(OurMutex);
#endif
		print("error playing channel");
		return Holder<SoundHandle>();
	}
#ifndef VITA
	SDL_mutexV(OurMutex);
#endif

	// TODO
	return Holder<SoundHandle>();
}

int SDLAudio::CreateStream(Holder<SoundMgr> newMusic)
{
	print("SDLAudio setting new music");
	MusicReader = newMusic;

	// TODO
	return 0;
}

bool SDLAudio::Stop()
{
	// TODO
	MusicPlaying = false;
	Mix_HookMusic(NULL, NULL);
	return true;
}

bool SDLAudio::Play()
{
	MusicPlaying = true;
	Mix_HookMusic((void (*)(void*, Uint8*, int))music_callback, this);
	// TODO
	return true;
}

void SDLAudio::ResetMusics()
{
	// TODO
	MusicPlaying = false;
	Mix_HookMusic(NULL, NULL);
}

bool SDLAudio::CanPlay()
{
	return true;
}

void SDLAudio::UpdateListenerPos(int x, int y)
{
	// TODO
	XPos = x;
	YPos = y;
}

void SDLAudio::GetListenerPos(int& x, int& y)
{
	// TODO
	x = XPos;
	y = YPos;
}

void SDLAudio::buffer_callback(void *udata, char *stream, int len) {
	SDLAudio *driver = (SDLAudio *)udata;
	SDL_mutexP(driver->OurMutex);
	unsigned int remaining = len;
	while (remaining && driver->buffers.size() > 0) {
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
	SDL_mutexV(driver->OurMutex);
}

int SDLAudio::SetupNewStream(ieWord x, ieWord y, ieWord z,
			ieWord gain, bool point, int ambientRange)
{
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
	SDL_mutexP(OurMutex);
	for (unsigned int i = 0; i < buffers.size(); i++) {
		free(buffers[i].buf);
	}
	buffers.clear();
	SDL_mutexV(OurMutex);
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

	SDL_mutexP(OurMutex);
	buffers.push_back(d);
	SDL_mutexV(OurMutex);
}

#include "plugindef.h"

GEMRB_PLUGIN(0x52C524E, "SDL Audio Driver")
PLUGIN_DRIVER(SDLAudio, "SDLAudio")
END_PLUGIN()
