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

#include "../../includes/win32def.h"
#include "../Core/Interface.h"
#include "../Core/ResourceMgr.h"
#include "../Core/MusicMgr.h"
#include "../Core/Variables.h"
#include "ACMImp.h"

#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#ifdef WIN32
#include <io.h>
#else
#include <unistd.h>
#include <sys/time.h>
#endif
#include "AmbientMgrAL.h"

#define DisplayALError(string, error) printf("%s0x%04X", string, error);
#define ACM_BUFFERSIZE 8192
#define MUSICBUFFERS 10

static AudioStream streams[MAX_STREAMS], speech;
static ALuint MusicSource, MusicBuffers[MUSICBUFFERS];
static bool musicPlaying, stayAlive;
static CSoundReader *MusicReader = NULL;
static SDL_mutex* musicMutex = NULL;
static SDL_Thread* musicThread = NULL;
static unsigned char* static_memory = NULL;
static ALCcontext *alutContext = NULL;

//This stuff is a modified version of alut

static ALboolean GemRBalutInit()
{
	ALCdevice *device;
	ALCcontext *context;

	device = alcOpenDevice (NULL);
	if (device == NULL) {
		return AL_FALSE;
	}

	context = alcCreateContext (device, NULL);
	if (context == NULL) {
		alcCloseDevice (device);
		return AL_FALSE;
	}

	if (!alcMakeContextCurrent (context)) {
		alcDestroyContext (context);
		alcCloseDevice (device);
		return AL_FALSE;
	}
	alutContext = context;
	return AL_TRUE;
}

static void GemRBalutExit()
{
	ALCdevice *device;

	alcMakeContextCurrent (NULL);

	device = alcGetContextsDevice (alutContext);
	alcDestroyContext (alutContext);
	if (alcGetError (device) == ALC_NO_ERROR) {
		alcCloseDevice (device);
	}
	alutContext = NULL;
}
//

static int isWAVC(DataStream* stream)
{
	if (!stream) {
		return -1;
	}
	char Signature[4];
	stream->Read( Signature, 4 );
	stream->Seek( 0, GEM_STREAM_START );
#ifdef HAS_VORBIS_SUPPORT
	if(strnicmp(Signature, "oggs", 4) == 0) {
		return SND_READER_OGG;
	} //ogg
#endif
	if(strnicmp(Signature, "RIFF", 4) == 0) {
		return SND_READER_WAV;
	} //wav
	if (*( unsigned int * ) Signature == IP_ACM_SIG) {
		return SND_READER_ACM;
	} //acm
	if (memcmp( Signature, "WAVC", 4 ) == 0) {
		return SND_READER_ACM;
	} //wavc
	return -1;
}

static ALenum GetFormatEnum(int channels, int bits)
{
	switch (channels) {
		case 1:
			if (bits == 8)
				return AL_FORMAT_MONO8;
			else
				return AL_FORMAT_MONO16;
			break;

		case 2:
			if (bits == 8)
				return AL_FORMAT_STEREO8;
			else
				return AL_FORMAT_STEREO16;
			break;
	}
	return AL_FORMAT_MONO8;
}

void ACMImp::clearstreams()
{
	if (musicPlaying) {
		if (alIsSource( MusicSource )) {
			alSourceStop( MusicSource );
			alDeleteSources( 1, &MusicSource );
		}
		musicPlaying = false;
		for (int i = 0; i < MUSICBUFFERS; i++) {
			if (alIsBuffer( MusicBuffers[i] ))
				alDeleteBuffers( 1, &MusicBuffers[i] );
		}
	}
	for (int i = 0; i < MAX_STREAMS; i++) {
		if(!streams[i].free) {
			if (alIsSource(streams[i].Source)) {
				alSourceStop( streams[i].Source );
				alDeleteSources( 1, &streams[i].Source );
			}
			if (alIsBuffer(speech.Buffer))
				alDeleteBuffers( 1, &streams[i].Buffer );
			streams[i].free = true;
		}
	}
	if (!speech.free) {
		if (alIsSource(speech.Source)) {
			alSourceStop( speech.Source );
			alDeleteSources( 1, &speech.Source );
		}
		if (alIsBuffer(speech.Buffer))
			alDeleteBuffers( 1, &speech.Buffer );
		speech.free = true;
	}
	if(MusicReader) {
		delete MusicReader;
		MusicReader = NULL;
	}
}

