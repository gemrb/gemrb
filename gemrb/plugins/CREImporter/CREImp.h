#ifndef CREIMP_H
#define CREIMP_H

#include "../Core/ActorMgr.h"

class CREImp : public ActorMgr
{
private:
	DataStream * str;
	bool autoFree;
public:
	CREImp(void);
	~CREImp(void);
	bool Open(DataStream * stream, bool autoFree = true);
	Actor * GetActor();
};

#endif
