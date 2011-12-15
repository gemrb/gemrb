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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

#include "WorldMap.h"

#include "win32def.h"

#include "Game.h"
#include "Interface.h"
#include "TableMgr.h"
#include "Video.h"

#include <list>

WMPAreaEntry::WMPAreaEntry()
{
	MapIcon = NULL;
	StrCaption = NULL;
	StrTooltip = NULL;
}

WMPAreaEntry::~WMPAreaEntry()
{
	if (StrCaption) {
		core->FreeString(StrCaption);
	}
	if (StrTooltip) {
		core->FreeString(StrTooltip);
	}
	core->GetVideoDriver()->FreeSprite(MapIcon);
}

void WMPAreaEntry::SetAreaStatus(ieDword arg, int op)
{
	switch (op) {
	case BM_SET: AreaStatus = arg; break;
	case BM_OR: AreaStatus |= arg; break;
	case BM_NAND: AreaStatus &= ~arg; break;
	case BM_XOR: AreaStatus ^= arg; break;
	case BM_AND: AreaStatus &= arg; break;
	}
	core->GetVideoDriver()->FreeSprite(MapIcon);
}

const char* WMPAreaEntry::GetCaption()
{
	if (!StrCaption) {
		StrCaption = core->GetString(LocCaptionName);
	}
	return StrCaption;
}

const char* WMPAreaEntry::GetTooltip()
{
	if (!StrTooltip) {
		StrTooltip = core->GetString(LocTooltipName);
	}
	return StrTooltip;
}

static int gradients[5]={18,22,19,3,4};

void WMPAreaEntry::SetPalette(int gradient, Sprite2D* MapIcon)
{
	if (!MapIcon) return;
	Palette *palette = new Palette;
	core->GetPalette( gradient&255, 256, palette->col );
	MapIcon->SetPalette(palette);
}

Sprite2D *WMPAreaEntry::GetMapIcon(AnimationFactory *bam)
{
	if (!bam) {
		return NULL;
	}
	if (!MapIcon) {
		int color = -1;
		int frame = 0;
		switch (AreaStatus&(WMP_ENTRY_ACCESSIBLE|WMP_ENTRY_VISITED))
		{
			case WMP_ENTRY_ACCESSIBLE: frame = 0; break;
			case WMP_ENTRY_VISITED: frame = 4; break;
			case WMP_ENTRY_ACCESSIBLE|WMP_ENTRY_VISITED: frame = 1; break;
			case 0: frame = 2; break;
		}
		if (bam->GetCycleSize(IconSeq)<5) {
			color = gradients[frame];
			frame = 0;
		}
		MapIcon = bam->GetFrame((ieWord) frame, (ieByte) IconSeq);
		if (!MapIcon) {
			print("WMPAreaEntry::GetMapIcon failed for frame %d, seq %d\n", frame, IconSeq);
			return NULL;
		}
		if (color>=0) {
			// Note: should a game use the same map icon for two different
			// map locations, we have to duplicate the MapIcon sprite here.
			// This doesn't occur in BG1, so no need to do that for the moment.
			SetPalette(color, MapIcon);
		}
	}
	MapIcon->acquire();
	return MapIcon;
}

ieDword WMPAreaEntry::GetAreaStatus()
{
	ieDword tmp = AreaStatus;
	if (core->HasFeature(GF_KNOW_WORLD) ) {
		tmp |=WMP_ENTRY_VISITED;
	}
	return tmp;
}

WorldMap::WorldMap(void)
{
	MapMOS = NULL;
	Distances = NULL;
	GotHereFrom = NULL;
	bam = NULL;
}

//Allocate AE and AL only in Core, otherwise Win32 will
//be buggy
void WorldMap::AddAreaEntry(WMPAreaEntry *ae)
{
	area_entries.push_back(ae);
}

void WorldMap::AddAreaLink(WMPAreaLink *al)
{
	area_links.push_back(al);
}

WMPAreaEntry *WorldMap::GetNewAreaEntry() const
{
	return new WMPAreaEntry();
}

void WorldMap::SetAreaEntry(unsigned int x, WMPAreaEntry *ae)
{
	//if index is too large, we break
	if (x>area_entries.size()) {
		abort();
	}
	//altering an existing entry
	if (x<area_entries.size()) {
		if (area_entries[x]) {
			delete area_entries[x];
		}
		area_entries[x]=ae;
		return;
	}
	//adding a new entry
	area_entries.push_back(ae);
}

