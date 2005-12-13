/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/AREImporter/AREImp.cpp,v 1.142 2005/12/13 19:42:06 avenger_teambg Exp $
 *
 */

#include "../../includes/win32def.h"
#include "AREImp.h"
#include "../Core/TileMapMgr.h"
#include "../Core/AnimationMgr.h"
#include "../Core/Interface.h"
#include "../Core/ActorMgr.h"
#include "../Core/CachedFileStream.h"
#include "../Core/ImageMgr.h"
#include "../Core/Ambient.h"
#include "../Core/ResourceMgr.h"
#include "../Core/DataFileMgr.h"
#include "../Core/Game.h"


#define DEF_OPEN   0
#define DEF_CLOSE  1
#define DEF_HOPEN  2
#define DEF_HCLOSE 3

#define DEF_COUNT 4

#define DOOR_HIDDEN 128

//something non signed, non ascii
#define UNINITIALIZED_BYTE  0x11

static ieResRef Sounds[DEF_COUNT] = {
	{UNINITIALIZED_BYTE},
};

DataFileMgr *INInote = NULL;

//called from ~Interface
void AREImp::ReleaseMemory()
{
	if(INInote) {
		core->FreeInterface( INInote );
		INInote = NULL;
	}
}

void ReadAutonoteINI()
{
	INInote = ( DataFileMgr * )
		core->GetInterface( IE_INI_CLASS_ID );
	FileStream* fs = new FileStream();
	char tINInote[_MAX_PATH];
	PathJoin( tINInote, core->GamePath, "autonote.ini", NULL );
	ResolveFilePath( tINInote );
	fs->Open( tINInote, true );
	INInote->Open( fs, true );
}

AREImp::AREImp(void)
{
	autoFree = false;
	str = NULL;
	if (Sounds[0][0] == UNINITIALIZED_BYTE) {
		memset( Sounds, 0, sizeof( Sounds ) );
		int SoundTable = core->LoadTable( "defsound" );
		TableMgr* at = core->GetTable( SoundTable );
		if (at) {
			for (int i = 0; i < DEF_COUNT; i++) {
				strncpy( Sounds[i], at->QueryField( i, 0 ), 8 );
				if(Sounds[i][0]=='*') {
					Sounds[i][0]=0;
				}
			}
		}
		core->DelTable( SoundTable );
	}
}

AREImp::~AREImp(void)
{
	if (autoFree && str) {
		delete( str );
	}
	Sounds[0][0]=UNINITIALIZED_BYTE;
}

bool AREImp::Open(DataStream* stream, bool autoFree)
{
	if (stream == NULL) {
		return false;
	}
	if (this->autoFree && str) {
		delete( str );
	}
	str = stream;
	this->autoFree = autoFree;
	char Signature[8];
	str->Read( Signature, 8 );

	if (strncmp( Signature, "AREAV1.0", 8 ) != 0) {
		if (strncmp( Signature, "AREAV9.1", 8 ) != 0) {
			return false;
		} else {
			bigheader = 16;
		}
	} else {
		bigheader = 0;
	}
	//TEST VERSION: SKIPPING VALUES
	str->ReadResRef( WEDResRef );
	str->ReadDword( &LastSave );
	str->ReadDword( &AreaFlags );
	//skipping bg1 area connection fields
	str->Seek( 0x48, GEM_STREAM_START );
	str->ReadWord( &AreaType );
	str->ReadWord( &WRain );
	str->ReadWord( &WSnow );
	str->ReadWord( &WFog );
	str->ReadWord( &WLightning );
	str->ReadWord( &WUnknown );
	//bigheader gap is here
	str->Seek( 0x54 + bigheader, GEM_STREAM_START );
	str->ReadDword( &ActorOffset );
	str->ReadWord( &ActorCount );
	str->ReadWord( &InfoPointsCount );
	str->ReadDword( &InfoPointsOffset );
	str->ReadDword( &SpawnOffset );
	str->ReadDword( &SpawnCount );
	str->ReadDword( &EntrancesOffset );
	str->ReadDword( &EntrancesCount );
	str->ReadDword( &ContainersOffset );
	str->ReadWord( &ContainersCount );
	str->ReadWord( &ItemsCount );
	str->ReadDword( &ItemsOffset );
	str->ReadDword( &VerticesOffset );
	str->ReadWord( &VerticesCount );
	str->ReadWord( &AmbiCount );
	str->ReadDword( &AmbiOffset );
	str->ReadDword( &VariablesOffset );
	str->ReadDword( &VariablesCount );
	ieDword tmp;
	str->ReadDword( &tmp );
	str->ReadResRef( Script );
	str->ReadDword( &ExploredBitmapSize );
	str->ReadDword( &ExploredBitmapOffset );
	str->ReadDword( &DoorsCount );
	str->ReadDword( &DoorsOffset );
	str->ReadDword( &AnimCount );
	str->ReadDword( &AnimOffset );
	str->ReadDword( &TileCount );
	str->ReadDword( &TileOffset );
	str->ReadDword( &SongHeader );
	str->ReadDword( &RestHeader );
	if (core->HasFeature(GF_AUTOMAP_INI) ) {
		str->ReadDword( &tmp ); //skipping unknown in PST
	}
	str->ReadDword( &NoteOffset );
	str->ReadDword( &NoteCount );
	return true;
}

