#ifndef ACMIMP_H
#define ACMIMP_H

#include "../Core/SoundMgr.h"
#include "../Core/FileStream.h"
//#ifndef WIN32
#include "../../includes/fmod/fmod.h"
//#else
//#include "../../includes/fmodwin32/fmod.h"
//#endif

typedef struct AudioStream {
	FSOUND_STREAM * stream;
	bool playing;
	bool end;
	bool free;
	int channel;
} AudioStream;

class ACMImp : public SoundMgr
{
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
