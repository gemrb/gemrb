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

#include "OpenALAudio.h"

void AudioStream::ClearProcessedBuffers(bool del)
{
	ALint processed = 0;
	alGetSourcei( Source, AL_BUFFERS_PROCESSED, &processed );
	if (processed > 0) {
		ALuint * b = new ALuint[processed];
		alSourceUnqueueBuffers( Source, processed, b );

		if (del)
			alDeleteBuffers(processed, b);

		delete[] b;
	}

}

void AudioStream::ClearIfStopped()
{
	if (free || locked) return;

	if (alIsSource(Source)) {
		ALint state;
		alGetSourcei( Source, AL_SOURCE_STATE, &state );
		if (state == AL_STOPPED) {
			ClearProcessedBuffers(false);
			alDeleteSources( 1, &Source );
			Source = 0;
			Buffer = 0;
			free = true;
			ambient = false;
			locked = false;
		}
	}
}

OpenALAudioDriver::OpenALAudioDriver(void)
{
	alutContext = NULL;
	MusicPlaying = false;
	music_memory = (unsigned char*) malloc(ACM_BUFFERSIZE);
	MusicSource = 0;
	memset(MusicBuffer, 0, MUSICBUFFERS*sizeof(ALuint));
	musicMutex = SDL_CreateMutex();
	MusicReader = 0;
}

bool OpenALAudioDriver::Init(void)
{
	ALCdevice *device;
	ALCcontext *context;

	device = alcOpenDevice (NULL);
	if (device == NULL) {
		return false;
	}

	context = alcCreateContext (device, NULL);
	if (context == NULL) {
		alcCloseDevice (device);
		return false;
	}

	if (!alcMakeContextCurrent (context)) {
		alcDestroyContext (context);
		alcCloseDevice (device);
		return false;
	}
	alutContext = context;

	//1 for speech
	int sources = CountAvailableSources(MAX_STREAMS+1);
	num_streams = sources - 1;

	char buf[255];
	sprintf(buf, "Allocated %d streams.%s", num_streams,
		    (num_streams < MAX_STREAMS ? " (Fewer than desired.)" : "" ) );

	printMessage( "OpenAL", buf, WHITE );

	stayAlive = true;
	musicThread = SDL_CreateThread( MusicManager, this );

	ambim = new AmbientMgrAL;
	speech.free = true;
	speech.ambient = false;
	return true;
}

int OpenALAudioDriver::CountAvailableSources(int limit)
{
	ALuint* src = new ALuint[limit+2];
	int i;
	for (i = 0; i < limit+2; ++i) {
		alGenSources(1, &src[i]);
		if (alGetError() != AL_NO_ERROR)
			break;
	}
	if (i > 0)
		alDeleteSources(i, src);
	delete[] src;

	// Leave two sources free for internal OpenAL usage
	// (Might not be strictly necessary...)
	i -= 2;

	// Return number of succesfully allocated sources
	return i;
}

OpenALAudioDriver::~OpenALAudioDriver(void)
{
	ALCdevice *device;

	alcMakeContextCurrent (NULL);

	device = alcGetContextsDevice (alutContext);
	alcDestroyContext (alutContext);
	if (alcGetError (device) == ALC_NO_ERROR) {
		alcCloseDevice (device);
	}
	alutContext = NULL;
	SDL_mutexP(musicMutex);
	SDL_KillThread(musicThread);
	SDL_mutexV(musicMutex);

	SDL_DestroyMutex(musicMutex);
	musicMutex = NULL;

	free(music_memory);
	if(MusicReader)
		core->FreeInterface(MusicReader);

	delete ambim;
}

