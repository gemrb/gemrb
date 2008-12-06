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
 * $Id$
 *
 */

#include "../../includes/win32def.h"
#include "WEDImp.h"
#include "../Core/TileSetMgr.h"
#include "../Core/Interface.h"
#include "../Core/ResourceMgr.h"

struct wed_polygon {
	ieDword FirstVertex;
	ieDword CountVertex;
	ieWord Flags;
	ieWord MinX, MaxX, MinY, MaxY;
};

//the net sizeof(wed_polygon) is 0x12 but not all compilers know that
#define WED_POLYGON_SIZE  0x12

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
	DoorPolygonsCount = 0;
	str->ReadDword( &PolygonsOffset );
	str->ReadDword( &VerticesOffset );
	str->ReadDword( &WallGroupsOffset );
	str->ReadDword( &PILTOffset );
	return true;
}

int WEDImp::AddOverlay(TileMap *tm, Overlay *overlays, bool rain)
{
	ieResRef res;
	int usedoverlays = 0;

	memcpy(res, overlays->TilesetResRef,sizeof(ieResRef));
	if (rain) {
		strncat(res,"R",8);
		//no rain tileset available, rolling back
		if (!core->Exists(res,IE_TIS_CLASS_ID)) {
			memcpy(res, overlays->TilesetResRef,sizeof(ieResRef));
		}
	}
	TileOverlay *over = new TileOverlay( overlays->Width, overlays->Height );
	DataStream* tisfile = core->GetResourceMgr()->GetResource(res, IE_TIS_CLASS_ID);
	TileSetMgr* tis = ( TileSetMgr* ) core->GetInterface( IE_TIS_CLASS_ID );
	tis->Open( tisfile );
	for (int y = 0; y < overlays->Height; y++) {
		for (int x = 0; x < overlays->Width; x++) {
			str->Seek( overlays->TilemapOffset +
				( y * overlays->Width * 10 ) + ( x * 10 ),
				GEM_STREAM_START );
			ieWord startindex, count, secondary;
			ieByte overlaymask;
			str->ReadWord( &startindex );
			str->ReadWord( &count );
			//should be always 0xffff
			str->ReadWord( &secondary );
			//should be always 0
			str->Read( &overlaymask, 1 );
			str->Seek( overlays->TILOffset + ( startindex * 2 ),
				GEM_STREAM_START );
			ieWord* indices = ( ieWord* ) calloc( count, sizeof(ieWord) );
			str->Read( indices, count * sizeof(ieWord) );
			if( DataStream::IsEndianSwitch()) {
				swab( (char*) indices, (char*) indices, count * sizeof(ieWord) );
			}
			Tile* tile;
			if (secondary == 0xffff) {
				tile = tis->GetTile( indices, count );
			} else {
				tile = tis->GetTile( indices, 1, &secondary );
			}
			tile->om = overlaymask;
			usedoverlays |= overlaymask;
			over->AddTile( tile );
			free( indices );
		}
	}
	
	if (rain) {
		tm->AddRainOverlay( over );
	} else {
		tm->AddOverlay( over );
	}
	core->FreeInterface( tis );
	return usedoverlays;
}

//this will replace the tileset of an existing tilemap, or create a new one
TileMap* WEDImp::GetTileMap(TileMap *tm)
{
	int usedoverlays;

	if (!core->IsAvailable( IE_TIS_CLASS_ID )) {
		printf( "[WEDImporter]: No TileSet Importer Available.\n" );
		return NULL;
	}
	if (!overlays.size()) {
		return NULL;
	}

	if (!tm) {
		tm = new TileMap();
	}

	usedoverlays = AddOverlay(tm, &overlays.at(0), false);
	AddOverlay(tm, &overlays.at(0), true);

	//reading additional overlays
	int mask=2;
	for(ieDword i=1;i<OverlaysCount;i++) {
		//skipping unused overlays
		if (!(mask&usedoverlays)) {
			tm->AddOverlay( NULL );
			tm->AddRainOverlay( NULL );
			mask<<=1;
			continue;
		}
		mask<<=1;

		AddOverlay(tm, &overlays.at(i), false);
		AddOverlay(tm, &overlays.at(i), true);
	}
	return tm;
}

