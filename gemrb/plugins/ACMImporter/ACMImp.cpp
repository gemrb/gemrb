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

std::vector<AudioStream> streams;

signed char synchstreamcallback(FSOUND_STREAM *stream, void *buff, int len, int param) 
{
	printf("Callback\n");
	if(stream) {
		for(int i = 0; i < streams.size(); i++) {
			if(streams[i].stream == stream) {
				streams[i].playing = false;
				streams[i].end = false;
				streams[i].free = false;
				//FSOUND_Stream_SetTime(streams[i].stream, 0);
				FSOUND_Stream_Stop(streams[i].stream);
				//FSOUND_Stream_SetTime(streams[i].stream, 0);
				//streams[i].channel = FSOUND_Stream_Play(FSOUND_FREE, streams[i].stream);
				//FSOUND_SetPaused(streams[i].channel, true);
				core->GetMusicMgr()->PlayNext();
				return true;
			}
		}
	}
	else
		return true;
	return false;
}

ACMImp::ACMImp(void)
{
}

ACMImp::~ACMImp(void)
{
}

bool ACMImp::Init(void)
{
#ifndef WIN32
	FSOUND_SetOutput(FSOUND_OUTPUT_OSS);
	printf("Using OSS Driver...");
#else
	FSOUND_SetOutput(FSOUND_OUTPUT_DSOUND);
	printf("Using DirectSound...");
#endif
	if(FSOUND_Init(44100, 32, 0) == false) {
		return false;
	}
	streams.clear();
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
#ifndef WIN32
		FSOUND_STREAM * sound = FSOUND_Stream_Open(path, FSOUND_LOOP_OFF | FSOUND_2D, 0, 0);
#else
		FSOUND_STREAM * sound = FSOUND_Stream_OpenFile(path, FSOUND_LOOP_OFF | FSOUND_2D, 0);
#endif
		if(sound) {
			/*if(!FSOUND_Stream_SetEndCallback(sound, endstreamcallback, 0)) {
				printMessage("ACMImporter", "SetEndCallback Failed\n", YELLOW);
			}
			AudioStream as;
			as.stream = sound;
			as.playing = true;
			as.end = false;
			as.free = false;
			int ret = -1;
			bool found = false;
			for(int i = 0; i < streams.size(); i++) {
				if(streams[i].free) {
					streams[i] = as;
					found = true;
					ret = i;
					break;
				}
			}
			if(!found) {
				streams.push_back(as);
				ret = streams.size()-1;
			}*/
			FSOUND_Stream_Play(FSOUND_FREE, sound);
			return 0;
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
#ifndef WIN32
		FSOUND_STREAM * sound = FSOUND_Stream_Open(path, FSOUND_LOOP_OFF | FSOUND_2D, 0, 0);
#else
		FSOUND_STREAM * sound = FSOUND_Stream_OpenFile(path, FSOUND_LOOP_OFF | FSOUND_2D, 0);
#endif
		if(sound) {
			/*if(!FSOUND_Stream_SetEndCallback(sound, endstreamcallback, 0)) {
				printMessage("ACMImporter", "SetEndCallback Failed\n", YELLOW);
			}
			AudioStream as;
			as.stream = sound;
			as.playing = true;
			as.end = false;
			as.free = false;
			int ret = -1;
			bool found = false;
			for(int i = 0; i < streams.size(); i++) {
				if(streams[i].free) {
					streams[i] = as;
					found = true;
					ret = i;
					break;
				}
			}
			if(!found) {
				streams.push_back(as);
				ret = streams.size()-1;
			}*/
			FSOUND_Stream_Play(FSOUND_FREE, sound);
			return 0;
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
#ifndef WIN32
		FSOUND_STREAM * sound = FSOUND_Stream_Open(outFile, FSOUND_LOOP_OFF | FSOUND_2D, 0, 0);
#else
		FSOUND_STREAM * sound = FSOUND_Stream_OpenFile(outFile, FSOUND_LOOP_OFF | FSOUND_2D, 0);
#endif
		if(sound) {
			if(!FSOUND_Stream_SetSyncCallback(sound, synchstreamcallback, 0)) {
				printMessage("ACMImporter", "SetEndCallback Failed\n", YELLOW);
			}
			AudioStream as;
			as.stream = sound;
			as.playing = false;
			as.end = false;
			as.free = false;
			as.channel = -1;
			int ret = -1;
			bool found = false;
			unsigned int strFlags = FSOUND_Stream_GetMode(sound);
			int lastsample = (FSOUND_Stream_GetLength(sound) / (strFlags & FSOUND_16BITS ? 2 : 1)) / (strFlags & FSOUND_STEREO ? 2 : 1);
			FSOUND_Stream_AddSyncPoint(sound, lastsample-1000, "End");
			//as.channel = FSOUND_Stream_Play(FSOUND_FREE, sound);
			//FSOUND_SetPaused(as.channel, true);
			for(int i = 0; i < streams.size(); i++) {
				if(streams[i].free) {
					streams[i] = as;
					found = true;
					ret = i;
					break;
				}
			}
			if(!found) {
				streams.push_back(as);
				ret = streams.size()-1;
			}
			return ret;
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
#ifndef WIN32
		FSOUND_STREAM * sound = FSOUND_Stream_Open(outFile, FSOUND_LOOP_OFF | FSOUND_2D, 0, 0);
#else
		FSOUND_STREAM * sound = FSOUND_Stream_OpenFile(outFile, FSOUND_LOOP_OFF | FSOUND_2D, 0);
#endif
		if(sound) {
			if(!FSOUND_Stream_SetSyncCallback(sound, synchstreamcallback, 0)) {
				printMessage("ACMImporter", "SetEndCallback Failed\n", YELLOW);
			}
			
			AudioStream as;
			as.stream = sound;
			as.playing = false;
			as.end = false;
			as.free = false;
			as.channel = -1;
			int ret = -1;
			bool found = false;
			unsigned int strFlags = FSOUND_Stream_GetMode(sound);
			int lastsample = FSOUND_Stream_GetLength(sound) * (strFlags & FSOUND_16BITS ? 2 : 1) * (strFlags & FSOUND_STEREO ? 2 : 1);
			FSOUND_Stream_AddSyncPoint(sound, lastsample-1000, "End");
			//as.channel = FSOUND_Stream_Play(FSOUND_FREE, sound);
			//FSOUND_SetPaused(as.channel, true);
			for(int i = 0; i < streams.size(); i++) {
				if(streams[i].free) {
					streams[i] = as;
					found = true;
					ret = i;
					break;
				}
			}
			if(!found) {
				streams.push_back(as);
				ret = streams.size()-1;
			}
			return ret;
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
	int ret = ConvertAcmWav(fhandle, -1L, buffer, samples_written, 0);
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
	if(index >= streams.size())
		return false;
	if(streams[index].free)
		return true;
	if(streams[index].playing)
	FSOUND_Stream_Stop(streams[index].stream);
	//streams[index].channel = FSOUND_Stream_Play(FSOUND_FREE, streams[index].stream);
	//FSOUND_Stream_SetTime(streams[index].stream, 0);
	//FSOUND_SetPaused(streams[index].channel, true);
	streams[index].playing = false;
	return true;
}

bool ACMImp::Play(unsigned long index)
{
	if(index >= streams.size())
		return false;
	if(streams[index].free)
		return false;
	if(streams[index].playing)
		return true;
	streams[index].channel = FSOUND_Stream_Play(0, streams[index].stream);
	streams[index].playing = true;
	return true;
}