Map* AREImp::GetMap(const char *ResRef)
{
	unsigned int i,x;

	Map* map = new Map();
	if(!map) {
		printf("Can't allocate map (out of memory).\n");
		abort();
	}
	if (core->SaveAsOriginal) {
		map->version = bigheader;
	}

	map->AreaFlags=AreaFlags;
	map->Rain=WRain;
	map->Snow=WSnow;
	map->Fog=WFog;
	map->Lightning=WLightning;
	map->AreaType=AreaType;

	//we have to set this here because the actors will receive their
	//current area setting here, areas' 'scriptname' is their name
	map->SetScriptName( ResRef );

	if (!core->IsAvailable( IE_WED_CLASS_ID )) {
		printf( "[AREImporter]: No Tile Map Manager Available.\n" );
		return false;
	}
	TileMapMgr* tmm = ( TileMapMgr* ) core->GetInterface( IE_WED_CLASS_ID );
	DataStream* wedfile = core->GetResourceMgr()->GetResource( WEDResRef, IE_WED_CLASS_ID );
	tmm->Open( wedfile );
	TileMap* tm = tmm->GetTileMap();
	if (!tm) {
		printf( "[AREImporter]: No Tile Map Available.\n" );
		core->FreeInterface( tmm );
		return false;
	}

	if (Script[0]) {
		map->Scripts[0] = new GameScript( Script, ST_AREA );
	} else {
		map->Scripts[0] = NULL;
	}
	if (map->Scripts[0]) {
		map->Scripts[0]->MySelf = map;
	}

	ieResRef TmpResRef;
	snprintf( TmpResRef, 9, "%sLM", WEDResRef);

	ImageMgr* lm = ( ImageMgr* ) core->GetInterface( IE_BMP_CLASS_ID );
	DataStream* lmstr = core->GetResourceMgr()->GetResource( TmpResRef, IE_BMP_CLASS_ID );
	lm->Open( lmstr, true );

	snprintf( TmpResRef, 9, "%sSR", WEDResRef);

	ImageMgr* sr = ( ImageMgr* ) core->GetInterface( IE_BMP_CLASS_ID );
	DataStream* srstr = core->GetResourceMgr()->GetResource( TmpResRef, IE_BMP_CLASS_ID );
	sr->Open( srstr, true );

	// Small map for MapControl
	ImageMgr* sm = ( ImageMgr* ) core->GetInterface( IE_MOS_CLASS_ID );
	DataStream* smstr = core->GetResourceMgr()->GetResource( WEDResRef, IE_MOS_CLASS_ID );
	sm->Open( smstr, true );

	map->AddTileMap( tm, lm, sr, sm );
	strnuprcpy( map->WEDResRef, WEDResRef, 8);

	str->Seek( SongHeader, GEM_STREAM_START );
	//5 is the number of song indices
	for (i = 0; i < MAX_RESCOUNT; i++) {
		str->ReadDword( map->SongHeader.SongList + i );
	}
	str->Seek( RestHeader, GEM_STREAM_START );
	for (i = 0; i < MAX_RESCOUNT; i++) {
		str->ReadDword( map->RestHeader.Strref + i );
	}
	for (i = 0; i < MAX_RESCOUNT; i++) {
		str->ReadResRef( map->RestHeader.CreResRef[i] );
	}
	str->ReadWord( &map->RestHeader.CreatureNum );
	if( map->RestHeader.CreatureNum>MAX_RESCOUNT ) {
		map->RestHeader.CreatureNum = MAX_RESCOUNT;
	}
	str->Seek( 14, GEM_CURRENT_POS );
	str->ReadWord( &map->RestHeader.DayChance );
	str->ReadWord( &map->RestHeader.NightChance );

	printf( "Loading regions\n" );
	//Loading InfoPoints
	for (i = 0; i < InfoPointsCount; i++) {
		str->Seek( InfoPointsOffset + ( i * 0xC4 ), GEM_STREAM_START );
		ieWord Type, VertexCount;
		ieDword FirstVertex, Cursor, Flags;
		ieWord TrapDetDiff, TrapRemDiff, Trapped, TrapDetected;
		ieWord LaunchX, LaunchY;
		char Name[33], Entrance[33];
		ieResRef Script, DialogResRef, KeyResRef, Destination;
		str->Read( Name, 32 );
		Name[32] = 0;
		str->ReadWord( &Type );
		Region bbox;
		ieWord tmp;
		str->ReadWord( &tmp );
		bbox.x = tmp;
		str->ReadWord( &tmp );
		bbox.y = tmp;
		str->ReadWord( &tmp );
		bbox.w = tmp - bbox.x;
		str->ReadWord( &tmp );
		bbox.h = tmp - bbox.y;
		str->ReadWord( &VertexCount );
		str->ReadDword( &FirstVertex );
		ieDword tmp2;
		str->ReadDword( &tmp2 );
		str->ReadDword( &Cursor );
		str->ReadResRef( Destination );
		str->Read( Entrance, 32 );
		Entrance[32] = 0;
		str->ReadDword( &Flags );
		ieStrRef StrRef;
		str->ReadDword( &StrRef );
		str->ReadWord( &TrapDetDiff );
		str->ReadWord( &TrapRemDiff );
		str->ReadWord( &Trapped );
		str->ReadWord( &TrapDetected );
		str->ReadWord( &LaunchX );
		str->ReadWord( &LaunchY );
		str->ReadResRef( KeyResRef );
		str->ReadResRef( Script );
			//maybe we have to store this
		str->Seek( 56, GEM_CURRENT_POS );
		str->ReadResRef( DialogResRef );
		char* string = core->GetString( StrRef );
		str->Seek( VerticesOffset + ( FirstVertex * 4 ), GEM_STREAM_START );
		Point* points = ( Point* ) malloc( VertexCount*sizeof( Point ) );
		for (x = 0; x < VertexCount; x++) {
			str->ReadWord( (ieWord*) &points[x].x );
			str->ReadWord( (ieWord*) &points[x].y );
		}
		Gem_Polygon* poly = new Gem_Polygon( points, VertexCount, &bbox);
		free( points );
		InfoPoint* ip = tm->AddInfoPoint( Name, Type, poly );
		ip->TrapDetectionDiff = TrapDetDiff;
		ip->TrapRemovalDiff = TrapRemDiff;
		ip->Trapped = Trapped;
		ip->TrapDetected = TrapDetected;
		ip->TrapLaunch.x = LaunchX;
		ip->TrapLaunch.y = LaunchY;
		ip->Cursor = Cursor;
		ip->overHeadText = string;
		ip->StrRef = StrRef; //we need this when saving area
		ip->textDisplaying = 0;
		ip->timeStartDisplaying = 0;
		ip->SetMap(map);
		ip->Pos.x = bbox.x + ( bbox.w / 2 );
		ip->Pos.y = bbox.y + ( bbox.h / 2 );
		ip->Flags = Flags;
		memcpy( ip->Destination, Destination, sizeof(Destination) );
		memcpy( ip->EntranceName, Entrance, sizeof(Entrance) );
		memcpy( ip->KeyResRef, KeyResRef, sizeof(KeyResRef) );
		memcpy( ip->DialogResRef, DialogResRef, sizeof(DialogResRef) );
		if (Script[0]) {
			ip->Scripts[0] = new GameScript( Script, ST_TRIGGER );
		} else {
			ip->Scripts[0] = NULL;
		}
		if (ip->Scripts[0]) {
			ip->Scripts[0]->MySelf = ip;
		}
	}

	printf( "Loading containers\n" );
	//Loading Containers
	for (i = 0; i < ContainersCount; i++) {
		str->Seek( ContainersOffset + ( i * 0xC0 ), GEM_STREAM_START );
		char Name[33];
		ieWord Type, LockDiff;
		ieDword Flags;
		ieWord TrapDetDiff, TrapRemDiff, Trapped, TrapDetected;
		ieWord XPos, YPos;
		ieWord LaunchX, LaunchY;
		ieDword ItemIndex, ItemCount;
		ieResRef KeyResRef;
		ieStrRef OpenFail;

		str->Read( Name, 32 );
		Name[32] = 0;
		str->ReadWord( &XPos );
		str->ReadWord( &YPos );
		str->ReadWord( &Type );
		str->ReadWord( &LockDiff );
		str->ReadDword( &Flags );
		str->ReadWord( &TrapDetDiff );
		str->ReadWord( &TrapRemDiff );
		str->ReadWord( &Trapped );
		str->ReadWord( &TrapDetected );
		str->ReadWord( &LaunchX );
		str->ReadWord( &LaunchY );
		Region bbox;
		ieWord tmp;
		str->ReadWord( &tmp );
		bbox.x = tmp;
		str->ReadWord( &tmp );
		bbox.y = tmp;
		str->ReadWord( &tmp );
		bbox.w = tmp - bbox.x;
		str->ReadWord( &tmp );
		bbox.h = tmp - bbox.y;
		str->ReadDword( &ItemIndex );
		str->ReadDword( &ItemCount );
		str->ReadResRef( Script );
		ieDword firstIndex, vertCount;
		str->ReadDword( &firstIndex );
		str->ReadDword( &vertCount );
		//str->Read( Name, 32 );
		str->Seek( 32, GEM_CURRENT_POS);
		str->ReadResRef( KeyResRef);
		str->Seek( 4, GEM_CURRENT_POS);
		str->ReadDword( &OpenFail );

		str->Seek( VerticesOffset + ( firstIndex * 4 ), GEM_STREAM_START );
		Point* points = ( Point* ) malloc( vertCount*sizeof( Point ) );
		for (unsigned int x = 0; x < vertCount; x++) {
			ieWord tmp;
			str->ReadWord( &tmp );
			points[x].x = tmp;
			str->ReadWord( &tmp );
			points[x].y = tmp;
		}
		Gem_Polygon* poly = new Gem_Polygon( points, vertCount, &bbox );
		free( points );
		Container* c = map->AddContainer( Name, Type, poly );
		//c->SetMap(map);
		c->Pos.x = XPos;
		c->Pos.y = YPos;
		c->LockDifficulty = LockDiff;
		c->Flags = Flags;
		c->TrapDetectionDiff = TrapDetDiff;
		c->TrapRemovalDiff = TrapRemDiff;
		c->Trapped = Trapped;
		c->TrapDetected = TrapDetected;
		c->TrapLaunch.x = LaunchX;
		c->TrapLaunch.y = LaunchY;
		//reading items into a container
		str->Seek( ItemsOffset+( ItemIndex * 0x14 ), GEM_STREAM_START);
		while(ItemCount--) {
			//cannot add directly to inventory (ground piles)
			c->AddItem( core->ReadItem(str));
		}
		if (Script[0]) {
			c->Scripts[0] = new GameScript( Script, ST_CONTAINER );
		} else {
			c->Scripts[0] = NULL;
		}
		if (c->Scripts[0]) {
			c->Scripts[0]->MySelf = c;
		}
		strnuprcpy(c->KeyResRef, KeyResRef, 8);
		c->OpenFail = OpenFail;
	}

	printf( "Loading doors\n" );
	//Loading Doors
	for (i = 0; i < DoorsCount; i++) {
		str->Seek( DoorsOffset + ( i * 0xc8 ), GEM_STREAM_START );
		int count;
		ieDword Flags;
		ieDword OpenFirstVertex, ClosedFirstVertex;
		ieDword OpenFirstImpeded, ClosedFirstImpeded;
		ieWord OpenVerticesCount, ClosedVerticesCount;
		ieWord OpenImpededCount, ClosedImpededCount;
		char LongName[33];
		char LinkedInfo[33];
		ieResRef ShortName;
		ieWord minX, maxX, minY, maxY;
		ieDword cursor;
		ieResRef KeyResRef, Script;
		ieWord TrapDetect, TrapRemoval;
		ieWord LaunchX, LaunchY;
		ieDword TrapFlags, Locked, LockRemoval;
		Region BBClosed, BBOpen;
		ieStrRef OpenStrRef;
		ieStrRef NameStrRef;
		ieResRef Dialog;

		str->Read( LongName, 32 );
		LongName[32] = 0;
		str->ReadResRef( ShortName );
		str->ReadDword( &Flags );
		str->ReadDword( &OpenFirstVertex );
		str->ReadWord( &OpenVerticesCount );
		str->ReadWord( &ClosedVerticesCount );
		str->ReadDword( &ClosedFirstVertex );
		str->ReadWord( &minX );
		str->ReadWord( &minY );
		str->ReadWord( &maxX );
		str->ReadWord( &maxY );
		BBOpen.x = minX;
		BBOpen.y = minY;
		BBOpen.w = maxX - minX;
		BBOpen.h = maxY - minY;
		str->ReadWord( &minX );
		str->ReadWord( &minY );
		str->ReadWord( &maxX );
		str->ReadWord( &maxY );
		BBClosed.x = minX;
		BBClosed.y = minY;
		BBClosed.w = maxX - minX;
		BBClosed.h = maxY - minY;
		str->ReadDword( &OpenFirstImpeded );
		str->ReadWord( &OpenImpededCount );
		str->ReadWord( &ClosedImpededCount );
		str->ReadDword( &ClosedFirstImpeded );
		str->Seek( 4, GEM_CURRENT_POS );
		ieResRef OpenResRef, CloseResRef;
		str->ReadResRef( OpenResRef );
		str->ReadResRef( CloseResRef );
		str->ReadDword( &cursor );
		str->ReadWord( &TrapDetect );
		str->ReadWord( &TrapRemoval );
		str->ReadDword( &TrapFlags );
		str->ReadWord( &LaunchX );
		str->ReadWord( &LaunchY );
		str->ReadResRef( KeyResRef );
		str->ReadResRef( Script );
		str->ReadDword( &Locked );
		str->ReadDword( &LockRemoval );
		Point toOpen[2];
		str->ReadWord( &minX );
		toOpen[0].x = minX;
		str->ReadWord( &minY );
		toOpen[0].y = minY;
		str->ReadWord( &maxX );
		toOpen[1].x = maxX;
		str->ReadWord( &maxY );
		toOpen[1].y = maxY;
		str->ReadDword( &OpenStrRef);
		str->Read( LinkedInfo, 32);
		str->ReadDword( &NameStrRef);
		str->ReadResRef( Dialog );

		//Reading Open Polygon
		str->Seek( VerticesOffset + ( OpenFirstVertex * 4 ), GEM_STREAM_START );
		Point* points = ( Point* )
			malloc( OpenVerticesCount*sizeof( Point ) );
		for (x = 0; x < OpenVerticesCount; x++) {
			str->ReadWord( &minX );
			points[x].x = minX;
			str->ReadWord( &minY );
			points[x].y = minY;
		}
		Gem_Polygon* open = new Gem_Polygon( points, OpenVerticesCount, &BBOpen );
		free( points );

		//Reading Closed Polygon
		str->Seek( VerticesOffset + ( ClosedFirstVertex * 4 ),
				GEM_STREAM_START );
		points = ( Point * ) malloc( ClosedVerticesCount * sizeof( Point ) );
		for (x = 0; x < ClosedVerticesCount; x++) {
			str->ReadWord( &minX );
			points[x].x = minX;
			str->ReadWord( &minY );
			points[x].y = minY;
		}
		Gem_Polygon* closed = new Gem_Polygon( points, ClosedVerticesCount, &BBClosed );
		free( points );

		//Getting Door Information from the WED File
		bool BaseClosed;
		unsigned short * indices = tmm->GetDoorIndices( ShortName, &count, BaseClosed );
		if (core->HasFeature(GF_REVERSE_DOOR)) {
			BaseClosed = !BaseClosed;
		}
		Door* door;
		door = tm->AddDoor( ShortName, LongName, Flags, BaseClosed,
					indices, count, open, closed );

		//Reading Open Impeded blocks
		str->Seek( VerticesOffset + ( OpenFirstImpeded * 4 ),
				GEM_STREAM_START );
		points = ( Point * ) malloc( OpenImpededCount * sizeof( Point ) );
		for (x = 0; x < OpenImpededCount; x++) {
			str->ReadWord( &minX );
			points[x].x = minX;
			str->ReadWord( &minY );
			points[x].y = minY;
		}
		door->open_ib = points;
		door->oibcount = OpenImpededCount;

		//Reading Closed Impeded blocks
		str->Seek( VerticesOffset + ( ClosedFirstImpeded * 4 ),
				GEM_STREAM_START );
		points = ( Point * ) malloc( ClosedImpededCount * sizeof( Point ) );
		for (x = 0; x < ClosedImpededCount; x++) {
			str->ReadWord( &minX );
			points[x].x = minX;
			str->ReadWord( &minY );
			points[x].y = minY;
		}
		door->closed_ib = points;
		door->cibcount = ClosedImpededCount;
		door->SetMap(map);
		door->SetDoorOpen(door->IsOpen(), false, 0);

		door->TrapDetectionDiff = TrapDetect;
		door->TrapRemovalDiff = TrapRemoval;
		door->TrapFlags = TrapFlags;
		door->TrapLaunch.x = LaunchX;
		door->TrapLaunch.y = LaunchY;

		door->Cursor = cursor;
		memcpy( door->KeyResRef, KeyResRef, sizeof(KeyResRef) );
		if (Script[0]) {
			door->Scripts[0] = new GameScript( Script, ST_DOOR );
		} else {
			door->Scripts[0] = NULL;
		}

		if (door->Scripts[0]) {
			door->Scripts[0]->MySelf = door;
		}

		door->toOpen[0] = toOpen[0];
		door->toOpen[1] = toOpen[1];
		//Leave the default sound untouched
		if (OpenResRef[0])
			memcpy( door->OpenSound, OpenResRef, sizeof(OpenResRef) );
		else {
			if (Flags & DOOR_HIDDEN)
				memcpy( door->OpenSound, Sounds[DEF_HOPEN], 9 );
			else
				memcpy( door->OpenSound, Sounds[DEF_OPEN], 9 );
		}
		if (CloseResRef[0])
			memcpy( door->CloseSound, CloseResRef, sizeof(CloseResRef) );
		else {
			if (Flags & DOOR_HIDDEN)
				memcpy( door->CloseSound, Sounds[DEF_HCLOSE], 9 );
			else
				memcpy( door->CloseSound, Sounds[DEF_CLOSE], 9 );
		}
		door->LockDifficulty=LockRemoval;
		door->OpenStrRef=OpenStrRef;
		strnspccpy(door->LinkedInfo, LinkedInfo, 32);
		//these 2 fields are not sure
		door->NameStrRef=NameStrRef;
		strnuprcpy(door->Dialog, Dialog, 8);
	}

	printf( "Loading spawnpoints\n" );
	//Loading SpawnPoints
	for (i = 0; i < SpawnCount; i++) {
		str->Seek( SpawnOffset + (i*0xc8), GEM_STREAM_START );
		char Name[33];
		ieWord XPos, YPos;
		ieWord Count, Difficulty, Flags;
		ieResRef creatures[MAX_RESCOUNT];
		ieWord DayChance, NightChance;
		ieDword Schedule;
		
		str->Read( Name, 32 );
		Name[32] = 0;
		str->ReadWord( &XPos );
		str->ReadWord( &YPos );
		for (unsigned int j = 0;j < MAX_RESCOUNT; j++) {
			str->ReadResRef( creatures[j] );
		}
		str->ReadWord( &Count);
		str->Seek( 14, GEM_CURRENT_POS); //skipping unknowns
		str->ReadWord( &Difficulty);
		str->ReadWord( &Flags);
		str->ReadDword( &Schedule);
		str->ReadWord( &DayChance);
		str->ReadWord( &NightChance);

		Spawn *sp = map->AddSpawn(Name, XPos, YPos, creatures, Count);
		sp->appearance = Schedule;
		sp->Difficulty = Difficulty;
		sp->Flags = Flags;
		sp->DayChance = DayChance;
		sp->NightChance = NightChance;
		//the rest is not read, we seek for every record
	}

	printf( "Loading actors\n" );
	//Loading Actors
	str->Seek( ActorOffset, GEM_STREAM_START );
	if (!core->IsAvailable( IE_CRE_CLASS_ID )) {
		printf( "[AREImporter]: No Actor Manager Available, skipping actors\n" );
	} else {
		ActorMgr* actmgr = ( ActorMgr* ) core->GetInterface( IE_CRE_CLASS_ID );
		for (i = 0; i < ActorCount; i++) {
			char DefaultName[33];
			ieResRef CreResRef;
			ieDword TalkCount;
			ieDword Orientation, Schedule;
			ieWord XPos, YPos, XDes, YDes;
			ieResRef Dialog;
			ieResRef Scripts[8]; //the original order
			ieDword Flags;
			str->Read( DefaultName, 32);
			DefaultName[32]=0;
			str->ReadWord( &XPos );
			str->ReadWord( &YPos );
			str->ReadWord( &XDes );
			str->ReadWord( &YDes );
			str->ReadDword( &Flags );
			str->Seek( 8, GEM_CURRENT_POS );
			str->ReadDword( &Orientation );
			str->Seek( 8, GEM_CURRENT_POS );
			str->ReadDword( &Schedule );
			str->ReadDword( &TalkCount );
			str->ReadResRef( Dialog );
			//TODO: script order			
			memset(Scripts,0,sizeof(Scripts));

			str->ReadResRef( Scripts[SCR_OVERRIDE] );
			str->ReadResRef( Scripts[SCR_CLASS] );
			str->ReadResRef( Scripts[SCR_RACE] );
			str->ReadResRef( Scripts[SCR_GENERAL] );
			str->ReadResRef( Scripts[SCR_DEFAULT] );
			str->ReadResRef( Scripts[SCR_SPECIFICS] );
			str->ReadResRef( CreResRef );
			DataStream* crefile;
			Actor *ab;
			ieDword CreOffset, CreSize;
			str->ReadDword( &CreOffset );
			str->ReadDword( &CreSize );
			//TODO: iwd2 script?
			str->ReadResRef( Scripts[SCR_AREA] );
			str->Seek( 120, GEM_CURRENT_POS );
			//actually, Flags&1 signs that the creature
			//is not loaded yet, so !(Flags&1) means it is embedded
			if (CreOffset != 0 && !(Flags&1) ) {
				CachedFileStream *fs = new CachedFileStream( (CachedFileStream *) str, CreOffset, CreSize, true);
				crefile = (DataStream *) fs;
			} else {
				crefile = core->GetResourceMgr()->GetResource( CreResRef, IE_CRE_CLASS_ID );
			}
			if(!actmgr->Open( crefile, true )) {
				printf("Couldn't read actor: %s!\n", CreResRef);
				continue;
			}
			ab = actmgr->GetActor();
			if(!ab)
				continue;
			map->AddActor(ab);
			ab->Pos.x = XPos;
			ab->Pos.y = YPos;
			ab->Destination.x = XDes;
			ab->Destination.y = YDes;
			//copying the scripting name into the actor
			//if the CreatureAreaFlag was set to 8
			if ((Flags&AF_NAME_OVERRIDE) || (core->HasFeature(GF_IWD2_SCRIPTNAME)) ) {
				ab->SetScriptName(DefaultName);
			}
	
			for (int j=0;j<8;j++) {
				if (Scripts[j][0]) {
					ab->SetScript(Scripts[j],j);
				}
			}
			ab->SetOrientation( Orientation,0 );
			ab->TalkCount = TalkCount;
			//maybe there is a flag (deactivate), but 
			//right now we just set this
			//it is automatically enabled, we should disable it
			//if required
			//ab->Active = SCR_VISIBLE;
		}
		core->FreeInterface( actmgr );
	}

	printf( "Loading animations\n" );
	//Loading Animations
	str->Seek( AnimOffset, GEM_STREAM_START );
	if (!core->IsAvailable( IE_BAM_CLASS_ID )) {
		printf( "[AREImporter]: No Animation Manager Available, skipping animations\n" );
	} else {
		for (i = 0; i < AnimCount; i++) {
			AreaAnimation* anim = new AreaAnimation();
			str->Read(anim->Name, 32);
			ieWord animX, animY;
			str->ReadWord( &animX );
			str->ReadWord( &animY );
			anim->Pos.x=animX;
			anim->Pos.y=animY;
			str->ReadDword( &anim->appearance );
			str->ReadResRef( anim->BAM );			
			str->ReadWord( &anim->sequence );
			str->ReadWord( &anim->frame );
			str->ReadDword( &anim->Flags );
			str->ReadWord( &anim->unknown38 );  //not completely understood, seems like a percentage or speed value
			str->ReadWord( &anim->transparency );
			str->ReadWord( &anim->unknown3c ); //not completely understood, if not 0, sequence is started
			str->Read( &anim->startchance,1 );
			if (anim->startchance<=0) {
				anim->startchance=100;      //percentage of starting a cycle
			}
			str->Read( &anim->skipcycle,1 ); //how many cycles are skipped	(100% skippage)	
			str->ReadResRef( anim->Palette );
			str->ReadDword( &anim->unknown48 );
			AnimationFactory* af = ( AnimationFactory* )
			core->GetResourceMgr()->GetFactoryResource( anim->BAM, IE_BAM_CLASS_ID );
			if (!af) {
				printf("Cannot load animation: %s\n", anim->BAM);
				continue;
			}
			if (anim->Flags & A_ANI_ALLCYCLES) {
				anim->animcount = af->GetCycleCount();
				anim->animation = (Animation **) malloc(anim->animcount * sizeof(Animation *) );
				for(int j=0;j<anim->animcount;j++) {
					anim->animation[j]=GetAnimationPiece(af, j, anim);
				}
			} else {
				anim->animcount = 1;
				anim->animation = (Animation **) malloc( sizeof(Animation *) );
				anim->animation[0]=GetAnimationPiece(af, anim->sequence, anim);
			}
			if (anim->Flags & A_ANI_PALETTE) {
				anim->SetPalette(anim->Palette);
			}

			map->AddAnimation( anim );
		}
	}

	printf( "Loading entrances\n" );
	//Loading Entrances
	str->Seek( EntrancesOffset, GEM_STREAM_START );
	for (i = 0; i < EntrancesCount; i++) {
		char Name[33];
		ieWord XPos, YPos, Face;
		str->Read( Name, 32 );
		Name[32] = 0;
		str->ReadWord( &XPos );
		str->ReadWord( &YPos );
		str->ReadWord( &Face );
		str->Seek( 66, GEM_CURRENT_POS );
		map->AddEntrance( Name, XPos, YPos, Face );
	}

	printf( "Loading variables\n" );
	//Loading Variables
	//map->vars=new Variables();
	//map->vars->SetType( GEM_VARIABLES_INT );

	str->Seek( VariablesOffset, GEM_STREAM_START );
	for (i = 0; i < VariablesCount; i++) {
		char Name[33];
		ieDword Value;
		str->Read( Name, 32 );
		Name[32] = 0;
		str->Seek( 8, GEM_CURRENT_POS );
		str->ReadDword( &Value );
		str->Seek( 40, GEM_CURRENT_POS );
		map->locals->SetAt( Name, Value );
	}
	
	printf( "Loading ambients\n" );
	str->Seek( AmbiOffset, GEM_STREAM_START );
	for (i = 0; i < AmbiCount; i++) {
		int j;
		ieResRef sounds[MAX_RESCOUNT];
		ieWord tmpWord;

		Ambient *ambi = new Ambient();
		str->Read( &ambi->name, 32 );
		str->ReadWord( &tmpWord );
		ambi->origin.x = tmpWord;
		str->ReadWord( &tmpWord );
		ambi->origin.y = tmpWord;
		str->ReadWord( &ambi->radius );
		str->ReadWord( &ambi->height );
		str->Seek( 6, GEM_CURRENT_POS );
		str->ReadWord( &ambi->gain );
		for (j = 0;j < MAX_RESCOUNT; j++) {
			str->ReadResRef( sounds[j] );
		}
		str->ReadWord( &tmpWord );
		str->Seek( 2, GEM_CURRENT_POS );
		str->ReadDword( &ambi->interval );
		str->ReadDword( &ambi->perset );
		// schedule bits
		str->ReadDword( &ambi->appearance );
		str->ReadDword( &ambi->flags );
		str->Seek( 64, GEM_CURRENT_POS );
		//this is a physical limit
		if (tmpWord>MAX_RESCOUNT) {
			tmpWord=MAX_RESCOUNT;
		}
		for (j = 0; j < tmpWord; j++) {
			char *sound = (char *) malloc(9);
			memcpy(sound, sounds[j], 9);
			ambi->sounds.push_back(sound);
		}
		map->AddAmbient(ambi);
	}

	printf( "Loading automap notes\n" );
	str->Seek( NoteOffset, GEM_STREAM_START );

	Point point;
	ieDword color;
	char *text;
	//Don't bother with autonote.ini if the area has autonotes (ie. it is a saved area)
	int pst = core->HasFeature( GF_AUTOMAP_INI );
	if (pst && !NoteCount) {
		if( !INInote ) {
			ReadAutonoteINI();
		}
		//add autonote.ini entries
		if( INInote ) {
			color = 1; //read only note
			int count = INInote->GetKeyAsInt( map->GetScriptName(), "count", 0);
			while (count) {
				char key[32];
				int value;
				sprintf(key, "text%d",count);
				value = INInote->GetKeyAsInt( map->GetScriptName(), key, 0);
				text = core->GetString(value);
				sprintf(key, "xPos%d",count);
				value = INInote->GetKeyAsInt( map->GetScriptName(), key, 0);
				point.x = value;
				sprintf(key, "yPos%d",count);
				value = INInote->GetKeyAsInt( map->GetScriptName(), key, 0);
				point.y = value;
				map->AddMapNote( point, color, text );
				count--;
			}
		}
	}
	for (i = 0; i < NoteCount; i++) {
		if (pst) {
			ieDword px,py;

			str->ReadDword(&px);
			str->ReadDword(&py);
			point.x=px;
			point.y=py;
			text = (char *) malloc( 500 );
			str->Read(text, 500 );
			text[499] = 0;
			str->ReadDword(&color); //readonly == 1
			str->Seek(20, GEM_CURRENT_POS);
			text = (char *) realloc( text, strlen(text) );
		}
		else {
			ieWord px,py;
			ieDword strref;

			str->ReadWord( &px );
			str->ReadWord( &py );
			point.x=px;
			point.y=py;
			str->ReadDword( &strref );
			str->ReadWord( &px );
			str->ReadWord( &py );
			color=py;
			str->Seek( 40, GEM_CURRENT_POS );
			text = core->GetString( strref,0 );
		}
		map->AddMapNote( point, color, text );
	}

	printf( "Loading tiles\n" );
	//Loading Tiled objects (if any)
	str->Seek( TileOffset, GEM_STREAM_START );
	for (i = 0; i < TileCount; i++) {
		char Name[33];
		ieResRef ID;
		ieDword Flags;
		ieDword OpenIndex, OpenCount;
		ieDword ClosedIndex, ClosedCount;
		str->Read( Name, 32 );
		Name[32] = 0;
		str->ReadResRef( ID );
		str->ReadDword( &Flags );
		str->ReadDword( &OpenIndex );
		str->ReadDword( &OpenCount );
		str->ReadDword( &ClosedIndex );
		str->ReadDword( &ClosedCount );
		str->Seek( 48, GEM_CURRENT_POS );
		//absolutely no idea where these 'tile indices' are stored
		//are they tileset tiles or impeded block tiles
		map->TMap->AddTile( ID, Name, Flags, NULL,0, NULL, 0 );
	}


	printf( "Loading explored bitmap\n" );
	i = map->GetExploredMapSize();
	if (ExploredBitmapSize==i) {
		map->ExploredBitmap = (ieByte *) malloc(ExploredBitmapSize);
		str->Seek( ExploredBitmapOffset, GEM_STREAM_START );
		str->Read( map->ExploredBitmap, ExploredBitmapSize );
	}
	else {
		if( ExploredBitmapSize ) {
			printMessage("AREImp", " ", LIGHT_RED);
			printf("ExploredBitmapSize in game: %d != %d. Clearing it\n", ExploredBitmapSize, i);
		}
		ExploredBitmapSize = i;
		map->ExploredBitmap = (ieByte *) calloc(i, 1);
	}
	map->VisibleBitmap = (ieByte *) calloc(i, 1);

	printf( "Loading wallgroups\n");
	map->SetWallGroups( tmm->GetWallPolygonsCount(),tmm->GetWallGroups() );

	core->FreeInterface( tmm );
	return map;
}

