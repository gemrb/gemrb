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

static AudioStream streams[MAX_STREAMS];

void ACMImp::clearstreams(bool free) {
	for(int i = 0; i < MAX_STREAMS; i++) {
		if(!streams[i].free && free) {
			FSOUND_Stream_Close(streams[i].stream);
			FSOUND_DSP_Free(streams[i].dsp);
		}
		streams[i].free = true;
	}
}

signed char __stdcall ACMImp::endstreamcallback(FSOUND_STREAM *stream, void *buff, int len, int param) 
{
	//printf("End Callback\n");
	FSOUND_Stream_Close(stream);
	return 0;
}

signed char __stdcall ACMImp::synchstreamcallback(FSOUND_STREAM *stream, void *buff, int len, int param)
{
	//printf("Synch Callback\n");
	if(stream) {
		for(int i = 0; i < MAX_STREAMS; i++) {
			if(streams[i].stream == stream) {
				streams[i].playing = false;
				streams[i].end = false;
				streams[i].free = false;
				core->GetMusicMgr()->PlayNext();
				return 1;
			}
		}
	}
	else
		return 1;
	return 0;
}

void * __stdcall ACMImp::dspcallback(void *originalbuffer, void *newbuffer, int length, int param)
{
	//printf("DSP CallBack\n");
	if(!streams[param].end) {
#ifdef WIN32
		int curtime = FSOUND_Stream_GetTime(streams[param].stream);
		int tottime = FSOUND_Stream_GetLengthMs(streams[param].stream);
#endif
		unsigned long volume;
		if(!core->GetDictionary()->Lookup("Volume Music", volume)) {
			core->GetDictionary()->SetAt("Volume Music", 10);
			volume = 10;
		}
		FSOUND_SetVolume(streams[param].channel, (volume/10.0)*255);
#ifdef WIN32
		//printf("curtime = %d, tottime = %d, delta = %d\n", curtime, tottime, tottime-curtime);
		if(curtime > tottime-125) {
			streams[param].playing = false;
			streams[param].end = true;
			core->GetMusicMgr()->PlayNext();
			//printf("Playing Next Track\n");
		}
#endif
	}
	return newbuffer;
}

ACMImp::ACMImp(void)
{
	for(int i = 0; i < MAX_STREAMS; i++) {
		streams[i].free = true;
	}
}

ACMImp::~ACMImp(void)
{
	clearstreams(true);
	FSOUND_Close();
}

bool ACMImp::Init(void)
{
#ifndef WIN32
	FSOUND_SetOutput(FSOUND_OUTPUT_OSS);
	printf("[%.2f] Using OSS Driver...", FSOUND_GetVersion());
#else
	FSOUND_SetOutput(FSOUND_OUTPUT_DSOUND);
	printf("[%.2f] Using DirectSound...", FSOUND_GetVersion());
#endif
	if(FSOUND_Init(44100, 32, 0) == false) {
		return false;
	}
	if(!FSOUND_Stream_SetBufferSize(100))
		printf("Error Setting Buffer Size\n");
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
		FSOUND_STREAM * sound = FSOUND_Stream_Open(path, FSOUND_LOOP_OFF | FSOUND_2D, 0, 0);
		if(sound) {
			if(!FSOUND_Stream_SetEndCallback(sound, endstreamcallback, 0)) {
				printMessage("ACMImporter", "SetEndCallback Failed\n", YELLOW);
			}
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
		FSOUND_STREAM * sound = FSOUND_Stream_Open(path, FSOUND_LOOP_OFF | FSOUND_2D, 0, 0);
		if(sound) {
			if(!FSOUND_Stream_SetEndCallback(sound, endstreamcallback, 0)) {
				printMessage("ACMImporter", "SetEndCallback Failed\n", YELLOW);
			}
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
		FSOUND_STREAM * sound = FSOUND_Stream_Open(outFile, FSOUND_LOOP_OFF | FSOUND_2D, 0, 0);
		if(sound) {
#ifndef WIN32
			if(!FSOUND_Stream_SetSyncCallback(sound, synchstreamcallback, 0)) {
				printMessage("ACMImporter", "SetSychCallback Failed\n", YELLOW);
			}
#endif
			/*if(!FSOUND_Stream_SetEndCallback(sound, endstreamcallback, 0)) {
				printMessage("ACMImporter", "SetEndCallback Failed\n", YELLOW);
			}*/
			int ret = -1;
			for(int i = 0; i < MAX_STREAMS; i++) {
				if(streams[i].free) {
					streams[i].stream = sound;
					streams[i].playing = false;
					streams[i].end = false;
					streams[i].free = false;
					streams[i].channel = -1;
					streams[i].dsp = FSOUND_Stream_CreateDSP(sound, dspcallback, 1, i);
					FSOUND_DSP_SetActive(streams[i].dsp, true);
					ret = i;
					break;
				}
			}
			if(ret == -1) {
				FSOUND_Stream_Close(sound);
				printf("Too many streams loaded\n");
				return 0xffffffff;
			}
			FSOUND_SAMPLE * smp = FSOUND_Stream_GetSample(sound);
			int freq;
			FSOUND_Sample_GetDefaults(smp, &freq, NULL, NULL, NULL);
			unsigned int lastsample = FSOUND_Stream_GetLengthMs(sound)*(freq / 1000.0);
			if(!FSOUND_Stream_AddSyncPoint(sound, lastsample-800, "End"))
				printf("Error setting the Synch Point: %d\n", FSOUND_GetError());
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
		FSOUND_STREAM * sound = FSOUND_Stream_Open(outFile, FSOUND_LOOP_OFF, 0, 0);
		if(sound) {
#ifndef WIN32
			if(!FSOUND_Stream_SetSyncCallback(sound, synchstreamcallback, 0)) {
				printMessage("ACMImporter", "SetSychCallback Failed\n", YELLOW);
			}
#endif
			/*if(!FSOUND_Stream_SetEndCallback(sound, endstreamcallback, 0)) {
				printMessage("ACMImporter", "SetEndCallback Failed\n", YELLOW);
			}*/
			int ret = -1;
			for(int i = 0; i < MAX_STREAMS; i++) {
				if(streams[i].free) {
					streams[i].stream = sound;
					streams[i].playing = false;
					streams[i].end = false;
					streams[i].free = false;
					streams[i].channel = -1;
					streams[i].dsp = FSOUND_Stream_CreateDSP(sound, dspcallback, 1, i);
					ret = i;
					break;
				}
			}
			if(ret == -1) {
				FSOUND_Stream_Close(sound);
				printf("Too many streams loaded\n");
				return 0xffffffff;
			}
			FSOUND_SAMPLE * smp = FSOUND_Stream_GetSample(sound);
			int freq;
			FSOUND_Sample_GetDefaults(smp, &freq, NULL, NULL, NULL);
			unsigned int lastsample = FSOUND_Stream_GetLengthMs(sound)*(freq / 1000.0);
			if(!FSOUND_Stream_AddSyncPoint(sound, lastsample-800, "End"))
				printf("Error setting the Synch Point\n");
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
	if(index >= MAX_STREAMS)
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
	if(index >= MAX_STREAMS)
		return false;
	if(streams[index].free)
		return false;
	if(streams[index].playing)
		return true;
#ifdef WIN32
	FSOUND_DSP_SetActive(streams[index].dsp, true);
#endif
	streams[index].channel = FSOUND_Stream_Play(FSOUND_FREE, streams[index].stream);
	streams[index].playing = true;
	streams[index].end = false;
	return true;
}

void ACMImp::ResetMusics()
{
	clearstreams(true);
}
