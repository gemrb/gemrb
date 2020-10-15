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

#include "OpenALAudio.h"

#include "GameData.h"

#include <cassert>
#include <cstdio>

using namespace GemRB;

template <typename TO, typename FROM>
TO fnptr_cast(FROM ptr) {
	union {
		FROM ptr_from;
		TO ptr_to;
	} u;
	u.ptr_from = ptr;

	return u.ptr_to;
}

#ifdef HAVE_OPENAL_EFX_H
static LPALGENEFFECTS alGenEffects = NULL;
static LPALDELETEEFFECTS alDeleteEffects = NULL;
static LPALISEFFECT alIsEffect = NULL;
static LPALGENAUXILIARYEFFECTSLOTS alGenAuxiliaryEffectSlots = NULL;
static LPALDELETEAUXILIARYEFFECTSLOTS alDeleteAuxiliaryEffectSlots = NULL;
static LPALEFFECTI alEffecti = NULL;
static LPALEFFECTF alEffectf = NULL;
static LPALAUXILIARYEFFECTSLOTI alAuxiliaryEffectSloti = NULL;
#endif

static bool checkALError(const char* msg, log_level level) {
	int error = alGetError();
	if (error != AL_NO_ERROR) {
		Log(level, "OpenAL", "%s: 0x%x - %s", msg, error, alGetString(error));
		return true;
	}
	return false;
}

static void showALCError(const char* msg, log_level level, ALCdevice *device) {
	int error = alcGetError(device);
	if (error != AL_NO_ERROR) {
		Log(level, "OpenAL", "%s: 0x%x", msg, error);
	} else {
		Log(level, "OpenAL", "%s", msg);
	}
}

void OpenALSoundHandle::SetPos(int XPos, int YPos) {
	if (!parent) return;

	ALfloat SourcePos[] = {
		(float) XPos, (float) YPos, 0.0f
	};

	alSourcefv(parent->Source, AL_POSITION, SourcePos);
	checkALError("Unable to set source position", WARNING);
}

bool OpenALSoundHandle::Playing() {
	if (!parent) return false;

	parent->ClearIfStopped();
	return parent != 0;
}

void OpenALSoundHandle::Stop() {
	if (!parent) return;

	parent->ForceClear();
}

void OpenALSoundHandle::StopLooping() {
	if (!parent) return;

	alSourcei(parent->Source, AL_LOOPING, 0);
	checkALError("Unable to stop audio loop", WARNING);
}

void AudioStream::ClearProcessedBuffers()
{
	ALint processed = 0;
	alGetSourcei( Source, AL_BUFFERS_PROCESSED, &processed );
	checkALError("Failed to get processed buffers", WARNING);

	if (processed > 0) {
		ALuint * b = new ALuint[processed];
		alSourceUnqueueBuffers( Source, processed, b );
		checkALError("Failed to unqueue buffers", WARNING);

		if (delete_buffers) {
#ifdef __APPLE__ // mac os x and iOS
			/* FIXME: hackish
				somebody with more knowledge than me could perhapps figure out
				why Apple's implementation of alSourceUnqueueBuffers seems to delay (threading thing?)
				and possible how better to deal with this.
			*/
			do{
				alDeleteBuffers(processed, b);
			}while(alGetError() != AL_NO_ERROR);
#else
			alDeleteBuffers(processed, b);
			checkALError("Failed to delete buffers", WARNING);
#endif
		}

		delete[] b;
	}

}

void AudioStream::ClearIfStopped()
{
	if (free || locked) return;

	if (!Source || !alIsSource(Source)) {
		checkALError("No AL Context", WARNING);
		return;
	}

	ALint state;
	alGetSourcei( Source, AL_SOURCE_STATE, &state );
	if (!checkALError("Failed to check source state", WARNING) &&
			state == AL_STOPPED)
	{
		ClearProcessedBuffers();
		alDeleteSources( 1, &Source );
		checkALError("Failed to delete source", WARNING);
		Source = 0;
		Buffer = 0;
		free = true;
		if (handle) { handle->Invalidate(); handle.release(); }
		ambient = false;
		locked = false;
		delete_buffers = false;
	}
}

void AudioStream::ForceClear()
{
	if (!Source || !alIsSource(Source)) return;

	alSourceStop(Source);
	checkALError("Failed to stop source", WARNING);
	ClearProcessedBuffers();
	ClearIfStopped();
}

