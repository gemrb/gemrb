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
	ieWord UniqueTileCount; // nNumUniqueTiles in the original (currently unused)
	ieWord MovementType; // nMovementType in the original (currently unused)
	ieDword TilemapOffset;
	ieDword TILOffset;
};

class WEDImporter : public TileMapMgr {
private:
	std::vector<Overlay> overlays;
	DataStream* str = nullptr;
	ieDword OverlaysCount = 0, OverlaysOffset = 0;
	ieDword DoorsCount = 0, DoorsOffset = 0, DoorTilesOffset = 0, DoorPolygonsCount = 0;
	ieDword WallPolygonsCount = 0, WallGroupsOffset = 0;
	ieDword PolygonsOffset = 0, VerticesOffset = 0;
	ieDword SecHeaderOffset = 0, PILTOffset = 0;
	//these will change as doors are being read, so get them in time!
	ieWord OpenPolyCount = 0, ClosedPolyCount = 0;
	ieDword OpenPolyOffset = 0, ClosedPolyOffset = 0;
	bool ExtendedNight = false;

private:
	void GetDoorPolygonCount(ieWord count, ieDword offset);
	int AddOverlay(TileMap *tm, const Overlay *overlays, bool rain) const;
public:
	WEDImporter(void);
	~WEDImporter(void);
	bool Open(DataStream* stream);
	//if tilemap already exists, don't create it
	TileMap* GetTileMap(TileMap *tm) const;
	ieWord* GetDoorIndices(char* ResRef, int* count, bool& BaseClosed);
	Wall_Polygon **GetWallGroups() const;
	ieDword GetWallPolygonsCount() const { return WallPolygonsCount; }
	ieDword GetPolygonsCount() const { return WallPolygonsCount + DoorPolygonsCount; }
	void SetupOpenDoor(unsigned int &index, unsigned int &count) const;
	void SetupClosedDoor(unsigned int &index, unsigned int &count) const;
	void SetExtendedNight(bool night) { ExtendedNight = night; }
};

}

#endif
