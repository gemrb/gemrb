#ifndef ACMIMP_H
#define ACMIMP_H

#include "../Core/SoundMgr.h"
#include "../Core/FileStream.h"
//#ifndef WIN32
#include "../../includes/fmod/fmod.h"
//#else
//#include "../../includes/fmodwin32/fmod.h"
//#endif

#define MAX_STREAMS  30

typedef struct AudioStream {
	FSOUND_STREAM * stream;
	FSOUND_DSPUNIT * dsp;
	bool playing;
	bool end;
	bool free;
	int channel;
} AudioStream;

class ACMImp : public SoundMgr
{
private:
	void clearstreams(bool free);
	static signed char __stdcall endstreamcallback(FSOUND_STREAM *stream, void *buff, int len, int param);
	static signed char __stdcall synchstreamcallback(FSOUND_STREAM *stream, void *buff, int len, int param);
	static void * __stdcall dspcallback(void *originalbuffer, void *newbuffer, int length, int param);
public:
	ACMImp(void);
	~ACMImp(void);
	bool Init(void);
	unsigned long Play(const char * ResRef);
	bool AcmToWav(DataStream *inFile, const char * tmpFile, const char * outFile);
	unsigned long LoadFile(const char * filename);
	bool Play(unsigned long index);
	bool Stop(unsigned long index);
};

#endif
