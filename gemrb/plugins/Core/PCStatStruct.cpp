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
 * $Id$
 *
 */

#include "../../includes/win32def.h"
#include "../../includes/globals.h" //for abort()
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
	memset( QuickItemSlots, -1, sizeof(QuickItemSlots) );
	memset( QuickItemHeaders, -1, sizeof(QuickItemHeaders) );
	memset( QuickWeaponSlots, -1, sizeof(QuickWeaponSlots) );
	memset( QuickWeaponHeaders, -1, sizeof(QuickWeaponHeaders) );
	memset( PortraitIcons, -1, sizeof(PortraitIcons) );
	memset( PortraitIconString, 0, sizeof(PortraitIconString) );
}

void PCStatsStruct::IncrementChapter()
{
	KillsChapterXP = 0;
	KillsChapterCount = 0;
}

void PCStatsStruct::NotifyKill(ieDword xp, ieStrRef name)
{
	if (BestKilledXP<=xp) {
		BestKilledXP = xp;
		BestKilledName = name;
	}

	KillsTotalXP += xp;
	KillsChapterXP += xp;
	KillsTotalCount ++;
	KillsChapterCount ++;
printf("Killcount increased\n");
}

//init quick weapon/item slots
void PCStatsStruct::InitQuickSlot(unsigned int which, ieWord slot, ieWord headerindex)
{
	switch(which) {
	case ACT_QSLOT1:
		QuickItemSlots[0]=slot;
		QuickItemHeaders[0]=headerindex;
		break;
	case ACT_QSLOT2:
		QuickItemSlots[1]=slot;
		QuickItemHeaders[1]=headerindex;
		break;
	case ACT_QSLOT3:
		QuickItemSlots[2]=slot;
		QuickItemHeaders[2]=headerindex;
		break;
	case ACT_QSLOT4:
		QuickItemSlots[3]=slot;
		QuickItemHeaders[3]=headerindex;
		break;
	case ACT_QSLOT5:
		QuickItemSlots[4]=slot;
		QuickItemHeaders[4]=headerindex;
		break;
	case ACT_WEAPON1:
		QuickWeaponSlots[0]=slot;
		QuickWeaponHeaders[0]=0;
		QuickWeaponSlots[4]=slot+1;
		QuickWeaponHeaders[4]=0;
		break;
	case ACT_WEAPON2:
		QuickWeaponSlots[1]=slot;
		QuickWeaponHeaders[1]=0;
		QuickWeaponSlots[5]=slot+1;
		QuickWeaponHeaders[5]=0;
		break;
	case ACT_WEAPON3:
		QuickWeaponSlots[2]=slot;
		QuickWeaponHeaders[2]=0;
		QuickWeaponSlots[6]=slot+1;
		QuickWeaponHeaders[6]=0;
		break;
	case ACT_WEAPON4:
		QuickWeaponSlots[3]=slot;
		QuickWeaponHeaders[3]=0;
		QuickWeaponSlots[7]=slot+1;
		QuickWeaponHeaders[7]=0;
		break;
	}
}

void PCStatsStruct::SetSlotIndex(unsigned int which, ieWord headerindex)
{
	//this is not correct, not the slot, but a separate headerindex should be here
	switch(which) {
	case ACT_QSLOT1: QuickItemHeaders[0]=headerindex; return;
	case ACT_QSLOT2: QuickItemHeaders[1]=headerindex; return;
	case ACT_QSLOT3: QuickItemHeaders[2]=headerindex; return;
	case ACT_QSLOT4: QuickItemHeaders[3]=headerindex; return;
	case ACT_QSLOT5: QuickItemHeaders[4]=headerindex; return;
	}
	///it shouldn't reach this point
	abort();
}

void PCStatsStruct::GetSlotAndIndex(unsigned int which, ieWord &slot, ieWord &headerindex)
{
	int idx;

	switch(which) {
	case ACT_QSLOT1: idx = 0; break;
	case ACT_QSLOT2: idx = 1; break;
	case ACT_QSLOT3: idx = 2; break;
	case ACT_QSLOT4: idx = 3; break;
	case ACT_QSLOT5: idx = 4; break;
	default: abort();
	}
	slot=QuickItemSlots[idx];
	headerindex=QuickItemHeaders[idx];
}
//
