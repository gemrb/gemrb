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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/WorldMap.h,v 1.9 2005/02/19 16:46:50 avenger_teambg Exp $
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
	ieDword Flags;
	ieResRef EncounterAreaResRef[5];
	ieDword EncounterChance;
//	char unknown[128];
} WMPAreaLink;

class GEM_EXPORT WorldMap {
public:
	WorldMap();
	~WorldMap();
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
	Sprite2D* MapMOS;
public:
	void AddAreaEntry(WMPAreaEntry *ae);
	void AddAreaLink(WMPAreaLink *al);
	std::vector< WMPAreaEntry*> area_entries;
	std::vector< WMPAreaLink*> area_links;
	/* returns area entry indexed by name */
	WMPAreaEntry* GetArea(const ieResRef AreaName);
	/* updates visibility of adjacent areas */
	void UpdateAreaVisibility(const ieResRef AreaName, int direction);
	/* sets area status */
	void SetAreaStatus(const ieResRef, int Bits, int Op);
};

#endif  // ! WORLDMAP_H
