#ifndef TILESETMGR_H
#define TILESETMGR_H

#include "Plugin.h"
#include "DataStream.h"
#include "Tile.h"

#ifdef WIN32

#ifdef GEM_BUILD_DLL
#define GEM_EXPORT __declspec(dllexport)
#else
#define GEM_EXPORT __declspec(dllimport)
#endif

#else
#define GEM_EXPORT
#endif

class GEM_EXPORT TileSetMgr : public Plugin
{
public:
	TileSetMgr(void);
	virtual ~TileSetMgr(void);
	virtual bool Open(DataStream * stream, bool autoFree = true) = 0;
	virtual Tile * GetTile(unsigned short * indexes, int count) = 0;
};

#endif
