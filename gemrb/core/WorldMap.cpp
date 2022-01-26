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

#include "Game.h"
#include "Interface.h"
#include "TableMgr.h"
#include "Video/Video.h"
#include "RNG.h"

#include <list>
#include <utility>


namespace GemRB {

void WMPAreaEntry::SetAreaStatus(ieDword arg, BitOp op)
{
	SetBits(AreaStatus, arg, op);
	MapIcon = nullptr;
}

String WMPAreaEntry::GetCaption()
{
	if (StrCaption.empty()) {
		StrCaption = core->GetString(LocCaptionName);
	}
	return StrCaption;
}

String WMPAreaEntry::GetTooltip()
{
	if (StrTooltip.empty()) {
		StrTooltip = core->GetString(LocTooltipName);
	}
	return StrTooltip;
}

Holder<Sprite2D> WMPAreaEntry::GetMapIcon(const AnimationFactory *bam)
{
	if (!bam || IconSeq == (ieDword) -1) {
		return NULL;
	}
	if (!MapIcon) {
		int frame = 0;
		switch (AreaStatus&(WMP_ENTRY_ACCESSIBLE|WMP_ENTRY_VISITED))
		{
			case WMP_ENTRY_ACCESSIBLE: frame = 0; break;
			case WMP_ENTRY_VISITED: frame = 4; break;
			case WMP_ENTRY_ACCESSIBLE|WMP_ENTRY_VISITED: frame = 1; break;
			case 0: frame = 2; break;
		}

		// iwd1, bg1 and pst all have this format
		if (bam->GetCycleSize(IconSeq)<5) {
			SingleFrame = true;
			frame = 0;
		}
		MapIcon = bam->GetFrame((ieWord) frame, (ieByte) IconSeq);
		if (!MapIcon) {
			Log(ERROR, "WMPAreaEntry", "GetMapIcon failed for frame %d, seq %d", frame, IconSeq);
			return NULL;
		}
	}
	return MapIcon;
}

ieDword WMPAreaEntry::GetAreaStatus() const
{
	ieDword tmp = AreaStatus;
	if (core->HasFeature(GF_KNOW_WORLD) ) {
		tmp |=WMP_ENTRY_VISITED;
	}
	return tmp;
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

void WorldMap::SetAreaEntry(unsigned int x, WMPAreaEntry *ae)
{
	//if index is too large, we break
	if (x>area_entries.size()) {
		error("WorldMap", "Trying to set invalid entry (%d/%d)\n", x, (int)area_entries.size());
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

void WorldMap::InsertAreaLink(unsigned int areaidx, unsigned int dir, const WMPAreaLink *arealink)
{
	WMPAreaLink *al = new WMPAreaLink(*arealink);
	unsigned int idx = area_entries[areaidx]->AreaLinksIndex[dir];
	area_links.insert(area_links.begin()+idx,al);

	WMPAreaEntry *ae;
	size_t max = area_entries.size();
	for (unsigned int pos = 0; pos < max; pos++) {
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
}

void WorldMap::SetAreaLink(unsigned int x, const WMPAreaLink *arealink)
{
	WMPAreaLink *al = new WMPAreaLink(*arealink);

	//if index is too large, we break
	if (x>area_links.size()) {
		error("WorldMap", "Trying to set invalid link (%d/%d)", x, (int)area_links.size());
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
	for (auto entry : area_entries) {
		delete entry;
	}
	for (auto link : area_links) {
		delete link;
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

void WorldMap::SetMapMOS(Holder<Sprite2D> newmos)
{
	MapMOS = std::move(newmos);
}

WMPAreaEntry* WorldMap::GetArea(const ResRef& areaName, unsigned int &i) const
{
	i=(unsigned int) area_entries.size();
	while (i--) {
		if (areaName == area_entries[i]->AreaName) {
			return area_entries[i];
		}
	}
	// try also with the original name (needed for centering on Candlekeep)
	i = (unsigned int) area_entries.size();
	while (i--) {
		if (areaName == area_entries[i]->AreaResRef) {
			return area_entries[i];
		}
	}
	return NULL;
}

//Find Worldmap location by nearest area with a smaller number
//Counting backwards, stop at 1000 boundaries.
//It is not possible to simply round to 1000, because there are 
//WMP entries like AR8001, and we need to find the best match
WMPAreaEntry* WorldMap::FindNearestEntry(const ResRef& areaName, unsigned int &i) const
{
	int value = 0;
	ResRef tmp;

	sscanf(areaName.CString() + 2, "%4d", &value);
	do {
		tmp.SNPrintF("%.2s%04d", areaName.CString(), value);
		WMPAreaEntry* ret = GetArea(tmp, i);
		if (ret) {
			return ret;
		}
		if (value%1000 == 0) break;
		value--;
	}
	while (true); //value%1000 should protect us from infinite loops
	i = -1;
	return NULL;
}

//this is a pathfinding algorithm
//you have to find the optimal path
int WorldMap::CalculateDistances(const ResRef& areaName, int direction)
{
	//first, update reachable/visible areas by worlde.2da if exists
	UpdateReachableAreas();
	UpdateAreaVisibility(areaName, direction);
	if (direction==-1) {
		return 0;
	}

	if (direction<0 || direction>3) {
		Log(ERROR, "WorldMap", "CalculateDistances for invalid direction: %s", areaName.CString());
		return -1;
	}

	unsigned int i;
	if (!GetArea(areaName, i)) {
		Log(ERROR, "WorldMap", "CalculateDistances for invalid Area: %s", areaName.CString());
		return -1;
	}
	if (Distances) {
		free(Distances);
	}
	if (GotHereFrom) {
		free(GotHereFrom);
	}

	Log(MESSAGE, "WorldMap", "CalculateDistances for Area: %s", areaName.CString());

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
	while(!pending.empty()) {
		i=pending.front();
		pending.pop_front();
		const WMPAreaEntry* ae = area_entries[i];
		memset( seen_entry, -1, memsize );
		//all directions should be used
		for(int d=0;d<4;d++) {
			int j=ae->AreaLinksIndex[d];
			int k=j+ae->AreaLinksCount[d];
			if ((size_t) k>area_links.size()) {
				Log(ERROR, "WorldMap", "The worldmap file is corrupted... and it would crash right now! Entry #: %d Direction: %d",
					i, d);
				break;
			}
			for(;j<k;j++) {
				const WMPAreaLink* al = area_links[j];
				const WMPAreaEntry* ae2 = area_entries[al->AreaIndex];
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
	unsigned int cnt = GetEntryCount();
	for (unsigned int i = 0; i < cnt; i++) {
		const WMPAreaEntry *ae = area_entries[i];
		for (int direction=0;direction<4;direction++)
		{
			int j=ae->AreaLinksIndex[direction];
			if (link_index < j) continue;

			j += ae->AreaLinksCount[direction];
			if (link_index < j) {
				return i;
			}
		}
	}
	return (ieDword) -1;
}

WMPAreaLink *WorldMap::GetLink(const ResRef& A, const ResRef& B) const
{
	unsigned int i;
	const WMPAreaEntry *ae = GetArea(A, i);
	if (!ae) {
		return NULL;
	}

	//looking for destination area, returning the first link found
	for (i = 0; i < 4; i++) {
		unsigned int j = ae->AreaLinksCount[i];
		unsigned int k = ae->AreaLinksIndex[i];
		while(j--) {
			WMPAreaLink *al = area_links[k++];
			const WMPAreaEntry *ae2 = area_entries[al->AreaIndex];
			//or arearesref?
			if (ae2->AreaName == B) {
				return al;
			}
		}
	}
	return NULL;
}

//call this function to find out which area we fall into
//not necessarily the target area
//if it isn't the same, then a random encounter happened!
WMPAreaLink *WorldMap::GetEncounterLink(const ResRef& areaName, bool &encounter) const
{
	if (!GotHereFrom) {
		return NULL;
	}
	unsigned int i;
	const WMPAreaEntry *ae = GetArea(areaName, i); //target area
	if (!ae) {
		Log(ERROR, "WorldMap", "No such area: %s", areaName.CString());
		return NULL;
	}
	std::list<WMPAreaLink*> walkpath;
	Log(DEBUG, "WorldMap", "Gathering path information for: %s", areaName.CString());
	while (GotHereFrom[i]!=-1) {
		Log(DEBUG, "WorldMap", "Adding path to %d", i);
		walkpath.push_back(area_links[GotHereFrom[i]]);
		i = WhoseLinkAmI(GotHereFrom[i]);
		if (i==(ieDword) -1) {
			error("WorldMap", "Something has been screwed up here (incorrect path)!\n");
		}
	}

	Log(DEBUG, "WorldMap", "Walkpath size is: %d", (int) walkpath.size());
	if (walkpath.empty()) {
		return NULL;
	}
	std::list<WMPAreaLink*>::reverse_iterator p=walkpath.rbegin();
	WMPAreaLink *lastpath;
	encounter=false;
	do {
		lastpath = *p;
		if (lastpath->EncounterChance > (unsigned int) RAND(0, 99)) {
			encounter=true;
			break;
		}
		++p;
	}
	while(p!=walkpath.rend() );
	return lastpath;
}

//adds a temporary AreaEntry to the world map
//this entry has two links for each direction, leading to the two areas
//we were travelling between when using the supplied link
void WorldMap::SetEncounterArea(const ResRef& area, const WMPAreaLink *link) {
	unsigned int i;
	if (GetArea(area, i)) {
		return;
	}

	//determine the area the link came from
	unsigned int j, cnt = GetLinkCount();
	for (j = 0; j < cnt; ++j) {
		if (link == area_links[j]) {
			break;
		}
	}

	i = WhoseLinkAmI(j);
	if (i == (unsigned int) -1) {
		Log(ERROR, "WorldMap", "Could not add encounter area");
		return;
	}

	WMPAreaEntry *ae = new WMPAreaEntry();
	ae->SetAreaStatus(WMP_ENTRY_VISIBLE|WMP_ENTRY_ACCESSIBLE|WMP_ENTRY_VISITED, BitOp::SET);
	ae->AreaName = area;
	ae->AreaResRef = area;
	ae->LocCaptionName = -1;
	ae->LocTooltipName = -1;
	ae->IconSeq = -1;
	ae->LoadScreenResRef.Reset();

	const WMPAreaEntry *src = area_entries[i];
	const WMPAreaEntry *dest = area_entries[link->AreaIndex];
	ae->pos.x = src->pos.x + (dest->pos.x - src->pos.x) / 2;
	ae->pos.y = src->pos.y + (dest->pos.y - src->pos.y) / 2;

	//setup the area links
	WMPAreaLink *ldest = new WMPAreaLink(*link);
	ldest->DistanceScale /= 2;
	ldest->EncounterChance = 0;

	link = GetLink(dest->AreaName, src->AreaName);
	if (!link) {
		Log(ERROR, "WorldMap", "Could not find link from %s to %s",
			dest->AreaName.CString(), src->AreaName.CString());
		delete ae;
		delete ldest;
		return;
	}

	WMPAreaLink *lsrc = new WMPAreaLink(*link);
	lsrc->DistanceScale /= 2;
	lsrc->EncounterChance = 0;

	size_t idx = area_links.size();
	AddAreaLink(ldest);
	AddAreaLink(lsrc);

	for (i = 0; i < 4; ++i) {
		ae->AreaLinksCount[i] = 2;
		ae->AreaLinksIndex[i] = static_cast<ieDword>(idx);
	}
	
	encounterArea = area_entries.size();
	AddAreaEntry(ae);
}

void WorldMap::ClearEncounterArea()
{
	if (encounterArea >= area_entries.size()) {
		return;
	}

	WMPAreaEntry *ea = area_entries[encounterArea];
	area_entries.erase(area_entries.begin() + encounterArea);

	WMPAreaLink *l = area_links[ea->AreaLinksIndex[0]];
	delete l;
	l = area_links[ea->AreaLinksIndex[0] + 1];
	delete l;

	//NOTE: if anything else added links after us we'd have to globally
	//update all link indices, but since ambush areas do not allow
	//saving/loading we should be okay with this
	area_links.erase(area_links.begin() + ea->AreaLinksIndex[0],
		area_links.begin() + ea->AreaLinksIndex[0] + ea->AreaLinksCount[0]);

	delete ea;
	encounterArea = -1;
}

int WorldMap::GetDistance(const ResRef& areaName) const
{
	if (!Distances) {
		return -1;
	}
	unsigned int i;
	if (GetArea(areaName, i)) {
		return Distances[i];
	}
	return -1;
}

void WorldMap::UpdateAreaVisibility(const ResRef& areaName, int direction)
{
	unsigned int i;

	WMPAreaEntry* ae = GetArea(areaName, i);
	if (!ae)
		return;
	//we are here, so we visited and it is visible too (i guess)
	Log(DEBUG, "WorldMap", "Updated Area visibility: %s (visited, accessible and visible)", areaName.CString());

	ae->SetAreaStatus(WMP_ENTRY_VISITED|WMP_ENTRY_VISIBLE|WMP_ENTRY_ACCESSIBLE, BitOp::OR);
	if (direction<0 || direction>3)
		return;
	i=ae->AreaLinksCount[direction];
	while (i--) {
		const WMPAreaLink* al = area_links[ae->AreaLinksIndex[direction] + i];
		WMPAreaEntry* ae2 = area_entries[al->AreaIndex];
		if (ae2->GetAreaStatus()&WMP_ENTRY_ADJACENT) {
			Log(DEBUG, "WorldMap", "Updated Area visibility: %s (accessible and visible)", ae2->AreaName.CString());
			ae2->SetAreaStatus(WMP_ENTRY_VISIBLE|WMP_ENTRY_ACCESSIBLE, BitOp::OR);
		}
	}
}

void WorldMap::SetAreaStatus(const ResRef& areaName, int Bits, BitOp Op) const
{
	unsigned int i;
	WMPAreaEntry* ae = GetArea(areaName, i);
	if (!ae)
		return;
	ae->SetAreaStatus(Bits, Op);
}

void WorldMap::UpdateReachableAreas() const
{
	AutoTable tab = gamedata->LoadTable("worlde", true);
	if (!tab) {
		return;
	}
	const Game *game = core->GetGame();
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
			SetAreaStatus(areaname, WMP_ENTRY_VISIBLE | WMP_ENTRY_ADJACENT | WMP_ENTRY_ACCESSIBLE, BitOp::OR);
		}
	}
}

/****************** WorldMapArray *******************/
WorldMapArray::WorldMapArray(size_t count)
{
	maps.resize(count);
}

size_t WorldMapArray::FindAndSetCurrentMap(const ResRef& area)
{
	unsigned int idx;
	for (size_t i = 0; i < maps.size(); ++i) {
		if (maps[i].GetArea (area, idx) ) {
			CurrentMap = i;
			return i;
		}
	}
	return CurrentMap;
}

WorldMap *WorldMapArray::NewWorldMap(size_t index)
{
	return &maps[index];
}

}