void WorldMap::InsertAreaLink(unsigned int areaidx, unsigned int dir, WMPAreaLink *arealink)
{
	unsigned int pos;
	WMPAreaEntry *ae;

	WMPAreaLink *al = new WMPAreaLink();
	memcpy(al, arealink, sizeof(WMPAreaLink) );
	unsigned int idx = area_entries[areaidx]->AreaLinksIndex[dir];
	area_links.insert(area_links.begin()+idx,al);

	unsigned int max = area_entries.size();
	for(pos = 0; pos<max; pos++) {
		ae = area_entries[pos];
		for (unsigned int k=0;k<4;k++) {
			if ((pos==areaidx) && (k==dir)) {
				ae->AreaLinksCount[k]++;
				continue;
			}
			if(ae->AreaLinksIndex[k]>=idx) {
				ae->AreaLinksIndex[k]++;
			}
		}
	}
	//update the link count, just in case
	AreaLinksCount++;
}

void WorldMap::SetAreaLink(unsigned int x, WMPAreaLink *arealink)
{
	WMPAreaLink *al =new WMPAreaLink();

	//change this to similar code as above if WMPAreaLink gets non-struct members
	memcpy( al,arealink,sizeof(WMPAreaLink) );

	//if index is too large, we break
	if (x>area_links.size()) {
		abort();
	}
	//altering an existing link
	if (x<area_links.size()) {
		if (area_links[x]) {
			delete area_links[x];
		}
		area_links[x]=al;
		return;
	}
	//adding a new link
	area_links.push_back(al);
}

WorldMap::~WorldMap(void)
{
	unsigned int i;

	for (i = 0; i < area_entries.size(); i++) {
		delete( area_entries[i] );
	}
	for (i = 0; i < area_links.size(); i++) {
		delete( area_links[i] );
	}
	if (MapMOS) {
		core->GetVideoDriver()->FreeSprite(MapMOS);
	}
	if (Distances) {
		free(Distances);
	}
	if (GotHereFrom) {
		free(GotHereFrom);
	}
	if (bam) bam = NULL;
}

void WorldMap::SetMapIcons(AnimationFactory *newicons)
{
	bam = newicons;
}
void WorldMap::SetMapMOS(Sprite2D *newmos)
{
	if (MapMOS) {
		core->GetVideoDriver()->FreeSprite(MapMOS);
	}
	MapMOS = newmos;
}

WMPAreaEntry* WorldMap::GetArea(const ieResRef AreaName, unsigned int &i) const
{
	i=(unsigned int) area_entries.size();
	while (i--) {
		if (!strnicmp(AreaName, area_entries[i]->AreaName,8)) {
			return area_entries[i];
		}
	}
	return NULL;
}

//Find Worldmap location by nearest area with a smaller number
//Counting backwards, stop at 1000 boundaries.
//It is not possible to simply round to 1000, because there are 
//WMP entries like AR8001, and we need to find the best match
WMPAreaEntry* WorldMap::FindNearestEntry(const ieResRef AreaName, unsigned int &i) const
{
	int value = 0;
	ieResRef tmp;

	sscanf(&AreaName[2],"%4d", &value);
	do {
		snprintf(tmp, 9, "%.2s%04d", AreaName, value);
		WMPAreaEntry* ret = GetArea(tmp, i);
		if (ret) {
			return ret;
		}
		if (value%1000 == 0) break;
		value--;
	}
	while(1); //value%1000 should protect us from infinite loops
	i = -1;
	return NULL;
}

