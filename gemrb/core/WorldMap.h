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

/**
 * @file WorldMap.h
 * Declares WorldMap, class describing a top level map of the world.
 * @author The GemRB Project
 */


#ifndef WORLDMAP_H
#define WORLDMAP_H

#include "exports.h"
#include "ie_types.h"

#include "AnimationFactory.h"
#include "EnumIndex.h"
#include "Sprite2D.h"

#include <vector>

namespace GemRB {

/** this is the physical order the links appear in WMPAreaEntry */
enum class WMPDirection : uint8_t {
	NONE = 0xff,
	NORTH = 0,
	WEST = 1,
	SOUTH = 2,
	EAST = 3,
	count
};

/** Area is visible on WorldMap */
#define WMP_ENTRY_VISIBLE    0x1
/** Area is visible on WorldMap only when party is in adjacent area */
#define WMP_ENTRY_ADJACENT   0x2
/** Area can be travelled into from WorldMap */
#define WMP_ENTRY_ACCESSIBLE 0x4
/** Area has already been visited by party */
#define WMP_ENTRY_VISITED    0x8
/** Area can be travelled into from WorldMap */
#define WMP_ENTRY_WALKABLE   (WMP_ENTRY_VISIBLE|WMP_ENTRY_ACCESSIBLE)
/** Area can be passed through when travelling directly to some more distant area on WorldMap */
#define WMP_ENTRY_PASSABLE   (WMP_ENTRY_VISIBLE|WMP_ENTRY_ACCESSIBLE|WMP_ENTRY_VISITED)


/**
 * @class WMPAreaEntry
 * Holds information about an Area on a WorldMap.
 */

class GEM_EXPORT WMPAreaEntry {
public:
	WMPAreaEntry() noexcept = default;
	ieDword GetAreaStatus() const;
	void SetAreaStatus(ieDword status, BitOp op);

	//! return the map icon of this location. Free the sprite afterwards.
	Holder<Sprite2D> GetMapIcon(const AnimationFactory *bam);
	// note that this is only valid after GetMapIcon has been called
	bool HighlightSelected() const { return SingleFrame; }
	String GetCaption();
	String GetTooltip();
private:
	ieDword AreaStatus = 0;
	Holder<Sprite2D> MapIcon = nullptr;
	String StrCaption;
	String StrTooltip;
	bool SingleFrame = false;

public:
	ResRef AreaName;
	ResRef AreaResRef;
	ieVariable AreaLongName;
	ieDword IconSeq = 0;
	Point pos;
	ieStrRef LocCaptionName = ieStrRef::INVALID;
	ieStrRef LocTooltipName = ieStrRef::INVALID;
	ResRef LoadScreenResRef;
	EnumArray<WMPDirection, ieDword> AreaLinksIndex;
	EnumArray<WMPDirection, ieDword> AreaLinksCount;
};

/**
 * @struct WMPAreaLink
 * Defines connection and travelling between WorldMap areas
 */

struct WMPAreaLink {
	ieDword AreaIndex;
	ieVariable DestEntryPoint;
	ieDword DistanceScale;
	ieDword DirectionFlags; //where will the player appear on dest. area
	ResRef EncounterAreaResRef[5];
	ieDword EncounterChance;
};

/**
 * @class WorldMap
 * Top level map of the world.
 * Also defines links between areas, although they are used only when travelling from this map.
 */

class GEM_EXPORT WorldMap {
public: //struct members
	ResRef MapResRef;
	ieDword Width = 0;
	ieDword Height = 0;
	ieDword MapNumber = 0;
	ieStrRef AreaName = ieStrRef::INVALID;
	ieDword unknown1 = 0;
	ieDword unknown2 = 0;
	ResRef MapIconResRef;
	ieDword Flags = 0;

	AnimationFactory* bam = nullptr;
private: //non-struct members
	Holder<Sprite2D> MapMOS = nullptr;
	std::vector<WMPAreaEntry> area_entries;
	std::vector<WMPAreaLink> area_links;
	std::vector<int> Distances;
	std::vector<int> GotHereFrom;
	size_t encounterArea = -1;
public:
	WorldMap() noexcept = default;

	void SetMapIcons(AnimationFactory *bam);
	Holder<Sprite2D> GetMapMOS() const { return MapMOS; }
	void SetMapMOS(Holder<Sprite2D> newmos);
	int GetEntryCount() const { return (int) area_entries.size(); }
	WMPAreaEntry *GetEntry(unsigned int index) { return &area_entries[index]; }
	const WMPAreaEntry *GetEntry(unsigned int index) const { return &area_entries[index]; }
	int GetLinkCount() const { return (int) area_links.size(); }
	const WMPAreaLink *GetLink(unsigned int index) const { return &area_links[index]; }
	void SetAreaEntry(unsigned int index, WMPAreaEntry&& areaentry);
	void InsertAreaLink(unsigned int idx, WMPDirection dir, WMPAreaLink&& arealink);
	void SetAreaLink(unsigned int index, const WMPAreaLink *arealink);
	void AddAreaEntry(WMPAreaEntry&& ae);
	void AddAreaLink(WMPAreaLink&& al);
	/** Calculates the distances from A, call this when first on an area */
	int CalculateDistances(const ResRef& A, WMPDirection direction);
	/** Returns the precalculated distance to area B */
	int GetDistance(const ResRef& A) const;
	/** Returns the link between area A and area B */
	WMPAreaLink *GetLink(const ResRef& A, const ResRef& B);
	/** Returns the area link we will fall into if we head in B direction */
	/** If the area name differs it means we are in a random encounter */
	WMPAreaLink *GetEncounterLink(const ResRef& B, bool &encounter);
	/** Sets area status */
	void SetAreaStatus(const ResRef&, int Bits, BitOp Op);
	/** Gets area pointer and index from area name.
	 * also called from WorldMapArray to find the right map	*/
	WMPAreaEntry* GetArea(const ResRef& areaName, unsigned int &i);
	const WMPAreaEntry* GetArea(const ResRef& areaName, unsigned int &i) const;
	/** Finds an area name closest to the given area */
	const WMPAreaEntry* FindNearestEntry(const ResRef& areaName, unsigned int &i) const;
	void SetEncounterArea(const ResRef& area, const WMPAreaLink *link);
	void ClearEncounterArea();
private:
	/** updates visibility of adjacent areas, called from CalculateDistances */
	void UpdateAreaVisibility(const ResRef& areaName, WMPDirection direction);
	/** internal function to calculate the distances from areaindex */
	void CalculateDistance(int areaindex, int direction);
	unsigned int WhoseLinkAmI(int link_index) const;
	/** update reachable areas from worlde.2da */
	void UpdateReachableAreas();
};

class GEM_EXPORT WorldMapArray {
private:
	mutable std::vector<WorldMap> maps; // FIXME: our constness is all screwed up
	size_t CurrentMap = 0;
	bool single = true;
public:
	explicit WorldMapArray(size_t count);

	bool IsSingle() const { return single; }
	void SetSingle(bool arg) { single = arg; }
	size_t GetMapCount() const { return maps.size(); }
	size_t GetCurrentMapIndex() const { return CurrentMap; }
	WorldMap *NewWorldMap(size_t index);
	WorldMap *GetWorldMap(size_t index) const { return &maps[index]; }
	WorldMap *GetCurrentMap() const { return &maps[CurrentMap]; }
	void SetWorldMap(size_t index);
	void SetCurrentMap(size_t index) { CurrentMap = index; }
	size_t FindAndSetCurrentMap(const ResRef& area);
};

}

#endif // ! WORLDMAP_H
