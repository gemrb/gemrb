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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/ACMImporter/ACMImp.cpp,v 1.52 2004/08/12 23:26:38 edheldil Exp $
 *
 */

#include "../../includes/win32def.h"
#include "../Core/Interface.h"
#include "../Core/Ambient.h"
#include "ACMImp.h"
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <cmath>
#include <cassert>
#ifdef WIN32
#include <io.h>
#else
#include <unistd.h>
#include <sys/time.h>
#endif

#define DisplayALError(string, error) printf("%s0x%04X", string, error);
#define ACM_BUFFERSIZE 8192
#define MUSICBUFERS 10

// the distance at which sound is played at full volume
#define REFERENCE_DISTANCE 50

static AudioStream streams[MAX_STREAMS], speech;
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
			ALint processed;
			alSourcef( MusicSource, AL_GAIN, ( core->VolumeMusic / 100.0f ) );
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

	for (i = 0; i < MUSICBUFERS; i++)
		MusicBuffers[i] = 0;
	MusicSource = 0;
	for (i = 0; i < MAX_STREAMS; i++) {
		streams[i].free = true;
	}
	MusicReader = NULL;
	musicPlaying = false;
	musicMutex = SDL_CreateMutex();
	static_memory = (unsigned char *) malloc(ACM_BUFFERSIZE);
	musicThread = SDL_CreateThread( PlayListManager, NULL );
	ambim = new AmbientMgr();
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
	
	delete ambim;

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
	if (time_length) *time_length = ((cnt / riff_chans) * 1000) / samplerate;
	//it is always reading the stuff into 16 bits
	alBufferData( Buffer, GetFormatEnum( riff_chans, 16 ), memory, cnt1, samplerate );
	delete( acm );
	free(memory);
	if (( error = alGetError() ) != AL_NO_ERROR) {
		DisplayALError( "[ACMImp::Play] alBufferData : ", error );
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
unsigned long ACMImp::Play(const char* ResRef, int XPos, int YPos, unsigned long flags)
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
		if (!alIsSource( speech.Source )) {
			alGenSources( 1, &speech.Source );

			alSourcef( speech.Source, AL_PITCH, 1.0f );
			alSourcef( speech.Source, AL_GAIN, 1.0f );
			alSourcefv( speech.Source, AL_VELOCITY, SourceVel );
			alSourcei( speech.Source, AL_LOOPING, 0 );
			alSourcef( speech.Source, AL_REFERENCE_DISTANCE, REFERENCE_DISTANCE );
		}
		alSourceStop( speech.Source );	// legal nop if not playing
		alSourcei( speech.Source, AL_SOURCE_RELATIVE, flags & GEM_SND_RELATIVE );
		alSourcefv( speech.Source, AL_POSITION, SourcePos );
		alSourcei( speech.Source, AL_BUFFER, Buffer );
		if (alIsBuffer( speech.Buffer )) {
			alDeleteBuffers( 1, &speech.Buffer );
		}
		speech.Buffer = Buffer;
		alSourcePlay( speech.Source );
		return time_length;
	} else {
		alGenSources( 1, &Source );
		if (( error = alGetError() ) != AL_NO_ERROR) {
			DisplayALError( "[ACMImp::Play] alGenSources : ", error );
			return 0;
		}
	
		alSourcei( Source, AL_BUFFER, Buffer );
		alSourcef( Source, AL_PITCH, 1.0f );
		alSourcef( Source, AL_GAIN, 1.0f );
		alSourcefv( Source, AL_VELOCITY, SourceVel );
		alSourcei( Source, AL_LOOPING, 0 );
		alSourcef( Source, AL_REFERENCE_DISTANCE, REFERENCE_DISTANCE );
		alSourcei( Source, AL_SOURCE_RELATIVE, flags & GEM_SND_RELATIVE );
		alSourcefv( Source, AL_POSITION, SourcePos );
			
		if (alGetError() != AL_NO_ERROR) {
			return 0;
		}
	
		for (i = 0; i < MAX_STREAMS; i++) {
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
}

// legal nop if already reset
void ACMImp::AmbientMgr::reset()
{
	if (NULL != player){
		SDL_mutexP(mutex);
	}
	for (std::vector<AmbientSource *>::iterator it = ambientSources.begin(); it != ambientSources.end(); ++it) {
		delete (*it);
	}
	ambientSources.clear();
	SoundMgr::AmbientMgr::reset();
	if (NULL != player) {
		SDL_CondSignal(cond);
		SDL_mutexV(mutex);
		SDL_WaitThread(player, NULL);
		player = NULL;
	}
}

void ACMImp::AmbientMgr::setAmbients(const std::vector<Ambient *> &a)
{
	SoundMgr::AmbientMgr::setAmbients(a);
	assert(NULL == player);
	
	ambientSources.reserve(a.size());
	for (std::vector<Ambient *>::const_iterator it = a.begin(); it != a.end(); ++it) {
		ambientSources.push_back(new AmbientSource(*it));
	}
	
	player = SDL_CreateThread(&play, (void *) this);
}

void ACMImp::AmbientMgr::activate(const std::string &name) 
{
	if (NULL != player)
		SDL_mutexP(mutex);
	SoundMgr::AmbientMgr::activate(name);
	if (NULL != player) {
		SDL_CondSignal(cond);
		SDL_mutexV(mutex);
	}
}

void ACMImp::AmbientMgr::activate()
{
	if (NULL != player)
		SDL_mutexP(mutex);
	SoundMgr::AmbientMgr::activate();
	if (NULL != player) {
		SDL_CondSignal(cond);
		SDL_mutexV(mutex);
	}
}

void ACMImp::AmbientMgr::deactivate(const std::string &name) 
{
	if (NULL != player)
		SDL_mutexP(mutex);
	SoundMgr::AmbientMgr::deactivate(name);
	if (NULL != player) {
		SDL_CondSignal(cond);
		SDL_mutexV(mutex);
	}
}

void ACMImp::AmbientMgr::deactivate() 
{
	if (NULL != player)
		SDL_mutexP(mutex);
	SoundMgr::AmbientMgr::deactivate();
	hardStop();
	if (NULL != player)
		SDL_mutexV(mutex);
}

void ACMImp::AmbientMgr::hardStop()
{
	for (std::vector<AmbientSource *>::iterator it = ambientSources.begin(); it != ambientSources.end(); ++it) {
		(*it)->hardStop();
	}
}

int ACMImp::AmbientMgr::play(void *am) 
{
	AmbientMgr * ambim = (AmbientMgr *) am;
	SDL_mutexP(ambim->mutex);
	while (0 != ambim->ambientSources.size()) {
		if (NULL == core->GetGame()) { // we don't have any game, and we need one
			break;
		}
		unsigned int delay = ambim->tick(SDL_GetTicks());
		assert(delay > 0);
		SDL_CondWaitTimeout(ambim->cond, ambim->mutex, delay);
	}
	SDL_mutexV(ambim->mutex);
	return 0;
}

unsigned int ACMImp::AmbientMgr::tick(unsigned int ticks)
{
	unsigned int delay = 60000; // wait one minute if all sources are off
	
	if (!active)
		return delay;
	
	ALfloat listen[3];
	alGetListenerfv( AL_POSITION, listen );
	Point listener;
	listener.x = (short) listen[0];
	listener.y = (short) listen[1];
	
	unsigned int timeslice = ((core->GetGame()->GameTime / 60 + 30) / 60 - 1) % 24;
	
	for (std::vector<AmbientSource *>::iterator it = ambientSources.begin(); it != ambientSources.end(); ++it) {
		unsigned int newdelay = (*it)->tick(ticks, listener, timeslice);
		if (newdelay < delay)
			delay = newdelay;
	}
	return delay;
}

ACMImp::AmbientMgr::AmbientSource::AmbientSource(Ambient *a)
: ambient(a), lastticks(0), enqueued(0)
{
	alGenSources( 1, &source );
	
	ALfloat position[] = { (float) a->getOrigin().x, (float) a->getOrigin().y, (float) a->getHeight() };
	alSourcefv( source, AL_POSITION, position );
	alSourcef( source, AL_GAIN, 0.01f * a->getGain() );
	alSourcei( source, AL_REFERENCE_DISTANCE, REFERENCE_DISTANCE );
	alSourcei( source, AL_ROLLOFF_FACTOR, (a->getFlags() & IE_AMBI_POINT) ? 1 : 0 );
	
/*	ALint state, queued, processed;
	alGetSourcei( source, AL_SOURCE_STATE, &state );
	alGetSourcei( source, AL_BUFFERS_QUEUED, &queued );
	alGetSourcei( source, AL_BUFFERS_PROCESSED, &processed );
	printf("ambient %s: source %x, state %x, queued %d, processed %d\n", ambient->getName().c_str(), source, state, queued, processed);
	if (!alIsSource( source )) printf("hey, it's not a source!\n");*/
	
	// preload sounds
	unsigned int i=a->sounds.size();
	buffers.reserve(i);
	buflens.reserve(i);
	while(i--) {
		int timelen;
		ALuint buffer = LoadSound(a->sounds[i], &timelen);
		if (!buffer) {
			printf("Invalid SoundResRef: %.8s, Dequeueing...\n",a->sounds[i]);
			free(a->sounds[i]);
		        a->sounds.erase(a->sounds.begin() + i);
		} else {
			buffers.push_back(buffer);
			buflens.push_back(timelen);
		}
		
	}
/*	
	// use OpenAL to loop in this special case so we don't have to
	if ((buffers.size() == 1) && (a->getInterval() == 0)) {
		alSourcei( source, AL_LOOPING, 1 );
		alSourcei( source, AL_BUFFER, buffers[0] );
	}*/
}

ACMImp::AmbientMgr::AmbientSource::~AmbientSource()
{
	alSourceStop( source );	// legal nop if not playing
//	printf("deleting source %x\n", source);
	alDeleteSources( 1, &source );
	for (std::vector<ALuint>::iterator it = buffers.begin(); it != buffers.end(); ++it) {
		alDeleteBuffers( 1, &(*it) );
	}
}

unsigned int ACMImp::AmbientMgr::AmbientSource::tick(unsigned int ticks, Point listener, unsigned int timeslice)
{
	ALint state;
/*	ALint queued, processed;
	alGetSourcei( source, AL_SOURCE_STATE, &state );
	alGetSourcei( source, AL_BUFFERS_QUEUED, &queued );
	alGetSourcei( source, AL_BUFFERS_PROCESSED, &processed );
	printf("ambient %s: source %x, state %x, queued %d, processed %d\n", ambient->getName().c_str(), source, state, queued, processed);
	if (!alIsSource( source )) printf("hey, it's not a source!\n");*/
	
	if ((! (ambient->getFlags() & IE_AMBI_ENABLED)) || (! ambient->getAppearance()&(1<<timeslice))) {
		// don't really stop the source, since we don't want to stop playing abruptly in the middle of
		// a sample (do we?), and it would end playing by itself in a while (Divide)
		//this is correct (Avenger)
		return UINT_MAX;
	}
	
	int delay = ambient->getInterval() * 1000;
	int left = lastticks - ticks + delay;
	if (0 < left) // we are still waiting
		return left;
	if (enqueued > 0) // we have already played that much
		enqueued += left;
	if (enqueued < 0)
		enqueued = 0;
	
	lastticks = ticks;
	if (0 == delay) // it's a non-stop ambient, so in any case wait only a sec
		delay = 1000;
	
	if (! (ambient->getFlags() & IE_AMBI_MAIN) && !isHeard( listener )) { // we are out of range
		return delay;
	}
	
	dequeProcessed();
	
	/* it seems that the following (commented out) is not the purpose of the perset field, as
	it leads to ambients playing non-stop and queues overfilled */
/*	int leftNum = ambient -> getPerset(); */
	int leftNum = 1;
	int leftMS = 0;
	if (0 == ambient->getInterval()) {
		leftNum = 0;
		leftMS = 1000 - enqueued; // let's have at least 1 second worth queue
	}
	while (0 < leftNum || 0 < leftMS) {
		int len = enqueue();
		--leftNum;
		leftMS -= len;
		enqueued += len;
	}
	
	// oh, and don't forget to push play
	alGetSourcei( source, AL_SOURCE_STATE, &state );
	if (AL_PLAYING != state) { // play on playing source would rewind it
		alSourcePlay( source );
	}
	
	return delay;
}

/* dequeues already processed buffers */
void ACMImp::AmbientMgr::AmbientSource::dequeProcessed()
{
	ALint processed;
	alGetSourcei( source, AL_BUFFERS_PROCESSED, &processed );
	if (0 == processed) return;
	ALuint * buffers = (ALuint *) malloc ( processed * sizeof(ALuint) );
	alSourceUnqueueBuffers( source, processed, buffers );
	free(buffers);
	// do not destroy buffers since we reuse them
}

/* enqueues a random sound and returns its length */
unsigned int ACMImp::AmbientMgr::AmbientSource::enqueue()
{
	int index = rand() % buffers.size();
	/* yeah, yeah, I know what rand(3) says... but we don't need much randomness here and this is fast
	 * (fast to write, also ;-)
	 */
	
	alSourceQueueBuffers( source, 1, &(buffers[index]) );
	
	return buflens[index];
}

bool ACMImp::AmbientMgr::AmbientSource::isHeard(const Point &listener) const
{
	float xdist = listener.x - ambient->getOrigin().x;
	float ydist = listener.y - ambient->getOrigin().y;
	float dist = sqrt(xdist * xdist + ydist * ydist);
	return dist < ambient->getRadius();
}

void ACMImp::AmbientMgr::AmbientSource::hardStop()
{
	alSourceStop( source );
	dequeProcessed();
}
