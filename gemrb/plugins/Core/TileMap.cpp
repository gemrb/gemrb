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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/TileMap.cpp,v 1.10 2003/11/30 09:49:57 avenger_teambg Exp $
 *
 */

#include "../../includes/win32def.h"
#include "TileMap.h"
#include "Interface.h"

extern Interface * core;

TileMap::TileMap(void)
{
	XCellCount = 0;
	YCellCount = 0;
}

TileMap::~TileMap(void)
{
	for(int i = 0; i < overlays.size(); i++) {
		delete(overlays[i]);
	}
}

void TileMap::AddOverlay(TileOverlay * overlay)
{
	if(overlay->w > XCellCount)
		XCellCount = overlay->w;
	if(overlay->h > YCellCount)
		YCellCount = overlay->h;
	overlays.push_back(overlay);
}

Door * TileMap::AddDoor(char * Name, unsigned char DoorClosed, unsigned short * indexes, int count, Gem_Polygon * open, Gem_Polygon * closed)
{
	Door door;
	strncpy(door.Name, Name, 8);
	door.tiles = indexes;
	door.count = count;
	door.open = open;
	door.closed = closed;
	door.DoorClosed = DoorClosed;
	for(int i = 0; i < count; i++) {
		overlays[0]->tiles[indexes[i]]->tileIndex = DoorClosed;
	}
	doors.push_back(door);
	return &doors.at(doors.size()-1);
}

void TileMap::ToggleDoor(Door * door)
{
	unsigned char state = 0;
	door->DoorClosed = !door->DoorClosed;
	if(door->DoorClosed) {
		state = 1;
		if(door->CloseSound[0] != '\0')
			core->GetSoundMgr()->Play(door->CloseSound);
	}
	else {
		if(door->OpenSound[0] != '\0')
			core->GetSoundMgr()->Play(door->OpenSound);
	}
	for(int i = 0; i < door->count; i++) {
		overlays[0]->tiles[door->tiles[i]]->tileIndex = state;
	}
}

void TileMap::DrawOverlay(unsigned int index, Region viewport)
{
	if(index < overlays.size())
		overlays[index]->Draw(viewport);
}

Door * TileMap::GetDoor(unsigned short x, unsigned short y)
{
	for(int i = 0; i < doors.size(); i++) {
		Door * door = &doors.at(i);
		if(door->DoorClosed) {
			if(door->closed->BBox.x > x)
				continue;
			if(door->closed->BBox.y > y)
				continue;
			if(door->closed->BBox.x+door->closed->BBox.w < x)
				continue;
			if(door->closed->BBox.y+door->closed->BBox.h < y)
				continue;
			if(door->closed->PointIn(x,y))
				return door;
		}
		else {
			if(door->open->BBox.x > x)
				continue;
			if(door->open->BBox.y > y)
				continue;
			if(door->open->BBox.x+door->open->BBox.w < x)
				continue;
			if(door->open->BBox.y+door->open->BBox.h < y)
				continue;
			if(door->open->PointIn(x,y))
				return door;
		}
	}
	return NULL;
}
Container * TileMap::AddContainer(char * Name, unsigned short Type, Gem_Polygon * outline)
{
	Container c;
	strncpy(c.Name, Name, 32);
	c.Type = Type;
	c.outline = outline;
	containers.push_back(c);
	return &containers.at(containers.size()-1);
}
Container * TileMap::GetContainer(unsigned short x, unsigned short y)
{
	for(int i = 0; i < containers.size(); i++) {
		Container * c = &containers.at(i);
		if(c->outline->BBox.x > x)
			continue;
		if(c->outline->BBox.y > y)
			continue;
		if(c->outline->BBox.x+c->outline->BBox.w < x)
			continue;
		if(c->outline->BBox.y+c->outline->BBox.h < y)
			continue;
		if(c->outline->PointIn(x,y))
			return c;
	}
	return NULL;	
}
InfoPoint * TileMap::AddInfoPoint(char * Name, unsigned short Type, Gem_Polygon * outline)
{
	InfoPoint ip;
	strncpy(ip.Name, Name, 32);
	ip.Type = Type;
	ip.outline = outline;
	infoPoints.push_back(ip);
	return &infoPoints.at(infoPoints.size()-1);
}
InfoPoint * TileMap::GetInfoPoint(unsigned short x, unsigned short y)
{
	for(int i = 0; i < infoPoints.size(); i++) {
		InfoPoint * ip = &infoPoints.at(i);
		if(ip->outline->BBox.x > x)
			continue;
		if(ip->outline->BBox.y > y)
			continue;
		if(ip->outline->BBox.x+ip->outline->BBox.w < x)
			continue;
		if(ip->outline->BBox.y+ip->outline->BBox.h < y)
			continue;
		if(ip->outline->PointIn(x,y))
			return ip;
	}
	return NULL;	
}
