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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

#include "AREImporter.h"

#include "win32def.h"
#include "strrefs.h"
#include "ie_cursors.h"

#include "ActorMgr.h"
#include "Ambient.h"
#include "DataFileMgr.h"
#include "DisplayMessage.h"
#include "EffectMgr.h"
#include "Game.h"
#include "GameData.h"
#include "ImageMgr.h"
#include "Interface.h"
#include "Palette.h"
#include "PluginMgr.h"
#include "ProjectileServer.h"
#include "RNG.h"
#include "TileMapMgr.h"
#include "GameScript/GameScript.h"
#include "Scriptable/Container.h"
#include "Scriptable/Door.h"
#include "Scriptable/InfoPoint.h"
#include "System/FileStream.h"
#include "System/SlicedStream.h"

#include <stdlib.h>
#ifdef ANDROID
// android lacks mblen
int wctomb(char *s, wchar_t wc) { return wcrtomb(s, wc, NULL); }
int mbtowc(wchar_t *pwc, const char *s, size_t n) { return mbrtowc(pwc, s, n, NULL); }
#endif

using namespace GemRB;

#define DEF_OPEN   0
#define DEF_CLOSE  1
#define DEF_HOPEN  2
#define DEF_HCLOSE 3

#define DEF_COUNT 4

//something non signed, non ascii
#define UNINITIALIZED_BYTE  0x11

static ieResRef Sounds[DEF_COUNT] = {
	{UNINITIALIZED_BYTE},
};

struct ResRefToStrRef {
	ieResRef areaName;
	ieStrRef text;
	bool trackFlag;
	int difficulty;
};

Holder<DataFileMgr> INInote;
ResRefToStrRef *tracks = NULL;
int trackcount = 0;

static void ReleaseMemory()
{
	INInote.release();

	delete [] tracks;
	tracks = NULL;
}

static void ReadAutonoteINI()
{
	INInote = PluginHolder<DataFileMgr>(IE_INI_CLASS_ID);
	char tINInote[_MAX_PATH];
	PathJoin( tINInote, core->GamePath, "autonote.ini", NULL );
	FileStream* fs = FileStream::OpenFile( tINInote );
	INInote->Open(fs);
}

static int GetTrackString(const ieResRef areaName)
{
	int i;
	bool trackflag = displaymsg->HasStringReference(STR_TRACKING);

	if (!tracks) {
		AutoTable tm("tracking", true);
		if (!tm.ok())
			return -1;
		trackcount = tm->GetRowCount();
		tracks = new ResRefToStrRef[trackcount];
		for (i=0;i<trackcount;i++) {
			const char *poi = tm->QueryField(i,0);
			if (poi[0]=='O' && poi[1]=='_') {
				tracks[i].trackFlag=false;
				poi+=2;
			} else {
				tracks[i].trackFlag=trackflag;
			}
			tracks[i].text=(ieStrRef) atoi(poi);
			tracks[i].difficulty=atoi(tm->QueryField(i,1));
			strnlwrcpy(tracks[i].areaName, tm->GetRowName(i), 8 );
		}
	}

	for (i=0;i<trackcount;i++) {
		if (!strnicmp(tracks[i].areaName, areaName, 8)) {
			return i;
		}
	}
	return -1;
}

AREImporter::AREImporter(void)
{
	EntrancesOffset = ContainersOffset = InfoPointsOffset = SpawnOffset = 0;
	EntrancesCount = ContainersCount = InfoPointsCount = SpawnCount = 0;
	ItemsOffset = VariablesOffset = AmbiOffset = TileOffset = TrapOffset = 0;
	ItemsCount = VariablesCount = AmbiCount = TileCount = TrapCount = 0;
	ActorCount = VerticesCount = NoteCount = 0;
	ActorOffset = VerticesOffset = NoteOffset = EffectOffset = 0;
	AreaDifficulty = AreaFlags = AreaType = 0;
	SongHeader = RestHeader = bigheader = 0;
	WRain = WSnow = WFog = WLightning = WUnknown = 0;
	EmbeddedCreOffset = AnimOffset = AnimCount = DoorsOffset = DoorsCount = 0;
	ExploredBitmapSize = ExploredBitmapOffset = 0;
	LastSave = 0;

	str = NULL;
	if (Sounds[0][0] == UNINITIALIZED_BYTE) {
		memset( Sounds, 0, sizeof( Sounds ) );
		AutoTable at("defsound");
		if (at.ok()) {
			for (int i = 0; i < DEF_COUNT; i++) {
				strncpy( Sounds[i], at->QueryField( i, 0 ), 8 );
				if(Sounds[i][0]=='*') {
					Sounds[i][0]=0;
				}
			}
		}
	}
}

AREImporter::~AREImporter(void)
{
	delete str;
	Sounds[0][0]=UNINITIALIZED_BYTE;
}

bool AREImporter::Open(DataStream* stream)
{
	if (stream == NULL) {
		return false;
	}
	delete str;
	str = stream;
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
	// unused wind speed, TODO: EEs use it for transparency
	// a single byte was re-purposed to control the alpha on the stencil water for more or less transparency.
	// If you set it to 0, then the water should be appropriately 50% transparent.
	// If you set it to any other number, it will be that transparent.
	// It's 1 byte, so setting it to 128 you'll have the same as the default of 0
	str->ReadWord( &WUnknown );

	AreaDifficulty = 0;
	if (bigheader) {
		// are9.1 difficulty bits for level2/level3
		// ar4000 for example has a bunch of actors for all area difficulty levels, so these here are likely just the allowed levels
		AreaDifficulty = 1;
		ieByte tmp = 0;
		int avgPartyLevel = core->GetGame()->GetTotalPartyLevel(false) / core->GetGame()->GetPartySize(false);
		str->Read(&tmp, 1); // 0x54
		if (tmp && avgPartyLevel >= tmp) {
			AreaDifficulty = 2;
		}
		tmp = 0;
		str->Read(&tmp, 1); // 0x55
		if (tmp && avgPartyLevel >= tmp) {
			AreaDifficulty = 4;
		}
		// 0x56 held the average party level at load time (usually 1, since it had no access yet),
		// but we resolve everything here and store AreaDifficulty instead
	}
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
	str->ReadDword( &TrapOffset );
	str->ReadDword( &TrapCount );
	str->ReadResRef( Dream1 );
	str->ReadResRef( Dream2 );
	return true;
}

//alter a map to the night/day version in case of an extended night map (bg2 specific)
//return true, if change happened, in which case a movie is played by the Game object
bool AREImporter::ChangeMap(Map *map, bool day_or_night)
{
	ieResRef TmpResRef;

	//get the right tilemap name
	if (day_or_night) {
		memcpy( TmpResRef, map->WEDResRef, 9);
	} else {
		snprintf( TmpResRef, 9, "%.7sN", map->WEDResRef);
	}
	PluginHolder<TileMapMgr> tmm(IE_WED_CLASS_ID);
	DataStream* wedfile = gamedata->GetResource( TmpResRef, IE_WED_CLASS_ID );
	tmm->Open( wedfile );
	tmm->SetExtendedNight( !day_or_night );

	//alter the tilemap object, not all parts of that object are coming from the wed/tis
	//this is why we have to be careful
	//TODO: consider refactoring TileMap so invariable data coming from the .ARE file
	//are not handled by it, then TileMap could be simply swapped
	TileMap* tm = map->GetTileMap();

	if (tm) {
		tm->ClearOverlays();
	}
	tm = tmm->GetTileMap(tm);
	if (!tm) {
		Log(ERROR, "AREImporter", "No tile map available.");
		return false;
	}

	// Small map for MapControl
	ResourceHolder<ImageMgr> sm = GetResourceHolder<ImageMgr>(TmpResRef);

	// night small map is *optional*!
	if (!sm) {
		//fall back to day minimap
		sm = GetResourceHolder<ImageMgr>(map->WEDResRef);
	}

	//the map state was altered, no need to hold this off for any later
	map->DayNight = day_or_night;

	//get the lightmap name
	if (day_or_night) {
		snprintf( TmpResRef, 9, "%.6sLM", map->WEDResRef);
	} else {
		snprintf( TmpResRef, 9, "%.6sLN", map->WEDResRef);
	}

	ResourceHolder<ImageMgr> lm = GetResourceHolder<ImageMgr>(TmpResRef);
	if (!lm) {
		Log(ERROR, "AREImporter", "No lightmap available.");
		return false;
	}

	//alter the lightmap and the minimap (the tileset was already swapped)
	map->ChangeTileMap(lm->GetImage(), sm?sm->GetSprite2D():NULL);

	// update the tiles and tilecount (eg. door0304 in Edwin's Docks (ar0300) entrance
	for (size_t i = 0; i < tm->GetDoorCount(); i++) {
		Door* door = tm->GetDoor(i);
		bool baseClosed, oldOpen = door->IsOpen();
		int count;
		unsigned short *indices = tmm->GetDoorIndices(door->ID, &count, baseClosed);
		door->SetTiles(indices, count);
		// reset open state to the one in the old wed
		door->SetDoorOpen(oldOpen, false, 0);
	}

	return true;
}

