#include "../../includes/win32def.h"
#include "Video.h"

Video::Video(void)
{
	Evnt = NULL;
}

Video::~Video(void)
{
}

/** Set Event Manager */
void Video::SetEventMgr(EventMgr * evnt)
{
	//if 'evnt' is NULL then no Event Manager will be used
	Evnt = evnt;
}
