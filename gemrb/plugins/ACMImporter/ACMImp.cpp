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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/ACMImporter/ACMImp.cpp,v 1.18 2003/12/02 14:56:22 balrog994 Exp $
 *
 */

#include "../../includes/win32def.h"
#include "../Core/Interface.h"
#include "ACMImp.h"
#include "acmsound.h"
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#ifdef WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

#define DisplayALError(string, error) printf("%s0x%04X", string, error);

static AudioStream streams[MAX_STREAMS];
static AudioStream musics[MAX_STREAMS];
static int BufferDuration, BufferStartPlayTime;
static SDL_mutex *musicMutex;
static bool musicPlaying;
static int musicIndex;
static SDL_Thread * musicThread;

void ACMImp::clearstreams(bool free) {
	for(int i = 0; i < MAX_STREAMS; i++) {
		if(!musics[i].free && free) {
			alDeleteBuffers(1, &musics[i].Buffer);
			alDeleteSources(1, &musics[i].Source);
		}
		streams[i].free = true;
	}
}

int ACMImp::PlayListManager(void * data)
{
	while(true) {
		SDL_mutexP(musicMutex);
		if(musicPlaying) {
			unsigned long volume;
			if(!core->GetDictionary()->Lookup("Volume Music", volume)) {
				core->GetDictionary()->SetAt("Volume Music", 100);
				volume = 100;
			}
			alSourcef(musics[musicIndex].Source, AL_GAIN, volume/100.0);
#ifdef WIN32
			unsigned long time = GetTickCount();
#else
			struct timeval tv;
			gettimeofday(&tv, NULL);
			unsigned long time = (tv.tv_usec/1000) + (tv.tv_sec*1000);
#endif
			if((time - BufferStartPlayTime) > (BufferDuration-100))
				core->GetMusicMgr()->PlayNext();
		}
		SDL_mutexV(musicMutex);
		SDL_Delay(1);
	}
	return 0;
}

ACMImp::ACMImp(void)
{
	for(int i = 0; i < MAX_STREAMS; i++) {
		streams[i].free = true;
		musics[i].free = true;
	}
	musicMutex = SDL_CreateMutex();
	musicPlaying = false;
	musicThread = SDL_CreateThread(PlayListManager, NULL);
}

ACMImp::~ACMImp(void)
{
	clearstreams(true);
	SDL_KillThread(musicThread);
	SDL_DestroyMutex(musicMutex);
}

bool ACMImp::Init(void)
{
	alutInit(0, NULL);
	ALenum error = alGetError();
	if(error != AL_NO_ERROR)
		return false;
	return true;
}

