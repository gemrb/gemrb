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
#include "StackLock.h"

#include "ie_types.h"

#include "Interface.h"
#include "LRUCache.h"
#include "MusicMgr.h"
#include "SoundMgr.h"
#include "System/FileStream.h"
#include "MapReverb.h"

#include <SDL.h>

#ifndef WIN32
#ifdef __APPLE_CC__
#include <OpenAL/al.h>
#include <OpenAL/alc.h>
#ifdef HAVE_OPENAL_EFX_H
#include <OpenAL/efx.h>
#endif
#else
#include <AL/al.h>
#include <AL/alc.h>
#ifdef HAVE_OPENAL_EFX_H
#include <AL/efx.h>
#endif
#endif
#else
#include <al.h>
#include <alc.h>
#ifdef HAVE_OPENAL_EFX_H
#include <efx.h>
#endif
#endif

#if ANDROID && SDL_COMPILEDVERSION < SDL_VERSIONNUM(1,3,0)
// Pely's build only
#include <AL/android.h>
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
	virtual ~OpenALSoundHandle() { }
	virtual void SetPos(int XPos, int YPos);
	virtual bool Playing();
	virtual void Stop();
	virtual void StopLooping();
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
	~OpenALAudioDriver(void);
	void PrintDeviceList();
	bool Init(void);
	Holder<SoundHandle> Play(const char* ResRef, unsigned int channel,
					int XPos, int YPos, unsigned int flags = 0,
					unsigned int *length = 0);
	void UpdateVolume(unsigned int flags);
	bool CanPlay();
	void ResetMusics();
	bool Play();
	bool Stop();
	bool Pause();
	bool Resume();
	int CreateStream(Holder<SoundMgr>);
	void UpdateListenerPos(int XPos, int YPos );
	void GetListenerPos( int &XPos, int &YPos );
	bool ReleaseStream(int stream, bool HardStop);
	int SetupNewStream( ieWord x, ieWord y, ieWord z,
					ieWord gain, bool point, int ambientRange);
	int QueueAmbient(int stream, const char* sound);
	void SetAmbientStreamVolume(int stream, int volume);
	void SetAmbientStreamPitch(int stream, int pitch);
	void QueueBuffer(int stream, unsigned short bits,
				int channels, short* memory,
				int size, int samplerate);
	void UpdateMapAmbient(MapReverb&);
private:
	int QueueALBuffer(ALuint source, ALuint buffer);

private:
	ALCcontext *alutContext;
	ALuint MusicSource;
	bool MusicPlaying;
	SDL_mutex* musicMutex;
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
	SDL_Thread* musicThread;

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
