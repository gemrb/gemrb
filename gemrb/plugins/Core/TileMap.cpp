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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/TileMap.cpp,v 1.33 2005/01/09 14:54:57 edheldil Exp $
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

Door* TileMap::AddDoor(char* Name, unsigned long Flags, int ClosedIndex,
	unsigned short* indexes, int count, Gem_Polygon* open, Gem_Polygon* closed)
{
	Door* door = new Door( overlays[0] );
	door->Flags = Flags;
	door->closedIndex = ClosedIndex;
	door->SetTiles( indexes, count );
	door->SetPolygon( false, open );
	door->SetPolygon( true, closed );
	if ( Flags&1 )  {
		door->SetDoorClosed( true );
	} else {
		door->SetDoorClosed( false );
	}
	door->SetName( Name );
	doors.push_back( door );
	return doors.at( doors.size() - 1 );
}

void TileMap::DrawOverlay(unsigned int index, Region viewport)
{
	if (index < overlays.size()) {
		overlays[index]->Draw( viewport );
	}
}

#define CELL_SIZE  32
#define CELL_RATIO 2

//vp: 45 480 640 407 ; viewport: 0 0 640 407 ; Xcc, Ycc: 50 43 ; wh: 200 172
//vid->SetViewport(45 480)
//sx, sy: 2 30 ; dx, dy: 43 56 ; vp: 0 0 640 407

void TileMap::DrawExploredBitmap(ieByte* mask, Region viewport)
{

	// viewport - pos & size of the control
	int w = XCellCount * CELL_RATIO;
	int h = YCellCount * CELL_RATIO;
	Color black = { 0, 0, 0, 255 };

	Video* vid = core->GetVideoDriver();
	Region vp = vid->GetViewport();
	//printf("vp: %d %d %d %d ; viewport: %d %d %d %d ; Xcc, Ycc: %d %d ; wh: %d %d \n", vp.x, vp.y, vp.w, vp.h, viewport.x, viewport.y, viewport.w, viewport.h, XCellCount, YCellCount, w, h);
	vp.x += viewport.x;
	vp.y += viewport.y;
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
	vid->SetViewport( vp.x - viewport.x, vp.y - viewport.y );
	//printf("vid->SetViewport(%d %d)\n", vp.x - viewport.x, vp.y - viewport.y );
	int sx = ( vp.x - viewport.x ) / CELL_SIZE;
	int sy = ( vp.y - viewport.y ) / CELL_SIZE;
	int dx = ( vp.x + vp.w + CELL_SIZE - 1 ) / CELL_SIZE;
	int dy = ( vp.y + vp.h + CELL_SIZE - 1 ) / CELL_SIZE;
	vp.x = viewport.x;
	vp.y = viewport.y;
	vp.w = viewport.w;
	vp.h = viewport.h;
	//printf("sx, sy: %d %d ; dx, dy: %d %d ; vp: %d %d %d %d\n", sx, sy, dx, dy, vp.x, vp.y, vp.w, vp.h);
	for (int y = sy; y < dy && y < h; y++) {
		for (int x = sx; x < dx && x < w; x++) {
		  /*
			Tile* tile = tiles[( y* w ) + x];
			//this hack is for alternate tiles with a value of -1
			if (!tile->anim[tile->tileIndex]) {
		        	tile->tileIndex=0;
			}
		  */
			int b0 = (w * y + x);
		  int bb = b0 / 8;
		  int bi = b0 % 8;

		  
		  if (!(mask[bb] & (1 << bi))) {
			  Region r = Region(viewport.x + ( (x - sx) * CELL_SIZE ), viewport.y + ( (y - sy) * CELL_SIZE ), CELL_SIZE, CELL_SIZE);
			  vid->DrawRect(r, black, true, true);
			}
			/*
				vid->BlitSprite( tile->anim[tile->tileIndex]->NextFrame(),
						viewport.x + ( x * 16 ), viewport.y + ( y * 16 ),
						false, &vp );
			*/
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
		if (stricmp( door->Name, Name ) == 0)
			return door;
	}
	return NULL;
}

Container* TileMap::AddContainer(char* Name, unsigned short Type,
	Gem_Polygon* outline)
{
	Container* c = new Container();
	strncpy( c->Name, Name, 32 );
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
		int len = ( int ) strlen( cn->Name );
		int p = 0;
		for (int x = 0; x < len; x++) {
			if (cn->Name[x] == ' ')
				continue;
			if (cn->Name[x] != Name[p])
				break;
			if (x == len - 1)
				return cn;
			p++;
		}
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
	Container *container = GetContainer(position,CN_PILE);
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
	        container = AddContainer(heapname,CN_HEAP, outline);
		container->Pos=position;
	}
	container->inventory.AddItem(item);
}

InfoPoint* TileMap::AddInfoPoint(char* Name, unsigned short Type,
	Gem_Polygon* outline)
{
	InfoPoint* ip = new InfoPoint();
	strncpy( ip->Name, Name, 32 );
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
		int len = ( int ) strlen( ip->Name );
		int p = 0;
		for (int x = 0; x < len; x++) {
			if (ip->Name[x] == ' ')
				continue;
			if (ip->Name[x] != Name[p])
				break;
			if (x == len - 1)
				return ip;
			p++;
		}
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
