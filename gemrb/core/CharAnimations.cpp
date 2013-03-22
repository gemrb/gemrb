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

#include "CharAnimations.h"

#include "win32def.h"

#include "AnimationFactory.h"
#include "DataFileMgr.h"
#include "Game.h"
#include "GameData.h"
#include "ImageMgr.h"
#include "Interface.h"
#include "Map.h"
#include "Palette.h"
#include "Video.h"

namespace GemRB {

static int AvatarsCount = 0;
static AvatarStruct *AvatarTable = NULL;
static const ieByte SixteenToNine[16]={0,1,2,3,4,5,6,7,8,7,6,5,4,3,2,1};
static const ieByte SixteenToFive[16]={0,0,1,1,2,2,3,3,4,4,3,3,2,2,1,1};

static const int zOrder_TwoPiece[2] = { 1, 0 };

static const int zOrder_Mirror16[16][4] = {
	{ 0, 3, 2, 1 },
	{ 0, 3, 2, 1 },
	{ 0, 3, 1, 2 },
	{ 0, 3, 1, 2 },
	{ 1, 0, 3, 2 },
	{ 1, 0, 3, 2 },
	{ 1, 0, 3, 2 },
	{ 1, 0, 3, 2 },
	{ 1, 0, 3, 2 },
	{ 1, 0, 3, 2 },
	{ 1, 0, 3, 2 },
	{ 1, 0, 3, 2 },
	{ 1, 0, 3, 2 },
	{ 0, 3, 1, 2 },
	{ 0, 3, 1, 2 },
	{ 0, 3, 2, 1 }
};

static const int zOrder_8[8][4] = {
	{ 0, 3, 2, 1 },
	{ 0, 3, 1, 2 },
	{ 1, 0, 3, 2 },
	{ 1, 2, 0, 3 },
	{ 1, 2, 0, 3 },
	{ 2, 0, 3, 1 },
	{ 2, 0, 3, 1 },
	{ 2, 0, 3, 1 }
};

struct EquipResRefData {
	char Suffix[9];
	unsigned char Cycle;
};


void CharAnimations::ReleaseMemory()
{
	if (AvatarTable) {
		free(AvatarTable);
		AvatarTable=NULL;
	}
}

int CharAnimations::GetAvatarsCount()
{
	return AvatarsCount;
}

AvatarStruct *CharAnimations::GetAvatarStruct(int RowNum)
{
	return AvatarTable+RowNum;
}

unsigned int CharAnimations::GetAnimationID() const
{
	if (AvatarsRowNum==~0u) return 0;
	return AvatarTable[AvatarsRowNum].AnimID;
}

int CharAnimations::GetCircleSize() const
{
	if (AvatarsRowNum==~0u) return -1;
	return AvatarTable[AvatarsRowNum].CircleSize;
}
int CharAnimations::NoPalette() const
{
	if (AvatarsRowNum==~0u) return -1;
	return AvatarTable[AvatarsRowNum].PaletteType;
}

int CharAnimations::GetAnimType() const
{
	if (AvatarsRowNum==~0u) return -1;
	return AvatarTable[AvatarsRowNum].AnimationType;
}

int CharAnimations::GetSize() const
{
	if (AvatarsRowNum==~0u) return 0;
	return AvatarTable[AvatarsRowNum].Size;
}

int CharAnimations::GetBloodColor() const
{
	if(AvatarsRowNum==~0u) return 0;
	return AvatarTable[AvatarsRowNum].BloodColor;
}

unsigned int CharAnimations::GetFlags() const
{
	if(AvatarsRowNum==~0u) return 0;
	return AvatarTable[AvatarsRowNum].Flags;
}

unsigned char CharAnimations::MaybeOverrideStance(unsigned char stance) const
{
	if(AvatarsRowNum==~0u) return stance;
	return AvatarTable[AvatarsRowNum].StanceOverride[stance];
}

static ieResRef EmptySound={0};

const ieResRef &CharAnimations::GetWalkSound() const
{
	if(AvatarsRowNum==~0u) return EmptySound;
	return AvatarTable[AvatarsRowNum].WalkSound;
}

int CharAnimations::GetWalkSoundCount() const
{
	if(AvatarsRowNum==~0u) return 0;
	return AvatarTable[AvatarsRowNum].WalkSoundCount;
}

int CharAnimations::GetActorPartCount() const
{
	if (AvatarsRowNum==~0u) return -1;
	switch (AvatarTable[AvatarsRowNum].AnimationType) {
	case IE_ANI_NINE_FRAMES: //dragon animations
		return 9;
	case IE_ANI_FOUR_FRAMES: //wyvern animations
		return 4;
	case IE_ANI_TWO_PIECE:   //ankheg animations
		return 2;
	case IE_ANI_PST_GHOST:   //special pst anims
		if (AvatarTable[AvatarsRowNum].Prefixes[1][0]=='*') {
			return 1;
		}
		if (AvatarTable[AvatarsRowNum].Prefixes[2][0]=='*') {
			return 2;
		}
		if (AvatarTable[AvatarsRowNum].Prefixes[3][0]=='*') {
			return 3;
		}
		return 4;
	default:
		return 1;
	}
}

int CharAnimations::GetTotalPartCount() const
{
	if (AvatarsRowNum==~0u) return -1;
	switch (AvatarTable[AvatarsRowNum].AnimationType) {
	case IE_ANI_FOUR_FILES:
	case IE_ANI_FOUR_FILES_2:
		return GetActorPartCount() + 1; // only weapon
	case IE_ANI_CODE_MIRROR:
		return GetActorPartCount() + 3; // equipment
	case IE_ANI_TWENTYTWO:
		return GetActorPartCount() + 3; // equipment
	default:
		return GetActorPartCount();
	}
}

const ieResRef& CharAnimations::GetArmourLevel(int ArmourLevel) const
{
	//ignore ArmourLevel for the static pst anims (all sprites are displayed)
	if (AvatarTable[AvatarsRowNum].AnimationType == IE_ANI_PST_GHOST) {
		ArmourLevel = 0;
	}
	return AvatarTable[AvatarsRowNum].Prefixes[ArmourLevel];
}

void CharAnimations::SetArmourLevel(int ArmourLevel)
{
	if (AvatarsRowNum==~0u) return;
	//ignore ArmourLevel for the static pst anims (all sprites are displayed)
	if (AvatarTable[AvatarsRowNum].AnimationType == IE_ANI_PST_GHOST) {
		ArmourLevel = 0;
	}
	CopyResRef( ResRef, AvatarTable[AvatarsRowNum].Prefixes[ArmourLevel] );
	DropAnims();
}

//RangedType could be weird, reducing its value to 0,1,2
void CharAnimations::SetRangedType(int rt)
{
	if ((unsigned int) rt<2) {
		RangedType=(ieByte) rt;
	} else {
		RangedType=2;
	}
}

void CharAnimations::SetWeaponType(int wt)
{
	if (wt != WeaponType) {
		WeaponType = wt;
		DropAnims();
	}
}

void CharAnimations::SetHelmetRef(const char* ref)
{
	HelmetRef[0] = ref[0];
	HelmetRef[1] = ref[1];

	// TODO: Only drop helmet anims?
	// Note: this doesn't happen "often", so this isn't a performance
	//       bottleneck. (wjp)
	DropAnims();
	gamedata->FreePalette(palette[PAL_HELMET], 0);
	gamedata->FreePalette(modifiedPalette[PAL_HELMET], 0);
}

void CharAnimations::SetWeaponRef(const char* ref)
{
	WeaponRef[0] = ref[0];
	WeaponRef[1] = ref[1];

	// TODO: Only drop weapon anims?
	DropAnims();
	gamedata->FreePalette(palette[PAL_WEAPON], 0);
	gamedata->FreePalette(modifiedPalette[PAL_WEAPON], 0);
}

void CharAnimations::SetOffhandRef(const char* ref)
{
	OffhandRef[0] = ref[0];
	OffhandRef[1] = ref[1];

	// TODO: Only drop shield/offhand anims?
	DropAnims();
	gamedata->FreePalette(palette[PAL_OFFHAND], 0);
	gamedata->FreePalette(modifiedPalette[PAL_OFFHAND], 0);
}

void CharAnimations::LockPalette(const ieDword *gradients)
{
	if (lockPalette) return;
	//cannot lock colors for PST animations
	if (GetAnimType() >= IE_ANI_PST_ANIMATION_1)
	{
		return;
	}
	//force initialisation of animation
	SetColors( gradients );
	GetAnimation(0,0);
	if (palette[PAL_MAIN]) {
		lockPalette=true;
	}
}

//                            0   1   2   3   4   5   6   7   8   9  10  11  12  13  14  15  16  17  18
static const char *StancePrefix[]={"3","2","5","5","4","4","2","2","5","4","1","3","3","3","4","1","4","4","4"};
static const char *CyclePrefix[]= {"0","0","1","1","1","1","0","0","1","1","0","1","1","1","1","1","1","1","1"};
static const unsigned int CycleOffset[] = {0,  0,  0,  0,  0,  9,  0,  0,  0, 18,  0,  0,  9,  18,  0,  0,  0,  0,  0};

void CharAnimations::SetColors(const ieDword *arg)
{
	Colors = arg;
	SetupColors(PAL_MAIN);
	SetupColors(PAL_WEAPON);
	SetupColors(PAL_OFFHAND);
	SetupColors(PAL_HELMET);
}

void CharAnimations::CheckColorMod()
{
	if (!GlobalColorMod.locked) {
		if (GlobalColorMod.type != RGBModifier::NONE) {
			GlobalColorMod.type = RGBModifier::NONE;
			GlobalColorMod.speed = 0;
		  change[0]=change[1]=change[2]=change[3]=true;
		}
	}
	unsigned int location;

	for (location = 0; location < 32; ++location) {
		if (!ColorMods[location].phase) {
		  if (ColorMods[location].type != RGBModifier::NONE) {
				ColorMods[location].type = RGBModifier::NONE;
				ColorMods[location].speed = 0;
		    change[location>>3]=true;
		  }
		}
	}
	//this is set by sanctuary and stoneskin (override global colors)
	lockPalette = false;
}

void CharAnimations::SetupColors(PaletteType type)
{
	Palette* pal = palette[(int)type];

	if (!pal) {
		return;
	}

	if (!Colors) {
		return;
	}

	if (GetAnimType() >= IE_ANI_PST_ANIMATION_1) {
		// Only do main palette
		if (type != PAL_MAIN) {
			return;
		}
		// TODO: handle equipment colour glows

		// Colors[6] is the COLORCOUNT stat in PST.
		// It tells how many customisable color slots we have.
		// The color slots start from the end of the palette and go
		// backwards. There are 6 available slots with a size of 32 each.
		// Actually, the slots seem to be written in the cre file
		// but we ignore them, i'm not sure this is correct
		int colorcount = Colors[6];
		int size = 32;
		//the color count shouldn't be more than 6!
		if (colorcount>6) colorcount=6;
		int dest = 256-colorcount*size;
		bool needmod = false;
		if (GlobalColorMod.type != RGBModifier::NONE) {
			needmod = true;
		}
		//don't drop the palette, it disables rgb pulse effects
		//also don't bail out, we need to free the modified palette later
		//so this entire block is needless
		/*
		if ((colorcount == 0) && (needmod==false) ) {
		  gamedata->FreePalette(palette[PAL_MAIN], PaletteResRef);
			PaletteResRef[0]=0;
			return;
		}
		*/
		for (int i = 0; i < colorcount; i++) {
			core->GetPalette( Colors[i]&255, size,
				&palette[PAL_MAIN]->col[dest] );
			dest +=size;
		}

		if (needmod) {
			if (!modifiedPalette[PAL_MAIN])
				modifiedPalette[PAL_MAIN] = new Palette();
			modifiedPalette[PAL_MAIN]->SetupGlobalRGBModification(palette[PAL_MAIN], GlobalColorMod);
		} else {
			gamedata->FreePalette(modifiedPalette[PAL_MAIN], 0);
		}
		return;
	}

	int PType = NoPalette();
	if ( PType && (type == PAL_MAIN) ) {
		bool needmod = false;
		if (GlobalColorMod.type != RGBModifier::NONE) {
			needmod = true;
		}
		if (!needmod && PaletteResRef[0]) {
			gamedata->FreePalette(palette[PAL_MAIN], PaletteResRef);
		}
		PaletteResRef[0]=0;
		//handling special palettes like MBER_BL (black bear)
		if (PType!=1) {
			if (GetAnimType()==IE_ANI_NINE_FRAMES) {
				snprintf(PaletteResRef,9,"%.4s_%-.2s%s",ResRef, (char *) &PType, StancePrefix[StanceID]);
			} else {
				snprintf(PaletteResRef,9,"%.4s_%-.2s",ResRef, (char *) &PType);
			}
			strlwr(PaletteResRef);
			Palette *tmppal = gamedata->GetPalette(PaletteResRef);
			if (tmppal) {
				palette[PAL_MAIN] = tmppal;
			} else {
				PaletteResRef[0]=0;
			}
		}
		if (needmod) {
			if (!modifiedPalette[PAL_MAIN])
				modifiedPalette[PAL_MAIN] = new Palette();
			modifiedPalette[PAL_MAIN]->SetupGlobalRGBModification(palette[PAL_MAIN], GlobalColorMod);
		} else {
			gamedata->FreePalette(modifiedPalette[PAL_MAIN], 0);
		}
		return;
	}

	pal->SetupPaperdollColours(Colors, (int)type);
	if (lockPalette) {
		return;
	}

	int i;
	bool needmod = false;
	if (GlobalColorMod.type != RGBModifier::NONE) {
		needmod = true;
	} else {
		for (i = 0; i < 7; ++i) {
			if (ColorMods[i+8*((int)type)].type != RGBModifier::NONE)
				needmod = true;
		}
	}


	if (needmod) {
		if (!modifiedPalette[(int)type])
			modifiedPalette[(int)type] = new Palette();

		if (GlobalColorMod.type != RGBModifier::NONE) {
			modifiedPalette[(int)type]->SetupGlobalRGBModification(palette[(int)type], GlobalColorMod);
		} else {
			modifiedPalette[(int)type]->SetupRGBModification(palette[(int)type],ColorMods, (int)type);
		}
	} else {
		gamedata->FreePalette(modifiedPalette[(int)type], 0);
	}

}

Palette* CharAnimations::GetPartPalette(int part)
{
	int actorPartCount = GetActorPartCount();
	PaletteType type = PAL_MAIN;

	// always use unmodified BAM palette for the supporting part
	if (GetAnimType() == IE_ANI_TWO_PIECE && part == 1) return NULL;
	else if (part == actorPartCount) type = PAL_WEAPON;
	else if (part == actorPartCount+1) type = PAL_OFFHAND;
	else if (part == actorPartCount+2) type = PAL_HELMET;

	if (modifiedPalette[(int)type])
		return modifiedPalette[(int)type];

	return palette[(int)type];
}

static int compare_avatars(const void *a, const void *b)
{
	unsigned int aa = ((AvatarStruct *)a)->AnimID;
	unsigned int bb = ((AvatarStruct *)b)->AnimID;
	return (int) (aa-bb);
}

void CharAnimations::InitAvatarsTable()
{
	AutoTable Avatars("avatars");
	if (!Avatars) {
		error("CharAnimations", "A critical animation file is missing!\n");
	}
	AvatarTable = (AvatarStruct *) calloc ( AvatarsCount = Avatars->GetRowCount(), sizeof(AvatarStruct) );
	int i=AvatarsCount;
	DataFileMgr *resdata = core->GetResDataINI();
	while(i--) {
		AvatarTable[i].AnimID=(unsigned int) strtol(Avatars->GetRowName(i),NULL,0 );
		strnlwrcpy(AvatarTable[i].Prefixes[0],Avatars->QueryField(i,AV_PREFIX1),8);
		strnlwrcpy(AvatarTable[i].Prefixes[1],Avatars->QueryField(i,AV_PREFIX2),8);
		strnlwrcpy(AvatarTable[i].Prefixes[2],Avatars->QueryField(i,AV_PREFIX3),8);
		strnlwrcpy(AvatarTable[i].Prefixes[3],Avatars->QueryField(i,AV_PREFIX4),8);
		AvatarTable[i].AnimationType=(ieByte) atoi(Avatars->QueryField(i,AV_ANIMTYPE) );
		AvatarTable[i].CircleSize=(ieByte) atoi(Avatars->QueryField(i,AV_CIRCLESIZE) );
		const char *tmp = Avatars->QueryField(i,AV_USE_PALETTE);
		//QueryField will always return a zero terminated string
		//so tmp[0] must exist
		if ( isalpha (tmp[0]) ) {
			//this is a hack, we store 2 letters on an integer
			//it was allocated with calloc, so don't bother erasing it
			strncpy( (char *) &AvatarTable[i].PaletteType, tmp, 3);
		}
		else {
			AvatarTable[i].PaletteType=atoi(Avatars->QueryField(i,AV_USE_PALETTE) );
		}
		char size = Avatars->QueryField(i,AV_SIZE)[0];
		if (size == '*') {
			size = 0;
		}
		AvatarTable[i].Size = size;

		AvatarTable[i].WalkScale = 0;
		AvatarTable[i].RunScale = 0;
		AvatarTable[i].Bestiary = -1;
		
		for (int j = 0; j < MAX_ANIMS; j++)
			AvatarTable[i].StanceOverride[j] = j;

		if (resdata) {
			char section[12];
			snprintf(section,10,"%d", i);

			if (!resdata->GetKeysCount(section)) continue;

			float walkscale = resdata->GetKeyAsFloat(section, "walkscale", 0.0f);
			if (walkscale != 0.0f) AvatarTable[i].WalkScale = (int)(1000.0f / walkscale);
			float runscale = resdata->GetKeyAsFloat(section, "runscale", 0.0f);
			if (runscale != 0.0f) AvatarTable[i].RunScale = (int)(1000.0f / runscale);
			AvatarTable[i].Bestiary = resdata->GetKeyAsInt(section, "bestiary", -1);
		}
	}
	qsort(AvatarTable, AvatarsCount, sizeof(AvatarStruct), compare_avatars);


	AutoTable blood("bloodclr");
	if (blood) {
		int rows = blood->GetRowCount();
		for(int i=0;i<rows;i++) {
			unsigned long value = 0;
			unsigned long flags = 0;
			unsigned long rmin = 0;
			unsigned long rmax = 0xffff;

			valid_number(blood->QueryField(i,0), (long &)value);
			valid_number(blood->QueryField(i,1), (long &)rmin);
			valid_number(blood->QueryField(i,2), (long &)rmax);
			valid_number(blood->QueryField(i,3), (long &)flags);
			if (value>255 || rmin>rmax || rmax>0xffff) {
				Log(ERROR, "CharAnimations", "Invalid bloodclr entry: %02x %04x-%04x ",
						(unsigned int) value, (unsigned int) rmin, (unsigned int) rmax);
				continue;
			}
			for(int j=0;j<AvatarsCount;j++) {
				if (rmax<AvatarTable[j].AnimID) break;
				if (rmin>AvatarTable[j].AnimID) continue;
				AvatarTable[j].BloodColor = (char) value;
				AvatarTable[j].Flags = (unsigned int) flags;
			}
		}
	}

	AutoTable walk("walksnd");
	if (walk) {
		int rows = walk->GetRowCount();
		for(int i=0;i<rows;i++) {
			ieResRef value;
			unsigned long rmin = 0;
			unsigned long rmax = 0xffff;
			unsigned long range = 0;

			strnuprcpy(value, walk->QueryField(i,0), 8);
			valid_number(walk->QueryField(i,1), (long &)rmin);
			valid_number(walk->QueryField(i,2), (long &)rmax);
			valid_number(walk->QueryField(i,3), (long &)range);
			if (value[0]=='*') {
				value[0]=0;
				range = 0;
			}
			if (range>255 || rmin>rmax || rmax>0xffff) {
				Log(ERROR, "CharAnimations", "Invalid walksnd entry: %02x %04x-%04x ",
						(unsigned int) range, (unsigned int) rmin, (unsigned int) rmax);
				continue;
			}
			for(int j=0;j<AvatarsCount;j++) {
				if (rmax<AvatarTable[j].AnimID) break;
				if (rmin>AvatarTable[j].AnimID) continue;
				memcpy(AvatarTable[j].WalkSound, value, sizeof(ieResRef) );
				AvatarTable[j].WalkSoundCount = (unsigned int) range;
			}
		}
	}

	AutoTable stances("stances", true);
	if (stances) {
		int rows = stances->GetRowCount();
		for (int i = 0; i < rows; i++) {
			unsigned long id = 0, s1 = 0, s2 = 0;
			valid_number(stances->GetRowName(i), (long &)id);
			valid_number(stances->QueryField(i, 0), (long &)s1);
			valid_number(stances->QueryField(i, 1), (long &)s2);

			if (s1 >= MAX_ANIMS || s2 >= MAX_ANIMS) {
				Log(ERROR, "CharAnimations", "Invalid stances entry: %04x %d %d",
						(unsigned int) id, (unsigned int) s1, (unsigned int) s2);
				continue;
			}

			for (int j = 0; j < AvatarsCount; j++) {
				if (id < AvatarTable[j].AnimID) break;
				if (id == AvatarTable[j].AnimID) {
					AvatarTable[j].StanceOverride[s1] = s2;
					break;
				}
			}
		}
	}
}

CharAnimations::CharAnimations(unsigned int AnimID, ieDword ArmourLevel)
{
	Colors = NULL;
	int i,j;
	for (i = 0; i < 4; ++i) {
		change[i] = true;
		modifiedPalette[i] = NULL;
		palette[i] = NULL;
	}
	nextStanceID = 0;
	StanceID = 0;
	autoSwitchOnEnd = false;
	lockPalette = false;
	if (!AvatarsCount) {
		InitAvatarsTable();
	}

	for (i = 0; i < MAX_ANIMS; i++) {
		for (j = 0; j < MAX_ORIENT; j++) {
			Anims[i][j] = NULL;
		}
	}
	ArmorType = 0;
	RangedType = 0;
	WeaponType = 0;
	PaletteResRef[0] = 0;
	WeaponRef[0] = 0;
	HelmetRef[0] = 0;
	OffhandRef[0] = 0;
	for (i = 0; i < 32; ++i) {
		ColorMods[i].type = RGBModifier::NONE;
		ColorMods[i].speed = 0;
		// make initial phase depend on location to make the pulse appear
		// less even
		ColorMods[i].phase = 5*i;
		ColorMods[i].locked = false;
	}
	GlobalColorMod.type = RGBModifier::NONE;
	GlobalColorMod.speed = 0;
	GlobalColorMod.phase = 0;
	GlobalColorMod.locked = false;
	lastModUpdate = 0;

	AvatarsRowNum=AvatarsCount;
	if (core->HasFeature(GF_ONE_BYTE_ANIMID) ) {
		ieDword tmp = AnimID&0xf000;
		if (tmp==0x6000 || tmp==0xe000) {
			AnimID&=0xff;
		}
	}

	while (AvatarsRowNum--) {
		if (AvatarTable[AvatarsRowNum].AnimID<=AnimID) {
			SetArmourLevel( ArmourLevel );
			return;
		}
	}
	ResRef[0]=0;
	Log(ERROR, "CharAnimations", "Invalid or nonexistent avatar entry:%04X", AnimID);
}

//we have to drop them when armourlevel changes
void CharAnimations::DropAnims()
{
	Animation** tmppoi;
	int partCount = GetTotalPartCount();
	for (int StanceID = 0; StanceID < MAX_ANIMS; StanceID++) {
		for (int i = 0; i < MAX_ORIENT; i++) {
			if (Anims[StanceID][i]) {
				tmppoi = Anims[StanceID][i];
				for (int j = 0; j < partCount; j++)
					delete Anims[StanceID][i][j];
				delete[] tmppoi;

				// anims can only be duplicated at the Animation** level
				for (int IDb=StanceID;IDb < MAX_ANIMS; IDb++) {
					for (int i2 = 0; i2<MAX_ORIENT; i2++) {
						if (Anims[IDb][i2] == tmppoi) {
							Anims[IDb][i2] = 0;
						}
					}
				}
			}
		}
	}
}

CharAnimations::~CharAnimations(void)
{
	DropAnims();
	gamedata->FreePalette(palette[PAL_MAIN], PaletteResRef);
	int i;
	for (i = 1; i < 4; ++i)
		gamedata->FreePalette(palette[i], 0);
	for (i = 0; i < 4; ++i)
		gamedata->FreePalette(modifiedPalette[i], 0);
}
/*
This is a simple Idea of how the animation are coded

There are the following animation types:

IE_ANI_CODE_MIRROR: The code automatically mirrors the needed frames
			(as in the example above)

			These Animations are stores using the following template:
			[NAME][ARMORTYPE][ACTIONCODE]

			Each BAM File contains only 9 Orientations, the missing 7 Animations
			are created by Horizontally Mirroring the 1-7 Orientations.

IE_ANI_CODE_MIRROR_2: another mirroring type with more animations
			[NAME]g[1,11-15,2,21-26]

IE_ANI_CODE_MIRROR_3: Almost identical to IE_ANI_CODE_MIRROR_2, but with fewer cycles in g26

IE_ANI_ONE_FILE:	The whole animation is in one file, no mirroring needed.
			Each animation group is 16 Cycles.

IE_ANI_TWO_FILES:	The whole animation is in 2 files. The East and West part are in 2 BAM Files.
			Example:
			ACHKg1
			ACHKg1E

			Each BAM File contains many animation groups, each animation group
			stores 5 Orientations, the missing 3 are stored in East BAM Files.


IE_ANI_FOUR_FILES:	The Animation is coded in Four Files. Probably it is an old Two File animation with
			additional frames added in a second time.

IE_ANI_FOUR_FILES_2:	Like IE_ANI_FOUR_FILES but with only 16 cycles per frame.

IE_ANI_FOUR_FILES_3: A variation of the four files animation without equipment, and
			with even and odd orientations split across two files, plus the standard
			separate eastern parts, so the layout is
			[NAME][H|L]G1[/E]

IE_ANI_TWENTYTWO:	This Animation Type stores the Animation in the following format
			[NAME][ACTIONCODE][/E]
			ACTIONCODE=A1-6, CA, SX, SA (sling is A1)
			The g1 file contains several animation states. See MHR
			Probably it could use A7-9 files too, bringing the file numbers to 28.
			This is the original bg1 format.

IE_ANI_SIX_FILES:	The layout for these files is:
			[NAME][g1-3][/E]
			Each state contains 16 Orientations, but the last 6 are stored in the East file.
			g1 contains only the walking animation.
			G2 contains stand, ready, get hit, die and twitch.
			g3 contains 3 attacks.

IE_ANI_SIX_FILES_2:     Similar to SIX_FILES, but the orientation numbers are reduced like in FOUR_FILES. Only one animation uses it: MOGR

IE_ANI_TWO_FILES_2:	Animations using this type are stored using the following template:
			[NAME]g1[/E]
			Each state contains 8 Orientations, but the second 4 are stored in the East file.
			From the standard animations, only AHRS and ACOW belong to this type.

IE_ANI_TWO_FILES_3:	Animations using this type are stored using the following template:
			[NAME][ACTIONTYPE][/E]

			Example:
			MBFI*
			MBFI*E

			Each BAM File contains one animation group, each animation group
			stores 5 Orientations though the files contain all 8 Cycles, the missing 3 are stored in East BAM Files in Cycle: Stance*8+ (5,6,7).
			This is the standard IWD animation, but BG2 also has it.
			See MMR

IE_ANI_TWO_FILES_3B:	Animations using this type are stored using the following template:
			[NAME][ACTIONTYPE][/E]

			Example:
			MBFI*
			MBFI*E

			This is a cut down version of IE_ANI_TWO_FILES_3. A2, CA and SP suffixes are missing.
			This is the standard IWD animation, but BG2 also has it.
			See MOR2

IE_ANI_TWO_PIECE: This is a modified IE_ANI_SIX_FILES with supporting still frames (using a
			different palette) stored in a second set of files. Currently only used by MAKH

IE_ANI_FOUR_FRAMES:	These animations are large, four bams make a frame.


IE_ANI_NINE_FRAMES:     These animations are huge, nine bams make a frame.


IE_ANI_FRAGMENT:        These animations are used for projectile graphics.
			A single file contains 5 cycles (code mirror for east animation)

IE_ANI_PST_ANIMATION_1:
IE_ANI_PST_ANIMATION_2:
IE_ANI_PST_ANIMATION_3:
			Planescape: Torment Animations are stored in a different
			way than the other games. This format uses the following template:
			[C/D][ACTIONTYPE][NAME][B]

			Example:
			CAT1MRTB

			Each Animation stores 5 Orientations, which are automatically mirrored
			to form an 8 Orientation Animation. PST Animations have a different Palette
			format. This Animation Type handles the PST Palette format too.

			NOTE: Walking/Running animations store 9 Orientations.
			The second variation is missing the resting stance (STD) and the transitions.
			These creatures are always in combat stance (don't rest).
			Animation_3 is without STC (combat stance), they are always standing

IE_ANI_PST_STAND:	This is a modified PST animation, it contains only a
			Standing image for every orientations, it follows the
			[C/D]STD[NAME][B] standard.

IE_ANI_PST_GHOST:	This is a special static animation with no standard
			All armourlevels are drawn simultaneously. There is no orientation or stance.


  WEST PART  |  EAST PART
             |
    NW  NNW  N  NNE  NE
 NW 006 007 008 009 010 NE
WNW 005      |      011 ENE
  W 004     xxx     012 E
WSW 003      |      013 ESE
 SW 002 001 000 015 014 SE
    SW  SSW  S  SSE  SE
             |
             |

*/

Animation** CharAnimations::GetAnimation(unsigned char Stance, unsigned char Orient)
{
	if (StanceID>=MAX_ANIMS) {
		error("CharAnimation", "Illegal stance ID\n");
	}

	//for paletted dragon animations, we need the stance id
	StanceID = nextStanceID = Stance;
	int AnimType = GetAnimType();

	//alter stance here if it is missing and you know a substitute
	//probably we should feed this result back to the actor?
	switch (AnimType) {
		case -1: //invalid animation
			return NULL;

		case IE_ANI_PST_STAND:
			StanceID=IE_ANI_AWAKE;
			break;
		case IE_ANI_PST_GHOST:
			StanceID=IE_ANI_AWAKE;
			Orient=0;
			break;
		case IE_ANI_PST_ANIMATION_3: //stc->std
			if (StanceID==IE_ANI_READY) {
				StanceID=IE_ANI_AWAKE;
			}
			break;
		case IE_ANI_PST_ANIMATION_2: //std->stc
			if (StanceID==IE_ANI_AWAKE) {
				StanceID=IE_ANI_READY;
			}
			break;
	}
	//pst animations don't have separate animation for sleep/die
	if (AnimType >= IE_ANI_PST_ANIMATION_1) {
		if (StanceID==IE_ANI_DIE) {
			StanceID=IE_ANI_TWITCH;
		}
	}

	StanceID = MaybeOverrideStance(StanceID);

	//TODO: Implement Auto Resource Loading
	//setting up the sequencing of animation cycles
	autoSwitchOnEnd = false;
	switch (StanceID) {
		case IE_ANI_DAMAGE:
			nextStanceID = IE_ANI_READY;
			autoSwitchOnEnd = true;
			break;
		case IE_ANI_SLEEP: //going to sleep
			nextStanceID = IE_ANI_TWITCH;
			autoSwitchOnEnd = true;
			break;
		case IE_ANI_TWITCH: //dead, sleeping
			autoSwitchOnEnd = false;
			break;
		case IE_ANI_DIE: //going to die
			nextStanceID = IE_ANI_TWITCH;
			autoSwitchOnEnd = true;
			break;
		case IE_ANI_WALK:
		case IE_ANI_RUN:
		case IE_ANI_CAST: // looping
		case IE_ANI_READY:
			break;
		case IE_ANI_AWAKE:
			break;
		case IE_ANI_EMERGE:
		case IE_ANI_GET_UP:
		case IE_ANI_HEAD_TURN:
		case IE_ANI_PST_START:
			nextStanceID = IE_ANI_AWAKE;
			autoSwitchOnEnd = true;
			break;
		case IE_ANI_CONJURE: //ending
		case IE_ANI_SHOOT:
		case IE_ANI_ATTACK:
		case IE_ANI_ATTACK_JAB:
		case IE_ANI_ATTACK_SLASH:
		case IE_ANI_ATTACK_BACKSLASH:
			nextStanceID = IE_ANI_READY;
			autoSwitchOnEnd = true;
			break;
		default:
			Log(MESSAGE, "CharAnimation", "Invalid Stance: %d", StanceID);
			break;
	}
	Animation** anims = Anims[StanceID][Orient];

	if (anims) {
		return anims;
	}

	int partCount = GetTotalPartCount();
	int actorPartCount = GetActorPartCount();
	if (partCount <= 0) return 0;
	anims = new Animation*[partCount];

	EquipResRefData* equipdat = 0;
	for (int part = 0; part < partCount; ++part)
	{
		anims[part] = 0;

		//newresref is based on the prefix (ResRef) and various
		// other things.
		//this is longer than expected so it won't overflow
		char NewResRef[12];
		unsigned char Cycle = 0;
		if (part < actorPartCount) {
			// Character animation parts

			if (equipdat) delete equipdat;

			//we need this long for special anims
			strlcpy( NewResRef, ResRef, sizeof(ieResRef) );
			GetAnimResRef( StanceID, Orient, NewResRef, Cycle, part, equipdat);
		} else {
			// Equipment animation parts

			anims[part] = 0;
			if (GetSize() == 0) continue;

			if (part == actorPartCount) {
				if (WeaponRef[0] == 0) continue;
				// weapon
				GetEquipmentResRef(WeaponRef,false,NewResRef,Cycle,equipdat);
			} else if (part == actorPartCount+1) {
				if (OffhandRef[0] == 0) continue;
				if (WeaponType == IE_ANI_WEAPON_2H) continue;
				// off-hand
				if (WeaponType == IE_ANI_WEAPON_1H) {
					GetEquipmentResRef(OffhandRef,false,NewResRef,Cycle,
										 equipdat);
				} else { // IE_ANI_WEAPON_2W
					GetEquipmentResRef(OffhandRef,true,NewResRef,Cycle,
										 equipdat);
				}
			} else if (part == actorPartCount+2) {
				if (HelmetRef[0] == 0) continue;
				// helmet
				GetEquipmentResRef(HelmetRef,false,NewResRef,Cycle,equipdat);
			}
		}
		NewResRef[8]=0; //cutting right to size

		AnimationFactory* af = ( AnimationFactory* )
			gamedata->GetFactoryResource( NewResRef,
					IE_BAM_CLASS_ID, IE_NORMAL );

		if (!af) {
			if (part < actorPartCount) {
				Log(ERROR, "CharAnimations", "Couldn't create animationfactory: %s (%04x)",
						NewResRef, GetAnimationID());;
				for (int i = 0; i < part; ++i)
					delete anims[i];
				delete[] anims;
				delete equipdat;
				return 0;
			} else {
				// not fatal if animation for equipment is missing
				continue;
			}
		}

		Animation* a = af->GetCycle( Cycle );
		anims[part] = a;

		if (!a) {
			if (part < actorPartCount) {
				Log(ERROR, "CharAnimations", "Couldn't load animation: %s, cycle %d",
						 NewResRef, Cycle);
				for (int i = 0; i < part; ++i)
					delete anims[i];
				delete[] anims;
				delete equipdat;
				return 0;
			} else {
				// not fatal if animation for equipment is missing
				continue;
			}
		}

		if (part < actorPartCount) {
			//if you need to revert this change, consider true paletted
			//animations which need a GlobalColorMod (mgir for example)

			//if (!palette[PAL_MAIN] && ((GlobalColorMod.type!=RGBModifier::NONE) || (NoPalette()!=1)) ) {
			if(!palette[PAL_MAIN]) {
				// This is the first time we're loading an Animation.
				// We copy the palette of its first frame into our own palette
				palette[PAL_MAIN] = a->GetFrame(0)->GetPalette()->Copy();
				// ...and setup the colours properly
				SetupColors(PAL_MAIN);
			}
		} else if (part == actorPartCount) {
			if (!palette[PAL_WEAPON]) {
				palette[PAL_WEAPON] = a->GetFrame(0)->GetPalette()->Copy();
				SetupColors(PAL_WEAPON);
			}
		} else if (part == actorPartCount+1) {
			if (!palette[PAL_OFFHAND]) {
				palette[PAL_OFFHAND] = a->GetFrame(0)->GetPalette()->Copy();
				SetupColors(PAL_OFFHAND);
			}
		} else if (part == actorPartCount+2) {
			if (!palette[PAL_HELMET]) {
				palette[PAL_HELMET] = a->GetFrame(0)->GetPalette()->Copy();
				SetupColors(PAL_HELMET);
			}
		}

		//animation is affected by game flags
		a->gameAnimation = true;
		a->SetPos( 0 );

		//setting up the sequencing of animation cycles
		switch (StanceID) {
			case IE_ANI_DAMAGE:
			case IE_ANI_SLEEP:
			case IE_ANI_TWITCH:
			case IE_ANI_DIE:
			case IE_ANI_PST_START:
			case IE_ANI_HEAD_TURN:
			case IE_ANI_CONJURE:
			case IE_ANI_SHOOT:
			case IE_ANI_ATTACK:
			case IE_ANI_ATTACK_JAB:
			case IE_ANI_ATTACK_SLASH:
			case IE_ANI_ATTACK_BACKSLASH:
				a->Flags |= A_ANI_PLAYONCE;
				break;
			case IE_ANI_EMERGE:
			case IE_ANI_GET_UP:
				a->playReversed = true;
				a->Flags |= A_ANI_PLAYONCE;
				break;
		}
		switch (GetAnimType()) {
			case IE_ANI_NINE_FRAMES: //dragon animations
			case IE_ANI_FOUR_FRAMES: //wyvern animations
			case IE_ANI_BIRD:
			case IE_ANI_CODE_MIRROR:
			case IE_ANI_CODE_MIRROR_2: //9 orientations
			case IE_ANI_CODE_MIRROR_3:
			case IE_ANI_PST_ANIMATION_3: //no stc just std
			case IE_ANI_PST_ANIMATION_2: //no std just stc
			case IE_ANI_PST_ANIMATION_1:
			case IE_ANI_FRAGMENT:
				if (Orient > 8) {
					a->MirrorAnimation( );
				}
				break;
			default:
				break;
		}

		// make animarea of part 0 encompass the animarea of the other parts
		if (part > 0)
			anims[0]->AddAnimArea(a);

	}

	switch (GetAnimType()) {
		case IE_ANI_NINE_FRAMES: //dragon animations
		case IE_ANI_FOUR_FRAMES: //wyvern animations
		case IE_ANI_BIRD:
		case IE_ANI_CODE_MIRROR:
		case IE_ANI_SIX_FILES: //16 anims some are stored elsewhere
		case IE_ANI_ONE_FILE: //16 orientations
		case IE_ANI_CODE_MIRROR_2: //9 orientations
		case IE_ANI_CODE_MIRROR_3:
			Anims[StanceID][Orient] = anims;
			break;
		case IE_ANI_TWO_FILES:
		case IE_ANI_TWENTYTWO:
		case IE_ANI_TWO_FILES_2:
		case IE_ANI_TWO_FILES_3:
		case IE_ANI_TWO_FILES_3B:
		case IE_ANI_FOUR_FILES:
		case IE_ANI_FOUR_FILES_2:
		case IE_ANI_SIX_FILES_2:
		case IE_ANI_TWO_PIECE:
		case IE_ANI_FRAGMENT:
			Orient&=~1;
			Anims[StanceID][Orient] = anims;
			Anims[StanceID][Orient + 1] = anims;
			break;
		case IE_ANI_FOUR_FILES_3:
			//only 8 orientations for WALK
			if (StanceID == IE_ANI_WALK) {
				Orient&=~1;
				Anims[StanceID][Orient] = anims;
				Anims[StanceID][Orient + 1] = anims;
			} else {
				Anims[StanceID][Orient] = anims;
			}
			break;

		case IE_ANI_PST_ANIMATION_3: //no stc just std
		case IE_ANI_PST_ANIMATION_2: //no std just stc
		case IE_ANI_PST_ANIMATION_1:
			switch (StanceID) {
				case IE_ANI_WALK:
				case IE_ANI_RUN:
				case IE_ANI_PST_START:
					Anims[StanceID][Orient] = anims;
					break;
				default:
					Orient &=~1;
					Anims[StanceID][Orient] = anims;
					Anims[StanceID][Orient + 1] = anims;
					break;
			}
			break;

		case IE_ANI_PST_STAND:
			Orient &=~1;
			Anims[StanceID][Orient] = anims;
			Anims[StanceID][Orient+1] = anims;
			break;
		case IE_ANI_PST_GHOST:
			Orient = 0;
			StanceID = IE_ANI_AWAKE;
			Anims[StanceID][0] = anims;
			break;
		default:
			error("CharAnimations", "Unknown animation type\n");
	}
	delete equipdat;

	return Anims[StanceID][Orient];
}

static const int one_file[19]={2, 1, 0, 0, 2, 3, 0, 1, 0, 4, 1, 0, 0, 0, 3, 1, 4, 4, 4};

void CharAnimations::GetAnimResRef(unsigned char StanceID,
					 unsigned char Orient,
					 char* NewResRef, unsigned char& Cycle,
					 int Part, EquipResRefData*& EquipData)
{
	EquipData = 0;
	Orient &= 15;
	switch (GetAnimType()) {
		case IE_ANI_FOUR_FRAMES:
			AddFFSuffix( NewResRef, StanceID, Cycle, Orient, Part );
			break;

		case IE_ANI_NINE_FRAMES:
			AddNFSuffix( NewResRef, StanceID, Cycle, Orient, Part );
			break;

		case IE_ANI_CODE_MIRROR:
			AddVHRSuffix( NewResRef, StanceID, Cycle, Orient, EquipData );
			break;

		case IE_ANI_BIRD:
			// TODO: use 0-8 for gliding; those only have a single frame
			Cycle = 9 + SixteenToNine[Orient];
			break;

		case IE_ANI_FRAGMENT:
			Cycle = SixteenToFive[Orient];
			break;

		case IE_ANI_ONE_FILE:
			Cycle = (ieByte) (one_file[StanceID] * 16 + Orient);
			break;

		case IE_ANI_SIX_FILES:
			AddSixSuffix( NewResRef, StanceID, Cycle, Orient );
			break;

		case IE_ANI_TWENTYTWO: //5+3 animations
			AddMHRSuffix( NewResRef, StanceID, Cycle, Orient, EquipData );
			break;

		case IE_ANI_TWO_FILES_2: //4+4 animations
			AddLR2Suffix( NewResRef, StanceID, Cycle, Orient );
			break;

		case IE_ANI_TWO_FILES_3: //IWD style anims
			AddMMRSuffix( NewResRef, StanceID, Cycle, Orient );
			break;

		case IE_ANI_TWO_FILES_3B: //IWD style anims
			AddMMR2Suffix( NewResRef, StanceID, Cycle, Orient );
			break;

		case IE_ANI_TWO_FILES:
			AddTwoFileSuffix(NewResRef, StanceID, Cycle, Orient );
			break;

		case IE_ANI_FOUR_FILES:
			AddLRSuffix( NewResRef, StanceID, Cycle, Orient, EquipData );
			break;

		case IE_ANI_FOUR_FILES_2:
			AddLRSuffix2( NewResRef, StanceID, Cycle, Orient, EquipData );
			break;

		case IE_ANI_FOUR_FILES_3:
			AddHLSuffix( NewResRef, StanceID, Cycle, Orient );
			break;

		case IE_ANI_SIX_FILES_2: //MOGR (variant of FOUR_FILES)
			AddLR3Suffix( NewResRef, StanceID, Cycle, Orient );
			break;

		case IE_ANI_TWO_PIECE: //MAKH
			AddTwoPieceSuffix( NewResRef, StanceID, Cycle, Orient, Part );
			break;

		case IE_ANI_CODE_MIRROR_2: //9 orientations
			AddVHR2Suffix( NewResRef, StanceID, Cycle, Orient );
			break;

		case IE_ANI_CODE_MIRROR_3: // like IE_ANI_CODE_MIRROR_2 but with fewer cycles in g26
			AddVHR3Suffix( NewResRef, StanceID, Cycle, Orient );
			break;

		case IE_ANI_PST_ANIMATION_1:
		case IE_ANI_PST_ANIMATION_2:
		case IE_ANI_PST_ANIMATION_3:
			AddPSTSuffix( NewResRef, StanceID, Cycle, Orient );
			break;

		case IE_ANI_PST_STAND:
			sprintf(NewResRef,"%cSTD%4s",ResRef[0], ResRef+1);
			Cycle = (ieByte) SixteenToFive[Orient];
			break;
		case IE_ANI_PST_GHOST: // pst static animations
			//still doesn't handle the second cycle of the golem anim
			Cycle = 0;
			strnlwrcpy(NewResRef, AvatarTable[AvatarsRowNum].Prefixes[Part], 8);
			break;
		default:
			error("CharAnimations", "Unknown animation type in avatars.2da row: %d\n", AvatarsRowNum);
	}
}

void CharAnimations::GetEquipmentResRef(const char* equipRef, bool offhand,
	char* ResRef, unsigned char& Cycle, EquipResRefData* equip)
{
	switch (GetAnimType()) {
		case IE_ANI_FOUR_FILES:
		case IE_ANI_FOUR_FILES_2:
			GetLREquipmentRef( ResRef, Cycle, equipRef, offhand, equip );
			break;
		case IE_ANI_CODE_MIRROR:
			GetVHREquipmentRef( ResRef, Cycle, equipRef, offhand, equip );
			break;
		case IE_ANI_TWENTYTWO:
			GetMHREquipmentRef( ResRef, Cycle, equipRef, offhand, equip );
			break;
		default:
			error("CharAnimations", "Unsupported animation type for equipment animation.\n");
		break;
	}
}

const int* CharAnimations::GetZOrder(unsigned char Orient)
{
	switch (GetAnimType()) {
		case IE_ANI_CODE_MIRROR:
			return zOrder_Mirror16[Orient];
		case IE_ANI_TWENTYTWO:
			return zOrder_8[Orient/2];
		case IE_ANI_FOUR_FILES:
			return 0; // FIXME
		case IE_ANI_TWO_PIECE:
			return zOrder_TwoPiece;
		default:
			return 0;
	}
}


void CharAnimations::AddPSTSuffix(char* ResRef, unsigned char StanceID,
	unsigned char& Cycle, unsigned char Orient)
{
	const char *Prefix;

	switch (StanceID) {
		case IE_ANI_ATTACK:
		case IE_ANI_ATTACK_SLASH:
		case IE_ANI_ATTACK_JAB:
		case IE_ANI_ATTACK_BACKSLASH:
			Cycle=SixteenToFive[Orient];
			Prefix="at1"; break;
		case IE_ANI_DAMAGE:
			Cycle=SixteenToFive[Orient];
			Prefix="hit"; break;
		case IE_ANI_GET_UP:
		case IE_ANI_EMERGE:
			Cycle=SixteenToFive[Orient];
			Prefix="gup"; break;
		case IE_ANI_AWAKE:
			Cycle=SixteenToFive[Orient];
			Prefix="std"; break;
		case IE_ANI_READY:
			Cycle=SixteenToFive[Orient];
			Prefix="stc"; break;
		case IE_ANI_DIE:
		case IE_ANI_SLEEP:
		case IE_ANI_TWITCH:
			Cycle=SixteenToFive[Orient];
			Prefix="dfb"; break;
		case IE_ANI_RUN:
			Cycle=SixteenToNine[Orient];
			Prefix="run"; break;
		case IE_ANI_WALK:
			Cycle=SixteenToNine[Orient];
			Prefix="wlk"; break;
		case IE_ANI_HEAD_TURN:
			Cycle=SixteenToFive[Orient];
			if (rand()&1) {
				Prefix="sf2";
				sprintf(ResRef,"%c%3s%4s",this->ResRef[0], Prefix, this->ResRef+1);
				if (gamedata->Exists(ResRef, IE_BAM_CLASS_ID) ) {
					return;
				}
			}
			Prefix="sf1";
			sprintf(ResRef,"%c%3s%4s",this->ResRef[0], Prefix, this->ResRef+1);
			if (gamedata->Exists(ResRef, IE_BAM_CLASS_ID) ) {
				return;
			}
			Prefix = "stc";
			break;
		case IE_ANI_PST_START:
			Cycle=0;
			Prefix="ms1"; break;
		default: //just in case
			Cycle=SixteenToFive[Orient];
			Prefix="stc"; break;
	}
	sprintf(ResRef,"%c%3s%4s",this->ResRef[0], Prefix, this->ResRef+1);
}

void CharAnimations::AddVHR2Suffix(char* ResRef, unsigned char StanceID,
	unsigned char& Cycle, unsigned char Orient)
{
	Cycle=SixteenToNine[Orient];

	switch (StanceID) {
		case IE_ANI_ATTACK: //temporarily
		case IE_ANI_ATTACK_BACKSLASH:
			strcat( ResRef, "g21" );
			break;

		case IE_ANI_ATTACK_SLASH:
			strcat( ResRef, "g2" );
			break;

		case IE_ANI_ATTACK_JAB:
			strcat( ResRef, "g26" );
			Cycle+=45;
			break;

		case IE_ANI_CAST: //looping
			strcat( ResRef, "g25" );
			Cycle+=45;
			break;

		case IE_ANI_CONJURE://ending
			strcat( ResRef, "g26" );
			Cycle+=54;
			break;

		case IE_ANI_SHOOT:
			strcat( ResRef, "g24" );
			Cycle+=27;
			break;

		case IE_ANI_HEAD_TURN:
		case IE_ANI_AWAKE:
			strcat( ResRef, "g12" );
			Cycle+=18;
			break;

		case IE_ANI_SLEEP:
			strcat( ResRef, "g15" );
			Cycle+=45;
			break;

		case IE_ANI_TWITCH:
			strcat( ResRef, "g14" );
			Cycle+=45;
			break;

		case IE_ANI_DIE:
		case IE_ANI_EMERGE:
		case IE_ANI_GET_UP:
		case IE_ANI_PST_START:
			strcat( ResRef, "g14" );
			Cycle+=36;
			break;

		case IE_ANI_DAMAGE:
			strcat( ResRef, "g13" );
			Cycle+=27;
			break;

		case IE_ANI_READY:
			strcat( ResRef, "g1" );
			Cycle+=9;
			break;

		case IE_ANI_WALK:
			strcat( ResRef, "g11" );
			break;

		case IE_ANI_HIDE:
			strcat( ResRef, "g22" );
			break;
		default:
			error("CharAnimation", "VHR2 Animation: unhandled stance: %s %d\n", ResRef, StanceID);
			break;
	}
}

void CharAnimations::AddVHR3Suffix(char* ResRef, unsigned char StanceID,
	unsigned char& Cycle, unsigned char Orient)
{
	Cycle=SixteenToNine[Orient];

	switch (StanceID) {
		case IE_ANI_ATTACK: //temporarily
		case IE_ANI_ATTACK_BACKSLASH:
			strcat( ResRef, "g21" );
			break;

		case IE_ANI_ATTACK_SLASH:
			strcat( ResRef, "g2" );
			break;

		case IE_ANI_ATTACK_JAB:
			strcat( ResRef, "g26" );
			Cycle+=18;
			break;

		case IE_ANI_CAST: //looping
			strcat( ResRef, "g25" );
			Cycle+=45;
			break;

		case IE_ANI_CONJURE://ending
			strcat( ResRef, "g26" );
			Cycle+=36;
			break;

		case IE_ANI_SHOOT:
			strcat( ResRef, "g24" );
			Cycle+=27;
			break;

		case IE_ANI_HEAD_TURN:
		case IE_ANI_AWAKE:
			strcat( ResRef, "g12" );
			Cycle+=18;
			break;

		case IE_ANI_SLEEP:
			strcat( ResRef, "g15" );
			Cycle+=45;
			break;

		case IE_ANI_TWITCH:
			strcat( ResRef, "g14" );
			Cycle+=45;
			break;

		case IE_ANI_DIE:
		case IE_ANI_EMERGE:
		case IE_ANI_GET_UP:
		case IE_ANI_PST_START:
			strcat( ResRef, "g14" );
			Cycle+=36;
			break;

		case IE_ANI_DAMAGE:
			strcat( ResRef, "g13" );
			Cycle+=27;
			break;

		case IE_ANI_READY:
			strcat( ResRef, "g1" );
			Cycle+=9;
			break;

		case IE_ANI_WALK:
			strcat( ResRef, "g11" );
			break;
		default:
			error("CharAnimation", "VHR3 Animation: unhandled stance: %s %d\n", ResRef, StanceID);
			break;
	}
}

// Note: almost like SixSuffix
void CharAnimations::AddFFSuffix(char* ResRef, unsigned char StanceID,
	unsigned char& Cycle, unsigned char Orient, int Part)
{
	Cycle=SixteenToNine[Orient];
	switch (StanceID) {
		case IE_ANI_WALK:
			strcat( ResRef, "g1" );
			break;

		case IE_ANI_ATTACK:
		case IE_ANI_ATTACK_SLASH:
			strcat( ResRef, "g3" );
			break;

		case IE_ANI_ATTACK_BACKSLASH:
			strcat( ResRef, "g3" );
			Cycle += 16;
			break;

		case IE_ANI_ATTACK_JAB:
		case IE_ANI_CAST:
		case IE_ANI_CONJURE:
			strcat( ResRef, "g3" );
			Cycle += 32;
			break;

		case IE_ANI_HEAD_TURN: //could be wrong
		case IE_ANI_AWAKE:
			strcat( ResRef, "g2" );
			break;

		case IE_ANI_READY:
			strcat( ResRef, "g2" );
			Cycle += 16;
			break;

		case IE_ANI_DAMAGE:
			strcat( ResRef, "g2" );
			Cycle += 32;
			break;

		case IE_ANI_DIE:
		case IE_ANI_GET_UP:
		case IE_ANI_EMERGE:
		case IE_ANI_PST_START:
			strcat( ResRef, "g2" );
			Cycle += 48;
			break;

		case IE_ANI_SLEEP:
		case IE_ANI_TWITCH:
			strcat( ResRef, "g2" );
			Cycle += 64;
			break;

		default:
			error("CharAnimation", "Four frames Animation: unhandled stance: %s %d\n", ResRef, StanceID);
			break;

	}
	ResRef[6]=(char) (Part+'1');
	ResRef[7]=0;
}

void CharAnimations::AddNFSuffix(char* ResRef, unsigned char StanceID,
	unsigned char& Cycle, unsigned char Orient, int Part)
{
	char prefix[10];

	Cycle = SixteenToNine[Orient];
	snprintf(prefix, 9, "%s%s%d%s%d", ResRef, StancePrefix[StanceID], Part+1,
			 CyclePrefix[StanceID], Cycle);
	strnlwrcpy(ResRef,prefix,8);
	Cycle=(ieByte) (Cycle+CycleOffset[StanceID]);
}

//Attack
//h1, h2, w2
//static const char *SlashPrefix[]={"a1","a4","a7"};
//static const char *BackPrefix[]={"a2","a5","a8"};
//static const char *JabPrefix[]={"a3","a6","a9"};
static const char *SlashPrefix[]={"a1","a2","a7"};
static const char *BackPrefix[]={"a3","a4","a8"};
static const char *JabPrefix[]={"a5","a6","a9"};
static const char *RangedPrefix[]={"sa","sx","ss"};
static const char *RangedPrefixOld[]={"sa","sx","a1"};

void CharAnimations::AddVHRSuffix(char* ResRef, unsigned char StanceID,
	unsigned char& Cycle, unsigned char Orient, EquipResRefData*& EquipData)
{
	Cycle = SixteenToNine[Orient];
	EquipData = new EquipResRefData;
	EquipData->Suffix[0] = 0;
	switch (StanceID) {
		case IE_ANI_ATTACK:
		case IE_ANI_ATTACK_SLASH:
			strcat( ResRef, SlashPrefix[WeaponType] );
			strcpy( EquipData->Suffix, SlashPrefix[WeaponType] );
			break;

		case IE_ANI_ATTACK_BACKSLASH:
			strcat( ResRef, BackPrefix[WeaponType] );
			strcpy( EquipData->Suffix, BackPrefix[WeaponType] );
			break;

		case IE_ANI_ATTACK_JAB:
			strcat( ResRef, JabPrefix[WeaponType] );
			strcpy( EquipData->Suffix, JabPrefix[WeaponType] );
			break;

		case IE_ANI_AWAKE:
			strcat( ResRef, "g17" );
			strcpy( EquipData->Suffix, "g1" );
			Cycle += 63;
			break;

		case IE_ANI_CAST: //looping
			strcat( ResRef, "ca" );
			strcpy( EquipData->Suffix, "ca" );
			break;

		case IE_ANI_CONJURE: //ending
			strcat( ResRef, "ca" );
			strcpy( EquipData->Suffix, "ca" );
			Cycle += 9;
			break;

		case IE_ANI_DAMAGE:
			strcat( ResRef, "g14" );
			strcpy( EquipData->Suffix, "g1" );
			Cycle += 36;
			break;

		case IE_ANI_DIE:
			strcat( ResRef, "g15" );
			strcpy( EquipData->Suffix, "g1" );
			Cycle += 45;
			break;
			//I cannot find an emerge animation...
			//Maybe is Die reversed
		case IE_ANI_GET_UP:
		case IE_ANI_EMERGE:
		case IE_ANI_PST_START:
			strcat( ResRef, "g19" );
			strcpy( EquipData->Suffix, "g1" );
			Cycle += 81;
			break;

		case IE_ANI_HEAD_TURN:
			if (rand()&1) {
				strcat( ResRef, "g12" );
				Cycle += 18;
			} else {
				strcat( ResRef, "g18" );
				Cycle += 72;
			}
			strcpy( EquipData->Suffix, "g1" );
			break;

			//Unknown... maybe only a transparency effect apply
		case IE_ANI_HIDE:
			break;

		case IE_ANI_READY:
			if ( WeaponType == IE_ANI_WEAPON_2H ) {
				strcat( ResRef, "g13" );
				Cycle += 27;
			} else {
				strcat( ResRef, "g1" );
				Cycle += 9;
			}
			strcpy( EquipData->Suffix, "g1" );
			break;
			//This depends on the ranged weapon equipped
		case IE_ANI_SHOOT:
			strcat( ResRef, RangedPrefix[RangedType] );
			strcpy( EquipData->Suffix, RangedPrefix[RangedType] );
			break;

		case IE_ANI_SLEEP:
			strcat( ResRef, "g16" );
			strcpy( EquipData->Suffix, "g1" );
			Cycle += 54;
			break;

		case IE_ANI_TWITCH:
			strcat( ResRef, "g16" );
			strcpy( EquipData->Suffix, "g1" );
			Cycle += 54;
			break;

		case IE_ANI_WALK:
			strcat( ResRef, "g11" );
			strcpy( EquipData->Suffix, "g1" );
			break;

		default:
			error("CharAnimation", "VHR Animation: unhandled stance: %s %d\n", ResRef, StanceID);
			break;
	}
	EquipData->Cycle = Cycle;
}

void CharAnimations::GetVHREquipmentRef(char* ResRef, unsigned char& Cycle,
			const char* equipRef, bool offhand,
			EquipResRefData* equip)
{
	Cycle = equip->Cycle;
	if (offhand) {
		sprintf( ResRef, "wq%c%c%co%s", GetSize(), equipRef[0], equipRef[1], equip->Suffix );
	} else {
		sprintf( ResRef, "wq%c%c%c%s", GetSize(), equipRef[0], equipRef[1], equip->Suffix );
	}
}

void CharAnimations::AddSixSuffix(char* ResRef, unsigned char StanceID,
	unsigned char& Cycle, unsigned char Orient)
{
	switch (StanceID) {
		case IE_ANI_WALK:
			strcat( ResRef, "g1" );
			Cycle = Orient;
			break;

		case IE_ANI_ATTACK:
		case IE_ANI_ATTACK_SLASH:
			strcat( ResRef, "g3" );
			Cycle = Orient;
			break;

		case IE_ANI_ATTACK_BACKSLASH:
			strcat( ResRef, "g3" );
			Cycle = 16 + Orient;
			break;

		case IE_ANI_ATTACK_JAB:
			strcat( ResRef, "g3" );
			Cycle = 32 + Orient;
			break;

		case IE_ANI_HEAD_TURN: //could be wrong
		case IE_ANI_AWAKE:
			strcat( ResRef, "g2" );
			Cycle = 0 + Orient;
			break;

		case IE_ANI_READY:
			strcat( ResRef, "g2" );
			Cycle = 16 + Orient;
			break;

		case IE_ANI_DAMAGE:
			strcat( ResRef, "g2" );
			Cycle = 32 + Orient;
			break;

		case IE_ANI_DIE:
		case IE_ANI_GET_UP:
		case IE_ANI_EMERGE:
		case IE_ANI_PST_START:
			strcat( ResRef, "g2" );
			Cycle = 48 + Orient;
			break;

		case IE_ANI_TWITCH:
		case IE_ANI_SLEEP:
			strcat( ResRef, "g2" );
			Cycle = 64 + Orient;
			break;

		default:
			error("CharAnimation", "Six Animation: unhandled stance: %s %d\n", ResRef, StanceID);
			break;

	}
	if (Orient>9) {
		strcat( ResRef, "e" );
	}
}

void CharAnimations::AddLR2Suffix(char* ResRef, unsigned char StanceID,
	unsigned char& Cycle, unsigned char Orient)
{
	Orient /= 2;

	switch (StanceID) {
		case IE_ANI_READY:
		case IE_ANI_CAST: //looping
		case IE_ANI_CONJURE://ending
		case IE_ANI_HIDE:
		case IE_ANI_WALK:
		case IE_ANI_AWAKE:
			Cycle = 0 + Orient;
			break;

		case IE_ANI_SHOOT:
		case IE_ANI_ATTACK:
		case IE_ANI_ATTACK_SLASH:
		case IE_ANI_ATTACK_BACKSLASH:
		case IE_ANI_ATTACK_JAB:
		case IE_ANI_HEAD_TURN:
			Cycle = 8 + Orient;
			break;

		case IE_ANI_DIE:
		case IE_ANI_GET_UP:
		case IE_ANI_EMERGE:
		case IE_ANI_PST_START:
			Cycle = 24 + Orient;
			break;

		case IE_ANI_DAMAGE:
			Cycle = 16 + Orient;
			break;

		case IE_ANI_SLEEP:
		case IE_ANI_TWITCH:
			Cycle = 32 + Orient;
			break;
		default:
			error("CharAnimation", "LR2 Animation: unhandled stance: %s %d\n", ResRef, StanceID);
			break;
	}
	if (Orient>=4) {
		strcat( ResRef, "g1e" );
	} else {
		strcat( ResRef, "g1" );
	}
}

void CharAnimations::AddMHRSuffix(char* ResRef, unsigned char StanceID,
	unsigned char& Cycle, unsigned char Orient, EquipResRefData*& EquipData)
{
	Orient /= 2;
	EquipData = new EquipResRefData;
	EquipData->Suffix[0] = 0;

	switch (StanceID) {
		case IE_ANI_ATTACK:
		case IE_ANI_ATTACK_SLASH:
			strcat (ResRef, SlashPrefix[WeaponType]);
			strcpy( EquipData->Suffix, SlashPrefix[WeaponType] );
			Cycle = Orient;
			break;

		case IE_ANI_ATTACK_BACKSLASH:
			strcat (ResRef, BackPrefix[WeaponType]);
			strcpy( EquipData->Suffix, BackPrefix[WeaponType] );
			Cycle = Orient;
			break;

		case IE_ANI_ATTACK_JAB:
			strcat (ResRef, JabPrefix[WeaponType]);
			strcpy( EquipData->Suffix, JabPrefix[WeaponType] );
			Cycle = Orient;
			break;

		case IE_ANI_READY:
			strcat( ResRef, "g1" );
			strcpy( EquipData->Suffix, "g1" );
			if ( WeaponType == IE_ANI_WEAPON_2W ) {
				Cycle = 24 + Orient;
			} else {
				Cycle = 8 + Orient;
			}
			break;

		case IE_ANI_CAST://looping
			strcat( ResRef, "ca" );
			strcpy( EquipData->Suffix, "ca" );
			Cycle = 8 + Orient;
			break;

		case IE_ANI_CONJURE://ending
			strcat( ResRef, "ca" );
			strcpy( EquipData->Suffix, "ca" );
			Cycle = Orient;
			break;

		case IE_ANI_DAMAGE:
			strcat( ResRef, "g1" );
			strcpy( EquipData->Suffix, "g1" );
			Cycle = 40 + Orient;
			break;

		case IE_ANI_DIE:
		case IE_ANI_GET_UP:
		case IE_ANI_PST_START:
			strcat( ResRef, "g1" );
			strcpy( EquipData->Suffix, "g1" );
			Cycle = 48 + Orient;
			break;

			//I cannot find an emerge animation...
			//Maybe is Die reversed
		case IE_ANI_EMERGE:
			strcat( ResRef, "g1" );
			strcpy( EquipData->Suffix, "g1" );
			Cycle = 48 + Orient;
			break;

		case IE_ANI_HEAD_TURN:
			strcat( ResRef, "g1" );
			strcpy( EquipData->Suffix, "g1" );
			Cycle = 32 + Orient;
			break;

			//Unknown... maybe only a transparency effect apply
		case IE_ANI_HIDE:
			break;

		case IE_ANI_AWAKE:
			strcat( ResRef, "g1" );
			strcpy( EquipData->Suffix, "g1" );
			Cycle = 16 + Orient;
			break;

			//This depends on the ranged weapon equipped
		case IE_ANI_SHOOT:
			strcat (ResRef, RangedPrefixOld[RangedType]);
			strcpy( EquipData->Suffix, RangedPrefixOld[RangedType] );
			Cycle = Orient;
			break;

		case IE_ANI_SLEEP:
			strcat( ResRef, "g1" );
			strcpy( EquipData->Suffix, "g1" );
			Cycle = 64 + Orient;
			break;

		case IE_ANI_TWITCH:
			strcat( ResRef, "g1" );
			strcpy( EquipData->Suffix, "g1" );
			Cycle = 56 + Orient;
			break;

		case IE_ANI_WALK:
			strcat( ResRef, "g1" );
			strcpy( EquipData->Suffix, "g1" );
			Cycle = Orient;
			break;
		default:
			error("CharAnimation", "MHR Animation: unhandled stance: %s %d\n", ResRef, StanceID);
			break;
	}
	if (Orient>=5) {
		strcat( ResRef, "e" );
		strcat( EquipData->Suffix, "e" );
	}
	EquipData->Cycle = Cycle;
}

void CharAnimations::GetMHREquipmentRef(char* ResRef, unsigned char& Cycle,
			const char* equipRef, bool offhand,
			EquipResRefData* equip)
{
	Cycle = equip->Cycle;
	if (offhand) {
		//i think there is no offhand stuff for bg1, lets use the bg2 equivalent here?
		sprintf( ResRef, "wq%c%c%co%s", GetSize(), equipRef[0], equipRef[1], equip->Suffix );
	} else {
		sprintf( ResRef, "wp%c%c%c%s", GetSize(), equipRef[0], equipRef[1], equip->Suffix );
	}
}

void CharAnimations::AddTwoFileSuffix( char* ResRef, unsigned char StanceID,
	unsigned char& Cycle, unsigned char Orient)
{
	switch(StanceID) {
		case IE_ANI_HEAD_TURN:
			Cycle = 16 + Orient / 2;
			break;
		case IE_ANI_DAMAGE:
			Cycle = 24 + Orient / 2;
			break;
		case IE_ANI_SLEEP:
		case IE_ANI_TWITCH:
			Cycle = 40 + Orient / 2;
			break;
		case IE_ANI_GET_UP:
		case IE_ANI_EMERGE:
		case IE_ANI_DIE:
		case IE_ANI_PST_START:
			Cycle = 32 + Orient / 2;
			break;
		case IE_ANI_WALK:
			Cycle = Orient / 2;
			break;
		default:
			Cycle = 8 + Orient / 2;
			break;
	}
	strcat( ResRef, "g1" );
	if (Orient > 9) {
		strcat( ResRef, "e" );
	}
}

void CharAnimations::AddLRSuffix2( char* ResRef, unsigned char StanceID,
	unsigned char& Cycle, unsigned char Orient, EquipResRefData *&EquipData)
{
	EquipData = new EquipResRefData;
	EquipData->Suffix[0] = 0;
	switch (StanceID) {
		case IE_ANI_ATTACK:
		case IE_ANI_ATTACK_BACKSLASH:
		case IE_ANI_ATTACK_SLASH:
		case IE_ANI_ATTACK_JAB:
			strcat( ResRef, "g2" );
			strcpy( EquipData->Suffix, "g2" );
			Cycle = Orient / 2;
			break;
		case IE_ANI_CAST:
		case IE_ANI_CONJURE:
		case IE_ANI_SHOOT:
			strcat( ResRef, "g2" );
			strcpy( EquipData->Suffix, "g2" );
			Cycle = 8 + Orient / 2;
			break;
		case IE_ANI_WALK:
			strcat( ResRef, "g1" );
			strcpy( EquipData->Suffix, "g1" );
			Cycle = Orient / 2;
			break;
		case IE_ANI_READY:
			strcat( ResRef, "g1" );
			strcpy( EquipData->Suffix, "g1" );
			Cycle = 8 + Orient / 2;
			break;
		case IE_ANI_HEAD_TURN: //could be wrong
		case IE_ANI_AWAKE:
			strcat( ResRef, "g1" );
			strcpy( EquipData->Suffix, "g1" );
			Cycle = 16 + Orient / 2;
			break;
		case IE_ANI_DAMAGE:
			strcat( ResRef, "g1" );
			strcpy( EquipData->Suffix, "g1" );
			Cycle = 24 + Orient / 2;
			break;
		case IE_ANI_GET_UP:
		case IE_ANI_EMERGE:
		case IE_ANI_PST_START:
		case IE_ANI_DIE:
			strcat( ResRef, "g1" );
			strcpy( EquipData->Suffix, "g1" );
			Cycle = 32 + Orient / 2;
			break;
		case IE_ANI_SLEEP:
		case IE_ANI_HIDE:
		case IE_ANI_TWITCH:
			strcat( ResRef, "g1" );
			strcpy( EquipData->Suffix, "g1" );
			Cycle = 40 + Orient / 2;
			break;
		default:
			error("CharAnimation", "LRSuffix2 Animation: unhandled stance: %s %d\n", ResRef, StanceID);
			break;
	}
	if (Orient > 9) {
		strcat( ResRef, "e" );
		strcat( EquipData->Suffix, "e");
	}
	EquipData->Cycle = Cycle;
}

void CharAnimations::AddTwoPieceSuffix(char* ResRef, unsigned char StanceID,
	unsigned char& Cycle, unsigned char Orient, int Part)
{
	if (Part == 1) {
		strcat( ResRef, "d" );
	}

	switch (StanceID) {
		case IE_ANI_DIE:
			strcat( ResRef, "g1" );
			Cycle = 8 + Orient / 2;
			break;
		case IE_ANI_TWITCH:
		case IE_ANI_SLEEP:
			strcat( ResRef, "g1" );
			Cycle = 16 + Orient / 2;
			break;
		case IE_ANI_READY:
		case IE_ANI_HEAD_TURN:
		case IE_ANI_AWAKE:
		case IE_ANI_DAMAGE:
			strcat( ResRef, "g1" );
			Cycle = 24 + Orient / 2;
			break;
		case IE_ANI_WALK:
			strcat( ResRef, "g2" );
			Cycle = Orient / 2;
			break;
		case IE_ANI_GET_UP:
		case IE_ANI_EMERGE:
			strcat( ResRef, "g2" );
			Cycle = 8 + Orient / 2;
			break;
		case IE_ANI_HIDE:
			strcat( ResRef, "g2" );
			Cycle = 16 + Orient / 2;
			break;
		case IE_ANI_ATTACK:
		case IE_ANI_ATTACK_BACKSLASH:
			strcat( ResRef, "g3" );
			Cycle = Orient / 2;
			break;
		case IE_ANI_ATTACK_SLASH:
		case IE_ANI_ATTACK_JAB:
		case IE_ANI_CAST:
		case IE_ANI_CONJURE:
		case IE_ANI_SHOOT:
			strcat( ResRef, "g3" );
			Cycle = 8 + Orient / 2;
			break;
		default:
			error("CharAnimation", "Two-piece Animation: unhandled stance: %s %d", ResRef, StanceID);
			break;
	}
	if (Orient > 9) {
		strcat( ResRef, "e" );
	}
}

void CharAnimations::AddLRSuffix( char* ResRef, unsigned char StanceID,
	unsigned char& Cycle, unsigned char Orient, EquipResRefData *&EquipData)
{
	EquipData = new EquipResRefData;
	EquipData->Suffix[0] = 0;
	switch (StanceID) {
		case IE_ANI_ATTACK:
		case IE_ANI_ATTACK_BACKSLASH:
			strcat( ResRef, "g2" );
			strcpy( EquipData->Suffix, "g2" );
			Cycle = Orient / 2;
			break;
		case IE_ANI_ATTACK_SLASH:
		case IE_ANI_CAST:
		case IE_ANI_CONJURE:
			strcat( ResRef, "g2" );
			strcpy( EquipData->Suffix, "g2" );
			Cycle = 8 + Orient / 2;
			break;
		case IE_ANI_ATTACK_JAB:
		case IE_ANI_SHOOT:
			strcat( ResRef, "g2" );
			strcpy( EquipData->Suffix, "g2" );
			Cycle = 16 + Orient / 2;
			break;
		case IE_ANI_WALK:
			strcat( ResRef, "g1" );
			strcpy( EquipData->Suffix, "g1" );
			Cycle = Orient / 2;
			break;
		case IE_ANI_READY:
			strcat( ResRef, "g1" );
			strcpy( EquipData->Suffix, "g1" );
			Cycle = 8 + Orient / 2;
			break;
		case IE_ANI_HEAD_TURN: //could be wrong
		case IE_ANI_AWAKE:
			strcat( ResRef, "g1" );
			strcpy( EquipData->Suffix, "g1" );
			Cycle = 16 + Orient / 2;
			break;
		case IE_ANI_DAMAGE:
			strcat( ResRef, "g1" );
			strcpy( EquipData->Suffix, "g1" );
			Cycle = 24 + Orient / 2;
			break;
		case IE_ANI_GET_UP:
		case IE_ANI_EMERGE:
		case IE_ANI_PST_START:
		case IE_ANI_DIE:
			strcat( ResRef, "g1" );
			strcpy( EquipData->Suffix, "g1" );
			Cycle = 32 + Orient / 2;
			break;
		case IE_ANI_TWITCH:
		case IE_ANI_SLEEP:
			strcat( ResRef, "g1" );
			strcpy( EquipData->Suffix, "g1" );
			Cycle = 40 + Orient / 2;
			break;
		default:
			error("CharAnimation", "LR Animation: unhandled stance: %s %d\n", ResRef, StanceID);
			break;
	}
	if (Orient > 9) {
		strcat( ResRef, "e" );
		strcat( EquipData->Suffix, "e");
	}
	EquipData->Cycle = Cycle;
}

void CharAnimations::GetLREquipmentRef(char* ResRef, unsigned char& Cycle,
			const char* equipRef, bool /*offhand*/,
			EquipResRefData* equip)
{
	Cycle = equip->Cycle;
	//hackhackhack
	sprintf( ResRef, "%4s%c%s", this->ResRef, equipRef[0], equip->Suffix );
}

//Only for the ogre animation (MOGR)
void CharAnimations::AddLR3Suffix( char* ResRef, unsigned char StanceID,
	unsigned char& Cycle, unsigned char Orient)
{
	switch (StanceID) {
		case IE_ANI_ATTACK:
		case IE_ANI_ATTACK_BACKSLASH:
			strcat( ResRef, "g2" );
			Cycle = Orient / 2;
			break;
		case IE_ANI_ATTACK_SLASH:
			strcat( ResRef, "g2" );
			Cycle = 8 + Orient / 2;
			break;
		case IE_ANI_ATTACK_JAB:
			strcat( ResRef, "g2" );
			Cycle = 8 + Orient / 2;  //there is no third attack animation
			break;
		case IE_ANI_CAST:
		case IE_ANI_CONJURE:
		case IE_ANI_SHOOT:
			strcat( ResRef, "g3" );
			Cycle = Orient / 2;
			break;
		case IE_ANI_WALK:
			strcat( ResRef, "g1" );
			Cycle = 16 + Orient / 2;
			break;
		case IE_ANI_READY:
			strcat( ResRef, "g1" );
			Cycle = 8 + Orient / 2;
			break;
		case IE_ANI_HEAD_TURN: //could be wrong
		case IE_ANI_AWAKE:
			strcat( ResRef, "g1" );
			Cycle = Orient / 2;
			break;
		case IE_ANI_DAMAGE:
			strcat( ResRef, "g3" );
			Cycle = 8 + Orient / 2;
			break;
		case IE_ANI_DIE:
		case IE_ANI_GET_UP:
		case IE_ANI_EMERGE:
		case IE_ANI_PST_START:
		case IE_ANI_SLEEP:
			strcat( ResRef, "g3" );
			Cycle = 16 + Orient / 2;
			break;
		case IE_ANI_TWITCH:
			strcat( ResRef, "g3" );
			Cycle = 24 + Orient / 2;
			break;
		default:
			error("CharAnimation", "LR3 Animation: unhandled stance: %s %d\n", ResRef, StanceID);
			break;
	}
	if (Orient > 9) {
		strcat( ResRef, "e" );
	}
}

void CharAnimations::AddMMR2Suffix(char* ResRef, unsigned char StanceID,
	unsigned char& Cycle, unsigned char Orient)
{
	switch (StanceID) {
		case IE_ANI_ATTACK:
		case IE_ANI_ATTACK_SLASH:
		case IE_ANI_ATTACK_BACKSLASH:
		case IE_ANI_ATTACK_JAB:
		case IE_ANI_CONJURE:
		case IE_ANI_CAST:
			strcat( ResRef, "a1" );
			Cycle = ( Orient / 2 );
			break;

		case IE_ANI_SHOOT:
			strcat( ResRef, "a4" );
			Cycle = ( Orient / 2 );
			break;

		case IE_ANI_AWAKE:
		case IE_ANI_READY:
			strcat( ResRef, "sd" );
			Cycle = ( Orient / 2 );
			break;

		case IE_ANI_HEAD_TURN:
			strcat( ResRef, "sc" );
			Cycle = ( Orient / 2 );
			break;

		case IE_ANI_DAMAGE:
			strcat( ResRef, "gh" );
			Cycle = ( Orient / 2 );
			break;

		case IE_ANI_DIE:
			strcat( ResRef, "de" );
			Cycle = ( Orient / 2 );
			break;

		case IE_ANI_GET_UP:
		case IE_ANI_EMERGE:
		case IE_ANI_PST_START:
			strcat( ResRef, "gu" );
			Cycle = ( Orient / 2 );
			break;

			//Unknown... maybe only a transparency effect apply
		case IE_ANI_HIDE:
			break;

		case IE_ANI_SLEEP:
			strcat( ResRef, "sl" );
			Cycle = ( Orient / 2 );
			break;

		case IE_ANI_TWITCH:
			strcat( ResRef, "tw" );
			Cycle = ( Orient / 2 );
			break;

		case IE_ANI_WALK:
			strcat( ResRef, "wk" );
			Cycle = ( Orient / 2 );
			break;
		default:
			error("CharAnimation", "MMR Animation: unhandled stance: %s %d\n", ResRef, StanceID);
			break;
	}
	if (Orient > 9) {
		strcat( ResRef, "e" );
	}
}

void CharAnimations::AddMMRSuffix(char* ResRef, unsigned char StanceID,
	unsigned char& Cycle, unsigned char Orient)
{
	switch (StanceID) {
		case IE_ANI_ATTACK:
		case IE_ANI_ATTACK_SLASH:
		case IE_ANI_ATTACK_BACKSLASH:
			strcat( ResRef, "a1" );
			Cycle = ( Orient / 2 );
			break;

		case IE_ANI_SHOOT:
			strcat( ResRef, "a4" );
			Cycle = ( Orient / 2 );
			break;

		case IE_ANI_ATTACK_JAB:
			strcat( ResRef, "a2" );
			Cycle = ( Orient / 2 );
			break;

		case IE_ANI_AWAKE:
		case IE_ANI_READY:
			strcat( ResRef, "sd" );
			Cycle = ( Orient / 2 );
			break;

		case IE_ANI_CONJURE:
			strcat( ResRef, "ca" );
			Cycle = ( Orient / 2 );
			break;

		case IE_ANI_CAST:
			strcat( ResRef, "sp" );
			Cycle = ( Orient / 2 );
			break;

		case IE_ANI_HEAD_TURN:
			strcat( ResRef, "sc" );
			Cycle = ( Orient / 2 );
			break;

		case IE_ANI_DAMAGE:
			strcat( ResRef, "gh" );
			Cycle = ( Orient / 2 );
			break;

		case IE_ANI_DIE:
			strcat( ResRef, "de" );
			Cycle = ( Orient / 2 );
			break;

		case IE_ANI_GET_UP:
		case IE_ANI_EMERGE:
		case IE_ANI_PST_START:
			strcat( ResRef, "gu" );
			Cycle = ( Orient / 2 );
			break;

			//Unknown... maybe only a transparency effect apply
		case IE_ANI_HIDE:
			break;

		case IE_ANI_SLEEP:
			strcat( ResRef, "sl" );
			Cycle = ( Orient / 2 );
			break;

		case IE_ANI_TWITCH:
			strcat( ResRef, "tw" );
			Cycle = ( Orient / 2 );
			break;

		case IE_ANI_WALK:
			strcat( ResRef, "wk" );
			Cycle = ( Orient / 2 );
			break;
		default:
			error("CharAnimation", "MMR Animation: unhandled stance: %s %d\n", ResRef, StanceID);
			break;
	}
	if (Orient > 9) {
		strcat( ResRef, "e" );
	}
}

void CharAnimations::AddHLSuffix(char* ResRef, unsigned char StanceID,
	unsigned char& Cycle, unsigned char Orient)
{
	//even orientations in 'h', odd in 'l', and since the WALK animation
	//with fewer orientations is first in h, all other stances in that
	//file need to be offset by those cycles
	int offset = ((Orient % 2)^1) * 8;

	switch (StanceID) {

		case IE_ANI_WALK:
			//only available in 8 orientations instead of the usual 16
			Cycle = 0 + Orient / 2;
			offset = 1;
			break;

		case IE_ANI_HEAD_TURN:
			Cycle = offset + Orient / 2;
			break;

		case IE_ANI_AWAKE:
		case IE_ANI_READY:
		//the following are not available
		case IE_ANI_CAST:
		case IE_ANI_CONJURE:
		case IE_ANI_HIDE:
		case IE_ANI_SHOOT:
		case IE_ANI_ATTACK:
		case IE_ANI_ATTACK_SLASH:
		case IE_ANI_ATTACK_BACKSLASH:
		case IE_ANI_ATTACK_JAB:
			Cycle = 8 + offset + Orient / 2;
			break;

		case IE_ANI_DAMAGE:
			Cycle = 16 + offset + Orient / 2;
			break;

		case IE_ANI_DIE:
		case IE_ANI_GET_UP:
		case IE_ANI_EMERGE:
			Cycle = 24 + offset + Orient / 2;
			break;

		case IE_ANI_SLEEP:
		case IE_ANI_TWITCH:
			Cycle = 32 + offset + Orient / 2;
			break;

		default:
			error("CharAnimation", "HL Animation: unhandled stance: %s %d", ResRef, StanceID);
			break;
	}
	strcat(ResRef, offset ? "hg1" : "lg1");
	if (Orient > 9) {
		strcat(ResRef, "e");
	}
}

void CharAnimations::PulseRGBModifiers()
{
	unsigned long time = core->GetGame()->Ticks;

	if (time - lastModUpdate <= 40)
		return;

	if (time - lastModUpdate > 400) lastModUpdate = time - 40;

	int inc = (time - lastModUpdate)/40;
	
	if (GlobalColorMod.type != RGBModifier::NONE &&
		GlobalColorMod.speed > 0)
	{
		GlobalColorMod.phase += inc;
		change[0] = change[1] = change[2] = change[3] = true;

		// reset if done
		if (GlobalColorMod.phase > 2*GlobalColorMod.speed) {
			GlobalColorMod.type = RGBModifier::NONE;
			GlobalColorMod.phase = 0;
			GlobalColorMod.speed = 0;
			GlobalColorMod.locked = false;
		}
	}

	for (int i = 0; i < 32; ++i) {
		if (ColorMods[i].type != RGBModifier::NONE &&
			ColorMods[i].speed > 0)
		{
			ColorMods[i].phase += inc;
			change[i>>3] = true;
			if (ColorMods[i].phase > 2*ColorMods[i].speed) {
				ColorMods[i].type = RGBModifier::NONE;
				ColorMods[i].phase = 0;
				ColorMods[i].speed = 0;
				ColorMods[i].locked = false;
			}
		}
	}

	if (change[0]) {
		change[0]=0;
		SetupColors(PAL_MAIN);
	}
	if (change[1]) {
		change[1]=0;
		SetupColors(PAL_WEAPON);
	}
	if (change[2]) {
		change[2]=0;
		SetupColors(PAL_OFFHAND);
	}
	if (change[3]) {
		change[3]=0;
		SetupColors(PAL_HELMET);
	}

	lastModUpdate += inc*40;
}

void CharAnimations::DebugDump()
{
	Log (DEBUG, "CharAnimations", "Anim ID   : %04x", GetAnimationID() );
	Log (DEBUG, "CharAnimations", "BloodColor: %d", GetBloodColor() );
	Log (DEBUG, "CharAnimations", "Flags     : %04x", GetFlags() );
}

}
