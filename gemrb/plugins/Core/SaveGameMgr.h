#ifndef SAVEGAMEMGR_H
#define SAVEGAMEMGR_H

#include "Plugin.h"
#include "DataStream.h"
#include "Game.h"

#ifdef WIN32

#ifdef GEM_BUILD_DLL
#define GEM_EXPORT __declspec(dllexport)
#else
#define GEM_EXPORT __declspec(dllimport)
#endif

#else
#define GEM_EXPORT
#endif

class GEM_EXPORT SaveGameMgr : public Plugin
{
public:
	SaveGameMgr(void);
	virtual ~SaveGameMgr(void);
	virtual bool Open(DataStream * stream, bool autoFree = true) = 0;
	virtual Game * GetGame() = 0;
};

#endif
