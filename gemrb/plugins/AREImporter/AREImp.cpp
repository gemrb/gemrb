/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/AREImporter/AREImp.cpp,v 1.43 2004/03/15 15:25:14 avenger_teambg Exp $
 *
 */

#include "../../includes/win32def.h"
#include "AREImp.h"
#include "../Core/TileMapMgr.h"
#include "../Core/AnimationMgr.h"
#include "../Core/Interface.h"
#include "../Core/ActorMgr.h"
#include "../Core/FileStream.h"
#include "../Core/ImageMgr.h"

#define DEF_OPEN   0
#define DEF_CLOSE  1
#define DEF_HOPEN  2
#define DEF_HCLOSE 3

#define DEF_COUNT 4

#define DOOR_HIDDEN 128

static char Sounds[DEF_COUNT][9] = {
	-1
};

AREImp::AREImp(void)
{
	autoFree = false;
	str = NULL;
	if (Sounds[0][0] == -1) {
		memset( Sounds, 0, sizeof( Sounds ) );
		int SoundTable = core->LoadTable( "defsound" );
		TableMgr* at = core->GetTable( SoundTable );
		if (at) {
			for (int i = 0; i < DEF_COUNT; i++) {
				strncpy( Sounds[i], at->QueryField( i, 0 ), 8 );
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
	int bigheader;
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
	str->Read( WEDResRef, 8 );
	str->Seek( 0x54 + bigheader, GEM_STREAM_START );
	str->Read( &ActorOffset, 4 );
	str->Read( &ActorCount, 2 );
	str->Seek( 0x5A + bigheader, GEM_STREAM_START );
	str->Read( &InfoPointsCount, 2 );
	str->Read( &InfoPointsOffset, 4 );
	str->Seek( 0x68 + bigheader, GEM_STREAM_START );
	str->Read( &EntrancesOffset, 4 );
	str->Read( &EntrancesCount, 4 );
	str->Read( &ContainersOffset, 4 );
	str->Read( &ContainersCount, 2 );
	str->Seek( 0x7C + bigheader, GEM_STREAM_START );
	str->Read( &VerticesOffset, 4 );
	str->Read( &VerticesCount, 2 );
	str->Seek( 0x94 + bigheader, GEM_STREAM_START );
	str->Read( Script, 8 );
	Script[8] = 0;
	//core->LoadScript(Script);
	str->Seek( 0xA4 + bigheader, GEM_STREAM_START );
	str->Read( &DoorsCount, 4 );
	str->Read( &DoorsOffset, 4 );
	//str->Seek(0xac, GEM_STREAM_START);
	//str->Seek(0xac+bigheader, GEM_STREAM_START);
	str->Read( &AnimCount, 4 );
	str->Read( &AnimOffset, 4 );
	str->Seek( 8, GEM_CURRENT_POS ); //skipping some
	str->Read( &SongHeader, 4 );
	return true;
}

Map* AREImp::GetMap()
{
	Map* map = new Map();
	strncpy( map->scriptName, WEDResRef, 8);
	map->scriptName[8]=0;

	if (!core->IsAvailable( IE_WED_CLASS_ID )) {
		printf( "[AREImporter]: No Tile Map Manager Available.\n" );
		return false;
	}
	TileMapMgr* tmm = ( TileMapMgr* ) core->GetInterface( IE_WED_CLASS_ID );
	DataStream* wedfile = core->GetResourceMgr()->GetResource( WEDResRef,
													IE_WED_CLASS_ID );
	tmm->Open( wedfile );
	TileMap* tm = tmm->GetTileMap();

	map->Scripts[0] = new GameScript( Script, IE_SCRIPT_AREA );
	map->MySelf = map;
	if (map->Scripts[0]) {
		map->Scripts[0]->MySelf = map;
	}

	char ResRef[9];
	strcpy( ResRef, WEDResRef );
	strcat( ResRef, "LM" );

	ImageMgr* lm = ( ImageMgr* ) core->GetInterface( IE_BMP_CLASS_ID );
	DataStream* lmstr = core->GetResourceMgr()->GetResource( ResRef,
													IE_BMP_CLASS_ID );
	lm->Open( lmstr, true );

	strcpy( ResRef, WEDResRef );
	strcat( ResRef, "SR" );
	printf( "Loading %s\n", ResRef );
	ImageMgr* sr = ( ImageMgr* ) core->GetInterface( IE_BMP_CLASS_ID );
	DataStream* srstr = core->GetResourceMgr()->GetResource( ResRef,
													IE_BMP_CLASS_ID );
	sr->Open( srstr, true );

	str->Seek( SongHeader, GEM_STREAM_START );
	//5 is the number of song indices
	for (int i = 0; i < 5; i++) {
		str->Read( map->SongHeader.SongList + i, 4 );
	}

	printf( "Loading doors\n" );
	//Loading Doors
	for (unsigned long i = 0; i < DoorsCount; i++) {
		str->Seek( DoorsOffset + ( i * 0xc8 ), GEM_STREAM_START );
		int count;
		unsigned long Flags, OpenFirstVertex, ClosedFirstVertex;
		unsigned short OpenVerticesCount, ClosedVerticesCount;
		char LongName[33], ShortName[9];
		short minX, maxX, minY, maxY;
		unsigned long cursor;
		Region BBClosed, BBOpen;
		str->Read( LongName, 32 );
		LongName[32] = 0;
		str->Read( ShortName, 8 );
		ShortName[8] = 0;
		str->Read( &Flags, 4 );
		str->Read( &OpenFirstVertex, 4 );
		str->Read( &OpenVerticesCount, 2 );
		str->Read( &ClosedVerticesCount, 2 );
		str->Read( &ClosedFirstVertex, 4 );
		str->Read( &minX, 2 );
		str->Read( &minY, 2 );
		str->Read( &maxX, 2 );
		str->Read( &maxY, 2 );
		BBOpen.x = minX;
		BBOpen.y = minY;
		BBOpen.w = maxX - minX;
		BBOpen.h = maxY - minY;
		str->Read( &minX, 2 );
		str->Read( &minY, 2 );
		str->Read( &maxX, 2 );
		str->Read( &maxY, 2 );
		BBClosed.x = minX;
		BBClosed.y = minY;
		BBClosed.w = maxX - minX;
		BBClosed.h = maxY - minY;
		str->Seek( 0x10, GEM_CURRENT_POS );
		char OpenResRef[9], CloseResRef[9];
		str->Read( OpenResRef, 8 );
		OpenResRef[8] = 0;
		str->Read( CloseResRef, 8 );
		CloseResRef[8] = 0;
		str->Read( &cursor, 4 );
		str->Seek( 36, GEM_CURRENT_POS );
		Point toOpen[2];
		str->Read( &toOpen[0].x, 2 );
		str->Read( &toOpen[0].y, 2 );
		str->Read( &toOpen[1].x, 2 );
		str->Read( &toOpen[1].y, 2 );
		//Reading Open Polygon
		str->Seek( VerticesOffset + ( OpenFirstVertex * 4 ), GEM_STREAM_START );
		Point* points = ( Point* )
			malloc( OpenVerticesCount*sizeof( Point ) );
		for (int x = 0; x < OpenVerticesCount; x++) {
			str->Read( &points[x].x, 2 );
			str->Read( &points[x].y, 2 );
		}
		Gem_Polygon* open = new Gem_Polygon( points, OpenVerticesCount );
		open->BBox = BBOpen;
		free( points );
		//Reading Closed Polygon
		str->Seek( VerticesOffset + ( ClosedFirstVertex * 4 ),
				GEM_STREAM_START );
		points = ( Point * ) malloc( ClosedVerticesCount * sizeof( Point ) );
		for (int x = 0; x < ClosedVerticesCount; x++) {
			str->Read( &points[x].x, 2 );
			str->Read( &points[x].y, 2 );
		}
		Gem_Polygon* closed = new Gem_Polygon( points, ClosedVerticesCount );
		closed->BBox = BBClosed;
		free( points );
		//Getting Door Information from the WED File
		bool BaseClosed;
		unsigned short * indices = tmm->GetDoorIndices( ShortName, &count,
											BaseClosed );
		Door* door;
		door = tm->AddDoor( ShortName, Flags, BaseClosed,
					indices, count, open, closed );
		door->Cursor = cursor;
		door->toOpen[0] = toOpen[0];
		door->toOpen[1] = toOpen[1];
		//Leave the default sound untouched
		if (OpenResRef[0])
			memcpy( door->OpenSound, OpenResRef, 9 );
		else {
			if (Flags & DOOR_HIDDEN)
				memcpy( door->OpenSound, Sounds[DEF_HOPEN], 9 );
			else
				memcpy( door->OpenSound, Sounds[DEF_OPEN], 9 );
		}
		if (CloseResRef[0])
			memcpy( door->CloseSound, CloseResRef, 9 );
		else {
			if (Flags & DOOR_HIDDEN)
				memcpy( door->CloseSound, Sounds[DEF_HCLOSE], 9 );
			else
				memcpy( door->CloseSound, Sounds[DEF_CLOSE], 9 );
		}
	}
	printf( "Loading containers\n" );
	//Loading Containers
	for (int i = 0; i < ContainersCount; i++) {
		str->Seek( ContainersOffset + ( i * 0xC0 ), GEM_STREAM_START );
		unsigned short Type, LockDiff, Locked, Unknown, TrapDetDiff,
		TrapRemDiff, Trapped, TrapDetected;
		char Name[33];
		Point p;
		str->Read( Name, 32 );
		Name[32] = 0;
		str->Read( &p.x, 2 );
		str->Read( &p.y, 2 );
		str->Read( &Type, 2 );
		str->Read( &LockDiff, 2 );
		str->Read( &Locked, 2 );
		str->Read( &Unknown, 2 );
		str->Read( &TrapDetDiff, 2 );
		str->Read( &TrapRemDiff, 2 );
		str->Read( &Trapped, 2 );
		str->Read( &TrapDetected, 2 );
		str->Seek( 4, GEM_CURRENT_POS );
		Region bbox;
		str->Read( &bbox.x, 2 );
		str->Read( &bbox.y, 2 );
		str->Read( &bbox.w, 2 );
		str->Read( &bbox.h, 2 );
		bbox.w -= bbox.x;
		bbox.h -= bbox.y;
		str->Seek( 16, GEM_CURRENT_POS );
		unsigned long firstIndex, vertCount;
		str->Read( &firstIndex, 4 );
		str->Read( &vertCount, 4 );
		str->Seek( VerticesOffset + ( firstIndex * 4 ), GEM_STREAM_START );
		Point* points = ( Point* ) malloc( vertCount*sizeof( Point ) );
		for (unsigned long x = 0; x < vertCount; x++) {
			str->Read( &points[x].x, 2 );
			str->Read( &points[x].y, 2 );
		}
		Gem_Polygon* poly = new Gem_Polygon( points, vertCount );
		free( points );
		poly->BBox = bbox;
		Container* c = tm->AddContainer( Name, Type, poly );
		c->LockDifficulty = LockDiff;
		c->Locked = Locked;
		c->TrapDetectionDiff = TrapDetDiff;
		c->TrapRemovalDiff = TrapRemDiff;
		c->Trapped = Trapped;
		c->TrapDetected = TrapDetected;
	}
	printf( "Loading regions\n" );
	//Loading InfoPoints
	for (int i = 0; i < InfoPointsCount; i++) {
		str->Seek( InfoPointsOffset + ( i * 0xC4 ), GEM_STREAM_START );
		unsigned short Type, VertexCount;
		unsigned long FirstVertex, Cursor, EndFlags;
		unsigned short TrapDetDiff, TrapRemDiff, Trapped, TrapDetected;
		unsigned short LaunchX, LaunchY;
		char Name[33], Script[9], Key[9], Destination[9], Entrance[33];
		str->Read( Name, 32 );
		Name[32] = 0;
		str->Read( &Type, 2 );
		Region bbox;
		str->Read( &bbox.x, 2 );
		str->Read( &bbox.y, 2 );
		str->Read( &bbox.w, 2 );
		str->Read( &bbox.h, 2 );
		bbox.w -= bbox.x;
		bbox.h -= bbox.y;
		str->Read( &VertexCount, 2 );
		str->Read( &FirstVertex, 4 );
		str->Seek( 4, GEM_CURRENT_POS );
		str->Read( &Cursor, 4 );
		str->Read( Destination, 8 );
		Destination[8] = 0;
		str->Read( Entrance, 32 );
		Entrance[32] = 0;
		str->Read( &EndFlags, 4 );
		unsigned long StrRef;
		str->Read( &StrRef, 4 );
		str->Read( &TrapDetDiff, 2 );
		str->Read( &TrapRemDiff, 2 );
		str->Read( &Trapped, 2 );
		str->Read( &TrapDetected, 2 );
		str->Read( &LaunchX, 2 );
		str->Read( &LaunchY, 2 );
		str->Read( Key, 8 );
		Key[8] = 0;
		str->Read( Script, 8 );
		Script[8] = 0;
		char* string = core->GetString( StrRef );
		str->Seek( VerticesOffset + ( FirstVertex * 4 ), GEM_STREAM_START );
		Point* points = ( Point* ) malloc( VertexCount*sizeof( Point ) );
		for (int x = 0; x < VertexCount; x++) {
			str->Read( &points[x].x, 2 );
			str->Read( &points[x].y, 2 );
		}
		Gem_Polygon* poly = new Gem_Polygon( points, VertexCount );
		free( points );
		poly->BBox = bbox;
		InfoPoint* ip = tm->AddInfoPoint( Name, Type, poly );
		ip->TrapDetectionDifficulty = TrapDetDiff;
		ip->TrapRemovalDifficulty = TrapRemDiff;
		ip->Trapped = Trapped;
		ip->TrapDetected = TrapDetected;
		ip->TrapLaunchX = LaunchX;
		ip->TrapLaunchY = LaunchY;
		ip->Cursor = Cursor;
		ip->overHeadText = string;
		ip->textDisplaying = 0;
		ip->timeStartDisplaying = 0;
		ip->XPos = bbox.x + ( bbox.w / 2 );
		ip->YPos = bbox.y + ( bbox.h / 2 );
		ip->EndAction = EndFlags;
		strcpy( ip->Destination, Destination );
		strcpy( ip->EntranceName, Entrance );
		//ip->triggered = false;
		//strcpy(ip->Script, Script);
		if (Script[0] != 0) {
			ip->Scripts[0] = new GameScript( Script, IE_SCRIPT_TRIGGER );
			ip->Scripts[0]->MySelf = ip;
		} else
			ip->Scripts[0] = NULL;
	}
	printf( "Loading actors\n" );
	//Loading Actors
	str->Seek( ActorOffset, GEM_STREAM_START );
	if (!core->IsAvailable( IE_CRE_CLASS_ID )) {
		printf( "[AREImporter]: No Actor Manager Available, skipping actors\n" );
		return map;
	}
	ActorMgr* actmgr = ( ActorMgr* ) core->GetInterface( IE_CRE_CLASS_ID );
	for (int i = 0; i < ActorCount; i++) {
		char CreResRef[9];
		unsigned long TalkCount;
		unsigned long Orientation, Schedule;
		unsigned short XPos, YPos, XDes, YDes;
		str->Seek( 32, GEM_CURRENT_POS );
		str->Read( &XPos, 2 );
		str->Read( &YPos, 2 );
		str->Read( &XDes, 2 );
		str->Read( &YDes, 2 );
		str->Seek( 12, GEM_CURRENT_POS );
		str->Read( &Orientation, 4 );
		str->Seek( 8, GEM_CURRENT_POS );
		str->Read( &Schedule, 4 );
		str->Read( &TalkCount, 4 );
		str->Seek( 56, GEM_CURRENT_POS );
		str->Read( CreResRef, 8 );
		CreResRef[8] = 0;
		DataStream* crefile;
		unsigned long CreOffset, CreSize;
		str->Read( &CreOffset, 4 );
		str->Read( &CreSize, 4 );
		str->Seek( 128, GEM_CURRENT_POS );
		if (CreOffset != 0) {
			char cpath[_MAX_PATH];
			strcpy( cpath, core->GamePath );
			strcat( cpath, str->filename );
			_FILE* str = _fopen( cpath, "rb" );
			FileStream* fs = new FileStream();
			fs->Open( str, CreOffset, CreSize, true );
			crefile = fs;
		} else {
			crefile = core->GetResourceMgr()->GetResource( CreResRef,
												IE_CRE_CLASS_ID );
		}
		actmgr->Open( crefile, true );
		Actor* ab = actmgr->GetActor();
		ab->XPos = XPos;
		ab->YPos = YPos;
		ab->XDes = XDes;
		ab->YDes = YDes;
		ab->AnimID = IE_ANI_AWAKE;
		//copying the area name into the actor
		strcpy(ab->Area, map->scriptName);

		if (ab->BaseStats[IE_STATE_ID] & STATE_DEAD)
			ab->AnimID = IE_ANI_SLEEP;
		ab->Orientation = ( unsigned char ) Orientation;
		ab->TalkCount = TalkCount;
		/*for(int i = 0; i < MAX_SCRIPTS; i++) {
							if((stricmp(ab->actor->Scripts[i], "None") == 0) || (ab->actor->Scripts[i][0] == '\0')) {
								ab->Scripts[i] = NULL;
								continue;
							}
							ab->Scripts[i] = new GameScript(ab->actor->Scripts[i], 0);
							ab->Scripts[i]->MySelf = ab;
						}*/
		map->AddActor( ab );
	}
	core->FreeInterface( actmgr );
	str->Seek( AnimOffset, GEM_STREAM_START );
	if (!core->IsAvailable( IE_BAM_CLASS_ID )) {
		printf( "[AREImporter]: No Animation Manager Available, skipping animations\n" );
		return map;
	}
	//AnimationMgr * am = (AnimationMgr*)core->GetInterface(IE_BAM_CLASS_ID);
	for (unsigned int i = 0; i < AnimCount; i++) {
		Animation* anim;
		str->Seek( 32, GEM_CURRENT_POS );
		unsigned short animX, animY;
		str->Read( &animX, 2 );
		str->Read( &animY, 2 );
		str->Seek( 4, GEM_CURRENT_POS );
		char animBam[9];
		str->Read( animBam, 8 );
		animBam[8] = 0;
		unsigned short animCycle, animFrame;
		str->Read( &animCycle, 2 );
		str->Read( &animFrame, 2 );
		unsigned long animFlags;
		str->Read( &animFlags, 4 );
		str->Seek( 20, GEM_CURRENT_POS );
		unsigned char mode = ( ( animFlags & 2 ) != 0 ) ?
			IE_SHADED :
			IE_NORMAL;
		//am->Open(core->GetResourceMgr()->GetResource(animBam, IE_BAM_CLASS_ID), true);
		//anim = am->GetAnimation(animCycle, animX, animY);
		AnimationFactory* af = ( AnimationFactory* )
			core->GetResourceMgr()->GetFactoryResource( animBam,
										IE_BAM_CLASS_ID );
		anim = af->GetCycle( ( unsigned char ) animCycle );
		if (!anim)
			anim = af->GetCycle( 0 );
		anim->x = animX;
		anim->y = animY;
		anim->BlitMode = mode;
		anim->free = false;
		strcpy( anim->ResRef, animBam );
		map->AddAnimation( anim );
	}
	printf( "Loading entrances\n" );
	//Loading Entrances
	str->Seek( EntrancesOffset, GEM_STREAM_START );
	for (int i = 0; i < EntrancesCount; i++) {
		char Name[33];
		short XPos, YPos;
		str->Read( Name, 32 );
		Name[32] = 0;
		str->Read( &XPos, 2 );
		str->Read( &YPos, 2 );
		str->Seek( 68, GEM_CURRENT_POS );
		map->AddEntrance( Name, XPos, YPos );
	}
	map->AddTileMap( tm, lm, sr );
	core->FreeInterface( tmm );
	return map;
}