ALuint OpenALAudioDriver::loadSound(const char *ResRef, unsigned int &time_length)
{
	ALuint Buffer = 0;

	CacheEntry *e;
	void* p;

	if (!ResRef[0]) {
		return 0;
	}
	if(buffercache.Lookup(ResRef, p))
	{
		e = (CacheEntry*) p;
		time_length = e->Length;
		return e->Buffer;
	}
	//no cache entry...
	DataStream* stream = core->GetResourceMgr()->GetResource(ResRef, IE_WAV_CLASS_ID);
	if (!stream)
		return 0;

	alGenBuffers(1, &Buffer);
	int error = alGetError();
	if ( error != AL_NO_ERROR ) {
		printMessage("OpenAL", "Unable to create buffer", WHITE );
		printf (": %d", error);
		printStatus("ERROR", YELLOW);
		delete stream;
		return 0;
	}

	SoundMgr* acm = (SoundMgr*) core->GetInterface(IE_WAV_CLASS_ID);
	if (!acm->Open(stream)) {
		core->FreeInterface(acm);
		return 0;
	}
	int cnt = acm->get_length();
	int riff_chans = acm->get_channels();
	int samplerate = acm->get_samplerate();
	//multiply always by 2 because it is in 16 bits
	int rawsize = cnt * 2;
	unsigned char * memory = (unsigned char*) malloc(rawsize);
	//multiply always with 2 because it is in 16 bits
	int cnt1 = acm->read_samples( ( short* ) memory, cnt ) * 2;
	//Sound Length in milliseconds
	time_length = ((cnt / riff_chans) * 1000) / samplerate;
	//it is always reading the stuff into 16 bits
	alBufferData( Buffer, GetFormatEnum( riff_chans, 16 ), memory, cnt1, samplerate );
	core->FreeInterface( acm );
	free(memory);

	if (( alGetError() ) != AL_NO_ERROR) {
		printMessage( "OpenAL", "Error : unable to fill buffer", WHITE );
		printStatus("ERROR", YELLOW );
		alDeleteBuffers( 1, &Buffer );
		return 0;
	}

	e = new CacheEntry;
	e->Buffer = Buffer;
	e->Length = ((cnt / riff_chans) * 1000) / samplerate;

	buffercache.SetAt(ResRef, (void*)e);
	//printf("LoadSound: added %s to cache: %d. Cache size now %d\n", ResRef, e->Buffer, buffercache.GetCount());

	if (buffercache.GetCount() > BUFFER_CACHE_SIZE) {
		evictBuffer();
	}
	return Buffer;
}

unsigned int OpenALAudioDriver::Play(const char* ResRef, int XPos, int YPos, unsigned int flags)
{
	ALuint Buffer;
	unsigned int time_length;

	Buffer = loadSound( ResRef, time_length );
	if (Buffer == 0) {
		return 0;
	}

	ALuint Source;
	ALfloat SourcePos[] = {
		(float) XPos, (float) YPos, 0.0f
	};
	ALfloat SourceVel[] = {
		0.0f, 0.0f, 0.0f
	};

	ieDword volume = 100;

	if (flags & GEM_SND_SPEECH) {
		//speech has a single channel, if a new speech started
		//we stop the previous one
		if (speech.free || !alIsSource( speech.Source )) {
			alGenSources( 1, &speech.Source );
			if (alGetError() != AL_NO_ERROR) {
				printMessage( "OpenAL", "Unable to attach a source for speech", WHITE );
				printStatus( "ERROR", YELLOW );
				return 0;
			}

			alSourcef( speech.Source, AL_PITCH, 1.0f );
			alSourcefv( speech.Source, AL_VELOCITY, SourceVel );
			alSourcei( speech.Source, AL_LOOPING, 0 );
			alSourcef( speech.Source, AL_REFERENCE_DISTANCE, REFERENCE_DISTANCE );
			if (alGetError() != AL_NO_ERROR) {
				printMessage( "OpenAL", "Unable to set speech parameters", WHITE);
				printStatus( "ERROR", YELLOW );
			}
			speech.free = false;
			printf("speech.free: %d source:%d\n", speech.free,speech.Source);
		} else {
			alSourceStop( speech.Source );
			if (alGetError() != AL_NO_ERROR) {
				printMessage( "OpenAL", "Unable to stop speech", WHITE);
				printStatus( "ERROR", YELLOW );
			}
			speech.ClearProcessedBuffers(false);
		}
		core->GetDictionary()->Lookup( "Volume Voices", volume );
		alSourcef( speech.Source, AL_GAIN, 0.01f * volume );
		alSourcei( speech.Source, AL_SOURCE_RELATIVE, flags & GEM_SND_RELATIVE );
		alSourcefv( speech.Source, AL_POSITION, SourcePos );
		alSourcei( speech.Source, AL_BUFFER, Buffer );
		speech.Buffer = Buffer;
		alSourcePlay( speech.Source );
		if (alGetError() != AL_NO_ERROR) {
			printMessage( "OpenAL", "Unable to play speech", WHITE);
			printStatus( "ERROR", YELLOW );
			return 0;
		}
		return time_length;
	}

	int stream = -1;
	for (int i = 0; i < num_streams; i++) {
		streams[i].ClearIfStopped();
		if (streams[i].free) {
			stream = i;
			break;
		}
	}

	if (stream == -1) {
		// Failed to assign new sound.
		// The buffercache will handle deleting Buffer.
		return 0;
	}

	// not speech
	alGenSources( 1, &Source );
	if (alGetError() != AL_NO_ERROR) {
		//failed to generate new sound source
		printMessage( "OpenAL", "Unable to create a source", WHITE );
		printStatus( "ERROR", YELLOW );
		return 0;
	}

	alSourcef( Source, AL_PITCH, 1.0f );
	alSourcefv( Source, AL_VELOCITY, SourceVel );
	alSourcei( Source, AL_LOOPING, 0 );
	alSourcef( Source, AL_REFERENCE_DISTANCE, REFERENCE_DISTANCE );
	core->GetDictionary()->Lookup( "Volume SFX", volume );
	alSourcef( Source, AL_GAIN, 0.01f * volume );
	alSourcei( Source, AL_SOURCE_RELATIVE, flags & GEM_SND_RELATIVE );
	alSourcefv( Source, AL_POSITION, SourcePos );
	alSourcei( Source, AL_BUFFER, Buffer );

	if (alGetError() != AL_NO_ERROR) {
		printMessage( "OpenAL", "Unable to play sound", WHITE);
		printStatus( "ERROR", YELLOW );
		return 0;
	}

	streams[stream].Buffer = Buffer;
	streams[stream].Source = Source;
	streams[stream].free = false;
	alSourcePlay( Source );
	return time_length;
}

