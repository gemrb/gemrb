#include "../../includes/win32def.h"
#include "../Core/Interface.h"
#include "ACMImp.h"
#include "../../includes/fmod/fmod.h"
#include "acmsound.h"
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <io.h>

signed char endstreamcallback(FSOUND_STREAM *stream, void *buff, int len, int param) 
{
	if(stream) {
		return FSOUND_Stream_Close(stream);
	}
	return TRUE;
}

ACMImp::ACMImp(void)
{
}

ACMImp::~ACMImp(void)
{
}

bool ACMImp::Init(void)
{
	if(FSOUND_Init(44100, 32, 0) == FALSE)
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
		FSOUND_STREAM * sound = FSOUND_Stream_OpenFile(path, FSOUND_LOOP_OFF | FSOUND_2D, 0);
		if(sound) {
			if(!FSOUND_Stream_SetEndCallback(sound, endstreamcallback, 0)) {
				printMessage("ACMImporter", "SetEndCallback Failed\n", YELLOW);
			}
			FSOUND_Stream_Play(FSOUND_FREE, sound);
			return true;
		}
		return false;
	}
	if(!AcmToWav(ResRef)) {
		printMessage("ACMImporter", "ACM Decompression Failed\n", LIGHT_RED);
		return false;
	}
	str = fopen(path, "rb");
	if(str != NULL) {
		fclose(str);
		FSOUND_STREAM * sound = FSOUND_Stream_OpenFile(path, FSOUND_LOOP_OFF | FSOUND_2D, 0);
		if(sound) {
			if(!FSOUND_Stream_SetEndCallback(sound, endstreamcallback, 0)) {
				printMessage("ACMImporter", "SetEndCallback Failed\n", YELLOW);
			}
			FSOUND_Stream_Play(FSOUND_FREE, sound);
			return true;
		}
		return false;
	}
	printMessage("ACMImporter", "Cannot find decompressed file in Cache\n", LIGHT_RED);
	return false;
}

bool ACMImp::AcmToWav(const char * ResRef)
{
	DataStream * str = core->GetResourceMgr()->GetResource(ResRef, IE_WAV_CLASS_ID);
	if(str == NULL)
		return false;
	char path[_MAX_PATH];
	strcpy(path, core->CachePath);
	strcat(path, ResRef);
	strcat(path, ".tmp");
	FILE * out = fopen(path, "wb");
	int maxlen = str->Size();
	int bufsize = (maxlen < 1024000 ? maxlen : 1024000);
	void *buf = malloc(bufsize);
	int p = 0;
	while(p < maxlen) {
		int len = str->Read(buf, bufsize);
		if(len <= 0)
			break;
		p+=len;
		fwrite(buf, 1, len, out);
	}
	fclose(out);
	free(buf);
	int fhandle = open(path, O_RDONLY | O_BINARY);
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
	strcpy(path, core->CachePath);
	strcat(path, ResRef);
	strcat(path, core->TypeExt(IE_WAV_CLASS_ID));
	out = fopen(path, "wb");
	if(!out) {
		delete buffer;
		return false;
	}
	fwrite(buffer, 1, samples_written, out);
	fclose(out);
	delete buffer;
	return true;
}