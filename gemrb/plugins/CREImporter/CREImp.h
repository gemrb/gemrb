#ifndef CREIMP_H
#define CREIMP_H

#include "../Core/ActorMgr.h"

#define IE_CRE_V1_0		0
#define IE_CRE_V1_2		1
#define IE_CRE_V2_2		2
#define IE_CRE_V9_0		3

class CREImp : public ActorMgr
{
private:
	DataStream * str;
	bool autoFree;
	unsigned char CREVersion;
public:
	CREImp(void);
	~CREImp(void);
	bool Open(DataStream * stream, bool autoFree = true);
	Actor * GetActor();
public:
	void release(void)
	{
		delete this;
	}
};

#endif