// everything is the same up to DOOR_FOUND, but then it gets messy (see Door.h)
static const ieDword gemrbDoorFlags[6] = { DOOR_TRANSPARENT, DOOR_KEY, DOOR_SLIDE, DOOR_USEUPKEY, DOOR_LOCKEDINFOTEXT, DOOR_WARNINGINFOTEXT };
// the last two are 0, since they are outside the original bit range, so all the constants can coexist
static const ieDword iwd2DoorFlags[6] = { DOOR_LOCKEDINFOTEXT, DOOR_TRANSPARENT, DOOR_WARNINGINFOTEXT, DOOR_KEY, 0, 0 };
inline ieDword FixIWD2DoorFlags(ieDword Flags, bool reverse)
{
	ieDword bit, otherbit, maskOff = 0, maskOn= 0;
	for (int i=0; i < 6; i++) {
		if (!reverse) {
			bit = gemrbDoorFlags[i];
			otherbit = iwd2DoorFlags[i];
		} else {
			bit = iwd2DoorFlags[i];
			otherbit = gemrbDoorFlags[i];
		}
		if (Flags & bit) {
			maskOff |= bit;
			maskOn |= otherbit;
		}
	}
	// delayed bad bit removal due to chain overlapping
	return (Flags & ~maskOff) | maskOn;
}

static Ambient* SetupMainAmbients(Map *map, bool day_or_night) {
	ieResRef *main1[2] = { &map->SongHeader.MainNightAmbient1, &map->SongHeader.MainDayAmbient1 };
	ieResRef *main2[2] = { &map->SongHeader.MainNightAmbient2, &map->SongHeader.MainDayAmbient2 };
	ieDword vol[2] = { map->SongHeader.MainNightAmbientVol, map->SongHeader.MainDayAmbientVol };
	ieResRef mainAmbient = "";
	if (*main1[day_or_night][0]) {
		CopyResRef(mainAmbient, *main1[day_or_night]);
	}
	// the second ambient is always longer, was meant as a memory optimisation w/ IE_AMBI_HIMEM
	// however that was implemented only for the normal ambients
	// nowadays we can just skip the first
	if (*main2[day_or_night][0]) {
		CopyResRef(mainAmbient, *main2[day_or_night]);
	}
	if (!mainAmbient[0]) return NULL;

	Ambient *ambi = new Ambient();
	ambi->flags = IE_AMBI_ENABLED | IE_AMBI_LOOPING | IE_AMBI_MAIN | IE_AMBI_NOSAVE;
	ambi->gain = vol[day_or_night];
	// sounds and name
	char *sound = (char *) malloc(9);
	memcpy(sound, mainAmbient, 9);
	ambi->sounds.push_back(sound);
	memcpy(ambi->name, sound, 9);
	ambi->appearance = (1<<25) - 1; // default to all 24 bits enabled, one per hour
	ambi->radius = 50; // REFERENCE_DISTANCE
	return ambi;
}

