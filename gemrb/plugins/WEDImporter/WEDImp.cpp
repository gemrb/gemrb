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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/WEDImporter/WEDImp.cpp,v 1.14 2004/09/13 16:53:16 avenger_teambg Exp $
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
	if (str && autoFree) {
		delete( str );
	}
}

bool WEDImp::Open(DataStream* stream, bool autoFree)
{
	if (stream == NULL) {
		return false;
	}
	if (str && this->autoFree) {
		delete( str );
	}
	str = stream;
	this->autoFree = autoFree;
	char Signature[8];
	str->Read( Signature, 8 );
	if (strncmp( Signature, "WED V1.3", 8 ) != 0) {
		printf( "[WEDImporter]: This file is not a valid WED File\n" );
		return false;
	}
	str->ReadDword( &OverlaysCount );
	str->ReadDword( &DoorsCount );
	str->ReadDword( &OverlaysOffset );
	str->ReadDword( &SecHeaderOffset );
	str->ReadDword( &DoorsOffset );
	str->ReadDword( &DoorTilesOffset );
	str->Seek( OverlaysOffset, GEM_STREAM_START );
	for (unsigned int i = 0; i < OverlaysCount; i++) {
		Overlay o;
		str->ReadWord( &o.Width );
		str->ReadWord( &o.Height );
		str->Read( o.TilesetResRef, 8 );
		str->ReadDword( &o.unknown );
		str->ReadDword( &o.TilemapOffset );
		str->ReadDword( &o.TILOffset );
		overlays.push_back( o );
	}
	//Reading the Secondary Header
	str->Seek( SecHeaderOffset, GEM_STREAM_START );
	str->ReadDword( &WallPolygonsCount );
	str->ReadDword( &PolygonsOffset );
	str->ReadDword( &VerticesOffset );
	str->ReadDword( &WallGroupsOffset );
	str->ReadDword( &PILTOffset );
	return true;
}

