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

#ifdef ANDROID
#include "swab.h"
#endif

#include "WEDImporter.h"

#include "win32def.h"

#include "GameData.h"
#include "Interface.h"
#include "PluginMgr.h"
#include "TileSetMgr.h"

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

using namespace GemRB;

struct wed_polygon {
	ieDword FirstVertex;
	ieDword CountVertex;
	ieWord Flags;
	ieWord MinX, MaxX, MinY, MaxY;
};

//the net sizeof(wed_polygon) is 0x12 but not all compilers know that
#define WED_POLYGON_SIZE  0x12

WEDImporter::WEDImporter(void)
{
	str = NULL;
}

WEDImporter::~WEDImporter(void)
{
	delete str;
}

bool WEDImporter::Open(DataStream* stream)
{
	if (stream == NULL) {
		return false;
	}
	delete str;
	str = stream;
	char Signature[8];
	str->Read( Signature, 8 );
	if (strncmp( Signature, "WED V1.3", 8 ) != 0) {
		Log(ERROR, "WEDImporter", "This file is not a valid WED File");
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
	ExtendedNight = false;
	return true;
}

int WEDImporter::AddOverlay(TileMap *tm, Overlay *overlays, bool rain)
{
	ieResRef res;
	int usedoverlays = 0;

	memcpy(res, overlays->TilesetResRef, sizeof(ieResRef));
	int len = strlen(res);
	// in BG1 extended night WEDs alway reference the day TIS instead of the matching night TIS
	if (ExtendedNight && len == 6) {
		strcat(res, "N");
		if (!gamedata->Exists(res, IE_TIS_CLASS_ID)) {
			res[len] = '\0';
		} else {
			len++;
		}
	}
	if (rain && len < 8) {
		strcat(res, "R");
		//no rain tileset available, rolling back
		if (!gamedata->Exists(res, IE_TIS_CLASS_ID)) {
			res[len] = '\0';
		}
	}
	DataStream* tisfile = gamedata->GetResource(res, IE_TIS_CLASS_ID);
	if (!tisfile) {
		return -1;
	}
	PluginHolder<TileSetMgr> tis(IE_TIS_CLASS_ID);
	tis->Open( tisfile );
	TileOverlay *over = new TileOverlay( overlays->Width, overlays->Height );
	for (int y = 0; y < overlays->Height; y++) {
		for (int x = 0; x < overlays->Width; x++) {
			str->Seek( overlays->TilemapOffset +
				( y * overlays->Width + x) * 10,
				GEM_STREAM_START );
			ieWord startindex, count, secondary;
			ieByte overlaymask, animspeed;
			str->ReadWord( &startindex );
			str->ReadWord( &count );
			str->ReadWord( &secondary );
			str->Read( &overlaymask, 1 );
			str->Read( &animspeed, 1 );
			if (animspeed == 0) {
				animspeed = ANI_DEFAULT_FRAMERATE;
			}
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
				tile->anim[1]->fps = animspeed;
			}
			tile->anim[0]->fps = animspeed;
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
	return usedoverlays;
}

//this will replace the tileset of an existing tilemap, or create a new one
TileMap* WEDImporter::GetTileMap(TileMap *tm)
{
	int usedoverlays;
	bool freenew = false;

	if (!overlays.size()) {
		return NULL;
	}

	if (!tm) {
		tm = new TileMap();
		freenew = true;
	}

	usedoverlays = AddOverlay(tm, &overlays.at(0), false);
	if (usedoverlays == -1) {
		if (freenew) {
			delete tm;
		}
		return NULL;
	}
	// rain_overlays[0] is never used
	tm->AddRainOverlay( NULL );

	//reading additional overlays
	int mask=2;
	for(ieDword i=1;i<OverlaysCount;i++) {
		//skipping unused overlays
		if (!(mask&usedoverlays)) {
			tm->AddOverlay( NULL );
			tm->AddRainOverlay( NULL );
		} else {
			// XXX: should fix AddOverlay not to load an overlay twice if there's no rain version!!
			AddOverlay(tm, &overlays.at(i), false);
			AddOverlay(tm, &overlays.at(i), true);
		}
		mask<<=1;
	}
	return tm;
}

void WEDImporter::GetDoorPolygonCount(ieWord count, ieDword offset)
{
	ieDword basecount = offset-PolygonsOffset;
	if (basecount%WED_POLYGON_SIZE) {
		basecount+=WED_POLYGON_SIZE;
		Log(WARNING, "WEDImporter", "Found broken door polygon header!");
	}
	ieDword polycount = basecount/WED_POLYGON_SIZE+count-WallPolygonsCount;
	if (polycount>DoorPolygonsCount) {
		DoorPolygonsCount=polycount;
	}
}

void WEDImporter::SetupClosedDoor(unsigned int &index, unsigned int &count)
{
	index = (ClosedPolyOffset-PolygonsOffset)/WED_POLYGON_SIZE;
	count = ClosedPolyCount;
}

void WEDImporter::SetupOpenDoor(unsigned int &index, unsigned int &count)
{
	index = (OpenPolyOffset-PolygonsOffset)/WED_POLYGON_SIZE;
	count = OpenPolyCount;
}

ieWord* WEDImporter::GetDoorIndices(char* ResRef, int* count, bool& BaseClosed)
{
	ieWord DoorClosed, DoorTileStart, DoorTileCount, * DoorTiles;
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
		Log(ERROR, "WEDImporter", "Found door without WED entry!");
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
	BaseClosed = DoorClosed != 0;
	return DoorTiles;
}

Wall_Polygon **WEDImporter::GetWallGroups()
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


#include "plugindef.h"

GEMRB_PLUGIN(0x7486BE7, "WED File Importer")
PLUGIN_CLASS(IE_WED_CLASS_ID, WEDImporter)
END_PLUGIN()
