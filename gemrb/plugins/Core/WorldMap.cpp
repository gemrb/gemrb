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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/WorldMap.cpp,v 1.9 2005/02/19 16:46:50 avenger_teambg Exp $
 *
 */

#include "../../includes/win32def.h"
#include "WorldMap.h"
#include "Interface.h"

extern Interface *core;

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
}

void WorldMap::AddAreaEntry(WMPAreaEntry *ae)
{
	area_entries.push_back(ae);
}

void WorldMap::AddAreaLink(WMPAreaLink *al)
{
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
	if(MapMOS) {
		core->GetVideoDriver()->FreeSprite(MapMOS);
	}
}

WMPAreaEntry* WorldMap::GetArea(const ieResRef AreaName)
{
	int i=area_entries.size();
	while (i--) {
		if(!strnicmp(AreaName, area_entries[i]->AreaName,8)) {
			return area_entries[i];
		}
	}
	return NULL;
}

void WorldMap::UpdateAreaVisibility(const ieResRef AreaName, int direction)
{
	if (direction<0 || direction>3)
		return;
	WMPAreaEntry* ae=GetArea(AreaName);
	if (!ae)
		return;
	int i=ae->AreaLinksCount[direction];
	while (i--) {
		WMPAreaEntry* ae2 = area_entries[ae->AreaLinksIndex[direction]+i];
		if (ae2->AreaStatus&WMP_ENTRY_ADJACENT) {
			ae2->AreaStatus|=WMP_ENTRY_VISIBLE;
		}
	}
}

void WorldMap::SetAreaStatus(const ieResRef AreaName, int Bits, int Op)
{
	WMPAreaEntry* ae=GetArea(AreaName);
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

