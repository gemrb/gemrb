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
#include "Sprite2D.h"

#include <vector>

namespace GemRB {

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

/** this is the physical order the links appear in WMPAreaEntry */
typedef enum ieDirectionType {
	WMP_NORTH=0,
	WMP_WEST=1,
	WMP_SOUTH=2, 
	WMP_EAST=3
} ieDirectionType;

/**
 * @class WMPAreaEntry
 * Holds information about an Area on a WorldMap.
 */

class GEM_EXPORT WMPAreaEntry {
public:
	WMPAreaEntry();
	~WMPAreaEntry();
	ieDword GetAreaStatus();
	void SetAreaStatus(ieDword status, int op);

	//! return the map icon of this location. Free the sprite afterwards.
	Sprite2D *GetMapIcon(AnimationFactory *bam);
	const char* GetCaption();
	const char* GetTooltip();
private:
	ieDword AreaStatus;
	Sprite2D *MapIcon;
	char *StrCaption;
	char *StrTooltip;

	void SetPalette(int gradient, Sprite2D *MapIcon);
public:
	ieResRef AreaName;
	ieResRef AreaResRef;
	ieVariable AreaLongName;
	ieDword IconSeq;
	ieDword X;
	ieDword Y;
	ieStrRef LocCaptionName;
	ieStrRef LocTooltipName;
	ieResRef LoadScreenResRef;
	ieDword AreaLinksIndex[4];
	ieDword AreaLinksCount[4];
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
	ieResRef EncounterAreaResRef[5];
	ieDword EncounterChance;
};

/**
 * @class WorldMap
 * Top level map of the world.
 * Also defines links between areas, although they are used only when travelling from this map.
 */

class GEM_EXPORT WorldMap {
public:
	WorldMap();
	~WorldMap();
public: //struct members
	ieResRef MapResRef;
	ieDword Width;
	ieDword Height;
	ieDword MapNumber;
	ieStrRef AreaName;
	ieDword unknown1;
	ieDword unknown2;
	ieDword AreaEntriesCount;
	ieDword AreaEntriesOffset;
	ieDword AreaLinksOffset;
	ieDword AreaLinksCount;
	ieResRef MapIconResRef;

	AnimationFactory *bam;
private: //non-struct members
	Sprite2D* MapMOS;
	std::vector< WMPAreaEntry*> area_entries;
	std::vector< WMPAreaLink*> area_links;
	int *Distances;
	int *GotHereFrom;
public:
	void SetMapIcons(AnimationFactory *bam);
	Sprite2D* GetMapMOS() const { return MapMOS; }
	void SetMapMOS(Sprite2D *newmos);
	int GetEntryCount() const { return (int) area_entries.size(); }
	WMPAreaEntry *GetEntry(unsigned int index) { return area_entries[index]; }
	int GetLinkCount() const { return (int) area_links.size(); }
	WMPAreaLink *GetLink(unsigned int index) const { return area_links[index]; }
	WMPAreaEntry *GetNewAreaEntry() const;
	void SetAreaEntry(unsigned int index, WMPAreaEntry *areaentry);
	void InsertAreaLink(unsigned int idx, unsigned int dir, WMPAreaLink *arealink);
	void SetAreaLink(unsigned int index, WMPAreaLink *arealink);
	void AddAreaEntry(WMPAreaEntry *ae);
	void AddAreaLink(WMPAreaLink *al);
	/** Calculates the distances from A, call this when first on an area */
	int CalculateDistances(const ieResRef A, int direction);
	/** Returns the precalculated distance to area B */
	int GetDistance(const ieResRef A) const;
	/** Returns the link between area A and area B */
	WMPAreaLink *GetLink(const ieResRef A, const ieResRef B) const;
	/** Returns the area link we will fall into if we head in B direction */
	/** If the area name differs it means we are in a random encounter */
	WMPAreaLink *GetEncounterLink(const ieResRef B, bool &encounter) const;
	/** Sets area status */
	void SetAreaStatus(const ieResRef, int Bits, int Op);
	/** Gets area pointer and index from area name.
	 * also called from WorldMapArray to find the right map	*/
	WMPAreaEntry* GetArea(const ieResRef AreaName, unsigned int &i) const;
	/** Finds an area name closest to the given area */
	WMPAreaEntry* FindNearestEntry(const ieResRef AreaName, unsigned int &i) const;
private:
	/** updates visibility of adjacent areas, called from CalculateDistances */
	void UpdateAreaVisibility(const ieResRef AreaName, int direction);
	/** internal function to calculate the distances from areaindex */
	void CalculateDistance(int areaindex, int direction);
	unsigned int WhoseLinkAmI(int link_index) const;
	/** update reachable areas from worlde.2da */
	void UpdateReachableAreas();
};

class GEM_EXPORT WorldMapArray {
public:
	WorldMapArray(unsigned int count);
	~WorldMapArray();
	void SetWorldMap(WorldMap *m, unsigned int index);
private:
	WorldMap **all_maps;
	unsigned int MapCount;
	unsigned int CurrentMap;
	bool single;
public:
	bool IsSingle() const { return single; }
	void SetSingle(bool arg) { single = arg; }
	unsigned int GetMapCount() const { return MapCount; }
	unsigned int GetCurrentMapIndex() const { return CurrentMap; }
	WorldMap *NewWorldMap(unsigned int index);
	WorldMap *GetWorldMap(unsigned int index) const { return all_maps[index]; }
	WorldMap *GetCurrentMap() const { return all_maps[CurrentMap]; }
	void SetWorldMap(unsigned int index);
	void SetCurrentMap(unsigned int index) { CurrentMap = index; }
	unsigned int FindAndSetCurrentMap(const ieResRef area);
};

}

#endif // ! WORLDMAP_H
