#ifndef WEDIMP_H
#define WEDIMP_H

#include "../Core/TileMapMgr.h"

typedef struct Overlay {
	unsigned short	Width;
	unsigned short	Height;
	char			TilesetResRef[8];
	unsigned long	TilemapOffset;
	unsigned long	TILOffset;
} Overlay;

class WEDImp : public TileMapMgr
{
private:
	std::vector<Overlay> overlays;
	DataStream * str;
	bool autoFree;
public:
	WEDImp(void);
	~WEDImp(void);
	bool Open(DataStream * stream, bool autoFree = true);
	TileMap * GetTileMap();
};

#endif
