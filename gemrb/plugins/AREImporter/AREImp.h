#ifndef AREIMP_H
#define AREIMP_H

#include "../Core/MapMgr.h"

class AREImp : public MapMgr
{
private:
	DataStream * str;
	bool autoFree;
	char WEDResRef[8];
	unsigned long ActorOffset, AnimOffset, AnimCount;
	unsigned short ActorCount;
public:
	AREImp(void);
	~AREImp(void);
	bool Open(DataStream * stream, bool autoFree = true);
	Map * GetMap();
public:
	void release(void)
	{
		delete this;
	}
};

#endif
