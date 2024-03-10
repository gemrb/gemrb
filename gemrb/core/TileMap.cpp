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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

#include "TileMap.h"

#include "Interface.h"

#include "Scriptable/Container.h"
#include "Scriptable/Door.h"
#include "Scriptable/InfoPoint.h"
#include "Scriptable/TileObject.h"
#include "Video/Video.h"

namespace GemRB {

TileMap::~TileMap(void)
{
	for (const InfoPoint *infoPoint : infoPoints) {
		delete infoPoint;
	}

	for (const Door *door : doors) {
		delete door;
	}

	for (const Container *container : containers) {
		delete container;
	}
}

//this needs in case of a tileset switch (for extended night)
void TileMap::ClearOverlays()
{
	overlays.clear();
	rain_overlays.clear();
}

//tiled objects
TileObject* TileMap::AddTile(const ResRef& ID, const ieVariable& Name, unsigned int Flags,
	unsigned short* openindices, int opencount, unsigned short* closeindices, int closecount)
{
	TileObject* tile = new TileObject();
	tile->flags = Flags;
	tile->name = Name; // would probably need MakeVariable if the whole class wasn't unused
	tile->tileset = ID;
	tile->SetOpenTiles( openindices, opencount );
	tile->SetClosedTiles( closeindices, closecount );
	tiles.push_back(tile);
	return tile;
}

TileObject* TileMap::GetTile(unsigned int idx)
{
	if (idx >= tiles.size()) {
		return NULL;
	}
	return tiles[idx];
}

//doors
Door* TileMap::AddDoor(const ResRef& ID, const ieVariable& Name, unsigned int Flags,
	int ClosedIndex, std::vector<ieWord> indices, DoorTrigger&& dt)
{
	Door* door = new Door(overlays[0], std::move(dt));
	door->Flags = Flags;
	door->closedIndex = ClosedIndex;
	door->SetTiles(std::move(indices));
	door->SetName( ID );
	door->SetScriptName( Name );
	doors.push_back( door );
	return door;
}

Door* TileMap::GetDoor(size_t idx) const
{
	if (idx >= doors.size()) {
		return NULL;
	}
	return doors[idx];
}

Door* TileMap::GetDoor(const Point &p) const
{
	for (Door* door : doors) {
		if (door->HitTest(p)) return door;
	}
	return NULL;
}

Door* TileMap::GetDoorByPosition(const Point &p) const
{
	for (Door *door : doors) {
		if (door->toOpen[0].x==p.x && door->toOpen[0].y==p.y) {
			return door;
		}
		if (door->toOpen[1].x==p.x && door->toOpen[1].y==p.y) {
			return door;
		}
	}
	return NULL;
}

Door* TileMap::GetDoor(const ieVariable& Name) const
{
	for (Door *door : doors) {
		if (door->GetScriptName() == Name)
			return door;
	}
	return NULL;
}

void TileMap::UpdateDoors()
{
	for (Door *door : doors) {
		door->SetNewOverlay(overlays[0]);
	}
}

// used during time compression in bg1 ... but potentially problematic, so we don't enable it elsewhere
void TileMap::AutoLockDoors() const
{
	if (!core->HasFeature(GFFlags::RANDOM_BANTER_DIALOGS)) return;

	for (Door *door : doors) {
		if (door->CantAutoClose()) continue;
		if (core->Roll(1, 2, -1)) continue; // just a guess
		door->SetDoorOpen(false, false, 0);
	}
}

//overlays, allow pushing of NULL
void TileMap::AddOverlay(TileOverlayPtr overlay)
{
	if (overlay) {
		XCellCount = std::max(XCellCount, overlay->size.w);
		YCellCount = std::max(YCellCount, overlay->size.h);
	}
	overlays.push_back(std::move(overlay));
}

void TileMap::AddRainOverlay(TileOverlayPtr overlay)
{
	if (overlay) {
		XCellCount = std::max(XCellCount, overlay->size.w);
		YCellCount = std::max(YCellCount, overlay->size.h);
	}
	rain_overlays.push_back(std::move(overlay));
}

void TileMap::DrawOverlays(const Region& viewport, bool rain, BlitFlags flags)
{
	overlays[0]->Draw(viewport, rain ? rain_overlays : overlays, flags);
}

//containers
void TileMap::AddContainer(Container *c)
{
	containers.push_back(c);
}

Container* TileMap::GetContainer(size_t idx) const
{
	if (idx >= containers.size()) {
		return NULL;
	}
	return containers[idx];
}

Container* TileMap::GetContainer(const ieVariable& Name) const
{
	for (Container *container : containers) {
		if (container->GetScriptName() == Name) {
			return container;
		}
	}
	return NULL;
}

//look for a container at position
//use type = IE_CONTAINER_PILE if you want to find ground piles only
//in this case, empty piles won't be found!
Container* TileMap::GetContainer(const Point &position, int type) const
{
	for (Container *container : containers) {
		if (type != -1 && type != container->containerType) {
			continue;
		}

		if (!container->BBox.PointInside(position)) continue;

		//IE piles don't have polygons, the bounding box is enough for them
		if (container->containerType == IE_CONTAINER_PILE) {
			//don't find empty piles if we look for any container
			//if we looked only for piles, then we still return them
			if ((type == -1) && !container->inventory.GetSlotCount()) {
				continue;
			}
			return container;
		} else if (container->outline && container->outline->PointIn(position)) {
			return container;
		}
	}
	return NULL;
}

Container* TileMap::GetContainerByPosition(const Point &position, int type) const
{
	for (Container *container : containers) {
		if (type != -1 && type != container->containerType) {
			continue;
		}

		if (container->Pos.x != position.x || container->Pos.y != position.y) {
			continue;
		}

		//IE piles don't have polygons, the bounding box is enough for them
		if (container->containerType == IE_CONTAINER_PILE) {
			//don't find empty piles if we look for any container
			//if we looked only for piles, then we still return them
			if ((type == -1) && !container->inventory.GetSlotCount()) {
				continue;
			}
			return container;
		}
		return container;
	}
	return NULL;
}

int TileMap::CleanupContainer(Container *container)
{
	if (container->containerType != IE_CONTAINER_PILE)
		return 0;
	if (container->inventory.GetSlotCount())
		return 0;

	for (size_t i = 0; i < containers.size(); i++) {
		if (containers[i]==container) {
			containers.erase(containers.begin()+i);
			delete container;
			return 1;
		}
	}
	Log(ERROR, "TileMap", "Invalid container cleanup: {}",
		container->GetScriptName());
	return 1;
}

//infopoints
InfoPoint* TileMap::AddInfoPoint(const ieVariable& Name, unsigned short Type, const std::shared_ptr<Gem_Polygon>& outline)
{
	InfoPoint* ip = new InfoPoint();
	ip->SetScriptName( Name );
	switch (Type) {
		case 0:
			ip->Type = ST_PROXIMITY;
			break;

		case 1:
			ip->Type = ST_TRIGGER;
			break;

		case 2:
			ip->Type = ST_TRAVEL;
			break;
		//this is just to satisfy whiny compilers
		default:
			ip->Type = ST_PROXIMITY;
			break;
	}
	ip->outline = outline;
	if (ip->outline)
		ip->BBox = outline->BBox;
	//ip->Active = true; //set active on creation
	infoPoints.push_back( ip );
	return ip;
}

//if detectable is set, then only detectable infopoints will be returned
InfoPoint* TileMap::GetInfoPoint(const Point &p, bool skipSilent) const
{
	for (InfoPoint *infoPoint : infoPoints) {
		//these flags disable any kind of user interaction
		//scripts can still access an infopoint by name
		if (infoPoint->Flags & (INFO_DOOR | TRAP_DEACTIVATED))
			continue;

		if (infoPoint->Type == ST_PROXIMITY && !infoPoint->VisibleTrap(0)) {
			continue;
		}

		// skip portals without PORTAL_CURSOR set
		if (infoPoint->IsPortal() && !(infoPoint->Trapped & PORTAL_CURSOR)) {
				continue;
		}

		if (skipSilent && infoPoint->Flags & TRAP_SILENT) {
			continue;
		}

		if (!(infoPoint->GetInternalFlag() & IF_ACTIVE))
			continue;

		if (infoPoint->outline) {
			if (infoPoint->outline->PointIn(p)) {
				return infoPoint;
			}
		} else if (infoPoint->BBox.PointInside(p)) {
			return infoPoint;
		}
	}
	return NULL;
}

InfoPoint* TileMap::GetInfoPoint(const ieVariable& Name) const
{
	for (InfoPoint *infoPoint : infoPoints) {
		if (infoPoint->GetScriptName() == Name) {
			return infoPoint;
		}
	}
	return NULL;
}

InfoPoint* TileMap::GetInfoPoint(size_t idx) const
{
	if (idx >= infoPoints.size()) {
		return NULL;
	}
	return infoPoints[idx];
}

InfoPoint* TileMap::GetTravelTo(const ResRef& Destination) const
{
	for (InfoPoint *infoPoint : infoPoints) {
		if (infoPoint->Type != ST_TRAVEL) continue;

		if (infoPoint->Destination == Destination) {
			return infoPoint;
		}
	}
	return NULL;
}

InfoPoint* TileMap::AdjustNearestTravel(Point& p) const
{
	unsigned int min = UINT_MAX;
	InfoPoint* best = nullptr;

	for (InfoPoint *infoPoint : infoPoints) {
		if (infoPoint->Type != ST_TRAVEL) continue;

		unsigned int dist = SquaredDistance(p, infoPoint->Pos);
		if (dist < min) {
			min = dist;
			best = infoPoint;
		}
	}
	if (best) {
		p = best->Pos;
	}
	return best;
}

Size TileMap::GetMapSize() const
{
	return Size((XCellCount*64), (YCellCount*64));
}

}