/* this stuff is in a separate thread, it is using static_memory, others shouldn't use that */
int ACMImp::PlayListManager(void* /*data*/)
{
	ALuint buffersreturned = 0;
	ALboolean bFinished = AL_FALSE;
	while (stayAlive) {
		SDL_mutexP( musicMutex );
		if (musicPlaying) {
			ALint state;
			alGetSourcei( MusicSource, AL_SOURCE_STATE, &state );
			switch (state) {
				default:
					printf( "WARNING: Unhandled Music state: %x\n", state );
					musicPlaying = false;
					alSourcePlay( MusicSource );
					SDL_mutexV( musicMutex );
					return -1;
				case AL_INITIAL:
					 {
						printf( "Music in INITIAL State. AutoStarting\n" );
						for (int i = 0; i < MUSICBUFFERS; i++) {
							MusicReader->read_samples( ( short* ) static_memory, ACM_BUFFERSIZE >> 1 );
							alBufferData( MusicBuffers[i], AL_FORMAT_STEREO16,
								static_memory, ACM_BUFFERSIZE,
								MusicReader->get_samplerate() );
						}
						alSourceQueueBuffers( MusicSource, MUSICBUFFERS, MusicBuffers );
						if (alIsSource( MusicSource )) {
							alSourcePlay( MusicSource );
						}
					}
					break;
				case AL_STOPPED:
					printf( "WARNING: Buffer Underrun. AutoRestarting Stream Playback\n" );
					if (alIsSource( MusicSource )) {
						alSourcePlay( MusicSource );
					}
					break;
				case AL_PLAYING:
					break;
			}
			ALint processed;
			alGetSourcei( MusicSource, AL_BUFFERS_PROCESSED, &processed );
			if (processed > 0) {
				buffersreturned += processed;
				while (processed) {
					ALuint BufferID;
					alSourceUnqueueBuffers( MusicSource, 1, &BufferID );
					if (!bFinished) {
						int size = ACM_BUFFERSIZE;
						int cnt = MusicReader->read_samples( ( short* ) static_memory, ACM_BUFFERSIZE >> 1 );
						size -= ( cnt * 2 );
						if (size != 0)
							bFinished = AL_TRUE;
						if (bFinished) {
							printf( "Playing Next Music: Last Size was %d\n",
								cnt );
							core->GetMusicMgr()->PlayNext();
							if (MusicReader) {
								printf( "Queuing New Music\n" );
								int cnt1 = MusicReader->read_samples( ( short* ) ( static_memory + ( cnt*2 ) ), size >> 1 );
								printf( "Added %d Samples", cnt1 );
								bFinished = false;
							} else {
								printf( "No Other Music\n" );
								memset( static_memory + ( cnt * 2 ), 0, size );
								musicPlaying = false;
							}
						}
						alBufferData( BufferID, AL_FORMAT_STEREO16, static_memory, ACM_BUFFERSIZE, MusicReader->get_samplerate() );
						alSourceQueueBuffers( MusicSource, 1, &BufferID );
						processed--;
					}
				}
			}
		}
		SDL_mutexV( musicMutex );
		SDL_Delay( 30 );
	}
	return 0;
}

ACMImp::ACMImp(void)
{
	unsigned int i;

	for (i = 0; i < MUSICBUFFERS; i++)
		MusicBuffers[i] = 0;
	MusicSource = 0;
	for (i = 0; i < MAX_STREAMS; i++) {
		streams[i].free = true;
	}
	speech.free = true;
	MusicReader = NULL;
	musicPlaying = false;
	musicMutex = SDL_CreateMutex();
	static_memory = (unsigned char *) malloc(ACM_BUFFERSIZE);
	stayAlive = true;
	musicThread = SDL_CreateThread( PlayListManager, NULL );
	ambim = new AmbientMgrAL();
}