Animation *AREImp::GetAnimationPiece(AnimationFactory *af, int animCycle, AreaAnimation *a)
{
	Animation *anim = af->GetCycle( ( unsigned char ) animCycle );
	if (!anim)
		anim = af->GetCycle( 0 );
	if (!anim) {
		printf("Cannot load animation: %s\n", a->BAM);
		return NULL;
	}
	anim->pos = a->frame;
	anim->Flags = a->Flags;
	anim->x = a->Pos.x;
	anim->y = a->Pos.y;
	if (anim->Flags&A_ANI_MIRROR) {
		anim->MirrorAnimation();
	}
	if (anim->Flags&A_ANI_PALETTE) {
		Color *Palette = (Color *) malloc(sizeof(Color) * 256);
		ImageMgr *bmp = (ImageMgr *) core->GetInterface( IE_BMP_CLASS_ID);
		if (bmp) {
			DataStream* s = core->GetResourceMgr()->GetResource( a->Palette, IE_BMP_CLASS_ID );
			bmp->Open( s, true);
			bmp->GetPalette(0, 256, Palette);
			core->FreeInterface( bmp );
		}
		anim->SetPalette( Palette, true );
		free (Palette);
	}
	if (anim->Flags&A_ANI_BLEND) {
		anim->BlendAnimation();
	}
	return anim;
}

