#ifndef TILEMAPMGR_H
#define TILEMAPMGR_H

#include "Plugin.h"
#include "DataStream.h"
#include "TileMap.h"

#ifdef WIN32

#ifdef GEM_BUILD_DLL
#define GEM_EXPORT __declspec(dllexport)
#else
#define GEM_EXPORT __declspec(dllimport)
#endif

#else
#define GEM_EXPORT
#endif

class GEM_EXPORT TileMapMgr : public Plugin
{
public:
	TileMapMgr(void);
	virtual ~TileMapMgr(void);
	virtual bool Open(DataStream * stream, bool autoFree = true) = 0;
	virtual TileMap * GetTileMap() = 0;
};

#endif
