#ifndef MOVIEPLAYER_H
#define MOVIEPLAYER_H

#include "../../includes/win32def.h"
#include "../../includes/globals.h"
#include "Plugin.h"
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

class GEM_EXPORT MoviePlayer : public Plugin
{
public:
	MoviePlayer(void);
	virtual ~MoviePlayer(void);
	virtual bool Open(DataStream * stream, bool autoFree = true) = 0;
	virtual int Play() = 0;
};

#endif