unsigned long ACMImp::Play(const char * ResRef)
{
	char path[_MAX_PATH];
	strcpy(path, core->CachePath);
	strcat(path, ResRef);
	strcat(path, core->TypeExt(IE_WAV_CLASS_ID));
	FILE * str = fopen(path, "rb");
	if(str != NULL) {
		fclose(str);

		ALuint Buffer;
		ALuint Source;
		ALfloat SourcePos[] = {0.0f, 0.0f, 0.0f};
		ALfloat SourceVel[] = {0.0f, 0.0f, 0.0f};
		ALsizei size, freq;
		ALenum format;
		ALboolean loop;
		ALvoid *data;
		ALenum error;
		alGenBuffers(1, &Buffer);
		if((error = alGetError()) != AL_NO_ERROR) {
			DisplayALError("[ACMImp::Play] alGenBuffers : ", error);
		}
		{
			alutLoadWAVFile(path, &format, &data, &size, &freq, &loop);
			if((error = alGetError()) != AL_NO_ERROR) {
				DisplayALError("[ACMImp::Play] alutLoadWAVFile : ", error);
			}
			alBufferData(Buffer, format, data, size, freq);
			if((error = alGetError()) != AL_NO_ERROR) {
				DisplayALError("[ACMImp::Play] alBufferData : ", error);
			}
			alutUnloadWAV(format, data, size, freq);
			if((error = alGetError()) != AL_NO_ERROR) {
				DisplayALError("[ACMImp::Play] alutUnloadWAV : ", error);
			}
			alGenSources(1, &Source);
			if((error = alGetError()) != AL_NO_ERROR) {
				DisplayALError("[ACMImp::Play] alGenSources : ", error);
			}

			alSourcei (Source, AL_BUFFER,   Buffer   );
			alSourcef (Source, AL_PITCH,    1.0f     );
			alSourcef (Source, AL_GAIN,     1.0f     );
			alSourcefv(Source, AL_POSITION, SourcePos);
			alSourcefv(Source, AL_VELOCITY, SourceVel);
			alSourcei (Source, AL_LOOPING,  loop     );

			if (alGetError() != AL_NO_ERROR)
				return 0xffffffff;

			for(int i = 0; i < MAX_STREAMS; i++) {
				if(!streams[i].free) {
					ALint state;
					alGetSourcei(streams[i].Source, AL_SOURCE_STATE, &state);
					if(state == AL_STOPPED) {
						alDeleteBuffers(1, &streams[i].Buffer);
						alDeleteSources(1, &streams[i].Source);
						streams[i].Buffer = Buffer;
						streams[i].Source = Source;
						streams[i].playing = false;
						alSourcePlay(Source);
						return 0;
					}
				} else {
					streams[i].Buffer = Buffer;
					streams[i].Source = Source;
					streams[i].free = false;
					streams[i].playing = false;
					alSourcePlay(Source);
					return 0;
				}
			}

			alDeleteBuffers(1, &Buffer);
			alDeleteSources(1, &Source);
		}
		return 0xffffffff;
	}
	char tmpFile[_MAX_PATH];
	strcpy(tmpFile, core->CachePath);
	strcat(tmpFile, ResRef);
	strcat(tmpFile, ".tmp");
	DataStream * dstr = core->GetResourceMgr()->GetResource(ResRef, IE_WAV_CLASS_ID);
	if(!AcmToWav(dstr, tmpFile, path)) {
		printMessage("ACMImporter", "ACM Decompression Failed\n", LIGHT_RED);
		return 0xffffffff;
	}
	delete(dstr);
	str = fopen(path, "rb");
	if(str != NULL) {
		fclose(str);
		ALuint Buffer;
		ALuint Source;
		ALfloat SourcePos[] = {0.0f, 0.0f, 0.0f};
		ALfloat SourceVel[] = {0.0f, 0.0f, 0.0f};
		ALsizei size, freq;
		ALenum format;
		ALboolean loop;
		ALvoid *data;
		ALenum error;
		alGenBuffers(1, &Buffer);
		if((error = alGetError()) != AL_NO_ERROR) {
			DisplayALError("[ACMImp::Play] alGenBuffers : ", error);
		}
		{
			alutLoadWAVFile(path, &format, &data, &size, &freq, &loop);
			if((error = alGetError()) != AL_NO_ERROR) {
				DisplayALError("[ACMImp::Play] alutLoadWAVFile : ", error);
			}
			alBufferData(Buffer, format, data, size, freq);
			if((error = alGetError()) != AL_NO_ERROR) {
				DisplayALError("[ACMImp::Play] alBufferData : ", error);
			}
			alutUnloadWAV(format, data, size, freq);
			if((error = alGetError()) != AL_NO_ERROR) {
				DisplayALError("[ACMImp::Play] alutUnloadWAV : ", error);
			}
			alGenSources(1, &Source);
			if((error = alGetError()) != AL_NO_ERROR) {
				DisplayALError("[ACMImp::Play] alGenSources : ", error);
			}


			alSourcei (Source, AL_BUFFER,   Buffer   );
			alSourcef (Source, AL_PITCH,    1.0f     );
			alSourcef (Source, AL_GAIN,     1.0f     );
			alSourcefv(Source, AL_POSITION, SourcePos);
			alSourcefv(Source, AL_VELOCITY, SourceVel);
			alSourcei (Source, AL_LOOPING,  loop     );

			if (alGetError() != AL_NO_ERROR)
				return 0xffffffff;

			for(int i = 0; i < MAX_STREAMS; i++) {
				if(!streams[i].free) {
					ALint state;
					alGetSourcei(streams[i].Source, AL_SOURCE_STATE, &state);
					if(state == AL_STOPPED) {
						alDeleteBuffers(1, &streams[i].Buffer);
						alDeleteSources(1, &streams[i].Source);
						streams[i].Buffer = Buffer;
						streams[i].Source = Source;
						streams[i].playing = false;
						alSourcePlay(Source);
						return 0;
					}
				} else {
					streams[i].Buffer = Buffer;
					streams[i].Source = Source;
					streams[i].free = false;
					streams[i].playing = false;
					alSourcePlay(Source);
					return 0;
				}
			}

			alDeleteBuffers(1, &Buffer);
			alDeleteSources(1, &Source);
		}
		return 0xffffffff;
	}
	printMessage("ACMImporter", "Cannot find decompressed file in Cache\n", LIGHT_RED);
	return 0xffffffff;
}

