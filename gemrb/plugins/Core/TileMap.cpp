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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/TileMap.cpp,v 1.42 2005/03/19 16:15:57 avenger_teambg Exp $
 *
 */

#include "../../includes/win32def.h"
#include "TileMap.h"
#include "Interface.h"

extern Interface* core;

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

void TileMap::AddOverlay(TileOverlay* overlay)
{
	if (overlay->w > XCellCount) {
		XCellCount = overlay->w;
	}
	if (overlay->h > YCellCount) {
		YCellCount = overlay->h;
	}
	overlays.push_back( overlay );
}

Door* TileMap::AddDoor(const char *ID, const char* Name, unsigned int Flags,
	int ClosedIndex, unsigned short* indexes, int count,
	Gem_Polygon* open, Gem_Polygon* closed)
{
	Door* door = new Door( overlays[0] );
	door->Flags = Flags;
	door->closedIndex = ClosedIndex;
	door->SetTiles( indexes, count );
	door->SetPolygon( false, open );
	door->SetPolygon( true, closed );
	door->SetName( ID );
	door->SetScriptName( Name );
	doors.push_back( door );
	return doors.at( doors.size() - 1 );
}

void TileMap::DrawOverlay(unsigned int index, Region viewport)
{
	if (index < overlays.size()) {
		overlays[index]->Draw( viewport );
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
	Color black = { 0, 0, 0, 255 };

	Video* vid = core->GetVideoDriver();
	Region vp = vid->GetViewport();

	vp.x -= viewport.x;
	vp.y -= viewport.y;
	vp.w = viewport.w;
	vp.h = viewport.h;
	if (( vp.x + vp.w ) > w * CELL_SIZE) {
		vp.x = ( w * CELL_SIZE - vp.w );
	}
	if (vp.x < viewport.x) {
		vp.x = viewport.x;
	}
	if (( vp.y + vp.h ) > h * CELL_SIZE) {
		vp.y = ( h * CELL_SIZE - vp.h );
	}
	if (vp.y < viewport.y) {
		vp.y = viewport.y;
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
				vid->DrawRect(r, black, true, true);
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
				if(! IS_EXPLORED( x - 1, y )) e |= 2;
				if(! IS_EXPLORED( x, y + 1 )) e |= 4;
				if(! IS_EXPLORED( x + 1, y )) e |= 8;

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
					vid->DrawRect(r, black, true, true);
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
				if(! IS_VISIBLE( x - 1, y )) e |= 2;
				if(! IS_VISIBLE( x, y + 1 )) e |= 4;
				if(! IS_VISIBLE( x + 1, y )) e |= 8;

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

Door* TileMap::GetDoor(unsigned int idx)
{
	if (idx >= doors.size()) {
		return NULL;
	}
	return doors[idx];
}

Door* TileMap::GetDoor(Point &p)
{
	for (size_t i = 0; i < doors.size(); i++) {
		Door* door = doors[i];
		if (door->Flags&1) {
			if (door->closed->BBox.x > p.x)
				continue;
			if (door->closed->BBox.y > p.y)
				continue;
			if (door->closed->BBox.x + door->closed->BBox.w < p.x)
				continue;
			if (door->closed->BBox.y + door->closed->BBox.h < p.y)
				continue;
			if (door->closed->PointIn( p ))
				return door;
		} else {
			if (door->open->BBox.x > p.x)
				continue;
			if (door->open->BBox.y > p.y)
				continue;
			if (door->open->BBox.x + door->open->BBox.w < p.x)
				continue;
			if (door->open->BBox.y + door->open->BBox.h < p.y)
				continue;
			if (door->open->PointIn( p ))
				return door;
		}
	}
	return NULL;
}

Door* TileMap::GetDoor(const char* Name)
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

Container* TileMap::AddContainer(const char* Name, unsigned short Type,
	Gem_Polygon* outline)
{
	Container* c = new Container();
	c->SetScriptName( Name );
	c->Type = Type;
	c->outline = outline;
	containers.push_back( c );
	return c;
}
Container* TileMap::GetContainer(unsigned int idx)
{
	if (idx >= containers.size()) {
		return NULL;
	}
	return containers[idx];
}

Container* TileMap::GetContainer(const char* Name)
{
	for (size_t i = 0; i < containers.size(); i++) {
		Container* cn = containers[i];
		if (stricmp( cn->GetScriptName(), Name ) == 0)
			return cn;
	}
	return NULL;
}

Container* TileMap::GetContainer(Point &position, int type)
{
	for (size_t i = 0; i < containers.size(); i++) {
		Container* c = containers[i];
		if(type!=-1) {
			if(c->Type!=type)
				continue;
		}
		if (c->outline->BBox.x > position.x)
			continue;
		if (c->outline->BBox.y > position.y)
			continue;
		if (c->outline->BBox.x + c->outline->BBox.w < position.x)
			continue;
		if (c->outline->BBox.y + c->outline->BBox.h < position.y)
			continue;
		if (c->outline->PointIn( position ))
			return c;
	}
	return NULL;
}

void TileMap::AddItemToLocation(Point &position, CREItem *item)
{
	Point tmp[4];
	char heapname[32];
	sprintf(heapname,"heap_%hd.%hd",position.x,position.y);
	Container *container = GetContainer(position,IE_CONTAINER_PILE);
	if(!container) {
		tmp[0].x=position.x-5;
		tmp[0].y=position.y-5;
		tmp[1].x=position.x+5;
		tmp[1].y=position.y-5;
		tmp[2].x=position.x+5;
		tmp[2].y=position.y+5;
		tmp[3].x=position.x-5;
		tmp[3].y=position.y+5;
		Gem_Polygon* outline = new Gem_Polygon( tmp, 4 );
		container = AddContainer(heapname,IE_CONTAINER_PILE, outline);
		container->Pos=position;
	}
	container->inventory.AddItem(item);
}

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
	}
	ip->outline = outline;
	ip->Active = true;
	infoPoints.push_back( ip );
	return infoPoints.at( infoPoints.size() - 1 );
}
InfoPoint* TileMap::GetInfoPoint(Point &p)
{
	for (size_t i = 0; i < infoPoints.size(); i++) {
		InfoPoint* ip = infoPoints[i];
		//these flags disable any kind of user interaction
		//scripts can still access an infopoint by name
		if (ip->Flags&(INFO_DOOR|TRAP_DEACTIVATED) )
			continue;

		if (!ip->Active)
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

InfoPoint* TileMap::GetInfoPoint(const char* Name)
{
	for (size_t i = 0; i < infoPoints.size(); i++) {
		InfoPoint* ip = infoPoints[i];
		if (stricmp( ip->GetScriptName(), Name ) == 0)
			return ip;
	}
	return NULL;
}

InfoPoint* TileMap::GetInfoPoint(unsigned int idx)
{
	if (idx >= infoPoints.size()) {
		return NULL;
	}
	return infoPoints[idx];
}