bool OpenALAudioDriver::IsSpeaking()
{
	speech.ClearIfStopped();
	return !speech.free;
}

void OpenALAudioDriver::UpdateVolume(unsigned int flags)
{
	ieDword volume;

	if (flags & GEM_SND_VOL_MUSIC) {
		SDL_mutexP( musicMutex );
		core->GetDictionary()->Lookup("Volume Music", volume);
		if (alIsSource(MusicSource))
		    alSourcef(MusicSource, AL_GAIN, volume * 0.01f);
		SDL_mutexV(musicMutex);
	}

	if (flags & GEM_SND_VOL_AMBIENTS) {
		core->GetDictionary()->Lookup("Volume Ambients", volume);
		((AmbientMgrAL*) ambim)->UpdateVolume(volume);
	}
}

bool OpenALAudioDriver::CanPlay()
{
	return true;
}

void OpenALAudioDriver::ResetMusics()
{
	MusicPlaying = false;
	SDL_mutexP( musicMutex );
	if (alIsSource(MusicSource)) {
		alSourceStop(MusicSource);
		alDeleteSources(1, &MusicSource );
		for (int i=0; i<2; i++) {
		    if (alIsBuffer(MusicBuffer[i]))
		        alDeleteBuffers(1, MusicBuffer+i);
		}
	}
	SDL_mutexV( musicMutex );
}

bool OpenALAudioDriver::Play()
{
	SDL_mutexP( musicMutex );
	if (!MusicPlaying)
		MusicPlaying = true;
	SDL_mutexV( musicMutex );

	return true;
}

bool OpenALAudioDriver::Stop()
{
	SDL_mutexP( musicMutex );
	if (!alIsSource( MusicSource )) {
		SDL_mutexV( musicMutex );
		return false;
	}
	alSourceStop( MusicSource );
	MusicPlaying = false;
	alDeleteSources( 1, &MusicSource );
	MusicSource = 0;
	SDL_mutexV( musicMutex );
	return true;
}

