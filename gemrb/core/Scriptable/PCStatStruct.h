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

#ifndef PCSTATSTRUCT_H
#define PCSTATSTRUCT_H

#include "exports.h"
#include "ie_types.h"

#include <list>

namespace GemRB {

#define MAX_QUICKITEMSLOT   5   //pst has 5
#define MAX_QUICKWEAPONSLOT 8   //iwd2 has 4x2
#define MAX_QSLOTS          9   //iwd2 has 9
#define MAX_PORTRAIT_ICONS 12
#define MAX_FAVOURITES     4
#define SOUNDFOLDERSIZE    32+1
#define MAX_INTERACT       24

//action buttons
//the order of these buttons are based on opcode #144 
#define ACT_NONE 100 //iwd2's maximum is 99
#define ACT_STEALTH 0
#define ACT_THIEVING 1
#define ACT_CAST 2
#define ACT_QSPELL1 3
#define ACT_QSPELL2 4
#define ACT_QSPELL3 5
#define ACT_TURN 6
#define ACT_TALK 7
#define ACT_USE 8
#define ACT_QSLOT1 9
#define ACT_QSLOT4 10 //this seems to be intentionally so
#define ACT_QSLOT2 11
#define ACT_QSLOT3 12
#define ACT_QSLOT5 31 //this is intentional too
#define ACT_INNATE 13
#define ACT_DEFEND 14 //these are gemrb specific
#define ACT_ATTACK 15 
#define ACT_WEAPON1 16
#define ACT_WEAPON2 17
#define ACT_WEAPON3 18
#define ACT_WEAPON4 19
#define ACT_BARDSONG 20
#define ACT_STOP 21
#define ACT_SEARCH 22
#define ACT_SHAPE 23
#define ACT_TAMING 24
#define ACT_SKILLS 25
#define ACT_WILDERNESS 26
//gui navigation (scrolling button rows left or right)
#define ACT_LEFT 32
#define ACT_RIGHT 33
#define ACT_BARD   40
#define ACT_CLERIC   41
#define ACT_DRUID   42
#define ACT_PALADIN   43
#define ACT_RANGER   44
#define ACT_SORCERER   45
#define ACT_WIZARD   46
#define ACT_DOMAIN   47
#define ACT_WILDSHAPE 48

#define ACT_IWDQSPELL 50
#define ACT_IWDQITEM  60
#define ACT_IWDQSPEC  70
#define ACT_IWDQSONG  80

#define MAX_ACT_COUNT 100

#define ES_COUNT 16  //number of iwd2 persistent feat preset values (original iwd2 had only 5)

#define GUIBT_COUNT (MAX_QSLOTS + 3)

#define FAV_SPELL  0
#define FAV_WEAPON 1

class GEM_EXPORT PCStatsStruct {
public:
	ieStrRef  BestKilledName;
	ieDword   BestKilledXP;
	ieDword   AwayTime;
	ieDword   JoinDate;
	ieDword   unknown10;
	ieDword   KillsChapterXP;
	ieDword   KillsChapterCount;
	ieDword   KillsTotalXP;
	ieDword   KillsTotalCount;
	ieResRef  FavouriteSpells[MAX_FAVOURITES];
	ieWord    FavouriteSpellsCount[MAX_FAVOURITES];
	ieResRef  FavouriteWeapons[MAX_FAVOURITES];
	ieWord    FavouriteWeaponsCount[MAX_FAVOURITES];
	ieResRef  SoundSet;
	char      SoundFolder[SOUNDFOLDERSIZE];
	ieDword   ExtraSettings[ES_COUNT];     //iwd2 - expertise, hamstring, arterial strike, etc
	ieResRef  QuickSpells[MAX_QSLOTS];     //iwd2 uses 9, others use only 3
	ieWord    QuickWeaponSlots[MAX_QUICKWEAPONSLOT]; //iwd2 uses 8, others use only 4
	ieWord    QuickWeaponHeaders[MAX_QUICKWEAPONSLOT];
	ieWord    QuickItemSlots[MAX_QUICKITEMSLOT];           //pst has 5, others use only 3
	ieWord    QuickItemHeaders[MAX_QUICKITEMSLOT];
	ieByte    QSlots[GUIBT_COUNT];          //iwd2 specific
	ieByte    QuickSpellClass[MAX_QSLOTS];
	ieWord    PortraitIcons[MAX_PORTRAIT_ICONS];
	ieWord    PreviousPortraitIcons[MAX_PORTRAIT_ICONS];
	ieByte    PortraitIconString[MAX_PORTRAIT_ICONS+2];
	ieDword   LastLeft;   //trigger
	ieDword   LastJoined; //trigger
	ieDword   Interact[MAX_INTERACT];
	ieWordSigned Happiness;
	std::list<int> ClassLevels;
private:
	void SetQuickItemSlot(int x, int slot, int headerindex);
public:
	PCStatsStruct();
	PCStatsStruct(std::list<int> levels);
	PCStatsStruct& operator=(const PCStatsStruct &source);
	void Init(bool all=true);
	void IncrementChapter();
	void NotifyKill(ieDword xp, ieStrRef name);
	void InitQuickSlot(unsigned int which, int slot, int headerindex);
	void SetSlotIndex(unsigned int which, ieWord headerindex);
	void GetSlotAndIndex(unsigned int which, ieWord &slot, ieWord &headerindex);
	int GetHeaderForSlot(int slot);
	void RegisterFavourite(ieResRef fav, int what);
	void UpdateClassLevels(std::list<int> levels);
};
}

#endif