Map* AREImporter::GetMap(const char *ResRef, bool day_or_night)
{
	unsigned int i,x;

	// if this area does not have extended night, force it to day mode
	if (!(AreaFlags & AT_EXTENDED_NIGHT))
		day_or_night = true;

	Map* map = new Map();
	if (!map) {
		Log(ERROR, "AREImporter", "Can't allocate map (out of memory).");
		return NULL;
	}
	if (core->SaveAsOriginal) {
		map->version = bigheader;
	}

	map->AreaFlags = AreaFlags;
	map->Rain = WRain;
	map->Snow = WSnow;
	map->Fog = WFog;
	map->Lightning = WLightning;
	map->AreaType = AreaType;
	map->DayNight = day_or_night;
	map->AreaDifficulty = AreaDifficulty;
	strnlwrcpy( map->WEDResRef, WEDResRef, 8);
	strnlwrcpy( map->Dream[0], Dream1, 8);
	strnlwrcpy( map->Dream[1], Dream2, 8);

	//we have to set this here because the actors will receive their
	//current area setting here, areas' 'scriptname' is their name
	map->SetScriptName( ResRef );
	int idx = GetTrackString( ResRef );
	if (idx>=0) {
		map->SetTrackString(tracks[idx].text, tracks[idx].trackFlag, tracks[idx].difficulty);
	} else {
		map->SetTrackString((ieStrRef) -1, false, 0);
	}

	if (!core->IsAvailable( IE_WED_CLASS_ID )) {
		Log(ERROR, "AREImporter", "No tile map manager available.");
		delete map;
		return NULL;
	}
	ieResRef TmpResRef;

	if (day_or_night) {
		memcpy( TmpResRef, WEDResRef, 9);
	} else {
		snprintf( TmpResRef, 9, "%.7sN", WEDResRef);
	}

	PluginHolder<TileMapMgr> tmm(IE_WED_CLASS_ID);
	DataStream* wedfile = gamedata->GetResource( WEDResRef, IE_WED_CLASS_ID );
	tmm->Open( wedfile );

	//there was no tilemap set yet, so lets just send a NULL
	TileMap* tm = tmm->GetTileMap(NULL);
	if (!tm) {
		Log(ERROR, "AREImporter", "No tile map available.");
		delete map;
		return NULL;
	}

	// Small map for MapControl
	ResourceHolder<ImageMgr> sm = GetResourceHolder<ImageMgr>(TmpResRef);
	if (!sm) {
		//fall back to day minimap
		sm = GetResourceHolder<ImageMgr>(map->WEDResRef);
	}

	//if the Script field is empty, the area name will be copied into it on first load
	//this works only in the iwd branch of the games
	if (!Script[0] && core->HasFeature(GF_FORCE_AREA_SCRIPT) ) {
		memcpy(Script, ResRef, sizeof(ieResRef) );
	}

	if (Script[0]) {
		//for some reason the area's script is run from the last slot
		//at least one area script depends on this, if you need something
		//more customisable, add a game flag
		map->Scripts[MAX_SCRIPTS-1] = new GameScript( Script, map );
	}

	if (day_or_night) {
		snprintf( TmpResRef, 9, "%.6sLM", WEDResRef);
	} else {
		snprintf( TmpResRef, 9, "%.6sLN", WEDResRef);
	}

	ResourceHolder<ImageMgr> lm = GetResourceHolder<ImageMgr>(TmpResRef);
	if (!lm) {
		Log(ERROR, "AREImporter", "No lightmap available.");
		return NULL;
	}

	snprintf( TmpResRef, 9, "%.6sSR", WEDResRef);

	ResourceHolder<ImageMgr> sr = GetResourceHolder<ImageMgr>(TmpResRef);
	if (!sr) {
		Log(ERROR, "AREImporter", "No searchmap available.");
		return NULL;
	}

	snprintf( TmpResRef, 9, "%.6sHT", WEDResRef);

	ResourceHolder<ImageMgr> hm = GetResourceHolder<ImageMgr>(TmpResRef);
	if (!hm) {
		Log(ERROR, "AREImporter", "No heightmap available.");
		return NULL;
	}

	map->AddTileMap( tm, lm->GetImage(), sr->GetBitmap(), sm ? sm->GetSprite2D() : NULL, hm->GetBitmap() );

	Log(DEBUG, "AREImporter", "Loading songs");
	str->Seek( SongHeader, GEM_STREAM_START );
	//5 is the number of song indices
	for (i = 0; i < MAX_RESCOUNT; i++) {
		str->ReadDword( map->SongHeader.SongList + i );
	}

	str->ReadResRef(map->SongHeader.MainDayAmbient1);
	str->ReadResRef(map->SongHeader.MainDayAmbient2);
	str->ReadDword(&map->SongHeader.MainDayAmbientVol);

	str->ReadResRef(map->SongHeader.MainNightAmbient1);
	str->ReadResRef(map->SongHeader.MainNightAmbient2);
	str->ReadDword(&map->SongHeader.MainNightAmbientVol);

	// check for existence of main ambients (bg1)
	#define DAY_BITS (((1<<18) - 1) ^ ((1<<6) - 1)) // day: bits 6-18 per DLTCEP
	Ambient *ambi = SetupMainAmbients(map, true);
	if (ambi) {
		// schedule for day/night
		// if the two ambients are the same, just add one, so there's no restart
		if (memcmp(map->SongHeader.MainDayAmbient2, map->SongHeader.MainNightAmbient2, 8)) {
			ambi->appearance = DAY_BITS;
			map->AddAmbient(ambi);
			// night
			ambi = SetupMainAmbients(map, false);
			if (ambi) {
				ambi->appearance ^= DAY_BITS; // night: bits 0-5 + 19-23, [dusk till dawn]
			}
		}
		// bgt ar7300 has a nigth ambient only in the first slot
		if (ambi) {
			map->AddAmbient(ambi);
		}
	}

	if (core->HasFeature(GF_PST_STATE_FLAGS)) {
		str->ReadDword(&map->SongHeader.reverbID);
	} else {
		// all data has it at 0, so we don't bother reading
		map->SongHeader.reverbID = EFX_PROFILE_REVERB_INVALID;
	}
	map->SetupReverbInfo();

	str->Seek( RestHeader + 32, GEM_STREAM_START );
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
	str->ReadWord( &map->RestHeader.Difficulty);  //difficulty?
	str->ReadDword( &map->RestHeader.sduration);  //spawn duration
	str->ReadWord( &map->RestHeader.rwdist);      //random walk distance
	str->ReadWord( &map->RestHeader.owdist);      //other walk distance
	str->ReadWord( &map->RestHeader.Maximum);     //maximum number of creatures
	str->ReadWord( &map->RestHeader.Enabled);
	str->ReadWord( &map->RestHeader.DayChance );
	str->ReadWord( &map->RestHeader.NightChance );

	Log(DEBUG, "AREImporter", "Loading regions");
	core->LoadProgress(70);
	//Loading InfoPoints
	for (i = 0; i < InfoPointsCount; i++) {
		str->Seek( InfoPointsOffset + ( i * 0xC4 ), GEM_STREAM_START );
		ieWord Type, VertexCount;
		ieDword FirstVertex, Cursor, Flags;
		ieWord TrapDetDiff, TrapRemDiff, Trapped, TrapDetected;
		ieWord LaunchX, LaunchY;
		ieWord PosX, PosY;
		ieWord TalkX, TalkY;
		ieVariable Name, Entrance;
		ieResRef Script, KeyResRef, Destination;
		ieResRef DialogResRef, WavResRef; //adopted pst specific fields
		ieStrRef DialogName;
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
		str->ReadDword( &tmp2 ); //named triggerValue in the IE source
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
		str->ReadWord( &PosX);
		str->ReadWord( &PosY);
		/* ARE 9.1: 4B per position after that, but let's just try the lower two ones. */
		if (16 == map->version) {
			str->ReadWord(&PosX);
			str->Seek(2, GEM_CURRENT_POS);
			str->ReadWord(&PosY);
			str->Seek(30, GEM_CURRENT_POS);
		} else {
			//maybe we have to store this
			str->Seek( 36, GEM_CURRENT_POS );
		}

		if (core->HasFeature(GF_INFOPOINT_DIALOGS)) {
			str->ReadResRef( WavResRef );
			str->ReadWord( &TalkX);
			str->ReadWord( &TalkY);
			str->ReadDword( &DialogName );
			str->ReadResRef( DialogResRef );
		} else {
			memset(WavResRef, 0, sizeof(WavResRef));
			TalkX = 0;
			TalkY = 0;
			DialogName = -1;
			memset(DialogResRef, 0, sizeof(DialogResRef));
		}

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
		// translate door cursor on infopoint to correct cursor
		if (Cursor == IE_CURSOR_DOOR) Cursor = IE_CURSOR_PASS;
		ip->Cursor = Cursor;
		String* str = core->GetString( StrRef );
		ip->SetOverheadText(*str, false);
		delete str;
		ip->StrRef = StrRef; //we need this when saving area
		ip->SetMap(map);
		ip->Flags = Flags;
		ip->UsePoint.x = PosX;
		ip->UsePoint.y = PosY;
		//FIXME: PST doesn't use this field
		if (ip->GetUsePoint()) {
			ip->Pos = ip->UsePoint;
		} else {
			ip->Pos.x = bbox.x + ( bbox.w / 2 );
			ip->Pos.y = bbox.y + ( bbox.h / 2 );
		}
		memcpy( ip->Destination, Destination, sizeof(Destination) );
		memcpy( ip->EntranceName, Entrance, sizeof(Entrance) );
		memcpy( ip->KeyResRef, KeyResRef, sizeof(KeyResRef) );

		//these appear only in PST, but we could support them everywhere
		// HOWEVER they did not use them as witnessed in ar0101 (0101prt1 and 0101prt2) :(
		if (core->HasFeature(GF_PST_STATE_FLAGS)) {
			TalkX = ip->Pos.x;
			TalkY = ip->Pos.y;
		}
		ip->TalkPos.x=TalkX;
		ip->TalkPos.y=TalkY;
		ip->DialogName=DialogName;
		ip->SetDialog(DialogResRef);
		ip->SetEnter(WavResRef);

		if (Script[0]) {
			ip->Scripts[0] = new GameScript( Script, ip );
		} else {
			ip->Scripts[0] = NULL;
		}
	}

	Log(DEBUG, "AREImporter", "Loading containers");
	for (i = 0; i < ContainersCount; i++) {
		str->Seek( ContainersOffset + ( i * 0xC0 ), GEM_STREAM_START );
		ieVariable Name;
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
		ieDword firstIndex;
		ieWord vertCount, unknown;
		str->ReadDword( &firstIndex );
		//the vertex count is only 16 bits, there is a weird flag
		//after it, which is usually 0, but sometimes set to 1
		str->ReadWord( &vertCount );
		str->ReadWord( &unknown );   //trigger range
		//str->Read( Name, 32 );     //owner's scriptname
		str->Seek( 32, GEM_CURRENT_POS);
		str->ReadResRef( KeyResRef);
		str->Seek( 4, GEM_CURRENT_POS); //break difficulty
		str->ReadDword( &OpenFail );

		str->Seek( VerticesOffset + ( firstIndex * 4 ), GEM_STREAM_START );
		Point* points = ( Point* ) malloc( vertCount*sizeof( Point ) );
		for (x = 0; x < vertCount; x++) {
			ieWord tmp;
			str->ReadWord( &tmp );
			points[x].x = tmp;
			str->ReadWord( &tmp );
			points[x].y = tmp;
		}
		if (vertCount == 0 && bbox.w == 0 && bbox.h == 0) {
			/* piles have no polygons and no bounding box in some areas,
			 * but bg2 gives them this bounding box at first load,
			 * should we specifically check for Type==IE_CONTAINER_PILE? */
			bbox.x = XPos - 7;
			bbox.y = YPos - 5;
			bbox.w = 16;
			bbox.h = 12;
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

		if (Type==IE_CONTAINER_PILE)
			Script[0]=0;

		if (Script[0]) {
			c->Scripts[0] = new GameScript( Script, c );
		} else {
			c->Scripts[0] = NULL;
		}
		strnlwrcpy(c->KeyResRef, KeyResRef, 8);
		if (!OpenFail) OpenFail = (ieStrRef)-1; // rewrite 0 to -1
		c->OpenFail = OpenFail;
	}

	Log(DEBUG, "AREImporter", "Loading doors");
	for (i = 0; i < DoorsCount; i++) {
		str->Seek( DoorsOffset + ( i * 0xc8 ), GEM_STREAM_START );
		int count;
		ieDword Flags;
		ieDword OpenFirstVertex, ClosedFirstVertex;
		ieDword OpenFirstImpeded, ClosedFirstImpeded;
		ieWord OpenVerticesCount, ClosedVerticesCount;
		ieWord OpenImpededCount, ClosedImpededCount;
		ieVariable LongName, LinkedInfo;
		ieResRef ShortName;
		ieWord minX, maxX, minY, maxY;
		ieDword cursor;
		ieResRef KeyResRef, Script;
		ieWord TrapDetect, TrapRemoval;
		ieWord Trapped, TrapDetected;
		ieWord LaunchX, LaunchY;
		ieDword DiscoveryDiff, LockRemoval;
		Region BBClosed, BBOpen;
		ieStrRef OpenStrRef;
		ieStrRef NameStrRef;
		ieResRef Dialog;
		ieWord hp, ac;

		str->Read( LongName, 32 );
		LongName[32] = 0;
		str->ReadResRef( ShortName );
		str->ReadDword( &Flags );
		if (map->version == 16) {
			Flags = FixIWD2DoorFlags(Flags, false);
		}
		if (AreaType & AT_OUTDOOR) Flags |= DOOR_TRANSPARENT; // actually true only for fog-of-war, excluding other actors
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
		str->ReadWord(&hp); // hitpoints
		str->ReadWord(&ac); // AND armorclass, according to IE dev info
		ieResRef OpenResRef, CloseResRef;
		str->ReadResRef( OpenResRef );
		str->ReadResRef( CloseResRef );
		str->ReadDword( &cursor );
		str->ReadWord( &TrapDetect );
		str->ReadWord( &TrapRemoval );
		str->ReadWord( &Trapped );
		str->ReadWord( &TrapDetected );
		str->ReadWord( &LaunchX );
		str->ReadWord( &LaunchY );
		str->ReadResRef( KeyResRef );
		str->ReadResRef( Script );
		str->ReadDword( &DiscoveryDiff );
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
		if (core->HasFeature(GF_AUTOMAP_INI) ) {
			str->Read( LinkedInfo, 24);
			LinkedInfo[24] = 0; // LinkedInfo unused in pst anyway?
		} else {
			str->Read( LinkedInfo, 32);
		}
		str->ReadDword( &NameStrRef);
		str->ReadResRef( Dialog );
		if (core->HasFeature(GF_AUTOMAP_INI) ) {
			// maybe this is important? but seems not
			str->Seek( 8, GEM_CURRENT_POS );
		}

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

		tmm->SetupClosedDoor(door->closed_wg_index, door->closed_wg_count);
		tmm->SetupOpenDoor(door->open_wg_index, door->open_wg_count);

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

		door->hp = hp;
		door->ac = ac;
		door->TrapDetectionDiff = TrapDetect;
		door->TrapRemovalDiff = TrapRemoval;
		door->Trapped = Trapped;
		door->TrapDetected = TrapDetected;
		door->TrapLaunch.x = LaunchX;
		door->TrapLaunch.y = LaunchY;

		door->Cursor = cursor;
		memcpy( door->KeyResRef, KeyResRef, sizeof(KeyResRef) );
		if (Script[0]) {
			door->Scripts[0] = new GameScript( Script, door );
		} else {
			door->Scripts[0] = NULL;
		}

		door->toOpen[0] = toOpen[0];
		door->toOpen[1] = toOpen[1];
		//Leave the default sound untouched
		if (OpenResRef[0])
			memcpy( door->OpenSound, OpenResRef, sizeof(OpenResRef) );
		else {
			if (Flags & DOOR_SECRET)
				memcpy( door->OpenSound, Sounds[DEF_HOPEN], 9 );
			else
				memcpy( door->OpenSound, Sounds[DEF_OPEN], 9 );
		}
		if (CloseResRef[0])
			memcpy( door->CloseSound, CloseResRef, sizeof(CloseResRef) );
		else {
			if (Flags & DOOR_SECRET)
				memcpy( door->CloseSound, Sounds[DEF_HCLOSE], 9 );
			else
				memcpy( door->CloseSound, Sounds[DEF_CLOSE], 9 );
		}
		door->DiscoveryDiff=DiscoveryDiff;
		door->LockDifficulty=LockRemoval;
		if (!OpenStrRef) OpenStrRef = (ieStrRef)-1; // rewrite 0 to -1
		door->OpenStrRef=OpenStrRef;
		strnspccpy(door->LinkedInfo, LinkedInfo, 32);
		//these 2 fields are not sure
		door->NameStrRef=NameStrRef;
		door->SetDialog(Dialog);
	}

	Log(DEBUG, "AREImporter", "Loading spawnpoints");
	for (i = 0; i < SpawnCount; i++) {
		str->Seek( SpawnOffset + (i*0xc8), GEM_STREAM_START );
		ieVariable Name;
		ieWord XPos, YPos;
		ieWord Count, Difficulty, Frequency, Method;
		ieWord Maximum, Enabled;
		ieResRef creatures[MAX_RESCOUNT];
		ieWord DayChance, NightChance;
		ieDword Schedule;
		ieDword sduration;
		ieWord rwdist, owdist;

		str->Read( Name, 32 );
		Name[32] = 0;
		str->ReadWord( &XPos );
		str->ReadWord( &YPos );
		for (unsigned int j = 0;j < MAX_RESCOUNT; j++) {
			str->ReadResRef( creatures[j] );
		}
		str->ReadWord( &Count);
		str->ReadWord( &Difficulty);
		str->ReadWord( &Frequency );
		str->ReadWord( &Method);
		str->ReadDword( &sduration); //time to live for spawns
		str->ReadWord( &rwdist);     //random walk distance (0 is unlimited)
		str->ReadWord( &owdist);     //other walk distance (inactive in all engines?)
		str->ReadWord( &Maximum);
		str->ReadWord( &Enabled);
		str->ReadDword( &Schedule);
		str->ReadWord( &DayChance);
		str->ReadWord( &NightChance);

		Spawn *sp = map->AddSpawn(Name, XPos, YPos, creatures, Count);
		sp->Difficulty = Difficulty;
		//this value is used in a division, better make it nonzero now
		//this will fix any old gemrb saves vs. the original engine
		if (!Frequency) {
			Frequency = 1;
		}
		sp->Frequency = Frequency;
		sp->Method = Method;
		sp->sduration = sduration;
		sp->rwdist = rwdist;
		sp->owdist = owdist;
		sp->Maximum = Maximum;
		sp->Enabled = Enabled;
		sp->appearance = Schedule;
		sp->DayChance = DayChance;
		sp->NightChance = NightChance;
		//the rest is not read, we seek for every record
	}

	core->LoadProgress(75);
	Log(DEBUG, "AREImporter", "Loading actors");
	str->Seek( ActorOffset, GEM_STREAM_START );
	if (!core->IsAvailable( IE_CRE_CLASS_ID )) {
		Log(WARNING, "AREImporter", "No Actor Manager Available, skipping actors");
	} else {
		PluginHolder<ActorMgr> actmgr(IE_CRE_CLASS_ID);
		for (i = 0; i < ActorCount; i++) {
			ieVariable DefaultName;
			ieResRef CreResRef;
			ieDword TalkCount;
			ieDword Orientation, Schedule, RemovalTime;
			ieWord XPos, YPos, XDes, YDes, MaxDistance, Spawned;
			ieResRef Dialog;
			ieResRef Scripts[8]; //the original order is shown in scrlev.ids
			ieDword Flags;
			ieByte DifficultyMargin;

			str->Read( DefaultName, 32);
			DefaultName[32]=0;
			str->ReadWord( &XPos );
			str->ReadWord( &YPos );
			str->ReadWord( &XDes );
			str->ReadWord( &YDes );
			str->ReadDword( &Flags );
			str->ReadWord( &Spawned );
			str->Seek( 1, GEM_CURRENT_POS ); // one letter of a ResRef, changed to * at runtime, purpose unknown (portraits?), but not needed either
			str->Read( &DifficultyMargin, 1 ); // iwd2 only
			str->Seek( 4, GEM_CURRENT_POS ); //actor animation, unused
			str->ReadDword( &Orientation );
			str->ReadDword( &RemovalTime );
			str->ReadWord( &MaxDistance );
			str->Seek( 2, GEM_CURRENT_POS ); // apparently unused https://gibberlings3.net/forums/topic/21724-a
			str->ReadDword( &Schedule );
			str->ReadDword( &TalkCount );
			str->ReadResRef( Dialog );

			memset(Scripts, 0, sizeof(Scripts));
			str->ReadResRef( Scripts[SCR_OVERRIDE] );
			str->ReadResRef( Scripts[SCR_GENERAL] );
			str->ReadResRef( Scripts[SCR_CLASS] );
			str->ReadResRef( Scripts[SCR_RACE] );
			str->ReadResRef( Scripts[SCR_DEFAULT] );
			str->ReadResRef( Scripts[SCR_SPECIFICS] );
			str->ReadResRef( CreResRef );
			DataStream* crefile;
			Actor *ab;
			ieDword CreOffset, CreSize;
			str->ReadDword( &CreOffset );
			str->ReadDword( &CreSize );
			// another iwd2 script slot
			str->ReadResRef( Scripts[SCR_AREA] );
			str->Seek( 120, GEM_CURRENT_POS );
			//not iwd2, this field is garbage
			if (!core->HasFeature(GF_IWD2_SCRIPTNAME)) {
				Scripts[SCR_AREA][0]=0;
			}
			//actually, Flags&1 signs that the creature
			//is not loaded yet, so !(Flags&1) means it is embedded
			if (CreOffset != 0 && !(Flags&1) ) {
				crefile = SliceStream( str, CreOffset, CreSize, true );
			} else {
				crefile = gamedata->GetResource( CreResRef, IE_CRE_CLASS_ID );
			}
			if(!actmgr->Open(crefile)) {
				Log(ERROR, "AREImporter", "Couldn't read actor: %s!", CreResRef);
				continue;
			}
			ab = actmgr->GetActor(0);
			if(!ab)
				continue;
			map->AddActor(ab, false);
			ab->Pos.x = XPos;
			ab->Pos.y = YPos;
			ab->Destination.x = XPos;
			ab->Destination.y = YPos;
			ab->HomeLocation.x = XDes;
			ab->HomeLocation.y = YDes;
			ab->maxWalkDistance = MaxDistance;
			ab->Spawned = Spawned;
			ab->appearance = Schedule;
			//copying the scripting name into the actor
			//if the CreatureAreaFlag was set to 8
			if ((Flags&AF_NAME_OVERRIDE) || (core->HasFeature(GF_IWD2_SCRIPTNAME)) ) {
				ab->SetScriptName(DefaultName);
			}
			//IWD2 specific hacks
			if (core->HasFeature(GF_3ED_RULES)) {
				//This flag is used for something else in IWD2
				if (Flags&AF_NAME_OVERRIDE) {
					ab->BaseStats[IE_EA]=EA_EVILCUTOFF;
				}
				if (Flags&AF_SEEN_PARTY) {
					ab->SetMCFlag(MC_SEENPARTY,OP_OR);
				}
				if (Flags&AF_INVULNERABLE) {
					ab->SetMCFlag(MC_INVULNERABLE,OP_OR);
				}
				if (!(Flags&AF_ENABLED)) {
					// DifficultyMargin - only enable actors that are difficult enough vs the area difficulty
					// 1 - area difficulty 1
					// 2 - area difficulty 2
					// 4 - area difficulty 3
					if (DifficultyMargin && !(DifficultyMargin & map->AreaDifficulty)) {
						ab->DestroySelf();
					}
				}
			}
			ab->DifficultyMargin = DifficultyMargin;

			if (Dialog[0]) {
				ab->SetDialog(Dialog);
			}
			for (int j=0;j<8;j++) {
				if (Scripts[j][0]) {
					ab->SetScript(Scripts[j],j);
				}
			}
			ab->SetOrientation( Orientation,0 );
			ab->TalkCount = TalkCount;
			ab->RemovalTime = RemovalTime;
			ab->RefreshEffects(NULL);
		}
	}

	int pst = core->HasFeature( GF_AUTOMAP_INI );

	core->LoadProgress(90);
	Log(DEBUG, "AREImporter", "Loading animations");
	str->Seek( AnimOffset, GEM_STREAM_START );
	if (!core->IsAvailable( IE_BAM_CLASS_ID )) {
		Log(WARNING, "AREImporter", "No Animation Manager Available, skipping animations");
	} else {
		for (i = 0; i < AnimCount; i++) {
			AreaAnimation* anim = new AreaAnimation();
			str->Read(anim->Name, 32);
			ieWord animX, animY, startFrameRange;
			str->ReadWord( &animX );
			str->ReadWord( &animY );
			anim->Pos.x=animX;
			anim->Pos.y=animY;
			str->ReadDword( &anim->appearance );
			str->ReadResRef( anim->BAM );
			str->ReadWord( &anim->sequence );
			str->ReadWord( &anim->frame );
			str->ReadDword( &anim->Flags );
			anim->originalFlags = anim->Flags;
			str->ReadWordSigned( &anim->height );
			str->ReadWord( &anim->transparency );
			str->ReadWord( &startFrameRange );
			str->Read( &anim->startchance,1 );
			if (anim->startchance<=0) {
				anim->startchance=100; //percentage of starting a cycle
			}
			if (startFrameRange && (anim->Flags&A_ANI_RANDOM_START) ) {
				anim->frame = RAND(0, startFrameRange - 1);
			}
			anim->startFrameRange = 0; //this will never get resaved (iirc)
			str->Read( &anim->skipcycle,1 ); //how many cycles are skipped	(100% skippage)
			str->ReadResRef( anim->PaletteRef );
			// TODO: EE: word with anim width for PVRZ/WBM resources (if flag bits are set, see A_ANI_ defines)
			// 0x4a holds the height
			str->ReadDword( &anim->unknown48 );

			if (pst) {
				AdjustPSTFlags(anim);
			}

			//set up the animation, it cannot be done here
			//because a StaticSequence action can change
			//it later
			map->AddAnimation( anim );
			//the animation was safely transferred to internal memory
			delete anim;
		}
	}

	Log(DEBUG, "AREImporter", "Loading entrances");
	str->Seek( EntrancesOffset, GEM_STREAM_START );
	for (i = 0; i < EntrancesCount; i++) {
		ieVariable Name;
		ieWord XPos, YPos, Face;
		str->Read( Name, 32 );
		Name[32] = 0;
		str->ReadWord( &XPos );
		str->ReadWord( &YPos );
		str->ReadWord( &Face );
		str->Seek( 66, GEM_CURRENT_POS );
		map->AddEntrance( Name, XPos, YPos, Face );
	}

	Log(DEBUG, "AREImporter", "Loading variables");
	map->locals->LoadInitialValues(ResRef);
	str->Seek( VariablesOffset, GEM_STREAM_START );
	for (i = 0; i < VariablesCount; i++) {
		ieVariable Name;
		ieDword Value;
		str->Read( Name, 32 );
		Name[32] = 0;
		str->Seek( 8, GEM_CURRENT_POS );
		str->ReadDword( &Value );
		str->Seek( 40, GEM_CURRENT_POS );
		map->locals->SetAt( Name, Value );
	}

	Log(DEBUG, "AREImporter", "Loading ambients");
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
		str->Seek( 2, GEM_CURRENT_POS );
		str->ReadDword( &ambi->pitchVariance );
		str->ReadWord( &ambi->gainVariance );
		str->ReadWord( &ambi->gain );
		for (j = 0;j < MAX_RESCOUNT; j++) {
			str->ReadResRef( sounds[j] );
		}
		str->ReadWord( &tmpWord );
		str->Seek( 2, GEM_CURRENT_POS );
		str->ReadDword( &ambi->interval );
		str->ReadDword( &ambi->intervalVariance );
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

	Log(DEBUG, "AREImporter", "Loading automap notes");
	str->Seek( NoteOffset, GEM_STREAM_START );

	Point point;
	ieDword color = 0;

	//Don't bother with autonote.ini if the area has autonotes (ie. it is a saved area)
	if (pst && !NoteCount) {
		if( !INInote ) {
			ReadAutonoteINI();
		}
		//add autonote.ini entries
		if( INInote ) {
			color = 1; //read only note
			const char *scriptName = map->GetScriptName();
			int count = INInote->GetKeyAsInt(scriptName, "count", 0);
			while (count) {
				char key[32];
				int value;
				snprintf(key, sizeof(key), "xPos%d",count);
				value = INInote->GetKeyAsInt(scriptName, key, 0);
				point.x = value;
				snprintf(key, sizeof(key), "yPos%d",count);
				value = INInote->GetKeyAsInt(scriptName, key, 0);
				point.y = value;
				snprintf(key, sizeof(key), "text%d",count);
				value = INInote->GetKeyAsInt(scriptName, key, 0);
				map->AddMapNote( point, color, value);
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

			char bytes[501]; // 500 + null
			str->Read(bytes, 500 );
			bytes[500] = '\0';
			String* text = StringFromCString(bytes);
			str->ReadDword(&color); //readonly == 1
			map->AddMapNote(point, color, text);
			str->Seek(20, GEM_CURRENT_POS);
		} else {
			ieWord px,py;

			str->ReadWord( &px );
			str->ReadWord( &py );
			point.x=px;
			point.y=py;
			ieStrRef strref = 0;
			str->ReadDword( &strref );
			str->ReadWord( &px );
			str->ReadWord( &py );
			color=py;
			str->Seek( 40, GEM_CURRENT_POS );
			map->AddMapNote( point, color, strref );
		}
	}

	//this is a ToB feature (saves the unexploded projectiles)
	Log(DEBUG, "AREImporter", "Loading traps");
	for (i = 0; i < TrapCount; i++) {
		ieResRef TrapResRef;
		ieDword TrapEffOffset;
		ieWord TrapSize, ProID;
		ieWord X,Y,Z;
		ieDword Ticks;
		ieByte TargetType;
		ieByte Owner;

		str->Seek( TrapOffset + ( i * 0x1c ), GEM_STREAM_START );

		str->ReadResRef( TrapResRef );
		str->ReadDword( &TrapEffOffset );
		str->ReadWord( &TrapSize );
		str->ReadWord( &ProID );
		str->ReadDword( &Ticks );  //actually, delaycount/repetitioncount
		str->ReadWord( &X );
		str->ReadWord( &Y );
		str->ReadWord( &Z );
		str->Read(&TargetType, 1); //according to dev info, this is 'targettype'; "Enemy-ally targetting" on IESDP
		str->Read(&Owner, 1); // party member index that created this projectile (0-5)
		int TrapEffectCount = TrapSize/0x108;
		if(TrapEffectCount*0x108!=TrapSize) {
			Log(ERROR, "AREImporter", "TrapEffectSize in game: %d != %d. Clearing it",
				TrapSize, TrapEffectCount*0x108);
				continue;
		}
		//The projectile is always created, the worst that can happen
		//is a dummy projectile
		//The projectile ID is 214 for TRAPSNAR
		//It is off by one compared to projectl.ids, but the same as missile.ids
		Projectile *pro = core->GetProjectileServer()->GetProjectileByIndex(ProID-1);

		//This could be wrong on msvc7 with its separate memory managers
		EffectQueue *fxqueue = new EffectQueue();
		DataStream *fs = new SlicedStream( str, TrapEffOffset, TrapSize);

		ReadEffects(fs, fxqueue, TrapEffectCount);
		Actor * caster = core->GetGame()->FindPC(Owner + 1);
		pro->SetEffects(fxqueue);
		if (caster) {
			// Since the level info isn't stored, we assume it's the same as if the trap was just placed.
			// It matters for the normal thief traps (they scale with level 4 times), while the rest don't scale.
			// To be more flexible and better handle disabled dualclasses, we don't hardcode it to the thief level.
			// Perhaps simplify and store the level in Z? Would need a check in the original (don't break saves).
			ieDword level = caster->GetThiefLevel();
			pro->SetCaster(caster->GetGlobalID(), level ? level : caster->GetXPLevel(false));
		}
		Point pos(X,Y);
		map->AddProjectile( pro, pos, pos);
	}

	Log(DEBUG, "AREImporter", "Loading tiles");
	//Loading Tiled objects (if any)
	str->Seek( TileOffset, GEM_STREAM_START );
	for (i = 0; i < TileCount; i++) {
		ieVariable Name;
		ieResRef ID;
		ieDword Flags;
		// these fields could be different size: ieDword ClosedCount, OpenCount;
		ieWord ClosedCount, OpenCount;
		ieDword ClosedIndex, OpenIndex;
		str->Read( Name, 32 );
		Name[32] = 0;
		str->ReadResRef( ID );
		str->ReadDword( &Flags );
		//IE dev info says this:
		str->ReadDword( &OpenIndex );
		str->ReadWord( &OpenCount );
		str->ReadWord( &ClosedCount );
		str->ReadDword( &ClosedIndex );
		//end of disputed section

		str->Seek( 48, GEM_CURRENT_POS );
		//absolutely no idea where these 'tile indices' are stored
		//are they tileset tiles or impeded block tiles
		map->TMap->AddTile( ID, Name, Flags, NULL,0, NULL, 0 );
	}

	Log(DEBUG, "AREImporter", "Loading explored bitmap");
	i = map->GetExploredMapSize();
	if (ExploredBitmapSize==i) {
		map->ExploredBitmap = (ieByte *) malloc(i);
		str->Seek( ExploredBitmapOffset, GEM_STREAM_START );
		str->Read( map->ExploredBitmap, i );
	}
	else {
		if( ExploredBitmapSize ) {
			Log(ERROR, "AREImporter", "ExploredBitmapSize in game: %d != %d. Clearing it",
				ExploredBitmapSize, i);
		}
		ExploredBitmapSize = i;
		map->ExploredBitmap = (ieByte *) calloc(i, 1);
	}
	map->VisibleBitmap = (ieByte *) calloc(i, 1);

	Log(DEBUG, "AREImporter", "Loading wallgroups");
	map->SetWallGroups( tmm->GetPolygonsCount(),tmm->GetWallGroups() );
	//setting up doors
	for (i=0;i<DoorsCount;i++) {
		Door *door = tm->GetDoor(i);
		door->SetDoorOpen(door->IsOpen(), false, 0);
	}

	return map;
}

void AREImporter::AdjustPSTFlags(AreaAnimation *areaAnim) {
	/**
	 * For PST, map animation flags work differently to a degree that they
	 * should not be mixed together with the rest as they even tend to
	 * break things (like stopping early, hiding under FoW).
	 *
	 * So far, a better approximation towards handling animations is:
	 * - always set A_ANI_BLEND, A_ANI_SYNC,
	 * - unset A_ANI_PLAYONCE, A_ANI_NOT_IN_FOG, A_ANI_BACKGROUND.
	 *
	 * The actual use of bits in PST map anims isn't fully solved here.
	 */
	areaAnim->Flags |= (A_ANI_BLEND | A_ANI_SYNC);
	areaAnim->Flags &= ~(A_ANI_PLAYONCE | A_ANI_NOT_IN_FOG | A_ANI_BACKGROUND);
}

void AREImporter::ReadEffects(DataStream *ds, EffectQueue *fxqueue, ieDword EffectsCount)
{
	unsigned int i;

	PluginHolder<EffectMgr> eM(IE_EFF_CLASS_ID);
	eM->Open(ds);

	for (i = 0; i < EffectsCount; i++) {
		Effect fx;

		eM->GetEffectV20( &fx );
		// NOTE: AddEffect() allocates a new effect
		fxqueue->AddEffect( &fx );
	}
}

int AREImporter::GetStoredFileSize(Map *map)
{
	unsigned int i;
	int headersize = map->version+0x11c;
	ActorOffset = headersize;

	//get only saved actors (no familiars or partymembers)
	//summons?
	ActorCount = (ieWord) map->GetActorCount(false);
	headersize += ActorCount * 0x110;

	PluginHolder<ActorMgr> am(IE_CRE_CLASS_ID);
	EmbeddedCreOffset = headersize;

	for (i=0;i<ActorCount;i++) {
		headersize += am->GetStoredFileSize(map->GetActor(i, false) );
	}

	InfoPointsOffset = headersize;

	InfoPointsCount = (ieWord) map->TMap->GetInfoPointCount();
	headersize += InfoPointsCount * 0xc4;
	SpawnOffset = headersize;

	SpawnCount = map->GetSpawnCount();
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

	AmbiCount = map->GetAmbientCount(true);
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
	EffectOffset = headersize;

	TrapCount = (ieDword) map->GetTrapCount(piter);
	for(i=0;i<TrapCount;i++) {
		const Projectile *pro = map->GetNextTrap(piter);
		if (pro) {
			EffectQueue *fxqueue = pro->GetEffects();
			if (fxqueue) {
				headersize += fxqueue->GetSavedEffectsCount() * 0x108;
			}
		}
	}

	TrapOffset = headersize;
	headersize += TrapCount * 0x1c;
	NoteOffset = headersize;

	int pst = core->HasFeature( GF_AUTOMAP_INI );
	NoteCount = map->GetMapNoteCount();
	headersize += NoteCount * (pst?0x214: 0x34);
	SongHeader = headersize;

	headersize += 0x90;
	RestHeader = headersize;

	headersize += 0xe4;
	return headersize;
}

int AREImporter::PutHeader(DataStream *stream, const Map *map)
{
	char Signature[56];
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

	if (map->version == 16) { //writing 14 bytes of 0's
		char tmp[1] = { '0' };
		if (map->AreaDifficulty == 2) {
			tmp[0] = 1;
		}
		stream->Write( tmp, 1);
		tmp[0] = 0;
		if (map->AreaDifficulty == 4) {
			tmp[0] = 1;
		}
		stream->Write( tmp, 1);
		stream->Write( Signature, 6);
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

	//the saved area script is in the last script slot!
	const GameScript *s = map->Scripts[MAX_SCRIPTS - 1];
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
		i=52;
	} else {
		i=56;
	}
	stream->WriteDword( &NoteOffset );
	stream->WriteDword( &NoteCount );
	stream->WriteDword( &TrapOffset );
	stream->WriteDword( &TrapCount );
	stream->WriteResRef( map->Dream[0] );
	stream->WriteResRef( map->Dream[1] );
	//usually 56 empty bytes (but pst used up 4 elsewhere)
	stream->Write( Signature, i);
	return 0;
}

int AREImporter::PutDoors(DataStream *stream, const Map *map, ieDword &VertIndex)
{
	char filling[8];
	ieWord tmpWord = 0;

	memset(filling,0,sizeof(filling) );
	for (unsigned int i=0;i<DoorsCount;i++) {
		Door *d = map->TMap->GetDoor(i);

		stream->Write( d->GetScriptName(), 32);
		stream->WriteResRef( d->ID);
		if (map->version == 16) {
			d->Flags = FixIWD2DoorFlags(d->Flags, true);
		}
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
		stream->WriteWord( &d->hp);
		stream->WriteWord( &d->ac);
		stream->WriteResRef( d->OpenSound);
		stream->WriteResRef( d->CloseSound);
		stream->WriteDword( &d->Cursor);
		stream->WriteWord( &d->TrapDetectionDiff);
		stream->WriteWord( &d->TrapRemovalDiff);
		stream->WriteWord( &d->Trapped);
		stream->WriteWord( &d->TrapDetected);
		tmpWord = (ieWord) d->TrapLaunch.x;
		stream->WriteWord( &tmpWord);
		tmpWord = (ieWord) d->TrapLaunch.y;
		stream->WriteWord( &tmpWord);
		stream->WriteResRef( d->KeyResRef);
		const GameScript *s = d->Scripts[0];
		if (s) {
			stream->WriteResRef( s->GetName() );
		} else {
			stream->Write( filling, 8);
		}
		stream->WriteDword( &d->DiscoveryDiff);
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
		if (core->HasFeature(GF_AUTOMAP_INI) ) {
			stream->Write( d->LinkedInfo, 24);
		} else {
			stream->Write( d->LinkedInfo, 32);
		}
		stream->WriteDword( &d->NameStrRef);
		stream->WriteResRef( d->GetDialog());
		if (core->HasFeature(GF_AUTOMAP_INI) ) {
			stream->Write( filling, 8);
		}
	}
	return 0;
}

int AREImporter::PutPoints(DataStream *stream, const Point *p, unsigned int count)
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

int AREImporter::PutVertices(DataStream *stream, const Map *map)
{
	unsigned int i;

	//regions
	for(i=0;i<InfoPointsCount;i++) {
		InfoPoint *ip = map->TMap->GetInfoPoint(i);
		PutPoints(stream, ip->outline->points, ip->outline->count);
	}
	//containers
	for(i=0;i<ContainersCount;i++) {
		const Container *c = map->TMap->GetContainer(i);
		PutPoints(stream, c->outline->points, c->outline->count);
	}
	//doors
	for(i=0;i<DoorsCount;i++) {
		const Door *d = map->TMap->GetDoor(i);
		PutPoints(stream, d->open->points, d->open->count);
		PutPoints(stream, d->closed->points, d->closed->count);
		PutPoints(stream, d->open_ib, d->oibcount);
		PutPoints(stream, d->closed_ib, d->cibcount);
	}
	return 0;
}

int AREImporter::PutItems(DataStream *stream, const Map *map)
{
	for (unsigned int i=0;i<ContainersCount;i++) {
		const Container *c = map->TMap->GetContainer(i);

		for(int j=0;j<c->inventory.GetSlotCount();j++) {
			const CREItem *ci = c->inventory.GetSlotItem(j);

			stream->WriteResRef( ci->ItemResRef);
			stream->WriteWord( &ci->Expired);
			stream->WriteWord( &ci->Usages[0]);
			stream->WriteWord( &ci->Usages[1]);
			stream->WriteWord( &ci->Usages[2]);
			stream->WriteDword( &ci->Flags);
		}
	}
	return 0;
}

int AREImporter::PutContainers(DataStream *stream, const Map *map, ieDword &VertIndex)
{
	char filling[56];
	ieDword ItemIndex = 0;
	ieDword tmpDword;
	ieWord tmpWord;

	memset(filling,0,sizeof(filling) );
	for (unsigned int i=0;i<ContainersCount;i++) {
		const Container *c = map->TMap->GetContainer(i);

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
		const GameScript *s = c->Scripts[0];
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

int AREImporter::PutRegions(DataStream *stream, const Map *map, ieDword &VertIndex)
{
	ieDword tmpDword = 0;
	ieWord tmpWord;
	char filling[36];

	memset(filling,0,sizeof(filling) );
	for (unsigned int i=0;i<InfoPointsCount;i++) {
		const InfoPoint *ip = map->TMap->GetInfoPoint(i);

		stream->Write( ip->GetScriptName(), 32);
		//this is a hack, we abuse a coincidence
		//ST_PROXIMITY = 1, ST_TRIGGER = 2, ST_TRAVEL = 3
		//translates to trap = 0, info = 1, travel = 2
		tmpWord = ((ieWord) ip->Type) - 1;
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
		const GameScript *s = ip->Scripts[0];
		if (s) {
			stream->WriteResRef( s->GetName() );
		} else {
			stream->Write( filling, 8);
		}
		tmpWord = (ieWord) ip->UsePoint.x;
		ieDword tmpDword2 = ip->UsePoint.x;
		stream->WriteWord( &tmpWord);
		tmpWord = (ieWord) ip->UsePoint.y;
		tmpDword = ip->UsePoint.y;
		stream->WriteWord( &tmpWord);
		if (16 == map->version) {
			stream->WriteDword(&tmpDword2);
			stream->WriteDword(&tmpDword);
			stream->Write(filling, 28); //unknown
		} else {
			stream->Write(filling, 36); //unknown
		}
		//these are probably only in PST
		stream->WriteResRef( ip->EnterWav);
		tmpWord = (ieWord) ip->TalkPos.x;
		stream->WriteWord( &tmpWord);
		tmpWord = (ieWord) ip->TalkPos.y;
		stream->WriteWord( &tmpWord);
		stream->WriteDword( &ip->DialogName);
		stream->WriteResRef( ip->GetDialog());
	}
	return 0;
}

int AREImporter::PutSpawns(DataStream *stream, const Map *map)
{
	ieWord tmpWord;
	char filling[56];

	memset(filling,0,sizeof(filling) );
	for (unsigned int i=0;i<SpawnCount;i++) {
		const Spawn *sp = map->GetSpawn(i);

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
		stream->WriteWord( &sp->Difficulty);
		stream->WriteWord( &sp->Frequency);
		stream->WriteWord( &sp->Method);
		stream->WriteDword( &sp->sduration); //spawn duration
		stream->WriteWord( &sp->rwdist);     //random walk distance
		stream->WriteWord( &sp->owdist);     //other walk distance
		stream->WriteWord( &sp->Maximum);
		stream->WriteWord( &sp->Enabled);
		stream->WriteDword( &sp->appearance);
		stream->WriteWord( &sp->DayChance);
		stream->WriteWord( &sp->NightChance);
		stream->Write( filling, 56); //most likely unused crap
	}
	return 0;
}

void AREImporter::PutScript(DataStream *stream, const Actor *ac, unsigned int index)
{
	char filling[8];

	const GameScript *s = ac->Scripts[index];
	if (s) {
		stream->WriteResRef( s->GetName() );
	} else {
		memset(filling,0,sizeof(filling));
		stream->Write( filling, 8);
	}
}

int AREImporter::PutActors(DataStream *stream, const Map *map)
{
	ieDword tmpDword = 0;
	ieWord tmpWord;
	ieByte tmpByte;
	ieDword CreatureOffset = EmbeddedCreOffset;
	char filling[120];
	unsigned int i;

	PluginHolder<ActorMgr> am(IE_CRE_CLASS_ID);
	memset(filling,0,sizeof(filling) );
	for (i=0;i<ActorCount;i++) {
		Actor *ac = map->GetActor(i, false);

		stream->Write( ac->GetScriptName(), 32);
		tmpWord = (ieWord) ac->Pos.x;
		stream->WriteWord( &tmpWord);
		tmpWord = (ieWord) ac->Pos.y;
		stream->WriteWord( &tmpWord);
		tmpWord = (ieWord) ac->HomeLocation.x;
		stream->WriteWord( &tmpWord);
		tmpWord = (ieWord) ac->HomeLocation.y;
		stream->WriteWord( &tmpWord);

		stream->WriteDword( &tmpDword); //used fields flag always 0 for saved areas
		tmpWord = ac->Spawned;
		stream->WriteWord( &tmpWord);
		stream->Write(filling, 1); // letter
		tmpByte = ac->DifficultyMargin;
		stream->Write( &tmpByte, 1 );
		stream->WriteDword( &tmpDword); //actor animation, unused
		tmpWord = ac->GetOrientation();
		stream->WriteWord( &tmpWord);
		tmpWord = 0;
		stream->WriteWord( &tmpWord); //unknown
		stream->WriteDword( &ac->RemovalTime);
		stream->WriteWord( &ac->maxWalkDistance);
		stream->WriteWord( &tmpWord); //more unknowns
		stream->WriteDword( &ac->appearance);
		stream->WriteDword( &ac->TalkCount);
		stream->WriteResRef( ac->GetDialog());
		PutScript(stream, ac, SCR_OVERRIDE);
		PutScript(stream, ac, SCR_GENERAL);
		PutScript(stream, ac, SCR_CLASS);
		PutScript(stream, ac, SCR_RACE);
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
		assert(stream->GetPos() == CreatureOffset);
		Actor *ac = map->GetActor(i, false);

		//reconstructing offsets again
		CreatureOffset += am->GetStoredFileSize(ac);
		am->PutActor( stream, ac);
	}
	assert(stream->GetPos() == CreatureOffset);

	return 0;
}

int AREImporter::PutAnimations(DataStream *stream, const Map *map)
{
	ieWord tmpWord;

	aniIterator iter = map->GetFirstAnimation();
	while(const AreaAnimation *an = map->GetNextAnimation(iter)) {
		stream->Write( an->Name, 32);
		tmpWord = (ieWord) an->Pos.x;
		stream->WriteWord( &tmpWord);
		tmpWord = (ieWord) an->Pos.y;
		stream->WriteWord( &tmpWord);
		stream->WriteDword( &an->appearance);
		stream->WriteResRef( an->BAM);
		stream->WriteWord( &an->sequence);
		stream->WriteWord( &an->frame);

		if (core->HasFeature(GF_AUTOMAP_INI)) {
			/* PST toggles the active bit only, and we need to keep the rest. */
			ieDword flags = (an->originalFlags & ~A_ANI_ACTIVE) | (an->Flags & A_ANI_ACTIVE);
			stream->WriteDword(&flags);
		} else {
			stream->WriteDword(&an->Flags);
		}

		stream->WriteWord((const ieWord *) &an->height);
		stream->WriteWord( &an->transparency);
		stream->WriteWord( &an->startFrameRange); //used by A_ANI_RANDOM_START
		stream->Write( &an->startchance,1);
		stream->Write( &an->skipcycle,1);
		stream->WriteResRef( an->PaletteRef);
		stream->WriteDword( &an->unknown48);//seems utterly unused
	}
	return 0;
}

int AREImporter::PutEntrances(DataStream *stream, const Map *map)
{
	ieWord tmpWord;
	char filling[66];

	memset(filling,0,sizeof(filling) );
	for (unsigned int i=0;i<EntrancesCount;i++) {
		const Entrance *e = map->GetEntrance(i);

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

int AREImporter::PutVariables(DataStream *stream, const Map *map)
{
	char filling[40];
	Variables::iterator pos=NULL;
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

int AREImporter::PutAmbients(DataStream *stream, const Map *map)
{
	char filling[64];
	ieWord tmpWord;

	memset(filling,0,sizeof(filling) );
	ieWord realCount = map->GetAmbientCount();
	for (ieWord i = 0; i < realCount; i++) {
		const Ambient *am = map->GetAmbient(i);
		if (am->flags & IE_AMBI_NOSAVE) continue;
		stream->Write( am->name, 32 );
		tmpWord = (ieWord) am->origin.x;
		stream->WriteWord( &tmpWord );
		tmpWord = (ieWord) am->origin.y;
		stream->WriteWord( &tmpWord );
		stream->WriteWord( &am->radius );
		stream->Write( filling, 2 );
		stream->WriteDword( &am->pitchVariance );
		stream->WriteWord( &am->gainVariance );
		stream->WriteWord( &am->gain );
		tmpWord = (ieWord) am->sounds.size();
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
		stream->WriteDword( &am->intervalVariance );
		stream->WriteDword( &am->appearance );
		stream->WriteDword( &am->flags );
		stream->Write( filling, 64);
	}
	return 0;
}

int AREImporter::PutMapnotes(DataStream *stream, const Map *map)
{
	char filling[8];
	ieDword tmpDword;
	ieWord tmpWord;

	//different format
	int pst = core->HasFeature( GF_AUTOMAP_INI );

	memset(filling,0,sizeof(filling) );
	for (unsigned int i=0;i<NoteCount;i++) {
		const MapNote& mn = map->GetMapNote(i);
		int x;

		if (pst) {
			tmpDword = (ieDword) mn.Pos.x;
			stream->WriteDword( &tmpDword );
			tmpDword = (ieDword) mn.Pos.y;
			stream->WriteDword( &tmpDword );
			int len = 0;
			if (mn.text) {
				// limited to 500 *bytes* of text, convert to a multibyte encoding.
				// we convert to MB because it fits more than if we wrote the wide characters
				char* mbstring = MBCStringFromString(*mn.text);
				// FIXME: depends on locale blah blah (see MBCStringFromString definition)
				if (mbstring) {
					// only care about number of bytes before null so strlen is what we want despite being MB string
					len = (std::min)(static_cast<int>(strlen(mbstring)), 500);
					stream->Write( mbstring, len);
					free(mbstring);
				} else {
					Log(WARNING, "AREImporter", "MapNote converted to an invalid multibyte sequence; cannot write it to file.\nFailed Note: %ls", mn.text->c_str());
				}
			}

			// pad the remaining space
			x = 500 - len;
			for (int j=0;j<x/8;j++) {
				stream->Write( filling, 8);
			}
			x = x%8;
			if (x) {
				stream->Write( filling, x);
			}
			tmpDword = (ieDword) mn.color;
			stream->WriteDword(&tmpDword);
			for (x=0;x<5;x++) { //5 empty dwords
				stream->Write( filling, 4);
			}
		} else {
			tmpWord = (ieWord) mn.Pos.x;
			stream->WriteWord( &tmpWord );
			tmpWord = (ieWord) mn.Pos.y;
			stream->WriteWord( &tmpWord );
			stream->WriteDword( &mn.strref);
			stream->WriteWord( &tmpWord );
			stream->WriteWord( &mn.color );
			tmpDword = 1;
			stream->WriteDword( &tmpDword );
			for (x=0;x<9;x++) { //9 empty dwords
				stream->Write( filling, 4);
			}
		}
	}
	return 0;
}

int AREImporter::PutEffects(DataStream *stream, const EffectQueue *fxqueue)
{
	PluginHolder<EffectMgr> eM(IE_EFF_CLASS_ID);
	assert(eM != nullptr);

	std::list< Effect* >::const_iterator f=fxqueue->GetFirstEffect();
	ieDword EffectsCount = fxqueue->GetSavedEffectsCount();
	for(unsigned int i=0;i<EffectsCount;i++) {
		const Effect *fx = fxqueue->GetNextSavedEffect(f);

		assert(fx!=NULL);

		eM->PutEffectV2(stream, fx);
	}
	return 0;
}

int AREImporter::PutTraps( DataStream *stream, const Map *map)
{
	ieDword Offset;
	ieDword tmpDword;
	ieResRef name;
	ieWord type = 0;
	Point dest(0,0);

	Offset = EffectOffset;
	ieDword i = map->GetTrapCount(piter);
	while(i--) {
		ieWord tmpWord = 0;
		ieByte tmpByte = 0xff;
		Projectile *pro = map->GetNextTrap(piter);
		if (pro) {
			//The projectile ID is based on missile.ids which is
			//off by one compared to projectl.ids
			type = pro->GetType()+1;
			dest = pro->GetDestination();
			strnuprcpy(name, pro->GetName(), 8);
			const EffectQueue *fxqueue = pro->GetEffects();
			if (fxqueue) {
				tmpWord = fxqueue->GetSavedEffectsCount();
			}
			ieDword ID = pro->GetCaster();
			// lookup caster via Game, since the the current map can already be empty when switching them
			const Actor *actor = core->GetGame()->GetActorByGlobalID(ID);
			//0xff if not in party
			//party slot if in party
			if (actor) tmpByte = (ieByte) (actor->InParty-1);
		}

		stream->WriteResRef( name );
		stream->WriteDword( &Offset );
		//size of fxqueue;
		assert(tmpWord<256);
		tmpWord *= 0x108;
		Offset += tmpWord;
		stream->WriteWord( &tmpWord );  //size in bytes
		stream->WriteWord( &type );     //missile.ids
		tmpDword = 0;
		stream->WriteDword(&tmpDword); // unknown field, Ticks
		tmpWord = (ieWord) dest.x;
		stream->WriteWord( &tmpWord );
		tmpWord = (ieWord) dest.y;
		stream->WriteWord( &tmpWord );
		tmpWord = 0;
		stream->WriteWord(&tmpWord); // unknown field, Z
		stream->Write(&tmpByte, 1);   // unknown field, TargetType
		stream->Write(&tmpByte, 1);   // Owner
	}
	return 0;
}

int AREImporter::PutExplored(DataStream *stream, const Map *map)
{
	stream->Write( map->ExploredBitmap, ExploredBitmapSize);
	return 0;
}

int AREImporter::PutTiles(DataStream *stream, const Map *map)
{
	char filling[48];
	ieDword tmpDword = 0;

	memset(filling,0,sizeof(filling) );
	for (unsigned int i=0;i<TileCount;i++) {
		const TileObject *am = map->TMap->GetTile(i);
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

int AREImporter::PutSongHeader(DataStream *stream, const Map *map)
{
	int i;
	char filling[8];
	ieDword tmpDword = 0;

	memset(filling,0,sizeof(filling) );
	for(i=0;i<MAX_RESCOUNT;i++) {
		stream->WriteDword( &map->SongHeader.SongList[i]);
	}
	//day
	stream->WriteResRef(map->SongHeader.MainDayAmbient1);
	stream->WriteResRef(map->SongHeader.MainDayAmbient2);
	stream->WriteDword(&map->SongHeader.MainDayAmbientVol);
	//night
	stream->WriteResRef(map->SongHeader.MainNightAmbient1);
	stream->WriteResRef(map->SongHeader.MainNightAmbient2);
	stream->WriteDword(&map->SongHeader.MainNightAmbientVol);
	//song flag
	stream->WriteDword(&map->SongHeader.reverbID);
	//lots of empty crap (15x4)
	for(i=0;i<15;i++) {
		stream->WriteDword( &tmpDword);
	}
	return 0;
}

int AREImporter::PutRestHeader(DataStream *stream, const Map *map)
{
	int i;
	ieDword tmpDword = 0;

	char filling[32];
	memset(filling,0,sizeof(filling) );
	stream->Write( filling, 32); //empty label
	for(i=0;i<MAX_RESCOUNT;i++) {
		stream->WriteDword( &map->RestHeader.Strref[i]);
	}
	for(i=0;i<MAX_RESCOUNT;i++) {
		stream->WriteResRef( map->RestHeader.CreResRef[i]);
	}
	stream->WriteWord( &map->RestHeader.CreatureNum);
	stream->WriteWord( &map->RestHeader.Difficulty);
	stream->WriteDword( &map->RestHeader.sduration);
	stream->WriteWord( &map->RestHeader.rwdist);
	stream->WriteWord( &map->RestHeader.owdist);
	stream->WriteWord( &map->RestHeader.Maximum);
	stream->WriteWord( &map->RestHeader.Enabled);
	stream->WriteWord( &map->RestHeader.DayChance);
	stream->WriteWord( &map->RestHeader.NightChance);
	for(i=0;i<14;i++) {
		stream->WriteDword( &tmpDword);
	}
	return 0;
}

/* no saving of tiled objects, are they used anywhere? */
int AREImporter::PutArea(DataStream *stream, Map *map)
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

	ieDword i = map->GetTrapCount(piter);
	while(i--) {
		const Projectile *trap = map->GetNextTrap(piter);
		if (!trap) {
			continue;
		}

		const EffectQueue *fxqueue = trap->GetEffects();

		if (!fxqueue) {
			continue;
		}

		ret = PutEffects( stream, fxqueue);
		if (ret) {
			return ret;
		}
	}

	ret = PutTraps( stream, map);
	if (ret) {
		return ret;
	}

	ret = PutMapnotes( stream, map);
	if (ret) {
		return ret;
	}

	ret = PutSongHeader( stream, map);
	if (ret) {
		return ret;
	}

	ret = PutRestHeader( stream, map);

	return ret;
}

#include "plugindef.h"

GEMRB_PLUGIN(0x145B60F0, "ARE File Importer")
PLUGIN_CLASS(IE_ARE_CLASS_ID, AREImporter)
PLUGIN_CLEANUP(ReleaseMemory)
END_PLUGIN()