int AREImp::GetStoredFileSize(Map *map)
{
	unsigned int i;
	int headersize = map->version+0x11c;
	ActorOffset = headersize;

	//get only saved actors (no familiars or partymembers)
	//summons?
	ActorCount = (ieWord) map->GetActorCount(false);
	headersize += ActorCount * 0x110;

	ActorMgr* am = ( ActorMgr* ) core->GetInterface( IE_CRE_CLASS_ID );
	EmbeddedCreOffset = headersize;

	for (i=0;i<ActorCount;i++) {
		headersize += am->GetStoredFileSize(map->GetActor(i, false) );
	}
	core->FreeInterface(am);

	InfoPointsOffset = headersize;

	InfoPointsCount = (ieWord) map->TMap->GetInfoPointCount();
	headersize += InfoPointsCount * 0xc4;
	SpawnOffset = headersize;

	SpawnCount = (ieDword) map->GetSpawnCount();
	headersize += SpawnCount * 0xc8;
	EntrancesOffset = headersize;

	EntrancesCount = (ieDword) map->GetEntranceCount();
	headersize += EntrancesCount * 0x68;
	ContainersOffset = headersize;

	//this one removes empty heaps and counts items, should be before
	//getting ContainersCount
	ItemsCount = (ieDword) map->ConsolidateContainers();
	ContainersCount = (ieDword) map->TMap->GetContainerCount();
	headersize += ContainersCount * 0xc0;
	ItemsOffset = headersize;
	headersize += ItemsCount * 0x14;
	DoorsOffset = headersize;

	DoorsCount = (ieDword) map->TMap->GetDoorCount();
	headersize += DoorsCount * 0xc8;
	VerticesOffset = headersize;

	VerticesCount = 0;
	for(i=0;i<InfoPointsCount;i++) {
		InfoPoint *ip=map->TMap->GetInfoPoint(i);
		VerticesCount+=ip->outline->count;
	}
	for(i=0;i<ContainersCount;i++) {
		Container *c=map->TMap->GetContainer(i);
		VerticesCount+=c->outline->count;
	}
	for(i=0;i<DoorsCount;i++) {
		Door *d=map->TMap->GetDoor(i);
		VerticesCount+=d->open->count+d->closed->count+d->oibcount+d->cibcount;
	}
	headersize += VerticesCount * 4;
	AmbiOffset = headersize;

	AmbiCount = (ieDword) map->GetAmbientCount();
	headersize += AmbiCount * 0xd4;
	VariablesOffset = headersize;

	VariablesCount = (ieDword) map->locals->GetCount();
	headersize += VariablesCount * 0x54;
	AnimOffset = headersize;

	AnimCount = (ieDword) map->GetAnimationCount();
	headersize += AnimCount * 0x4c;
	TileOffset = headersize;

	TileCount = (ieDword) map->TMap->GetTileCount();
	headersize += TileCount * 0x6c;
	ExploredBitmapOffset = headersize;

	ExploredBitmapSize = map->GetExploredMapSize();
	headersize += ExploredBitmapSize;
	NoteOffset = headersize;

	int pst = core->HasFeature( GF_AUTOMAP_INI );
	NoteCount = (ieDword) map->GetMapNoteCount();
	headersize += NoteCount * (pst?0x214: 0x22);
	RestHeader = headersize;

	headersize += 0xe4;
	SongHeader = headersize;

	headersize += 0x90;
	return headersize;
}

