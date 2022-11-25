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

#include "strrefs.h"
#include "ie_cursors.h"

#include "ActorMgr.h"
#include "Ambient.h"
#include "AnimationFactory.h"
#include "DataFileMgr.h"
#include "DisplayMessage.h"
#include "EffectMgr.h"
#include "Game.h"
#include "GameData.h"
#include "ImageMgr.h"
#include "Interface.h"
#include "PathFinder.h"
#include "Palette.h"
#include "PluginMgr.h"
#include "ProjectileServer.h"
#include "RNG.h"
#include "Plugins/TileMapMgr.h"
#include "GameScript/GameScript.h"
#include "Scriptable/Container.h"
#include "Scriptable/Door.h"
#include "Scriptable/InfoPoint.h"
#include "Streams/FileStream.h"
#include "Streams/SlicedStream.h"

#include <cstdlib>

using namespace GemRB;

#define DEF_OPEN   0
#define DEF_CLOSE  1
#define DEF_HOPEN  2
#define DEF_HCLOSE 3

//something non signed, non ascii
#define UNINITIALIZED_BYTE  0x11

struct ResRefToStrRef {
	ResRef areaName;
	ieStrRef text;
	bool trackFlag;
	int difficulty;
};

static std::vector<ResRefToStrRef> tracks;
std::shared_ptr<DataFileMgr> INInote;

static void ReadAutonoteINI()
{
	INInote = MakePluginHolder<DataFileMgr>(IE_INI_CLASS_ID);
	char tINInote[_MAX_PATH];
	PathJoin(tINInote, core->config.GamePath, "autonote.ini", nullptr);
	FileStream* fs = FileStream::OpenFile( tINInote );
	INInote->Open(fs);
}

struct PathFinderCosts {
	PathMapFlags Passable[16] = {
		PathMapFlags::NO_SEE,
		PathMapFlags::PASSABLE,
		PathMapFlags::PASSABLE,
		PathMapFlags::PASSABLE,
		PathMapFlags::PASSABLE,
		PathMapFlags::PASSABLE,
		PathMapFlags::PASSABLE,
		PathMapFlags::PASSABLE,
		PathMapFlags::IMPASSABLE,
		PathMapFlags::PASSABLE,
		PathMapFlags::SIDEWALL,
		PathMapFlags::IMPASSABLE,
		PathMapFlags::IMPASSABLE,
		PathMapFlags::IMPASSABLE,
		PathMapFlags::PASSABLE | PathMapFlags::TRAVEL,
		PathMapFlags::PASSABLE
	};

	int NormalCost = 10;
	int AdditionalCost = 4;
	
	static const PathFinderCosts& Get() {
		static PathFinderCosts pathfinder;
		return pathfinder;
	}
	
private:
	PathFinderCosts() noexcept {
		AutoTable tm = gamedata->LoadTable("terrain");

		if (!tm) {
			return;
		}

		const char* poi;

		for (int i = 0; i < 16; i++) {
			poi = tm->QueryField( 0, i ).c_str();
			if (*poi != '*')
				Passable[i] = PathMapFlags(atoi(poi));
		}
		poi = tm->QueryField( 1, 0 ).c_str();
		if (*poi != '*')
			NormalCost = atoi( poi );
		poi = tm->QueryField( 1, 1 ).c_str();
		if (*poi != '*')
			AdditionalCost = atoi( poi );
	}

	PathFinderCosts(const PathFinderCosts&) = delete;
	PathFinderCosts(PathFinderCosts&&) = delete;
};

static int GetTrackString(const ResRef &areaName)
{
	bool trackflag = displaymsg->HasStringReference(STR_TRACKING);

	if (tracks.empty()) {
		AutoTable tm = gamedata->LoadTable("tracking", true);
		if (!tm)
			return -1;
		TableMgr::index_t trackcount = tm->GetRowCount();
		tracks.resize(trackcount);
		for (TableMgr::index_t i = 0; i < trackcount; i++) {
			const char *poi = tm->QueryField(i,0).c_str();
			if (poi[0]=='O' && poi[1]=='_') {
				tracks[i].trackFlag=false;
				poi+=2;
			} else {
				tracks[i].trackFlag=trackflag;
			}
			tracks[i].text = ieStrRef(atoi(poi));
			tracks[i].difficulty = tm->QueryFieldSigned<int>(i,1);
			tracks[i].areaName = tm->GetRowName(i);
		}
	}

	for (int i = 0; i < int(tracks.size()); i++) {
		if (tracks[i].areaName == areaName) {
			return i;
		}
	}
	return -1;
}

static Holder<Sprite2D> LoadImageAs8bit(const ResRef& resref)
{
	ResourceHolder<ImageMgr> im = GetResourceHolder<ImageMgr>(resref);
	if (!im) {
		return nullptr;
	}
	
	auto spr = im->GetSprite2D();
	if (spr->Format().Bpp > 1) {
		static const PixelFormat fmt = PixelFormat::Paletted8Bit(nullptr, false);
		spr->ConvertFormatTo(fmt);
	}
	
	assert(spr->Format().Bpp == 1); // convert format can fail (but never should here)
	return spr;
}

static TileProps MakeTileProps(const TileMap* tm, const ResRef& wedref, bool day_or_night)
{
	ResRef TmpResRef;

	if (day_or_night) {
		TmpResRef.Format("{:.6}LM", wedref);
	} else {
		TmpResRef.Format("{:.6}LN", wedref);
	}

	auto lightmap = LoadImageAs8bit(TmpResRef);
	if (!lightmap) {
		throw std::runtime_error("No lightmap available.");
	}

	TmpResRef.Format("{:.6}SR", wedref);

	auto searchmap = LoadImageAs8bit(TmpResRef);
	if (!searchmap) {
		throw std::runtime_error("No searchmap available.");
	}

	TmpResRef.Format("{:.6}HT", wedref);

	auto heightmap = LoadImageAs8bit(TmpResRef);
	if (!heightmap) {
		throw std::runtime_error("No heightmap available.");
	}
	
	const Size propsize(tm->XCellCount * 4, CeilDiv(tm->YCellCount * 64, 12));

	PixelFormat fmt = TileProps::pixelFormat;
	fmt.palette = lightmap->GetPalette();
	auto propImg = core->GetVideoDriver()->CreateSprite(Region(Point(), propsize), nullptr, fmt);

	auto propit = propImg->GetIterator();
	auto end = propit.end(propit);
	
	auto hmpal = heightmap->GetPalette();
	auto smit = searchmap->GetIterator();
	auto hmit = heightmap->GetIterator();
	auto lmit = lightmap->GetIterator();
	
	// Sadly, the original data is occasionally cropped poorly and some images are a few pixels smaller than they ought to be
	// an example of this is BG2 AR0325
	// therefore, we must have some out-of-bounds defaults and only advance iterators when they are inside propsize
	for (; propit != end; ++propit) {
		const Point& pos = propit.Position();
		uint8_t r = TileProps::defaultSearchMap;
		uint8_t g = TileProps::defaultMaterial;
		uint8_t b = TileProps::defaultElevation;
		uint8_t a = TileProps::defaultLighting;
		
		if (smit.clip.PointInside(pos)) {
			uint8_t smval = *smit; // r + g
			assert((smval & 0xf0) == 0);
			r = uint8_t(PathFinderCosts::Get().Passable[smval]);
			g = smval;
			++smit;
		}
		
		if (hmit.clip.PointInside(pos)) {
			b = hmpal->col[*hmit].r; // pick any channel, they are all the same
			++hmit;
		}
		
		if (lmit.clip.PointInside(pos)) {
			a = *lmit;
			++lmit;
		}
		
		propit.WriteRGBA(r, g, b, a);
	}

	return TileProps(propImg);
}

bool AREImporter::Import(DataStream* str)
{
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
	str->ReadDword(LastSave);
	str->ReadDword(AreaFlags);
	//skipping bg1 area connection fields
	str->Seek( 0x48, GEM_STREAM_START );
	str->ReadEnum<MapEnv>(AreaType);
	str->ReadWord(WRain);
	str->ReadWord(WSnow);
	str->ReadWord(WFog);
	str->ReadWord(WLightning);
	// unused wind speed, TODO: EEs use it for transparency
	// a single byte was re-purposed to control the alpha on the stencil water for more or less transparency.
	// If you set it to 0, then the water should be appropriately 50% transparent.
	// If you set it to any other number, it will be that transparent.
	// It's 1 byte, so setting it to 128 you'll have the same as the default of 0
	str->ReadWord(WUnknown);

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
	str->ReadDword(ActorOffset);
	str->ReadWord(ActorCount);
	str->ReadWord(InfoPointsCount);
	str->ReadDword(InfoPointsOffset);
	str->ReadDword(SpawnOffset);
	str->ReadDword(SpawnCount);
	str->ReadDword(EntrancesOffset);
	str->ReadDword(EntrancesCount);
	str->ReadDword(ContainersOffset);
	str->ReadWord(ContainersCount);
	str->ReadWord(ItemsCount);
	str->ReadDword(ItemsOffset);
	str->ReadDword(VerticesOffset);
	str->ReadWord(VerticesCount);
	str->ReadWord(AmbiCount);
	str->ReadDword(AmbiOffset);
	str->ReadDword(VariablesOffset);
	str->ReadDword(VariablesCount);
	ieDword tmp; // unused TiledObjectFlagCount and TiledObjectFlagOffset
	str->ReadDword(tmp);
	str->ReadResRef( Script );
	str->ReadDword(ExploredBitmapSize);
	str->ReadDword(ExploredBitmapOffset);
	str->ReadDword(DoorsCount);
	str->ReadDword(DoorsOffset);
	str->ReadDword(AnimCount);
	str->ReadDword(AnimOffset);
	str->ReadDword(TileCount);
	str->ReadDword(TileOffset);
	str->ReadDword(SongHeader);
	str->ReadDword(RestHeader);
	if (core->HasFeature(GF_AUTOMAP_INI) ) {
		str->ReadDword(tmp); //skipping unknown in PST
	}
	str->ReadDword(NoteOffset);
	str->ReadDword(NoteCount);
	str->ReadDword(TrapOffset);
	str->ReadDword(TrapCount);
	str->ReadResRef( Dream1 );
	str->ReadResRef( Dream2 );
	// 56 bytes of reserved space
	return true;
}

