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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/AREImporter/AREImp.cpp,v 1.74 2004/10/10 13:03:08 avenger_teambg Exp $
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
#include "../Core/Ambient.h"

//in areas 10 is a magic number for resref counts
#define MAX_RESCOUNT 10 

#define DEF_OPEN   0
#define DEF_CLOSE  1
#define DEF_HOPEN  2
#define DEF_HCLOSE 3

#define DEF_COUNT 4

#define DOOR_HIDDEN 128

static char Sounds[DEF_COUNT][9] = {
	{-1},
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
}

//this is the same as the function in the creature, you might want to rationalize it
CREItem* AREImp::GetItem()
{
	CREItem *itm = new CREItem();

	str->ReadResRef( itm->ItemResRef );
	str->ReadWord( &itm->Unknown08 );
	str->ReadWord( &itm->Usages[0] );
	str->ReadWord( &itm->Usages[1] );
	str->ReadWord( &itm->Usages[2] );
	str->ReadDword( &itm->Flags );

	return itm;
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
	str->ReadResRef( WEDResRef );
	str->ReadDword( &LastSave );
	str->ReadDword( &AreaFlags );
	str->Seek( 0x48 + bigheader, GEM_STREAM_START );
	str->ReadWord( &AreaType );
	str->ReadWord( &WRain );
	str->ReadWord( &WSnow );
	str->ReadWord( &WFog );
	str->ReadWord( &WLightning );
	str->ReadWord( &WUnknown );
	str->ReadDword( &ActorOffset );
	str->ReadWord( &ActorCount );
	str->Seek( 0x5A + bigheader, GEM_STREAM_START );
	str->ReadWord( &InfoPointsCount  );
	str->ReadDword( &InfoPointsOffset );
	str->Seek( 0x68 + bigheader, GEM_STREAM_START );
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
	str->Seek( 0xA4 + bigheader, GEM_STREAM_START );
	str->ReadDword( &DoorsCount );
	str->ReadDword( &DoorsOffset );
	str->ReadDword( &AnimCount );
	str->ReadDword( &AnimOffset );
	str->Seek( 8, GEM_CURRENT_POS ); //skipping some
	str->ReadDword( &SongHeader );
	str->ReadDword( &RestHeader );
	if (core->HasFeature(GF_AUTOMAP_INI) ) {
		str->ReadDword( &tmp ); //skipping crap in PST
	}
	str->ReadDword( &NoteOffset );
	str->ReadDword( &NoteCount );
	return true;
}

