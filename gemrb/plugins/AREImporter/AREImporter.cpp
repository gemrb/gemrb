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

#include "ie_cursors.h"
#include "strrefs.h"

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
#include "PluginMgr.h"
#include "ProjectileServer.h"
#include "RNG.h"

#include "GameScript/GameScript.h"
#include "Plugins/TileMapMgr.h"
#include "Scriptable/Container.h"
#include "Scriptable/Door.h"
#include "Scriptable/InfoPoint.h"
#include "Scriptable/TileObject.h"
#include "Streams/FileStream.h"
#include "Streams/SlicedStream.h"

#include <cstdlib>

using namespace GemRB;

#define DEF_OPEN   0
#define DEF_CLOSE  1
#define DEF_HOPEN  2
#define DEF_HCLOSE 3

//something non signed, non ascii
#define UNINITIALIZED_BYTE 0x11

static std::vector<TrackingData> tracks;
PluginHolder<DataFileMgr> INInote;

static void ReadAutonoteINI()
{
	INInote = MakePluginHolder<DataFileMgr>(IE_INI_CLASS_ID);
	path_t tINInote = PathJoin(core->config.GamePath, "autonote.ini");
	FileStream* fs = FileStream::OpenFile(tINInote);
	INInote->Open(std::unique_ptr<DataStream> { fs });
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

	static const PathFinderCosts& Get()
	{
		static PathFinderCosts pathfinder;
		return pathfinder;
	}

private:
	PathFinderCosts() noexcept
	{
		AutoTable tm = gamedata->LoadTable("terrain");

		if (!tm) {
			return;
		}

		const char* poi;

		for (int i = 0; i < 16; i++) {
			poi = tm->QueryField(0, i).c_str();
			if (*poi != '*')
				Passable[i] = PathMapFlags(atoi(poi));
		}
		poi = tm->QueryField(1, 0).c_str();
		if (*poi != '*')
			NormalCost = atoi(poi);
		poi = tm->QueryField(1, 1).c_str();
		if (*poi != '*')
			AdditionalCost = atoi(poi);
	}

	PathFinderCosts(const PathFinderCosts&) = delete;
	PathFinderCosts(PathFinderCosts&&) = delete;
};