TileMap* WEDImp::GetTileMap()
{
	TileMap* tm = new TileMap();
	//TODO: Implement Multi Overlay
	TileOverlay* over = new TileOverlay( overlays[0].Width,
								overlays[0].Height );
	DataStream* tisfile = core->GetResourceMgr()->GetResource( overlays[0].TilesetResRef,
													IE_TIS_CLASS_ID );
	if (!core->IsAvailable( IE_TIS_CLASS_ID )) {
		printf( "[WEDImporter]: No TileSet Importer Available.\n" );
		return NULL;
	}
	TileSetMgr* tis = ( TileSetMgr* ) core->GetInterface( IE_TIS_CLASS_ID );
	tis->Open( tisfile );
	for (int y = 0; y < overlays[0].Height; y++) {
		for (int x = 0; x < overlays[0].Width; x++) {
			str->Seek( overlays[0].TilemapOffset +
					( y * overlays[0].Width * 10 ) +
					( x * 10 ),
					GEM_STREAM_START );
			ieWord startindex, count, secondary;
			ieByte overlaymask;
			str->ReadWord( &startindex );
			str->ReadWord( &count );
			str->ReadWord( &secondary );
			//now we are not sure if this is a real byte
			//could be a dword, so endian problems may happen!!!
			str->Read( &overlaymask, 1 );
			//TODO: Consider Alternative Tile and Overlay Mask
			str->Seek( overlays[0].TILOffset + ( startindex * 2 ),
					GEM_STREAM_START );
			ieWord* indexes = ( ieWord* ) calloc( count, sizeof(ieWord) );
			str->Read( indexes, count * sizeof(ieWord) );
			if( DataStream::IsEndianSwitch()) {
				swab( (char*) indexes, (char*) indexes, count * sizeof(ieWord) );
			}
			Tile* tile;
			if (secondary == 0xffff)
				tile = tis->GetTile( indexes, count );
			else {
				tile = tis->GetTile( indexes, 1, &secondary );
			}
			tile->om = overlaymask;
			over->AddTile( tile );
			free( indexes );
		}
	}
	tm->AddOverlay( over );
	//Clipping Polygons
	/*
	for(int d = 0; d < DoorsCount; d++) {
		str->Seek(DoorsOffset + (d*0x1A), GEM_STREAM_START);
		ieWord DoorClosed, DoorTileStart, DoorTileCount, *DoorTiles;
		ieWord OpenPolyCount, ClosedPolyCount;
		ieDword OpenPolyOffset, ClosedPolyOffset;
		char Name[9];
		str->Read(Name, 8);
		Name[8] = 0;
		str->ReadWord( &DoorClosed );
		str->ReadWord( &DoorTileStart );
		str->ReadWord( &DoorTileCount );
		str->ReadWord( &OpenPolyCount );
		str->ReadWord( &ClosedPolyCount );
		str->ReadDword( &OpenPolyOffset );
		str->ReadDword( &ClosedPolyOffset );
		//Reading Door Tile Cells
		str->Seek(DoorTilesOffset + (DoorTileStart*2), GEM_STREAM_START);
		DoorTiles = (ieWord*)calloc(DoorTileCount,sizeof(ieWord));
		str->Read(DoorTiles, DoorTileCount*sizeof(ieWord));
		if( DataStream::IsEndianSwitch()) {
			swab( (char*) DoorTiles, (char*) DoorTiles, DoorTileCount * sizeof( ieWord) );
		}
		//Reading the Open Polygon
		str->Seek(OpenPolyOffset, GEM_STREAM_START);
		ieDword StartingVertex, VerticesCount;
		ieWordt BitFlag, MinX, MaxX, MinY, MaxY;
		Region BBox;
		str->ReadDword( &StartingVertex );
		str->ReadDword( &VerticesCount );
		str->ReadWord( &BitFlag );
		str->ReadWord( &MinX );
		str->ReadWord( &MaxX );
		str->ReadWord( &MinY );
		str->ReadWord( &MaxY );
		BBox.x = minX;
		BBox.y = minY;
		BBox.w = maxX - minX;
		BBox.h = maxY - minY;

		//Reading Vertices
		str->Seek(VerticesOffset + (StartingVertex*4), GEM_STREAM_START);
		Point * points = (Point*)malloc(VerticesCount*sizeof(Point));
		for(int i = 0; i < VerticesCount; i++) {
			str->ReadWord( &points[i].x );
			str->ReadWord( &points[i].y );
		}
		Gem_Polygon * open = new Gem_Polygon(points, VerticesCount, BBox);
		free(points);
		//Reading the closed Polygon
		str->Seek(ClosedPolyOffset, GEM_STREAM_START);
		str->ReadDword( &StartingVertex );
		str->ReadDword( &VerticesCount );
		str->ReadWord( &BitFlag );
		str->ReadWord( &MinX );
		str->ReadWord( &MaxX );
		str->ReadWord( &MinY );
		str->ReadWord( &MaxY );
		BBox.x = minX;
		BBox.y = minY;
		BBox.w = maxX - minX;
		BBox.h = maxY - minY;

		//Reading Vertices
		str->Seek(VerticesOffset + (StartingVertex*4), GEM_STREAM_START);
		points = (Point*)malloc(VerticesCount*sizeof(Point));
		for(int i = 0; i < VerticesCount; i++) {
			str->ReadWord( &points[i].x );
			str->ReadWord( &points[i].y );
		}
		Gem_Polygon * closed = new Gem_Polygon(points, VerticesCount, BBox);
		free(points);
		tm->AddDoor(Name, DoorClosed, DoorTiles, DoorTileCount, open, closed);
	}*/
	core->FreeInterface( tis );
	return tm;
}

ieWord* WEDImp::GetDoorIndices(char* ResRef, int* count, bool& BaseClosed)
{
	ieWord DoorClosed, DoorTileStart, DoorTileCount, * DoorTiles;
	ieWord OpenPolyCount, ClosedPolyCount;
	ieDword OpenPolyOffset, ClosedPolyOffset;
	char Name[9];
	unsigned int i;
	for (i = 0; i < DoorsCount; i++) {
		str->Seek( DoorsOffset + ( i * 0x1A ), GEM_STREAM_START );
		str->Read( Name, 8 );
		Name[8] = 0;
		if (strnicmp( Name, ResRef, 8 ) == 0)
			break;
	}
	//The door has no representation in the WED file
	if (i == DoorsCount) {
		*count = 0;
		printf( "Found door without WED entry!\n" );
		return NULL;
	}

	str->ReadWord( &DoorClosed );
	str->ReadWord( &DoorTileStart );
	str->ReadWord( &DoorTileCount );
	str->ReadWord( &OpenPolyCount );
	str->ReadWord( &ClosedPolyCount );
	str->ReadDword( &OpenPolyOffset );
	str->ReadDword( &ClosedPolyOffset );
	//Reading Door Tile Cells
	str->Seek( DoorTilesOffset + ( DoorTileStart * 2 ), GEM_STREAM_START );
	DoorTiles = ( ieWord* ) calloc( DoorTileCount, sizeof( ieWord) );
	str->Read( DoorTiles, DoorTileCount * sizeof( ieWord ) );
	if( DataStream::IsEndianSwitch()) {
		swab( (char*) DoorTiles, (char*) DoorTiles, DoorTileCount * sizeof( ieWord) );
	}
	*count = DoorTileCount;
	if (DoorClosed) {
		BaseClosed = true;
	} else {
		BaseClosed = false;
	}
	return DoorTiles;
}

