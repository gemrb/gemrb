/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003-2004 The GemRB Project
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

#ifndef OPENALAUDIO_H_INCLUDED
#define OPENALAUDIO_H_INCLUDED

#include "ie_types.h"

#include "AmbientMgr.h"
#include "Audio.h"
#include "LRUCache.h"
#include "MusicMgr.h"
#include "SoundMgr.h"

#include "Streams/FileStream.h"

#include <mutex>
#include <thread>
#include <utility>

#if __APPLE__
	#include <OpenAL/OpenAL.h> // umbrella include for all the headers we want
#else
	#include "al.h"
	#include "alc.h"
	#ifdef HAVE_OPENAL_EFX_H
		#include "efx.h"
	#endif
#endif

#define RETRY              5
#define BUFFER_CACHE_SIZE  100
#define MAX_STREAMS        30
#define MUSICBUFFERS       10
#define REFERENCE_DISTANCE 50
#define ACM_BUFFERSIZE     8192

#define LISTENER_HEIGHT 200.0f

namespace GemRB {

class OpenALSoundHandle : public SoundHandle {
protected:
	struct AudioStream* parent;

public:
	explicit OpenALSoundHandle(AudioStream* p)
		: parent(p) {}
	void SetPos(const Point&) override;
	bool Playing() override;
	void Stop() override;
	void StopLooping() override;
	void Invalidate() { parent = 0; }
};

using OpenALuintPair = std::pair<ALuint, ALuint>;

struct AudioStream {
	// Spatial stereo is laid out as two sources and buffers,
	// for every other case, just the first value is relevant.
	OpenALuintPair buffers = { 0, 0 };
	OpenALuintPair sources = { 0, 0 };
	int Duration = 0;
	bool free = true;
	bool ambient = false;
	bool locked = false;
	bool delete_buffers = false;

	void ClearIfStopped();
	bool ClearIfStopped(ALuint source);
	void ClearProcessedBuffers() const;
	void ClearProcessedBuffers(ALuint source) const;
	void ForceClear();
	void SetPitch(int pitch) const;
	void SetPos(const Point&) const;
	void SetVolume(int volume) const;
	void StopLooping() const;
	void Stop() const;
	void Stop(ALuint source) const;

	Holder<OpenALSoundHandle> handle;
};

struct CacheEntry {
	OpenALuintPair Buffer;
	tick_t Length;

	CacheEntry(OpenALuintPair buffer, tick_t length)
		: Buffer(buffer), Length(length) {}
	CacheEntry(const CacheEntry&) = delete;
	CacheEntry(CacheEntry&& other) noexcept
		: Buffer(other.Buffer), Length(other.Length)
	{
		other.Buffer = { 0, 0 };
	}
	CacheEntry& operator=(const CacheEntry&) = delete;
	CacheEntry& operator=(CacheEntry&& other) noexcept
	{
		this->Buffer = other.Buffer;
		other.Buffer = { 0, 0 };
		this->Length = other.Length;

		return *this;
	}

	void evictionNotice()
	{
		Buffer = { 0, 0 };
	}

	~CacheEntry()
	{
		alDeleteBuffers(1, &Buffer.first);
		if (Buffer.second != 0) {
			alDeleteBuffers(1, &Buffer.second);
		}
	}
};

struct OpenALPlaying {
	bool operator()(const CacheEntry& entry) const
	{
		alDeleteBuffers(1, &entry.Buffer.first);
		bool success = alGetError() == AL_NO_ERROR;

		if (success && entry.Buffer.second) {
			alDeleteBuffers(1, &entry.Buffer.second);
		}

		return success;
	}
};

class OpenALAudioDriver : public Audio {
public:
	OpenALAudioDriver(void);
	~OpenALAudioDriver(void) override;
	void PrintDeviceList() const;
	bool Init(void) override;
	Holder<SoundHandle> Play(StringView ResRef, SFXChannel channel,
				 const Point&, unsigned int flags = 0,
				 tick_t* length = nullptr) override;
	void UpdateVolume(unsigned int flags) override;
	bool CanPlay() override;
	void ResetMusics() final;
	bool Play() override;
	bool Stop() override;
	bool Pause() override;
	bool Resume() override;
	int CreateStream(ResourceHolder<SoundMgr>) override;
	void UpdateListenerPos(const Point&) override;
	Point GetListenerPos() override;
	bool ReleaseStream(int stream, bool HardStop) override;
	int SetupNewStream(int x, int y, int z,
			   ieWord gain, bool point, int ambientRange) override;
	tick_t QueueAmbient(int stream, const ResRef& sound, bool spatial) override;
	void SetAmbientStreamVolume(int stream, int volume) override;
	void SetAmbientStreamPitch(int stream, int pitch) override;
	void QueueBuffer(int stream, unsigned short bits,
			 int channels, short* memory,
			 int size, int samplerate) override;
	void UpdateMapAmbient(const MapReverbProperties&) override;

private:
	int QueueALBuffers(OpenALuintPair source, OpenALuintPair buffer) const;
	int QueueALBuffer(ALuint source, ALuint buffer) const;

private:
	ALCcontext* alutContext = nullptr;
	ALuint MusicSource = 0;
	bool MusicPlaying = false;
	std::recursive_mutex musicMutex;
	ALuint MusicBuffer[MUSICBUFFERS] {};
	ResourceHolder<SoundMgr> MusicReader;
	LRUCache<CacheEntry, OpenALPlaying> buffercache { BUFFER_CACHE_SIZE };
	AudioStream speech;
	AudioStream streams[MAX_STREAMS];
	int num_streams = 0;

	std::atomic_bool stayAlive { true };
	std::vector<short> musicBuffer;
	std::thread musicThread;

	bool hasReverbProperties = false;
	bool hasEFX = false;
	ALuint efxEffectSlot = 0;
	ALuint efxEffect = 0;
	MapReverbProperties reverbProperties;

	OpenALuintPair loadSound(StringView ResRef, tick_t& time_length, bool spatial = false);
	int CountAvailableSources(int limit);
	bool evictBuffer();
	void clearBufferCache(bool force);
	ALenum GetFormatEnum(int channels, int bits) const;
	static int MusicManager(void* args);

	bool InitEFX(void);
	ALuint CreateAndConfigSource(ALuint source, ieDword volume, ALint loop, unsigned int flags, const Point& p, SFXChannel channel) const;
	void ConfigSource(ALuint source, ieDword volume, ALint loop, unsigned int flags, const Point& p, SFXChannel channel) const;
};

}

#endif // OPENALAUDIO_H_INCLUDED