OpenALAudioDriver::OpenALAudioDriver(void)
{
	alutContext = NULL;
	MusicPlaying = false;
	music_memory = (short*) malloc(ACM_BUFFERSIZE);
	MusicSource = num_streams = 0;
	memset(MusicBuffer, 0, MUSICBUFFERS*sizeof(ALuint));
	musicMutex = SDL_CreateMutex();
	ambim = NULL;
	musicThread = NULL;
	stayAlive = false;
	hasReverbProperties = false;
#ifdef HAVE_OPENAL_EFX_H
	hasEFX = false;
	efxEffectSlot = efxEffect = 0;
	memset(&reverbProperties.reverbData, 0, sizeof(reverbProperties.reverbData));
	reverbProperties.reverbDisabled = true;
#endif
}

void OpenALAudioDriver::PrintDeviceList ()
{
	char *deviceList;

	if (alcIsExtensionPresent(NULL, (ALchar*)"ALC_ENUMERATION_EXT") == AL_TRUE) { // try out enumeration extension
		Log(MESSAGE, "OpenAL", "Usable audio output devices:");
		deviceList = (char *)alcGetString(NULL, ALC_DEVICE_SPECIFIER);

		while(deviceList && *deviceList) {
			Log(MESSAGE,"OpenAL", "Devices: %s", deviceList);
			deviceList+=strlen(deviceList)+1;
		}
		return;
	}
	Log(MESSAGE, "OpenAL", "No device enumeration present.");
}

bool OpenALAudioDriver::Init(void)
{
	Log(MESSAGE, "OpenAL", "Initializing OpenAL driver:\nAL Version:%s\nAL Renderer:%s\nAL Vendor:%s",
		alGetString(AL_VERSION), alGetString(AL_RENDERER), alGetString(AL_VENDOR));

	ALCdevice *device;
	ALCcontext *context;

	device = alcOpenDevice (NULL);
	if (device == NULL) {
		showALCError("Failed to open device", ERROR, device);
		PrintDeviceList();
		return false;
	}

	context = alcCreateContext (device, NULL);
	if (context == NULL) {
		showALCError("Failed to create context", ERROR, device);
		alcCloseDevice (device);
		return false;
	}

	if (!alcMakeContextCurrent (context)) {
		showALCError("Failed to select context", ERROR, device);
		alcDestroyContext (context);
		alcCloseDevice (device);
		return false;
	}
	alutContext = context;

	//1 for speech
	int sources = CountAvailableSources(MAX_STREAMS+1);
	num_streams = sources - 1;

	Log(MESSAGE, "OpenAL", "Allocated %d streams.%s",
		num_streams, (num_streams < MAX_STREAMS ? " (Fewer than desired.)" : "" ));

	stayAlive = true;
#if	SDL_VERSION_ATLEAST(1, 3, 0)
	/* as of changeset 3a041d215edc SDL_CreateThread has a 'name' parameter */
	musicThread = SDL_CreateThread( MusicManager, "OpenALAudio", this );
#else
	musicThread = SDL_CreateThread( MusicManager, this );
#endif

	if (!InitEFX()) {
		Log(MESSAGE, "OpenAL", "EFX not available.");
	}

	ambim = new AmbientMgrAL;
	speech.free = true;
	speech.ambient = false;
	return true;
}

