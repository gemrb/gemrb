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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/WEDImporter/WEDImp.cpp,v 1.6 2003/11/30 09:53:09 avenger_teambg Exp $
 *
 */

#include "../../includes/win32def.h"
#include "WEDImp.h"
#include "../Core/TileSetMgr.h"
#include "../Core/Interface.h"

WEDImp::WEDImp(void)
{
	str = NULL;
	autoFree = false;
}

WEDImp::~WEDImp(void)
{
	if(str && autoFree)
		delete(str);
}

bool WEDImp::Open(DataStream * stream, bool autoFree)
{
	if(stream == NULL)
		return false;
	if(str && this->autoFree)
		delete(str);
	str = stream;
	this->autoFree = autoFree;
	char Signature[8];
	str->Read(Signature, 8);
	if(strncmp(Signature, "WED V1.3", 8) != 0) {
		printf("[WEDImporter]: This file is not a valid WED File\n");
		return false;
	}
	str->Read(&OverlaysCount, 4);
	str->Read(&DoorsCount, 4);
	str->Read(&OverlaysOffset, 4);
	str->Read(&SecHeaderOffset, 4);
	str->Read(&DoorsOffset, 4);
	str->Read(&DoorTilesOffset, 4);
	str->Seek(OverlaysOffset, GEM_STREAM_START);
	for(unsigned int i = 0; i < OverlaysCount; i++) {
		Overlay o;
		str->Read(&o.Width, 2);
		str->Read(&o.Height, 2);
		str->Read(o.TilesetResRef, 8);
		str->Seek(4, GEM_CURRENT_POS);
		str->Read(&o.TilemapOffset, 4);
		str->Read(&o.TILOffset, 4);
		overlays.push_back(o);
	}
	//Reading the Secondary Header
	str->Seek(SecHeaderOffset, GEM_STREAM_START);
	str->Read(&WallPolygonsCount, 4);
	str->Read(&PolygonsOffset, 4);
	str->Read(&VerticesOffset, 4);
	str->Read(&WallGroupsOffset, 4);
	str->Read(&PILTOffset, 4);
	return true;
}