int AREImp::PutHeader(DataStream *stream, Map *map)
{
	char Signature[80];
	ieDword tmpDword = 0;
	ieWord tmpWord = 0;
	int pst = core->HasFeature( GF_AUTOMAP_INI );

	memcpy( Signature, "AREAV1.0", 8);
	if (map->version==16) {
		Signature[5]='9';
		Signature[7]='1';
	}
	stream->Write( Signature, 8);
	stream->WriteResRef( map->WEDResRef);
	stream->WriteDword( &core->GetGame()->GameTime ); //lastsaved
	stream->WriteDword( &map->AreaFlags);
	
	memset(Signature, 0, sizeof(Signature)); //8 bytes 0
	stream->Write( Signature, 8); //northref
	stream->WriteDword( &tmpDword);
	stream->Write( Signature, 8); //westref
	stream->WriteDword( &tmpDword);
	stream->Write( Signature, 8); //southref
	stream->WriteDword( &tmpDword);
	stream->Write( Signature, 8); //eastref
	stream->WriteDword( &tmpDword);

	stream->WriteWord( &map->AreaType);
	stream->WriteWord( &map->Rain);
	stream->WriteWord( &map->Snow);
	stream->WriteWord( &map->Fog);
	stream->WriteWord( &map->Lightning);
	stream->WriteWord( &tmpWord);

	if (map->version==16) { //writing 16 bytes of 0's
		stream->Write( Signature, 8);
		stream->Write( Signature, 8);
	}

	stream->WriteDword( &ActorOffset);
	stream->WriteWord( &ActorCount);
	stream->WriteWord( &InfoPointsCount );
	stream->WriteDword( &InfoPointsOffset );
	stream->WriteDword( &SpawnOffset );
	stream->WriteDword( &SpawnCount );
	stream->WriteDword( &EntrancesOffset );
	stream->WriteDword( &EntrancesCount );
	stream->WriteDword( &ContainersOffset );
	stream->WriteWord( &ContainersCount );
	stream->WriteWord( &ItemsCount );
	stream->WriteDword( &ItemsOffset );
	stream->WriteDword( &VerticesOffset );
	stream->WriteWord( &VerticesCount );
	stream->WriteWord( &AmbiCount );
	stream->WriteDword( &AmbiOffset );
	stream->WriteDword( &VariablesOffset );
	stream->WriteDword( &VariablesCount );
	stream->WriteDword( &tmpDword);
	GameScript *s = map->Scripts[0];
	if (s) {
		stream->WriteResRef( s->GetName() );
	} else {
		stream->Write( Signature, 8);
	}
	stream->WriteDword( &ExploredBitmapSize);
	stream->WriteDword( &ExploredBitmapOffset);
	stream->WriteDword( &DoorsCount );
	stream->WriteDword( &DoorsOffset );
	stream->WriteDword( &AnimCount );
	stream->WriteDword( &AnimOffset );	
	stream->WriteDword( &TileCount);
	stream->WriteDword( &TileOffset);
	stream->WriteDword( &SongHeader);
	stream->WriteDword( &RestHeader);
	//an empty dword for pst
	int i;
	if (pst) {
		tmpDword = 0xffffffff;
		stream->WriteDword( &tmpDword);
		i=76;
	} else {
		i=80;
	}
	stream->WriteDword( &NoteOffset );
	stream->WriteDword( &NoteCount );
	//usually 80 empty bytes (but pst used up 4 elsewhere)
	stream->Write( Signature, i);
	return 0;
}