unsigned long ACMImp::LoadFile(const char * filename)
{
  char tmp[_MAX_PATH];
  ExtractFileFromPath(tmp, filename);
  char * ResRef = strtok(tmp, ".");
  char outFile[_MAX_PATH];
	strcpy(outFile, core->CachePath);
	strcat(outFile, ResRef);
	strcat(outFile, core->TypeExt(IE_WAV_CLASS_ID));
	FILE * str = fopen(outFile, "rb");
	if(str != NULL) {
		fclose(str);
		
		ALuint Buffer;
		ALuint Source;
		ALfloat SourcePos[] = {0.0f, 0.0f, 0.0f};
		ALfloat SourceVel[] = {0.0f, 0.0f, 0.0f};
		ALsizei size, freq;
		ALenum format;
		ALboolean loop;
		ALvoid *data;
		ALenum error;
		alGenBuffers(1, &Buffer);
		if((error = alGetError()) != AL_NO_ERROR) {
			DisplayALError("[ACMImp::LoadFile] alGenBuffers : ", error);
		}
		{
			alutLoadWAVFile(outFile, &format, &data, &size, &freq, &loop);
			if((error = alGetError()) != AL_NO_ERROR) {
				DisplayALError("[ACMImp::LoadFile] alutLoadWAVFile : ", error);
			}
			alBufferData(Buffer, format, data, size, freq);
			if((error = alGetError()) != AL_NO_ERROR) {
				DisplayALError("[ACMImp::LoadFile] alBufferData : ", error);
			}
			alutUnloadWAV(format, data, size, freq);
			if((error = alGetError()) != AL_NO_ERROR) {
				DisplayALError("[ACMImp::LoadFile] alutUnloadWAV : ", error);
			}
			alGenSources(1, &Source);
			if((error = alGetError()) != AL_NO_ERROR) {
				DisplayALError("[ACMImp::LoadFile] alGenSources : ", error);
			}

			alSourcei (Source, AL_BUFFER,   Buffer   );
			alSourcef (Source, AL_PITCH,    1.0f     );
			alSourcef (Source, AL_GAIN,     1.0f     );
			alSourcefv(Source, AL_POSITION, SourcePos);
			alSourcefv(Source, AL_VELOCITY, SourceVel);
			alSourcei (Source, AL_LOOPING,  loop     );

			if (alGetError() != AL_NO_ERROR)
				return 0xffffffff;

			for(int i = 0; i < MAX_STREAMS; i++) {
				if(musics[i].free) {
					musics[i].Buffer = Buffer;
					musics[i].Source = Source;
					musics[i].Duration = (size/(double)(freq*4))*1000;
					musics[i].free = false;
					musics[i].playing = false;
					return i;
				}
			}

			alDeleteBuffers(1, &Buffer);
			alDeleteSources(1, &Source);
		}

		return 0xffffffff;
	}
	char tmpFile[_MAX_PATH];
	strcpy(tmpFile, core->CachePath);
	strcat(tmpFile, ResRef);
	strcat(tmpFile, ".tmp");
	FileStream * dstr = new FileStream();
	if(!dstr->Open(filename, true)) {
		delete(dstr);
		return 0xffffffff;
	}
	if(!AcmToWav(dstr, tmpFile, outFile)) {
		printMessage("ACMImporter", "ACM Decompression Failed\n", LIGHT_RED);
		delete(dstr);
		return 0xffffffff;
	}
	delete(dstr);
	str = fopen(outFile, "rb");
	if(str != NULL) {
		fclose(str);
		
		ALuint Buffer;
		ALuint Source;
		ALfloat SourcePos[] = {0.0f, 0.0f, 0.0f};
		ALfloat SourceVel[] = {0.0f, 0.0f, 0.0f};
		ALsizei size, freq;
		ALenum format;
		ALboolean loop;
		ALvoid *data;
		ALenum error;
		alGenBuffers(1, &Buffer);
		if((error = alGetError()) != AL_NO_ERROR) {
			DisplayALError("[ACMImp::LoadFile] alGenBuffers : ", error);
		}
		{
			alutLoadWAVFile(outFile, &format, &data, &size, &freq, &loop);
			if((error = alGetError()) != AL_NO_ERROR) {
				DisplayALError("[ACMImp::LoadFile] alutLoadWAVFile : ", error);
			}
			alBufferData(Buffer, format, data, size, freq);
			if((error = alGetError()) != AL_NO_ERROR) {
				DisplayALError("[ACMImp::LoadFile] alBufferData : ", error);
			}
			alutUnloadWAV(format, data, size, freq);
			if((error = alGetError()) != AL_NO_ERROR) {
				DisplayALError("[ACMImp::LoadFile] alutUnloadWAV : ", error);
			}
			alGenSources(1, &Source);
			if((error = alGetError()) != AL_NO_ERROR) {
				DisplayALError("[ACMImp::LoadFile] alGenSources : ", error);
			}

			alSourcei (Source, AL_BUFFER,   Buffer   );
			alSourcef (Source, AL_PITCH,    1.0f     );
			alSourcef (Source, AL_GAIN,     1.0f     );
			alSourcefv(Source, AL_POSITION, SourcePos);
			alSourcefv(Source, AL_VELOCITY, SourceVel);
			alSourcei (Source, AL_LOOPING,  loop     );

			if (alGetError() != AL_NO_ERROR)
				return 0xffffffff;

			for(int i = 0; i < MAX_STREAMS; i++) {
				if(musics[i].free) {
					musics[i].Buffer = Buffer;
					musics[i].Source = Source;
					musics[i].free = false;
					musics[i].playing = false;
					return i;
				}
			}

			alDeleteBuffers(1, &Buffer);
			alDeleteSources(1, &Source);
		}

		return 0xffffffff;
	}
	printMessage("ACMImporter", "Cannot find decompressed file in Cache\n", LIGHT_RED);
	return 0xffffffff;
}