ACMImp::~ACMImp(void)
{
	//signal the thread to quit on its own
	stayAlive = false;
	SDL_Delay( 30 );
	//locking the mutex so we could gracefully kill the thread
	SDL_mutexP( musicMutex );
	//the thread is safely killable now
	SDL_KillThread( musicThread );
	//release the mutex after the thread was killed
	SDL_mutexV( musicMutex );
	//the mutex could be removed now too
	SDL_DestroyMutex( musicMutex );
	clearstreams( );
	if(MusicReader) {
		delete MusicReader;
		MusicReader = NULL;
	}
	//freeing the memory of the music thread
	free(static_memory);

	delete ambim;

	GemRBalutExit();
}

#define RETRY 5

bool ACMImp::Init(void)
{
	int i;

	GemRBalutInit( );
	ALenum error = alGetError();
	if (error != AL_NO_ERROR) {
		return false;
	}

	if (MusicSource && alIsSource( MusicSource )) {
		alSourceStop( MusicSource );
		alDeleteSources( 1, &MusicSource );
	}
	MusicSource = 0;
	for (i = 0; i < RETRY; i++) {
		alGenSources( 1, &MusicSource );
		if (( error = alGetError() ) != AL_NO_ERROR) {
			DisplayALError( "[ACMImp::Init] alGenSources : ", error );
		}
		if (alIsSource( MusicSource )) {
			break;
		}
		printf( "Retrying to open sound, last error:(%d)\n", alGetError() );
		SDL_Delay( 15 * 1000 ); //it is given in milliseconds
		GemRBalutInit( );
	}
	if (i == RETRY) {
		return false;
	}

	ALfloat SourcePos[] = {
		0.0f, 0.0f, 0.0f
	};
	ALfloat SourceVel[] = {
		0.0f, 0.0f, 0.0f
	};

	ieDword volume;
	core->GetDictionary()->Lookup( "Volume Music", volume );
	alSourcef( MusicSource, AL_PITCH, 1.0f );
	alSourcef( MusicSource, AL_GAIN, 0.01f * volume );
	alSourcei( MusicSource, AL_SOURCE_RELATIVE, 1 );
	alSourcefv( MusicSource, AL_POSITION, SourcePos );
	alSourcefv( MusicSource, AL_VELOCITY, SourceVel );
	alSourcei( MusicSource, AL_LOOPING, 0 );
	return true;
}

/* load a sound and returns name of a fresh AL buffer containing it
 * returns 0 if failed (0 is not a legal buffer name)
 * returns length of the sound in time_length unless the pointer is NULL
 */
ALuint ACMImp::LoadSound(const char *ResRef, int *time_length)
{
	DataStream* stream = core->GetResourceMgr()->GetResource( ResRef, IE_WAV_CLASS_ID );
	if (!stream) {
		return 0;
	}

	ALuint Buffer;
	ALenum error;

	for (unsigned int i = 0; i < RETRY; i++) {
		alGenBuffers( 1, &Buffer );
		if (( error = alGetError() ) == AL_NO_ERROR) {
			break;
		}
	}
	if (error != AL_NO_ERROR) {
		DisplayALError( "[ACMImp::LoadSound] Cannot Create a Buffer for this sound. Skipping", error );
		delete( stream );
		return 0;
	}
	int type = isWAVC( stream );
	if (type<0 ) {
		delete( stream );
		return 0;
	}

	CSoundReader* acm;
	acm = CreateSoundReader( stream, type, stream->Size(), true );
	if (!acm) {
		printMessage( "ACMImp::Play","Can't create sound reader. ", LIGHT_RED );
		return 0;
	}
	int cnt = acm->get_length();
	int riff_chans = acm->get_channels();
	int samplerate = acm->get_samplerate();
	//multiply always by 2 because it is in 16 bits
	int rawsize = cnt * riff_chans * 2;
	unsigned char * memory = (unsigned char*) malloc(rawsize);
	//multiply always with 2 because it is in 16 bits
	int cnt1 = acm->read_samples( ( short* ) memory, cnt ) * riff_chans * 2;
	//Sound Length in milliseconds
	if (time_length) *time_length = ((cnt / riff_chans) * 1000) / samplerate;
	//it is always reading the stuff into 16 bits
	alBufferData( Buffer, GetFormatEnum( riff_chans, 16 ), memory, cnt1, samplerate );
	delete( acm );
	free(memory);

	if (( error = alGetError() ) != AL_NO_ERROR) {
		DisplayALError( "[ACMImp::LoadSound] alBufferData : ", error );
		alDeleteBuffers( 1, &Buffer );
		return 0;
	}
	return Buffer;
}