void WEDImp::GetDoorPolygonCount(ieWord count, ieDword offset)
{
	ieDword basecount = offset-PolygonsOffset;
	if (basecount%WED_POLYGON_SIZE) {
		basecount+=WED_POLYGON_SIZE;
		printf("[WEDImporter]: Found broken door polygon header!\n");
	}
	ieDword polycount = basecount/WED_POLYGON_SIZE+count-WallPolygonsCount;
	if (polycount>DoorPolygonsCount) {
		DoorPolygonsCount=polycount;
	}
}

void WEDImp::SetupClosedDoor(unsigned int &index, unsigned int &count)
{
	index = (ClosedPolyOffset-PolygonsOffset)/WED_POLYGON_SIZE;
	count = ClosedPolyCount;
}

void WEDImp::SetupOpenDoor(unsigned int &index, unsigned int &count)
{
	index = (OpenPolyOffset-PolygonsOffset)/WED_POLYGON_SIZE;
	count = OpenPolyCount;
}

ieWord* WEDImp::GetDoorIndices(char* ResRef, int* count, bool& BaseClosed)
{
	ieWord DoorClosed, DoorTileStart, DoorTileCount, * DoorTiles;
//	ieWord OpenPolyCount, ClosedPolyCount;
//	ieDword OpenPolyOffset, ClosedPolyOffset;
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
		printf( "[WEDImporter]: Found door without WED entry!\n" );
		return NULL;
	}

	str->ReadWord( &DoorClosed );
	str->ReadWord( &DoorTileStart );
	str->ReadWord( &DoorTileCount );
	str->ReadWord( &OpenPolyCount );
	str->ReadWord( &ClosedPolyCount );
	str->ReadDword( &OpenPolyOffset );
	str->ReadDword( &ClosedPolyOffset );

	GetDoorPolygonCount(OpenPolyCount, OpenPolyOffset);
	GetDoorPolygonCount(ClosedPolyCount, ClosedPolyOffset);

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

Wall_Polygon **WEDImp::GetWallGroups()
{
	ieDword polygonCount = WallPolygonsCount+DoorPolygonsCount;

	Wall_Polygon **Polygons = (Wall_Polygon **) calloc( polygonCount, sizeof(Wall_Polygon *) );

	wed_polygon *PolygonHeaders = new wed_polygon[polygonCount];

	str->Seek (PolygonsOffset, GEM_STREAM_START);
	
	ieDword i; //msvc6.0 isn't ISO compatible, so this variable cannot be declared in 'for'
	for (i=0;i<polygonCount;i++) {
		str->ReadDword ( &PolygonHeaders[i].FirstVertex);
		str->ReadDword ( &PolygonHeaders[i].CountVertex);
		str->ReadWord ( &PolygonHeaders[i].Flags);
		str->ReadWord ( &PolygonHeaders[i].MinX);
		str->ReadWord ( &PolygonHeaders[i].MaxX);
		str->ReadWord ( &PolygonHeaders[i].MinY);
		str->ReadWord ( &PolygonHeaders[i].MaxY);
	}

	for (i=0;i<polygonCount;i++) {
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
		str->Read (points, count * sizeof (Point) );
		if( DataStream::IsEndianSwitch()) {
			swab( (char*) points, (char*) points, count * sizeof (Point) );
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
		delete [] points;
		if (flags&WF_BASELINE) {
			Polygons[i]->SetBaseline(base0, base1);
		}
		Polygons[i]->SetPolygonFlag(flags);
	}
	delete [] PolygonHeaders;

	return Polygons;
}

