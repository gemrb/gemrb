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

#ifndef SDLAUDIO_H
#define SDLAUDIO_H

#include "Audio.h"
#include "LRUCache.h"

#include <mutex>
#include <vector>

#include <SDL_mixer.h>

#define MIXER_CHANNELS 16
#define BUFFER_CACHE_SIZE 100
#define AUDIO_DISTANCE_ROLLOFF_MOD 1.3

namespace GemRB {

class SDLAudioSoundHandle : public SoundHandle 
{
public:
	SDLAudioSoundHandle(Mix_Chunk *chunk, int channel, bool relative) : mixChunk(chunk), chunkChannel(channel), sndRelative(relative) { };
	virtual ~SDLAudioSoundHandle() { }
	virtual void SetPos(int XPos, int YPos);
	virtual bool Playing();
	virtual void Stop();
	virtual void StopLooping();
	void Invalidate() { }
private:
	Mix_Chunk *mixChunk;
	int chunkChannel;
	bool sndRelative;
};

struct BufferedData {
	char *buf;
	unsigned int size;
};

struct CacheEntry {
	Mix_Chunk *chunk;
	unsigned int Length;
};

class SDLAudio : public Audio {
public:
	SDLAudio(void);
	~SDLAudio(void);
	bool Init(void);
	Holder<SoundHandle> Play(const char* ResRef, unsigned int channel,
		int XPos, int YPos, unsigned int flags = 0, unsigned int *length = 0);
	int CreateStream(Holder<SoundMgr>);
	bool Play();
	bool Stop();
	bool Pause() { return true; } /*not implemented*/
	bool Resume() { return true; } /*not implemented*/
	bool CanPlay();
	void ResetMusics();
	void UpdateListenerPos(int XPos, int YPos);
	void GetListenerPos(int& XPos, int& YPos);
	void UpdateVolume(unsigned int) {}

	int SetupNewStream(ieWord x, ieWord y, ieWord z, ieWord gain, bool point, int ambientRange);
	int QueueAmbient(int stream, const char* sound);
	bool ReleaseStream(int stream, bool hardstop);
	void SetAmbientStreamVolume(int stream, int gain);
	void SetAmbientStreamPitch(int stream, int pitch);
	void QueueBuffer(int stream, unsigned short bits, int channels,
				short* memory, int size, int samplerate);

private:
	void FreeBuffers();

	static void SetAudioStreamVolume(uint8_t *stream, int len, int volume);
	static void music_callback(void *udata, uint8_t *stream, int len);
	static void buffer_callback(void *udata, uint8_t *stream, int len);
	bool evictBuffer();
	void clearBufferCache();
	Mix_Chunk* loadSound(const char *ResRef, unsigned int &time_length);

	Point listenerPos;
	Holder<SoundMgr> MusicReader;

	bool MusicPlaying;
	unsigned int curr_buffer_offset;
	std::vector<BufferedData> buffers;

	int audio_rate;
	unsigned short audio_format;
	int audio_channels;

	std::recursive_mutex MusicMutex;
	LRUCache buffercache;
};

}

#endif