static int GetTrackString(const ResRef& areaName)
{
	bool trackFlag = DisplayMessage::HasStringReference(HCStrings::Tracking);

	if (tracks.empty()) {
		AutoTable tm = gamedata->LoadTable("tracking", true);
		if (!tm)
			return -1;
		TableMgr::index_t trackcount = tm->GetRowCount();
		tracks.resize(trackcount);
		for (TableMgr::index_t i = 0; i < trackcount; i++) {
			const char* poi = tm->QueryField(i, 0).c_str();
			if (poi[0] == 'O' && poi[1] == '_') {
				tracks[i].enabled = false;
				poi += 2;
			} else {
				tracks[i].enabled = trackFlag;
			}
			tracks[i].text = ieStrRef(atoi(poi));
			tracks[i].difficulty = tm->QueryFieldSigned<int>(i, 1);
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
	ResourceHolder<ImageMgr> im = gamedata->GetResourceHolder<ImageMgr>(resref);
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

// override some diagonal-only transitions to save on pathfinding time
static void OverrideMaterialMap(const ResRef& wedRef, Holder<Sprite2D> searchMap)
{
	AutoTable smOverride = gamedata->LoadTable("smoverri", true);
	if (!smOverride) return;

	TableMgr::index_t areaCount = smOverride->GetRowCount();
	for (TableMgr::index_t row = 0; row < areaCount; row++) {
		if (wedRef != smOverride->GetRowName(row)) continue;

		int x = smOverride->QueryFieldSigned<int>(row, 0);
		int y = smOverride->QueryFieldSigned<int>(row, 1);
		uint8_t material = smOverride->QueryFieldUnsigned<uint8_t>(row, 2);
		Point badPoint(x, y);
		auto it = searchMap->GetIterator();
		auto end = PixelFormatIterator::end(it);
		while (it != end) {
			if (it.Position() == badPoint) {
				*it = material;
				break;
			}
			++it;
		}
	}
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
	OverrideMaterialMap(wedref, searchmap);

	TmpResRef.Format("{:.6}HT", wedref);

	auto heightmap = LoadImageAs8bit(TmpResRef);
	if (!heightmap) {
		throw std::runtime_error("No heightmap available.");
	}

	const Size propsize(tm->XCellCount * 4, CeilDiv(tm->YCellCount * 64, 12));

	PixelFormat fmt = TileProps::pixelFormat;
	fmt.palette = lightmap->GetPalette();
	auto propImg = VideoDriver->CreateSprite(Region(Point(), propsize), nullptr, fmt);

	auto propit = propImg->GetIterator();
	auto end = Sprite2D::Iterator::end(propit);

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
			b = hmpal->GetColorAt(*hmit).r; // pick any channel, they are all the same
			++hmit;
		}

		if (lmit.clip.PointInside(pos)) {
			a = *lmit;
			++lmit;
		}

		propit.WriteRGBA(r, g, b, a);
	}

	return TileProps(std::move(propImg));
}

bool AREImporter::Import(DataStream* str)
{
	char Signature[8];
	str->Read(Signature, 8);

	if (strncmp(Signature, "AREAV1.0", 8) != 0) {
		if (strncmp(Signature, "AREAV9.1", 8) != 0) {
			return false;
		} else {
			bigheader = 16;
		}
	} else {
		bigheader = 0;
	}

	str->ReadResRef(WEDResRef);
	str->ReadDword(LastSave);
	str->ReadDword(AreaFlags);
	//skipping bg1 area connection fields
	str->Seek(0x48, GEM_STREAM_START);
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
	str->Seek(0x54 + bigheader, GEM_STREAM_START);
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
	str->ReadResRef(Script);
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
	if (core->HasFeature(GFFlags::AUTOMAP_INI)) {
		str->ReadDword(tmp); //skipping unknown in PST
	}
	str->ReadDword(NoteOffset);
	str->ReadDword(NoteCount);
	str->ReadDword(TrapOffset);
	str->ReadDword(TrapCount);
	str->ReadResRef(Dream1);
	str->ReadResRef(Dream2);
	// 56 bytes of reserved space
	return true;
}

//alter a map to the night/day version in case of an extended night map (bg2 specific)
//return true, if change happened, in which case a movie is played by the Game object
bool AREImporter::ChangeMap(Map* map, bool day_or_night)
{
	ResRef TmpResRef;

	//get the right tilemap name
	if (day_or_night) {
		TmpResRef = map->WEDResRef;
	} else {
		TmpResRef.Format("{:.7}N", map->WEDResRef);
	}
	PluginHolder<TileMapMgr> tmm = MakePluginHolder<TileMapMgr>(IE_WED_CLASS_ID);
	DataStream* wedfile = gamedata->GetResourceStream(TmpResRef, IE_WED_CLASS_ID);
	tmm->Open(wedfile);
	tmm->SetExtendedNight(!day_or_night);

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
		ResourceHolder<ImageMgr> sm = gamedata->GetResourceHolder<ImageMgr>(TmpResRef);

		if (sm) {
			// night small map is *optional*!
			// keep the existing map if this one is null
			map->SmallMap = sm->GetSprite2D();
		}

		//the map state was altered, no need to hold this off for any later
		map->DayNight = day_or_night;

		tm->UpdateDoors();

		map->SetTileMapProps(std::move(props));
	} catch (const std::exception& e) {
		Log(ERROR, "AREImporter", "{}", e);
		return false;
	}

	// update the tiles and tilecount (eg. door0304 in Edwin's Docks (ar0300) entrance
	for (const auto& door : tm->GetDoors()) {
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
	ieDword bit, otherbit, maskOff = 0, maskOn = 0;
	for (int i = 0; i < 6; i++) {
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

	Ambient* ambi = new Ambient();
	ambi->flags = IE_AMBI_ENABLED | IE_AMBI_LOOPING | IE_AMBI_MAIN | IE_AMBI_NOSAVE;
	ambi->gain = static_cast<ieWord>(mainAmbients.AmbientVol);
	// sounds and name
	ambi->sounds.emplace_back(mainAmbient);
	ambi->name = mainAmbient;
	ambi->appearance = (1 << 25) - 1; // default to all 24 bits enabled, one per hour
	ambi->radius = 50; // REFERENCE_DISTANCE
	return ambi;
}

void AREImporter::GetSongs(DataStream* str, Map* map, std::vector<Ambient*>& ambients) const
{
	// 5 is the number of song indices
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

	// check for existence of main ambients (bg1)
	constexpr int dayBits = ((1 << 18) - 1) ^ ((1 << 6) - 1); // day: bits 6-18 per DLTCEP
	Ambient* ambi = SetupMainAmbients(dayAmbients);
	if (!ambi) return;

	// schedule for day/night
	// if the two ambients are the same, just add one, so there's no restart
	if (dayAmbients.Ambient2 != nightAmbients.Ambient2) {
		ambi->appearance = dayBits;
		ambients.push_back(ambi);
		// night
		ambi = SetupMainAmbients(nightAmbients);
		if (ambi) {
			ambi->appearance ^= dayBits; // night: bits 0-5 + 19-23, [dusk till dawn]
		}
	}
	// bgt ar7300 has a night ambient only in the first slot
	if (ambi) {
		ambients.push_back(ambi);
	}
}

void AREImporter::GetRestHeader(DataStream* str, Map* map) const
{
	for (auto& ref : map->RestHeader.Strref) {
		str->ReadStrRef(ref);
	}
	for (auto& ref : map->RestHeader.CreResRef) {
		str->ReadResRef(ref);
	}
	str->ReadWord(map->RestHeader.CreatureNum);
	if (map->RestHeader.CreatureNum > MAX_RESCOUNT) {
		map->RestHeader.CreatureNum = MAX_RESCOUNT;
	}
	str->ReadWord(map->RestHeader.Difficulty); // difficulty?
	str->ReadDword(map->RestHeader.Duration);
	str->ReadWord(map->RestHeader.RandomWalkDistance);
	str->ReadWord(map->RestHeader.FollowDistance);
	str->ReadWord(map->RestHeader.Maximum); // maximum number of creatures
	str->ReadWord(map->RestHeader.Enabled);
	str->ReadWord(map->RestHeader.DayChance);
	str->ReadWord(map->RestHeader.NightChance);
	// 14 reserved dwords
}

void AREImporter::GetInfoPoint(DataStream* str, int idx, Map* map) const
{
	str->Seek(InfoPointsOffset + idx * 0xC4, GEM_STREAM_START);

	ieWord ipType;
	ieWord vertexCount;
	ieDword firstVertex;
	ieDword cursor;
	ieDword ipFlags;
	ieWord trapDetDiff;
	ieWord trapRemDiff;
	ieWord trapped;
	ieWord trapDetected;
	Point launchP;
	Point pos;
	Point talkPos;
	Region bbox;
	ieVariable ipName;
	ieVariable entrance;
	ResRef script0;
	ResRef keyResRef;
	ResRef destination;
	// two adopted pst specific fields
	ResRef dialogResRef;
	ResRef wavResRef;
	ieStrRef dialogName;

	str->ReadVariable(ipName);
	str->ReadWord(ipType);
	str->ReadRegion(bbox, true);
	str->ReadWord(vertexCount);
	str->ReadDword(firstVertex);
	ieDword triggerValue;
	str->ReadDword(triggerValue); // named triggerValue in the IE source
	str->ReadDword(cursor);
	str->ReadResRef(destination);
	str->ReadVariable(entrance);
	str->ReadDword(ipFlags);
	ieStrRef overheadRef;
	str->ReadStrRef(overheadRef);
	str->ReadWord(trapDetDiff);
	str->ReadWord(trapRemDiff);
	str->ReadWord(trapped);
	str->ReadWord(trapDetected);
	str->ReadPoint(launchP);
	str->ReadResRef(keyResRef);
	str->ReadResRef(script0);
	// ARE 9.1: 4B per position after that.
	if (16 == map->version) {
		str->ReadPoint(pos); // OverridePoint in NI
		if (pos.IsZero()) {
			str->ReadScalar(pos.x); // AlternatePoint in NI
			str->ReadScalar(pos.y);
		} else {
			str->Seek(8, GEM_CURRENT_POS);
		}
		str->Seek(26, GEM_CURRENT_POS);
	} else {
		str->ReadPoint(pos); // TransitionWalkToX, TransitionWalkToY
		// maybe we have to store this
		// bg2: 15 reserved dwords, the above point is actually in dwords (+1),
		// but since it's the last thing the underseek doesn't matter
		str->Seek(36, GEM_CURRENT_POS);
	}

	if (core->HasFeature(GFFlags::INFOPOINT_DIALOGS)) {
		str->ReadResRef(wavResRef);
		str->ReadPoint(talkPos);
		str->ReadStrRef(dialogName);
		str->ReadResRef(dialogResRef);
	} else {
		wavResRef.Reset();
		dialogName = ieStrRef::INVALID;
		dialogResRef.Reset();
	}

	InfoPoint* ip = nullptr;
	str->Seek(VerticesOffset + firstVertex * 4, GEM_STREAM_START);
	if (vertexCount <= 1) {
		// this is exactly the same as bbox.origin
		if (vertexCount == 1) {
			Point pos2;
			str->ReadPoint(pos2);
			assert(pos2 == bbox.origin);
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

		ip = map->TMap->AddInfoPoint(ipName, ipType, nullptr);
		ip->BBox = bbox;
	} else if (vertexCount == 2) {
#define MSG "Encountered a bogus polygon with 2 vertices!"
#if NDEBUG
		Log(ERROR, "AREImporter", MSG);
		return;
#else // make this fatal on debug builds
		error("AREImporter", MSG);
#endif
#undef MSG
	} else {
		std::vector<Point> points(vertexCount);
		for (int x = 0; x < vertexCount; x++) {
			str->ReadPoint(points[x]);
		}
		// recalculate the bbox if it was not provided
		auto poly = std::make_shared<Gem_Polygon>(std::move(points), bbox.size.IsInvalid() ? nullptr : &bbox);
		bbox = poly->BBox;
		ip = map->TMap->AddInfoPoint(ipName, ipType, poly);
	}

	ip->TrapDetectionDiff = trapDetDiff;
	ip->TrapRemovalDiff = trapRemDiff;
	ip->Trapped = trapped;
	ip->TrapDetected = trapDetected;
	ip->TrapLaunch = launchP;
	// translate door cursor on infopoint to correct cursor
	if (cursor == IE_CURSOR_DOOR) cursor = IE_CURSOR_PASS;
	ip->Cursor = cursor;
	ip->overHead.SetText(core->GetString(overheadRef), false);
	ip->StrRef = overheadRef; //we need this when saving area
	ip->SetMap(map);
	ip->Flags = ipFlags;
	ip->UsePoint = pos;
	// FIXME: PST doesn't use this field
	if (ip->GetUsePoint()) {
		ip->Pos = ip->UsePoint;
	} else {
		ip->Pos = bbox.Center();
	}
	ip->Destination = destination;
	ip->EntranceName = entrance;
	ip->KeyResRef = keyResRef;

	// these appear only in PST, but we could support them everywhere
	// HOWEVER they did not use them as witnessed in ar0101 (0101prt1 and 0101prt2) :(
	if (core->HasFeature(GFFlags::PST_STATE_FLAGS)) {
		talkPos = ip->Pos;
	}
	ip->TalkPos = talkPos;
	ip->DialogName = dialogName;

	// PST has garbage here and there
	if (dialogResRef.IsASCII()) {
		ip->SetDialog(dialogResRef);
	}
	if (wavResRef.IsASCII()) {
		ip->SetEnter(wavResRef);
	}

	if (script0.IsEmpty()) {
		ip->Scripts[0] = nullptr;
	} else {
		ip->Scripts[0] = new GameScript(script0, ip);
	}

	if (ip->Type != ST_TRAVEL || !ip->outline) return;
	// mark the searchmap under travel regions as passable (not travel)
	for (int i = 0; i < ip->BBox.w; i += 8) {
		for (int j = 0; j < ip->BBox.h; j += 6) {
			NavmapPoint sample(ip->BBox.x + i, ip->BBox.y + j);
			if (!ip->outline->PointIn(sample)) continue;
			SearchmapPoint below = Map::ConvertCoordToTile(sample);
			PathMapFlags tmp = map->tileProps.QuerySearchMap(below);
			map->tileProps.PaintSearchMap(below, tmp | PathMapFlags::PASSABLE);
		}
	}
}

void AREImporter::GetContainer(DataStream* str, int idx, Map* map)
{
	str->Seek(ContainersOffset + idx * 0xC0, GEM_STREAM_START);

	ieVariable containerName;
	ieWord containerType;
	ieWord lockDiff;
	ieWord trapDetDiff;
	ieWord trapRemDiff;
	ieWord trapped;
	ieWord trapDetected;
	ieWord vertCount;
	ieWord unknown;
	Point pos;
	Point launchPos;
	Region bbox;
	ieDword containerFlags;
	ieDword itemIndex;
	ieDword itemCount;
	ieDword firstIndex;
	ResRef keyResRef;
	ieStrRef openFail;

	str->ReadVariable(containerName);
	str->ReadPoint(pos);
	str->ReadWord(containerType);
	str->ReadWord(lockDiff);
	str->ReadDword(containerFlags);
	str->ReadWord(trapDetDiff);
	str->ReadWord(trapRemDiff);
	str->ReadWord(trapped);
	str->ReadWord(trapDetected);
	str->ReadPoint(launchPos);
	str->ReadRegion(bbox, true);
	str->ReadDword(itemIndex);
	str->ReadDword(itemCount);
	str->ReadResRef(Script);

	str->ReadDword(firstIndex);
	// the vertex count is only 16 bits, there is a weird flag
	// after it, which is usually 0, but sometimes set to 1
	str->ReadWord(vertCount);
	str->ReadWord(unknown); // trigger range
	//str->Read(Name, 32); // owner's scriptname
	str->Seek(32, GEM_CURRENT_POS);
	str->ReadResRef(keyResRef);
	str->Seek(4, GEM_CURRENT_POS); // break difficulty
	str->ReadStrRef(openFail);
	// 14 reserved dwords

	str->Seek(VerticesOffset + firstIndex * 4, GEM_STREAM_START);

	Container* c = nullptr;
	if (vertCount == 0) {
		// piles have no polygons and no bounding box in some areas,
		// but bg2 gives them this bounding box at first load,
		// should we specifically check for Type == IE_CONTAINER_PILE?
		if (bbox.size.IsInvalid()) {
			bbox.x = pos.x - 7;
			bbox.y = pos.y - 5;
			bbox.w = 16;
			bbox.h = 12;
		}
		c = map->AddContainer(containerName, containerType, nullptr);
		c->BBox = bbox;
	} else {
		std::vector<Point> points(vertCount);
		for (int x = 0; x < vertCount; x++) {
			str->ReadPoint(points[x]);
		}
		auto poly = std::make_shared<Gem_Polygon>(std::move(points), &bbox);
		c = map->AddContainer(containerName, containerType, poly);
	}

	c->Pos = pos;
	c->LockDifficulty = lockDiff;
	c->Flags = containerFlags;
	c->TrapDetectionDiff = trapDetDiff;
	c->TrapRemovalDiff = trapRemDiff;
	c->Trapped = trapped;
	c->TrapDetected = trapDetected;
	c->TrapLaunch = launchPos;
	// reading items into a container
	str->Seek(ItemsOffset + itemIndex * 0x14, GEM_STREAM_START);
	while (itemCount--) {
		// cannot add directly to inventory (ground piles)
		c->AddItem(core->ReadItem(str));
	}

	if (containerType == IE_CONTAINER_PILE) Script.Reset();

	if (Script.IsEmpty()) {
		c->Scripts[0] = nullptr;
	} else {
		c->Scripts[0] = new GameScript(Script, c);
	}
	c->KeyResRef = keyResRef;
	if (!openFail) openFail = ieStrRef(-1); // rewrite 0 to -1
	c->OpenFail = openFail;
}

void AREImporter::GetDoor(DataStream* str, int idx, Map* map, PluginHolder<TileMapMgr> tmm) const
{
	str->Seek(DoorsOffset + idx * 0xC8, GEM_STREAM_START);

	ieDword doorFlags;
	ieDword openFirstVertex;
	ieDword closedFirstVertex;
	ieDword openFirstImpeded;
	ieDword closedFirstImpeded;
	ieDword cursor;
	ieDword discoveryDiff;
	ieDword lockRemoval;
	ieVariable longName;
	ieVariable linkedInfo;
	ResRef shortName;
	ResRef openResRef;
	ResRef closeResRef;
	ResRef keyResRef;
	ResRef script0;
	ResRef dialog;
	ieWord openVerticesCount;
	ieWord closedVerticesCount;
	ieWord openImpededCount;
	ieWord closedImpededCount;
	ieWord trapDetect;
	ieWord trapRemoval;
	ieWord trapped;
	ieWord trapDetected;
	ieWord hp;
	ieWord ac;
	Point launchP;
	Point toOpen[2];
	Region closedBBox;
	Region openedBBox;
	ieStrRef openStrRef;
	ieStrRef nameStrRef;

	str->ReadVariable(longName);
	str->ReadResRef(shortName);
	str->ReadDword(doorFlags);
	if (map->version == 16) {
		doorFlags = FixIWD2DoorFlags(doorFlags, false);
	}
	if (AreaType & AT_OUTDOOR) doorFlags |= DOOR_TRANSPARENT; // actually true only for fog-of-war, excluding other actors

	str->ReadDword(openFirstVertex);
	str->ReadWord(openVerticesCount);
	str->ReadWord(closedVerticesCount);
	str->ReadDword(closedFirstVertex);
	str->ReadRegion(openedBBox, true);
	str->ReadRegion(closedBBox, true);
	str->ReadDword(openFirstImpeded);
	str->ReadWord(openImpededCount);
	str->ReadWord(closedImpededCount);
	str->ReadDword(closedFirstImpeded);
	str->ReadWord(hp); // hitpoints
	str->ReadWord(ac); // AND armorclass, according to IE dev info
	str->ReadResRef(openResRef);
	str->ReadResRef(closeResRef);
	str->ReadDword(cursor);
	str->ReadWord(trapDetect);
	str->ReadWord(trapRemoval);
	str->ReadWord(trapped);
	str->ReadWord(trapDetected);
	str->ReadPoint(launchP);
	str->ReadResRef(keyResRef);
	str->ReadResRef(script0);
	str->ReadDword(discoveryDiff);
	str->ReadDword(lockRemoval);
	str->ReadPoint(toOpen[0]);
	str->ReadPoint(toOpen[1]);
	str->ReadStrRef(openStrRef);
	if (core->HasFeature(GFFlags::AUTOMAP_INI) || map->version == 16) { // true in all games? IESDP has 24 bits for v1 too
		char tmp[25];
		str->Read(tmp, 24);
		tmp[24] = 0;
		linkedInfo = tmp; // linkedInfo unused in pst anyway?
	} else {
		str->ReadVariable(linkedInfo);
	}
	str->ReadStrRef(nameStrRef); // trigger name
	str->ReadResRef(dialog);
	if (core->HasFeature(GFFlags::AUTOMAP_INI)) {
		// maybe this is important? but seems not
		str->Seek(8, GEM_CURRENT_POS);
	}

	// Reading Open Polygon
	std::shared_ptr<Gem_Polygon> open = nullptr;
	str->Seek(VerticesOffset + openFirstVertex * 4, GEM_STREAM_START);
	if (openVerticesCount) {
		std::vector<Point> points(openVerticesCount);
		for (int x = 0; x < openVerticesCount; x++) {
			str->ReadPoint(points[x]);
		}
		open = std::make_shared<Gem_Polygon>(std::move(points), &openedBBox);
	}

	// Reading Closed Polygon
	std::shared_ptr<Gem_Polygon> closed = nullptr;
	str->Seek(VerticesOffset + closedFirstVertex * 4, GEM_STREAM_START);
	if (closedVerticesCount) {
		std::vector<Point> points(closedVerticesCount);
		for (int x = 0; x < closedVerticesCount; x++) {
			str->ReadPoint(points[x]);
		}
		closed = std::make_shared<Gem_Polygon>(std::move(points), &closedBBox);
	}

	// Getting Door Information from the WED File
	bool baseClosed;
	auto indices = tmm->GetDoorIndices(shortName, baseClosed);
	if (core->HasFeature(GFFlags::REVERSE_DOOR)) {
		baseClosed = !baseClosed;
	}

	// iwd2 workaround: two ar6051 doors, acting as switches have detectable traps in the original, yet are missing the bit
	// looking at the original, it checks it, no script sets it on them, it's just bonkers
	// they are marked as detected already in the data, but don't appear as such
	if (longName == "AR6051_Lava_Switch" || longName == "AR6051_Acid_Switch") {
		doorFlags |= DOOR_DETECTABLE;
		trapDetect = 30;
		trapDetected = 0;
	}

	auto closedPolys = tmm->ClosedDoorPolygons();
	auto openPolys = tmm->OpenDoorPolygons();

	DoorTrigger dt(std::move(open), std::move(openPolys), std::move(closed), std::move(closedPolys));
	Door* door = map->TMap->AddDoor(shortName, longName, doorFlags, baseClosed, std::move(indices), std::move(dt));
	door->OpenBBox = openedBBox;
	door->ClosedBBox = closedBBox;

	// Reading Open Impeded blocks
	str->Seek(VerticesOffset + openFirstImpeded * 4, GEM_STREAM_START);
	door->open_ib.resize(openImpededCount);
	for (Point& point : door->open_ib) {
		str->ReadPoint(point);
	}

	// Reading Closed Impeded blocks
	str->Seek(VerticesOffset + closedFirstImpeded * 4, GEM_STREAM_START);

	door->closed_ib.resize(closedImpededCount);
	for (Point& point : door->closed_ib) {
		str->ReadPoint(point);
	}
	door->SetMap(map);

	door->hp = hp;
	door->ac = ac;
	door->TrapDetectionDiff = trapDetect;
	door->TrapRemovalDiff = trapRemoval;
	door->Trapped = trapped;
	door->TrapDetected = trapDetected;
	door->TrapLaunch = launchP;

	door->Cursor = cursor;
	door->KeyResRef = keyResRef;
	if (script0.IsEmpty()) {
		door->Scripts[0] = nullptr;
	} else {
		door->Scripts[0] = new GameScript(script0, door);
	}

	door->toOpen[0] = toOpen[0];
	door->toOpen[1] = toOpen[1];
	// Leave the default sound untouched
	if (!openResRef.IsEmpty()) {
		door->OpenSound = openResRef;
	} else if (doorFlags & DOOR_SECRET) {
		door->OpenSound = gamedata->defaultSounds[DEF_HOPEN];
	} else {
		door->OpenSound = gamedata->defaultSounds[DEF_OPEN];
	}
	if (!closeResRef.IsEmpty()) {
		door->CloseSound = closeResRef;
	} else if (doorFlags & DOOR_SECRET) {
		door->CloseSound = gamedata->defaultSounds[DEF_HCLOSE];
	} else {
		door->CloseSound = gamedata->defaultSounds[DEF_CLOSE];
	}
	if (!openStrRef) openStrRef = ieStrRef(-1); // rewrite 0 to -1
	door->LockedStrRef = openStrRef;

	door->DiscoveryDiff = discoveryDiff;
	door->LockDifficulty = lockRemoval;
	door->LinkedInfo = MakeVariable(linkedInfo);
	// these 2 fields are not sure
	door->NameStrRef = nameStrRef;
	door->SetDialog(dialog);
}

void AREImporter::GetSpawnPoint(DataStream* str, int idx, Map* map) const
{
	str->Seek(SpawnOffset + idx * 0xC8, GEM_STREAM_START);

	ieVariable spName;
	Point pos;
	ieWord spawningFrequency;
	ieWord creatureCount;
	std::vector<ResRef> creatures(MAX_RESCOUNT);

	str->ReadVariable(spName);
	str->ReadPoint(pos);
	for (auto& creature : creatures) {
		str->ReadResRef(creature);
	}
	str->ReadWord(creatureCount);
	assert(creatureCount <= MAX_RESCOUNT);
	creatures.resize(creatureCount);
	Spawn* sp = map->AddSpawn(spName, pos, std::move(creatures));

	str->ReadWord(sp->Difficulty);
	str->ReadWord(spawningFrequency);
	// this value is used in a division, better make it nonzero now
	// this will fix any old gemrb saves vs. the original engine
	if (!spawningFrequency) {
		spawningFrequency = 1;
	}
	sp->Frequency = spawningFrequency;
	str->ReadWord(sp->Method);
	if (sp->Method & SPF_BGT) {
		sp->Difficulty /= 100;
	}

	str->ReadDword(sp->sduration); // time to live for spawns
	str->ReadWord(sp->rwdist); // random walk distance (0 is unlimited), hunting range
	str->ReadWord(sp->owdist); // other walk distance (inactive in all engines?), follow range
	str->ReadWord(sp->Maximum);
	str->ReadWord(sp->Enabled);
	str->ReadDword(sp->appearance);
	str->ReadWord(sp->DayChance);
	str->ReadWord(sp->NightChance);
	// 14 reserved dwords
	// TODO: ee added several more fields; check if they're actually used first
}

bool AREImporter::GetActor(DataStream* str, PluginHolder<ActorMgr> actorMgr, Map* map) const
{
	static int pst = core->HasFeature(GFFlags::AUTOMAP_INI);

	ieVariable defaultName;
	ResRef creResRef;
	ResRef dialog;
	ResRef scripts[8]; // the original order is shown in scrlev.ids
	ieDword talkCount;
	ieDword orientation;
	ieDword schedule;
	ieDword removalTime;
	ieDword flags;
	ieDword creOffset;
	ieDword creSize;
	Point pos;
	Point destination;
	ieWord maxDistance;
	ieWord spawned;
	ieByte difficultyMargin;
	DataStream* creFile;

	str->ReadVariable(defaultName);
	str->ReadPoint(pos);
	str->ReadPoint(destination);
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
	str->ReadDword(creOffset);
	str->ReadDword(creSize);
	// another iwd2 script slot
	str->ReadResRef(scripts[SCR_AREA]);
	str->Seek(120, GEM_CURRENT_POS);
	// not iwd2, this field is garbage
	if (!core->HasFeature(GFFlags::IWD2_SCRIPTNAME)) {
		scripts[SCR_AREA].Reset();
	}

	// actually, Flags&1 signs that the creature
	// is not loaded yet, so !(Flags&1) means it is embedded
	if (creOffset != 0 && !(flags & AF_CRE_NOT_LOADED)) {
		creFile = SliceStream(str, creOffset, creSize, true);
	} else {
		creFile = gamedata->GetResourceStream(creResRef, IE_CRE_CLASS_ID);
	}
	if (!actorMgr->Open(creFile)) {
		Log(ERROR, "AREImporter", "Couldn't read actor: {}!", creResRef);
		return false;
	}
	Actor* act = actorMgr->GetActor(0);
	if (!act) return false;

	// PST generally doesn't appear to use the starting MC_ bits, but for some reason
	// there's a coaxmetal copy in the mortuary with both KEEP and REMOVE corpse
	// set that should never be seen. The actor is also already dead, so we don't end
	// up doing any of the regular cleanup on it (it's mrtghost.cre). Banish it instead.
	if (pst && act->GetBase(IE_STATE_ID) & STATE_DEAD && act->GetBase(IE_MC_FLAGS) & MC_REMOVE_CORPSE) {
		return false;
	}

	map->AddActor(act, false);
	act->Pos = pos;
	act->Destination = destination;
	act->HomeLocation = destination;
	act->maxWalkDistance = maxDistance;
	act->Spawned = spawned;
	act->appearance = schedule;
	// copying the scripting name into the actor
	// if the CreatureAreaFlag was set to 8
	// AF_NAME_OVERRIDE == AF_ENABLED, used for something else in IWD2
	if ((flags & AF_NAME_OVERRIDE) || core->HasFeature(GFFlags::IWD2_SCRIPTNAME)) {
		act->SetScriptName(defaultName);
	}
	// IWD2 specific hacks
	if (core->HasFeature(GFFlags::RULES_3ED)) {
		if (flags & AF_SEEN_PARTY) {
			act->SetMCFlag(MC_SEENPARTY, BitOp::OR);
		}
		if (flags & AF_INVULNERABLE) {
			act->SetMCFlag(MC_INVULNERABLE, BitOp::OR);
		}
		if (flags & AF_ENABLED) {
			act->BaseStats[IE_EA] = EA_EVILCUTOFF;
			act->SetMCFlag(MC_ENABLED, BitOp::OR);
		} else {
			// DifficultyMargin - only enable actors that are difficult enough vs the area difficulty
			// 1 - area difficulty 1
			// 2 - area difficulty 2
			// 4 - area difficulty 3
			if (difficultyMargin && !(difficultyMargin & map->AreaDifficulty)) {
				act->DestroySelf();
			}
		}
		// temporary hack while ar6104 causes pathfinding problems
		if (act->GetScriptName() == "Troll_11" && map->GetScriptName() == "ar6104") {
			act->DestroySelf();
		}
	}
	act->ignoredFields.difficultyMargin = difficultyMargin;

	act->SetDialog(dialog);

	for (int j = 0; j < 8; j++) {
		if (!scripts[j].IsEmpty()) {
			act->SetScript(scripts[j], j);
		}
	}
	act->SetOrientation(ClampToOrientation(orientation), false);
	act->TalkCount = talkCount;
	act->Timers.removalTime = removalTime;
	act->RefreshEffects();
	return true;
}

void AREImporter::GetAreaAnimation(DataStream* str, Map* map) const
{
	AreaAnimation anim = AreaAnimation();

	str->ReadVariable(anim.Name);
	str->ReadPoint(anim.Pos);
	str->ReadDword(anim.appearance);
	str->ReadResRef(anim.BAM);
	str->ReadWord(anim.sequence);
	str->ReadWord(anim.frame);
	str->ReadEnum(anim.flags);
	anim.originalFlags = anim.flags;
	str->ReadScalar(anim.height);
	if (core->HasFeature(GFFlags::IMPLICIT_AREAANIM_BACKGROUND)) {
		anim.height = ANI_PRI_BACKGROUND;
		anim.flags |= AreaAnimation::Flags::NoWall;
	}
	str->ReadWord(anim.transparency);
	ieWord startFrameRange;
	str->ReadWord(startFrameRange);
	str->Read(&anim.startchance, 1);
	if (anim.startchance <= 0) {
		anim.startchance = 100; // percentage of starting a cycle
	}
	if (startFrameRange && bool(anim.flags & AreaAnimation::Flags::RandStart)) {
		anim.frame = RAND<AreaAnimation::index_t>(0, startFrameRange - 1);
	}
	anim.startFrameRange = 0; // this will never get resaved (iirc)
	str->Read(&anim.skipcycle, 1); // how many cycles are skipped (100% skippage), "period" in bg2
	str->ReadResRef(anim.PaletteRef);
	// TODO: EE: word with anim width for PVRZ/WBM resources (if flag bits are set, see A_ANI_ defines)
	// 0x4a holds the height
	str->ReadDword(anim.unknown48);

	static int pst = core->HasFeature(GFFlags::AUTOMAP_INI);
	if (pst) {
		AdjustPSTFlags(anim);
	}

	// set up the animation, it cannot be done here
	// because a StaticSequence action can change it later
	map->AddAnimation(std::move(anim));
}

void AREImporter::GetAmbient(DataStream* str, std::vector<Ambient*>& ambients) const
{
	ResRef sounds[MAX_RESCOUNT];
	ieWord soundCount;
	ieDword interval;
	Ambient* ambient = new Ambient();

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
	str->ReadWord(soundCount);
	str->Seek(2, GEM_CURRENT_POS); // alignment padding
	str->ReadDword(interval);
	ambient->interval = interval * 1000;
	str->ReadDword(interval);
	ambient->intervalVariance = interval * 1000;
	// schedule bits
	str->ReadDword(ambient->appearance);
	str->ReadDword(ambient->flags);
	str->Seek(64, GEM_CURRENT_POS);
	// this is a physical limit
	if (soundCount > MAX_RESCOUNT) {
		soundCount = MAX_RESCOUNT;
	}
	for (int j = 0; j < soundCount; j++) {
		ambient->sounds.emplace_back(sounds[j]);
	}
	ambients.push_back(ambient);
}

void AREImporter::GetAutomapNotes(DataStream* str, Map* map) const
{
	static int pst = core->HasFeature(GFFlags::AUTOMAP_INI);
	Point point;

	if (!pst) {
		for (ieDword i = 0; i < NoteCount; i++) {
			str->ReadPoint(point);
			ieStrRef strref = ieStrRef::INVALID;
			str->ReadStrRef(strref);
			ieWord location; // (0=External (TOH/TOT), 1=Internal (TLK)
			str->ReadWord(location);
			ieWord color;
			str->ReadWord(color);
			// dword: ID in bg2
			str->Seek(40, GEM_CURRENT_POS);
			// BG2 allows editing the builtin notes, PST does not, iwd1 and bg1 have no notes and iwd2 only user notes
			map->AddMapNote(point, color, strref, false);
		}
		return;
	}

	// Don't bother with autonote.ini if the area has autonotes (ie. it is a saved area)
	auto flag = gamedata->GetFactoryResourceAs<AnimationFactory>("FLAG1", IE_BAM_CLASS_ID);
	if (flag == nullptr) {
		ResourceHolder<ImageMgr> roImg = gamedata->GetResourceHolder<ImageMgr>("RONOTE");
		ResourceHolder<ImageMgr> userImg = gamedata->GetResourceHolder<ImageMgr>("USERNOTE");

		AnimationFactory af("FLAG1", { roImg->GetSprite2D(), userImg->GetSprite2D() }, { { 1, 0 }, { 1, 1 } }, { 0, 1 });
		gamedata->AddFactoryResource<AnimationFactory>(std::move(af));
	}

	if (NoteCount) {
		for (ieDword i = 0; i < NoteCount; i++) {
			ieDword px;
			ieDword py;
			str->ReadDword(px);
			str->ReadDword(py);

			// in PST the coordinates are stored in small map space
			// our MapControl wants them in large map space so we must convert
			// its what other games use and its what our custom map note code uses
			const Size mapsize = map->GetSize();
			point.x = static_cast<int>(px * double(mapsize.w) / map->SmallMap->Frame.w);
			point.y = static_cast<int>(py * double(mapsize.h) / map->SmallMap->Frame.h);

			char bytes[501]; // 500 + null
			str->Read(bytes, 500);
			bytes[500] = '\0';
			ieDword readonly;
			str->ReadDword(readonly); // readonly == 1
			map->AddMapNote(point, 0, StringFromTLK(StringView(bytes)), readonly);
			str->Seek(20, GEM_CURRENT_POS);
		}
	} else {
		if (!INInote) {
			ReadAutonoteINI();
		}
		if (!INInote) return;

		// add autonote.ini entries
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
}

bool AREImporter::GetTrap(DataStream* str, int idx, Map* map) const
{
	str->Seek(TrapOffset + idx * 0x1C, GEM_STREAM_START);

	ResRef trapResRef;
	ieDword trapEffOffset;
	ieWord trapSize;
	ieWord proID;
	ieByte owner;
	Point pos;
	// currently unused:
	Point point;
	ieDword ticks;
	ieByte targetType;

	str->ReadResRef(trapResRef);
	str->ReadDword(trapEffOffset);
	str->ReadWord(trapSize);
	int trapEffectCount = trapSize / 0x108;
	if (trapEffectCount * 0x108 != trapSize) {
		Log(ERROR, "AREImporter", "TrapEffectSize in game: {} != {}. Clearing it", trapSize, trapEffectCount * 0x108);
		return false;
	}
	str->ReadWord(proID);
	str->ReadDword(ticks); // actually, delaycount/repetitioncount
	str->ReadPoint(point);
	str->Seek(2, GEM_CURRENT_POS); // unknown/unused 'Z'
	str->Read(&targetType, 1); // according to dev info, this is 'targettype'; "Enemy-ally targeting" on IESDP
	str->Read(&owner, 1); // party member index that created this projectile (0-5)
	// The projectile is always created, the worst that can happen
	// is a dummy projectile
	// The projectile ID is 214 for TRAPSNAR
	// It is off by one compared to projectl.ids, but the same as missile.ids
	Projectile* pro = core->GetProjectileServer()->GetProjectileByIndex(proID - 1);

	EffectQueue fxqueue = EffectQueue();
	DataStream* fs = new SlicedStream(str, trapEffOffset, trapSize);
	ReadEffects(fs, &fxqueue, trapEffectCount);
	const Actor* caster = core->GetGame()->FindPC(owner + 1);
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
	return true;
}

void AREImporter::GetTile(DataStream* str, Map* map) const
{
	ieVariable tileName;
	ResRef tileID;
	ieDword tileFlags;
	// these fields could be different size: ieDword ClosedCount, OpenCount;
	ieWord closedCount;
	ieWord openCount;
	ieDword closedIndex;
	ieDword openIndex;
	str->ReadVariable(tileName);
	str->ReadResRef(tileID);
	str->ReadDword(tileFlags);
	// IE dev info says this:
	str->ReadDword(openIndex); // PrimarySearchSquareStart in bg2
	str->ReadWord(openCount); // PrimarySearchSquareCount
	str->ReadWord(closedCount); // SecondarySearchSquareCount
	str->ReadDword(closedIndex); // SecondarySearcHSquareStart
	// end of disputed section

	str->Seek(48, GEM_CURRENT_POS); // 12 reserved dwords
	// absolutely no idea where these 'tile indices' are stored
	// are they tileset tiles or impeded block tiles
	map->TMap->AddTile(tileID, tileName, tileFlags, nullptr, 0, nullptr, 0);
}

Map* AREImporter::GetMap(const ResRef& resRef, bool day_or_night)
{
	// if this area does not have extended night, force it to day mode
	if (!(AreaFlags & AT_EXTENDED_NIGHT))
		day_or_night = true;

	PluginHolder<TileMapMgr> tmm = MakePluginHolder<TileMapMgr>(IE_WED_CLASS_ID);
	DataStream* wedfile = gamedata->GetResourceStream(WEDResRef, IE_WED_CLASS_ID);
	tmm->Open(wedfile);

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
	ResourceHolder<ImageMgr> sm = gamedata->GetResourceHolder<ImageMgr>(TmpResRef);
	if (!sm) {
		//fall back to day minimap
		sm = gamedata->GetResourceHolder<ImageMgr>(WEDResRef);
	}

	Map* map = nullptr;
	try {
		map = new Map(tm, MakeTileProps(tm, WEDResRef, day_or_night), sm ? sm->GetSprite2D() : nullptr);
	} catch (const std::exception& e) {
		Log(ERROR, "AREImporter", "{}", e);
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
	if (idx >= 0) {
		map->SetTrackString(tracks[idx].text, tracks[idx].enabled, tracks[idx].difficulty);
	} else {
		map->SetTrackString(ieStrRef(-1), false, 0);
	}

	//if the Script field is empty, the area name will be copied into it on first load
	//this works only in the iwd branch of the games
	if (Script.IsEmpty() && core->HasFeature(GFFlags::FORCE_AREA_SCRIPT)) {
		Script = resRef;
	}

	if (!Script.IsEmpty()) {
		//for some reason the area's script is run from the last slot
		//at least one area script depends on this, if you need something
		//more customisable, add a game flag
		map->Scripts[MAX_SCRIPTS - 1] = new GameScript(Script, map);
	}

	Log(DEBUG, "AREImporter", "Loading songs");
	std::vector<Ambient*> ambients;
	str->Seek(SongHeader, GEM_STREAM_START);
	GetSongs(str, map, ambients);
	// reverb to match against reverb.2da or iwd reverb.ids
	// (if the 2da doesn't exist - which we provide for all; they use the same values)
	ieDword reverbID; // set in PST, IWD1, 0 (NO_REVERB) elsewhere
	str->ReadDword(reverbID);
	// ignore 0 and use an area-type heuristic instead
	if (reverbID == 0) reverbID = EFX_PROFILE_REVERB_INVALID;

	str->Seek(RestHeader + 32, GEM_STREAM_START); // skip the name
	GetRestHeader(str, map);

	Log(DEBUG, "AREImporter", "Loading regions");
	core->LoadProgress(70);
	//Loading InfoPoints
	for (int i = 0; i < InfoPointsCount; i++) {
		GetInfoPoint(str, i, map);
	}

	Log(DEBUG, "AREImporter", "Loading containers");
	for (int i = 0; i < ContainersCount; i++) {
		GetContainer(str, i, map);
	}

	Log(DEBUG, "AREImporter", "Loading doors");
	for (ieDword i = 0; i < DoorsCount; i++) {
		GetDoor(str, i, map, tmm);
	}

	Log(DEBUG, "AREImporter", "Loading spawnpoints");
	for (ieDword i = 0; i < SpawnCount; i++) {
		GetSpawnPoint(str, i, map);
	}

	core->LoadProgress(75);
	Log(DEBUG, "AREImporter", "Loading actors");
	str->Seek(ActorOffset, GEM_STREAM_START);
	assert(core->IsAvailable(IE_CRE_CLASS_ID));
	auto actmgr = GetImporter<ActorMgr>(IE_CRE_CLASS_ID);
	for (int i = 0; i < ActorCount; i++) {
		if (!GetActor(str, actmgr, map)) continue;
	}

	core->LoadProgress(90);
	Log(DEBUG, "AREImporter", "Loading animations");
	str->Seek(AnimOffset, GEM_STREAM_START);
	for (ieDword i = 0; i < AnimCount; i++) {
		GetAreaAnimation(str, map);
	}

	Log(DEBUG, "AREImporter", "Loading entrances");
	str->Seek(EntrancesOffset, GEM_STREAM_START);
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
	core->LoadInitialValues(resRef, map->locals);
	str->Seek(VariablesOffset, GEM_STREAM_START);
	for (ieDword i = 0; i < VariablesCount; i++) {
		ieVariable Name;
		ieDword Value;
		str->ReadVariable(Name);
		str->Seek(8, GEM_CURRENT_POS); // type + resreftype, part of the partly implemented type system (uint, int, float, str)
		str->ReadDword(Value);
		str->Seek(40, GEM_CURRENT_POS); // values as an int32, float64, string
		map->locals[Name] = Value;
	}

	Log(DEBUG, "AREImporter", "Loading ambients");
	str->Seek(AmbiOffset, GEM_STREAM_START);
	for (int i = 0; i < AmbiCount; ++i) {
		GetAmbient(str, ambients);
	}
	map->SetAmbients(std::move(ambients), reverbID);

	Log(DEBUG, "AREImporter", "Loading automap notes");
	str->Seek(NoteOffset, GEM_STREAM_START);
	GetAutomapNotes(str, map);

	//this is a ToB feature (saves the unexploded projectiles)
	Log(DEBUG, "AREImporter", "Loading traps");
	for (ieDword i = 0; i < TrapCount; i++) {
		if (!GetTrap(str, i, map)) continue;
	}

	Log(DEBUG, "AREImporter", "Loading tiles");
	//Loading Tiled objects (if any)
	str->Seek(TileOffset, GEM_STREAM_START);
	for (ieDword i = 0; i < TileCount; i++) {
		GetTile(str, map);
	}

	Log(DEBUG, "AREImporter", "Loading explored bitmap");
	ieDword mapSize = ieDword(map->ExploredBitmap.Bytes());
	mapSize = std::min(mapSize, ExploredBitmapSize);
	str->Seek(ExploredBitmapOffset, GEM_STREAM_START);
	str->Read(map->ExploredBitmap.begin(), mapSize);

	Log(DEBUG, "AREImporter", "Loading wallgroups");
	map->SetWallGroups(tmm->GetWallGroups());
	// setting up doors - doing it here instead when reading doors, so actors can be bumped if needed
	for (ieDword i = 0; i < DoorsCount; i++) {
		Door* door = tm->GetDoor(i);
		door->SetDoorOpen(door->IsOpen(), false, 0);
	}

	return map;
}

void AREImporter::AdjustPSTFlags(AreaAnimation& areaAnim) const
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

// rename flags for clarity
#define PST_ANI_NO_WALL AreaAnimation::Flags::Once
#define PST_ANI_BLEND   AreaAnimation::Flags::Background

	areaAnim.flags = AreaAnimation::Flags::None; // Clear everything

	// Set default-on flags (currently only A_ANI_SYNC)
	areaAnim.flags |= AreaAnimation::Flags::Sync;

	// Copy still-relevant A_ANI_* flags
	areaAnim.flags |= areaAnim.originalFlags & AreaAnimation::Flags::Active;

	// Map known flags
	if (bool(areaAnim.originalFlags & PST_ANI_BLEND)) {
		areaAnim.flags |= AreaAnimation::Flags::BlendBlack;
	}
	if (bool(areaAnim.originalFlags & PST_ANI_NO_WALL)) {
		areaAnim.flags |= AreaAnimation::Flags::NoWall;
	}
}

void AREImporter::ReadEffects(DataStream* ds, EffectQueue* fxqueue, ieDword EffectsCount) const
{
	PluginHolder<EffectMgr> eM = MakePluginHolder<EffectMgr>(IE_EFF_CLASS_ID);
	eM->Open(ds);

	for (unsigned int i = 0; i < EffectsCount; i++) {
		fxqueue->AddEffect(eM->GetEffectV20());
	}
}

int AREImporter::GetStoredFileSize(Map* map)
{
	int headersize = map->version + 0x11c;
	ActorOffset = headersize;

	//get only saved actors (no familiars or partymembers)
	//summons?
	ActorCount = (ieWord) map->GetActorCount(false);
	headersize += ActorCount * 0x110;

	auto am = GetImporter<ActorMgr>(IE_CRE_CLASS_ID);
	EmbeddedCreOffset = headersize;

	for (unsigned int i = 0; i < ActorCount; i++) {
		headersize += am->GetStoredFileSize(map->GetActor(i, false));
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
	ItemsCount = (ieWord) map->ConsolidateContainers();
	ContainersCount = (ieWord) map->TMap->GetContainerCount();
	headersize += ContainersCount * 0xc0;
	ItemsOffset = headersize;
	headersize += ItemsCount * 0x14;
	DoorsOffset = headersize;

	DoorsCount = (ieDword) map->TMap->GetDoorCount();
	headersize += DoorsCount * 0xc8;
	VerticesOffset = headersize;

	VerticesCount = 0;
	for (unsigned int i = 0; i < InfoPointsCount; i++) {
		const InfoPoint* ip = map->TMap->GetInfoPoint(i);
		if (ip->outline) {
			VerticesCount += ip->outline->Count();
		} else {
			VerticesCount++;
		}
	}
	for (unsigned int i = 0; i < ContainersCount; i++) {
		const Container* c = map->TMap->GetContainer(i);
		if (c->outline)
			VerticesCount += c->outline->Count();
	}
	for (unsigned int i = 0; i < DoorsCount; i++) {
		const Door* d = map->TMap->GetDoor(i);
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

	VariablesCount = (ieDword) map->locals.size();
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
		const Projectile* pro = map->GetNextTrap(piter);
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

	NoteCount = map->GetMapNoteCount();
	headersize += NoteCount * (core->HasFeature(GFFlags::AUTOMAP_INI) ? 0x214 : 0x34);
	SongHeader = headersize;

	headersize += 0x90;
	RestHeader = headersize;

	headersize += 0xe4;
	return headersize;
}

int AREImporter::PutHeader(DataStream* stream, const Map* map) const
{
	ResRef signature = "AREAV1.0";

	if (map->version == 16) {
		signature[5] = '9';
		signature[7] = '1';
	}
	stream->WriteResRef(signature);
	stream->WriteResRef(map->WEDResRef);
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
		stream->Write(tmp, 1);
		tmp[0] = 0;
		if (map->AreaDifficulty == 4) {
			tmp[0] = 1;
		}
		stream->Write(tmp, 1);
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
	const GameScript* s = map->Scripts[MAX_SCRIPTS - 1];
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
	if (core->HasFeature(GFFlags::AUTOMAP_INI)) {
		stream->WriteDword(0xffffffff);
		i = 52;
	}
	stream->WriteDword(NoteOffset);
	stream->WriteDword(NoteCount);
	stream->WriteDword(TrapOffset);
	stream->WriteDword(TrapCount);
	stream->WriteResRef(map->Dream[0]);
	stream->WriteResRef(map->Dream[1]);
	//usually 56 empty bytes (but pst used up 4 elsewhere)
	stream->WriteFilling(i);
	return 0;
}

int AREImporter::PutDoors(DataStream* stream, const Map* map, ieDword& VertIndex) const
{
	for (unsigned int i = 0; i < DoorsCount; i++) {
		Door* d = map->TMap->GetDoor(i);

		stream->WriteVariable(d->GetScriptName());
		stream->WriteResRef(d->ID);
		ieDword flags = d->Flags;
		if (map->version == 16) {
			flags = FixIWD2DoorFlags(d->Flags, true);
		}
		stream->WriteDword(flags);
		stream->WriteDword(VertIndex);
		auto open = d->OpenTriggerArea();
		ieWord tmpWord = static_cast<ieWord>(open ? open->Count() : 0);
		stream->WriteWord(tmpWord);
		VertIndex += tmpWord;
		auto closed = d->ClosedTriggerArea();
		tmpWord = static_cast<ieWord>(closed ? closed->Count() : 0);
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
		stream->WriteResRef(d->OpenSound);
		stream->WriteResRef(d->CloseSound);
		stream->WriteDword(d->Cursor);
		stream->WriteWord(d->TrapDetectionDiff);
		stream->WriteWord(d->TrapRemovalDiff);
		stream->WriteWord(d->Trapped);
		stream->WriteWord(d->TrapDetected);
		stream->WritePoint(d->TrapLaunch);
		stream->WriteResRefLC(d->KeyResRef);
		const GameScript* s = d->Scripts[0];
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
		stream->WriteStrRef(d->LockedStrRef);
		if (core->HasFeature(GFFlags::AUTOMAP_INI)) {
			stream->WriteString(d->LinkedInfo, 24);
		} else {
			stream->WriteVariable(d->LinkedInfo);
		}
		stream->WriteStrRef(d->NameStrRef);
		stream->WriteResRef(d->GetDialog());
		if (core->HasFeature(GFFlags::AUTOMAP_INI)) {
			stream->WriteFilling(8);
		}
	}
	return 0;
}

int AREImporter::PutPoints(DataStream* stream, const std::vector<Point>& points) const
{
	for (const Point& p : points) {
		stream->WritePoint(p);
	}
	return 0;
}

int AREImporter::PutVertices(DataStream* stream, const Map* map) const
{
	//regions
	for (unsigned int i = 0; i < InfoPointsCount; i++) {
		const InfoPoint* ip = map->TMap->GetInfoPoint(i);
		if (ip->outline) {
			PutPoints(stream, ip->outline->vertices);
		} else {
			Point origin = ip->BBox.origin;
			stream->WritePoint(origin);
		}
	}
	//containers
	for (size_t i = 0; i < ContainersCount; i++) {
		const Container* c = map->TMap->GetContainer(i);
		if (c->outline) {
			PutPoints(stream, c->outline->vertices);
		}
	}
	//doors
	for (unsigned int i = 0; i < DoorsCount; i++) {
		const Door* d = map->TMap->GetDoor(i);
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

int AREImporter::PutItems(DataStream* stream, const Map* map) const
{
	for (size_t i = 0; i < ContainersCount; i++) {
		const Container* c = map->TMap->GetContainer(i);

		for (int j = 0; j < c->inventory.GetSlotCount(); j++) {
			const CREItem* ci = c->inventory.GetSlotItem(j);

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

int AREImporter::PutContainers(DataStream* stream, const Map* map, ieDword& VertIndex) const
{
	ieDword ItemIndex = 0;

	for (size_t i = 0; i < ContainersCount; i++) {
		const Container* c = map->TMap->GetContainer(i);

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
		const GameScript* s = c->Scripts[0];
		if (s) {
			stream->WriteResRefLC(s->GetName());
		} else {
			stream->WriteFilling(8);
		}
		//outline polygon index and count
		ieWord tmpWord = static_cast<ieWord>(c->outline ? c->outline->Count() : 0);
		stream->WriteDword(VertIndex);
		stream->WriteWord(tmpWord);
		VertIndex += tmpWord;
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

int AREImporter::PutRegions(DataStream* stream, const Map* map, ieDword& VertIndex) const
{
	for (unsigned int i = 0; i < InfoPointsCount; i++) {
		const InfoPoint* ip = map->TMap->GetInfoPoint(i);

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
		ieWord tmpWord = static_cast<ieWord>(ip->outline ? ip->outline->Count() : 1);
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
		const GameScript* s = ip->Scripts[0];
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
		stream->WriteResRef(ip->EnterWav);
		stream->WritePoint(ip->TalkPos);
		stream->WriteStrRef(ip->DialogName);
		stream->WriteResRef(ip->GetDialog());
	}
	return 0;
}

int AREImporter::PutSpawns(DataStream* stream, const Map* map) const
{
	ieWord tmpWord;

	for (unsigned int i = 0; i < SpawnCount; i++) {
		const Spawn* sp = map->GetSpawn(i);

		stream->WriteVariable(sp->Name);
		tmpWord = (ieWord) sp->Pos.x;
		stream->WriteWord(tmpWord);
		tmpWord = (ieWord) sp->Pos.y;
		stream->WriteWord(tmpWord);
		tmpWord = ieWord(sp->Creatures.size());
		int j;
		for (j = 0; j < tmpWord; j++) {
			stream->WriteResRef(sp->Creatures[j]);
		}
		while (j++ < MAX_RESCOUNT) {
			stream->WriteFilling(8);
		}
		stream->WriteWord(tmpWord);
		stream->WriteWord(sp->Difficulty);
		stream->WriteWord(sp->Frequency);
		stream->WriteWord(sp->Method);
		stream->WriteDword(sp->sduration); //spawn duration
		stream->WriteWord(sp->rwdist); //random walk distance
		stream->WriteWord(sp->owdist); //other walk distance
		stream->WriteWord(sp->Maximum);
		stream->WriteWord(sp->Enabled);
		stream->WriteDword(sp->appearance);
		stream->WriteWord(sp->DayChance);
		stream->WriteWord(sp->NightChance);
		stream->WriteFilling(56); //most likely unused crap
	}
	return 0;
}

void AREImporter::PutScript(DataStream* stream, const Actor* ac, unsigned int index) const
{
	const GameScript* s = ac->Scripts[index];
	if (s) {
		stream->WriteResRefLC(s->GetName());
	} else {
		stream->WriteFilling(8);
	}
}

int AREImporter::PutActors(DataStream* stream, const Map* map) const
{
	ieDword CreatureOffset = EmbeddedCreOffset;

	auto am = GetImporter<ActorMgr>(IE_CRE_CLASS_ID);
	for (unsigned int i = 0; i < ActorCount; i++) {
		const Actor* ac = map->GetActor(i, false);

		stream->WriteVariable(ac->GetScriptName());
		stream->WritePoint(ac->Pos);
		stream->WritePoint(ac->HomeLocation);

		stream->WriteDword(0); //used fields flag always 0 for saved areas
		stream->WriteWord(ac->Spawned);
		stream->WriteFilling(1); // letter
		stream->WriteScalar(ac->ignoredFields.difficultyMargin);
		stream->WriteDword(0); //actor animation, unused
		stream->WriteWord(ac->GetOrientation());
		stream->WriteWord(0); //unknown
		stream->WriteDword(ac->Timers.removalTime);
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
		const Actor* ac = map->GetActor(i, false);

		//reconstructing offsets again
		CreatureOffset += am->GetStoredFileSize(ac);
		am->PutActor(stream, ac);
	}
	assert(stream->GetPos() == CreatureOffset);

	return 0;
}

int AREImporter::PutAnimations(DataStream* stream, const Map* map) const
{
	auto iter = map->GetFirstAnimation();
	while (const AreaAnimation* an = map->GetNextAnimation(iter)) {
		stream->WriteVariable(an->Name);
		stream->WritePoint(an->Pos);
		stream->WriteDword(an->appearance);
		stream->WriteResRef(an->BAM);
		stream->WriteWord(an->sequence);
		stream->WriteWord(an->frame);

		if (core->HasFeature(GFFlags::AUTOMAP_INI)) {
			/* PST toggles the active bit only, and we need to keep the rest. */
			auto flags = (an->originalFlags & ~AreaAnimation::Flags::Active) | (an->flags & AreaAnimation::Flags::Active);
			stream->WriteEnum(flags);
		} else {
			stream->WriteEnum(an->flags);
		}

		stream->WriteScalar(an->height);
		stream->WriteWord(an->transparency);
		stream->WriteWord(an->startFrameRange); //used by A_ANI_RANDOM_START
		stream->Write(&an->startchance, 1);
		stream->Write(&an->skipcycle, 1);
		stream->WriteResRef(an->PaletteRef);
		stream->WriteDword(an->unknown48); //seems utterly unused
	}
	return 0;
}

int AREImporter::PutEntrances(DataStream* stream, const Map* map) const
{
	for (unsigned int i = 0; i < EntrancesCount; i++) {
		const Entrance* e = map->GetEntrance(i);

		stream->WriteVariable(e->Name);
		stream->WritePoint(e->Pos);
		stream->WriteWord(e->Face);
		//a large empty piece of crap
		stream->WriteFilling(66);
	}
	return 0;
}

int AREImporter::PutVariables(DataStream* stream, const Map* map) const
{
	for (const auto& entry : map->locals) {
		size_t len = entry.first.length();
		stream->Write(entry.first.c_str(), len);
		if (len < 40) {
			stream->WriteFilling(40 - len);
		}
		stream->WriteDword(entry.second);
		//40 bytes of empty crap
		stream->WriteFilling(40);
	}
	return 0;
}

int AREImporter::PutAmbients(DataStream* stream, const Map* map) const
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
			stream->WriteResRef(am->sounds[j]);
		}
		while (j++ < MAX_RESCOUNT) {
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

int AREImporter::PutMapnotes(DataStream* stream, const Map* map) const
{
	//different format
	int pst = core->HasFeature(GFFlags::AUTOMAP_INI);

	for (unsigned int i = 0; i < NoteCount; i++) {
		const MapNote& mn = map->GetMapNote(i);

		if (pst) {
			// in PST the coordinates are stored in small map space
			const Size& mapsize = map->GetSize();
			stream->WriteDword(static_cast<ieDword>(mn.Pos.x * double(map->SmallMap->Frame.w) / mapsize.w));
			stream->WriteDword(static_cast<ieDword>(mn.Pos.y * double(map->SmallMap->Frame.h) / mapsize.h));

			size_t len = 0;
			// limited to 500 *bytes* of text, convert to a multibyte encoding.
			// we convert to MB because it fits more than if we wrote the wide characters
			std::string mbstring = TLKStringFromString(mn.text);
			len = std::min<size_t>(mbstring.length(), 500);
			stream->Write(mbstring.c_str(), len);

			// pad the remaining space
			size_t x = 500 - len;
			for (size_t j = 0; j < x / 8; ++j) {
				stream->WriteFilling(8);
			}
			x = x % 8;
			if (x) {
				stream->WriteFilling(x);
			}
			stream->WriteDword(mn.readonly);
			for (x = 0; x < 5; x++) { //5 empty dwords
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

int AREImporter::PutEffects(DataStream* stream, const EffectQueue& fxqueue) const
{
	PluginHolder<EffectMgr> eM = MakePluginHolder<EffectMgr>(IE_EFF_CLASS_ID);
	assert(eM != nullptr);

	auto f = fxqueue.GetFirstEffect();
	ieDword EffectsCount = fxqueue.GetSavedEffectsCount();
	for (unsigned int i = 0; i < EffectsCount; i++) {
		const Effect* fx = fxqueue.GetNextSavedEffect(f);

		assert(fx != NULL);

		eM->PutEffectV2(stream, fx);
	}
	return 0;
}

int AREImporter::PutTraps(DataStream* stream, const Map* map) const
{
	ieDword Offset;
	ResRef name;
	ieWord type = 0;
	Point dest(0, 0);

	Offset = EffectOffset;
	proIterator iter;
	ieDword i = map->GetTrapCount(iter);
	while (i--) {
		ieWord tmpWord = 0;
		ieByte tmpByte = 0xff;
		const Projectile* pro = map->GetNextTrap(iter);
		if (pro) {
			//The projectile ID is based on missile.ids which is
			//off by one compared to projectl.ids
			type = pro->GetType() + 1;
			dest = pro->GetDestination();
			const ResRef& proName = pro->GetName();
			name = proName;
			const EffectQueue& fxqueue = pro->GetEffects();
			if (fxqueue) {
				tmpWord = static_cast<ieWord>(fxqueue.GetSavedEffectsCount());
			}
			ieDword ID = pro->GetCaster();
			// lookup caster via Game, since the the current map can already be empty when switching them
			const Actor* actor = core->GetGame()->GetActorByGlobalID(ID);
			//0xff if not in party
			//party slot if in party
			if (actor) tmpByte = (ieByte) (actor->InParty - 1);
		}

		stream->WriteResRefUC(name);
		stream->WriteDword(Offset);
		//size of fxqueue;
		assert(tmpWord < 256);
		tmpWord *= 0x108;
		Offset += tmpWord;
		stream->WriteWord(tmpWord); //size in bytes
		stream->WriteWord(type); //missile.ids
		stream->WriteDword(0); // unknown field, Ticks
		stream->WritePoint(dest);
		stream->WriteWord(0); // unknown field, Z
		stream->Write(&tmpByte, 1); // unknown field, TargetType
		stream->Write(&tmpByte, 1); // Owner
	}
	return 0;
}

int AREImporter::PutExplored(DataStream* stream, const Map* map) const
{
	stream->Write(map->ExploredBitmap.begin(), ExploredBitmapSize);
	return 0;
}

int AREImporter::PutTiles(DataStream* stream, const Map* map) const
{
	for (unsigned int i = 0; i < TileCount; i++) {
		const TileObject* am = map->TMap->GetTile(i);
		stream->WriteVariable(am->name);
		stream->WriteResRef(am->tileset);
		stream->WriteDword(am->flags);
		stream->WriteDword(am->openCount);
		//can't write tiles, otherwise now we should write a tile index
		stream->WriteDword(0);
		stream->WriteDword(am->closedCount);
		//can't write tiles otherwise now we should write a tile index
		stream->WriteDword(0);
		stream->WriteFilling(48);
	}
	return 0;
}

ieWord AREImporter::SavedAmbientCount(const Map* map) const
{
	ieWord count = 0;
	for (const Ambient* am : map->GetAmbients()) {
		if (am->flags & IE_AMBI_NOSAVE) continue;
		++count;
	}
	return count;
}

int AREImporter::PutMapAmbients(DataStream* stream, const Map* map) const
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

int AREImporter::PutRestHeader(DataStream* stream, const Map* map) const
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
	stream->WriteDword(map->RestHeader.Duration);
	stream->WriteWord(map->RestHeader.RandomWalkDistance);
	stream->WriteWord(map->RestHeader.FollowDistance);
	stream->WriteWord(map->RestHeader.Maximum);
	stream->WriteWord(map->RestHeader.Enabled);
	stream->WriteWord(map->RestHeader.DayChance);
	stream->WriteWord(map->RestHeader.NightChance);
	stream->WriteFilling(56);
	return 0;
}

/* no saving of tiled objects, are they used anywhere? */
int AREImporter::PutArea(DataStream* stream, const Map* map) const
{
	ieDword VertIndex = 0;
	int ret;

	if (!stream || !map) {
		return -1;
	}

	ret = PutHeader(stream, map);
	if (ret) {
		return ret;
	}

	ret = PutActors(stream, map);
	if (ret) {
		return ret;
	}

	ret = PutRegions(stream, map, VertIndex);
	if (ret) {
		return ret;
	}

	ret = PutSpawns(stream, map);
	if (ret) {
		return ret;
	}

	ret = PutEntrances(stream, map);
	if (ret) {
		return ret;
	}

	ret = PutContainers(stream, map, VertIndex);
	if (ret) {
		return ret;
	}

	ret = PutItems(stream, map);
	if (ret) {
		return ret;
	}

	ret = PutDoors(stream, map, VertIndex);
	if (ret) {
		return ret;
	}

	ret = PutVertices(stream, map);
	if (ret) {
		return ret;
	}

	ret = PutAmbients(stream, map);
	if (ret) {
		return ret;
	}

	ret = PutVariables(stream, map);
	if (ret) {
		return ret;
	}

	ret = PutAnimations(stream, map);
	if (ret) {
		return ret;
	}

	ret = PutTiles(stream, map);
	if (ret) {
		return ret;
	}

	ret = PutExplored(stream, map);
	if (ret) {
		return ret;
	}

	proIterator iter;
	ieDword i = map->GetTrapCount(iter);
	while (i--) {
		const Projectile* trap = map->GetNextTrap(iter);
		if (!trap) {
			continue;
		}

		const EffectQueue& fxqueue = trap->GetEffects();

		if (!fxqueue) {
			continue;
		}

		ret = PutEffects(stream, fxqueue);
		if (ret) {
			return ret;
		}
	}

	ret = PutTraps(stream, map);
	if (ret) {
		return ret;
	}

	ret = PutMapnotes(stream, map);
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

	ret = PutRestHeader(stream, map);

	return ret;
}

#include "plugindef.h"

GEMRB_PLUGIN(0x145B60F0, "ARE File Importer")
PLUGIN_CLASS(IE_ARE_CLASS_ID, ImporterPlugin<AREImporter>)
END_PLUGIN()
