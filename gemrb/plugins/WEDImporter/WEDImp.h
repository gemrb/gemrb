/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/WEDImporter/WEDImp.h,v 1.12 2005/10/22 16:30:54 avenger_teambg Exp $
 *
 */

#ifndef WEDIMP_H
#define WEDIMP_H

#include "../Core/TileMapMgr.h"

typedef struct Overlay {
	ieWord  Width;
	ieWord  Height;
	ieResRef TilesetResRef;
	ieDword unknown;
	ieDword TilemapOffset;
	ieDword TILOffset;
} Overlay;

class WEDImp : public TileMapMgr {
private:
	std::vector< Overlay> overlays;
	DataStream* str;
	bool autoFree;
	ieDword OverlaysCount, DoorsCount, OverlaysOffset, SecHeaderOffset,
		DoorsOffset, DoorTilesOffset;
	ieDword WallPolygonsCount, PolygonsOffset, VerticesOffset,
		WallGroupsOffset, PILTOffset;
public:
	WEDImp(void);
	~WEDImp(void);
	bool Open(DataStream* stream, bool autoFree = true);
	TileMap* GetTileMap();
	ieWord* GetDoorIndices(char* ResRef, int* count, bool& BaseClosed);
	Wall_Polygon **GetWallGroups();
	ieDword GetWallPolygonsCount() { return WallPolygonsCount; }
public:
	void release(void)
	{
		delete this;
	}
};

#endif
