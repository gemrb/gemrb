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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/WorldMap.cpp,v 1.7 2004/10/01 19:40:38 avenger_teambg Exp $
 *
 */

#include "../../includes/win32def.h"
#include "WorldMap.h"
#include "Interface.h"

extern Interface *core;

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

void WorldMap::SetAreaStatus(char *AreaName, int Bits, int Op)
{
	WMPAreaEntry* ae=NULL;

	int i=area_entries.size();
	while(i--) {
		if(!strnicmp(AreaName, area_entries[i]->AreaName,8)) {
			ae=area_entries[i];
			break;
		}
	}
	if(!ae)
		return;
	switch(Op) {
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

