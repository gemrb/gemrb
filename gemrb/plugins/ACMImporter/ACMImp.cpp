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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/ACMImporter/ACMImp.cpp,v 1.42 2004/04/21 19:03:11 balrog994 Exp $
 *
 */

#include "../../includes/win32def.h"
#include "../Core/Interface.h"
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

#define DisplayALError(string, error) printf("%s0x%04X", string, error);
#define ACM_BUFFERSIZE 8192
#define MUSICBUFERS 10

static AudioStream streams[MAX_STREAMS];
static CSoundReader *MusicReader;
static ALuint MusicSource, MusicBuffers[MUSICBUFERS];
static SDL_mutex* musicMutex;
static bool musicPlaying;
static SDL_Thread* musicThread;
static unsigned char* static_memory;

bool isWAVC(DataStream* stream)
{
	if (!stream) {
		return false;
	}
	char Signature[4];
	stream->Read( Signature, 4 );
	stream->Seek( 0, GEM_STREAM_START );
	/*
		if(strnicmp(Signature, "RIFF", 4) == 0)
			return false; //wav
		if(strnicmp(Signature, "oggs", 4) == 0)
			return false; //ogg
		if( * (unsigned short *) Signature == 0xfffb)
			return false; //mp3
		return true;
	*/
	if (*( unsigned int * ) Signature == IP_ACM_SIG) {
		return true;
	} //acm
	if (memcmp( Signature, "WAVC", 4 ) == 0) {
		return true;
	} //wavc
	return false;
}

ALenum GetFormatEnum(int channels, int bits)
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

void ACMImp::clearstreams(bool free)
{
	if (musicPlaying && free) {
		for (int i = 0; i < MUSICBUFERS; i++) {
			if (alIsBuffer( MusicBuffers[i] ))
				alDeleteBuffers( 1, &MusicBuffers[i] );
		}
		if (alIsSource( MusicSource ))
			alDeleteSources( 1, &MusicSource );
		musicPlaying = false;
	}
	for (int i = 0; i < MAX_STREAMS; i++) {
		if(!streams[i].free) {
			delete streams[i].reader;
			streams[i].free = true;
		}
	}
	if(MusicReader) {
		delete MusicReader;
		MusicReader = NULL;
	}
}

