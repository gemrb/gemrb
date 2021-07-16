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
#include "Video/Video.h"

#include "Scriptable/Container.h"
#include "Scriptable/Door.h"
#include "Scriptable/InfoPoint.h"

namespace GemRB {

TileMap::~TileMap(void)
{
	ClearOverlays();

	for (const InfoPoint *infoPoint : infoPoints) {
		delete infoPoint;
	}

	for (const Door *door : doors) {
		delete door;
	}
}

//this needs in case of a tileset switch (for extended night)
void TileMap::ClearOverlays()
{
	for (const TileOverlay *overlay : overlays) {
		delete overlay;
	}
	for (const TileOverlay *rain : rain_overlays) {
		delete rain;
	}
	overlays.clear();
	rain_overlays.clear();
}

//tiled objects
TileObject* TileMap::AddTile(const char *ID, const char* Name, unsigned int Flags,
	unsigned short* openindices, int opencount, unsigned short* closeindices, int closecount)
{
	TileObject* tile = new TileObject();
	tile->Flags=Flags;
	strnspccpy(tile->Name, Name, 32);
	tile->Tileset = ID;
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
Door* TileMap::AddDoor(const char *ID, const char* Name, unsigned int Flags,
	int ClosedIndex, unsigned short* indices, int count, DoorTrigger&& dt)
{
	Door* door = new Door(overlays[0], std::move(dt));
	door->Flags = Flags;
	door->closedIndex = ClosedIndex;
	door->SetTiles( indices, count );
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

Door* TileMap::GetDoor(const char* Name) const
{
	if (!Name) {
		return NULL;
	}
	for (Door *door : doors) {
		if (stricmp( door->GetScriptName(), Name ) == 0)
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
	if (!core->HasFeature(GF_RANDOM_BANTER_DIALOGS)) return;

	for (Door *door : doors) {
		if (door->CantAutoClose()) continue;
		if (core->Roll(1, 2, -1)) continue; // just a guess
		door->SetDoorOpen(false, false, 0);
	}
}

//overlays, allow pushing of NULL
void TileMap::AddOverlay(TileOverlay* overlay)
{
	if (overlay) {
		if (overlay->w > XCellCount) {
			XCellCount = overlay->w;
		}
		if (overlay->h > YCellCount) {
			YCellCount = overlay->h;
		}
	}
	overlays.push_back( overlay );
}

void TileMap::AddRainOverlay(TileOverlay* overlay)
{
	if (overlay) {
		if (overlay->w > XCellCount) {
			XCellCount = overlay->w;
		}
		if (overlay->h > YCellCount) {
			YCellCount = overlay->h;
		}
	}
	rain_overlays.push_back( overlay );
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

Container* TileMap::GetContainer(unsigned int idx) const
{
	if (idx >= containers.size()) {
		return NULL;
	}
	return containers[idx];
}

Container* TileMap::GetContainer(const char* Name) const
{
	for (Container *container : containers) {
		if (stricmp(container->GetScriptName(), Name) == 0) {
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
		} else if (container->outline->PointIn(position)) {
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
	Log(ERROR, "TileMap", "Invalid container cleanup: %s",
		container->GetScriptName());
	return 1;
}

//infopoints
InfoPoint* TileMap::AddInfoPoint(const char* Name, unsigned short Type, const std::shared_ptr<Gem_Polygon>& outline)
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
InfoPoint* TileMap::GetInfoPoint(const Point &p, bool detectable) const
{
	for (InfoPoint *infoPoint : infoPoints) {
		//these flags disable any kind of user interaction
		//scripts can still access an infopoint by name
		if (infoPoint->Flags & (INFO_DOOR | TRAP_DEACTIVATED))
			continue;

		if (detectable) {
			if (infoPoint->Type == ST_PROXIMITY && !infoPoint->VisibleTrap(0)) {
				continue;
			}
			// skip portals without PORTAL_CURSOR set
			if (infoPoint->IsPortal() && !(infoPoint->Trapped & PORTAL_CURSOR)) {
					continue;
			}
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

InfoPoint* TileMap::GetInfoPoint(const char* Name) const
{
	for (InfoPoint *infoPoint : infoPoints) {
		if (stricmp(infoPoint->GetScriptName(), Name) == 0) {
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

InfoPoint* TileMap::GetTravelTo(const char* Destination) const
{
	for (InfoPoint *infoPoint : infoPoints) {
		if (infoPoint->Type != ST_TRAVEL) continue;

		if (strnicmp(infoPoint->Destination, Destination, 8) == 0) {
			return infoPoint;
		}
	}
	return NULL;
}

InfoPoint *TileMap::AdjustNearestTravel(Point &p)
{
	int min = -1;
	InfoPoint *best = NULL;

	for (InfoPoint *infoPoint : infoPoints) {
		if (infoPoint->Type != ST_TRAVEL) continue;

		unsigned int dist = Distance(p, infoPoint);
		if (dist<(unsigned int) min) {
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
