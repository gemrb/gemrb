#ifndef MAPMGR_H
#define MAPMGR_H

#include "Plugin.h"
#include "DataStream.h"
#include "Map.h"

#ifdef WIN32

#ifdef GEM_BUILD_DLL
#define GEM_EXPORT __declspec(dllexport)
#else
#define GEM_EXPORT __declspec(dllimport)
#endif

#else
#define GEM_EXPORT
#endif

class GEM_EXPORT MapMgr : public Plugin
{
public:
	MapMgr(void);
	virtual ~MapMgr(void);
	virtual bool Open(DataStream * stream, bool autoFree = true) = 0;
	virtual Map * GetMap() = 0;
};

#endif
