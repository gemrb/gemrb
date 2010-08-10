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
#include "GameData.h"

#include "AmbientMgr.h"
#include "SoundMgr.h"

#include "SDL.h"
#include "SDL_mixer.h"

#include "Interface.h" // GetMusicMgr()
#include "MusicMgr.h"

SDLAudio::SDLAudio(void)
{
	XPos = 0;
	YPos = 0;
	ambim = new AmbientMgr();
}

SDLAudio::~SDLAudio(void)
{
	// TODO
	delete ambim;
}

bool SDLAudio::Init(void)
{
	// TODO: we assume SDLVideo already got loaded
	if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0) {
		return false;
	}
	if (Mix_OpenAudio(22050, MIX_DEFAULT_FORMAT, 2, 4096) < 0) {
		return false;
	}
	Mix_ReserveChannels(1); // for speech
	return true;
}

void SDLAudio::music_callback(void *udata, unsigned short *stream, int len) {
	SDLAudio *driver = (SDLAudio *)udata;
	// TODO: conversion? mutexes? sanity checks? :)
	int num_samples = len / 2;
	int cnt = driver->MusicReader->read_samples(( short* ) stream, num_samples);
	if (cnt != num_samples) {
		// TODO: this shouldn't be in the callback (see also the openal thread)
		printMessage("SDLAudio", "Playing Next Music\n", WHITE );
		core->GetMusicMgr()->PlayNext();

		stream = stream + (cnt * 2);
		len = len - (cnt * 2);

		if (!driver->MusicReader) {
			printMessage( "SDLAudio", "No Other Music to play\n", WHITE );
			memset(stream, 0, len);
			Mix_HookMusic(NULL, NULL);
			return;
		}

		music_callback(udata, stream, len);
	}
}

unsigned int SDLAudio::Play(const char* ResRef, int XPos, int YPos, unsigned int flags)
{
	// TODO: some panning
	(void)XPos;
	(void)YPos;

	// TODO: flags
	(void)flags;

	if (!ResRef) {
		if (flags & GEM_SND_SPEECH) {
			Mix_HaltChannel(0);
		}
		return 0;
	}

	// TODO: move this loading code somewhere central
	ResourceHolder<SoundMgr> acm(ResRef);
	if (!acm) {
		printf("failed acm load\n");
		return 0;
	}
	int cnt = acm->get_length();
	int riff_chans = acm->get_channels();
	int samplerate = acm->get_samplerate();
	//multiply always by 2 because it is in 16 bits
	int rawsize = cnt * 2;
	unsigned char * memory = (unsigned char*) malloc(rawsize);
	//multiply always with 2 because it is in 16 bits
	int cnt1 = acm->read_samples( ( short* ) memory, cnt ) * 2;
	//Sound Length in milliseconds
	unsigned int time_length = ((cnt / riff_chans) * 1000) / samplerate;

	// work out which rate we need
	int audio_rate;
	Uint16 audio_format;
	int audio_channels;
	Mix_QuerySpec(&audio_rate, &audio_format, &audio_channels);

	// convert our buffer, if necessary
	SDL_AudioCVT cvt;
	SDL_BuildAudioCVT(&cvt, AUDIO_S16SYS, riff_chans, samplerate,
			audio_format, audio_channels, audio_rate);
	cvt.buf = (Uint8*)malloc(cnt1*cvt.len_mult);
	memcpy(cvt.buf, memory, cnt1);
	cvt.len = cnt1;
	SDL_ConvertAudio(&cvt);

	// free old buffer
	free(memory);

	// make SDL_mixer chunk
	Mix_Chunk *chunk = Mix_QuickLoad_RAW(cvt.buf, cvt.len*cvt.len_ratio);
	if (!chunk) {
		printf("error loading chunk\n");
		return 0;
	}

	// play
	int channel = -1;
	if (flags & GEM_SND_SPEECH) {
		channel = 0;
	}
	channel = Mix_PlayChannel(channel, chunk, 0);
	if (channel < 0) {
		printf("error playing channel\n");
		return 0;
	}

	// TODO: keep track of the chunk, don't just leak!
	// free(cvt.buf); // TODO: safe?

	return time_length;
}

int SDLAudio::CreateStream(Holder<SoundMgr> newMusic)
{
	printf("SDLAudio setting new music\n");
	MusicReader = newMusic;

	// TODO
	return 0;
}

bool SDLAudio::Stop()
{
	// TODO
	Mix_HookMusic(NULL, NULL);
	return true;
}

bool SDLAudio::Play()
{
	Mix_HookMusic((void (*)(void*, Uint8*, int))music_callback, this);
	// TODO
	return true;
}

void SDLAudio::ResetMusics()
{
	// TODO
	Mix_HookMusic(NULL, NULL);
}

bool SDLAudio::CanPlay()
{
	// TODO
	return false;
}

bool SDLAudio::IsSpeaking()
{
	return Mix_Playing(0);
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

int SDLAudio::SetupNewStream(ieWord, ieWord, ieWord, ieWord, bool, bool)
{
	// TODO
	return -1;
}

int SDLAudio::QueueAmbient(int, const char*)
{
	// TODO
	return -1;
}

bool SDLAudio::ReleaseStream(int, bool)
{
	// TODO
	return true;
}

void SDLAudio::SetAmbientStreamVolume(int, int)
{
	// TODO
}

void SDLAudio::QueueBuffer(int, unsigned short, int, short*, int, int)
{
	// TODO
}



#include "plugindef.h"

GEMRB_PLUGIN(0x52C524E, "SDL Audio Driver")
PLUGIN_DRIVER(SDLAudio, "SDLAudio")
END_PLUGIN()
