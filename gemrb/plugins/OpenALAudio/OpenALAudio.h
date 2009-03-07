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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Id$
 *
 */

#ifndef OPENALAUDIO_H_INCLUDED
#define OPENALAUDIO_H_INCLUDED

#define RETRY 5
#define BUFFER_CACHE_SIZE 100
#define MAX_STREAMS 30
#define MUSICBUFFERS 10
#define REFERENCE_DISTANCE 50
#define ACM_BUFFERSIZE 8192

#include "SDL.h"
#include "../Core/Audio.h"
#include "../Core/LRUCache.h"
#include "../Core/Interface.h"
#include "../Core/MusicMgr.h"
#include "../../includes/ie_types.h"
#include "../Core/FileStream.h"
#include "../Core/SoundMgr.h"
#include "AmbientMgrAL.h"
#include "../Core/ResourceMgr.h"
#include "StackLock.h"

#ifndef WIN32
#include <AL/al.h>
#include <AL/alc.h>
#else
#include <al.h>
#include <alc.h>
#endif


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
};

struct CacheEntry {
    ALuint Buffer;
    unsigned int Length;
};

class OpenALAudioDriver : public Audio {
public:
    OpenALAudioDriver(void);
    ~OpenALAudioDriver(void);
    bool Init(void);
    unsigned int Play(const char* ResRef, int XPos = 0, int YPos = 0,
                      unsigned int flags = GEM_SND_RELATIVE);
    void release(void)
    {
        delete this;
    }
    bool IsSpeaking();
    void UpdateVolume(unsigned int flags);
    bool CanPlay();
    void ResetMusics();
    bool Play();
    bool Stop();
    int StreamFile( const char* filename );
    void UpdateListenerPos(int XPos, int YPos );
    void GetListenerPos( int &XPos, int &YPos );
    bool ReleaseStream(int stream, bool HardStop);
    int SetupNewStream( ieWord x, ieWord y, ieWord z,
                    ieWord gain, bool point, bool Ambient );
    int QueueAmbient(int stream, const char* sound);
    void SetAmbientStreamVolume(int stream, int volume);
    void QueueBuffer(int stream, unsigned short bits,
                int channels, short* memory,
                int size, int samplerate) ;
private:
    ALCcontext *alutContext;
    ALuint MusicSource;
    bool MusicPlaying;
    SDL_mutex* musicMutex;
    ALuint MusicBuffer[MUSICBUFFERS];
    SoundMgr* MusicReader;
    LRUCache buffercache;
    AudioStream speech;
    AudioStream streams[MAX_STREAMS];
    ALuint loadSound(const char* ResRef, unsigned int &time_length);
    int num_streams;
    int CountAvailableSources(int limit);
    bool evictBuffer();
    void clearBufferCache();
    ALenum GetFormatEnum(int channels, int bits);
    static int MusicManager(void* args);
    bool stayAlive;
    unsigned char* music_memory;
    SDL_Thread* musicThread;
};

#endif // OPENALAUDIO_H_INCLUDED
