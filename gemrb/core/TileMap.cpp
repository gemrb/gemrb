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
#include "Video.h"

#include "Scriptable/Container.h"
#include "Scriptable/Door.h"
#include "Scriptable/InfoPoint.h"

namespace GemRB {

TileMap::TileMap(void)
{
	XCellCount = 0;
	YCellCount = 0;
	LargeMap = !core->HasFeature(GF_SMALL_FOG);
}

TileMap::~TileMap(void)
{
	size_t i;

	for (i = 0; i < overlays.size(); i++) {
		delete( overlays[i] );
	}
	for (i = 0; i < overlays.size(); i++) {
		delete( rain_overlays[i]);
	}
	for (i = 0; i < infoPoints.size(); i++) {
		delete( infoPoints[i] );
	}
	for (i = 0; i < containers.size(); i++) {
		delete( containers[i] );
	}
	for (i = 0; i < doors.size(); i++) {
		delete( doors[i] );
	}
}

//this needs in case of a tileset switch (for extended night)
void TileMap::ClearOverlays()
{
	size_t i;

	for (i = 0; i < overlays.size(); i++) {
		delete( overlays[i] );
	}
	overlays.clear();
	for (i = 0; i < overlays.size(); i++) {
		delete( rain_overlays[i]);
	}
	rain_overlays.clear();
}

//tiled objects
TileObject* TileMap::AddTile(const char *ID, const char* Name, unsigned int Flags,
	unsigned short* openindices, int opencount, unsigned short* closeindices, int closecount)
{
	TileObject* tile = new TileObject();
	tile->Flags=Flags;
	strnspccpy(tile->Name, Name, 32);
	strnlwrcpy(tile->Tileset, ID, 8);
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
	int ClosedIndex, unsigned short* indices, int count,
	Gem_Polygon* open, Gem_Polygon* closed)
{
	Door* door = new Door( overlays[0] );
	door->Flags = Flags;
	door->closedIndex = ClosedIndex;
	door->SetTiles( indices, count );
	door->SetPolygon( false, closed );
	door->SetPolygon( true, open );
	door->SetName( ID );
	door->SetScriptName( Name );
	doors.push_back( door );
	return door;
}

Door* TileMap::GetDoor(unsigned int idx) const
{
	if (idx >= doors.size()) {
		return NULL;
	}
	return doors[idx];
}

Door* TileMap::GetDoor(const Point &p) const
{
	for (size_t i = 0; i < doors.size(); i++) {
		Gem_Polygon *doorpoly;

		Door* door = doors[i];
		if (door->Flags&DOOR_HIDDEN) {
			continue;
		}
		if (door->Flags&DOOR_OPEN)
			doorpoly = door->open;
		else
			doorpoly = door->closed;

		if (doorpoly->BBox.x > p.x)
			continue;
		if (doorpoly->BBox.y > p.y)
			continue;
		if (doorpoly->BBox.x + doorpoly->BBox.w < p.x)
			continue;
		if (doorpoly->BBox.y + doorpoly->BBox.h < p.y)
			continue;
		if (doorpoly->PointIn( p ))
			return door;
	}
	return NULL;
}

Door* TileMap::GetDoorByPosition(const Point &p) const
{
	for (size_t i = 0; i < doors.size(); i++) {
		Door* door = doors[i];

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
	for (size_t i = 0; i < doors.size(); i++) {
		Door* door = doors[i];
		if (stricmp( door->GetScriptName(), Name ) == 0)
			return door;
	}
	return NULL;
}

void TileMap::UpdateDoors()
{
	for (size_t i = 0; i < doors.size(); i++) {
		Door* door = doors[i];
		door->SetNewOverlay(overlays[0]);
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

void TileMap::DrawOverlays(Region screen, int rain, int flags)
{
	if (rain) {
		overlays[0]->Draw( screen, rain_overlays, flags );
	} else {
		overlays[0]->Draw( screen, overlays, flags );
	}
}

// Size of Fog-Of-War shadow tile (and bitmap)
#define CELL_SIZE  32

// Ratio of bg tile size and fog tile size
#define CELL_RATIO 2

// Returns 1 if map at (x;y) was explored, else 0. Points outside map are
//   always considered as explored
#define IS_EXPLORED( x, y )   (((x) < 0 || (x) >= w || (y) < 0 || (y) >= h) ? 1 : (explored_mask[(w * (y) + (x)) / 8] & (1 << ((w * (y) + (x)) % 8))))

#define IS_VISIBLE( x, y )   (((x) < 0 || (x) >= w || (y) < 0 || (y) >= h) ? 1 : (visible_mask[(w * (y) + (x)) / 8] & (1 << ((w * (y) + (x)) % 8))))

#define FOG(i)  vid->BlitSprite( core->FogSprites[i], r.x, r.y, true, &r )


void TileMap::DrawFogOfWar(ieByte* explored_mask, ieByte* visible_mask, Region viewport)
{
	// viewport - pos & size of the control
	int w = XCellCount * CELL_RATIO;
	int h = YCellCount * CELL_RATIO;
	if (LargeMap) {
		w++;
		h++;
	}

	Video* vid = core->GetVideoDriver();
	Region vp = vid->GetViewport();

	vp.w = viewport.w;
	vp.h = viewport.h;
	if (( vp.x + vp.w ) > w * CELL_SIZE) {
		vp.x = ( w * CELL_SIZE - vp.w );
	}
	if (vp.x < 0) {
		vp.x = 0;
	}
	if (( vp.y + vp.h ) > h * CELL_SIZE) {
		vp.y = ( h * CELL_SIZE - vp.h );
	}
	if (vp.y < 0) {
		vp.y = 0;
	}
	int sx = ( vp.x ) / CELL_SIZE;
	int sy = ( vp.y ) / CELL_SIZE;
	int dx = sx + vp.w / CELL_SIZE + 2;
	int dy = sy + vp.h / CELL_SIZE + 2;
	int x0 = sx * CELL_SIZE - vp.x;
	int y0 = sy * CELL_SIZE - vp.y;
	if (LargeMap) {
		x0 -= CELL_SIZE / 2;
		y0 -= CELL_SIZE / 2;
		dx++;
		dy++;
	}
	for (int y = sy; y < dy && y < h; y++) {
		for (int x = sx; x < dx && x < w; x++) {
			Region r = Region(x0 + viewport.x + ( (x - sx) * CELL_SIZE ), y0 + viewport.y + ( (y - sy) * CELL_SIZE ), CELL_SIZE, CELL_SIZE);
			if (! IS_EXPLORED( x, y )) {
				// Unexplored tiles are all black
				vid->DrawRect(r, ColorBlack, true, true);
				continue;  // Don't draw 'invisible' fog
			}
			else {
				// If an explored tile is adjacent to an
				//   unexplored one, we draw border sprite
				//   (gradient black <-> transparent)
				// Tiles in four cardinal directions have these
				//   values.
				//
				//      1
				//    2   8
				//      4
				//
				// Values of those unexplored are
				//   added together, the resulting number being
				//   an index of shadow sprite to use. For now,
				//   some tiles are made 'on the fly' by
				//   drawing two or more tiles

				int e = ! IS_EXPLORED( x, y - 1);
				if (! IS_EXPLORED( x - 1, y )) e |= 2;
				if (! IS_EXPLORED( x, y + 1 )) e |= 4;
				if (! IS_EXPLORED( x + 1, y )) e |= 8;

				switch (e) {
				case 1:
				case 2:
				case 3:
				case 4:
				case 6:
				case 8:
				case 9:
				case 12:
					FOG( e );
					break;
				case 5:
					FOG( 1 );
					FOG( 4 );
					break;
				case 7:
					FOG( 3 );
					FOG( 6 );
					break;
				case 10:
					FOG( 2 );
					FOG( 8 );
					break;
				case 11:
					FOG( 3 );
					FOG( 9 );
					break;
				case 13:
					FOG( 9 );
					FOG( 12 );
					break;
				case 14:
					FOG( 6 );
					FOG( 12 );
					break;
				case 15: //this is black too
					vid->DrawRect(r, ColorBlack, true, true);
					break;
				}
			}

			if (! IS_VISIBLE( x, y )) {
				// Invisible tiles are all gray
				FOG( 16 );
				continue;  // Don't draw 'invisible' fog
			}
			else {
				// If a visible tile is adjacent to an
				//   invisible one, we draw border sprite
				//   (gradient gray <-> transparent)
				// Tiles in four cardinal directions have these
				//   values.
				//
				//      1
				//    2   8
				//      4
				//
				// Values of those invisible are
				//   added together, the resulting number being
				//   an index of shadow sprite to use. For now,
				//   some tiles are made 'on the fly' by
				//   drawing two or more tiles

				int e = ! IS_VISIBLE( x, y - 1);
				if (! IS_VISIBLE( x - 1, y )) e |= 2;
				if (! IS_VISIBLE( x, y + 1 )) e |= 4;
				if (! IS_VISIBLE( x + 1, y )) e |= 8;

				switch (e) {
				case 1:
				case 2:
				case 3:
				case 4:
				case 6:
				case 8:
				case 9:
				case 12:
					FOG( 16 + e );
					break;
				case 5:
					FOG( 16 + 1 );
					FOG( 16 + 4 );
					break;
				case 7:
					FOG( 16 + 3 );
					FOG( 16 + 6 );
					break;
				case 10:
					FOG( 16 + 2 );
					FOG( 16 + 8 );
					break;
				case 11:
					FOG( 16 + 3 );
					FOG( 16 + 9 );
					break;
				case 13:
					FOG( 16 + 9 );
					FOG( 16 + 12 );
					break;
				case 14:
					FOG( 16 + 6 );
					FOG( 16 + 12 );
					break;
				case 15: //this is unseen too
					FOG( 16 );
					break;
				}
			}
		}
	}
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
	for (size_t i = 0; i < containers.size(); i++) {
		Container* cn = containers[i];
		if (stricmp( cn->GetScriptName(), Name ) == 0)
			return cn;
	}
	return NULL;
}

//look for a container at position
//use type = IE_CONTAINER_PILE if you want to find ground piles only
//in this case, empty piles won't be found!
Container* TileMap::GetContainer(const Point &position, int type) const
{
	for (size_t i = 0; i < containers.size(); i++) {
		Container* c = containers[i];
		if (type!=-1) {
			if (c->Type!=type) {
				continue;
			}
		}
		if (c->outline->BBox.x > position.x)
			continue;
		if (c->outline->BBox.y > position.y)
			continue;
		if (c->outline->BBox.x + c->outline->BBox.w < position.x)
			continue;
		if (c->outline->BBox.y + c->outline->BBox.h < position.y)
			continue;

		//IE piles don't have polygons, the bounding box is enough for them
		if (c->Type == IE_CONTAINER_PILE) {
			//don't find empty piles if we look for any container
			//if we looked only for piles, then we still return them
			if ((type==-1) && !c->inventory.GetSlotCount()) {
				continue;
			}
			return c;
		}
		if (c->outline->PointIn( position ))
			return c;
	}
	return NULL;
}

Container* TileMap::GetContainerByPosition(const Point &position, int type) const
{
	for (size_t i = 0; i < containers.size(); i++) {
		Container* c = containers[i];
		if (type!=-1) {
			if (c->Type!=type) {
				continue;
			}
		}

		if (c->Pos.x!=position.x || c->Pos.y!=position.y) {
			continue;
		}

		//IE piles don't have polygons, the bounding box is enough for them
		if (c->Type == IE_CONTAINER_PILE) {
			//don't find empty piles if we look for any container
			//if we looked only for piles, then we still return them
			if ((type==-1) && !c->inventory.GetSlotCount()) {
				continue;
			}
			return c;
		}
		return c;
	}
	return NULL;
}

int TileMap::CleanupContainer(Container *container)
{
	if (container->Type!=IE_CONTAINER_PILE)
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
InfoPoint* TileMap::AddInfoPoint(const char* Name, unsigned short Type,
	Gem_Polygon* outline)
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
	//ip->Active = true; //set active on creation
	infoPoints.push_back( ip );
	return ip;
}

//if detectable is set, then only detectable infopoints will be returned
InfoPoint* TileMap::GetInfoPoint(const Point &p, bool detectable) const
{
	for (size_t i = 0; i < infoPoints.size(); i++) {
		InfoPoint* ip = infoPoints[i];
		//these flags disable any kind of user interaction
		//scripts can still access an infopoint by name
		if (ip->Flags&(INFO_DOOR|TRAP_DEACTIVATED) )
			continue;

		if (detectable) {
			if ((ip->Type==ST_PROXIMITY) && !ip->VisibleTrap(0) ) {
				continue;
			}
			if (ip->IsPortal()) {
				// skip portals without PORTAL_CURSOR set
				if (!(ip->Trapped & PORTAL_CURSOR)) {
					continue;
				}
			}
		}

		if (!(ip->GetInternalFlag()&IF_ACTIVE))
			continue;
		if (ip->outline->BBox.x > p.x)
			continue;
		if (ip->outline->BBox.y > p.y)
			continue;
		if (ip->outline->BBox.x + ip->outline->BBox.w < p.x)
			continue;
		if (ip->outline->BBox.y + ip->outline->BBox.h < p.y)
			continue;
		if (ip->outline->PointIn( p ))
			return ip;
	}
	return NULL;
}

InfoPoint* TileMap::GetInfoPoint(const char* Name) const
{
	for (size_t i = 0; i < infoPoints.size(); i++) {
		InfoPoint* ip = infoPoints[i];
		if (stricmp( ip->GetScriptName(), Name ) == 0)
			return ip;
	}
	return NULL;
}

InfoPoint* TileMap::GetInfoPoint(unsigned int idx) const
{
	if (idx >= infoPoints.size()) {
		return NULL;
	}
	return infoPoints[idx];
}

InfoPoint* TileMap::GetTravelTo(const char* Destination) const
{
	size_t i=infoPoints.size();
	while (i--) {
		InfoPoint *ip = infoPoints[i];

		if (ip->Type!=ST_TRAVEL)
			continue;

		if (strnicmp( ip->Destination, Destination, 8 ) == 0) {
			return ip;
		}
	}
	return NULL;
}

InfoPoint *TileMap::AdjustNearestTravel(Point &p)
{
	int min = -1;
	InfoPoint *best = NULL;

	size_t i=infoPoints.size();
	while (i--) {
		InfoPoint *ip = infoPoints[i];

		if (ip->Type!=ST_TRAVEL)
			continue;

		unsigned int dist = Distance(p, ip);
		if (dist<(unsigned int) min) {
			min = dist;
			best = ip;
		}
	}
	if (best) {
		p = best->Pos;
	}
	return best;
}

Point TileMap::GetMapSize()
{
	return Point((short) (XCellCount*64), (short) (YCellCount*64));
}

}