int AREImp::PutDoors( DataStream *stream, Map *map, ieDword &VertIndex)
{
	char filling[8];
	ieDword tmpDword = 0;
	ieWord tmpWord = 0;

	memset(filling,0,sizeof(filling) );
	for (unsigned int i=0;i<DoorsCount;i++) {
		Door *d = map->TMap->GetDoor(i);

		stream->Write( d->GetScriptName(), 32);
		stream->WriteResRef( d->ID);
		stream->WriteDword( &d->Flags);
		stream->WriteDword( &VertIndex);
		tmpWord = (ieWord) d->open->count;
		stream->WriteWord( &tmpWord);
		VertIndex += tmpWord;
		tmpWord = (ieWord) d->closed->count;
		stream->WriteWord( &tmpWord);
		stream->WriteDword( &VertIndex);
		VertIndex += tmpWord;
		//open bounding box
		tmpWord = (ieWord) d->open->BBox.x;
		stream->WriteWord( &tmpWord);
		tmpWord = (ieWord) d->open->BBox.y;
		stream->WriteWord( &tmpWord);
		tmpWord = (ieWord) (d->open->BBox.x+d->open->BBox.w);
		stream->WriteWord( &tmpWord);
		tmpWord = (ieWord) (d->open->BBox.y+d->open->BBox.h);
		stream->WriteWord( &tmpWord);
		//closed bounding box
		tmpWord = (ieWord) d->closed->BBox.x;
		stream->WriteWord( &tmpWord);
		tmpWord = (ieWord) d->closed->BBox.y;
		stream->WriteWord( &tmpWord);
		tmpWord = (ieWord) (d->closed->BBox.x+d->closed->BBox.w);
		stream->WriteWord( &tmpWord);
		tmpWord = (ieWord) (d->closed->BBox.y+d->closed->BBox.h);
		stream->WriteWord( &tmpWord);
		//open and closed impeded blocks
		stream->WriteDword( &VertIndex);
		tmpWord = (ieWord) d->oibcount;
		stream->WriteWord( &tmpWord);
		VertIndex += tmpWord;
		tmpWord = (ieWord) d->cibcount;
		stream->WriteWord( &tmpWord);
		stream->WriteDword( &VertIndex);
		VertIndex += tmpWord;
		//unknown54
		stream->WriteDword( &tmpDword);
		stream->WriteResRef( d->OpenSound);
		stream->WriteResRef( d->CloseSound);
		stream->WriteDword( &d->Cursor);
		stream->WriteWord( &d->TrapDetectionDiff);
		stream->WriteWord( &d->TrapRemovalDiff);
		stream->WriteDword( &d->TrapFlags);
		tmpWord = (ieWord) d->TrapLaunch.x;
		stream->WriteWord( &tmpWord);
		tmpWord = (ieWord) d->TrapLaunch.y;
		stream->WriteWord( &tmpWord);
		stream->WriteResRef( d->KeyResRef);
		GameScript *s = d->Scripts[0];
		if (s) {
			stream->WriteResRef( s->GetName() );
		} else {
			stream->Write( filling, 8);
		}
		//unknown field 0-100
		//stream->WriteDword( &d->Locked);
		stream->WriteDword( &tmpDword);
		//lock difficulty field
		stream->WriteDword( &d->LockDifficulty);
		//opening locations
		tmpWord = (ieWord) d->toOpen[0].x;
		stream->WriteWord( &tmpWord);
		tmpWord = (ieWord) d->toOpen[0].y;
		stream->WriteWord( &tmpWord);
		tmpWord = (ieWord) d->toOpen[1].x;
		stream->WriteWord( &tmpWord);
		tmpWord = (ieWord) d->toOpen[1].y;
		stream->WriteWord( &tmpWord);
		stream->WriteDword( &d->OpenStrRef);
		stream->Write( d->LinkedInfo, 32);
		stream->WriteDword( &d->NameStrRef);
		stream->WriteResRef( d->Dialog);
	}
	return 0;
}

int AREImp::PutPoints( DataStream *stream, Point *p, unsigned int count)
{
	ieWord tmpWord;
	unsigned int j;

	for(j=0;j<count;j++) {
		tmpWord = p[j].x;
		stream->WriteWord( &tmpWord);
		tmpWord = p[j].y;
		stream->WriteWord( &tmpWord);
	}
	return 0;
}

int AREImp::PutVertices( DataStream *stream, Map *map)
{
	unsigned int i;

	//regions
	for(i=0;i<InfoPointsCount;i++) {
		InfoPoint *ip = map->TMap->GetInfoPoint(i);
		PutPoints(stream, ip->outline->points, ip->outline->count);
	}
	//containers
	for(i=0;i<ContainersCount;i++) {
		Container *c = map->TMap->GetContainer(i);
		PutPoints(stream, c->outline->points, c->outline->count);
	}
	//doors
	for(i=0;i<DoorsCount;i++) {
		Door *d = map->TMap->GetDoor(i);
		PutPoints(stream, d->open->points, d->open->count);
		PutPoints(stream, d->closed->points, d->closed->count);
		PutPoints(stream, d->open_ib, d->oibcount);
		PutPoints(stream, d->closed_ib, d->cibcount);
	}
	return 0;
}

int AREImp::PutItems( DataStream *stream, Map *map)
{
	for (unsigned int i=0;i<ContainersCount;i++) {
		Container *c = map->TMap->GetContainer(i);

		for(int j=0;j<c->inventory.GetSlotCount();j++) {
			CREItem *ci = c->inventory.GetSlotItem(j);

			stream->WriteResRef( ci->ItemResRef);
			stream->WriteWord( &ci->PurchasedAmount);
			stream->WriteWord( &ci->Usages[0]);
			stream->WriteWord( &ci->Usages[1]);
			stream->WriteWord( &ci->Usages[2]);
			stream->WriteDword( &ci->Flags);
		}
	}
	return 0;
}

