/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/PCStatStruct.cpp,v 1.1 2006/04/16 23:57:02 avenger_teambg Exp $
 *
 */

#include "../../includes/win32def.h"

#include "PCStatStruct.h"
#include <string.h>

PCStatsStruct::PCStatsStruct()
{
	BestKilledName = 0xffffffff;
	BestKilledXP = 0;
	AwayTime = 0;
	JoinDate = 0;
	unknown10 = 0;
	KillsChapterXP = 0;
	KillsChapterCount = 0;
	KillsTotalXP = 0;
	KillsTotalCount = 0;
	memset( FavouriteSpells, 0, sizeof(FavouriteSpells) );
	memset( FavouriteSpellsCount, 0, sizeof(FavouriteSpellsCount) );
	memset( FavouriteWeapons, 0, sizeof(FavouriteWeapons) );
	memset( FavouriteWeaponsCount, 0, sizeof(FavouriteWeaponsCount) );
	SoundSet[0]=0;
	SoundFolder[0]=0;
	QSlots[0]=0xff;
	memset( QuickSpells, 0, sizeof(QuickSpells) );
	memset( QuickSpellClass, 0xff, sizeof(QuickSpellClass) );
	memset( QuickItemSlots, 0, sizeof(QuickItemSlots) );
	memset( QuickWeaponSlots, 0, sizeof(QuickWeaponSlots) );
	memset( PortraitIcons, -1, sizeof(PortraitIcons) );
}

void PCStatsStruct::IncrementChapter()
{
	KillsChapterXP = 0;
	KillsChapterCount = 0;
}

//init quick weapon/item slots
void PCStatsStruct::InitQuickSlot(unsigned int which, unsigned int state)
{
	switch(which) {
	case ACT_QSLOT1: QuickItemSlots[0]=state; break;
	case ACT_QSLOT2: QuickItemSlots[1]=state; break;
	case ACT_QSLOT3: QuickItemSlots[2]=state; break;
	case ACT_QSLOT4: QuickItemSlots[3]=state; break;
	case ACT_QSLOT5: QuickItemSlots[4]=state; break;
	case ACT_WEAPON1: QuickWeaponSlots[0]=state; break;
	case ACT_WEAPON2: QuickWeaponSlots[1]=state; break;
	case ACT_WEAPON3: QuickWeaponSlots[2]=state; break;
	case ACT_WEAPON4: QuickWeaponSlots[3]=state; break;
	}
}
