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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/TileMap.cpp,v 1.19 2004/01/04 00:18:56 balrog994 Exp $
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
	for(size_t i = 0; i < overlays.size(); i++) {
		delete(overlays[i]);
	}
	for(size_t i = 0; i < infoPoints.size(); i++) {
		delete(infoPoints[i]);
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

Door * TileMap::AddDoor(char * Name, bool DoorClosed, unsigned short * indexes, int count, Gem_Polygon * open, Gem_Polygon * closed)
{
	Door *door = new Door(overlays[0]);
	door->SetTiles(indexes, count);
	door->SetPolygon(true, open);
	door->SetPolygon(false, closed);
	door->SetDoorClosed(DoorClosed);
	door->SetName(Name);
	doors.push_back(door);
	return doors.at(doors.size()-1);
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

Door * TileMap::GetDoor(unsigned int idx)
{
	if(idx>=doors.size() ) return NULL;
	return doors.at(idx);
}

Door * TileMap::GetDoor(unsigned short x, unsigned short y)
{
  for(size_t i = 0; i < doors.size(); i++) {
		Door * door = doors.at(i);
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

Door * TileMap::GetDoor(const char * Name)
{
	if(!Name)
		return NULL;
	for(size_t i = 0; i < doors.size(); i++) {
		Door * door = doors.at(i);
		if(stricmp(door->Name, Name) == 0)
			return door;
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
Container *TileMap::GetContainer(unsigned int idx)
{
	if(idx>=containers.size()) return NULL;
	return &containers.at(idx);
}

Container * TileMap::GetContainer(unsigned short x, unsigned short y)
{
	for(size_t i = 0; i < containers.size(); i++) {
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
	InfoPoint *ip = new InfoPoint();
	strncpy(ip->Name, Name, 32);
	switch(Type) {
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
	infoPoints.push_back(ip);
	return infoPoints.at(infoPoints.size()-1);
}
InfoPoint * TileMap::GetInfoPoint(unsigned short x, unsigned short y)
{
	for(size_t i = 0; i < infoPoints.size(); i++) {
		InfoPoint * ip = infoPoints.at(i);
		if(!ip->Active)
			continue;
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

InfoPoint * TileMap::GetInfoPoint(const char * Name)
{
	for(size_t i = 0; i < infoPoints.size(); i++) {
		InfoPoint * ip = infoPoints.at(i);
		int len = (int)strlen(ip->Name);
		int p = 0;
		for(int x = 0; x < len; x++) {
			if(ip->Name[x] == ' ')
				continue;
			if(ip->Name[x] != Name[p])
				break;
			if(x == len-1)
				return ip;
			p++;
		}
	}
	return NULL;
}

InfoPoint * TileMap::GetInfoPoint(unsigned int idx)
{
	if(idx >= infoPoints.size()) return NULL;
	return infoPoints.at(idx);
}