Map* AREImp::GetMap(const char *ResRef)
{
	unsigned int i,x;

	Map* map = new Map();
	map->AreaFlags=AreaFlags;
	map->AreaType=AreaType;

	//we have to set this here because the actors will receive their
	//current area setting here
	strncpy(map->scriptName, ResRef, 8);
	map->scriptName[8]=0;

	if (!core->IsAvailable( IE_WED_CLASS_ID )) {
		printf( "[AREImporter]: No Tile Map Manager Available.\n" );
		return false;
	}
	TileMapMgr* tmm = ( TileMapMgr* ) core->GetInterface( IE_WED_CLASS_ID );
	DataStream* wedfile = core->GetResourceMgr()->GetResource( WEDResRef, IE_WED_CLASS_ID );
	tmm->Open( wedfile );
	TileMap* tm = tmm->GetTileMap();

	map->Scripts[0] = new GameScript( Script, IE_SCRIPT_AREA );
	map->MySelf = map;
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

	str->Seek( SongHeader, GEM_STREAM_START );
	//5 is the number of song indices
	for (i = 0; i < 5; i++) {
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
	str->Seek( 14, GEM_CURRENT_POS );
	str->ReadWord( &map->RestHeader.DayChance );
	str->ReadWord( &map->RestHeader.NightChance );

	printf( "Loading doors\n" );
	//Loading Doors
	for (i = 0; i < DoorsCount; i++) {
		str->Seek( DoorsOffset + ( i * 0xc8 ), GEM_STREAM_START );
		int count;
		ieDword Flags, OpenFirstVertex, ClosedFirstVertex;
		ieWord OpenVerticesCount, ClosedVerticesCount;
		char LongName[33], ShortName[9];
		ieWord minX, maxX, minY, maxY;
		ieDword cursor;
		Region BBClosed, BBOpen;
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
		str->Seek( 0x10, GEM_CURRENT_POS );
		ieResRef OpenResRef, CloseResRef;
		str->ReadResRef( OpenResRef );
		str->ReadResRef( CloseResRef );
		str->ReadDword( &cursor );
		str->Seek( 36, GEM_CURRENT_POS );
		Point toOpen[2];
		str->ReadWord( &minX );
		toOpen[0].x = minX;
		str->ReadWord( &minY );
		toOpen[0].y = minY;
		str->ReadWord( &maxX );
		toOpen[1].x = maxX;
		str->ReadWord( &maxY );
		toOpen[1].y = maxY;
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
	for (i = 0; i < ContainersCount; i++) {
		str->Seek( ContainersOffset + ( i * 0xC0 ), GEM_STREAM_START );
		ieWord Type, LockDiff, Locked, Unknown;
		ieWord TrapDetDiff, TrapRemDiff, Trapped, TrapDetected;
		ieWord XPos, YPos;
		ieWord LaunchX, LaunchY;
		ieDword ItemIndex, ItemCount;
		char Name[33];
		str->Read( Name, 32 );
		Name[32] = 0;
		str->ReadWord( &XPos );
		str->ReadWord( &YPos );
		str->ReadWord( &Type );
		str->ReadWord( &LockDiff );
		str->ReadWord( &Locked );
		str->ReadWord( &Unknown );
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
		Container* c = tm->AddContainer( Name, Type, poly );
		c->area = map;
		c->Pos.x = XPos;
		c->Pos.y = YPos;
		c->LockDifficulty = LockDiff;
		c->Locked = Locked;
		c->TrapDetectionDiff = TrapDetDiff;
		c->TrapRemovalDiff = TrapRemDiff;
		c->Trapped = Trapped;
		c->TrapDetected = TrapDetected;
		c->TrapLaunch.x = LaunchX;
		c->TrapLaunch.y = LaunchY;
		//reading items into a container
		str->Seek( ItemsOffset+( ItemIndex * 0x14 ), GEM_STREAM_START);
		while(ItemCount--) {
			c->inventory.AddItem( GetItem());
		}
		if (Script[0] != 0) {
			c->Scripts[0] = new GameScript( Script, IE_SCRIPT_TRIGGER );
			c->Scripts[0]->MySelf = c;
		} else
			c->Scripts[0] = NULL;
	}
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
		//don't even bother reading the script if it isn't trapped
		if(Trapped || Type) {
			str->ReadResRef( Script );
		}
		else {
			Script[0] = 0;
		}
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
		ip->TrapDetectionDifficulty = TrapDetDiff;
		ip->TrapRemovalDifficulty = TrapRemDiff;
		//we don't need this flag, because the script is loaded
		//only if it exists
		ip->TrapDetected = TrapDetected;
		ip->TrapLaunch.x = LaunchX;
		ip->TrapLaunch.y = LaunchY;
		ip->Cursor = Cursor;
		ip->overHeadText = string;
		ip->textDisplaying = 0;
		ip->timeStartDisplaying = 0;
		ip->area = map;
		ip->Pos.x = bbox.x + ( bbox.w / 2 );
		ip->Pos.y = bbox.y + ( bbox.h / 2 );
		ip->Flags = Flags;
		strcpy( ip->Destination, Destination );
		strcpy( ip->EntranceName, Entrance );
		strcpy( ip->KeyResRef, KeyResRef );
		strcpy( ip->DialogResRef, DialogResRef );
		if (Script[0] != 0) {
			ip->Scripts[0] = new GameScript( Script, IE_SCRIPT_TRIGGER );
			ip->Scripts[0]->MySelf = ip;
		} else
			ip->Scripts[0] = NULL;
	}
	//we need this so we can filter global actors
	Game *game=core->GetGame();
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
			str->Read( DefaultName, 32);
			DefaultName[32]=0;
			str->ReadWord( &XPos );
			str->ReadWord( &YPos );
			str->ReadWord( &XDes );
			str->ReadWord( &YDes );
			str->Seek( 12, GEM_CURRENT_POS );
			str->ReadDword( &Orientation );
			str->Seek( 8, GEM_CURRENT_POS );
			str->ReadDword( &Schedule );
			str->ReadDword( &TalkCount );
			str->Seek( 56, GEM_CURRENT_POS );
			str->ReadResRef( CreResRef );
			DataStream* crefile;
			ieDword CreOffset, CreSize;
			str->ReadDword( &CreOffset );
			str->ReadDword( &CreSize );
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
				crefile = core->GetResourceMgr()->GetResource( CreResRef, IE_CRE_CLASS_ID );
			}
			actmgr->Open( crefile, true );
			Actor* ab = actmgr->GetActor();
			if(!ab)
				continue;
			ab->area = map;
			ab->Pos.x = XPos;
			ab->Pos.y = YPos;
			ab->Destination.x = XDes;
			ab->Destination.y = YDes;
			//copying the area name into the actor
			strcpy(ab->Area, map->scriptName);
			//copying the scripting name into the actor
			//this hack allows iwd starting cutscene to work
			if(stricmp(ab->scriptName,"none")==0) {
				ab->SetScriptName(DefaultName);
			}
	
			if (ab->BaseStats[IE_STATE_ID] & STATE_DEAD)
				ab->StanceID = IE_ANI_SLEEP;
			else
				ab->StanceID = IE_ANI_AWAKE;
			
			ab->Orientation = ( unsigned char ) Orientation;
			ab->TalkCount = TalkCount;
			//hack to not load global actors to area
			if(!game->FindPC(ab->scriptName) && !game->FindNPC(ab->scriptName) ) {
				map->AddActor( ab );
			} else {
				delete ab;
			}
		}
		core->FreeInterface( actmgr );
	}
	str->Seek( AnimOffset, GEM_STREAM_START );
	if (!core->IsAvailable( IE_BAM_CLASS_ID )) {
		printf( "[AREImporter]: No Animation Manager Available, skipping animations\n" );
	} else {
		for (i = 0; i < AnimCount; i++) {
			Animation* anim;
			str->Seek( 32, GEM_CURRENT_POS );
			ieWord animX, animY;
			str->ReadWord( &animX );
			str->ReadWord( &animY );
			str->Seek( 4, GEM_CURRENT_POS );
			ieResRef animBam;
			str->ReadResRef( animBam );
			ieWord animCycle, animFrame;
			str->ReadWord( &animCycle );
			str->ReadWord( &animFrame );
			ieDword animFlags;
			str->ReadDword( &animFlags );
			str->Seek( 20, GEM_CURRENT_POS );
			unsigned char mode = ( ( animFlags & 2 ) != 0 ) ?
				IE_SHADED : IE_NORMAL;
			AnimationFactory* af = ( AnimationFactory* )
			core->GetResourceMgr()->GetFactoryResource( animBam, IE_BAM_CLASS_ID );
			anim = af->GetCycle( ( unsigned char ) animCycle );
			if (!anim)
				anim = af->GetCycle( 0 );
			anim->x = animX;
			anim->y = animY;
			anim->BlitMode = mode;
			anim->autofree = false;
			strcpy( anim->ResRef, animBam );
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
	map->vars=new Variables();
	map->vars->SetType( GEM_VARIABLES_INT );

	str->Seek( VariablesOffset, GEM_STREAM_START );
	for (i = 0; i < VariablesCount; i++) {
		char Name[33];
		ieDword Value;
		str->Read( Name, 32 );
		Name[32] = 0;
		str->Seek( 8, GEM_CURRENT_POS );
		str->ReadDword( &Value );
		str->Seek( 40, GEM_CURRENT_POS );
		map->vars->SetAt( Name, Value );
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

	int tmp = core->HasFeature( GF_AUTOMAP_INI );
	if (tmp) {
		//add automap ini entries
	}
	for (i = 0; i < NoteCount; i++) {
		Point point;
		int color;
		char *text;
		if (tmp) {
			ieDword px,py;

			str->ReadDword(&px);
			str->ReadDword(&py);
			point.x=px;
			point.y=py;
			color=0;
			text = (char *) malloc( 524 );
			str->Read(text, 524 );
			text[523] = 0;
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
	
	map->AddTileMap( tm, lm, sr, sm );
	core->FreeInterface( tmm );
	return map;
}