bool OpenALAudioDriver::InitEFX(void) {
#ifdef HAVE_OPENAL_EFX_H
	ALCdevice *device = alcGetContextsDevice (alutContext);
	ALCint auxSends = 0;
	hasEFX = false;

	if (AL_FALSE == alcIsExtensionPresent(device, "ALC_EXT_EFX")) {
		return false;
	}

	alcGetIntegerv(device, ALC_MAX_AUXILIARY_SENDS, 1, &auxSends);

	if (auxSends < 1) {
		return false;
	}

	alGenEffects = fnptr_cast<LPALGENEFFECTS>(alGetProcAddress("alGenEffects"));
	alDeleteEffects = fnptr_cast<LPALDELETEEFFECTS>(alGetProcAddress("alDeleteEffects"));
	alIsEffect = fnptr_cast<LPALISEFFECT>(alGetProcAddress("alIsEffect"));
	alGenAuxiliaryEffectSlots = fnptr_cast<LPALGENAUXILIARYEFFECTSLOTS>(alGetProcAddress("alGenAuxiliaryEffectSlots"));
	alDeleteAuxiliaryEffectSlots = fnptr_cast<LPALDELETEAUXILIARYEFFECTSLOTS>(alGetProcAddress("alDeleteAuxiliaryEffectSlots"));
	alEffecti = fnptr_cast<LPALEFFECTI>(alGetProcAddress("alEffecti"));
	alEffectf = fnptr_cast<LPALEFFECTF>(alGetProcAddress("alEffectf"));
	alAuxiliaryEffectSloti = fnptr_cast<LPALAUXILIARYEFFECTSLOTI>(alGetProcAddress("alAuxiliaryEffectSloti"));

	if (!alGenEffects || !alDeleteEffects || !alIsEffect) {
		return false;
	}

	alGenAuxiliaryEffectSlots(1, &efxEffectSlot);

	if (AL_NO_ERROR != alGetError()) {
		return false;
	}

	alGenEffects(1, &efxEffect);

	if (AL_NO_ERROR != alGetError()) {
		return false;
	}

	if (alIsEffect(efxEffect)) {
		alEffecti(efxEffect, AL_EFFECT_TYPE, AL_EFFECT_REVERB);

		if (AL_NO_ERROR == alGetError()) {
			alAuxiliaryEffectSloti(efxEffectSlot, AL_EFFECTSLOT_EFFECT, efxEffect);

			if (AL_NO_ERROR == alGetError()) {
				hasEFX = true;
				return true;
			}
		}
	}
#endif
	return false;
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

	checkALError("Error while auto-detecting number of sources", WARNING);

	// Return number of successfully allocated sources
	return i;
}

OpenALAudioDriver::~OpenALAudioDriver(void)
{
	if (!ambim) {
		// initialisation must have failed
		return;
	}

	stayAlive = false;
// AmigaOS4 can't kill threads and would just wait forever
#ifndef __amigaos4__
	SDL_WaitThread(musicThread, NULL);
#endif

	for(int i =0; i<num_streams; i++) {
		streams[i].ForceClear();
	}
	speech.ForceClear();
	ResetMusics();
	clearBufferCache(true);

#ifdef HAVE_OPENAL_EFX_H
	if (hasEFX) {
		alDeleteAuxiliaryEffectSlots(1, &efxEffectSlot);
		alDeleteEffects(1, &efxEffect);
	}
#endif

	ALCdevice *device;

	alcMakeContextCurrent (NULL);

	device = alcGetContextsDevice (alutContext);
	alcDestroyContext (alutContext);
	if (alcGetError (device) == ALC_NO_ERROR) {
		alcCloseDevice (device);
	}
	alutContext = NULL;

	SDL_DestroyMutex(musicMutex);
	musicMutex = NULL;

	free(music_memory);

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
	alGenBuffers(1, &Buffer);
	if (checkALError("Unable to create sound buffer", ERROR)) {
		return 0;
	}

	ResourceHolder<SoundMgr> acm = GetResourceHolder<SoundMgr>(ResRef);
	if (!acm) {
		alDeleteBuffers( 1, &Buffer );
		checkALError("Unable to delete buffer!", ERROR);
		return 0;
	}
	int cnt = acm->get_length();
	unsigned int riff_chans = acm->get_channels();
	int samplerate = acm->get_samplerate();
	//multiply always by 2 because it is in 16 bits
	int rawsize = cnt * 2;
	short* memory = (short*) malloc(rawsize);
	//multiply always with 2 because it is in 16 bits
	unsigned int cnt1 = acm->read_samples( memory, cnt ) * 2;
	//Sound Length in milliseconds
	time_length = ((cnt / riff_chans) * 1000) / samplerate;
	//it is always reading the stuff into 16 bits
	alBufferData( Buffer, GetFormatEnum( riff_chans, 16 ), memory, cnt1, samplerate );
	free(memory);

	if (checkALError("Unable to fill buffer", ERROR)) {
		alDeleteBuffers( 1, &Buffer );
		checkALError("Error deleting buffer", WARNING);
		return 0;
	}

	e = new CacheEntry;
	e->Buffer = Buffer;
	e->Length = time_length;

	buffercache.SetAt(ResRef, (void*)e);
	//print("LoadSound: added %s to cache: %d. Cache size now %d", ResRef, e->Buffer, buffercache.GetCount());

	if (buffercache.GetCount() > BUFFER_CACHE_SIZE) {
		evictBuffer();
	}
	return Buffer;
}