bool ACMImp::AcmToWav(DataStream *inFile, const char * tmpFile, const char * outFile)
{
	//DataStream * str = core->GetResourceMgr()->GetResource(ResRef, IE_WAV_CLASS_ID);
	if(inFile == NULL)
		return false;
	FILE *out = NULL;
	out = fopen(tmpFile, "wb");
	if(!out)
		return false;
	int maxlen = inFile->Size();
	int bufsize = (maxlen < 1024000 ? maxlen : 1024000);
	void *buf = malloc(bufsize);
	int p = 0;
	while(p < maxlen) {
		int len = inFile->Read(buf, bufsize);
		if(len <= 0)
			break;
		p+=len;
		fwrite(buf, 1, len, out);
	}
	fclose(out);
	free(buf);
	int flags = 0;
	#ifdef WIN32
	flags = O_RDONLY | O_BINARY;
	#else
	flags = O_RDONLY;
	#endif
	int fhandle = open(tmpFile, flags);
	if(fhandle == -1)
		return false;
	unsigned char *buffer = NULL;
	long samples_written;
	int ret = ConvertAcmWav(fhandle, -1L, buffer, samples_written, core->ForceStereo);
	if(ret != 0) {
		if(buffer)
			delete buffer;
		return false;
	}
	close(fhandle);
	remove(tmpFile);
	out = fopen(outFile, "wb");
	if(!out) {
		delete buffer;
		return false;
	}
	fwrite(buffer, 1, samples_written, out);
	fclose(out);
	delete buffer;
	return true;
}

bool ACMImp::Stop(unsigned long index)
{
	if(index >= MAX_STREAMS)
		return false;
	if(musics[index].free)
		return true;
	if(musics[index].playing)
		alSourceStop(musics[index].Source);
	SDL_mutexP(musicMutex);
	musics[index].playing = false;
	if(musicPlaying)
		musicPlaying = false;
	SDL_mutexV(musicMutex);
	return true;
}

bool ACMImp::Play(unsigned long index)
{
	if(index >= MAX_STREAMS)
		return false;
	if(musics[index].free)
		return false;
	if(musics[index].playing)
		return true;
	SDL_mutexP(musicMutex);
	unsigned long volume;
	if(!core->GetDictionary()->Lookup("Volume Music", volume)) {
		core->GetDictionary()->SetAt("Volume Music", 100);
		volume = 100;
	}
	alSourcef(musics[musicIndex].Source, AL_GAIN, volume/100.0);
	alSourceRewind(musics[index].Source);
	alSourcePlay(musics[index].Source);
#ifdef WIN32
	unsigned long time = GetTickCount();
#else
	struct timeval tv;
	gettimeofday(&tv, NULL);
	unsigned long time = (tv.tv_usec/1000) + (tv.tv_sec*1000);
#endif
	BufferStartPlayTime = time;
	BufferDuration = musics[index].Duration;
	musics[index].playing = true;
	if(!musicPlaying)
		musicPlaying = true;
	musicIndex = index;
	SDL_mutexV(musicMutex);
	return true;
}

void ACMImp::ResetMusics()
{
	clearstreams(true);
}
