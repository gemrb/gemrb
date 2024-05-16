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

#define AMBIENT_CHANNELS 8
#define MIXER_CHANNELS 16
#define BUFFER_CACHE_SIZE 100
#define AUDIO_DISTANCE_ROLLOFF_MOD 1.3
#define AMBIENT_DISTANCE_ROLLOFF_MOD 5

namespace GemRB {

class SDLAudioSoundHandle : public SoundHandle 
{
public:
	SDLAudioSoundHandle(Mix_Chunk* chunk, SFXChannel channel, bool relative)
		: mixChunk(chunk), chunkChannel(int(channel)), sndRelative(relative) {};
	void SetPos(const Point&) final;
	bool Playing() final;
	void Stop() final;
	void StopLooping() final;
	void Invalidate() const {}

private:
	Mix_Chunk *mixChunk;
	int chunkChannel;
	bool sndRelative;
};

struct BufferedData {
	char *buf;
	unsigned int size;
};

struct SDLAudioStream {
	bool free = true;
	bool point = false;
	Point streamPos;
};

struct CacheEntry {
	Mix_Chunk *chunk;
	unsigned int Length;

	CacheEntry(Mix_Chunk *chunk, tick_t length) : chunk(chunk), Length(length) {}
	CacheEntry(const CacheEntry&) = delete;
	CacheEntry(CacheEntry && other) : chunk(other.chunk), Length(other.Length) {
		other.chunk = nullptr;
	}
	CacheEntry& operator=(const CacheEntry&) = delete;
	CacheEntry& operator=(CacheEntry && other) {
		this->chunk = other.chunk;
		other.chunk = nullptr;
		this->Length = other.Length;

		return *this;
	}

	void evictionNotice() const {}

	~CacheEntry() {
		if (chunk != nullptr) {
			free(chunk->abuf);
			free(chunk);
		}
	}
};

struct SDLAudioPlaying {
	bool operator()(const CacheEntry& entry) const
	{
		int numChannels = Mix_AllocateChannels(-1);

		for (int i = 0; i < numChannels; ++i) {
			if (Mix_Playing(i) && Mix_GetChunk(i) == entry.chunk) {
				return false;
			}
		}

		return true;
	}
};

class SDLAudio : public Audio {
public:
	SDLAudio(void);
	~SDLAudio(void) override;
	bool Init(void) override;
	Holder<SoundHandle> Play(StringView ResRef, SFXChannel channel,
				 const Point&, unsigned int flags = 0, tick_t* length = nullptr) override;
	int CreateStream(ResourceHolder<SoundMgr>) override;
	bool Play() override;
	bool Stop() override;
	bool Pause() override;
	bool Resume() override;
	bool CanPlay() override;
	void ResetMusics() final;
	void UpdateListenerPos(const Point&) override;
	Point GetListenerPos() override;
	void UpdateVolume(unsigned int flags) override;

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
	Mix_Chunk* loadSound(StringView ResRef, tick_t &time_length);

	Point listenerPos;
	ResourceHolder<SoundMgr> MusicReader;

	bool MusicPlaying = false;
	unsigned int curr_buffer_offset = 0;
	std::vector<BufferedData> buffers;

	int audio_rate = 0;
	unsigned short audio_format = 0;
	int audio_channels = 0;

	std::recursive_mutex MusicMutex;
	LRUCache<CacheEntry, SDLAudioPlaying> buffercache;
	SDLAudioStream ambientStreams[AMBIENT_CHANNELS];
};

}

#endif