Holder<SoundHandle> OpenALAudioDriver::Play(const char* ResRef, unsigned int channel, int XPos, int YPos,
	unsigned int flags, unsigned int *length)
{
	ALuint Buffer;
	unsigned int time_length;

	if (ResRef == NULL || !ResRef[0]) {
		if((flags & GEM_SND_SPEECH) && (speech.Source && alIsSource(speech.Source))) {
			//So we want him to be quiet...
			alSourceStop( speech.Source );
			checkALError("Unable to stop speech", WARNING);
			speech.ClearProcessedBuffers();
		}
		return Holder<SoundHandle>();
	}

	Buffer = loadSound( ResRef, time_length );
	if (Buffer == 0) {
		return Holder<SoundHandle>();
	}

	if (length) {
		*length = time_length;
	}

	ALfloat SourcePos[] = {
		(float) XPos, (float) YPos, 0.0f
	};
	ALfloat SourceVel[] = {
		0.0f, 0.0f, 0.0f
	};

	ieDword volume = 100;
	ALint loop = (flags & GEM_SND_LOOPING) ? 1 : 0;

	AudioStream* stream = NULL;

	if (flags & GEM_SND_SPEECH) {
		stream = &speech;

		if (!(flags & GEM_SND_QUEUE)) {
			//speech has a single channel, if a new speech started
			//we stop the previous one

			if(!speech.free && (speech.Source && alIsSource(speech.Source))) {
				alSourceStop( speech.Source );
				checkALError("Unable to stop speech", WARNING);
				speech.ClearProcessedBuffers();
			}
		}

		core->GetDictionary()->Lookup("Volume Voices", volume);

		loop = 0; // Speech ignores GEM_SND_LOOPING
	} else {
		// do we want to be able to queue sfx too? not so far. How would we?
		for (int i = 0; i < num_streams; i++) {
			streams[i].ClearIfStopped();
			if (streams[i].free) {
				stream = &streams[i];
				break;
			}
		}

		core->GetDictionary()->Lookup("Volume SFX", volume);

		if (stream == NULL) {
			// Failed to assign new sound.
			// The buffercache will handle deleting Buffer.
			return Holder<SoundHandle>();
		}
	}

	assert(stream);
	ALuint Source = stream->Source;

	if(!Source || !alIsSource(Source)) {
		alGenSources( 1, &Source );
		if (checkALError("Error creating source", ERROR)) {
			return Holder<SoundHandle>();
		}
	}

	alSourcef( Source, AL_PITCH, 1.0f );
	alSourcefv( Source, AL_VELOCITY, SourceVel );
	alSourcei( Source, AL_LOOPING, loop);
	alSourcef( Source, AL_REFERENCE_DISTANCE, REFERENCE_DISTANCE );
	alSourcef( Source, AL_GAIN, 0.01f * (volume / 100.0f) * GetVolume(channel) );
	alSourcei( Source, AL_SOURCE_RELATIVE, flags & GEM_SND_RELATIVE );
	alSourcefv( Source, AL_POSITION, SourcePos );
	checkALError("Unable to set audio parameters", WARNING);

#ifdef HAVE_OPENAL_EFX_H
	ieDword efxSetting;
	core->GetDictionary()->Lookup("Environmental Audio", efxSetting);

	if (efxSetting && hasReverbProperties && ((0 != XPos && 0 != YPos) || (flags & GEM_SND_RELATIVE))) {
		alSource3i(Source, AL_AUXILIARY_SEND_FILTER, efxEffectSlot, 0, 0);
	} else {
		alSource3i(Source, AL_AUXILIARY_SEND_FILTER, 0, 0, 0);
	}
#endif

	assert(!stream->delete_buffers);

	stream->Source = Source;
	stream->free = false;

	if (QueueALBuffer(Source, Buffer) != GEM_OK) {
		return Holder<SoundHandle>();
	}

	stream->handle = new OpenALSoundHandle(stream);
	return stream->handle.get();
}