//this is a pathfinding algorithm
//you have to find the optimal path
int WorldMap::CalculateDistances(const ieResRef AreaName, int direction)
{
	//first, update reachable/visible areas by worlde.2da if exists
	UpdateReachableAreas();
	UpdateAreaVisibility(AreaName, direction);
	if (direction==-1) {
		return 0;
	}

	if (direction<0 || direction>3) {
		printMessage("WorldMap", "CalculateDistances for invalid direction: %s\n", LIGHT_RED, AreaName);
		return -1;
	}

	unsigned int i;
	if (!GetArea(AreaName, i)) {
		printMessage("WorldMap", "CalculateDistances for invalid Area: %s\n", LIGHT_RED, AreaName);
		return -1;
	}
	if (Distances) {
		free(Distances);
	}
	if (GotHereFrom) {
		free(GotHereFrom);
	}

	printMessage("WorldMap", "CalculateDistances for Area: %s\n", GREEN, AreaName);

	size_t memsize =sizeof(int) * area_entries.size();
	Distances = (int *) malloc( memsize );
	GotHereFrom = (int *) malloc( memsize );
	memset( Distances, -1, memsize );
	memset( GotHereFrom, -1, memsize );
	Distances[i] = 0; //setting our own distance
	GotHereFrom[i] = -1; //we didn't move

	int *seen_entry = (int *) malloc( memsize );

	std::list<int> pending;
	pending.push_back(i);
	while(pending.size()) {
		i=pending.front();
		pending.pop_front();
		WMPAreaEntry* ae=area_entries[i];
		memset( seen_entry, -1, memsize );
		//all directions should be used
		for(int d=0;d<4;d++) {
			int j=ae->AreaLinksIndex[d];
			int k=j+ae->AreaLinksCount[d];
			if ((size_t) k>area_links.size()) {
				printMessage("WorldMap", "The worldmap file is corrupted... and it would crash right now!\nEntry #: %d Direction: %d\n", RED,
					i, d);
				break;
			}
			for(;j<k;j++) {
				WMPAreaLink* al = area_links[j];
				WMPAreaEntry* ae2 = area_entries[al->AreaIndex];
				unsigned int mydistance = (unsigned int) Distances[i];

				// we must only process the FIRST seen link to each area from this one
				if (seen_entry[al->AreaIndex] != -1) continue;
				seen_entry[al->AreaIndex] = 0;
/*
				if ( ( (ae->GetAreaStatus() & WMP_ENTRY_PASSABLE) == WMP_ENTRY_PASSABLE) &&
				( (ae2->GetAreaStatus() & WMP_ENTRY_WALKABLE) == WMP_ENTRY_WALKABLE)
*/
				if ( (ae2->GetAreaStatus() & WMP_ENTRY_WALKABLE) == WMP_ENTRY_WALKABLE) {
					// al->Flags is the entry direction
					mydistance += al->DistanceScale * 4;
					//nonexisting distance is the biggest!
					if ((unsigned) Distances[al->AreaIndex] > mydistance) {
						Distances[al->AreaIndex] = mydistance;
						GotHereFrom[al->AreaIndex] = j;
						pending.push_back(al->AreaIndex);
					}
				}
			}
		}
	}

	free(seen_entry);
	return 0;
}

//returns the index of the area owning this link
unsigned int WorldMap::WhoseLinkAmI(int link_index) const
{
	for (unsigned int i=0;i<AreaEntriesCount;i++) {
		WMPAreaEntry *ae=area_entries[i];
		for (int direction=0;direction<4;direction++)
		{
			int j=ae->AreaLinksIndex[direction];
			if (link_index>=j) {
				j+=ae->AreaLinksCount[direction];
				if(link_index<j) {
					return i;
				}
			}
		}
	}
	return (ieDword) -1;
}

WMPAreaLink *WorldMap::GetLink(const ieResRef A, const ieResRef B) const
{
	unsigned int i,j,k;

	WMPAreaEntry *ae=GetArea( A, i );
	if (!ae) {
		return NULL;
	}
	//looking for destination area, returning the first link found
	for (i=0;i<4;i++) {
		j=ae->AreaLinksCount[i];
		k=ae->AreaLinksIndex[i];
		while(j--) {
			WMPAreaLink *al = area_links[k++];
			WMPAreaEntry *ae2 = area_entries[al->AreaIndex];
			//or arearesref?
			if (strnicmp(ae2->AreaName, B, 8)==0) {
				return al;
			}
		}
	}
	return NULL;
}

//call this function to find out which area we fall into
//not necessarily the target area
//if it isn't the same, then a random encounter happened!
WMPAreaLink *WorldMap::GetEncounterLink(const ieResRef AreaName, bool &encounter) const
{
	if (!GotHereFrom) {
		return NULL;
	}
	unsigned int i;
	WMPAreaEntry *ae=GetArea( AreaName, i ); //target area
	if (!ae) {
		printMessage("WorldMap", "No such area: %s\n", LIGHT_RED, AreaName);
		return NULL;
	}
	std::list<WMPAreaLink*> walkpath;
	print("Gathering path information for: %s\n", AreaName);
	while (GotHereFrom[i]!=-1) {
		print("Adding path to %d\n", i);
		walkpath.push_back(area_links[GotHereFrom[i]]);
		i = WhoseLinkAmI(GotHereFrom[i]);
		if (i==(ieDword) -1) {
			error("WorldMap", "Something has been screwed up here (incorrect path)!\n");
		}
	}

	print("Walkpath size is: %d\n",(int) walkpath.size());
	if (!walkpath.size()) {
		return NULL;
	}
	std::list<WMPAreaLink*>::iterator p=walkpath.begin();
	WMPAreaLink *lastpath;
	encounter=false;
	do {
		lastpath = *p;
		if (lastpath->EncounterChance > (unsigned int) (rand()%100)) {
			encounter=true;
			break;
		}
		p++;
	}
	while(p!=walkpath.end() );
	return lastpath;
}