TileMap * WEDImp::GetTileMap()
{
	TileMap * tm = new TileMap();
	//TODO: Implement Multi Overlay
	TileOverlay * over = new TileOverlay(overlays[0].Width, overlays[0].Height);
	DataStream * tisfile = core->GetResourceMgr()->GetResource(overlays[0].TilesetResRef, IE_TIS_CLASS_ID);
	if(!core->IsAvailable(IE_TIS_CLASS_ID)) {
		printf("[WEDImporter]: No TileSet Importer Available.\n");
		return NULL;
	}
	TileSetMgr * tis = (TileSetMgr*)core->GetInterface(IE_TIS_CLASS_ID);
	tis->Open(tisfile);
	for(int y = 0; y < overlays[0].Height; y++) {
		for(int x = 0; x < overlays[0].Width; x++) {
			str->Seek(overlays[0].TilemapOffset + (y*overlays[0].Width*10) + (x*10), GEM_STREAM_START);
			unsigned short	startindex, count, secondary;
			unsigned char overlaymask;
			str->Read(&startindex, 2);
			str->Read(&count, 2);
			str->Read(&secondary, 2);
			str->Read(&overlaymask, 1);
			//TODO: Consider Alternative Tile and Overlay Mask
			str->Seek(overlays[0].TILOffset + (startindex*2), GEM_STREAM_START);
			unsigned short *indexes = (unsigned short*)malloc(count*2);
			str->Read(indexes, count*2);
			Tile * tile;
			if(secondary == 0xffff)
				tile = tis->GetTile(indexes, count);
			else {
				tile = tis->GetTile(indexes, 1, &secondary);
			}
			tile->om = overlaymask;
			over->AddTile(tile);
			free(indexes);
		}
	}
	tm->AddOverlay(over);
	//Clipping Polygons
	/*
	for(int d = 0; d < DoorsCount; d++) {
		str->Seek(DoorsOffset + (d*0x1A), GEM_STREAM_START);
		unsigned short DoorClosed, DoorTileStart, DoorTileCount, *DoorTiles;
		unsigned short OpenPolyCount, ClosedPolyCount;
		unsigned long OpenPolyOffset, ClosedPolyOffset;
		char Name[9];
		str->Read(Name, 8);
		Name[8] = 0;
		str->Read(&DoorClosed, 2);
		str->Read(&DoorTileStart, 2);
		str->Read(&DoorTileCount, 2);
		str->Read(&OpenPolyCount, 2);
		str->Read(&ClosedPolyCount, 2);
		str->Read(&OpenPolyOffset, 4);
		str->Read(&ClosedPolyOffset, 4);
		//Reading Door Tile Cells
		str->Seek(DoorTilesOffset + (DoorTileStart*2), GEM_STREAM_START);
		DoorTiles = (unsigned short*)malloc(DoorTileCount*sizeof(unsigned short));
		memset(DoorTiles, 0, DoorTileCount*sizeof(unsigned short));
		str->Read(DoorTiles, DoorTileCount*sizeof(unsigned short));
		//Reading the Open Polygon
		str->Seek(OpenPolyOffset, GEM_STREAM_START);
		unsigned long StartingVertex, VerticesCount;
		unsigned short BitFlag, MinX, MaxX, MinY, MaxY;
		str->Read(&StartingVertex, 4);
		str->Read(&VerticesCount, 4);
		str->Read(&BitFlag, 2);
		str->Read(&MinX, 2);
		str->Read(&MaxX, 2);
		str->Read(&MinY, 2);
		str->Read(&MaxY, 2);
		//Reading Vertices
		str->Seek(VerticesOffset + (StartingVertex*4), GEM_STREAM_START);
		Point * points = (Point*)malloc(VerticesCount*sizeof(Point));
		for(int i = 0; i < VerticesCount; i++) {
			str->Read(&points[i].x, 2);
			str->Read(&points[i].y, 2);
		}
		Gem_Polygon * open = new Gem_Polygon(points, VerticesCount);
		free(points);
		open->BBox.x = MinX;
		open->BBox.y = MinY;
		open->BBox.w = MaxX-MinX;
		open->BBox.h = MaxY-MinY;
		//Reading the closed Polygon
		str->Seek(ClosedPolyOffset, GEM_STREAM_START);
		str->Read(&StartingVertex, 4);
		str->Read(&VerticesCount, 4);
		str->Read(&BitFlag, 2);
		str->Read(&MinX, 2);
		str->Read(&MaxX, 2);
		str->Read(&MinY, 2);
		str->Read(&MaxY, 2);
		//Reading Vertices
		str->Seek(VerticesOffset + (StartingVertex*4), GEM_STREAM_START);
		points = (Point*)malloc(VerticesCount*sizeof(Point));
		for(int i = 0; i < VerticesCount; i++) {
			str->Read(&points[i].x, 2);
			str->Read(&points[i].y, 2);
		}
		Gem_Polygon * closed = new Gem_Polygon(points, VerticesCount);
		free(points);
		closed->BBox.x = MinX;
		closed->BBox.y = MinY;
		closed->BBox.w = MaxX-MinX;
		closed->BBox.h = MaxY-MinY;
		tm->AddDoor(Name, DoorClosed, DoorTiles, DoorTileCount, open, closed);
	}*/
	core->FreeInterface(tis);
	return tm;
}

unsigned short * WEDImp::GetDoorIndices(char * ResRef, int *count)
{
	unsigned short DoorClosed, DoorTileStart, DoorTileCount, *DoorTiles;
	unsigned short OpenPolyCount, ClosedPolyCount;
	unsigned long OpenPolyOffset, ClosedPolyOffset;
	char Name[9];
	int i;
	for(i = 0; i < DoorsCount; i++) {
		str->Seek(DoorsOffset + (i*0x1A), GEM_STREAM_START);
		str->Read(Name, 8);
		Name[8] = 0;
		if(strnicmp(Name, ResRef, 8)  == 0)
			break;
	}
	//The door has no representation in the WED file
	if(i==DoorsCount) {
		*count=0;
		printf("Found door without WED entry!\n");
		return NULL;
	}
		
	str->Read(&DoorClosed, 2);
	str->Read(&DoorTileStart, 2);
	str->Read(&DoorTileCount, 2);
	str->Read(&OpenPolyCount, 2);
	str->Read(&ClosedPolyCount, 2);
	str->Read(&OpenPolyOffset, 4);
	str->Read(&ClosedPolyOffset, 4);
	//Reading Door Tile Cells
	str->Seek(DoorTilesOffset + (DoorTileStart*2), GEM_STREAM_START);
	DoorTiles = (unsigned short*)malloc(DoorTileCount*sizeof(unsigned short));
	memset(DoorTiles, 0, DoorTileCount*sizeof(unsigned short));
	str->Read(DoorTiles, DoorTileCount*sizeof(unsigned short));
	*count = DoorTileCount;
	return DoorTiles;
}