void OpenALAudioDriver::UpdateVolume(unsigned int flags)
{
	ieDword volume;

	if (flags & GEM_SND_VOL_MUSIC) {
		SDL_mutexP( musicMutex );
		core->GetDictionary()->Lookup("Volume Music", volume);
		if (MusicSource && alIsSource(MusicSource))
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
	SDL_mutexP(musicMutex);
	MusicPlaying = false;
	if (MusicSource && alIsSource(MusicSource)) {
		alSourceStop(MusicSource);
		checkALError("Unable to stop music source", WARNING);
		alDeleteSources(1, &MusicSource );
		checkALError("Unable to delete music source", WARNING);
		MusicSource = 0;
		for (int i=0; i<MUSICBUFFERS; i++) {
			if (alIsBuffer(MusicBuffer[i])) {
				alDeleteBuffers(1, MusicBuffer+i);
				checkALError("Unable to delete music buffer", WARNING);
			}
		}
	}
	SDL_mutexV( musicMutex );
}

bool OpenALAudioDriver::Play()
{
	if (!MusicReader) return false;

	SDL_mutexP( musicMutex );
	if (!MusicPlaying)
		MusicPlaying = true;
	SDL_mutexV( musicMutex );

	return true;
}

bool OpenALAudioDriver::Stop()
{
	SDL_mutexP( musicMutex );
	if (!MusicSource || !alIsSource( MusicSource )) {
		SDL_mutexV( musicMutex );
		return false;
	}
	alSourceStop( MusicSource );
	checkALError("Unable to stop music source", WARNING);
	MusicPlaying = false;
	alDeleteSources( 1, &MusicSource );
	checkALError("Unable to delete music source", WARNING);
	MusicSource = 0;
	SDL_mutexV( musicMutex );
	return true;
}

bool OpenALAudioDriver::Pause()
{
	SDL_mutexP( musicMutex );
	if (!MusicSource || !alIsSource( MusicSource )) {
		SDL_mutexV( musicMutex );
		return false;
	}
	alSourcePause(MusicSource);
	checkALError("Unable to pause music source", WARNING);
	MusicPlaying = false;
	SDL_mutexV( musicMutex );
	((AmbientMgrAL*) ambim)->deactivate();
#ifdef ANDROID
#if SDL_COMPILEDVERSION < SDL_VERSIONNUM(1,3,0)
	al_android_pause_playback(); //call AudioTrack.pause() from JNI
#endif
#endif
	return true;
}

bool OpenALAudioDriver::Resume()
{
#ifdef ANDROID
#if SDL_COMPILEDVERSION < SDL_VERSIONNUM(1,3,0)
	al_android_resume_playback(); //call AudioTrack.play() from JNI
#endif
#endif
	SDL_mutexP( musicMutex );
	if (!MusicSource || !alIsSource( MusicSource )) {
		SDL_mutexV( musicMutex );
		return false;
	}
	alSourcePlay(MusicSource);
	checkALError("Unable to resume music source", WARNING);
	MusicPlaying = true;
	SDL_mutexV( musicMutex );
	((AmbientMgrAL*) ambim)->activate();
	return true;
}

int OpenALAudioDriver::CreateStream(Holder<SoundMgr> newMusic)
{
	StackLock l(musicMutex, "musicMutex in CreateStream()");

	// Free old MusicReader
	MusicReader = newMusic;
	if (!MusicReader) {
		MusicPlaying = false;
	}

	if (MusicBuffer[0] == 0) {
		alGenBuffers( MUSICBUFFERS, MusicBuffer );
		if (checkALError("Unable to create music buffers", ERROR)) {
			return -1;
		}
	}

	if (MusicSource == 0) {
		alGenSources( 1, &MusicSource );
		if (checkALError("Unable to create music source", ERROR)) {
			alDeleteBuffers(MUSICBUFFERS, MusicBuffer);
			return -1;
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
		checkALError("Unable to set music parameters", WARNING);
	}

	return 0;
}

void OpenALAudioDriver::UpdateListenerPos(int XPos, int YPos )
{
	alListener3f( AL_POSITION, (float) XPos, (float) YPos, LISTENER_HEIGHT );
	checkALError("Unable to update listener position.", WARNING);
}

void OpenALAudioDriver::GetListenerPos(int &XPos, int &YPos )
{
	ALfloat listen[3];
	alGetListenerfv( AL_POSITION, listen );
	if (checkALError("Unable to get listener pos", ERROR)) return;
	XPos = (int) listen[0];
	YPos = (int) listen[1];
}

bool OpenALAudioDriver::ReleaseStream(int stream, bool HardStop)
{
	if (stream < 0 || streams[stream].free || !streams[stream].locked)
		return false;
	streams[stream].locked = false;
	if (!HardStop) {
		// it's now unlocked, so it will automatically be reclaimed when needed
		return true;
	}

	ALuint Source = streams[stream].Source;
	alSourceStop(Source);
	checkALError("Unable to stop source", WARNING);
	streams[stream].ClearIfStopped();
	return true;
}

//This one is used for movies and ambients.
int OpenALAudioDriver::SetupNewStream( ieWord x, ieWord y, ieWord z,
		            ieWord gain, bool point, int ambientRange)
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
	if (stream == -1) {
		Log(ERROR, "OpenAL", "No available audio streams out of %d", num_streams);
		return -1;
	}

	ALuint source;
	alGenSources(1, &source);
	if (checkALError("Unable to create new source", ERROR)) {
		return -1;
	}

	ALfloat position[] = { (float) x, (float) y, (float) z };
	alSourcef( source, AL_PITCH, 1.0f );
	alSourcefv( source, AL_POSITION, position );
	alSourcef( source, AL_GAIN, 0.01f * gain );
	// AL_REFERENCE_DISTANCE is the distance under which the volume would normally drop by half
	alSourcei(source, AL_REFERENCE_DISTANCE, ambientRange > 0 ? ambientRange : REFERENCE_DISTANCE);
	alSourcei( source, AL_ROLLOFF_FACTOR, point ? 1 : 0 );
	alSourcei( source, AL_LOOPING, 0 );
	checkALError("Unable to set stream parameters", WARNING);

	streams[stream].Buffer = 0;
	streams[stream].Source = source;
	streams[stream].free = false;
	streams[stream].ambient = ambientRange > 0;
	streams[stream].locked = true;

	return stream;
}

int OpenALAudioDriver::QueueAmbient(int stream, const char* sound)
{
	if (streams[stream].free || !streams[stream].ambient)
		return -1;

	ALuint source = streams[stream].Source;

	// first dequeue any processed buffers
	streams[stream].ClearProcessedBuffers();

	if (sound == 0)
		return 0;

	unsigned int time_length;
	ALuint Buffer = loadSound(sound, time_length);
	if (0 == Buffer) {
		return -1;
	}

	assert(!streams[stream].delete_buffers);

	if (QueueALBuffer(source, Buffer) != GEM_OK) {
		return GEM_ERROR;
	}

	return time_length;
}

void OpenALAudioDriver::SetAmbientStreamVolume(int stream, int volume)
{
	if (streams[stream].free || !streams[stream].ambient)
		return;

	ALuint source = streams[stream].Source;
	alSourcef( source, AL_GAIN, 0.01f * volume );
	checkALError("Unable to set ambient volume", WARNING);
}

void OpenALAudioDriver::SetAmbientStreamPitch(int stream, int pitch)
{
	if (streams[stream].free || !streams[stream].ambient)
		return;

	ALuint source = streams[stream].Source;
	alSourcef( source, AL_PITCH, 0.01f * pitch );
	checkALError("Unable to set ambient pitch", WARNING);
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

			delete e;
			buffercache.Remove(k);

			//print("Removed buffer %s from ACMImp cache", k);
			break;
		}
		++n;
	}

	return res;
}

