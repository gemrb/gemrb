#ifndef TISIMP_H
#define TISIMP_H

#include "../Core/TileSetMgr.h"

class TISImp :
	public TileSetMgr
{
private:
	DataStream * str;
	bool autoFree;
	unsigned long headerShift;
	unsigned long TilesCount, TilesSectionLen, TileSize;
public:
	TISImp(void);
	~TISImp(void);
	bool Open(DataStream * stream, bool autoFree = true);
	Tile * GetTile(unsigned short * indexes, int count);
	Sprite2D * GetTile(int index);
};

#endif