int WorldMap::GetDistance(const ieResRef AreaName) const
{
	if (!Distances) {
		return -1;
	}
	unsigned int i;
	if (GetArea( AreaName, i )) {
		return Distances[i];
	}
	return -1;
}

void WorldMap::UpdateAreaVisibility(const ieResRef AreaName, int direction)
{
	unsigned int i;

	WMPAreaEntry* ae=GetArea(AreaName,i);
	if (!ae)
		return;
	//we are here, so we visited and it is visible too (i guess)
	print("Updated Area visibility: %s (visited, accessible and visible)\n", AreaName);

	ae->SetAreaStatus(WMP_ENTRY_VISITED|WMP_ENTRY_VISIBLE|WMP_ENTRY_ACCESSIBLE, BM_OR);
	if (direction<0 || direction>3)
		return;
	i=ae->AreaLinksCount[direction];
	while (i--) {
		WMPAreaLink* al = area_links[ae->AreaLinksIndex[direction]+i];
		WMPAreaEntry* ae2 = area_entries[al->AreaIndex];
		if (ae2->GetAreaStatus()&WMP_ENTRY_ADJACENT) {
			print("Updated Area visibility: %s (accessible, and visible)\n", ae2->AreaName);
			ae2->SetAreaStatus(WMP_ENTRY_VISIBLE|WMP_ENTRY_ACCESSIBLE, BM_OR);
		}
	}
}

void WorldMap::SetAreaStatus(const ieResRef AreaName, int Bits, int Op)
{
	unsigned int i;
	WMPAreaEntry* ae=GetArea(AreaName,i);
	if (!ae)
		return;
	ae->SetAreaStatus(Bits, Op);
}

void WorldMap::UpdateReachableAreas()
{
	AutoTable tab("worlde");
	if (!tab) {
		return;
	}
	Game *game = core->GetGame();
	if (!game) {
		return;
	}
	int idx = tab->GetRowCount();
	while (idx--) {
		// 2da rows in format <name> <variable name> <area>
		// we set the first three flags for <area> if <variable name> is set
		ieDword varval = 0;
		const char *varname = tab->QueryField(idx, 0);
		if (game->locals->Lookup(varname, varval) && varval) {
			const char *areaname = tab->QueryField(idx, 1);
			SetAreaStatus(areaname, WMP_ENTRY_VISIBLE | WMP_ENTRY_ADJACENT | WMP_ENTRY_ACCESSIBLE, BM_OR);
		}
	}
}

/****************** WorldMapArray *******************/
WorldMapArray::WorldMapArray(unsigned int count)
{
	CurrentMap = 0;
	MapCount = count;
	all_maps = (WorldMap **) calloc(count, sizeof(WorldMap *) );
	single = true;
}

WorldMapArray::~WorldMapArray()
{
	unsigned int i;

	for (i = 0; i<MapCount; i++) {
		if (all_maps[i]) {
			delete all_maps[i];
		}
	}
	free( all_maps );
}

unsigned int WorldMapArray::FindAndSetCurrentMap(const ieResRef area)
{
	unsigned int i, idx;

	for (i = CurrentMap; i<MapCount; i++) {
		if (all_maps[i]->GetArea (area, idx) ) {
			CurrentMap = i;
			return i;
		}
	}
	for (i = 0; i<CurrentMap; i++) {
		if (all_maps[i]->GetArea (area, idx) ) {
			CurrentMap = i;
			return i;
		}
	}
	return CurrentMap;
}

void WorldMapArray::SetWorldMap(WorldMap *m, unsigned int index)
{
	if (index<MapCount) {
		all_maps[index]=m;
	}
}

WorldMap *WorldMapArray::NewWorldMap(unsigned int index)
{
	if (all_maps[index]) {
		delete all_maps[index];
	}
	all_maps[index] = new WorldMap();
	return all_maps[index];
}
