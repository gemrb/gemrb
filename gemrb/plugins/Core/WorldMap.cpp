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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/WorldMap.cpp,v 1.14 2005/03/15 14:31:01 avenger_teambg Exp $
 *
 */

#include "../../includes/win32def.h"
#include "WorldMap.h"
#include "Interface.h"
#include <list>

WMPAreaEntry::WMPAreaEntry()
{
	MapIcon=NULL;
}
WMPAreaEntry::~WMPAreaEntry()
{
	if (MapIcon) {
		core->GetVideoDriver()->FreeSprite(MapIcon);
	}
}

WorldMap::WorldMap(void)
{
	MapMOS = NULL;
	Distances = NULL;
	GotHereFrom = NULL;
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

//Win32 friendly interface
void WorldMap::SetAreaEntry(unsigned int x, WMPAreaEntry *areaentry)
{
	WMPAreaEntry *ae =new WMPAreaEntry();

	//copying the struct part of the class
	memcpy( ae->AreaName, areaentry->AreaName, 
		(char *) (&ae->MapIcon)-(char *) (&ae->AreaName[0]) );
	//areaentry will be freed, we have to steal the sprite!
	ae->MapIcon=areaentry->MapIcon;
	areaentry->MapIcon=NULL;

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
}

WMPAreaEntry* WorldMap::GetArea(const ieResRef AreaName, int &i)
{
	i=area_entries.size();
	while (i--) {
		if (!strnicmp(AreaName, area_entries[i]->AreaName,8)) {
			return area_entries[i];
		}
	}
	return NULL;
}

//this is a pathfinding algorithm
//you have to find the optimal path
/*
void WorldMap::CalculateDistance(int i, int direction)
{
	WMPAreaEntry* ae=area_entries[i];
	int j=ae->AreaLinksIndex[direction];
	int k=j+ae->AreaLinksCount[direction];
	for(;j<k;j++) {
		WMPAreaLink* al = area_links[j];
		WMPAreaEntry* ae2 = area_entries[al->AreaIndex];
		int mydistance = Distances[i];
		if (
			( (ae->AreaStatus & WMP_ENTRY_PASSABLE) == WMP_ENTRY_PASSABLE) &&
			( (ae2->AreaStatus & WMP_ENTRY_WALKABLE) == WMP_ENTRY_WALKABLE)
			) {
			mydistance += al->DistanceScale * al->Flags;
			if (Distances[al->AreaIndex] !=-1) {
				Distances[al->AreaIndex] = mydistance;
				GotHereFrom[al->AreaIndex] = j;
			}
		}
	}
}
*/
int WorldMap::CalculateDistances(const ieResRef AreaName, int direction)
{
	if (direction<0 || direction>3)
		return -1;
	int i;
	if (!GetArea(AreaName, i)) {
		return -1;
	}
	if (Distances) {
		free(Distances);
	}
	if (GotHereFrom) {
		free(GotHereFrom);
	}
	UpdateAreaVisibility(AreaName, direction);

	int memsize =sizeof(int) * area_entries.size();
	Distances = (int *) malloc( memsize );
	GotHereFrom = (int *) malloc( memsize );
	memset( Distances, -1, memsize );
	memset( GotHereFrom, -1, memsize );
	Distances[i] = 0; //setting our own distance
	GotHereFrom[i] = -1; //we didn't move

	std::list<int> pending;
	pending.push_back(i);
	while(pending.size())
	{
		i=pending.front();
		pending.pop_front();
		WMPAreaEntry* ae=area_entries[i];
		int j=ae->AreaLinksIndex[direction];
		int k=j+ae->AreaLinksCount[direction];
		for(;j<k;j++) {
			WMPAreaLink* al = area_links[j];
			WMPAreaEntry* ae2 = area_entries[al->AreaIndex];
			unsigned int mydistance = (unsigned int) Distances[i];
			if ( ( (ae->AreaStatus & WMP_ENTRY_PASSABLE) == WMP_ENTRY_PASSABLE) &&
			( (ae2->AreaStatus & WMP_ENTRY_WALKABLE) == WMP_ENTRY_WALKABLE)
			) {
				mydistance += al->DistanceScale * al->Flags;
				//nonexisting distance is the biggest!
				if ((unsigned) Distances[al->AreaIndex] > mydistance) {
					Distances[al->AreaIndex] = mydistance;
					GotHereFrom[al->AreaIndex] = j;
					pending.push_back(al->AreaIndex);
				}
			}
		}
	}
	return 0;
}

//returns the index of the area owning this link
int WorldMap::WhoseLinkAmI(int link_index)
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
	return -1;
}

//call this function to find out which area we fall into
//not necessarily the target area
//if it isn't the same, then a random encounter happened!
WMPAreaLink *WorldMap::GetEncounterLink(const ieResRef AreaName, bool &encounter)
{
	if (!GotHereFrom) {
		return NULL;
	}
	int i;
	WMPAreaEntry *ae=GetArea( AreaName, i ); //target area
	if (!ae) {
		return NULL;
	}
	std::list<WMPAreaLink*> walkpath;
	printf("Gathering path information\n");
	while (GotHereFrom[i]!=-1) {
		printf("Adding path to %d\n", i);
		walkpath.push_back(area_links[GotHereFrom[i]]);
		i = WhoseLinkAmI(GotHereFrom[i]);
		if (i==-1) {
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
	int i;
	if (GetArea( AreaName, i )) {
			return Distances[i];
	}
	return -1;
}

void WorldMap::UpdateAreaVisibility(const ieResRef AreaName, int direction)
{
	if (direction<0 || direction>3)
		return;
	int i;
	WMPAreaEntry* ae=GetArea(AreaName,i);
	if (!ae)
		return;
	ae->AreaStatus|=WMP_ENTRY_VISITED|WMP_ENTRY_VISIBLE; //we are here, so we visited and it is visible too (i guess)
	i=ae->AreaLinksCount[direction];
	while (i--) {
		WMPAreaLink* al = area_links[ae->AreaLinksIndex[direction]+i];
		WMPAreaEntry* ae2 = area_entries[al->AreaIndex];
		if (ae2->AreaStatus&WMP_ENTRY_ADJACENT) {
			ae2->AreaStatus|=WMP_ENTRY_VISIBLE;
		}
	}
}

void WorldMap::SetAreaStatus(const ieResRef AreaName, int Bits, int Op)
{
	int i;
	WMPAreaEntry* ae=GetArea(AreaName,i);
	if (!ae)
		return;
	switch (Op) {
		case BM_SET:
			ae->AreaStatus=Bits;
			break;
		case BM_AND:
			ae->AreaStatus&=Bits;
			break;
		case BM_OR:
			ae->AreaStatus|=Bits;
			break;
		case BM_XOR:
			ae->AreaStatus^=Bits;
			break;
		case BM_NAND:
			ae->AreaStatus&=~Bits;
			break;
	}
}