/*
 * flags:
 * 	GEM_SND_SPEECH: replace any previous with this flag set
 * 	GEM_SND_RELATIVE: sound position is relative to the listener
 * the default flags are: GEM_SND_RELATIVE
 */
unsigned int ACMImp::Play(const char* ResRef, int XPos, int YPos, unsigned int flags)
{
	unsigned int i;

	int time_length;
	ALuint Buffer = LoadSound(ResRef, &time_length);
	if (0 == Buffer) {
		return 0;
	}
	ALuint Source;
	ALfloat SourcePos[] = {
		(float) XPos, (float) YPos, 0.0f
	};
	ALfloat SourceVel[] = {
		0.0f, 0.0f, 0.0f
	};

	ALenum error;
	ALint state;

	if (flags & GEM_SND_SPEECH) {
		if (speech.free || !alIsSource( speech.Source )) {
			alGenSources( 1, &speech.Source );
			if (( error = alGetError() ) != AL_NO_ERROR) {
				DisplayALError( "[ACMImp::Play] alGenSources : ", error );
			}

			alSourcef( speech.Source, AL_PITCH, 1.0f );
			alSourcefv( speech.Source, AL_VELOCITY, SourceVel );
			alSourcei( speech.Source, AL_LOOPING, 0 );
			alSourcef( speech.Source, AL_REFERENCE_DISTANCE, REFERENCE_DISTANCE );
			speech.free = false;
			printf("speech.free: %d source:%d\n", speech.free,speech.Source);
		} else {
			alSourceStop( speech.Source );
			if (alIsBuffer( speech.Buffer )) {
				alDeleteBuffers( 1, &speech.Buffer );
			}
			printf("***speech.free: %d source:%d\n", speech.free,speech.Source);
		}
		ieDword volume;
		core->GetDictionary()->Lookup( "Volume Voices", volume );
		alSourcef( speech.Source, AL_GAIN, 0.01f * volume );
		alSourcei( speech.Source, AL_SOURCE_RELATIVE, flags & GEM_SND_RELATIVE );
		alSourcefv( speech.Source, AL_POSITION, SourcePos );
		alSourcei( speech.Source, AL_BUFFER, Buffer );
		speech.Buffer = Buffer;
		alSourcePlay( speech.Source );
		return time_length;
	} else {
		alGenSources( 1, &Source );
		if (( error = alGetError() ) != AL_NO_ERROR) {
			DisplayALError( "[ACMImp::Play] alGenSources : ", error );
			return 0;
		}

		alSourcef( Source, AL_PITCH, 1.0f );
		alSourcefv( Source, AL_VELOCITY, SourceVel );
		alSourcei( Source, AL_LOOPING, 0 );
		alSourcef( Source, AL_REFERENCE_DISTANCE, REFERENCE_DISTANCE );
		ieDword volume;
		core->GetDictionary()->Lookup( "Volume SFX", volume );
		alSourcef( Source, AL_GAIN, 0.01f * volume );
		alSourcei( Source, AL_SOURCE_RELATIVE, flags & GEM_SND_RELATIVE );
		alSourcefv( Source, AL_POSITION, SourcePos );
		alSourcei( Source, AL_BUFFER, Buffer );

		if (alGetError() != AL_NO_ERROR) {
			return 0;
		}

		for (i = 0; i < MAX_STREAMS; i++) {
			if (!streams[i].free && alIsSource(streams[i].Source)) {
				alGetSourcei( streams[i].Source, AL_SOURCE_STATE, &state );
				if (state == AL_STOPPED) {
					alDeleteSources( 1, &streams[i].Source );
					alDeleteBuffers( 1, &streams[i].Buffer );
					streams[i].Buffer = Buffer;
					streams[i].Source = Source;
					alSourcePlay( Source );
					return time_length;
				}
			} else {
				streams[i].Buffer = Buffer;
				streams[i].Source = Source;
				streams[i].free = false;
				alSourcePlay( Source );
				return time_length;
			}
		}
	}

	alDeleteSources( 1, &Source );
	alDeleteBuffers( 1, &Buffer );

	return 0;
}

