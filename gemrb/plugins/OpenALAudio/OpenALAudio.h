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

#include "Audio.h"

#include "AmbientMgrAL.h"

#include "ie_types.h"

#include "LRUCache.h"
#include "MusicMgr.h"
#include "SoundMgr.h"
#include "System/FileStream.h"
#include "MapReverb.h"

#include <mutex>
#include <thread>

#if __APPLE__
#include <OpenAL/OpenAL.h> // umbrella include for all the headers we want
#else
#include "al.h"
#include "alc.h"
#ifdef HAVE_OPENAL_EFX_H
# include "efx.h"
#endif
#endif

#define RETRY 5
#define BUFFER_CACHE_SIZE 100
#define MAX_STREAMS 30
#define MUSICBUFFERS 10
#define REFERENCE_DISTANCE 50
#define ACM_BUFFERSIZE 8192

#define LISTENER_HEIGHT 100.0f

namespace GemRB {

class OpenALSoundHandle : public SoundHandle {
protected:
	struct AudioStream *parent;

public:
	OpenALSoundHandle(AudioStream *p) : parent(p) { }
	~OpenALSoundHandle() override { }
	void SetPos(const Point&) override;
	bool Playing() override;
	void Stop() override;
	void StopLooping() override;
	void Invalidate() { parent = 0; }
};

struct AudioStream {
	AudioStream() : Buffer(0), Source(0), Duration(0), free(true), ambient(false), locked(false), delete_buffers(false) { }

	ALuint Buffer;
	ALuint Source;
	int Duration;
	bool free;
	bool ambient;
	bool locked;
	bool delete_buffers;

	void ClearIfStopped();
	void ClearProcessedBuffers();
	void ForceClear();

	Holder<OpenALSoundHandle> handle;
};

struct CacheEntry {
	ALuint Buffer;
	unsigned int Length;
};

class OpenALAudioDriver : public Audio {
public:
	OpenALAudioDriver(void);
	~OpenALAudioDriver(void) override;
	void PrintDeviceList();
	bool Init(void) override;
	Holder<SoundHandle> Play(const char* ResRef, unsigned int channel,
					const Point&, unsigned int flags = 0,
					unsigned int *length = 0) override;
	void UpdateVolume(unsigned int flags) override;
	bool CanPlay() override;
	void ResetMusics() override;
	bool Play() override;
	bool Stop() override;
	bool Pause() override;
	bool Resume() override;
	int CreateStream(Holder<SoundMgr>) override;
	void UpdateListenerPos(const Point&) override;
	Point GetListenerPos() override;
	bool ReleaseStream(int stream, bool HardStop) override;
	int SetupNewStream( ieWord x, ieWord y, ieWord z,
					ieWord gain, bool point, int ambientRange) override;
	int QueueAmbient(int stream, const char* sound) override;
	void SetAmbientStreamVolume(int stream, int volume) override;
	void SetAmbientStreamPitch(int stream, int pitch) override;
	void QueueBuffer(int stream, unsigned short bits,
				int channels, short* memory,
				int size, int samplerate) override;
	void UpdateMapAmbient(MapReverb&) override;
private:
	int QueueALBuffer(ALuint source, ALuint buffer);

private:
	ALCcontext *alutContext;
	ALuint MusicSource;
	bool MusicPlaying;
	std::recursive_mutex musicMutex;
	ALuint MusicBuffer[MUSICBUFFERS];
	Holder<SoundMgr> MusicReader;
	LRUCache buffercache;
	AudioStream speech;
	AudioStream streams[MAX_STREAMS];
	ALuint loadSound(const char* ResRef, unsigned int &time_length);
	int num_streams;
	int CountAvailableSources(int limit);
	bool evictBuffer();
	void clearBufferCache(bool force);
	ALenum GetFormatEnum(int channels, int bits) const;
	static int MusicManager(void* args);
	bool stayAlive;
	short* music_memory;
	std::thread musicThread;

	bool InitEFX(void);
	bool hasReverbProperties;

#ifdef HAVE_OPENAL_EFX_H
	bool hasEFX;
	ALuint efxEffectSlot;
	ALuint efxEffect;
	MapReverbProperties reverbProperties;
#endif
};

}

#endif // OPENALAUDIO_H_INCLUDED
