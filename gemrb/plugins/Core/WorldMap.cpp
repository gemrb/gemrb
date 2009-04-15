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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * $Id$
 *
 */

#include "../../includes/win32def.h"
#include "WorldMap.h"
#include "Interface.h"
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
	//as long as mapicon comes from a factory, no need to free it here
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
	//invalidating MapIcon, no need to free it, it is from a factory
	MapIcon = NULL;
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



Sprite2D *WMPAreaEntry::GetMapIcon(AnimationFactory *bam)
{
	if (!bam) {
		return NULL;
	}
	if (!MapIcon) {
		int frame = 0;
		if (bam->GetCycleSize(IconSeq)>5) {
			switch (AreaStatus&(WMP_ENTRY_ACCESSIBLE|WMP_ENTRY_VISITED))
			{
			case WMP_ENTRY_ACCESSIBLE: frame = 0; break;
			case WMP_ENTRY_VISITED: frame = 4; break;
			case WMP_ENTRY_ACCESSIBLE|WMP_ENTRY_VISITED: frame = 1; break;
			case 0: frame = 2; break;
			}
		}
		MapIcon = bam->GetFrame((ieWord) frame, (ieByte) IconSeq);
	}
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

WMPAreaEntry *WorldMap::GetNewAreaEntry()
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

WMPAreaEntry* WorldMap::GetArea(const ieResRef AreaName, unsigned int &i)
{
	i=(unsigned int) area_entries.size();
	while (i--) {
		if (!strnicmp(AreaName, area_entries[i]->AreaName,8)) {
			return area_entries[i];
		}
	}
	return NULL;
}

//this is a pathfinding algorithm
//you have to find the optimal path

int WorldMap::CalculateDistances(const ieResRef AreaName, int direction)
{
	if (direction<0 || direction>3) {
		printMessage("WorldMap","", LIGHT_RED);
		printf("CalculateDistances for invalid direction: %s\n", AreaName);
		return -1;
	}

	unsigned int i;
	if (!GetArea(AreaName, i)) {
		printMessage("WorldMap","", LIGHT_RED);
		printf("CalculateDistances for invalid Area: %s\n", AreaName);
		return -1;
	}
	if (Distances) {
		free(Distances);
	}
	if (GotHereFrom) {
		free(GotHereFrom);
	}

	printMessage("WorldMap","", GREEN);
	printf("CalculateDistances for Area: %s\n", AreaName);
	UpdateAreaVisibility(AreaName, direction);

	size_t memsize =sizeof(int) * area_entries.size();
	Distances = (int *) malloc( memsize );
	GotHereFrom = (int *) malloc( memsize );
	memset( Distances, -1, memsize );
	memset( GotHereFrom, -1, memsize );
	Distances[i] = 0; //setting our own distance
	GotHereFrom[i] = -1; //we didn't move

	std::list<int> pending;
	pending.push_back(i);
	while(pending.size()) {
		i=pending.front();
		pending.pop_front();
		WMPAreaEntry* ae=area_entries[i];
		//all directions should be used
		for(int d=0;d<4;d++) {
			int j=ae->AreaLinksIndex[d];
			int k=j+ae->AreaLinksCount[d];
			if ((size_t) k>=area_links.size()) {
				printMessage("WorldMap","The worldmap file is corrupted... and it would crash right now!\n",RED);
				printf("Entry #: %d  Direction: %d\n",i,d);
				break;
			}
			for(;j<k;j++) {
				WMPAreaLink* al = area_links[j];
				WMPAreaEntry* ae2 = area_entries[al->AreaIndex];
				unsigned int mydistance = (unsigned int) Distances[i];

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
	return 0;
}

//returns the index of the area owning this link
unsigned int WorldMap::WhoseLinkAmI(int link_index)
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

WMPAreaLink *WorldMap::GetLink(const ieResRef A, const ieResRef B)
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
WMPAreaLink *WorldMap::GetEncounterLink(const ieResRef AreaName, bool &encounter)
{
	if (!GotHereFrom) {
		return NULL;
	}
	unsigned int i;
	WMPAreaEntry *ae=GetArea( AreaName, i ); //target area
	if (!ae) {
		printf("No such area: %s\n", AreaName);
		return NULL;
	}
	std::list<WMPAreaLink*> walkpath;
	printf("Gathering path information for: %s\n", AreaName);
	while (GotHereFrom[i]!=-1) {
		printf("Adding path to %d\n", i);
		walkpath.push_back(area_links[GotHereFrom[i]]);
		i = WhoseLinkAmI(GotHereFrom[i]);
		if (i==(ieDword) -1) {
			printf("Something has been screwed up here (incorrect path)!\n");
			abort();
		}
	}

	printf("Walkpath size is: %d\n",(int) walkpath.size());
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

int WorldMap::GetDistance(const ieResRef AreaName)
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
	if (direction<0 || direction>3)
		return;
	unsigned int i;
	WMPAreaEntry* ae=GetArea(AreaName,i);
	if (!ae)
		return;
	//we are here, so we visited and it is visible too (i guess)
	printf("Updated Area visibility: %s (visited, and visible)\n", AreaName);

	ae->SetAreaStatus(WMP_ENTRY_VISITED|WMP_ENTRY_VISIBLE, BM_OR);
	i=ae->AreaLinksCount[direction];
	while (i--) {
		WMPAreaLink* al = area_links[ae->AreaLinksIndex[direction]+i];
		WMPAreaEntry* ae2 = area_entries[al->AreaIndex];
		if (ae2->GetAreaStatus()&WMP_ENTRY_ADJACENT) {
			printf("Updated Area visibility: %s (accessible, and visible)\n", ae2->AreaName);
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
	printf("Updated Area visibility: %s\n", ae->AreaName);
	ae->SetAreaStatus(Bits, Op);
}

/****************** WorldMapArray *******************/
WorldMapArray::WorldMapArray(unsigned int count)
{
	CurrentMap = 0;
	MapCount = count;
	all_maps = (WorldMap **) calloc(count, sizeof(WorldMap *) );
}

WorldMapArray::~WorldMapArray()
{
	for (unsigned int i = 0; i<MapCount; i++) {
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
