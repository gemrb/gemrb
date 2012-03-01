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

#ifndef TILEMAP_H
#define TILEMAP_H

#include "exports.h"

#include "Polygon.h"
#include "TileOverlay.h"

namespace GemRB {

//special container types
#define IE_CONTAINER_PILE   4

class Container;
class Door;
class InfoPoint;
class TileObject;

class GEM_EXPORT TileMap {
private:
	std::vector< TileOverlay*> overlays;
	std::vector< TileOverlay*> rain_overlays;
	std::vector< Door*> doors;
	std::vector< Container*> containers;
	std::vector< InfoPoint*> infoPoints;
	std::vector< TileObject*> tiles;
	bool LargeMap;
public:
	TileMap(void);
	~TileMap(void);

	Door* AddDoor(const char* ID, const char* Name, unsigned int Flags,
		int ClosedIndex, unsigned short* indices, int count,
		Gem_Polygon* open, Gem_Polygon* closed);
	//gets door by active region (click target)
	Door* GetDoor(const Point &position) const;
	//gets door by activation position (spell target)
	Door* GetDoorByPosition(const Point &position) const;
	Door* GetDoor(unsigned int idx) const;
	Door* GetDoor(const char* Name) const;
	size_t GetDoorCount() { return doors.size(); }
	//update doors for a new overlay
	void UpdateDoors();

	/* type is an optional filter for container type*/
	void AddContainer(Container *c);
	//gets container by active region (click target)
	Container* GetContainer(const Point &position, int type=-1) const;
	//gets container by activation position (spell target)
	Container* GetContainerByPosition(const Point &position, int type=-1) const;
	Container* GetContainer(const char* Name) const;
	Container* GetContainer(unsigned int idx) const;
	/* cleans up empty heaps, returns 1 if container removed*/
	int CleanupContainer(Container *container);
	size_t GetContainerCount() const { return containers.size(); }

	InfoPoint* AddInfoPoint(const char* Name, unsigned short Type,
		Gem_Polygon* outline);
	InfoPoint* GetInfoPoint(const Point &position, bool detectable) const;
	InfoPoint* GetInfoPoint(const char* Name) const;
	InfoPoint* GetInfoPoint(unsigned int idx) const;
	InfoPoint* GetTravelTo(const char* Destination) const;
	InfoPoint* AdjustNearestTravel(Point &p);
	size_t GetInfoPointCount() const { return infoPoints.size(); }

	TileObject* AddTile(const char* ID, const char* Name, unsigned int Flags,
		unsigned short* openindices, int opencount,unsigned short* closeindices, int closecount);
	TileObject* GetTile(unsigned int idx);
	TileObject* GetTile(const char* Name);
	size_t GetTileCount() { return tiles.size(); }

	void ClearOverlays();
	void AddOverlay(TileOverlay* overlay);
	void AddRainOverlay(TileOverlay* overlay);
	void DrawOverlays(Region screen, int rain, int flags);
	void DrawFogOfWar(ieByte* explored_mask, ieByte* visible_mask, Region viewport);
	Point GetMapSize();
public:
	int XCellCount, YCellCount;
};

}

#endif