int OpenALAudioDriver::StreamFile(const char* filename)
{
	char path[_MAX_PATH];
	ALenum error;

	strcpy( path, core->GamePath );
	strcpy( path, filename );
	FileStream* str = new FileStream();
	if (!str->Open( path, true )) {
		delete str;
		printMessage("OpenAL", "",WHITE);
		printf( "Cannot find %s", path );
		printStatus("NOT FOUND", YELLOW );
		return -1;
	}
	SDL_mutexP( musicMutex );

	// Free old MusicReader
	core->FreeInterface(MusicReader);
	MusicReader = NULL;

	if (MusicBuffer[0] == 0) {
		alGenBuffers( MUSICBUFFERS, MusicBuffer );
	}

	MusicReader = (SoundMgr*) core->GetInterface( IE_WAV_CLASS_ID );
	if (!MusicReader->Open(str, true)) {
		delete str;
		core->FreeInterface(MusicReader);
		MusicReader = NULL;
		printMessage("OpenAL", "",WHITE);
		printf( "Cannot open %s", path );
		printStatus("ERROR", YELLOW );
	}

	if (MusicSource == 0) {
		alGenSources( 1, &MusicSource );
		if (( error = alGetError() ) != AL_NO_ERROR) {
			printMessage("OpenAL", "error : unable to attach a source", WHITE );
			printStatus("ERROR", YELLOW);
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

void OpenALAudioDriver::UpdateListenerPos(int XPos, int YPos )
{
	alListener3f( AL_POSITION, (float) XPos, (float) YPos, 0.0f );
}

void OpenALAudioDriver::GetListenerPos(int &XPos, int &YPos )
{
	ALfloat listen[3];
	alGetListenerfv( AL_POSITION, listen );
	XPos = (int) listen[0];
	YPos = (int) listen[1];
}

bool OpenALAudioDriver::ReleaseStream(int stream, bool HardStop)
{
	if (streams[stream].free || !streams[stream].locked)
		return false;
	if (!HardStop) {
		// unlock it, so it will automatically be reclaimed when needed
		streams[stream].locked = false;
		return true;
	}

	ALuint Source = streams[stream].Source;
	alSourceStop(Source);

	streams[stream].ClearProcessedBuffers(false);
	alDeleteSources(1, &streams[stream].Source);
	streams[stream].Source = 0;
	streams[stream].free = true;
	streams[stream].ambient = false;
	streams[stream].locked = false;

   	return true;
}

//This one is used for movies and ambients.
int OpenALAudioDriver::SetupNewStream( ieWord x, ieWord y, ieWord z,
		            ieWord gain, bool point, bool Ambient )
{
	// Find a free (or finished) stream for this sound
	int stream = -1;
	for (int i = 0; i < num_streams; i++) {
		streams[i].ClearIfStopped();
		if (streams[i].free) {
			stream = i;
			break;
		}
	}
	if (stream == -1) return -1;

	ALuint source;
	alGenSources(1, &source);

	ALfloat position[] = { (float) x, (float) y, (float) z };
	alSourcef( source, AL_PITCH, 1.0f );
	alSourcefv( source, AL_POSITION, position );
	alSourcef( source, AL_GAIN, 0.01f * gain );
	alSourcei( source, AL_REFERENCE_DISTANCE, REFERENCE_DISTANCE );
	alSourcei( source, AL_ROLLOFF_FACTOR, point ? 1 : 0 );

	streams[stream].Buffer = 0;
	streams[stream].Source = source;
	streams[stream].free = false;
	streams[stream].ambient = Ambient;
	streams[stream].locked = true;

	return stream;
}

int OpenALAudioDriver::QueueAmbient(int stream, const char* sound)
{
	if (streams[stream].free || !streams[stream].ambient)
		return -1;

	ALuint source = streams[stream].Source;

	// first dequeue any processed buffers
	streams[stream].ClearProcessedBuffers(false);

	if (sound == 0)
		return 0;

	unsigned int time_length;
	ALuint Buffer = loadSound(sound, time_length);
	if (0 == Buffer) {
		return -1;
	}

	alSourceQueueBuffers(source, 1, &Buffer);

	// play
	ALint state;
	alGetSourcei( source, AL_SOURCE_STATE, &state );
	if (state != AL_PLAYING) { // play on playing source would rewind it
		alSourcePlay( source );
	}

	return time_length;
}

void OpenALAudioDriver::SetAmbientStreamVolume(int stream, int volume)
{
	if (streams[stream].free || !streams[stream].ambient)
		return;

	ALuint source = streams[stream].Source;
	alSourcef( source, AL_GAIN, 0.01f * volume );
}

bool OpenALAudioDriver::evictBuffer()
{
	// Note: this function assumes the caller holds bufferMutex

	// Room for optimization: this is O(n^2) in the number of buffers
	// at the tail that are used. It can be O(n) if LRUCache supports it.

	unsigned int n = 0;
	void* p;
	const char* k;
	bool res;

	while ((res = buffercache.getLRU(n, k, p)) == true) {
		CacheEntry* e = (CacheEntry*)p;
		alDeleteBuffers(1, &e->Buffer);
		if (alGetError() == AL_NO_ERROR) {
			// Buffer was unused. An error would have indicated
			// the buffer was still attached to a source.

			buffercache.Remove(k);

			//printf("Removed buffer %s from ACMImp cache\n", k);
			break;
		}
		++n;
	}

	return res;
}

ALenum OpenALAudioDriver::GetFormatEnum(int channels, int bits)
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

int OpenALAudioDriver::MusicManager(void* arg)
{
	OpenALAudioDriver* driver = (OpenALAudioDriver*) arg;
	ALuint buffersreturned = 0;
	ALboolean bFinished = AL_FALSE;
	while (driver->stayAlive) {
		SDL_Delay(30);
		StackLock l(driver->musicMutex, "musicMutex in PlayListManager()");
		if (driver->MusicPlaying) {
			ALint state;
			alGetSourcei( driver->MusicSource, AL_SOURCE_STATE, &state );
			switch (state) {
				default:
					printMessage("OpenAL", "WARNING: Unhandled Music state", WHITE );
					printStatus("ERROR", YELLOW);
					driver->MusicPlaying = false;
					return -1;
				case AL_INITIAL:
					 {
						printMessage("OPENAL", "Music in INITIAL State. AutoStarting\n", WHITE );
						for (int i = 0; i < MUSICBUFFERS; i++) {
							driver->MusicReader->read_samples( ( short* ) driver->music_memory, ACM_BUFFERSIZE >> 1 );
							alBufferData( driver->MusicBuffer[i], AL_FORMAT_STEREO16,
								driver->music_memory, ACM_BUFFERSIZE,
								driver->MusicReader->get_samplerate() );
						}
						alSourceQueueBuffers( driver->MusicSource, MUSICBUFFERS, driver->MusicBuffer );
						if (alIsSource( driver->MusicSource )) {
							alSourcePlay( driver->MusicSource );
						}
					}
					break;
				case AL_STOPPED:
					printMessage("OpenAL", "WARNING: Buffer Underrun. AutoRestarting Stream Playback\n", WHITE );
					if (alIsSource( driver->MusicSource )) {
						alSourcePlay( driver->MusicSource );
					}
					break;
				case AL_PLAYING:
					break;
			}
			ALint processed;
			alGetSourcei( driver->MusicSource, AL_BUFFERS_PROCESSED, &processed );
			if (processed > 0) {
				buffersreturned += processed;
				while (processed) {
					ALuint BufferID;
					alSourceUnqueueBuffers( driver->MusicSource, 1, &BufferID );
					if (bFinished == AL_FALSE) {
						int size = ACM_BUFFERSIZE;
						int cnt = driver->MusicReader->read_samples( ( short* ) driver->music_memory, ACM_BUFFERSIZE >> 1 );
						size -= ( cnt * 2 );
						if (size != 0)
							bFinished = AL_TRUE;
						if (bFinished) {
							printMessage("OpenAL", "Playing Next Music\n", WHITE );
							core->GetMusicMgr()->PlayNext();
							if (driver->MusicReader) {
								printMessage( "OpenAL", "Queuing New Music\n", WHITE );
								driver->MusicReader->read_samples( ( short* ) ( driver->music_memory + ( cnt*2 ) ), size >> 1 );
								bFinished = AL_FALSE;
							} else {
								printMessage( "OpenAL", "No Other Music to play\n", WHITE );
								memset( driver->music_memory + ( cnt * 2 ), 0, size );
								driver->MusicPlaying = false;
							}
						}
						alBufferData( BufferID, AL_FORMAT_STEREO16, driver->music_memory, ACM_BUFFERSIZE, driver->MusicReader->get_samplerate() );
						alSourceQueueBuffers( driver->MusicSource, 1, &BufferID );
						processed--;
					}
				}
			}
		}
	}
	return 0;
}

//This one is used for movies, might be useful for others ?
void OpenALAudioDriver::QueueBuffer(int stream, unsigned short bits,
		        int channels, short* memory,
		        int size, int samplerate)
{
	ALuint Buffer;

	alGenBuffers(1, &Buffer);
	if ( alGetError() != AL_NO_ERROR ) {
		printMessage("OpenAL", "Unable to create buffer", WHITE );
		printStatus("ERROR", YELLOW);
		return;
	}

	alBufferData(Buffer, GetFormatEnum(channels, bits), memory, size, samplerate);

	alSourceQueueBuffers(streams[stream].Source, 1, &Buffer );

	ALenum state;
	alGetSourcei(streams[stream].Source, AL_SOURCE_STATE, &state);

	if(state != AL_PLAYING )
		alSourcePlay(streams[stream].Source);

	return;
}