/* this stuff is in a separate thread, it is using static_memory, others shouldn't use that */
int ACMImp::PlayListManager(void* data)
{
	ALuint buffersreturned = 0;
	ALboolean bFinished = AL_FALSE;
	while (true) {
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
						for (int i = 0; i < MUSICBUFERS; i++) {
							MusicReader->read_samples( ( short* ) static_memory, ACM_BUFFERSIZE >> 1 );
							alBufferData( MusicBuffers[i], AL_FORMAT_STEREO16,
								static_memory, ACM_BUFFERSIZE,
								MusicReader->get_samplerate() );
						}
						alSourceQueueBuffers( MusicSource, MUSICBUFERS, MusicBuffers );
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
			unsigned long volume;
			if (!core->GetDictionary()->Lookup( "Volume Music", volume )) {
				core->GetDictionary()->SetAt( "Volume Music", 100 );
				volume = 100;
			}
			ALint processed;
			alSourcef( MusicSource, AL_GAIN, ( volume / 100.0f ) );
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
	for (int i = 0; i < MUSICBUFERS; i++)
		MusicBuffers[i] = 0;
	MusicSource = 0;
	for (int i = 0; i < MAX_STREAMS; i++) {
		streams[i].free = true;
	}
	MusicReader = NULL;
	musicPlaying = false;
	musicMutex = SDL_CreateMutex();
	static_memory = (unsigned char *) malloc(ACM_BUFFERSIZE);
	musicThread = SDL_CreateThread( PlayListManager, NULL );
}

ACMImp::~ACMImp(void)
{
	//locking the mutex so we could gracefully kill the thread
	SDL_mutexP( musicMutex );
	//the thread is safely killable now
	SDL_KillThread( musicThread );
	//release the mutex after the thread was killed
	SDL_mutexV( musicMutex );
	//the mutex could be removed now too
	SDL_DestroyMutex( musicMutex );
	clearstreams( true );
	if(MusicReader) {
		delete MusicReader;
	}
	//freeing the memory of the music thread
	free(static_memory);
	alutExit();
}

#define RETRY 5

bool ACMImp::Init(void)
{
	int i;

	alutInit( 0, NULL );
	ALenum error = alGetError();
	if (error != AL_NO_ERROR) {
		return false;
	}

	if (MusicSource && alIsSource( MusicSource )) {
		alDeleteSources( 1, &MusicSource );
	}
	MusicSource = 0;
	for (i = 0; i < RETRY; i++) {
		alGenSources( 1, &MusicSource );
		if (( error = alGetError() ) != AL_NO_ERROR) {
			DisplayALError( "[ACMImp::Play] alGenSources : ", error );
		}
		if (alIsSource( MusicSource )) {
			break;
		}
		printf( "Retrying to open sound, last error:(%d)\n", alGetError() );
		SDL_Delay( 15 * 1000 ); //it is given in milliseconds
		alutInit( 0, NULL );
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

	alSourcef( MusicSource, AL_PITCH, 1.0f );
	alSourcef( MusicSource, AL_GAIN, 1.0f );
	alSourcefv( MusicSource, AL_POSITION, SourcePos );
	alSourcefv( MusicSource, AL_VELOCITY, SourceVel );
	alSourcei( MusicSource, AL_LOOPING, 0 );
	return true;
}

unsigned long ACMImp::Play(const char* ResRef, int XPos, int YPos)
{
	DataStream* stream = core->GetResourceMgr()->GetResource( ResRef, IE_WAV_CLASS_ID );
	if (!stream) {
		return 0;
	}

	ALuint Buffer;
	ALuint Source;
	ALfloat SourcePos[] = {
		(float) XPos, (float) YPos, 0.0f
	};
	ALfloat SourceVel[] = {
		0.0f, 0.0f, 0.0f
	};

	ALenum error;
	ALint state;

	for (int i = 0; i < RETRY; i++) {
		alGenBuffers( 1, &Buffer );
		if (( error = alGetError() ) == AL_NO_ERROR) {
			break;
		}
	}
	if (error != AL_NO_ERROR) {
		DisplayALError( "Cannot Create a Buffer for this sound. Skipping", error );
		return 0;
	}
	CSoundReader* acm;
	if (isWAVC( stream )) {
		acm = CreateSoundReader( stream, SND_READER_ACM, stream->Size(), true );
	} else {
		acm = CreateSoundReader( stream, SND_READER_WAV, stream->Size(), true );
	}
	long cnt = acm->get_length();
	long riff_chans = acm->get_channels();	
	//long bits = acm->get_bits();
	long samplerate = acm->get_samplerate();
	//multiply always by 2 because it is in 16 bits
	long rawsize = cnt * riff_chans * 2;
	unsigned char * memory = (unsigned char*) malloc(rawsize); 
	//multiply always with 2 because it is in 16 bits
	long cnt1 = acm->read_samples( ( short* ) memory, cnt ) * riff_chans * 2;
	//Sound Length in milliseconds
	int time_length = ((cnt / riff_chans) * 1000) / samplerate;
	//it is always reading the stuff into 16 bits
	alBufferData( Buffer, GetFormatEnum( riff_chans, 16 ), memory, cnt1, samplerate );
	delete( acm );
	free(memory);
	if (( error = alGetError() ) != AL_NO_ERROR) {
		DisplayALError( "[ACMImp::Play] alBufferData : ", error );
		alDeleteBuffers( 1, &Buffer );
		return 0;
	}

	alGenSources( 1, &Source );
	if (( error = alGetError() ) != AL_NO_ERROR) {
		DisplayALError( "[ACMImp::Play] alGenSources : ", error );
		return 0;
	}

	alSourcei( Source, AL_BUFFER, Buffer );
	alSourcef( Source, AL_PITCH, 1.0f );
	alSourcef( Source, AL_GAIN, 1.0f );
	alSourcefv( Source, AL_POSITION, SourcePos );
	alSourcefv( Source, AL_VELOCITY, SourceVel );
	alSourcei( Source, AL_LOOPING, 0 );

	if (alGetError() != AL_NO_ERROR) {
		return 0;
	}

	for (int i = 0; i < MAX_STREAMS; i++) {
		if (!streams[i].free) {
			alGetSourcei( streams[i].Source, AL_SOURCE_STATE, &state );
			if (state == AL_STOPPED) {
				alDeleteBuffers( 1, &streams[i].Buffer );
				alDeleteSources( 1, &streams[i].Source );
				streams[i].Buffer = Buffer;
				streams[i].Source = Source;
				streams[i].playing = false;
				alSourcePlay( Source );
				return time_length;
			}
		} else {
			streams[i].Buffer = Buffer;
			streams[i].Source = Source;
			streams[i].free = false;
			streams[i].playing = false;
			alSourcePlay( Source );
			return time_length;
		}
	}

	alDeleteBuffers( 1, &Buffer );
	alDeleteSources( 1, &Source );

	return 0;
}

unsigned long ACMImp::StreamFile(const char* filename)
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
		delete( MusicReader );
	}
	if (MusicBuffers[0] == 0) {
		alGenBuffers( MUSICBUFERS, MusicBuffers );
	}
	if (isWAVC( str )) {
		MusicReader = CreateSoundReader( str, SND_READER_ACM, str->Size(),
						true );
	} else {
		MusicReader = CreateSoundReader( str, SND_READER_WAV, str->Size(),
						true );
	}

	if (MusicSource == 0) {
		alGenSources( 1, &MusicSource );
		if (( error = alGetError() ) != AL_NO_ERROR) {
			DisplayALError( "[ACMImp::Play] alGenSources : ", error );
		}

		ALfloat SourcePos[] = {
			0.0f, 0.0f, 0.0f
		};
		ALfloat SourceVel[] = {
			0.0f, 0.0f, 0.0f
		};

		alSourcef( MusicSource, AL_PITCH, 1.0f );
		alSourcef( MusicSource, AL_GAIN, 1.0f );
		alSourcefv( MusicSource, AL_POSITION, SourcePos );
		alSourcefv( MusicSource, AL_VELOCITY, SourceVel );
		alSourcei( MusicSource, AL_LOOPING, 0 );
	}

	SDL_mutexV( musicMutex );
	return 0;
}

bool ACMImp::Stop()
{
	if (!alIsSource( MusicSource )) {
		return false;
	}
	SDL_mutexP( musicMutex );
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

void ACMImp::ResetMusics()
{
	clearstreams( true );
}

void ACMImp::UpdateViewportPos(int XPos, int YPos)
{
	alListener3f( AL_POSITION, ( float ) XPos, ( float ) YPos, 0.0f );
	if (alIsSource( MusicSource )) {
		alSource3f( MusicSource, AL_POSITION, ( float ) XPos, ( float ) YPos,
			0.0f );
	}
}
