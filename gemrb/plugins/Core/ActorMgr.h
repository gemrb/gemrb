#ifndef ACTORMGR_H
#define ACTORMGR_H

#include "Plugin.h"
#include "Actor.h"
#include "DataStream.h"

#ifdef WIN32

#ifdef GEM_BUILD_DLL
#define GEM_EXPORT __declspec(dllexport)
#else
#define GEM_EXPORT __declspec(dllimport)
#endif

#else
#define GEM_EXPORT
#endif

class GEM_EXPORT ActorMgr : public Plugin
{
public:
	ActorMgr(void);
	virtual ~ActorMgr(void);
	virtual bool Open(DataStream * stream, bool autoFree = true) = 0;
	virtual Actor * GetActor() = 0;
};

#endif