//alter a map to the night/day version in case of an extended night map (bg2 specific)
//return true, if change happened, in which case a movie is played by the Game object
bool AREImporter::ChangeMap(Map *map, bool day_or_night)
{
	ResRef TmpResRef;

	//get the right tilemap name
	if (day_or_night) {
		TmpResRef = map->WEDResRef;
	} else {
		TmpResRef.Format("{:.7}N", map->WEDResRef);
	}
	PluginHolder<TileMapMgr> tmm = MakePluginHolder<TileMapMgr>(IE_WED_CLASS_ID);
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
	
	try {
		TileProps props = MakeTileProps(tm, map->WEDResRef, day_or_night);
		
		// Small map for MapControl
		ResourceHolder<ImageMgr> sm = GetResourceHolder<ImageMgr>(TmpResRef);

		if (sm) {
			// night small map is *optional*!
			// keep the exising map if this one is null
			map->SmallMap = sm->GetSprite2D();
		}
		
		//the map state was altered, no need to hold this off for any later
		map->DayNight = day_or_night;
		
		tm->UpdateDoors();
		
		map->SetTileMapProps(std::move(props));
	} catch (const std::exception& e) {
		Log(ERROR, "AREImporter", "{}", e.what());
		return false;
	}

	// update the tiles and tilecount (eg. door0304 in Edwin's Docks (ar0300) entrance
	for (size_t i = 0; i < tm->GetDoorCount(); i++) {
		Door* door = tm->GetDoor(i);
		bool baseClosed, oldOpen = door->IsOpen();
		door->SetTiles(tmm->GetDoorIndices(door->ID, baseClosed));
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

Ambient* AREImporter::SetupMainAmbients(const Map::MainAmbients& mainAmbients)
{
	ResRef mainAmbient;
	if (!mainAmbients.Ambient1.IsEmpty()) {
		mainAmbient = mainAmbients.Ambient1;
	}
	// the second ambient is always longer, was meant as a memory optimisation w/ IE_AMBI_HIMEM
	// however that was implemented only for the normal ambients
	// nowadays we can just skip the first
	if (!mainAmbients.Ambient2.IsEmpty()) {
		mainAmbient = mainAmbients.Ambient2;
	}
	if (mainAmbient.IsEmpty()) return nullptr;

	Ambient *ambi = new Ambient();
	ambi->flags = IE_AMBI_ENABLED | IE_AMBI_LOOPING | IE_AMBI_MAIN | IE_AMBI_NOSAVE;
	ambi->gain = mainAmbients.AmbientVol;
	// sounds and name
	ambi->sounds.emplace_back(mainAmbient);
	ambi->name = mainAmbient;
	ambi->appearance = (1<<25) - 1; // default to all 24 bits enabled, one per hour
	ambi->radius = 50; // REFERENCE_DISTANCE
	return ambi;
}

Map* AREImporter::GetMap(const ResRef& resRef, bool day_or_night)
{
	// if this area does not have extended night, force it to day mode
	if (!(AreaFlags & AT_EXTENDED_NIGHT))
		day_or_night = true;
	
	PluginHolder<TileMapMgr> tmm = MakePluginHolder<TileMapMgr>(IE_WED_CLASS_ID);
	DataStream* wedfile = gamedata->GetResource( WEDResRef, IE_WED_CLASS_ID );
	tmm->Open( wedfile );

	//there was no tilemap set yet, so lets just send a NULL
	TileMap* tm = tmm->GetTileMap(NULL);
	if (!tm) {
		Log(ERROR, "AREImporter", "No tile map available.");
		return nullptr;
	}
	
	ResRef TmpResRef;
	if (day_or_night) {
		TmpResRef = WEDResRef;
	} else {
		TmpResRef.Format("{:.7}N", WEDResRef);
	}

	// Small map for MapControl
	ResourceHolder<ImageMgr> sm = GetResourceHolder<ImageMgr>(TmpResRef);
	if (!sm) {
		//fall back to day minimap
		sm = GetResourceHolder<ImageMgr>(WEDResRef);
	}
	
	Map* map = nullptr;
	try {
		map = new Map(tm, MakeTileProps(tm, WEDResRef, day_or_night), sm ? sm->GetSprite2D() : nullptr);
	} catch (const std::exception& e) {
		Log(ERROR, "AREImporter", "{}", e.what());
		return nullptr;
	}
	
	if (core->config.SaveAsOriginal) {
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
	map->WEDResRef = WEDResRef;
	map->Dream[0] = Dream1;
	map->Dream[1] = Dream2;

	//we have to set this here because the actors will receive their
	//current area setting here, areas' 'scriptname' is their name
	map->SetScriptName(resRef);
	// reset MasterArea, since the script name wasn't available in the constructor
	map->MasterArea = core->GetGame()->MasterArea(map->GetScriptRef());
	int idx = GetTrackString(resRef);
	if (idx>=0) {
		map->SetTrackString(tracks[idx].text, tracks[idx].trackFlag, tracks[idx].difficulty);
	} else {
		map->SetTrackString(ieStrRef(-1), false, 0);
	}
	
	//if the Script field is empty, the area name will be copied into it on first load
	//this works only in the iwd branch of the games
	if (Script.IsEmpty() && core->HasFeature(GF_FORCE_AREA_SCRIPT)) {
		Script = resRef;
	}

	if (!Script.IsEmpty()) {
		//for some reason the area's script is run from the last slot
		//at least one area script depends on this, if you need something
		//more customisable, add a game flag
		map->Scripts[MAX_SCRIPTS-1] = new GameScript( Script, map );
	}

	Log(DEBUG, "AREImporter", "Loading songs");
	str->Seek( SongHeader, GEM_STREAM_START );
	//5 is the number of song indices
	for (auto& list : map->SongList) {
		str->ReadDword(list);
	}
	
	Map::MainAmbients& dayAmbients = map->dayAmbients;

	str->ReadResRef(dayAmbients.Ambient1);
	str->ReadResRef(dayAmbients.Ambient2);
	str->ReadDword(dayAmbients.AmbientVol);
	
	Map::MainAmbients& nightAmbients = map->nightAmbients;

	str->ReadResRef(nightAmbients.Ambient1);
	str->ReadResRef(nightAmbients.Ambient2);
	str->ReadDword(nightAmbients.AmbientVol);
	
	std::vector<Ambient*> ambients;
	// check for existence of main ambients (bg1)
	#define DAY_BITS (((1<<18) - 1) ^ ((1<<6) - 1)) // day: bits 6-18 per DLTCEP
	Ambient *ambi = SetupMainAmbients(dayAmbients);
	if (ambi) {
		// schedule for day/night
		// if the two ambients are the same, just add one, so there's no restart
		if (dayAmbients.Ambient2 != nightAmbients.Ambient2) {
			ambi->appearance = DAY_BITS;
			ambients.push_back(ambi);
			// night
			ambi = SetupMainAmbients(nightAmbients);
			if (ambi) {
				ambi->appearance ^= DAY_BITS; // night: bits 0-5 + 19-23, [dusk till dawn]
			}
		}
		// bgt ar7300 has a nigth ambient only in the first slot
		if (ambi) {
			ambients.push_back(ambi);
		}
	}

	// reverb to match against reverb.2da or iwd reverb.ids
	// (if the 2da doesn't exist - which we provide for all; they use the same values)
	ieDword reverbID = EFX_PROFILE_REVERB_INVALID; // non PST data has it at 0, so we don't bother reading
	if (core->HasFeature(GF_PST_STATE_FLAGS)) {
		str->ReadDword(reverbID);
	}

	str->Seek(RestHeader + 32, GEM_STREAM_START); // skip the name
	for (auto& ref : map->RestHeader.Strref) {
		str->ReadStrRef(ref);
	}
	for (auto& ref : map->RestHeader.CreResRef) {
		str->ReadResRef(ref);
	}
	str->ReadWord(map->RestHeader.CreatureNum);
	if( map->RestHeader.CreatureNum>MAX_RESCOUNT ) {
		map->RestHeader.CreatureNum = MAX_RESCOUNT;
	}
	str->ReadWord(map->RestHeader.Difficulty);  //difficulty?
	str->ReadDword(map->RestHeader.sduration);  //spawn duration, lifespan
	str->ReadWord(map->RestHeader.rwdist);      //random walk distance, hunting range
	str->ReadWord(map->RestHeader.owdist);      //other walk distance, follow range
	str->ReadWord(map->RestHeader.Maximum);     //maximum number of creatures
	str->ReadWord(map->RestHeader.Enabled);
	str->ReadWord(map->RestHeader.DayChance);
	str->ReadWord(map->RestHeader.NightChance);
	// 14 reserved dwords

	Log(DEBUG, "AREImporter", "Loading regions");
	core->LoadProgress(70);
	//Loading InfoPoints
	for (int i = 0; i < InfoPointsCount; i++) {
		str->Seek( InfoPointsOffset + ( i * 0xC4 ), GEM_STREAM_START );
		ieWord Type, VertexCount;
		ieDword FirstVertex, Cursor, Flags;
		ieWord TrapDetDiff, TrapRemDiff, Trapped, TrapDetected;
		Point launchP;
		Point pos;
		Point talkPos;
		ieVariable Name, Entrance;
		ResRef script0;
		ResRef KeyResRef;
		ResRef Destination;
		// two adopted pst specific fields
		ResRef DialogResRef;
		ResRef WavResRef;
		ieStrRef DialogName;
		str->ReadVariable(Name);
		str->ReadWord(Type);
		Region bbox;
		str->ReadRegion(bbox, true);
		str->ReadWord(VertexCount);
		str->ReadDword(FirstVertex);
		ieDword tmp2;
		str->ReadDword(tmp2); //named triggerValue in the IE source
		str->ReadDword(Cursor);
		str->ReadResRef( Destination );
		str->ReadVariable(Entrance);
		str->ReadDword(Flags);
		ieStrRef StrRef;
		str->ReadStrRef(StrRef);
		str->ReadWord(TrapDetDiff);
		str->ReadWord(TrapRemDiff);
		str->ReadWord(Trapped);
		str->ReadWord(TrapDetected);
		str->ReadPoint(launchP);
		str->ReadResRef( KeyResRef );
		str->ReadResRef(script0);
		/* ARE 9.1: 4B per position after that. */
		if (16 == map->version) {
			str->ReadPoint(pos); // OverridePoint in NI
			if (pos.IsZero()) {
				str->ReadPoint(pos); // AlternatePoint in NI
			} else {
				str->Seek(8, GEM_CURRENT_POS);
			}
			str->Seek(26, GEM_CURRENT_POS);
		} else {
			str->ReadPoint(pos); // TransitionWalkToX, TransitionWalkToY
			//maybe we have to store this
			// bg2: 15 reserved dwords, the above point is actually in dwords (+1),
			// but since it's the last thing the underseek doesn't matter
			str->Seek( 36, GEM_CURRENT_POS );
		}

		if (core->HasFeature(GF_INFOPOINT_DIALOGS)) {
			str->ReadResRef( WavResRef );
			str->ReadPoint(talkPos);
			str->ReadStrRef(DialogName);
			str->ReadResRef( DialogResRef );
		} else {
			WavResRef.Reset();
			DialogName = ieStrRef::INVALID;
			DialogResRef.Reset();
		}

		InfoPoint* ip = nullptr;
		str->Seek( VerticesOffset + ( FirstVertex * 4 ), GEM_STREAM_START );
		if (VertexCount <= 1) {
			// this is exactly the same as bbox.origin
			if (VertexCount == 1) {
				Point pos;
				str->ReadPoint(pos);
				assert(pos == bbox.origin);
			}

			if (bbox.size.IsInvalid()) {
				// we approximate a bounding box equivalent to a small radius
				// we copied this from the Container code that seems to indicate
				// this is how the originals behave. It is probably "good enough"
				bbox.x = pos.x - 7;
				bbox.y = pos.y - 5;
				bbox.w = 16;
				bbox.h = 12;
			}

			ip = tm->AddInfoPoint( Name, Type, nullptr );
			ip->BBox = bbox;
		} else if (VertexCount == 2) {
#define MSG "Encountered a bogus polygon with 2 vertices!"
#if NDEBUG
			Log(ERROR, "AREImporter", MSG);
			continue;
#else // make this fatal on debug builds
			error("AREImporter", MSG);
#endif
#undef MSG
		} else {
			std::vector<Point> points(VertexCount);
			for (int x = 0; x < VertexCount; x++) {
				str->ReadPoint(points[x]);
			}
			auto poly = std::make_shared<Gem_Polygon>(std::move(points), &bbox);
			ip = tm->AddInfoPoint( Name, Type, poly );
		}

		ip->TrapDetectionDiff = TrapDetDiff;
		ip->TrapRemovalDiff = TrapRemDiff;
		ip->Trapped = Trapped;
		ip->TrapDetected = TrapDetected;
		ip->TrapLaunch = launchP;
		// translate door cursor on infopoint to correct cursor
		if (Cursor == IE_CURSOR_DOOR) Cursor = IE_CURSOR_PASS;
		ip->Cursor = Cursor;
		ip->SetOverheadText(core->GetString(StrRef), false);
		ip->StrRef = StrRef; //we need this when saving area
		ip->SetMap(map);
		ip->Flags = Flags;
		ip->UsePoint = pos;
		//FIXME: PST doesn't use this field
		if (ip->GetUsePoint()) {
			ip->Pos = ip->UsePoint;
		} else {
			ip->Pos = bbox.Center();
		}
		ip->Destination = Destination;
		ip->EntranceName = Entrance;
		ip->KeyResRef = KeyResRef;

		//these appear only in PST, but we could support them everywhere
		// HOWEVER they did not use them as witnessed in ar0101 (0101prt1 and 0101prt2) :(
		if (core->HasFeature(GF_PST_STATE_FLAGS)) {
			talkPos = ip->Pos;
		}
		ip->TalkPos = talkPos;
		ip->DialogName=DialogName;
		ip->SetDialog(DialogResRef);
		ip->SetEnter(WavResRef);

		if (script0.IsEmpty()) {
			ip->Scripts[0] = nullptr;
		} else {
			ip->Scripts[0] = new GameScript(script0, ip);
		}
	}

	Log(DEBUG, "AREImporter", "Loading containers");
	for (int i = 0; i < ContainersCount; i++) {
		str->Seek( ContainersOffset + ( i * 0xC0 ), GEM_STREAM_START );
		ieVariable Name;
		ieWord Type, LockDiff;
		ieDword Flags;
		ieWord TrapDetDiff, TrapRemDiff, Trapped, TrapDetected;
		Point pos;
		Point launchPos;
		ieDword ItemIndex, ItemCount;
		ResRef KeyResRef;
		ieStrRef OpenFail;

		str->ReadVariable(Name);
		str->ReadPoint(pos);
		str->ReadWord(Type);
		str->ReadWord(LockDiff);
		str->ReadDword(Flags);
		str->ReadWord(TrapDetDiff);
		str->ReadWord(TrapRemDiff);
		str->ReadWord(Trapped);
		str->ReadWord(TrapDetected);
		str->ReadPoint(launchPos);
		Region bbox;
		str->ReadRegion(bbox, true);
		str->ReadDword(ItemIndex);
		str->ReadDword(ItemCount);
		str->ReadResRef( Script );
		ieDword firstIndex;
		ieWord vertCount, unknown;
		str->ReadDword(firstIndex);
		//the vertex count is only 16 bits, there is a weird flag
		//after it, which is usually 0, but sometimes set to 1
		str->ReadWord(vertCount);
		str->ReadWord(unknown);   //trigger range
		//str->Read( Name, 32 );     //owner's scriptname
		str->Seek( 32, GEM_CURRENT_POS);
		str->ReadResRef( KeyResRef);
		str->Seek( 4, GEM_CURRENT_POS); //break difficulty
		str->ReadStrRef(OpenFail);
		// 14 reserved dwords

		str->Seek( VerticesOffset + ( firstIndex * 4 ), GEM_STREAM_START );

		Container* c = nullptr;
		if (vertCount == 0) {
			/* piles have no polygons and no bounding box in some areas,
			 * but bg2 gives them this bounding box at first load,
			 * should we specifically check for Type==IE_CONTAINER_PILE? */
			if (bbox.size.IsInvalid()) {
				bbox.x = pos.x - 7;
				bbox.y = pos.y - 5;
				bbox.w = 16;
				bbox.h = 12;
			}
			c = map->AddContainer( Name, Type, nullptr );
			c->BBox = bbox;
		} else {
			std::vector<Point> points(vertCount);
			for (int x = 0; x < vertCount; x++) {
				str->ReadPoint(points[x]);
			}
			auto poly = std::make_shared<Gem_Polygon>(std::move(points), &bbox);
			c = map->AddContainer( Name, Type, poly );
		}

		c->Pos = pos;
		c->LockDifficulty = LockDiff;
		c->Flags = Flags;
		c->TrapDetectionDiff = TrapDetDiff;
		c->TrapRemovalDiff = TrapRemDiff;
		c->Trapped = Trapped;
		c->TrapDetected = TrapDetected;
		c->TrapLaunch = launchPos;
		//reading items into a container
		str->Seek( ItemsOffset+( ItemIndex * 0x14 ), GEM_STREAM_START);
		while(ItemCount--) {
			//cannot add directly to inventory (ground piles)
			c->AddItem( core->ReadItem(str));
		}

		if (Type == IE_CONTAINER_PILE) Script.Reset();

		if (Script.IsEmpty()) {
			c->Scripts[0] = nullptr;
		} else {
			c->Scripts[0] = new GameScript(Script, c);
		}
		c->KeyResRef = KeyResRef;
		if (!OpenFail) OpenFail = ieStrRef(-1); // rewrite 0 to -1
		c->OpenFail = OpenFail;
	}

	Log(DEBUG, "AREImporter", "Loading doors");
	for (ieDword i = 0; i < DoorsCount; i++) {
		str->Seek( DoorsOffset + ( i * 0xc8 ), GEM_STREAM_START );
		ieDword Flags;
		ieDword OpenFirstVertex, ClosedFirstVertex;
		ieDword OpenFirstImpeded, ClosedFirstImpeded;
		ieWord OpenVerticesCount, ClosedVerticesCount;
		ieWord OpenImpededCount, ClosedImpededCount;
		ieVariable LongName, LinkedInfo;
		ResRef ShortName;
		ieDword cursor;
		ResRef KeyResRef;
		ResRef script0;
		ieWord TrapDetect, TrapRemoval;
		ieWord Trapped, TrapDetected;
		Point launchP;
		ieDword DiscoveryDiff, LockRemoval;
		Region BBClosed, BBOpen;
		ieStrRef OpenStrRef;
		ieStrRef NameStrRef;
		ResRef Dialog;
		ieWord hp, ac;

		str->ReadVariable(LongName);
		str->ReadResRef( ShortName );
		str->ReadDword(Flags);
		if (map->version == 16) {
			Flags = FixIWD2DoorFlags(Flags, false);
		}
		if (AreaType & AT_OUTDOOR) Flags |= DOOR_TRANSPARENT; // actually true only for fog-of-war, excluding other actors
		str->ReadDword(OpenFirstVertex);
		str->ReadWord(OpenVerticesCount);
		str->ReadWord(ClosedVerticesCount);
		str->ReadDword(ClosedFirstVertex);
		str->ReadRegion(BBOpen, true);
		str->ReadRegion(BBClosed, true);
		str->ReadDword(OpenFirstImpeded);
		str->ReadWord(OpenImpededCount);
		str->ReadWord(ClosedImpededCount);
		str->ReadDword(ClosedFirstImpeded);
		str->ReadWord(hp); // hitpoints
		str->ReadWord(ac); // AND armorclass, according to IE dev info
		ResRef OpenResRef, CloseResRef;
		str->ReadResRef( OpenResRef );
		str->ReadResRef( CloseResRef );
		str->ReadDword(cursor);
		str->ReadWord(TrapDetect);
		str->ReadWord(TrapRemoval);
		str->ReadWord(Trapped);
		str->ReadWord(TrapDetected);
		str->ReadPoint(launchP);
		str->ReadResRef( KeyResRef );
		str->ReadResRef(script0);
		str->ReadDword(DiscoveryDiff);
		str->ReadDword(LockRemoval);
		Point toOpen[2];
		str->ReadPoint(toOpen[0]);
		str->ReadPoint(toOpen[1]);
		str->ReadStrRef(OpenStrRef);
		if (core->HasFeature(GF_AUTOMAP_INI) ) {
			char tmp[25];
			str->Read(tmp, 24);
			tmp[24] = 0;
			LinkedInfo = tmp; // LinkedInfo unused in pst anyway?
		} else {
			str->ReadVariable(LinkedInfo);
		}
		str->ReadStrRef(NameStrRef); // trigger name
		str->ReadResRef( Dialog );
		if (core->HasFeature(GF_AUTOMAP_INI) ) {
			// maybe this is important? but seems not
			str->Seek( 8, GEM_CURRENT_POS );
		}

		//Reading Open Polygon
		std::shared_ptr<Gem_Polygon> open = nullptr;
		str->Seek( VerticesOffset + ( OpenFirstVertex * 4 ), GEM_STREAM_START );
		if (OpenVerticesCount) {
			std::vector<Point> points(OpenVerticesCount);
			for (int x = 0; x < OpenVerticesCount; x++) {
				str->ReadPoint(points[x]);
			}
			open = std::make_shared<Gem_Polygon>(std::move(points), &BBOpen );
		}

		//Reading Closed Polygon
		std::shared_ptr<Gem_Polygon> closed = nullptr;
		str->Seek( VerticesOffset + ( ClosedFirstVertex * 4 ),
				GEM_STREAM_START );
		if (ClosedVerticesCount) {
			std::vector<Point> points(ClosedVerticesCount);
			for (int x = 0; x < ClosedVerticesCount; x++) {
				str->ReadPoint(points[x]);
			}
			closed = std::make_shared<Gem_Polygon>(std::move(points), &BBClosed);
		}

		//Getting Door Information from the WED File
		bool BaseClosed;
		auto indices = tmm->GetDoorIndices( ShortName, BaseClosed );
		if (core->HasFeature(GF_REVERSE_DOOR)) {
			BaseClosed = !BaseClosed;
		}

		auto closedPolys = tmm->ClosedDoorPolygons();
		auto openPolys = tmm->OpenDoorPolygons();

		DoorTrigger dt(open, std::move(openPolys), closed, std::move(closedPolys));
		Door* door = tm->AddDoor( ShortName, LongName, Flags, BaseClosed,
								 std::move(indices), std::move(dt) );
		door->OpenBBox = BBOpen;
		door->ClosedBBox = BBClosed;

		//Reading Open Impeded blocks
		str->Seek( VerticesOffset + ( OpenFirstImpeded * 4 ),
				GEM_STREAM_START );
		door->open_ib.resize(OpenImpededCount);
		for (Point& point : door->open_ib) {
			str->ReadPoint(point);
		}

		//Reading Closed Impeded blocks
		str->Seek( VerticesOffset + ( ClosedFirstImpeded * 4 ),
				GEM_STREAM_START );
		
		door->closed_ib.resize(ClosedImpededCount);
		for (Point& point : door->closed_ib) {
			str->ReadPoint(point);
		}
		door->SetMap(map);

		door->hp = hp;
		door->ac = ac;
		door->TrapDetectionDiff = TrapDetect;
		door->TrapRemovalDiff = TrapRemoval;
		door->Trapped = Trapped;
		door->TrapDetected = TrapDetected;
		door->TrapLaunch = launchP;

		door->Cursor = cursor;
		door->KeyResRef = KeyResRef;
		if (script0.IsEmpty()) {
			door->Scripts[0] = nullptr;
		} else {
			door->Scripts[0] = new GameScript(script0, door);
		}

		door->toOpen[0] = toOpen[0];
		door->toOpen[1] = toOpen[1];
		//Leave the default sound untouched
		if (!OpenResRef.IsEmpty())
			door->OpenSound = OpenResRef;
		else {
			if (Flags & DOOR_SECRET)
				door->OpenSound = gamedata->defaultSounds[DEF_HOPEN];
			else
				door->OpenSound = gamedata->defaultSounds[DEF_OPEN];
		}
		if (!CloseResRef.IsEmpty())
			door->CloseSound = CloseResRef;
		else {
			if (Flags & DOOR_SECRET)
				door->CloseSound = gamedata->defaultSounds[DEF_HCLOSE];
			else
				door->CloseSound = gamedata->defaultSounds[DEF_CLOSE];
		}
		door->DiscoveryDiff=DiscoveryDiff;
		door->LockDifficulty=LockRemoval;
		if (!OpenStrRef) OpenStrRef = ieStrRef(-1); // rewrite 0 to -1
		door->OpenStrRef=OpenStrRef;
		door->LinkedInfo = MakeVariable(LinkedInfo);
		//these 2 fields are not sure
		door->NameStrRef=NameStrRef;
		door->SetDialog(Dialog);
	}

	Log(DEBUG, "AREImporter", "Loading spawnpoints");
	for (ieDword i = 0; i < SpawnCount; i++) {
		str->Seek( SpawnOffset + (i*0xc8), GEM_STREAM_START );
		ieVariable Name;
		Point Pos;
		ieWord Difficulty, Frequency, Method;
		ieWord Maximum, Enabled;
		
		std::vector<ResRef> creatures(MAX_RESCOUNT);
		ieWord DayChance, NightChance;
		ieDword Schedule;
		ieDword sduration;
		ieWord rwdist, owdist;

		str->ReadVariable(Name);
		str->ReadPoint(Pos);
		for (auto& creature : creatures) {
			str->ReadResRef(creature);
		}
		ieWord count;
		str->ReadWord(count);
		assert(count <= MAX_RESCOUNT);
		creatures.resize(count);
		str->ReadWord(Difficulty);
		str->ReadWord(Frequency);
		str->ReadWord(Method);
		str->ReadDword(sduration); //time to live for spawns
		str->ReadWord(rwdist);     // random walk distance (0 is unlimited), hunting range
		str->ReadWord(owdist);     // other walk distance (inactive in all engines?), follow range
		str->ReadWord(Maximum);
		str->ReadWord(Enabled);
		str->ReadDword(Schedule);
		str->ReadWord(DayChance);
		str->ReadWord(NightChance);
		// 14 reserved dwords

		Spawn *sp = map->AddSpawn(Name, Pos, std::move(creatures));
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

	int pst = core->HasFeature(GF_AUTOMAP_INI);

	core->LoadProgress(75);
	Log(DEBUG, "AREImporter", "Loading actors");
	str->Seek( ActorOffset, GEM_STREAM_START );
	assert(core->IsAvailable(IE_CRE_CLASS_ID));
	auto actmgr = GetImporter<ActorMgr>(IE_CRE_CLASS_ID);
	for (int i = 0; i < ActorCount; i++) {
		ieVariable defaultName;
		ResRef creResRef;
		ieDword talkCount;
		ieDword orientation;
		ieDword schedule;
		ieDword removalTime;
		Point pos;
		Point des;
		ieWord maxDistance;
		ieWord spawned;
		ResRef dialog;
		ResRef scripts[8]; //the original order is shown in scrlev.ids
		ieDword flags;
		ieByte difficultyMargin;

		str->ReadVariable(defaultName);
		str->ReadPoint(pos);
		str->ReadPoint(des);
		str->ReadDword(flags);
		str->ReadWord(spawned); // "type"
		str->Seek(1, GEM_CURRENT_POS); // one letter of a ResRef, changed to * at runtime, purpose unknown (portraits?), but not needed either
		str->Read(&difficultyMargin, 1); // iwd2 only, "alignbyte" in bg2 (padding)
		str->Seek(4, GEM_CURRENT_POS); //actor animation, unused
		str->ReadDword(orientation); // was word + padding in bg2
		str->ReadDword(removalTime);
		str->ReadWord(maxDistance); // hunting range
		str->Seek(2, GEM_CURRENT_POS); // apparently unused https://gibberlings3.net/forums/topic/21724-a (follow range)
		str->ReadDword(schedule);
		str->ReadDword(talkCount);
		str->ReadResRef(dialog);

		str->ReadResRef(scripts[SCR_OVERRIDE]);
		str->ReadResRef(scripts[SCR_GENERAL]);
		str->ReadResRef(scripts[SCR_CLASS]);
		str->ReadResRef(scripts[SCR_RACE]);
		str->ReadResRef(scripts[SCR_DEFAULT]);
		str->ReadResRef(scripts[SCR_SPECIFICS]);
		str->ReadResRef(creResRef);
		DataStream* creFile;
		Actor *act;
		ieDword CreOffset, CreSize;
		str->ReadDword(CreOffset);
		str->ReadDword(CreSize);
		// another iwd2 script slot
		str->ReadResRef(scripts[SCR_AREA]);
		str->Seek(120, GEM_CURRENT_POS);
		//not iwd2, this field is garbage
		if (!core->HasFeature(GF_IWD2_SCRIPTNAME)) {
			scripts[SCR_AREA].Reset();
		}
		//actually, Flags&1 signs that the creature
		//is not loaded yet, so !(Flags&1) means it is embedded
		if (CreOffset != 0 && !(flags & 1)) {
			creFile = SliceStream(str, CreOffset, CreSize, true);
		} else {
			creFile = gamedata->GetResource(creResRef, IE_CRE_CLASS_ID);
		}
		if(!actmgr->Open(creFile)) {
			Log(ERROR, "AREImporter", "Couldn't read actor: {}!", creResRef);
			continue;
		}
		act = actmgr->GetActor(0);
		if (!act) continue;

		// PST generally doesn't appear to use the starting MC_ bits, but for some reason
		// there's a coaxmetal copy in the mortuary with both KEEP and REMOVE corpse
		// set that should never be seen. The actor is also already dead, so we don't end
		// up doing any of the regular cleanup on it (it's mrtghost.cre). Banish it instead.
		if (pst && act->GetBase(IE_STATE_ID) & STATE_DEAD && act->GetBase(IE_MC_FLAGS) & MC_REMOVE_CORPSE) {
			continue;
		}
		map->AddActor(act, false);
		act->Pos = pos;
		act->Destination = des;
		act->HomeLocation = des;
		act->maxWalkDistance = maxDistance;
		act->Spawned = spawned;
		act->appearance = schedule;
		//copying the scripting name into the actor
		//if the CreatureAreaFlag was set to 8
		if ((flags & AF_NAME_OVERRIDE) || core->HasFeature(GF_IWD2_SCRIPTNAME)) {
			act->SetScriptName(defaultName);
		}
		//IWD2 specific hacks
		if (core->HasFeature(GF_3ED_RULES)) {
			//This flag is used for something else in IWD2
			if (flags & AF_NAME_OVERRIDE) {
				act->BaseStats[IE_EA] = EA_EVILCUTOFF;
			}
			if (flags & AF_SEEN_PARTY) {
				act->SetMCFlag(MC_SEENPARTY, BitOp::OR);
			}
			if (flags & AF_INVULNERABLE) {
				act->SetMCFlag(MC_INVULNERABLE, BitOp::OR);
			}
			if (!(flags & AF_ENABLED)) {
				// DifficultyMargin - only enable actors that are difficult enough vs the area difficulty
				// 1 - area difficulty 1
				// 2 - area difficulty 2
				// 4 - area difficulty 3
				if (difficultyMargin && !(difficultyMargin & map->AreaDifficulty)) {
					act->DestroySelf();
				}
			}
		}
		act->DifficultyMargin = difficultyMargin;

		if (!dialog.IsEmpty()) {
			act->SetDialog(dialog);
		}
		for (int j=0;j<8;j++) {
			if (!scripts[j].IsEmpty()) {
				act->SetScript(scripts[j], j);
			}
		}
		act->SetOrientation(ClampToOrientation(orientation), false);
		act->TalkCount = talkCount;
		act->RemovalTime = removalTime;
		act->RefreshEffects();
	}

	core->LoadProgress(90);
	Log(DEBUG, "AREImporter", "Loading animations");
	str->Seek( AnimOffset, GEM_STREAM_START );
	for (ieDword i = 0; i < AnimCount; i++) {
		AreaAnimation anim = AreaAnimation();
		str->ReadVariable(anim.Name);
		ieWord startFrameRange;
		str->ReadPoint(anim.Pos);
		str->ReadDword(anim.appearance);
		str->ReadResRef(anim.BAM);
		str->ReadWord(anim.sequence);
		str->ReadWord(anim.frame);
		str->ReadDword(anim.Flags);
		anim.originalFlags = anim.Flags;
		str->ReadScalar(anim.height);
		if (core->HasFeature(GF_IMPLICIT_AREAANIM_BACKGROUND)) {
			anim.height = ANI_PRI_BACKGROUND;
			anim.Flags |= A_ANI_NO_WALL;
		}
		str->ReadWord(anim.transparency);
		str->ReadWord(startFrameRange);
		str->Read(&anim.startchance, 1);
		if (anim.startchance <= 0) {
			anim.startchance = 100; // percentage of starting a cycle
		}
		if (startFrameRange && (anim.Flags & A_ANI_RANDOM_START)) {
			anim.frame = RAND(0, startFrameRange - 1);
		}
		anim.startFrameRange = 0; // this will never get resaved (iirc)
		str->Read(&anim.skipcycle, 1); // how many cycles are skipped (100% skippage), "period" in bg2
		str->ReadResRef(anim.PaletteRef);
		// TODO: EE: word with anim width for PVRZ/WBM resources (if flag bits are set, see A_ANI_ defines)
		// 0x4a holds the height
		str->ReadDword(anim.unknown48);

		if (pst) {
			AdjustPSTFlags(anim);
		}

		// set up the animation, it cannot be done here
		// because a StaticSequence action can change it later
		map->AddAnimation(std::move(anim));
	}

	Log(DEBUG, "AREImporter", "Loading entrances");
	str->Seek( EntrancesOffset, GEM_STREAM_START );
	for (ieDword i = 0; i < EntrancesCount; i++) {
		ieVariable Name;
		Point Pos;
		ieWord Face;
		str->ReadVariable(Name);
		str->ReadPoint(Pos);
		str->ReadWord(Face);
		str->Seek(66, GEM_CURRENT_POS); // just reserved bytes
		map->AddEntrance(Name, Pos, Face);
	}

	Log(DEBUG, "AREImporter", "Loading variables");
	map->locals->LoadInitialValues(resRef);
	str->Seek( VariablesOffset, GEM_STREAM_START );
	for (ieDword i = 0; i < VariablesCount; i++) {
		ieVariable Name;
		ieDword Value;
		str->ReadVariable(Name);
		str->Seek(8, GEM_CURRENT_POS); // type + resreftype, part of the partly implemented type system (uint, int, float, str)
		str->ReadDword(Value);
		str->Seek(40, GEM_CURRENT_POS); // values as an int32, float64, string
		map->locals->SetAt( Name, Value );
	}

	Log(DEBUG, "AREImporter", "Loading ambients");
	str->Seek( AmbiOffset, GEM_STREAM_START );
	for (int i = 0; i < AmbiCount; ++i) {
		ResRef sounds[MAX_RESCOUNT];
		ieWord tmpWord;

		Ambient *ambient = new Ambient();
		str->Read(&ambient->name, 32);
		str->ReadPoint(ambient->origin);
		str->ReadWord(ambient->radius);
		str->Seek(2, GEM_CURRENT_POS); // alignment padding
		str->ReadDword(ambient->pitchVariance);
		str->ReadWord(ambient->gainVariance);
		str->ReadWord(ambient->gain);
		for (auto& sound : sounds) {
			str->ReadResRef(sound);
		}
		str->ReadWord(tmpWord);
		str->Seek(2, GEM_CURRENT_POS); // alignment padding
		ieDword interval;
		str->ReadDword(interval);
		ambient->interval = interval * 1000;
		str->ReadDword(interval);
		ambient->intervalVariance = interval * 1000;
		// schedule bits
		str->ReadDword(ambient->appearance);
		str->ReadDword(ambient->flags);
		str->Seek( 64, GEM_CURRENT_POS );
		//this is a physical limit
		if (tmpWord>MAX_RESCOUNT) {
			tmpWord=MAX_RESCOUNT;
		}
		for (int j = 0; j < tmpWord; j++) {
			ambient->sounds.emplace_back(sounds[j]);
		}
		ambients.push_back(ambient);
	}
	
	map->SetAmbients(std::move(ambients), reverbID);

	Log(DEBUG, "AREImporter", "Loading automap notes");
	str->Seek( NoteOffset, GEM_STREAM_START );

	Point point;
	//Don't bother with autonote.ini if the area has autonotes (ie. it is a saved area)

	if (pst) {
		AnimationFactory* flags = (AnimationFactory*)gamedata->GetFactoryResource("FLAG1", IE_BAM_CLASS_ID);
		if (flags == NULL) {
			ResourceHolder<ImageMgr> roimg = GetResourceHolder<ImageMgr>("RONOTE");
			ResourceHolder<ImageMgr> userimg = GetResourceHolder<ImageMgr>("USERNOTE");

			flags = new AnimationFactory("FLAG1", {roimg->GetSprite2D(), userimg->GetSprite2D()},
										 {{1, 0}, {1, 1}}, {0, 1});
			gamedata->AddFactoryResource(flags);
		}

		if (!NoteCount) {
			if( !INInote ) {
				ReadAutonoteINI();
			}

			//add autonote.ini entries
			if( INInote ) {
				const ieVariable& scriptName = map->GetScriptName();
				int count = INInote->GetKeyAsInt(scriptName, "count", 0);
				while (count) {
					ieVariable key;
					int value;
					key.Format("xPos{}", count);
					value = INInote->GetKeyAsInt(scriptName, key, 0);
					point.x = value;
					key.Format("yPos{}", count);
					value = INInote->GetKeyAsInt(scriptName, key, 0);
					point.y = value;
					key.Format("text{}", count);
					value = INInote->GetKeyAsInt(scriptName, key, 0);
					map->AddMapNote(point, 0, ieStrRef(value), true);
					count--;
				}
			}
		} else {
			for (ieDword i = 0; i < NoteCount; i++) {
				ieDword px,py;
				str->ReadDword(px);
				str->ReadDword(py);

				// in PST the coordinates are stored in small map space
				// our MapControl wants them in large map space so we must convert
				// its what other games use and its what our custom map note code uses
				const Size mapsize = map->GetSize();
				point.x = px * double(mapsize.w) / map->SmallMap->Frame.w;
				point.y = py * double(mapsize.h) / map->SmallMap->Frame.h;

				char bytes[501]; // 500 + null
				str->Read(bytes, 500 );
				bytes[500] = '\0';
				String* text = StringFromCString(bytes);
				ieDword readonly;
				str->ReadDword(readonly); //readonly == 1
				if (readonly) {
					map->AddMapNote(point, 0, std::move(*text), true);
				} else {
					map->AddMapNote(point, 1, std::move(*text), false);
				}
				delete text;
				str->Seek(20, GEM_CURRENT_POS);
			}
		}
	} else {
		for (ieDword i = 0; i < NoteCount; i++) {
			str->ReadPoint(point);
			ieStrRef strref = ieStrRef::INVALID;
			str->ReadStrRef(strref);
			ieWord location; // (0=Extenal (TOH/TOT), 1=Internal (TLK)
			str->ReadWord(location);
			ieWord color;
			str->ReadWord(color);
			// dword: ID in bg2
			str->Seek( 40, GEM_CURRENT_POS );
			// FIXME: do any other games have read only notes?
			// BG2 allows editing the builtin notes, PST does not, what about others?
			map->AddMapNote(point, color, strref, false);
		}
	}

	//this is a ToB feature (saves the unexploded projectiles)
	Log(DEBUG, "AREImporter", "Loading traps");
	for (ieDword i = 0; i < TrapCount; i++) {
		ResRef TrapResRef;
		ieDword TrapEffOffset;
		ieWord TrapSize, ProID;
		Point pos;
		ieDword Ticks;
		ieByte TargetType;
		ieByte Owner;

		str->Seek( TrapOffset + ( i * 0x1c ), GEM_STREAM_START );

		str->ReadResRef( TrapResRef );
		str->ReadDword(TrapEffOffset);
		str->ReadWord(TrapSize);
		str->ReadWord(ProID);
		str->ReadDword(Ticks);  //actually, delaycount/repetitioncount
		str->ReadPoint(point);
		str->Seek(2, GEM_CURRENT_POS); // unknown/unused 'Z'
		str->Read(&TargetType, 1); //according to dev info, this is 'targettype'; "Enemy-ally targeting" on IESDP
		str->Read(&Owner, 1); // party member index that created this projectile (0-5)
		int TrapEffectCount = TrapSize/0x108;
		if(TrapEffectCount*0x108!=TrapSize) {
			Log(ERROR, "AREImporter", "TrapEffectSize in game: {} != {}. Clearing it",
				TrapSize, TrapEffectCount*0x108);
				continue;
		}
		//The projectile is always created, the worst that can happen
		//is a dummy projectile
		//The projectile ID is 214 for TRAPSNAR
		//It is off by one compared to projectl.ids, but the same as missile.ids
		Projectile *pro = core->GetProjectileServer()->GetProjectileByIndex(ProID-1);

		EffectQueue fxqueue = EffectQueue();
		DataStream *fs = new SlicedStream( str, TrapEffOffset, TrapSize);

		ReadEffects(fs, &fxqueue, TrapEffectCount);
		const Actor *caster = core->GetGame()->FindPC(Owner + 1);
		pro->SetEffects(std::move(fxqueue));
		if (caster) {
			// Since the level info isn't stored, we assume it's the same as if the trap was just placed.
			// It matters for the normal thief traps (they scale with level 4 times), while the rest don't scale.
			// To be more flexible and better handle disabled dualclasses, we don't hardcode it to the thief level.
			// Perhaps simplify and store the level in Z? Would need a check in the original (don't break saves).
			ieDword level = caster->GetThiefLevel();
			pro->SetCaster(caster->GetGlobalID(), level ? level : caster->GetXPLevel(false));
		}
		map->AddProjectile(pro, pos, pos);
	}

	Log(DEBUG, "AREImporter", "Loading tiles");
	//Loading Tiled objects (if any)
	str->Seek( TileOffset, GEM_STREAM_START );
	for (ieDword i = 0; i < TileCount; i++) {
		ieVariable Name;
		ResRef ID;
		ieDword Flags;
		// these fields could be different size: ieDword ClosedCount, OpenCount;
		ieWord ClosedCount, OpenCount;
		ieDword ClosedIndex, OpenIndex;
		str->ReadVariable(Name);
		str->ReadResRef( ID );
		str->ReadDword(Flags);
		//IE dev info says this:
		str->ReadDword(OpenIndex); // PrimarySearchSquareStart in bg2
		str->ReadWord(OpenCount); // PrimarySearchSquareCount
		str->ReadWord(ClosedCount); // SecondarySearchSquareCount
		str->ReadDword(ClosedIndex); // SecondarySearcHSquareStart
		//end of disputed section

		str->Seek(48, GEM_CURRENT_POS); // 12 reserved dwords
		//absolutely no idea where these 'tile indices' are stored
		//are they tileset tiles or impeded block tiles
		map->TMap->AddTile( ID, Name, Flags, NULL,0, NULL, 0 );
	}

	Log(DEBUG, "AREImporter", "Loading explored bitmap");
	ieDword mapSize = ieDword(map->ExploredBitmap.Bytes());
	assert(ExploredBitmapSize <= mapSize);
	mapSize = std::min(mapSize, ExploredBitmapSize);
	str->Seek(ExploredBitmapOffset, GEM_STREAM_START);
	str->Read(map->ExploredBitmap.begin(), mapSize);

	Log(DEBUG, "AREImporter", "Loading wallgroups");
	map->SetWallGroups(tmm->GetWallGroups());
	//setting up doors
	for (ieDword i = 0; i < DoorsCount; i++) {
		Door *door = tm->GetDoor(i);
		door->SetDoorOpen(door->IsOpen(), false, 0);
	}

	return map;
}

void AREImporter::AdjustPSTFlags(AreaAnimation &areaAnim) const
{
	/**
	 * For PST, map animation flags work differently to a degree that they
	 * should not be mixed together with the rest as they even tend to
	 * break things (like stopping early, hiding under FoW).
	 *
	 * So far, a better approximation towards handling animations is:
	 * - zero everything
	 * - always set A_ANI_SYNC
	 * - copy/map known flags (A_ANI_ACTIVE, A_ANI_NO_WALL, A_ANI_BLEND)
	 *
	 * Note that WF_COVERANIMS is enabled by default for PST, so ANI_NO_WALL
	 *   is important.
	 *
	 * The actual use of bits in PST map anims isn't fully solved here.
	 */

	#define PST_ANI_NO_WALL 0x0008  // A_ANI_PLAYONCE in other games
	#define PST_ANI_BLEND 0x0100  // A_ANI_BACKGROUND in other games

	areaAnim.Flags = 0;             // Clear everything

	// Set default-on flags (currently only A_ANI_SYNC)
	areaAnim.Flags |= A_ANI_SYNC;

	// Copy still-relevant A_ANI_* flags
	areaAnim.Flags |= areaAnim.originalFlags & A_ANI_ACTIVE;

	// Map known flags
	if (areaAnim.originalFlags & PST_ANI_BLEND) {
		areaAnim.Flags |= A_ANI_BLEND;
	}
	if (areaAnim.originalFlags & PST_ANI_NO_WALL) {
		areaAnim.Flags |= A_ANI_NO_WALL;
	}
}

void AREImporter::ReadEffects(DataStream *ds, EffectQueue *fxqueue, ieDword EffectsCount) const
{
	PluginHolder<EffectMgr> eM = MakePluginHolder<EffectMgr>(IE_EFF_CLASS_ID);
	eM->Open(ds);

	for (unsigned int i = 0; i < EffectsCount; i++) {
		fxqueue->AddEffect(eM->GetEffectV20());
	}
}

int AREImporter::GetStoredFileSize(Map *map)
{
	int headersize = map->version+0x11c;
	ActorOffset = headersize;

	//get only saved actors (no familiars or partymembers)
	//summons?
	ActorCount = (ieWord) map->GetActorCount(false);
	headersize += ActorCount * 0x110;

	auto am = GetImporter<ActorMgr>(IE_CRE_CLASS_ID);
	EmbeddedCreOffset = headersize;

	for (unsigned int i = 0; i < ActorCount; i++) {
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
	for (unsigned int i = 0; i < InfoPointsCount; i++) {
		const InfoPoint *ip = map->TMap->GetInfoPoint(i);
		if (ip->outline) {
			VerticesCount+=ip->outline->Count();
		} else {
			VerticesCount++;
		}
	}
	for (unsigned int i = 0; i < ContainersCount; i++) {
		const Container *c = map->TMap->GetContainer(i);
		if (c->outline)
			VerticesCount+=c->outline->Count();
	}
	for (unsigned int i = 0; i < DoorsCount; i++) {
		const Door *d = map->TMap->GetDoor(i);
		auto open = d->OpenTriggerArea();
		auto closed = d->ClosedTriggerArea();
		if (open)
			VerticesCount += open->Count();
		if (closed)
			VerticesCount += closed->Count();

		VerticesCount += d->open_ib.size() + d->closed_ib.size();
	}
	headersize += VerticesCount * 4;
	AmbiOffset = headersize;

	headersize += SavedAmbientCount(map) * 0xd4;
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

	ExploredBitmapSize = map->ExploredBitmap.Bytes();
	headersize += ExploredBitmapSize;
	EffectOffset = headersize;

	proIterator piter;
	TrapCount = (ieDword) map->GetTrapCount(piter);
	for (unsigned int i = 0; i < TrapCount; i++) {
		const Projectile *pro = map->GetNextTrap(piter);
		if (pro) {
			const EffectQueue& fxqueue = pro->GetEffects();
			if (fxqueue) {
				headersize += fxqueue.GetSavedEffectsCount() * 0x108;
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

int AREImporter::PutHeader(DataStream *stream, const Map *map) const
{
	char Signature[9];
	int pst = core->HasFeature( GF_AUTOMAP_INI );

	strlcpy(Signature, "AREAV1.0", 9);
	if (map->version==16) {
		Signature[5]='9';
		Signature[7]='1';
	}
	stream->Write( Signature, 8);
	stream->WriteResRef( map->WEDResRef);
	uint32_t time = core->GetGame()->GameTime;
	stream->WriteDword(time); //lastsaved
	stream->WriteDword(map->AreaFlags);

	stream->WriteFilling(12); // northref
	stream->WriteFilling(12); // westref
	stream->WriteFilling(12); // southref
	stream->WriteFilling(12); // eastref

	stream->WriteWord(map->AreaType);
	stream->WriteWord(map->Rain);
	stream->WriteWord(map->Snow);
	stream->WriteWord(map->Fog);
	stream->WriteWord(map->Lightning);
	stream->WriteFilling(2);

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
		stream->WriteFilling(6);
		stream->WriteFilling(8);
	}

	stream->WriteDword(ActorOffset);
	stream->WriteWord(ActorCount);
	stream->WriteWord(InfoPointsCount);
	stream->WriteDword(InfoPointsOffset);
	stream->WriteDword(SpawnOffset);
	stream->WriteDword(SpawnCount);
	stream->WriteDword(EntrancesOffset);
	stream->WriteDword(EntrancesCount);
	stream->WriteDword(ContainersOffset);
	stream->WriteWord(ContainersCount);
	stream->WriteWord(ItemsCount);
	stream->WriteDword(ItemsOffset);
	stream->WriteDword(VerticesOffset);
	stream->WriteWord(VerticesCount);
	stream->WriteWord(SavedAmbientCount(map));
	stream->WriteDword(AmbiOffset);
	stream->WriteDword(VariablesOffset);
	stream->WriteDword(VariablesCount);
	stream->WriteFilling(4);

	//the saved area script is in the last script slot!
	const GameScript *s = map->Scripts[MAX_SCRIPTS - 1];
	if (s) {
		stream->WriteResRefLC(s->GetName());
	} else {
		stream->WriteFilling(8);
	}
	stream->WriteDword(ExploredBitmapSize);
	stream->WriteDword(ExploredBitmapOffset);
	stream->WriteDword(DoorsCount);
	stream->WriteDword(DoorsOffset);
	stream->WriteDword(AnimCount);
	stream->WriteDword(AnimOffset);
	stream->WriteDword(TileCount);
	stream->WriteDword(TileOffset);
	stream->WriteDword(SongHeader);
	stream->WriteDword(RestHeader);
	//an empty dword for pst
	int i = 56;
	if (pst) {
		stream->WriteDword(0xffffffff);
		i=52;
	}
	stream->WriteDword(NoteOffset);
	stream->WriteDword(NoteCount);
	stream->WriteDword(TrapOffset);
	stream->WriteDword(TrapCount);
	stream->WriteResRef( map->Dream[0] );
	stream->WriteResRef( map->Dream[1] );
	//usually 56 empty bytes (but pst used up 4 elsewhere)
	stream->WriteFilling(i);
	return 0;
}

int AREImporter::PutDoors(DataStream *stream, const Map *map, ieDword &VertIndex) const
{
	for (unsigned int i=0;i<DoorsCount;i++) {
		Door *d = map->TMap->GetDoor(i);

		stream->WriteVariable(d->GetScriptName());
		stream->WriteResRef( d->ID);
		if (map->version == 16) {
			d->Flags = FixIWD2DoorFlags(d->Flags, true);
		}
		stream->WriteDword(d->Flags);
		stream->WriteDword(VertIndex);
		auto open = d->OpenTriggerArea();
		ieWord tmpWord = open ? open->Count() : 0;
		stream->WriteWord(tmpWord);
		VertIndex += tmpWord;
		auto closed = d->ClosedTriggerArea();
		tmpWord = closed ? closed->Count() : 0;
		stream->WriteWord(tmpWord);
		stream->WriteDword(VertIndex);
		VertIndex += tmpWord;
		//open bounding box
		stream->WriteWord(d->OpenBBox.x);
		stream->WriteWord(d->OpenBBox.y);
		stream->WriteWord(d->OpenBBox.x + d->OpenBBox.w);
		stream->WriteWord(d->OpenBBox.y + d->OpenBBox.h);
		//closed bounding box
		stream->WriteWord(d->ClosedBBox.x);
		stream->WriteWord(d->ClosedBBox.y);
		stream->WriteWord(d->ClosedBBox.x + d->ClosedBBox.w);
		stream->WriteWord(d->ClosedBBox.y + d->ClosedBBox.h);
		//open and closed impeded blocks
		stream->WriteDword(VertIndex);
		tmpWord = (ieWord) d->open_ib.size();
		stream->WriteWord(tmpWord);
		VertIndex += tmpWord;
		tmpWord = (ieWord) d->closed_ib.size();
		stream->WriteWord(tmpWord);
		stream->WriteDword(VertIndex);
		VertIndex += tmpWord;
		stream->WriteWord(d->hp);
		stream->WriteWord(d->ac);
		stream->WriteResRef( d->OpenSound);
		stream->WriteResRef( d->CloseSound);
		stream->WriteDword(d->Cursor);
		stream->WriteWord(d->TrapDetectionDiff);
		stream->WriteWord(d->TrapRemovalDiff);
		stream->WriteWord(d->Trapped);
		stream->WriteWord(d->TrapDetected);
		stream->WritePoint(d->TrapLaunch);
		stream->WriteResRefLC(d->KeyResRef);
		const GameScript *s = d->Scripts[0];
		if (s) {
			stream->WriteResRefLC(s->GetName());
		} else {
			stream->WriteFilling(8);
		}
		stream->WriteDword(d->DiscoveryDiff);
		//lock difficulty field
		stream->WriteDword(d->LockDifficulty);
		//opening locations
		stream->WritePoint(d->toOpen[0]);
		stream->WritePoint(d->toOpen[1]);
		stream->WriteStrRef(d->OpenStrRef);
		if (core->HasFeature(GF_AUTOMAP_INI) ) {
			stream->WriteString(d->LinkedInfo, 24);
		} else {
			stream->WriteVariable(d->LinkedInfo);
		}
		stream->WriteStrRef(d->NameStrRef);
		stream->WriteResRef( d->GetDialog());
		if (core->HasFeature(GF_AUTOMAP_INI) ) {
			stream->WriteFilling(8);
		}
	}
	return 0;
}

int AREImporter::PutPoints(DataStream *stream, const std::vector<Point>& points) const
{
	for (const Point& p : points) {
		stream->WritePoint(p);
	}
	return 0;
}

int AREImporter::PutVertices(DataStream *stream, const Map *map) const
{
	//regions
	for (unsigned int i = 0; i < InfoPointsCount; i++) {
		const InfoPoint *ip = map->TMap->GetInfoPoint(i);
		if (ip->outline) {
			PutPoints(stream, ip->outline->vertices);
		} else {
			Point origin = ip->BBox.origin;
			stream->WritePoint(origin);
		}
	}
	//containers
	for (unsigned int i = 0; i < ContainersCount; i++) {
		const Container *c = map->TMap->GetContainer(i);
		if (c->outline) {
			PutPoints(stream, c->outline->vertices);
		}
	}
	//doors
	for (unsigned int i = 0; i < DoorsCount; i++) {
		const Door *d = map->TMap->GetDoor(i);
		auto open = d->OpenTriggerArea();
		auto closed = d->ClosedTriggerArea();
		if (open)
			PutPoints(stream, open->vertices);
		if (closed)
			PutPoints(stream, closed->vertices);
		PutPoints(stream, d->open_ib);
		PutPoints(stream, d->closed_ib);
	}
	return 0;
}

int AREImporter::PutItems(DataStream *stream, const Map *map) const
{
	for (unsigned int i=0;i<ContainersCount;i++) {
		const Container *c = map->TMap->GetContainer(i);

		for(int j=0;j<c->inventory.GetSlotCount();j++) {
			const CREItem *ci = c->inventory.GetSlotItem(j);

			stream->WriteResRefUC(ci->ItemResRef);
			stream->WriteWord(ci->Expired);
			stream->WriteWord(ci->Usages[0]);
			stream->WriteWord(ci->Usages[1]);
			stream->WriteWord(ci->Usages[2]);
			stream->WriteDword(ci->Flags);
		}
	}
	return 0;
}

int AREImporter::PutContainers(DataStream *stream, const Map *map, ieDword &VertIndex) const
{
	ieDword ItemIndex = 0;

	for (unsigned int i=0;i<ContainersCount;i++) {
		const Container *c = map->TMap->GetContainer(i);

		//this is the editor name
		stream->WriteVariable(c->GetScriptName());
		stream->WritePoint(c->Pos);
		stream->WriteWord(c->containerType);
		stream->WriteWord(c->LockDifficulty);
		stream->WriteDword(c->Flags);
		stream->WriteWord(c->TrapDetectionDiff);
		stream->WriteWord(c->TrapRemovalDiff);
		stream->WriteWord(c->Trapped);
		stream->WriteWord(c->TrapDetected);
		stream->WritePoint(c->TrapLaunch);
		//outline bounding box
		stream->WriteWord(c->BBox.x);
		stream->WriteWord(c->BBox.y);
		stream->WriteWord(c->BBox.x + c->BBox.w);
		stream->WriteWord(c->BBox.y + c->BBox.h);
		//item index and offset
		ieDword tmpDword = c->inventory.GetSlotCount();
		stream->WriteDword(ItemIndex);
		stream->WriteDword(tmpDword);
		ItemIndex += tmpDword;
		const GameScript *s = c->Scripts[0];
		if (s) {
			stream->WriteResRefLC(s->GetName() );
		} else {
			stream->WriteFilling(8);
		}
		//outline polygon index and count
		ieWord tmpWord = (c->outline) ? c->outline->Count() : 0;
		stream->WriteDword(VertIndex);
		stream->WriteWord(tmpWord);
		VertIndex +=tmpWord;
		tmpWord = 0;
		stream->WriteWord(tmpWord); //vertex count is made short
		//this is the real scripting name
		stream->WriteVariable(c->GetScriptName());
		stream->WriteResRefLC(c->KeyResRef);
		stream->WriteDword(0); //unknown80
		stream->WriteStrRef(c->OpenFail);
		stream->WriteFilling(56); //unknown or unused stuff
	}
	return 0;
}

int AREImporter::PutRegions(DataStream *stream, const Map *map, ieDword &VertIndex) const
{
	for (unsigned int i=0;i<InfoPointsCount;i++) {
		const InfoPoint *ip = map->TMap->GetInfoPoint(i);

		stream->WriteVariable(ip->GetScriptName());
		//this is a hack, we abuse a coincidence
		//ST_PROXIMITY = 1, ST_TRIGGER = 2, ST_TRAVEL = 3
		//translates to trap = 0, info = 1, travel = 2
		stream->WriteWord(((ieWord) ip->Type) - 1);
		//outline bounding box
		stream->WriteWord(ip->BBox.x);
		stream->WriteWord(ip->BBox.y);
		stream->WriteWord(ip->BBox.x + ip->BBox.w);
		stream->WriteWord(ip->BBox.y + ip->BBox.h);
		ieWord tmpWord = (ip->outline) ? ip->outline->Count() : 1;
		stream->WriteWord(tmpWord);
		stream->WriteDword(VertIndex);
		VertIndex += tmpWord;
		stream->WriteDword(0); //unknown30
		stream->WriteDword(ip->Cursor);
		stream->WriteResRefUC(ip->Destination);
		stream->WriteVariableUC(ip->EntranceName);
		stream->WriteDword(ip->Flags);
		stream->WriteStrRef(ip->StrRef);
		stream->WriteWord(ip->TrapDetectionDiff);
		stream->WriteWord(ip->TrapRemovalDiff);
		stream->WriteWord(ip->Trapped); //unknown???
		stream->WriteWord(ip->TrapDetected);
		stream->WritePoint(ip->TrapLaunch);
		stream->WriteResRefLC(ip->KeyResRef);
		const GameScript *s = ip->Scripts[0];
		if (s) {
			stream->WriteResRefLC(s->GetName());
		} else {
			stream->WriteFilling(8);
		}
		stream->WritePoint(ip->UsePoint);
		if (16 == map->version) {
			stream->WriteDword(ip->UsePoint.x);
			stream->WriteDword(ip->UsePoint.y);
			stream->WriteFilling(28); //unknown
		} else {
			stream->WriteFilling(36); //unknown
		}
		//these are probably only in PST
		stream->WriteResRef( ip->EnterWav);
		stream->WritePoint(ip->TalkPos);
		stream->WriteStrRef(ip->DialogName);
		stream->WriteResRef( ip->GetDialog());
	}
	return 0;
}

int AREImporter::PutSpawns(DataStream *stream, const Map *map) const
{
	ieWord tmpWord;

	for (unsigned int i=0;i<SpawnCount;i++) {
		const Spawn *sp = map->GetSpawn(i);

		stream->WriteVariable(sp->Name);
		tmpWord = (ieWord) sp->Pos.x;
		stream->WriteWord(tmpWord);
		tmpWord = (ieWord) sp->Pos.y;
		stream->WriteWord(tmpWord);
		tmpWord = ieWord(sp->Creatures.size());
		int j;
		for (j = 0;j < tmpWord; j++) {
			stream->WriteResRef( sp->Creatures[j] );
		}
		while( j++<MAX_RESCOUNT) {
			stream->WriteFilling(8);
		}
		stream->WriteWord(tmpWord);
		stream->WriteWord(sp->Difficulty);
		stream->WriteWord(sp->Frequency);
		stream->WriteWord(sp->Method);
		stream->WriteDword(sp->sduration); //spawn duration
		stream->WriteWord(sp->rwdist);     //random walk distance
		stream->WriteWord(sp->owdist);     //other walk distance
		stream->WriteWord(sp->Maximum);
		stream->WriteWord(sp->Enabled);
		stream->WriteDword(sp->appearance);
		stream->WriteWord(sp->DayChance);
		stream->WriteWord(sp->NightChance);
		stream->WriteFilling(56); //most likely unused crap
	}
	return 0;
}

void AREImporter::PutScript(DataStream *stream, const Actor *ac, unsigned int index) const
{
	const GameScript *s = ac->Scripts[index];
	if (s) {
		stream->WriteResRefLC(s->GetName());
	} else {
		stream->WriteFilling(8);
	}
}

int AREImporter::PutActors(DataStream *stream, const Map *map) const
{
	ieDword CreatureOffset = EmbeddedCreOffset;

	auto am = GetImporter<ActorMgr>(IE_CRE_CLASS_ID);
	for (unsigned int i = 0; i < ActorCount; i++) {
		const Actor *ac = map->GetActor(i, false);

		stream->WriteVariable(ac->GetScriptName());
		stream->WritePoint(ac->Pos);
		stream->WritePoint(ac->HomeLocation);

		stream->WriteDword(0); //used fields flag always 0 for saved areas
		stream->WriteWord(ac->Spawned);
		stream->WriteFilling(1); // letter
		stream->WriteScalar(ac->DifficultyMargin);
		stream->WriteDword(0); //actor animation, unused
		stream->WriteWord(ac->GetOrientation());
		stream->WriteWord(0); //unknown
		stream->WriteDword(ac->RemovalTime);
		stream->WriteWord(ac->maxWalkDistance);
		stream->WriteWord(0); //more unknowns
		stream->WriteDword(ac->appearance);
		stream->WriteDword(ac->TalkCount);
		stream->WriteResRefLC(ac->GetDialog());
		PutScript(stream, ac, SCR_OVERRIDE);
		PutScript(stream, ac, SCR_GENERAL);
		PutScript(stream, ac, SCR_CLASS);
		PutScript(stream, ac, SCR_RACE);
		PutScript(stream, ac, SCR_DEFAULT);
		PutScript(stream, ac, SCR_SPECIFICS);
		//creature reference is empty because we are embedding it
		//the original engine used a '*'
		stream->WriteFilling(8);
		stream->WriteDword(CreatureOffset);
		ieDword CreatureSize = am->GetStoredFileSize(ac);
		stream->WriteDword(CreatureSize);
		CreatureOffset += CreatureSize;
		PutScript(stream, ac, SCR_AREA);
		stream->WriteFilling(120);
	}

	CreatureOffset = EmbeddedCreOffset;
	for (unsigned int i = 0; i < ActorCount; i++) {
		assert(stream->GetPos() == CreatureOffset);
		const Actor *ac = map->GetActor(i, false);

		//reconstructing offsets again
		CreatureOffset += am->GetStoredFileSize(ac);
		am->PutActor( stream, ac);
	}
	assert(stream->GetPos() == CreatureOffset);

	return 0;
}

int AREImporter::PutAnimations(DataStream *stream, const Map *map) const
{
	auto iter = map->GetFirstAnimation();
	while(const AreaAnimation *an = map->GetNextAnimation(iter)) {
		stream->WriteVariable(an->Name);
		stream->WritePoint(an->Pos);
		stream->WriteDword(an->appearance);
		stream->WriteResRef( an->BAM);
		stream->WriteWord(an->sequence);
		stream->WriteWord(an->frame);

		if (core->HasFeature(GF_AUTOMAP_INI)) {
			/* PST toggles the active bit only, and we need to keep the rest. */
			ieDword flags = (an->originalFlags & ~A_ANI_ACTIVE) | (an->Flags & A_ANI_ACTIVE);
			stream->WriteDword(flags);
		} else {
			stream->WriteDword(an->Flags);
		}

		stream->WriteScalar(an->height);
		stream->WriteWord(an->transparency);
		stream->WriteWord(an->startFrameRange); //used by A_ANI_RANDOM_START
		stream->Write( &an->startchance,1);
		stream->Write( &an->skipcycle,1);
		stream->WriteResRef( an->PaletteRef);
		stream->WriteDword(an->unknown48);//seems utterly unused
	}
	return 0;
}

int AREImporter::PutEntrances(DataStream *stream, const Map *map) const
{
	for (unsigned int i=0;i<EntrancesCount;i++) {
		const Entrance *e = map->GetEntrance(i);

		stream->WriteVariable(e->Name);
		stream->WritePoint(e->Pos);
		stream->WriteWord(e->Face);
		//a large empty piece of crap
		stream->WriteFilling(66);
	}
	return 0;
}

int AREImporter::PutVariables(DataStream *stream, const Map *map) const
{
	Variables::iterator pos=NULL;
	ieDword value;

	for (unsigned int i=0;i<VariablesCount;i++) {
		Variables::key_t name;
		pos = map->locals->GetNextAssoc(pos, name, value);
		size_t len = name.length();
		stream->Write(name.c_str(), len);
		if (len < 40) {
			stream->WriteFilling(40 - len);
		}
		stream->WriteDword(value);
		//40 bytes of empty crap
		stream->WriteFilling(40);
	}
	return 0;
}

int AREImporter::PutAmbients(DataStream *stream, const Map *map) const
{
	for (const auto& am : map->GetAmbients()) {
		if (am->flags & IE_AMBI_NOSAVE) continue;
		stream->WriteVariable(am->name);
		stream->WritePoint(am->origin);
		stream->WriteWord(am->radius);
		stream->WriteFilling(2);
		stream->WriteDword(am->pitchVariance);
		stream->WriteWord(am->gainVariance);
		stream->WriteWord(am->gain);
		size_t j = 0;
		for (; j < am->sounds.size(); j++) {
			stream->WriteResRef( am->sounds[j] );
		}
		while( j++<MAX_RESCOUNT) {
			stream->WriteFilling(8);
		}
		stream->WriteWord(am->sounds.size());
		stream->WriteFilling(2);
		stream->WriteDword(ieDword(am->interval / 1000));
		stream->WriteDword(ieDword(am->intervalVariance / 1000));
		stream->WriteDword(am->appearance);
		stream->WriteDword(am->flags);
		stream->WriteFilling(64);
	}
	return 0;
}

int AREImporter::PutMapnotes(DataStream *stream, const Map *map) const
{
	//different format
	int pst = core->HasFeature( GF_AUTOMAP_INI );

	for (unsigned int i=0;i<NoteCount;i++) {
		const MapNote& mn = map->GetMapNote(i);

		if (pst) {
			// in PST the coordinates are stored in small map space
			const Size& mapsize = map->GetSize();
			stream->WriteDword(mn.Pos.x * double(map->SmallMap->Frame.w) / mapsize.w);
			stream->WriteDword(mn.Pos.y * double(map->SmallMap->Frame.h) / mapsize.h);

			size_t len = 0;
			// limited to 500 *bytes* of text, convert to a multibyte encoding.
			// we convert to MB because it fits more than if we wrote the wide characters
			std::string mbstring = MBStringFromString(mn.text);
			// FIXME: depends on locale blah blah (see MBStringFromString definition)
			// only care about number of bytes before null so strlen is what we want despite being MB string
			len = std::min<size_t>(mbstring.length(), 500);
			stream->Write(mbstring.c_str(), len);

			// pad the remaining space
			size_t x = 500 - len;
			for (size_t j = 0; j < x / 8; ++j) {
				stream->WriteFilling(8);
			}
			x = x%8;
			if (x) {
				stream->WriteFilling(x);
			}
			stream->WriteDword(mn.readonly);
			for (x=0;x<5;x++) { //5 empty dwords
				stream->WriteFilling(4);
			}
		} else {
			stream->WritePoint(mn.Pos);
			stream->WriteStrRef(mn.strref);
			stream->WriteWord(mn.Pos.y);
			stream->WriteWord(mn.color);
			stream->WriteDword(1);
			for (int x = 0; x < 9; ++x) { //9 empty dwords
				stream->WriteFilling(4);
			}
		}
	}
	return 0;
}

int AREImporter::PutEffects(DataStream *stream, const EffectQueue& fxqueue) const
{
	PluginHolder<EffectMgr> eM = MakePluginHolder<EffectMgr>(IE_EFF_CLASS_ID);
	assert(eM != nullptr);

	auto f = fxqueue.GetFirstEffect();
	ieDword EffectsCount = fxqueue.GetSavedEffectsCount();
	for(unsigned int i=0;i<EffectsCount;i++) {
		const Effect *fx = fxqueue.GetNextSavedEffect(f);

		assert(fx!=NULL);

		eM->PutEffectV2(stream, fx);
	}
	return 0;
}

int AREImporter::PutTraps(DataStream *stream, const Map *map) const
{
	ieDword Offset;
	ResRef name;
	ieWord type = 0;
	Point dest(0,0);

	Offset = EffectOffset;
	proIterator iter;
	ieDword i = map->GetTrapCount(iter);
	while(i--) {
		ieWord tmpWord = 0;
		ieByte tmpByte = 0xff;
		const Projectile *pro = map->GetNextTrap(iter);
		if (pro) {
			//The projectile ID is based on missile.ids which is
			//off by one compared to projectl.ids
			type = pro->GetType()+1;
			dest = pro->GetDestination();
			const ResRef& proName = pro->GetName();
			name = proName;
			const EffectQueue& fxqueue = pro->GetEffects();
			if (fxqueue) {
				tmpWord = fxqueue.GetSavedEffectsCount();
			}
			ieDword ID = pro->GetCaster();
			// lookup caster via Game, since the the current map can already be empty when switching them
			const Actor *actor = core->GetGame()->GetActorByGlobalID(ID);
			//0xff if not in party
			//party slot if in party
			if (actor) tmpByte = (ieByte) (actor->InParty-1);
		}

		stream->WriteResRefUC(name);
		stream->WriteDword(Offset);
		//size of fxqueue;
		assert(tmpWord<256);
		tmpWord *= 0x108;
		Offset += tmpWord;
		stream->WriteWord(tmpWord);  //size in bytes
		stream->WriteWord(type);     //missile.ids
		stream->WriteDword(0); // unknown field, Ticks
		stream->WritePoint(dest);
		stream->WriteWord(0); // unknown field, Z
		stream->Write(&tmpByte, 1);   // unknown field, TargetType
		stream->Write(&tmpByte, 1);   // Owner
	}
	return 0;
}

int AREImporter::PutExplored(DataStream *stream, const Map *map) const
{
	stream->Write(map->ExploredBitmap.begin(), ExploredBitmapSize);
	return 0;
}

int AREImporter::PutTiles(DataStream *stream, const Map *map) const
{
	for (unsigned int i=0;i<TileCount;i++) {
		const TileObject *am = map->TMap->GetTile(i);
		stream->WriteVariable(am->Name);
		stream->WriteResRef( am->Tileset );
		stream->WriteDword(am->Flags);
		stream->WriteDword(am->opencount);
		//can't write tiles, otherwise now we should write a tile index
		stream->WriteDword(0);
		stream->WriteDword(am->closedcount);
		//can't write tiles otherwise now we should write a tile index
		stream->WriteDword(0);
		stream->WriteFilling(48);
	}
	return 0;
}

ieWord AREImporter::SavedAmbientCount(const Map* map) const
{
	int count = 0;
	for (const Ambient* am : map->GetAmbients())
	{
		if (am->flags & IE_AMBI_NOSAVE) continue;
		++count;
	}
	return count;
}

int AREImporter::PutMapAmbients(DataStream *stream, const Map *map) const
{
	//day
	stream->WriteResRef(map->dayAmbients.Ambient1);
	stream->WriteResRef(map->dayAmbients.Ambient2);
	stream->WriteDword(map->dayAmbients.AmbientVol);
	//night
	stream->WriteResRef(map->nightAmbients.Ambient1);
	stream->WriteResRef(map->nightAmbients.Ambient2);
	stream->WriteDword(map->nightAmbients.AmbientVol);
	//song flag
	stream->WriteDword(map->reverbID);
	//lots of empty crap (15x4)
	stream->WriteFilling(60);
	return 0;
}

int AREImporter::PutRestHeader(DataStream *stream, const Map *map) const
{
	stream->WriteFilling(32); //empty label
	for (const auto& ref : map->RestHeader.Strref) {
		stream->WriteStrRef(ref);
	}
	for (const auto& ref : map->RestHeader.CreResRef) {
		stream->WriteResRef(ref);
	}
	stream->WriteWord(map->RestHeader.CreatureNum);
	stream->WriteWord(map->RestHeader.Difficulty);
	stream->WriteDword(map->RestHeader.sduration);
	stream->WriteWord(map->RestHeader.rwdist);
	stream->WriteWord(map->RestHeader.owdist);
	stream->WriteWord(map->RestHeader.Maximum);
	stream->WriteWord(map->RestHeader.Enabled);
	stream->WriteWord(map->RestHeader.DayChance);
	stream->WriteWord(map->RestHeader.NightChance);
	stream->WriteFilling(56);
	return 0;
}

/* no saving of tiled objects, are they used anywhere? */
int AREImporter::PutArea(DataStream *stream, const Map *map) const
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

	proIterator iter;
	ieDword i = map->GetTrapCount(iter);
	while(i--) {
		const Projectile *trap = map->GetNextTrap(iter);
		if (!trap) {
			continue;
		}

		const EffectQueue& fxqueue = trap->GetEffects();

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
	
	for (const auto& list : map->SongList) {
		stream->WriteDword(list);
	}

	ret = PutMapAmbients(stream, map);
	if (ret) {
		return ret;
	}

	ret = PutRestHeader( stream, map);

	return ret;
}

#include "plugindef.h"

GEMRB_PLUGIN(0x145B60F0, "ARE File Importer")
PLUGIN_CLASS(IE_ARE_CLASS_ID, ImporterPlugin<AREImporter>)
END_PLUGIN()