unsigned int ACMImp::StreamFile(const char* filename)
{
	char path[_MAX_PATH];
	ALenum error;

	strcpy( path, core->GamePath );
	strcpy( path, filename );
	FileStream* str = new FileStream();
	if (!str->Open( path, true )) {
		delete( str );
		printf( "Cannot find %s\n", path );
		return 0xffffffff;
	}
	SDL_mutexP( musicMutex );
	if (MusicReader) {
		delete MusicReader;
		MusicReader = NULL;
	}
	if (MusicBuffers[0] == 0) {
		alGenBuffers( MUSICBUFFERS, MusicBuffers );
	}
	int type = isWAVC( str );
	if (type<0 ) {
		delete( str );
		SDL_mutexV( musicMutex );
		return 0;
	}
	MusicReader = CreateSoundReader( str, type, str->Size(), true );

	if (MusicSource == 0) {
		alGenSources( 1, &MusicSource );
		if (( error = alGetError() ) != AL_NO_ERROR) {
			DisplayALError( "[ACMImp::StreamFile] alGenSources : ", error );
		}

		ALfloat SourcePos[] = {
			0.0f, 0.0f, 0.0f
		};
		ALfloat SourceVel[] = {
			0.0f, 0.0f, 0.0f
		};

		ieDword volume;
		core->GetDictionary()->Lookup( "Volume Music", volume );
		alSourcef( MusicSource, AL_PITCH, 1.0f );
		alSourcef( MusicSource, AL_GAIN, 0.01f * volume );
		alSourcei( MusicSource, AL_SOURCE_RELATIVE, 1 );
		alSourcefv( MusicSource, AL_POSITION, SourcePos );
		alSourcefv( MusicSource, AL_VELOCITY, SourceVel );
		alSourcei( MusicSource, AL_LOOPING, 0 );
	}

	SDL_mutexV( musicMutex );
	return 0;
}

bool ACMImp::Stop()
{
	SDL_mutexP( musicMutex );
	if (!alIsSource( MusicSource )) {
		SDL_mutexV( musicMutex );
		return false;
	}
	alSourceStop( MusicSource );
	musicPlaying = false;
	alDeleteSources( 1, &MusicSource );
	MusicSource = 0;
	SDL_mutexV( musicMutex );
	return true;
}

bool ACMImp::Play()
{
	SDL_mutexP( musicMutex );
	if (!musicPlaying) {
		musicPlaying = true;
	}
	SDL_mutexV( musicMutex );
	return true;
}

bool ACMImp::CanPlay()
{
	return true;
}

bool ACMImp::IsSpeaking()
{
	if (speech.free) {
		return false;
	}
	if (!alIsSource( speech.Source )) {
		return false;
	}
	ALint state;

	alGetSourcei( speech.Source, AL_SOURCE_STATE, &state );
	return (state == AL_PLAYING);
}

void ACMImp::ResetMusics()
{
	SDL_mutexP( musicMutex );
	clearstreams( );
	SDL_mutexV( musicMutex );
}

void ACMImp::UpdateViewportPos(int XPos, int YPos)
{
	alListener3f( AL_POSITION, ( float ) XPos, ( float ) YPos, 0.0f );
}

void ACMImp::UpdateVolume( unsigned int which )
{
	if ((GEM_SND_VOL_MUSIC & which) && alIsSource( MusicSource )) {
		SDL_mutexP( musicMutex );
		ieDword volume;
		core->GetDictionary()->Lookup( "Volume Music", volume );
		alSourcef( MusicSource, AL_GAIN, 0.01f * volume );
		SDL_mutexV( musicMutex );
	}
	if ((GEM_SND_VOL_AMBIENTS & which) && ambim) {
		ieDword volume;
		core->GetDictionary()->Lookup( "Volume Ambients", volume );
		((AmbientMgrAL *) ambim) -> UpdateVolume( volume );
	}
}