int AREImp::PutContainers( DataStream *stream, Map *map, ieDword &VertIndex)
{
	char filling[56];
	ieDword ItemIndex = 0;
	ieDword tmpDword;
	ieWord tmpWord;

	memset(filling,0,sizeof(filling) );
	for (unsigned int i=0;i<ContainersCount;i++) {
		Container *c = map->TMap->GetContainer(i);

		//this is the editor name
		stream->Write( c->GetScriptName(), 32);
		tmpWord = (ieWord) c->Pos.x;
		stream->WriteWord( &tmpWord);
		tmpWord = (ieWord) c->Pos.y;
		stream->WriteWord( &tmpWord);
		stream->WriteWord( &c->Type);
		stream->WriteWord( &c->LockDifficulty);
		stream->WriteDword( &c->Flags);
		stream->WriteWord( &c->TrapDetectionDiff);
		stream->WriteWord( &c->TrapRemovalDiff);
		stream->WriteWord( &c->Trapped);
		stream->WriteWord( &c->TrapDetected);
		tmpWord = (ieWord) c->TrapLaunch.x;
		stream->WriteWord( &tmpWord);
		tmpWord = (ieWord) c->TrapLaunch.y;
		stream->WriteWord( &tmpWord);
		//outline bounding box
		tmpWord = (ieWord) c->outline->BBox.x;
		stream->WriteWord( &tmpWord);
		tmpWord = (ieWord) c->outline->BBox.y;
		stream->WriteWord( &tmpWord);
		tmpWord = (ieWord) (c->outline->BBox.x + c->outline->BBox.w);
		stream->WriteWord( &tmpWord);
		tmpWord = (ieWord) (c->outline->BBox.y + c->outline->BBox.h);
		stream->WriteWord( &tmpWord);
		//item index and offset
		tmpDword = c->inventory.GetSlotCount();
		stream->WriteDword( &ItemIndex);
		stream->WriteDword( &tmpDword);
		ItemIndex +=tmpDword;
		GameScript *s = c->Scripts[0];
		if (s) {
			stream->WriteResRef( s->GetName() );
		} else {
			stream->Write( filling, 8);
		}
		//outline polygon index and count
		tmpWord = c->outline->count;
		stream->WriteDword( &VertIndex);
		stream->WriteWord( &tmpWord);
		VertIndex +=tmpWord;
		tmpWord = 0;
		stream->WriteWord( &tmpWord); //vertex count is made short
		//this is the real scripting name
		stream->Write( c->GetScriptName(), 32);
		stream->WriteResRef( c->KeyResRef);
		stream->WriteDword( &tmpDword); //unknown80
		stream->WriteDword( &c->OpenFail);
		stream->Write( filling, 56); //unknown or unused stuff
	}
	return 0;
}

int AREImp::PutRegions( DataStream *stream, Map *map, ieDword &VertIndex)
{
	ieDword tmpDword = 0;
	ieWord tmpWord;
	char filling[56];

	memset(filling,0,sizeof(filling) );
	for (unsigned int i=0;i<InfoPointsCount;i++) {
		InfoPoint *ip = map->TMap->GetInfoPoint(i);

		stream->Write( ip->GetScriptName(), 32);
		tmpWord = (ieWord) ip->Type;
		stream->WriteWord( &tmpWord);
		//outline bounding box
		tmpWord = (ieWord) ip->outline->BBox.x;
		stream->WriteWord( &tmpWord);
		tmpWord = (ieWord) ip->outline->BBox.y;
		stream->WriteWord( &tmpWord);
		tmpWord = (ieWord) (ip->outline->BBox.x + ip->outline->BBox.w);
		stream->WriteWord( &tmpWord);
		tmpWord = (ieWord) (ip->outline->BBox.y + ip->outline->BBox.h);
		stream->WriteWord( &tmpWord);
		tmpWord = (ieWord) ip->outline->count;
		stream->WriteWord( &tmpWord);
		stream->WriteDword( &VertIndex);
		VertIndex += tmpWord;
		stream->WriteDword( &tmpDword); //unknown30
		stream->WriteDword( &ip->Cursor);
		stream->WriteResRef( ip->Destination);
		stream->Write( ip->EntranceName, 32);
		stream->WriteDword( &ip->Flags);
		stream->WriteDword( &ip->StrRef);
		stream->WriteWord( &ip->TrapDetectionDiff);
		stream->WriteWord( &ip->TrapRemovalDiff);
		stream->WriteWord( &ip->Trapped); //unknown???
		stream->WriteWord( &ip->TrapDetected);
		tmpWord = (ieWord) ip->TrapLaunch.x;
		stream->WriteWord( &tmpWord);
		tmpWord = (ieWord) ip->TrapLaunch.y;
		stream->WriteWord( &tmpWord);
		stream->WriteResRef( ip->KeyResRef);
		GameScript *s = ip->Scripts[0];
		if (s) {
			stream->WriteResRef( s->GetName() );
		} else {
			stream->Write( filling, 8);
		}
		stream->Write( filling, 56); //unknown, including some points
		stream->WriteResRef( ip->DialogResRef);
	}
	return 0;
}

int AREImp::PutSpawns( DataStream *stream, Map *map)
{
	ieWord tmpWord;
	char filling[56];

	memset(filling,0,sizeof(filling) );
	for (unsigned int i=0;i<SpawnCount;i++) {
		Spawn *sp = map->GetSpawn(i);

		stream->Write( sp->Name, 32);
		tmpWord = (ieWord) sp->Pos.x;
		stream->WriteWord( &tmpWord);
		tmpWord = (ieWord) sp->Pos.y;
		stream->WriteWord( &tmpWord);
		tmpWord = sp->GetCreatureCount();
		int j;
		for (j = 0;j < tmpWord; j++) {
			stream->WriteResRef( sp->Creatures[j] );
		}
		while( j++<MAX_RESCOUNT) {
			stream->Write( filling, 8);
		}
		stream->WriteWord( &tmpWord );
		stream->Write( filling, 14); //these values may actually mean something, but we don't care now
		stream->WriteWord( &sp->Difficulty);
		stream->WriteWord( &sp->Flags);
		stream->WriteDword( &sp->appearance);
		stream->WriteWord( &sp->DayChance);
		stream->WriteWord( &sp->NightChance);
		stream->Write( filling, 56); //most likely unused crap
	}
	return 0;
}

void AREImp::PutScript(DataStream *stream, Actor *ac, unsigned int index)
{
	char filling[8];

	GameScript *s = ac->Scripts[index];
	if (s) {
		stream->WriteResRef( s->GetName() );
	} else {
		memset(filling,0,sizeof(filling));
		stream->Write( filling, 8);
	}
}

int AREImp::PutActors( DataStream *stream, Map *map)
{
	ieDword tmpDword = 0;
	ieWord tmpWord;
	ieDword CreatureOffset = EmbeddedCreOffset;
	char filling[120];
	unsigned int i;

	ActorMgr *am = (ActorMgr *) core->GetInterface( IE_CRE_CLASS_ID );
	memset(filling,0,sizeof(filling) );
	for (i=0;i<ActorCount;i++) {
		Actor *ac = map->GetActor(i, false);

		stream->Write( ac->GetScriptName(), 32);
		tmpWord = (ieWord) ac->Pos.x;
		stream->WriteWord( &tmpWord);
		tmpWord = (ieWord) ac->Pos.y;
		stream->WriteWord( &tmpWord);
		tmpWord = (ieWord) ac->Destination.x;
		stream->WriteWord( &tmpWord);
		tmpWord = (ieWord) ac->Destination.y;
		stream->WriteWord( &tmpWord);

		stream->WriteDword( &tmpDword); //used fields flag always 0 for saved areas
		stream->WriteDword( &tmpDword); //unknown2c
		stream->WriteDword( &tmpDword); //actor animation, unused
		tmpWord = ac->GetOrientation();
		stream->WriteWord( &tmpWord);
		tmpWord = 0;
		stream->WriteWord( &tmpWord); //unknown
		stream->WriteDword( &tmpDword);
		stream->WriteDword( &tmpDword); //more unknowns
		stream->WriteDword( &ac->appearance);
		stream->WriteDword( &ac->TalkCount);
		stream->WriteResRef( ac->Dialog);
		PutScript(stream, ac, SCR_OVERRIDE);
		PutScript(stream, ac, SCR_CLASS);
		PutScript(stream, ac, SCR_RACE);
		PutScript(stream, ac, SCR_GENERAL);
		PutScript(stream, ac, SCR_DEFAULT);
		PutScript(stream, ac, SCR_SPECIFICS);
 		//creature reference is empty because we are embedding it
		//the original engine used a '*'
		stream->Write( filling, 8);
		stream->WriteDword( &CreatureOffset);
		ieDword CreatureSize = am->GetStoredFileSize(ac);
		stream->WriteDword( &CreatureSize);
		CreatureOffset += CreatureSize;
		PutScript(stream, ac, SCR_AREA);
		stream->Write( filling, 120);
	}

	CreatureOffset = EmbeddedCreOffset;
	for (i=0;i<ActorCount;i++) {
		Actor *ac = map->GetActor(i, false);

		//reconstructing offsets again
		am->GetStoredFileSize(ac);
		am->PutActor( stream, ac);
	}
	core->FreeInterface( am);

	return 0;
}

int AREImp::PutAnimations( DataStream *stream, Map *map)
{
	ieWord tmpWord;

	for (unsigned int i=0;i<AnimCount;i++) {
		AreaAnimation *an = map->GetAnimation(i);

		stream->Write( an->Name, 32);
		tmpWord = (ieWord) an->Pos.x;
		stream->WriteWord( &tmpWord);
		tmpWord = (ieWord) an->Pos.y;
		stream->WriteWord( &tmpWord);
		stream->WriteDword( &an->appearance);
		stream->WriteResRef( an->BAM);
		stream->WriteWord( &an->sequence);
		stream->WriteWord( &an->frame);
		stream->WriteDword( &an->Flags);
		stream->WriteWord( &an->unknown38);// speed or percentage
		stream->WriteWord( &an->transparency);
		stream->WriteWord( &an->unknown3c); //animation already played?
		stream->Write( &an->startchance,1);
		stream->Write( &an->skipcycle,1);
		stream->WriteResRef( an->Palette);
		stream->WriteDword( &an->unknown48);//seems utterly unused
	}
	return 0;
}

