#ifndef ACMIMP_H
#define ACMIMP_H

#include "../Core/SoundMgr.h"

class ACMImp : public SoundMgr
{
public:
	ACMImp(void);
	~ACMImp(void);
	bool Init(void);
	unsigned long Play(const char * ResRef);
	bool AcmToWav(const char * ResRef);
};

#endif
