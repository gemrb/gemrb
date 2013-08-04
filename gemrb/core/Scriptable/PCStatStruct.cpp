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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

#include "Scriptable/PCStatStruct.h"

#include "win32def.h"

#include <cstring>

namespace GemRB {

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
	memset( ExtraSettings, 0, sizeof(ExtraSettings) );
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
	memset( PreviousPortraitIcons, -1, sizeof(PreviousPortraitIcons) );
	memset( PortraitIconString, 0, sizeof(PortraitIconString) );
	LastLeft = 0;
	LastJoined = 0;
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
}

//init quick weapon/item slots
void PCStatsStruct::SetQuickItemSlot(int idx, int slot, int headerindex)
{
	if (slot>=0) {
		QuickItemSlots[idx]=(ieWord) slot;
	}
	QuickItemHeaders[idx]=(ieWord) headerindex;
}

void PCStatsStruct::InitQuickSlot(unsigned int which, int slot, int headerindex)
{
	if (!which) {
		int i;

		for(i=0;i<MAX_QUICKITEMSLOT;i++) {
			if (slot==QuickItemSlots[i]) {
				QuickItemHeaders[i]=headerindex;
				return;
			}
		}

		for(i=0;i<MAX_QUICKWEAPONSLOT;i++) {
			if (slot==QuickWeaponSlots[i]) {
				QuickWeaponHeaders[i]=headerindex;
				return;
			}
		}
		return;
	}

	//precalculate field values. Empty slots will get their ability header
	//initialized to the invalid value of 0xffff to stay binary compatible
	//with original
	int slot2, header;

	if (slot==0xffff) {
		slot2 = 0xffff;
		header = 0xffff;
	}
	else {
		slot2 = slot+1;
		header = 0;
	}
	switch(which) {
	case ACT_QSLOT1: SetQuickItemSlot(0,slot,headerindex); break;
	case ACT_QSLOT2: SetQuickItemSlot(1,slot,headerindex); break;
	case ACT_QSLOT3: SetQuickItemSlot(2,slot,headerindex); break;
	case ACT_QSLOT4: SetQuickItemSlot(3,slot,headerindex); break;
	case ACT_QSLOT5: SetQuickItemSlot(4,slot,headerindex); break;
	case ACT_IWDQITEM:
	case ACT_IWDQITEM+1:
	case ACT_IWDQITEM+2:
	case ACT_IWDQITEM+3:
	case ACT_IWDQITEM+4:
	case ACT_IWDQITEM+5: // crashy from here on until we do do/use 9 quickslots
	case ACT_IWDQITEM+6:
	case ACT_IWDQITEM+7:
	case ACT_IWDQITEM+8:
	case ACT_IWDQITEM+9: SetQuickItemSlot(which - ACT_IWDQITEM, slot, headerindex); break;
	case ACT_WEAPON1:
		QuickWeaponSlots[0]=slot;
		QuickWeaponHeaders[0]=header;
		QuickWeaponSlots[4]=slot2;
		QuickWeaponHeaders[4]=header;
		break;
	case ACT_WEAPON2:
		QuickWeaponSlots[1]=slot;
		QuickWeaponHeaders[1]=header;
		QuickWeaponSlots[5]=slot2;
		QuickWeaponHeaders[5]=header;
		break;
	case ACT_WEAPON3:
		QuickWeaponSlots[2]=slot;
		QuickWeaponHeaders[2]=header;
		QuickWeaponSlots[6]=slot2;
		QuickWeaponHeaders[6]=header;
		break;
	case ACT_WEAPON4:
		QuickWeaponSlots[3]=slot;
		QuickWeaponHeaders[3]=header;
		QuickWeaponSlots[7]=slot2;
		QuickWeaponHeaders[7]=header;
		break;
	default:
		Log(DEBUG, "PCSS", "InitQuickSlot: unknown which/slot %d/%d", which, slot);
	}
}