int AREImp::PutEntrances( DataStream *stream, Map *map)
{
	ieWord tmpWord;
	char filling[66];

	memset(filling,0,sizeof(filling) );
	for (unsigned int i=0;i<EntrancesCount;i++) {
		Entrance *e = map->GetEntrance(i);

		stream->Write( e->Name, 32);
		tmpWord = (ieWord) e->Pos.x;
		stream->WriteWord( &tmpWord);
		tmpWord = (ieWord) e->Pos.y;
		stream->WriteWord( &tmpWord);
		stream->WriteWord( &e->Face);
		//a large empty piece of crap
		stream->Write( filling, 66);
	}
	return 0;
}

int AREImp::PutVariables( DataStream *stream, Map *map)
{
	char filling[40];
	POSITION pos=NULL;
	const char *name;
	ieDword value;

	memset(filling,0,sizeof(filling) );
	for (unsigned int i=0;i<VariablesCount;i++) {
		pos=map->locals->GetNextAssoc( pos, name, value);
		//name isn't necessarily 32 bytes long, so we play safe
		strncpy(filling, name, 32);
		stream->Write( filling, 40);
		//clearing up after the strncpy so we'll write 0's next
		memset(filling,0,sizeof(filling) );
		stream->WriteDword( &value);
		//40 bytes of empty crap
		stream->Write( filling, 40);
	}
	return 0;
}

int AREImp::PutAmbients( DataStream *stream, Map *map)
{
	char filling[64];
	ieWord tmpWord;

	memset(filling,0,sizeof(filling) );
	for (unsigned int i=0;i<AmbiCount;i++) {
		Ambient *am = map->GetAmbient(i);
		stream->Write( am->name, 32 );
		tmpWord = (ieWord) am->origin.x;
		stream->WriteWord( &tmpWord );
		tmpWord = (ieWord) am->origin.y;
		stream->WriteWord( &tmpWord );
		stream->WriteWord( &am->radius );
		stream->WriteWord( &am->height );
		stream->Write( filling, 6 );
		stream->WriteWord( &am->gain );
		tmpWord = am->sounds.size();
		int j;
		for (j = 0;j < tmpWord; j++) {
			stream->WriteResRef( am->sounds[j] );
		}
		while( j++<MAX_RESCOUNT) {
			stream->Write( filling, 8);
		}
		stream->WriteWord( &tmpWord );
		stream->Write( filling, 2 );
		stream->WriteDword( &am->interval );
		stream->WriteDword( &am->perset );
		stream->WriteDword( &am->appearance );
		stream->WriteDword( &am->flags );
		stream->Write( filling, 64);
	}
	return 0;
}

int AREImp::PutMapnotes( DataStream *stream, Map *map)
{
	char filling[8];
	ieDword tmpDword;
	ieWord tmpWord;

	//different format
	int pst = core->HasFeature( GF_AUTOMAP_INI );

	memset(filling,0,sizeof(filling) );
	for (unsigned int i=0;i<NoteCount;i++) {
		MapNote *mn = map->GetMapNote(i);
		int x;

		if (pst) {
			tmpDword = (ieWord) mn->Pos.x;
			stream->WriteDword( &tmpDword );
			tmpDword = (ieDword) mn->Pos.y;
			stream->WriteDword( &tmpDword );
			int len = strlen(mn->text);
			if (len>500) len=500;
			stream->Write( mn->text, len);
			x = 500-len;
			for (int j=0;j<x/8;j++) {
				stream->Write( filling, 8);
			}
			x = x%8;
			if (x) {
				stream->Write( filling, x);
			}
			stream->WriteWord( &mn->color);
			stream->WriteWord( &tmpWord);
			for (x=0;x<5;x++) { //5 empty dwords
				stream->Write( filling, 4);
			}
		} else {
			tmpWord = (ieWord) mn->Pos.x;
			stream->WriteWord( &tmpWord );
			tmpWord = (ieWord) mn->Pos.y;
			stream->WriteWord( &tmpWord );
			//strref
			stream->WriteDword( &tmpDword);
			stream->WriteWord( &tmpWord);
			stream->WriteWord( &mn->color);
			tmpDword = 1;
			stream->WriteDword( &tmpDword);
			for (x=0;x<9;x++) { //9 empty dwords
				stream->Write( filling, 4);
			}
		}
	}
	return 0;
}

int AREImp::PutExplored( DataStream *stream, Map *map)
{
	stream->Write( map->ExploredBitmap, ExploredBitmapSize);
	return 0;
}

int AREImp::PutTiles( DataStream * stream, Map * map)
{
	char filling[48];
	ieDword tmpDword = 0;

	memset(filling,0,sizeof(filling) );
	for (unsigned int i=0;i<TileCount;i++) {
		TileObject *am = map->TMap->GetTile(i);
		stream->Write( am->Name, 32 );
		stream->WriteResRef( am->Tileset );
		stream->WriteDword( &am->Flags);
		stream->WriteDword( &am->opencount);
		//can't write tiles, otherwise now we should write a tile index
		stream->WriteDword( &tmpDword);
		stream->WriteDword( &am->closedcount);
		//can't write tiles otherwise now we should write a tile index
		stream->WriteDword( &tmpDword);
		stream->Write( filling, 48);
	}
	return 0;
}

int AREImp::PutSongHeader( DataStream *stream, Map *map)
{
	int i;
	char filling[8];
	ieDword tmpDword = 0;

	memset(filling,0,sizeof(filling) );
	for(i=0;i<MAX_RESCOUNT;i++) {
		stream->WriteDword( map->SongHeader.SongList);
	}
	//day
	stream->Write( filling,8);
	stream->Write( filling,8);
	stream->WriteDword( &tmpDword);
	//night
	stream->Write( filling,8);
	stream->Write( filling,8);
	stream->WriteDword( &tmpDword);
	//song flag
	stream->WriteDword( &tmpDword);
	//lots of empty crap
	for(i=0;i<23;i++) {
		stream->WriteDword( &tmpDword);
	}
	return 0;
}

int AREImp::PutRestHeader( DataStream *stream, Map *map)
{
	int i;
	ieDword tmpDword = 0;
	ieWord tmpWord = 0;

	for(i=0;i<MAX_RESCOUNT;i++) {
		stream->WriteDword( &map->RestHeader.Strref[i]);
	}
	for(i=0;i<MAX_RESCOUNT;i++) {
		stream->WriteResRef( map->RestHeader.CreResRef[i]);
	}
	stream->WriteWord( &map->RestHeader.CreatureNum);
	//lots of unknowns
	stream->WriteWord( &tmpWord);
	for(i=0;i<6;i++) {
		stream->WriteWord( &tmpWord);
	}
	stream->WriteWord( &map->RestHeader.DayChance);
	stream->WriteWord( &map->RestHeader.NightChance);
	for(i=0;i<14;i++) {
		stream->WriteDword( &tmpDword);
	}
	return 0;
}

/* no saving of tiled objects, are they used anywhere? */
int AREImp::PutArea(DataStream *stream, Map *map)
{
	ieDword VertIndex = 0;
	int ret;

	if (!stream || !map) {
		return -1;
	}

	ret = PutHeader( stream, map);
	if (ret) {
		return ret;
	}

	ret = PutActors( stream, map);
	if (ret) {
		return ret;
	}

	ret = PutRegions( stream, map, VertIndex);
	if (ret) {
		return ret;
	}

	ret = PutSpawns( stream, map);
	if (ret) {
		return ret;
	}

	ret = PutEntrances( stream, map);
	if (ret) {
		return ret;
	}

	ret = PutContainers( stream, map, VertIndex);
	if (ret) {
		return ret;
	}

	ret = PutItems( stream, map);
	if (ret) {
		return ret;
	}

	ret = PutDoors( stream, map, VertIndex);
	if (ret) {
		return ret;
	}

	ret = PutVertices( stream, map);
	if (ret) {
		return ret;
	}

	ret = PutAmbients( stream, map);
	if (ret) {
		return ret;
	}

	ret = PutVariables( stream, map);
	if (ret) {
		return ret;
	}

	ret = PutAnimations( stream, map);
	if (ret) {
		return ret;
	}

	ret = PutTiles( stream, map);
	if (ret) {
		return ret;
	}

	ret = PutExplored( stream, map);
	if (ret) {
		return ret;
	}

	ret = PutMapnotes( stream, map);
	if (ret) {
		return ret;
	}

	ret = PutRestHeader( stream, map);
	if (ret) {
		return ret;
	}

	ret = PutSongHeader( stream, map);

	return ret;
}

