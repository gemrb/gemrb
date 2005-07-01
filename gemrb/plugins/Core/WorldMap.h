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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/WorldMap.h,v 1.15 2005/07/01 20:39:36 avenger_teambg Exp $
 *
 */

#ifndef WORLDMAP_H
#define WORLDMAP_H

#include <vector>
#include "../../includes/ie_types.h"

#include "Sprite2D.h"


#ifdef WIN32

#ifdef GEM_BUILD_DLL
#define GEM_EXPORT __declspec(dllexport)
#else
#define GEM_EXPORT __declspec(dllimport)
#endif

#else
#define GEM_EXPORT
#endif


#define WMP_ENTRY_VISIBLE    0x1
#define WMP_ENTRY_ADJACENT   0x2   // visible from adjacent
#define WMP_ENTRY_ACCESSIBLE 0x4
#define WMP_ENTRY_VISITED    0x8
#define WMP_ENTRY_WALKABLE   (WMP_ENTRY_VISIBLE|WMP_ENTRY_ACCESSIBLE)
#define WMP_ENTRY_PASSABLE   (WMP_ENTRY_VISIBLE|WMP_ENTRY_ACCESSIBLE|WMP_ENTRY_VISITED)

//this is the physical order they appear in WMPAreaEntry
typedef enum ieDirectionType {
	WMP_NORTH=0,
	WMP_WEST=1,
	WMP_SOUTH=2, 
	WMP_EAST=3
} ieDirectionType;

class GEM_EXPORT WMPAreaEntry {
public:
	WMPAreaEntry();
	~WMPAreaEntry();
	ieResRef AreaName;
	ieResRef AreaResRef;
	char AreaLongName[32];
	ieDword AreaStatus;
	ieDword IconSeq;
	ieDword X;
	ieDword Y;
	ieStrRef LocCaptionName;
	ieStrRef LocTooltipName;
	ieResRef LoadScreenResRef;
	ieDword AreaLinksIndex[4];
	ieDword AreaLinksCount[4];

	Sprite2D* MapIcon;
};


typedef struct WMPAreaLink {
	ieDword AreaIndex;
	char DestEntryPoint[32];
	ieDword DistanceScale;
	ieDword DirectionFlags; //where will the player appear on dest. area
	ieResRef EncounterAreaResRef[5];
	ieDword EncounterChance;
} WMPAreaLink;

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

private: //non-struct members
	Sprite2D* MapMOS;
	std::vector< WMPAreaEntry*> area_entries;
	std::vector< WMPAreaLink*> area_links;
	int *Distances;
	int *GotHereFrom;
public:
	Sprite2D* GetMapMOS() { return MapMOS; }
	void SetMapMOS(Sprite2D *newmos);
	int GetEntryCount() { return area_entries.size(); }
	WMPAreaEntry *GetEntry(unsigned int index) { return area_entries[index]; }
	int GetLinkCount() { return area_links.size(); }
	WMPAreaLink *GetLink(unsigned int index) { return area_links[index]; }
	void SetAreaEntry(unsigned int index, WMPAreaEntry *areaentry);
	void SetAreaLink(unsigned int index, WMPAreaLink *arealink);
	void AddAreaEntry(WMPAreaEntry *ae);
	void AddAreaLink(WMPAreaLink *al);
	/* calculates the distances from A, call this when first on an area */
	int CalculateDistances(const ieResRef A, int direction);
	/* returns the precalculated distance to area B */
	int GetDistance(const ieResRef A);
	/* returns the link between area A and area B */
	WMPAreaLink *GetLink(const ieResRef A, const ieResRef B);
	/* returns the area link we will fall into if we head in B direction */
	/* if the area name differs it means we are in a random encounter */
	WMPAreaLink *GetEncounterLink(const ieResRef B, bool &encounter);
	/* sets area status */
	void SetAreaStatus(const ieResRef, int Bits, int Op);
	//internal function to get area pointer and index from area name
	//also called from WorldMapArray to find the right map	
	WMPAreaEntry* GetArea(const ieResRef AreaName, unsigned int &i);
private:
	// updates visibility of adjacent areas, called from CalculateDistances
	void UpdateAreaVisibility(const ieResRef AreaName, int direction);
	//internal function to calculate the distances from areaindex
	void CalculateDistance(int areaindex, int direction);
	unsigned int WhoseLinkAmI(int link_index);
};

class GEM_EXPORT WorldMapArray {
public:
	WorldMapArray(int count);
	~WorldMapArray();
	void SetWorldMap(WorldMap *m, unsigned int index);
private:
	WorldMap **all_maps;
	unsigned int MapCount;
	unsigned int CurrentMap;
public:
	int GetMapCount() { return MapCount; }
	int GetCurrentMapIndex() { return CurrentMap; }
	WorldMap *NewWorldMap(unsigned int index);
	WorldMap *GetWorldMap(unsigned int index) { return all_maps[index]; }
	WorldMap *GetCurrentMap() { return all_maps[CurrentMap]; }
	void SetWorldMap(unsigned int index);
	void SetCurrentMap(unsigned int index) { CurrentMap = index; }
	int FindAndSetCurrentMap(const ieResRef area);
};

#endif // ! WORLDMAP_H