//returns both the inventory slot and the header index associated to a quickslot
void PCStatsStruct::GetSlotAndIndex(unsigned int which, ieWord &slot, ieWord &headerindex)
{
	int idx;

	switch(which) {
	case ACT_QSLOT1: idx = 0; break;
	case ACT_QSLOT2: idx = 1; break;
	case ACT_QSLOT3: idx = 2; break;
	case ACT_QSLOT4: idx = 3; break;
	case ACT_QSLOT5: idx = 4; break;
	case ACT_IWDQITEM:
	case ACT_IWDQITEM+1:
	case ACT_IWDQITEM+2:
	case ACT_IWDQITEM+3:
	case ACT_IWDQITEM+4:
	case ACT_IWDQITEM+5: // crashy from here on until we do do/use 9 quickslots
	case ACT_IWDQITEM+6:
	case ACT_IWDQITEM+7:
	case ACT_IWDQITEM+8:
	case ACT_IWDQITEM+9: idx = which - ACT_IWDQITEM; break;
	default: error("Core", "Unknown Quickslot accessed '%d'.\n", which);
	}
	slot=QuickItemSlots[idx];
	headerindex=QuickItemHeaders[idx];
}

//return the item extended header assigned to an inventory slot (the ability to use)
//only quickslots have this assignment, equipment items got all abilities available
int PCStatsStruct::GetHeaderForSlot(int slot)
{
	int i;

	for(i=0;i<MAX_QUICKITEMSLOT;i++) {
		if(QuickItemSlots[i]==slot) return (ieWordSigned) QuickItemHeaders[i];
	}

	for(i=0;i<MAX_QUICKWEAPONSLOT;i++) {
		if(QuickWeaponSlots[i]==slot) return (ieWordSigned) QuickWeaponHeaders[i];
	}
	return -1;
}

//register favourite weapon or spell
//method:
//there are MAX_FAVOURITES slots, each with a resref and an usage count
//the currently registered resref will always be stored somewhere, the question is just where
//if it was an old favourite, just increase the usage count
//if it is the new favourite candidate (last slot) then just increase the usage count
//but also swap it with a previous slot if its usage count is now better, so the last slot is always the weakest
//finally if it was not found anywhere, register it as the new candidate with 1 usage

void PCStatsStruct::RegisterFavourite(ieResRef fav, int what)
{
	ieResRef *respoi;
	ieWord *cntpoi;

	switch (what) {
		case FAV_SPELL:
			respoi = FavouriteSpells;
			cntpoi = FavouriteSpellsCount;
			break;
		case FAV_WEAPON:
			respoi = FavouriteWeapons;
			cntpoi = FavouriteWeaponsCount;
			break;
		default:
			print("Illegal RegisterFavourite call...");
			abort();
			return;
	}
	//least favourite candidate position and count
	int minpos = 0;
	int mincnt = cntpoi[0];
	int pos = 0;
	for (pos = 0; pos<MAX_FAVOURITES-1; pos++) {
		if (!strnicmp(fav, respoi[pos], 8) ) {
			//found an old favourite, just increase its usage count and done
			if (cntpoi[pos]<0xffff) {
				cntpoi[pos]++;
			}
			return;
		}
		if (pos) {
			//collect least favourite for possible swapping
			if (cntpoi[pos]<mincnt) {
				minpos = pos;
				mincnt = cntpoi[pos];
			}
		}
	}

	//pos is always MAX_FAVOURITES-1 here
	if (strnicmp(fav, respoi[pos], 8) ) {
		//new favourite candidate, scrapping the old one
		cntpoi[pos] = 1;
		strnuprcpy(respoi[pos], fav, 8);
		return;
	}
	//increase the favourite candidate
	cntpoi[pos]++;
	if (cntpoi[pos]>mincnt) {
		//if it is now exceeding an old favourite, swap them

		//move the old resref to the last position
		memcpy(respoi[pos], respoi[minpos], sizeof(ieResRef) );
		//store the new resref into the new position
		strnuprcpy(respoi[minpos], fav, 8);
		//store the new count to the new position
		cntpoi[minpos] = cntpoi[pos];
		//store the old count to the last position
		cntpoi[pos] = mincnt;
	}
}

}
