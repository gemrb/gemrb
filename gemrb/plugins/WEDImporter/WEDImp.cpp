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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/WEDImporter/WEDImp.cpp,v 1.16 2005/10/22 16:30:54 avenger_teambg Exp $
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
		str->ReadResRef( o.TilesetResRef );
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
	TileOverlay* over = new TileOverlay( overlays[0].Width, overlays[0].Height );
	DataStream* tisfile = core->GetResourceMgr()->GetResource( overlays[0].TilesetResRef, IE_TIS_CLASS_ID );
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
		ieResRef Name;
		str->ReadResRef( Name );
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
	ieResRef Name;
	unsigned int i;

	for (i = 0; i < DoorsCount; i++) {
		str->Seek( DoorsOffset + ( i * 0x1A ), GEM_STREAM_START );
		str->ReadResRef( Name );
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

typedef struct {
  ieDword FirstVertex;
  ieDword CountVertex;
  ieWord Flags;
  ieWord MinX, MaxX, MinY, MaxY;
} wed_polygon;

Wall_Polygon **WEDImp::GetWallGroups()
{
	Wall_Polygon **Polygons = (Wall_Polygon **) calloc( WallPolygonsCount, sizeof(Wall_Polygon *) );

	wed_polygon *PolygonHeaders = new wed_polygon[WallPolygonsCount];

	str->Seek (PolygonsOffset, GEM_STREAM_START);
	
	for (ieDword i=0;i<WallPolygonsCount;i++) {
		str->ReadDword ( &PolygonHeaders[i].FirstVertex);
		str->ReadDword ( &PolygonHeaders[i].CountVertex);
		str->ReadWord ( &PolygonHeaders[i].Flags);
		str->ReadWord ( &PolygonHeaders[i].MinX);
		str->ReadWord ( &PolygonHeaders[i].MaxX);
		str->ReadWord ( &PolygonHeaders[i].MinY);
		str->ReadWord ( &PolygonHeaders[i].MaxY);
	}

	for (ieDword i=0;i<WallPolygonsCount;i++) {
		str->Seek (PolygonHeaders[i].FirstVertex*4+VerticesOffset, GEM_STREAM_START);
		//compose polygon
		ieDword count = PolygonHeaders[i].CountVertex;
		if (count<3) {
			//danger, danger
			continue;
		}
		ieDword flags = PolygonHeaders[i].Flags&~(WF_BASELINE|WF_HOVER);
		Point base0, base1;
		if (PolygonHeaders[i].Flags&WF_HOVER) {
			count-=2;
			ieWord x,y;
			str->ReadWord (&x);
			str->ReadWord (&y);
			base0 = Point(x,y);
			str->ReadWord (&x);
			str->ReadWord (&y);
			base1 = Point(x,y);
			flags |= WF_BASELINE;
		}
		Point *points = new Point[count];
		str->Read (points, count);
		if( DataStream::IsEndianSwitch()) {
			swab( (char*) points, (char*) points, PolygonHeaders[i].CountVertex * 2 * sizeof(ieWord) );
		}

		if (!(flags&WF_BASELINE) ) {
			if (PolygonHeaders[i].Flags&WF_BASELINE) {
				base0 = points[0];
				base1 = points[1];
				flags |= WF_BASELINE;
			}
		}
		Region rgn;
		rgn.x = PolygonHeaders[i].MinX;
		rgn.y = PolygonHeaders[i].MinY;
		rgn.w = PolygonHeaders[i].MaxX - PolygonHeaders[i].MinX;
		rgn.h = PolygonHeaders[i].MaxY - PolygonHeaders[i].MinY;
		Polygons[i] = new Wall_Polygon(points, count, &rgn);
		if (flags&WF_BASELINE) {
			Polygons[i]->SetBaseline(base0, base1);
		}
		Polygons[i]->SetPolygonFlag(flags);
	}
	delete [] PolygonHeaders;

	return Polygons;
}