void OpenALAudioDriver::clearBufferCache(bool force)
{
	// Room for optimization: any method of iterating over the buffers
	// would suffice. It doesn't have to be in LRU-order.
	void* p;
	const char* k;
	int n = 0;
	while (buffercache.getLRU(n, k, p)) {
		CacheEntry* e = (CacheEntry*)p;
		alDeleteBuffers(1, &e->Buffer);
		if (force || alGetError() == AL_NO_ERROR) {
			delete e;
			buffercache.Remove(k);
		} else
			++n;
	}
}

ALenum OpenALAudioDriver::GetFormatEnum(int channels, int bits) const
{
	switch (channels) {
		case 1:
			if (bits == 8)
				return AL_FORMAT_MONO8;
			else
				return AL_FORMAT_MONO16;

		case 2:
			if (bits == 8)
				return AL_FORMAT_STEREO8;
			else
				return AL_FORMAT_STEREO16;
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
			if (checkALError("Unable to query music source state", ERROR)) {
				driver->MusicPlaying = false;
				return -1;
			}
			switch (state) {
				default:
					Log(ERROR, "OpenAL", "Unhandled Music state '%d'.", state);
				// intentional fallthrough
				case AL_PAUSED:
					driver->MusicPlaying = false;
					return -1;
				case AL_INITIAL:
					Log(MESSAGE, "OpenAL", "Music in INITIAL State. AutoStarting");
					// ensure that MusicSource has no buffers attached by passing "NULL" buffer
					alSourcei(driver->MusicSource, AL_BUFFER, 0);
					checkALError("Unable to detach buffers from music source.", WARNING);
					for (int i = 0; i < MUSICBUFFERS; i++) {
						driver->MusicReader->read_samples( driver->music_memory, ACM_BUFFERSIZE >> 1 );
						alBufferData( driver->MusicBuffer[i], AL_FORMAT_STEREO16,
							driver->music_memory, ACM_BUFFERSIZE,
							driver->MusicReader->get_samplerate() );
					}
					checkALError("Unable to buffer data.", ERROR);
					// FIXME: determine if we should error out if any data fails to buffer
					alSourceQueueBuffers( driver->MusicSource, MUSICBUFFERS, driver->MusicBuffer );
					if (!checkALError("Unable to queue buffer.", ERROR)) {
						if (driver->MusicSource && alIsSource( driver->MusicSource )) {
							alSourcePlay( driver->MusicSource );
							if (!checkALError("Error playing music source", ERROR)) {
								// no errors happened
								bFinished = AL_FALSE;
								break;
							}
						}
					}
					// if all had gone well we would have broken out
					driver->MusicPlaying = false;
					return -1;
				case AL_STOPPED:
					Log(MESSAGE, "OpenAL", "WARNING: Buffer Underrun. AutoRestarting Stream Playback");
					if (driver->MusicSource && alIsSource( driver->MusicSource )) {
						alSourcePlay( driver->MusicSource );
						checkALError("Error playing music source", ERROR);
					}
					break;
				case AL_PLAYING:
					break;
			}
			ALint processed;
			alGetSourcei( driver->MusicSource, AL_BUFFERS_PROCESSED, &processed );
			if (checkALError("Unable to query music source state", ERROR)) {
				driver->MusicPlaying = false;
				return -1;
			}
			if (processed > 0) {
				buffersreturned += processed;
				while (processed) {
					ALuint BufferID;
					alSourceUnqueueBuffers( driver->MusicSource, 1, &BufferID );
					if (checkALError("Unable to unqueue music buffers", ERROR)) {
						driver->MusicPlaying = false;
						return -1;
					}
					if (bFinished == AL_FALSE) {
						int size = ACM_BUFFERSIZE;
						int cnt = driver->MusicReader->read_samples( driver->music_memory, ACM_BUFFERSIZE >> 1 );
						size -= ( cnt * 2 );
						if (size != 0)
							bFinished = AL_TRUE;
						if (bFinished) {
							Log(MESSAGE, "OpenAL", "Playing Next Music");
							core->GetMusicMgr()->PlayNext();
							if (driver->MusicPlaying) {
								Log(MESSAGE, "OpenAL", "Queuing New Music");
								driver->MusicReader->read_samples( ( driver->music_memory + cnt ), size >> 1 );
								bFinished = AL_FALSE;
							} else {
								Log(MESSAGE, "OpenAL", "No Other Music to play");
								memset( driver->music_memory + cnt, 0, size );
								driver->MusicPlaying = false;
								break;
							}
						}
						alBufferData( BufferID, AL_FORMAT_STEREO16, driver->music_memory, ACM_BUFFERSIZE, driver->MusicReader->get_samplerate() );
						if (checkALError("Unable to buffer music data", ERROR)) {
							driver->MusicPlaying = false;
							return -1;
						}
						alSourceQueueBuffers( driver->MusicSource, 1, &BufferID );
						if (checkALError("Unable to queue music buffers", ERROR)) {
							driver->MusicPlaying = false;
							return -1;
						}
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
	streams[stream].delete_buffers = true;
	streams[stream].ClearProcessedBuffers();

	ALuint Buffer;
	alGenBuffers(1, &Buffer);
	if (checkALError("Unable to create buffer", ERROR)) {
		return;
	}

	alBufferData(Buffer, GetFormatEnum(channels, bits), memory, size, samplerate);
	if (checkALError("Unable to buffer data", ERROR)) {
		alDeleteBuffers(1, &Buffer);
		return;
	}

	QueueALBuffer(streams[stream].Source, Buffer);
}

// !!!!!!!!!!!!!!!
// Private Methods
// !!!!!!!!!!!!!!!

int OpenALAudioDriver::QueueALBuffer(ALuint source, ALuint buffer)
{
#ifdef DEBUG_AUDIO
	ALint frequency, bits, channels;
	alGetBufferi(buffer, AL_FREQUENCY, &frequency);
	alGetBufferi(buffer, AL_BITS, &bits);
	alGetBufferi(buffer, AL_CHANNELS, &channels);
	checkALError("Error querying buffer properties.", WARNING);
	Log(DEBUG, "OpenAL", "Attempting to buffer audio source:%d\nFrequency:%d\nBits:%d\nChannels:%d",
		source, frequency, bits, channels);
#endif
	ALint type;
	alGetSourcei(source, AL_SOURCE_TYPE, &type);
	if (type == AL_STATIC || checkALError("Cannot get AL source type.", ERROR)) {
		Log(ERROR, "OpenAL", "Cannot queue a buffer to a static source.");
		return GEM_ERROR;
	}
	alSourceQueueBuffers(source, 1, &buffer);
	if (checkALError("Unable to queue buffer", ERROR)) {
		return GEM_ERROR;
	}

	ALenum state;
	alGetSourcei(source, AL_SOURCE_STATE, &state);
	if (checkALError("Unable to query source state", ERROR)) {
		return GEM_ERROR;
	}

	// queueing always implies playing for us
	if (state != AL_PLAYING ) {
		alSourcePlay(source);
		if (checkALError("Unable to play source", ERROR)) {
			return GEM_ERROR;
		}
	}
	return GEM_OK;
}

#ifdef HAVE_OPENAL_EFX_H
void OpenALAudioDriver::UpdateMapAmbient(MapReverb& mapReverb) {
	if (hasEFX) {
		mapReverb.getReverbProperties(reverbProperties);
		hasReverbProperties = true;

		alDeleteEffects(1, &efxEffect);
		alGenEffects(1, &efxEffect);

		if (!reverbProperties.reverbDisabled) {
			alEffecti(efxEffect, AL_EFFECT_TYPE, AL_EFFECT_REVERB);

			alEffectf(efxEffect, AL_REVERB_DENSITY, reverbProperties.reverbData.flDensity);
			alEffectf(efxEffect, AL_REVERB_DIFFUSION, reverbProperties.reverbData.flDiffusion);
			alEffectf(efxEffect, AL_REVERB_GAIN, reverbProperties.reverbData.flGain);
			alEffectf(efxEffect, AL_REVERB_GAINHF, reverbProperties.reverbData.flGainHF);
			alEffectf(efxEffect, AL_REVERB_DECAY_TIME, reverbProperties.reverbData.flDecayTime);
			alEffectf(efxEffect, AL_REVERB_DECAY_HFRATIO, reverbProperties.reverbData.flDecayHFRatio);
			alEffectf(efxEffect, AL_REVERB_REFLECTIONS_GAIN, reverbProperties.reverbData.flReflectionsGain);
			alEffectf(efxEffect, AL_REVERB_REFLECTIONS_DELAY, reverbProperties.reverbData.flReflectionsDelay);
			alEffectf(efxEffect, AL_REVERB_LATE_REVERB_GAIN, reverbProperties.reverbData.flLateReverbGain);
			alEffectf(efxEffect, AL_REVERB_LATE_REVERB_DELAY, reverbProperties.reverbData.flLateReverbDelay);
			alEffectf(efxEffect, AL_REVERB_AIR_ABSORPTION_GAINHF, reverbProperties.reverbData.flAirAbsorptionGainHF);
			alEffectf(efxEffect, AL_REVERB_ROOM_ROLLOFF_FACTOR, reverbProperties.reverbData.flRoomRolloffFactor);
			alEffecti(efxEffect, AL_REVERB_DECAY_HFLIMIT, reverbProperties.reverbData.iDecayHFLimit);
		} else {
			alEffecti(efxEffect, AL_EFFECT_TYPE, 0);
		}

		alAuxiliaryEffectSloti(efxEffectSlot, AL_EFFECTSLOT_EFFECT, efxEffect);
	}
}
#else
void OpenALAudioDriver::UpdateMapAmbient(MapReverb&) {
}
#endif

#include "plugindef.h"

GEMRB_PLUGIN(0x27DD67E0, "OpenAL Audio Driver")
PLUGIN_DRIVER(OpenALAudioDriver, "openal")
END_PLUGIN()
