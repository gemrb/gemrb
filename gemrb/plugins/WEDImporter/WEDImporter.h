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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

#ifndef WEDIMPORTER_H
#define WEDIMPORTER_H

#include "TileMapMgr.h"

namespace GemRB {

struct Overlay {
	ieWord  Width;
	ieWord  Height;
	ieResRef TilesetResRef;
	ieDword unknown;
	ieDword TilemapOffset;
	ieDword TILOffset;
};

class WEDImporter : public TileMapMgr {
private:
	std::vector< Overlay> overlays;
	DataStream* str;
	ieDword OverlaysCount, DoorsCount, OverlaysOffset, SecHeaderOffset,
		DoorsOffset, DoorTilesOffset;
	ieDword WallPolygonsCount, PolygonsOffset, VerticesOffset,
		WallGroupsOffset, PILTOffset;
	ieDword DoorPolygonsCount;
	//these will change as doors are being read, so get them in time!
	ieWord OpenPolyCount, ClosedPolyCount;
	ieDword OpenPolyOffset, ClosedPolyOffset;

private:
	void GetDoorPolygonCount(ieWord count, ieDword offset);
	int AddOverlay(TileMap *tm, Overlay *overlays, bool rain);
public:
	WEDImporter(void);
	~WEDImporter(void);
	bool Open(DataStream* stream);
	//if tilemap already exists, don't create it
	TileMap* GetTileMap(TileMap *tm);
	ieWord* GetDoorIndices(char* ResRef, int* count, bool& BaseClosed);
	Wall_Polygon **GetWallGroups();
	ieDword GetWallPolygonsCount() { return WallPolygonsCount; }
	ieDword GetPolygonsCount() { return WallPolygonsCount+DoorPolygonsCount; }
	void SetupOpenDoor(unsigned int &index, unsigned int &count);
	void SetupClosedDoor(unsigned int &index, unsigned int &count);
};

}

#endif
