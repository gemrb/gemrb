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

namespace GemRB {

class SDLAudioSoundHandle : public SoundHandle 
{
public:
	SDLAudioSoundHandle(Mix_Chunk *chunk, int channel, bool relative) : mixChunk(chunk), chunkChannel(channel), sndRelative(relative) { };
	virtual void SetPos(const Point&);
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
	~SDLAudio(void) override;
	bool Init(void) override;
	Holder<SoundHandle> Play(const char* ResRef, unsigned int channel,
		const Point&, unsigned int flags = 0, tick_t *length = 0) override;
	int CreateStream(std::shared_ptr<SoundMgr>) override;
	bool Play() override;
	bool Stop() override;
	bool Pause() override { return true; } /*not implemented*/
	bool Resume() override { return true; } /*not implemented*/
	bool CanPlay() override;
	void ResetMusics() override;
	void UpdateListenerPos(const Point&) override;
	Point GetListenerPos() override;
	void UpdateVolume(unsigned int) override {}

	int SetupNewStream(int x, int y, int z, ieWord gain, bool point, int ambientRange) override;
	tick_t QueueAmbient(int stream, const ResRef& sound) override;
	bool ReleaseStream(int stream, bool hardstop) override;
	void SetAmbientStreamVolume(int stream, int gain) override;
	void SetAmbientStreamPitch(int stream, int pitch) override;
	void QueueBuffer(int stream, unsigned short bits, int channels,
				short* memory, int size, int samplerate) override;

private:
	void FreeBuffers();

	static void SetAudioStreamVolume(uint8_t *stream, int len, int volume);
	static void music_callback(void *udata, uint8_t *stream, int len);
	static void buffer_callback(void *udata, uint8_t *stream, int len);
	bool evictBuffer();
	void clearBufferCache();
	Mix_Chunk* loadSound(const char *ResRef, tick_t &time_length);

	Point listenerPos;
	std::shared_ptr<SoundMgr> MusicReader;

	bool MusicPlaying = false;
	unsigned int curr_buffer_offset = 0;
	std::vector<BufferedData> buffers;

	int audio_rate = 0;
	unsigned short audio_format = 0;
	int audio_channels = 0;

	std::recursive_mutex MusicMutex;
	LRUCache buffercache;
};

}

#endif
