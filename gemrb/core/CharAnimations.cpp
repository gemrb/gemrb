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

#include "AnimationFactory.h"
#include "DataFileMgr.h"
#include "Game.h"
#include "GameData.h"
#include "ImageMgr.h"
#include "Interface.h"
#include "Map.h"
#include "Palette.h"
#include "RNG.h"

namespace GemRB {

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

CharAnimations::AvatarTableLoader::AvatarTableLoader() noexcept {
	AutoTable Avatars = gamedata->LoadTable("avatars");
	if (!Avatars) {
		error("CharAnimations", "A critical animation file is missing!\n");
	}
	int AvatarsCount = Avatars->GetRowCount();
	table.resize(AvatarsCount);
	const DataFileMgr *resdata = core->GetResDataINI();
	for (int i = AvatarsCount - 1; i >= 0; i--) {
		table[i].AnimID = strtounsigned<unsigned int>(Avatars->GetRowName(i));
		table[i].Prefixes[0] = Avatars->QueryField(i, AV_PREFIX1);
		table[i].Prefixes[1] = Avatars->QueryField(i, AV_PREFIX2);
		table[i].Prefixes[2] = Avatars->QueryField(i, AV_PREFIX3);
		table[i].Prefixes[3] = Avatars->QueryField(i, AV_PREFIX4);
		table[i].AnimationType=(ieByte) atoi(Avatars->QueryField(i,AV_ANIMTYPE) );
		table[i].CircleSize=(ieByte) atoi(Avatars->QueryField(i,AV_CIRCLESIZE) );
		const char *tmp = Avatars->QueryField(i,AV_USE_PALETTE);
		//QueryField will always return a zero terminated string
		//so tmp[0] must exist
		if ( isalpha (tmp[0]) ) {
			//this is a hack, we store 2 letters on an integer
			//it was allocated with calloc, so don't bother erasing it
			strncpy( (char *) &table[i].PaletteType, tmp, 3);
		}
		else {
			table[i].PaletteType=atoi(Avatars->QueryField(i,AV_USE_PALETTE) );
		}
		char size = Avatars->QueryField(i,AV_SIZE)[0];
		if (size == '*') {
			size = 0;
		}
		table[i].Size = size;

		table[i].WalkScale = 0;
		table[i].RunScale = 0;
		table[i].Bestiary = -1;
		
		for (unsigned char j = 0; j < MAX_ANIMS; j++) {
			table[i].StanceOverride[j] = j;
		}

		if (resdata) {
			char section[12];
			snprintf(section, 10, "%d", i%100000); // the mod is just to silent warnings, since we know i is small enough

			if (!resdata->GetKeysCount(section)) continue;

			float walkscale = resdata->GetKeyAsFloat(section, "walkscale", 0.0f);
			if (walkscale != 0.0f) table[i].WalkScale = (int)(1000.0f / walkscale);
			float runscale = resdata->GetKeyAsFloat(section, "runscale", 0.0f);
			if (runscale != 0.0f) table[i].RunScale = (int)(1000.0f / runscale);
			table[i].Bestiary = resdata->GetKeyAsInt(section, "bestiary", -1);
		}
	}
	
	std::sort(table.begin(), table.end(),[](const AvatarStruct &a, const AvatarStruct &b) {
		return a.AnimID < b.AnimID;
	});

	AutoTable blood = gamedata->LoadTable("bloodclr");
	if (blood) {
		int rows = blood->GetRowCount();
		for(int i=0;i<rows;i++) {
			char value = 0;
			unsigned int flags = 0;
			unsigned int rmin = 0;
			unsigned int rmax = 0xffff;

			valid_signednumber(blood->QueryField(i,0), value);
			valid_unsignednumber(blood->QueryField(i,1), rmin);
			valid_unsignednumber(blood->QueryField(i,2), rmax);
			valid_unsignednumber(blood->QueryField(i,3), flags);
			if (rmin > rmax || rmax > 0xffff) {
				Log(ERROR, "CharAnimations", "Invalid bloodclr entry: %02x %04x-%04x ", value, rmin, rmax);
				continue;
			}
			for (auto& row : table) {
				if (rmax < row.AnimID) break;
				if (rmin > row.AnimID) continue;
				row.BloodColor = value;
				row.Flags = flags;
			}
		}
	}

	AutoTable walk = gamedata->LoadTable("walksnd");
	if (walk) {
		int rows = walk->GetRowCount();
		for(int i=0;i<rows;i++) {
			ResRef value;
			unsigned int rmin = 0;
			unsigned int rmax = 0xffff;
			ieByte range = 0;

			value = walk->QueryField(i, 0);
			valid_unsignednumber(walk->QueryField(i,1), rmin);
			valid_unsignednumber(walk->QueryField(i,2), rmax);
			valid_unsignednumber(walk->QueryField(i,3), range);
			if (IsStar(value)) {
				value.Reset();
				range = 0;
			}
			if (rmin > rmax || rmax > 0xffff) {
				Log(ERROR, "CharAnimations", "Invalid walksnd entry: %02x %04x-%04x ", range, rmin, rmax);
				continue;
			}
			for (auto& row : table) {
				if (rmax < row.AnimID) break;
				if (rmin > row.AnimID) continue;
				row.WalkSound = value;
				row.WalkSoundCount = range;
			}
		}
	}

	AutoTable stances = gamedata->LoadTable("stances", true);
	if (stances) {
		int rows = stances->GetRowCount();
		for (int i = 0; i < rows; i++) {
			unsigned int id = 0;
			unsigned int s1 = 0;
			unsigned int s2 = 0;
			valid_unsignednumber(stances->GetRowName(i), id);
			valid_unsignednumber(stances->QueryField(i, 0), s1);
			valid_unsignednumber(stances->QueryField(i, 1), s2);

			if (s1 >= MAX_ANIMS || s2 >= MAX_ANIMS) {
				Log(ERROR, "CharAnimations", "Invalid stances entry: %04x %d %d", id, s1, s2);
				continue;
			}

			for (auto& row : table) {
				if (id < row.AnimID) break;
				if (id == row.AnimID) {
					row.StanceOverride[s1] = static_cast<unsigned char>(s2);
					break;
				}
			}
		}
	}

	AutoTable avatarShadows = gamedata->LoadTable("avatar_shadows");
	if (avatarShadows) {
		int rows = avatarShadows->GetRowCount();
		for (int i = 0; i < rows; ++i) {
			unsigned int id = 0;
			valid_unsignednumber(avatarShadows->GetRowName(i), id);

			for (auto& row : table) {
				if (id < row.AnimID) break;
				if (id == row.AnimID) {
					row.ShadowAnimation = avatarShadows->QueryField(i, 0);
					break;
				}
			}
		}
	}
}

size_t CharAnimations::GetAvatarsCount()
{
	return AvatarTableLoader::Get().size();
}

const AvatarStruct &CharAnimations::GetAvatarStruct(size_t RowNum)
{
	return AvatarTableLoader::Get()[RowNum];
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

char CharAnimations::GetSize() const
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
/*
 * For some actors (Arundel, FFG, fire giants) a new stance requires a
 * different palette to use. Presumably, this is relevant for PAL_MAIN only.
 */
void CharAnimations::MaybeUpdateMainPalette(Animation **anims) {
	if (previousStanceID != stanceID) {
		// Test if the palette in question is actually different to the one loaded.
		if (*PartPalettes[PAL_MAIN] != *(anims[0]->GetFrame(0)->GetPalette())) {
			PaletteResRef[PAL_MAIN].Reset();

			PartPalettes[PAL_MAIN] = anims[0]->GetFrame(0)->GetPalette()->Copy();
			SetupColors(PAL_MAIN);
		}
	}
}

static const ResRef EmptySound;

const ResRef &CharAnimations::GetWalkSound() const
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
	case IE_ANI_FOUR_FRAMES_2: //demogorgon animations
		return 4;
	case IE_ANI_TWO_PIECE:   //ankheg animations
		return 2;
	case IE_ANI_PST_GHOST:   //special pst anims
		if (IsStar(AvatarTable[AvatarsRowNum].Prefixes[1])) {
			return 1;
		}
		if (IsStar(AvatarTable[AvatarsRowNum].Prefixes[2])) {
			return 2;
		}
		if (IsStar(AvatarTable[AvatarsRowNum].Prefixes[3])) {
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

const ResRef& CharAnimations::GetArmourLevel(int ArmourLevel) const
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
	ResRefBase = AvatarTable[AvatarsRowNum].Prefixes[ArmourLevel];
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

void CharAnimations::SetWeaponType(unsigned char wt)
{
	if (wt != WeaponType) {
		WeaponType = wt;
		DropAnims();
	}
}

void CharAnimations::SetHelmetRef(const char* ref)
{
	strlcpy(HelmetRef, ref, sizeof(HelmetRef)+1);

	// Only drop helmet anims?
	// Note: this doesn't happen "often", so this isn't a performance
	//       bottleneck. (wjp)
	DropAnims();
	PartPalettes[PAL_HELMET] = nullptr;
	ModPartPalettes[PAL_HELMET] = nullptr;
}

void CharAnimations::SetWeaponRef(const char* ref)
{
	WeaponRef[0] = ref[0];
	WeaponRef[1] = ref[1];

	// TODO: Only drop weapon anims?
	DropAnims();
	PartPalettes[PAL_WEAPON] = nullptr;
	ModPartPalettes[PAL_WEAPON] = nullptr;
}

void CharAnimations::SetOffhandRef(const char* ref)
{
	OffhandRef[0] = ref[0];
	OffhandRef[1] = ref[1];

	// TODO: Only drop shield/offhand anims?
	DropAnims();
	PartPalettes[PAL_OFFHAND] = nullptr;
	ModPartPalettes[PAL_OFFHAND] = nullptr;
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
	if (PartPalettes[PAL_MAIN]) {
		lockPalette=true;
	}
}

// NOTE: change if MAX_ANIMS is increased
//                                          0   1   2   3   4   5   6   7   8   9  10  11  12  13  14  15  16  17  18
static const char StancePrefix[] =        {'3','2','5','5','4','4','2','2','5','4','1','3','3','3','4','1','4','4','4'};
static const char CyclePrefix[] =         {'0','0','1','1','1','1','0','0','1','1','0','0','0','0','1','1','1','1','1'};
static const unsigned int CycleOffset[] = { 0,  0,  0,  0,  0,  9,  0,  0,  0, 18,  0,  0,  9, 18,  0,  0,  0,  0,  0 };

#define NINE_FRAMES_PALETTE(stance)	((PaletteType) (StancePrefix[stance] - '1'))

void CharAnimations::SetColors(const ieDword *arg)
{
	Colors = arg;
	SetupColors(PAL_MAIN);
	SetupColors(PAL_MAIN_2);
	SetupColors(PAL_MAIN_3);
	SetupColors(PAL_MAIN_4);
	SetupColors(PAL_MAIN_5);
	SetupColors(PAL_WEAPON);
	SetupColors(PAL_OFFHAND);
	SetupColors(PAL_HELMET);
}

void CharAnimations::CheckColorMod()
{
	if (!GlobalColorMod.locked && GlobalColorMod.type != RGBModifier::NONE) {
		GlobalColorMod.type = RGBModifier::NONE;
		GlobalColorMod.speed = 0;
		for (bool& c : change) {
			c = true;
		}
	}

	for (unsigned int location = 0; location < PAL_MAX * 8; ++location) {
		if (!ColorMods[location].phase && ColorMods[location].type != RGBModifier::NONE) {
			ColorMods[location].type = RGBModifier::NONE;
			ColorMods[location].speed = 0;
			change[location >> 3] = true;
		}
	}
	//this is set by sanctuary and stoneskin (override global colors)
	lockPalette = false;
}

void CharAnimations::SetupColors(PaletteType type)
{
	PaletteHolder pal = PartPalettes[type];

	if (!pal) {
		return;
	}

	if (!Colors) {
		return;
	}
	
	int PType = NoPalette();

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
		constexpr int size = 32;
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
			const auto& pal32 = core->GetPalette32(static_cast<uint8_t>(Colors[i]));
			PartPalettes[PAL_MAIN]->CopyColorRange(&pal32[0], &pal32[32], static_cast<uint8_t>(dest));
			dest +=size;
		}

		if (needmod) {
			if (!ModPartPalettes[PAL_MAIN])
				ModPartPalettes[PAL_MAIN] = MakeHolder<Palette>();
			ModPartPalettes[PAL_MAIN]->SetupGlobalRGBModification(PartPalettes[PAL_MAIN], GlobalColorMod);
		} else {
			ModPartPalettes[PAL_MAIN] = nullptr;
		}
	} else if (PType && (type <= PAL_MAIN_5)) {
		//handling special palettes like MBER_BL (black bear)
		if (PType!=1) {
			if (GetAnimType()==IE_ANI_NINE_FRAMES) {
				PaletteResRef[type].SNPrintF("%.4s_%-.2s%c", ResRefBase.CString(), (char *) &PType, '1' + type);
			} else {
				if (ResRefBase == "MFIE") { // hack for magic golems
					PaletteResRef[type].SNPrintF("%.4s%-.2sB", ResRefBase.CString(), (char *) &PType);
				} else {
					PaletteResRef[type].SNPrintF("%.4s_%-.2s", ResRefBase.CString(), (char *) &PType);
				}
			}
			PaletteHolder tmppal = gamedata->GetPalette(PaletteResRef[type]);
			if (tmppal) {
				PartPalettes[type] = tmppal;
			} else {
				PaletteResRef[type].Reset();
			}
		}
		bool needmod = GlobalColorMod.type != RGBModifier::NONE;
		if (needmod) {
			if (!ModPartPalettes[type])
				ModPartPalettes[type] = MakeHolder<Palette>();
			ModPartPalettes[type]->SetupGlobalRGBModification(PartPalettes[type], GlobalColorMod);
		} else {
			ModPartPalettes[type] = nullptr;
		}
	} else {
		pal->SetupPaperdollColours(Colors, type);
		if (lockPalette) {
			return;
		}

		bool needmod = false;
		if (GlobalColorMod.type != RGBModifier::NONE) {
			needmod = true;
		} else {
			// TODO: should that -1 really be there??
			for (size_t i = 0; i < PAL_MAX - 1; ++i) {
				if (ColorMods[i+8*type].type != RGBModifier::NONE)
					needmod = true;
			}
		}

		if (needmod) {
			if (!ModPartPalettes[type])
				ModPartPalettes[type] = MakeHolder<Palette>();

			if (GlobalColorMod.type != RGBModifier::NONE) {
				ModPartPalettes[type]->SetupGlobalRGBModification(PartPalettes[type], GlobalColorMod);
			} else {
				ModPartPalettes[type]->SetupRGBModification(PartPalettes[type],ColorMods, type);
			}
		} else {
			ModPartPalettes[type] = nullptr;
		}
	}
}

PaletteHolder CharAnimations::GetPartPalette(int part) const
{
	int actorPartCount = GetActorPartCount();
	PaletteType type = PAL_MAIN;
	if (GetAnimType() == IE_ANI_NINE_FRAMES) {
		//these animations use several palettes
		type = NINE_FRAMES_PALETTE(stanceID);
	}
	else if (GetAnimType() == IE_ANI_FOUR_FRAMES_2) return nullptr;
	// always use unmodified BAM palette for the supporting part
	else if (GetAnimType() == IE_ANI_TWO_PIECE && part == 1) return nullptr;
	else if (part == actorPartCount) type = PAL_WEAPON;
	else if (part == actorPartCount+1) type = PAL_OFFHAND;
	else if (part == actorPartCount+2) type = PAL_HELMET;

	if (ModPartPalettes[type])
		return ModPartPalettes[type];

	return PartPalettes[type];
}

PaletteHolder CharAnimations::GetShadowPalette() const {
	return shadowPalette;
}

CharAnimations::CharAnimations(unsigned int AnimID, ieDword ArmourLevel)
{
	for (bool& c : change) {
		c = true;
	}

	for (size_t i = 0; i < MAX_ANIMS; i++) {
		for (size_t j = 0; j < MAX_ORIENT; j++) {
			Anims[i][j] = nullptr;
			shadowAnimations[i][j] = nullptr;
		}
	}

	for (size_t i = 0; i < PAL_MAX * 8; ++i) {
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
	GlobalColorMod.rgb = Color();

	AvatarsRowNum = GetAvatarsCount();
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
	Log(ERROR, "CharAnimations", "Invalid or nonexistent avatar entry:%04X", AnimID);
}

//we have to drop them when armourlevel changes
void CharAnimations::DropAnims()
{
	Animation** tmppoi;
	int partCount = GetTotalPartCount();
	for (int StanceID = 0; StanceID < MAX_ANIMS; StanceID++) {
		for (const auto& orient : Anims[StanceID]) {
			if (!orient) continue;

			tmppoi = orient;
			for (int j = 0; j < partCount; j++)
				delete orient[j];
			delete[] tmppoi;

			// anims can only be duplicated at the Animation** level
			for (int IDb = StanceID; IDb < MAX_ANIMS; IDb++) {
				for (auto& orient2 : Anims[IDb]) {
					if (orient2 == tmppoi) {
						orient2 = nullptr;
					}
				}
			}
		}
	}
}

CharAnimations::~CharAnimations(void)
{
	DropAnims();
	int i;
	for (i = 0; i <= PAL_MAIN_5; ++i)
		PartPalettes[i] = nullptr;
	for (; i < PAL_MAX; ++i)
		PartPalettes[i] = nullptr;
	for (i = 0; i < PAL_MAX; ++i)
		ModPartPalettes[i] = nullptr;

	if (shadowPalette) {
		shadowPalette = nullptr;
	}

	for (i = 0; i < MAX_ANIMS; ++i) {
		for (int j = 0; j < MAX_ORIENT; ++j) {
			if (shadowAnimations[i][j]) {
				delete shadowAnimations[i][j][0];
				delete[] shadowAnimations[i][j];
				j += 1;
			}
		}
	}
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

IE_ANI_TWO_FILES_3C:	Animations using this type are stored using the following template:
			[NAME][ACTIONTYPE]

			Example:
			MWDR*

			This is a cut down version of IE_ANI_TWO_FILES_3. E(ast) files are missing.
			See MWOR (missing de and gu suffixes)

IE_ANI_TWO_FILES_4: This type stores animations in two files (G1 and G2) which each having only
			one cycle. And both of those seem to be identical.

IE_ANI_TWO_FILES_5: Also uses G1 and G2 only but contains various cycles, including some special moves,
			with 9 orientations each. Only used by MMEL.

IE_ANI_TWO_PIECE: This is a modified IE_ANI_SIX_FILES with supporting still frames (using a
			different palette) stored in a second set of files. Currently only used by MAKH

IE_ANI_FOUR_FRAMES:	These animations are large, four bams make a frame.

IE_ANI_FOUR_FRAMES_2:	Another variation of the four frames format with more files.

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
	if (Stance >= MAX_ANIMS) {
		error("CharAnimation", "Illegal stance ID\n");
	}

	//for paletted dragon animations, we need the stance id
	stanceID = nextStanceID = Stance;
	int AnimType = GetAnimType();

	//alter stance here if it is missing and you know a substitute
	//probably we should feed this result back to the actor?
	switch (AnimType) {
		case -1: //invalid animation
			return nullptr;

		case IE_ANI_PST_STAND:
			stanceID = IE_ANI_AWAKE;
			break;
		case IE_ANI_PST_GHOST:
			stanceID = IE_ANI_AWAKE;
			Orient = 0;
			break;
		case IE_ANI_PST_ANIMATION_3: //stc->std
			if (stanceID == IE_ANI_READY) {
				stanceID = IE_ANI_AWAKE;
			}
			break;
		case IE_ANI_PST_ANIMATION_2: //std->stc
			if (stanceID == IE_ANI_AWAKE) {
				stanceID = IE_ANI_READY;
			}
			break;
	}

	//TODO: Implement Auto Resource Loading
	//setting up the sequencing of animation cycles
	autoSwitchOnEnd = false;
	switch (stanceID) {
		case IE_ANI_DAMAGE:
			nextStanceID = IE_ANI_READY;
			autoSwitchOnEnd = true;
			break;
		case IE_ANI_SLEEP: //going to sleep
		case IE_ANI_DIE: //going to die
			nextStanceID = IE_ANI_TWITCH;
			autoSwitchOnEnd = true;
			break;
		case IE_ANI_WALK:
		case IE_ANI_RUN:
		case IE_ANI_CAST: // looping
		case IE_ANI_READY:
		case IE_ANI_AWAKE:
		case IE_ANI_TWITCH: //dead, sleeping
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
			Log(MESSAGE, "CharAnimation", "Invalid Stance: %d", stanceID);
			break;
	}

	stanceID = MaybeOverrideStance(stanceID);

	bool lastFrameOnly = false;
	//pst (and some other) animations don't have separate animations for sleep/die
	if (Stance == IE_ANI_TWITCH &&
		(AnimType >= IE_ANI_PST_ANIMATION_1 || MaybeOverrideStance(IE_ANI_DIE) == stanceID))
	{
		lastFrameOnly = true;
	}

	Animation** anims = Anims[stanceID][Orient];
	if (anims) {
		MaybeUpdateMainPalette(anims);
		previousStanceID = stanceID;

		if (lastFrameOnly) {
			anims[0]->SetFrame(anims[0]->GetFrameCount() - 1);
		}

		return anims;
	}

	int partCount = GetTotalPartCount();
	int actorPartCount = GetActorPartCount();
	if (partCount <= 0) return nullptr;
	anims = new Animation*[partCount];

	EquipResRefData* equipdat = nullptr;
	for (int part = 0; part < partCount; ++part)
	{
		anims[part] = nullptr;

		//newresref is based on the prefix (ResRef) and various
		// other things, so it's longer than a typical ResRef
		std::string NewResRef;
		unsigned char Cycle = 0;
		if (part < actorPartCount) {
			// Character animation parts

			if (equipdat) delete equipdat;

			//we need this long for special anims
			NewResRef = ResRefBase;
			GetAnimResRef(stanceID, Orient, NewResRef, Cycle, part, equipdat);
		} else {
			// Equipment animation parts

			anims[part] = nullptr;
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

		// why do we bother with a bigger buffer if we truncate in the end? Let's see if it even happens in practice
		if (NewResRef.size() > 8) {
			Log(DEBUG, "CharAnimations", "Truncating animation ref (%s) to size.", NewResRef.c_str());
			NewResRef[8] = 0;
		}

		AnimationFactory* af = ( AnimationFactory* )
			gamedata->GetFactoryResource(NewResRef.c_str(), IE_BAM_CLASS_ID);

		if (!af) {
			if (part < actorPartCount) {
				Log(ERROR, "CharAnimations", "Couldn't create animationfactory: %s (%04x)",
						NewResRef.c_str(), GetAnimationID());
				for (int i = 0; i < part; ++i)
					delete anims[i];
				delete[] anims;
				delete equipdat;
				return nullptr;
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
						NewResRef.c_str(), Cycle);
				for (int i = 0; i < part; ++i)
					delete anims[i];
				delete[] anims;
				delete equipdat;
				return nullptr;
			} else {
				// not fatal if animation for equipment is missing
				continue;
			}
		}

		if (part < actorPartCount) {
			PaletteType ptype = PAL_MAIN;
			if (AnimType == IE_ANI_NINE_FRAMES) {
				//these animations use several palettes
				ptype = NINE_FRAMES_PALETTE(stanceID);
			}

			//if you need to revert this change, consider true paletted
			//animations which need a GlobalColorMod (mgir for example)

			//if (!palette[PAL_MAIN] && ((GlobalColorMod.type!=RGBModifier::NONE) || (NoPalette()!=1)) ) {
			if(!PartPalettes[ptype]) {
				// This is the first time we're loading an Animation.
				// We copy the palette of its first frame into our own palette
				PartPalettes[ptype] = a->GetFrame(0)->GetPalette()->Copy();
				// ...and setup the colours properly
				SetupColors(ptype);
			} else if (ptype == PAL_MAIN) {
				MaybeUpdateMainPalette(anims);
			}
		} else if (part == actorPartCount) {
			if (!PartPalettes[PAL_WEAPON]) {
				PartPalettes[PAL_WEAPON] = a->GetFrame(0)->GetPalette()->Copy();
				SetupColors(PAL_WEAPON);
			}
		} else if (part == actorPartCount+1) {
			if (!PartPalettes[PAL_OFFHAND]) {
				PartPalettes[PAL_OFFHAND] = a->GetFrame(0)->GetPalette()->Copy();
				SetupColors(PAL_OFFHAND);
			}
		} else if (part == actorPartCount+2) {
			if (!PartPalettes[PAL_HELMET]) {
				PartPalettes[PAL_HELMET] = a->GetFrame(0)->GetPalette()->Copy();
				SetupColors(PAL_HELMET);
			}
		}

		//animation is affected by game flags
		a->gameAnimation = true;
		if (lastFrameOnly) {
			a->SetFrame(a->GetFrameCount() - 1);
		} else {
			a->SetFrame(0);
		}

		//setting up the sequencing of animation cycles
		switch (stanceID) {
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
		switch (AnimType) {
			case IE_ANI_NINE_FRAMES: //dragon animations
			case IE_ANI_FOUR_FRAMES: //wyvern animations
			case IE_ANI_FOUR_FRAMES_2:
			case IE_ANI_BIRD:
			case IE_ANI_CODE_MIRROR:
			case IE_ANI_CODE_MIRROR_2: //9 orientations
			case IE_ANI_CODE_MIRROR_3:
			case IE_ANI_PST_ANIMATION_3: //no stc just std
			case IE_ANI_PST_ANIMATION_2: //no std just stc
			case IE_ANI_PST_ANIMATION_1:
			case IE_ANI_FRAGMENT:
			case IE_ANI_TWO_FILES_3C:
			case IE_ANI_TWO_FILES_5:
				if (Orient > 8) {
					a->MirrorAnimation(BlitFlags::MIRRORX);
				}
				break;
			default:
				break;
		}

		// make animarea of part 0 encompass the animarea of the other parts
		if (part > 0)
			anims[0]->AddAnimArea(a);

	}

	switch (AnimType) {
		case IE_ANI_NINE_FRAMES: //dragon animations
		case IE_ANI_FOUR_FRAMES: //wyvern animations
		case IE_ANI_FOUR_FRAMES_2:
		case IE_ANI_BIRD:
		case IE_ANI_CODE_MIRROR:
		case IE_ANI_SIX_FILES: //16 anims some are stored elsewhere
		case IE_ANI_ONE_FILE: //16 orientations
		case IE_ANI_TWO_FILES_5: // 9 orientations
		case IE_ANI_CODE_MIRROR_2: //9 orientations
		case IE_ANI_CODE_MIRROR_3:
		case IE_ANI_PST_GHOST:
			Anims[stanceID][Orient] = anims;
			break;
		case IE_ANI_TWO_FILES:
		case IE_ANI_TWENTYTWO:
		case IE_ANI_TWO_FILES_2:
		case IE_ANI_TWO_FILES_3:
		case IE_ANI_TWO_FILES_3B:
		case IE_ANI_TWO_FILES_3C:
		case IE_ANI_FOUR_FILES:
		case IE_ANI_FOUR_FILES_2:
		case IE_ANI_SIX_FILES_2:
		case IE_ANI_TWO_PIECE:
		case IE_ANI_FRAGMENT:
		case IE_ANI_PST_STAND:
			Orient&=~1;
			Anims[stanceID][Orient] = anims;
			Anims[stanceID][Orient + 1] = anims;
			break;
		case IE_ANI_FOUR_FILES_3:
			//only 8 orientations for WALK
			if (stanceID == IE_ANI_WALK) {
				Orient&=~1;
				Anims[stanceID][Orient] = anims;
				Anims[stanceID][Orient + 1] = anims;
			} else {
				Anims[stanceID][Orient] = anims;
			}
			break;
		case IE_ANI_TWO_FILES_4:
			for (auto& anim : Anims) {
				for (auto& orient : anim) {
					orient = anims;
				}
			}
			break; 

		case IE_ANI_PST_ANIMATION_3: //no stc just std
		case IE_ANI_PST_ANIMATION_2: //no std just stc
		case IE_ANI_PST_ANIMATION_1:
			switch (stanceID) {
				case IE_ANI_WALK:
				case IE_ANI_RUN:
				case IE_ANI_PST_START:
					Anims[stanceID][Orient] = anims;
					break;
				default:
					Orient &=~1;
					Anims[stanceID][Orient] = anims;
					Anims[stanceID][Orient + 1] = anims;
					break;
			}
			break;
		default:
			error("CharAnimations", "Unknown animation type\n");
	}
	delete equipdat;
	previousStanceID = stanceID;

	return Anims[stanceID][Orient];
}

Animation** CharAnimations::GetShadowAnimation(unsigned char stance, unsigned char orientation) {
	if (GetTotalPartCount() <= 0 || IE_ANI_TWENTYTWO != GetAnimType()) {
		return nullptr;
	}

	unsigned char newStanceID = MaybeOverrideStance(stance);

	switch (newStanceID) {
		case IE_ANI_WALK:
		case IE_ANI_RUN:
		case IE_ANI_CAST:
		case IE_ANI_SHOOT:
		case IE_ANI_ATTACK:
		case IE_ANI_ATTACK_JAB:
		case IE_ANI_ATTACK_SLASH:
		case IE_ANI_ATTACK_BACKSLASH:
		case IE_ANI_AWAKE:
		case IE_ANI_HEAD_TURN:
		case IE_ANI_DIE:
		case IE_ANI_READY:
		case IE_ANI_CONJURE:
			break;
		default:
			return nullptr;
	}

	if (shadowAnimations[newStanceID][orientation]) {
		return shadowAnimations[newStanceID][orientation];
	}

	Animation** animations = nullptr;

	if (!AvatarTable[AvatarsRowNum].ShadowAnimation.IsEmpty()) {
		int partCount = GetTotalPartCount();
		animations = new Animation*[partCount];

		std::string shadowName = AvatarTable[AvatarsRowNum].ShadowAnimation.CString();

		for (int i = 0; i < partCount; ++i) {
			animations[i] = nullptr;
		}

		EquipResRefData *dummy = nullptr;
		unsigned char cycle = 0;
		AddMHRSuffix(shadowName, newStanceID, cycle, orientation, dummy);
		delete dummy;

		AnimationFactory* af = static_cast<AnimationFactory*>(
			gamedata->GetFactoryResource(shadowName.c_str(), IE_BAM_CLASS_ID));

		if (!af) {
			delete[] animations;
			return nullptr;
		}

		Animation *animation = af->GetCycle(cycle);
		animations[0] = animation;

		if (!animation) {
			delete[] animations;
			return nullptr;
		}

		if (!shadowPalette) {
			shadowPalette = animation->GetFrame(0)->GetPalette()->Copy();
		}

		switch (newStanceID) {
			case IE_ANI_DAMAGE:
			case IE_ANI_TWITCH:
			case IE_ANI_DIE:
			case IE_ANI_HEAD_TURN:
			case IE_ANI_CONJURE:
			case IE_ANI_SHOOT:
			case IE_ANI_ATTACK:
			case IE_ANI_ATTACK_JAB:
			case IE_ANI_ATTACK_SLASH:
			case IE_ANI_ATTACK_BACKSLASH:
				animation->Flags |= A_ANI_PLAYONCE;
				break;
			default:
				break;
		}

		animation->gameAnimation = true;
		animation->SetFrame(0);
		animations[0]->AddAnimArea(animation);

		orientation &= ~1;
		shadowAnimations[newStanceID][orientation] = animations;
		shadowAnimations[newStanceID][orientation + 1] = animations;

		return shadowAnimations[newStanceID][orientation];
	}

	return nullptr;
}

static const int one_file[MAX_ANIMS] = {2, 1, 0, 0, 2, 3, 0, 1, 0, 4, 1, 0, 0, 0, 3, 1, 4, 4, 4};

void CharAnimations::GetAnimResRef(unsigned char StanceID,
					 unsigned char Orient,
					 std::string& NewResRef, unsigned char& Cycle,
					 int Part, EquipResRefData*& EquipData) const
{
	EquipData = nullptr;
	Orient &= 15;
	switch (GetAnimType()) {
		case IE_ANI_FOUR_FRAMES:
			AddFFSuffix( NewResRef, StanceID, Cycle, Orient, Part );
			break;

		case IE_ANI_FOUR_FRAMES_2:
			AddFF2Suffix( NewResRef, StanceID, Cycle, Orient, Part );
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
			AddMMRSuffix(NewResRef, StanceID, Cycle, Orient, false);
			break;

		case IE_ANI_TWO_FILES_3B: //IWD style anims
			AddMMR2Suffix( NewResRef, StanceID, Cycle, Orient );
			break;

		case IE_ANI_TWO_FILES_3C: //IWD style anims
			AddMMRSuffix(NewResRef, StanceID, Cycle, Orient, true);
			break;

		case IE_ANI_TWO_FILES_4:
			NewResRef += "g1";
			Cycle = 0;
			break;

		case IE_ANI_TWO_FILES_5:
			AddTwoFiles5Suffix( NewResRef, StanceID, Cycle, Orient );
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
			NewResRef = ResRefBase.CString()[0];
			NewResRef += "STD";
			NewResRef += *(ResRefBase.CString() + 1);
			Cycle = SixteenToFive[Orient];
			break;
		case IE_ANI_PST_GHOST: // pst static animations
			//still doesn't handle the second cycle of the golem anim
			Cycle = 0;
			NewResRef = AvatarTable[AvatarsRowNum].Prefixes[Part];
			break;
		default:
			error("CharAnimations", "Unknown animation type in avatars.2da row: %lu\n", static_cast<unsigned long>(AvatarsRowNum));
	}
}

void CharAnimations::GetEquipmentResRef(const char* equipRef, bool offhand,
	std::string& dest, unsigned char& Cycle, const EquipResRefData* equip) const
{
	switch (GetAnimType()) {
		case IE_ANI_FOUR_FILES:
		case IE_ANI_FOUR_FILES_2:
			GetLREquipmentRef(dest, Cycle, equipRef, offhand, equip);
			break;
		case IE_ANI_CODE_MIRROR:
			GetVHREquipmentRef(dest, Cycle, equipRef, offhand, equip);
			break;
		case IE_ANI_TWENTYTWO:
			GetMHREquipmentRef(dest, Cycle, equipRef, offhand, equip);
			break;
		default:
			error("CharAnimations", "Unsupported animation type for equipment animation.\n");
	}
}

const int* CharAnimations::GetZOrder(unsigned char Orient) const
{
	switch (GetAnimType()) {
		case IE_ANI_CODE_MIRROR:
			return zOrder_Mirror16[Orient];
		case IE_ANI_TWENTYTWO:
			return zOrder_8[Orient/2];
		case IE_ANI_FOUR_FILES:
			return nullptr; // FIXME
		case IE_ANI_TWO_PIECE:
			return zOrder_TwoPiece;
		default:
			return nullptr;
	}
}


void CharAnimations::AddPSTSuffix(std::string& dest, unsigned char StanceID,
	unsigned char& Cycle, unsigned char Orient) const
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
			if (RAND(0,1)) {
				Prefix="sf2";
				dest = ResRefBase.CString()[0];
				dest += Prefix;
				dest += (ResRefBase.CString() + 1);
				if (gamedata->Exists(dest.c_str(), IE_BAM_CLASS_ID)) {
					return;
				}
			}
			Prefix="sf1";
			dest = ResRefBase.CString()[0];
			dest += Prefix;
			dest += (ResRefBase.CString() + 1);
			if (gamedata->Exists(dest.c_str(), IE_BAM_CLASS_ID)) {
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
	dest = ResRefBase.CString()[0];
	dest += Prefix;
	dest += (ResRefBase.CString() + 1);
}

void CharAnimations::AddVHR2Suffix(std::string& dest, unsigned char StanceID,
	unsigned char& Cycle, unsigned char Orient) const
{
	Cycle=SixteenToNine[Orient];

	switch (StanceID) {
		case IE_ANI_ATTACK: //temporarily
		case IE_ANI_ATTACK_BACKSLASH:
			dest += "g21";
			Cycle+=9;
			break;

		case IE_ANI_ATTACK_SLASH:
			dest += "g2";
			break;

		case IE_ANI_ATTACK_JAB:
			dest += "g22";
			Cycle+=18;
			break;

		case IE_ANI_CAST: //looping
			dest += "g25";
			Cycle+=45;
			break;

		case IE_ANI_CONJURE://ending
			dest += "g26";
			Cycle+=54;
			break;

		case IE_ANI_SHOOT:
			dest += "g24";
			Cycle+=27;
			break;

		case IE_ANI_HEAD_TURN:
		case IE_ANI_AWAKE:
			dest += "g12";
			Cycle+=18;
			break;

		case IE_ANI_SLEEP:
		case IE_ANI_TWITCH:
			dest += "g15";
			Cycle+=45;
			break;

		case IE_ANI_DIE:
		case IE_ANI_EMERGE:
		case IE_ANI_GET_UP:
		case IE_ANI_PST_START:
			dest += "g14";
			Cycle+=36;
			break;

		case IE_ANI_DAMAGE:
			dest += "g13";
			Cycle+=27;
			break;

		case IE_ANI_READY:
			dest += "g1";
			Cycle+=9;
			break;

		case IE_ANI_WALK:
			dest += "g11";
			break;

		case IE_ANI_HIDE:
			dest += "g22";
			break;
		default:
			error("CharAnimation", "VHR2 Animation: unhandled stance: %s %d\n", dest.c_str(), StanceID);
	}
}

void CharAnimations::AddVHR3Suffix(std::string& dest, unsigned char StanceID,
	unsigned char& Cycle, unsigned char Orient) const
{
	Cycle=SixteenToNine[Orient];

	switch (StanceID) {
		case IE_ANI_ATTACK: //temporarily
		case IE_ANI_ATTACK_BACKSLASH:
			dest += "g21";
			Cycle+=9;
			break;

		case IE_ANI_ATTACK_SLASH:
			dest += "g2";
			break;

		case IE_ANI_ATTACK_JAB:
		case IE_ANI_CONJURE://ending
			dest += "g22";
			Cycle+=18;
			break;

		case IE_ANI_CAST: //looping
			dest += "g22";
			Cycle+=27;
			break;

		case IE_ANI_SHOOT:
			dest += "g23";
			Cycle+=27;
			break;

		case IE_ANI_HEAD_TURN:
		case IE_ANI_AWAKE:
		case IE_ANI_HIDE:
			dest += "g12";
			Cycle+=18;
			break;

		case IE_ANI_SLEEP:
		case IE_ANI_TWITCH:
			dest += "g15";
			Cycle+=45;
			break;

		case IE_ANI_DIE:
		case IE_ANI_EMERGE:
		case IE_ANI_GET_UP:
		case IE_ANI_PST_START:
			dest += "g14";
			Cycle+=36;
			break;

		case IE_ANI_DAMAGE:
			dest += "g13";
			Cycle+=27;
			break;

		case IE_ANI_READY:
			dest += "g1";
			Cycle+=9;
			break;

		case IE_ANI_WALK:
			dest += "g11";
			break;
		default:
			error("CharAnimation", "VHR3 Animation: unhandled stance: %s %d\n", dest.c_str(), StanceID);
	}
}

// Note: almost like SixSuffix
void CharAnimations::AddFFSuffix(std::string& dest, unsigned char StanceID,
	unsigned char& Cycle, unsigned char Orient, int Part) const
{
	Cycle=SixteenToNine[Orient];
	switch (StanceID) {
		case IE_ANI_WALK:
			dest += "g1";
			break;

		case IE_ANI_ATTACK:
		case IE_ANI_ATTACK_SLASH:
		case IE_ANI_SHOOT:
			dest += "g3";
			break;

		case IE_ANI_ATTACK_BACKSLASH:
			dest += "g3";
			Cycle += 16;
			break;

		case IE_ANI_ATTACK_JAB:
		case IE_ANI_CAST:
		case IE_ANI_CONJURE:
			dest += "g3";
			Cycle += 32;
			break;

		case IE_ANI_HEAD_TURN: //could be wrong
		case IE_ANI_HIDE: //could be wrong
		case IE_ANI_AWAKE:
			dest += "g2";
			break;

		case IE_ANI_READY:
			dest += "g2";
			Cycle += 16;
			break;

		case IE_ANI_DAMAGE:
			dest += "g2";
			Cycle += 32;
			break;

		case IE_ANI_DIE:
		case IE_ANI_GET_UP:
		case IE_ANI_EMERGE:
		case IE_ANI_PST_START:
			dest += "g2";
			Cycle += 48;
			break;

		case IE_ANI_SLEEP:
		case IE_ANI_TWITCH:
			dest += "g2";
			Cycle += 64;
			break;

		default:
			error("CharAnimation", "Four frames Animation: unhandled stance: %s %d\n", dest.c_str(), StanceID);

	}
	dest += static_cast<char>(Part + '1');
}

void CharAnimations::AddFF2Suffix(std::string& dest, unsigned char StanceID,
	unsigned char& Cycle, unsigned char Orient, int Part) const
{
	Cycle = SixteenToNine[Orient];
	switch (StanceID) {
		case IE_ANI_HEAD_TURN:
			dest += "g101";
			break;

		case IE_ANI_READY:
		case IE_ANI_AWAKE:
			dest += "g102";
			Cycle += 9;
			break;

		case IE_ANI_WALK:
			dest += "g101";
			break;

		case IE_ANI_CAST:
		case IE_ANI_CONJURE:
			dest += "g205";
			Cycle += 45;
			break;

		case IE_ANI_ATTACK:
		case IE_ANI_ATTACK_SLASH:
			dest += "g206";
			Cycle += 54;
			break;

		case IE_ANI_ATTACK_BACKSLASH:
			dest += "g202";
			break;

		case IE_ANI_ATTACK_JAB:
			dest += "g203";
			Cycle += 18;
			break;

		case IE_ANI_DIE:
		case IE_ANI_GET_UP:
		case IE_ANI_EMERGE:
			dest += "g104";
			Cycle += 36;
			break;

		case IE_ANI_SLEEP:
		case IE_ANI_TWITCH:
		case IE_ANI_DAMAGE:
			dest += "g103";
			Cycle += 27;
			break;

		default:
			error("CharAnimation", "Four frames 2 Animation: unhandled stance: %s %d\n", dest.c_str(), StanceID);

	}
	dest += static_cast<char>(Part + '1');
}

void CharAnimations::AddNFSuffix(std::string& dest, unsigned char StanceID,
	unsigned char& Cycle, unsigned char Orient, int Part) const
{
	char prefix[10];
	char prefix2[10];

	Cycle = SixteenToNine[Orient];
	snprintf(prefix, 9, "%s%c%d%c%d", dest.c_str(), StancePrefix[StanceID], (Part + 1) % 100,
			 CyclePrefix[StanceID], Cycle);
	strnlwrcpy(prefix2, prefix, 8);
	dest = prefix2;
	Cycle=(ieByte) (Cycle+CycleOffset[StanceID]);
}

//Attack
//h1, h2, w2
//static const char *SlashPrefix[]={"a1","a4","a7"};
//static const char *BackPrefix[]={"a2","a5","a8"};
//static const char *JabPrefix[]={"a3","a6","a9"};
static const char* const SlashPrefix[] = { "a1", "a2", "a7" };
static const char* const BackPrefix[] = { "a3", "a4", "a8" };
static const char* const JabPrefix[] = { "a5", "a6", "a9" };
static const char* const RangedPrefix[] = { "sa", "sx", "ss" };
static const char* const RangedPrefixOld[] = { "sa", "sx", "a1" };

void CharAnimations::AddVHRSuffix(std::string& dest, unsigned char StanceID,
	unsigned char& Cycle, unsigned char Orient, EquipResRefData*& EquipData) const
{
	Cycle = SixteenToNine[Orient];
	EquipData = new EquipResRefData;
	EquipData->Suffix[0] = 0;
	switch (StanceID) {
		case IE_ANI_ATTACK:
		case IE_ANI_ATTACK_SLASH:
			dest += SlashPrefix[WeaponType];
			strlcpy(EquipData->Suffix, SlashPrefix[WeaponType], sizeof(EquipData->Suffix));
			break;

		case IE_ANI_ATTACK_BACKSLASH:
			dest += BackPrefix[WeaponType];
			strlcpy(EquipData->Suffix, BackPrefix[WeaponType], sizeof(EquipData->Suffix));
			break;

		case IE_ANI_ATTACK_JAB:
			dest += JabPrefix[WeaponType];
			strlcpy(EquipData->Suffix, JabPrefix[WeaponType], sizeof(EquipData->Suffix));
			break;

		case IE_ANI_AWAKE:
			dest += "g17";
			strcpy( EquipData->Suffix, "g1" );
			Cycle += 63;
			break;

		case IE_ANI_CAST: //looping
			dest += "ca";
			strcpy( EquipData->Suffix, "ca" );
			break;

		case IE_ANI_CONJURE: //ending
			dest += "ca";
			strcpy( EquipData->Suffix, "ca" );
			Cycle += 9;
			break;

		case IE_ANI_DAMAGE:
			dest += "g14";
			strcpy( EquipData->Suffix, "g1" );
			Cycle += 36;
			break;

		case IE_ANI_DIE:
			dest += "g15";
			strcpy( EquipData->Suffix, "g1" );
			Cycle += 45;
			break;
			//I cannot find an emerge animation...
			//Maybe is Die reversed
		case IE_ANI_GET_UP:
		case IE_ANI_EMERGE:
		case IE_ANI_PST_START:
			dest += "g19";
			strcpy( EquipData->Suffix, "g1" );
			Cycle += 81;
			break;

		case IE_ANI_HEAD_TURN:
			if (RAND(0,1)) {
				dest += "g12";
				Cycle += 18;
			} else {
				dest += "g18";
				Cycle += 72;
			}
			strcpy( EquipData->Suffix, "g1" );
			break;

			//Unknown... maybe only a transparency effect apply
		case IE_ANI_HIDE:
			break;

		case IE_ANI_READY:
			if ( WeaponType == IE_ANI_WEAPON_2H ) {
				dest += "g13";
				Cycle += 27;
			} else {
				dest += "g1";
				Cycle += 9;
			}
			strcpy( EquipData->Suffix, "g1" );
			break;
			//This depends on the ranged weapon equipped
		case IE_ANI_SHOOT:
			dest += RangedPrefix[RangedType];
			strlcpy(EquipData->Suffix, RangedPrefix[RangedType], sizeof(EquipData->Suffix));
			break;

		case IE_ANI_SLEEP:
		case IE_ANI_TWITCH:
			dest += "g16";
			strcpy( EquipData->Suffix, "g1" );
			Cycle += 54;
			break;

		case IE_ANI_WALK:
			dest += "g11";
			strcpy( EquipData->Suffix, "g1" );
			break;

		default:
			error("CharAnimation", "VHR Animation: unhandled stance: %s %d\n", dest.c_str(), StanceID);
	}
	EquipData->Cycle = Cycle;
}

void CharAnimations::GetVHREquipmentRef(std::string& dest, unsigned char& Cycle,
			const char* equipRef, bool offhand,
			const EquipResRefData* equip) const
{
	Cycle = equip->Cycle;

	dest = "wq";
	dest += GetSize();
	dest += equipRef;
	if (offhand) {
		dest += "o";
	}
	dest += equip->Suffix;
}

void CharAnimations::AddSixSuffix(std::string& dest, unsigned char StanceID,
	unsigned char& Cycle, unsigned char Orient) const
{
	switch (StanceID) {
		case IE_ANI_WALK:
			dest += "g1";
			Cycle = Orient;
			break;

		case IE_ANI_ATTACK:
		case IE_ANI_ATTACK_SLASH:
			dest += "g3";
			Cycle = Orient;
			break;

		case IE_ANI_ATTACK_BACKSLASH:
			dest += "g3";
			Cycle = 16 + Orient;
			break;

		case IE_ANI_ATTACK_JAB:
			dest += "g3";
			Cycle = 32 + Orient;
			break;

		case IE_ANI_HEAD_TURN: //could be wrong
		case IE_ANI_AWAKE:
		case IE_ANI_CAST: //could be wrong
		case IE_ANI_CONJURE:
			dest += "g2";
			Cycle = 0 + Orient;
			break;

		case IE_ANI_READY:
		case IE_ANI_HIDE: //could be wrong
			dest += "g2";
			Cycle = 16 + Orient;
			break;

		case IE_ANI_DAMAGE:
			dest += "g2";
			Cycle = 32 + Orient;
			break;

		case IE_ANI_DIE:
		case IE_ANI_GET_UP:
		case IE_ANI_EMERGE:
		case IE_ANI_PST_START:
			dest += "g2";
			Cycle = 48 + Orient;
			break;

		case IE_ANI_TWITCH:
		case IE_ANI_SLEEP:
			dest += "g2";
			Cycle = 64 + Orient;
			break;

		default:
			error("CharAnimation", "Six Animation: unhandled stance: %s %d\n", dest.c_str(), StanceID);

	}
	if (Orient>9) {
		dest += "e";
	}
}

void CharAnimations::AddLR2Suffix(std::string& dest, unsigned char StanceID,
	unsigned char& Cycle, unsigned char Orient) const
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
			error("CharAnimation", "LR2 Animation: unhandled stance: %s %d\n", dest.c_str(), StanceID);
	}
	if (Orient>=4) {
		dest += "g1e";
	} else {
		dest += "g1";
	}
}

void CharAnimations::AddMHRSuffix(std::string& dest, unsigned char StanceID,
	unsigned char& Cycle, unsigned char Orient, EquipResRefData*& EquipData) const
{
	Orient /= 2;
	EquipData = new EquipResRefData;
	EquipData->Suffix[0] = 0;

	switch (StanceID) {
		case IE_ANI_ATTACK:
		case IE_ANI_ATTACK_SLASH:
			dest += SlashPrefix[WeaponType];
			strlcpy(EquipData->Suffix, SlashPrefix[WeaponType], sizeof(EquipData->Suffix));
			Cycle = Orient;
			break;

		case IE_ANI_ATTACK_BACKSLASH:
			dest += BackPrefix[WeaponType];
			strlcpy(EquipData->Suffix, BackPrefix[WeaponType], sizeof(EquipData->Suffix));
			Cycle = Orient;
			break;

		case IE_ANI_ATTACK_JAB:
			dest += JabPrefix[WeaponType];
			strlcpy(EquipData->Suffix, JabPrefix[WeaponType], sizeof(EquipData->Suffix));
			Cycle = Orient;
			break;

		case IE_ANI_READY:
			dest += "g1";
			strcpy( EquipData->Suffix, "g1" );
			if ( WeaponType == IE_ANI_WEAPON_2W ) {
				Cycle = 24 + Orient;
			} else {
				Cycle = 8 + Orient;
			}
			break;

		case IE_ANI_CAST://looping
			dest += "ca";
			strcpy( EquipData->Suffix, "ca" );
			Cycle = 8 + Orient;
			break;

		case IE_ANI_CONJURE://ending
			dest += "ca";
			strcpy( EquipData->Suffix, "ca" );
			Cycle = Orient;
			break;

		case IE_ANI_DAMAGE:
			dest += "g1";
			strcpy( EquipData->Suffix, "g1" );
			Cycle = 40 + Orient;
			break;

		case IE_ANI_DIE:
		case IE_ANI_GET_UP:
		case IE_ANI_PST_START:
		case IE_ANI_EMERGE: // I cannot find an emerge animation... Maybe it is Die reversed
			dest += "g1";
			strcpy( EquipData->Suffix, "g1" );
			Cycle = 48 + Orient;
			break;
		case IE_ANI_HEAD_TURN:
			dest += "g1";
			strcpy( EquipData->Suffix, "g1" );
			Cycle = 32 + Orient;
			break;

			//Unknown... maybe only a transparency effect apply
		case IE_ANI_HIDE:
			break;

		case IE_ANI_AWAKE:
			dest += "g1";
			strcpy( EquipData->Suffix, "g1" );
			Cycle = 16 + Orient;
			break;

			//This depends on the ranged weapon equipped
		case IE_ANI_SHOOT:
			dest += RangedPrefixOld[RangedType];
			strlcpy(EquipData->Suffix, RangedPrefixOld[RangedType], sizeof(EquipData->Suffix));
			Cycle = Orient;
			break;

		case IE_ANI_SLEEP:
			dest += "g1";
			strcpy( EquipData->Suffix, "g1" );
			Cycle = 64 + Orient;
			break;

		case IE_ANI_TWITCH:
			dest += "g1";
			strcpy( EquipData->Suffix, "g1" );
			Cycle = 56 + Orient;
			break;

		case IE_ANI_WALK:
			dest += "g1";
			strcpy( EquipData->Suffix, "g1" );
			Cycle = Orient;
			break;
		default:
			error("CharAnimation", "MHR Animation: unhandled stance: %s %d\n", dest.c_str(), StanceID);
	}
	if (Orient>=5) {
		dest += "e";
		strcat( EquipData->Suffix, "e" );
	}
	// NOTE: the two shadow animations (cshd, sshd) also have x-suffixed files,
	// but those are used (instead of the eastern ones) only if sprite
	// mirroring is on. "Mirror sprites" in bgee.lua, probably what was
	// SoftMirrorBlt in the original ini. Pretty useless.
	EquipData->Cycle = Cycle;
}

void CharAnimations::GetMHREquipmentRef(std::string& dest, unsigned char& Cycle,
			const char* equipRef, bool offhand,
			const EquipResRefData* equip) const
{
	Cycle = equip->Cycle;
	if (offhand) {
		//i think there is no offhand stuff for bg1, lets use the bg2 equivalent here?
		dest = "wq";
		dest += GetSize();
		dest += equipRef;
		dest += "o";
		dest += equip->Suffix;
	} else {
		dest = "wp";
		dest += GetSize();
		dest += equipRef;
		dest += equip->Suffix;
	}
}

void CharAnimations::AddTwoFileSuffix( std::string& dest, unsigned char StanceID,
	unsigned char& Cycle, unsigned char Orient) const
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
	dest += "g1";
	if (Orient > 9) {
		dest += "e";
	}
}

void CharAnimations::AddTwoFiles5Suffix( std::string& dest, unsigned char StanceID,
	unsigned char& Cycle, unsigned char Orient) const
{
	const char *suffix;
	Cycle=SixteenToNine[Orient];

	switch(StanceID) {
		case IE_ANI_WALK:
			suffix = "g1";
			break;
		case IE_ANI_READY:
			Cycle += 9;
			suffix = "g1";
			break;
		case IE_ANI_HEAD_TURN:
			Cycle += 18;
			suffix = "g1";
			break;
		case IE_ANI_DAMAGE:
			Cycle += 27;
			suffix = "g1";
			break;
		case IE_ANI_DIE:
			Cycle += 36;
			suffix = "g1";
			break;
		case IE_ANI_SLEEP:
		case IE_ANI_TWITCH:
			Cycle += 45;
			suffix = "g1";
			break;
		// dead but not quite dead...
		//	Cycle += 54;
		//	suffix = "g1";
		//	break;
		case IE_ANI_GET_UP:
		case IE_ANI_EMERGE:
			Cycle += 63;
			suffix = "g1";
			break;
		case IE_ANI_ATTACK:
			suffix = "g2";
			break;
		case IE_ANI_SHOOT:
			Cycle += 9;
			suffix = "g2";
			break;
		// yet another attack
		//	Cycle += 18;
		//	suffix = "g2";
		//	break;
		case IE_ANI_ATTACK_BACKSLASH:
			Cycle += 27;
			suffix = "g2";
			break;
		case IE_ANI_ATTACK_JAB:
			Cycle += 36;
			suffix = "g2";
			break;
		case IE_ANI_CONJURE:
			Cycle += 45;
			suffix = "g2";
			break;
		case IE_ANI_ATTACK_SLASH:
		case IE_ANI_CAST:
			Cycle += 54;
			suffix = "g2";
			break;
		default:
			Cycle += 18;
			suffix = "g1";
	}
	dest += suffix;
}

void CharAnimations::AddLRSuffix2( std::string& dest, unsigned char StanceID,
	unsigned char& Cycle, unsigned char Orient, EquipResRefData *&EquipData) const
{
	EquipData = new EquipResRefData;
	EquipData->Suffix[0] = 0;
	switch (StanceID) {
		case IE_ANI_ATTACK:
		case IE_ANI_ATTACK_BACKSLASH:
		case IE_ANI_ATTACK_SLASH:
		case IE_ANI_ATTACK_JAB:
			dest += "g2";
			strcpy( EquipData->Suffix, "g2" );
			Cycle = Orient / 2;
			break;
		case IE_ANI_CAST:
		case IE_ANI_CONJURE:
		case IE_ANI_SHOOT:
			dest += "g2";
			strcpy( EquipData->Suffix, "g2" );
			Cycle = 8 + Orient / 2;
			break;
		case IE_ANI_WALK:
			dest += "g1";
			strcpy( EquipData->Suffix, "g1" );
			Cycle = Orient / 2;
			break;
		case IE_ANI_READY:
			dest += "g1";
			strcpy( EquipData->Suffix, "g1" );
			Cycle = 8 + Orient / 2;
			break;
		case IE_ANI_HEAD_TURN: //could be wrong
		case IE_ANI_AWAKE:
			dest += "g1";
			strcpy( EquipData->Suffix, "g1" );
			Cycle = 16 + Orient / 2;
			break;
		case IE_ANI_DAMAGE:
			dest += "g1";
			strcpy( EquipData->Suffix, "g1" );
			Cycle = 24 + Orient / 2;
			break;
		case IE_ANI_GET_UP:
		case IE_ANI_EMERGE:
		case IE_ANI_PST_START:
		case IE_ANI_DIE:
			dest += "g1";
			strcpy( EquipData->Suffix, "g1" );
			Cycle = 32 + Orient / 2;
			break;
		case IE_ANI_SLEEP:
		case IE_ANI_HIDE:
		case IE_ANI_TWITCH:
			dest += "g1";
			strcpy( EquipData->Suffix, "g1" );
			Cycle = 40 + Orient / 2;
			break;
		default:
			error("CharAnimation", "LRSuffix2 Animation: unhandled stance: %s %d\n", dest.c_str(), StanceID);
	}
	if (Orient > 9) {
		dest += "e";
		strcat( EquipData->Suffix, "e");
	}
	EquipData->Cycle = Cycle;
}

void CharAnimations::AddTwoPieceSuffix(std::string& dest, unsigned char StanceID,
	unsigned char& Cycle, unsigned char Orient, int Part) const
{
	if (Part == 1) {
		dest += "d";
	}

	switch (StanceID) {
		case IE_ANI_DIE:
			dest += "g1";
			Cycle = 8 + Orient / 2;
			break;
		case IE_ANI_TWITCH:
		case IE_ANI_SLEEP:
			dest += "g1";
			Cycle = 16 + Orient / 2;
			break;
		case IE_ANI_READY:
		case IE_ANI_HEAD_TURN:
		case IE_ANI_AWAKE:
		case IE_ANI_DAMAGE:
			dest += "g1";
			Cycle = 24 + Orient / 2;
			break;
		case IE_ANI_WALK:
			dest += "g2";
			Cycle = Orient / 2;
			break;
		case IE_ANI_GET_UP:
		case IE_ANI_EMERGE:
			dest += "g2";
			Cycle = 8 + Orient / 2;
			break;
		case IE_ANI_HIDE:
			dest += "g2";
			Cycle = 16 + Orient / 2;
			break;
		case IE_ANI_ATTACK:
		case IE_ANI_ATTACK_BACKSLASH:
			dest += "g3";
			Cycle = Orient / 2;
			break;
		case IE_ANI_ATTACK_SLASH:
		case IE_ANI_ATTACK_JAB:
		case IE_ANI_CAST:
		case IE_ANI_CONJURE:
		case IE_ANI_SHOOT:
			dest += "g3";
			Cycle = 8 + Orient / 2;
			break;
		default:
			error("CharAnimation", "Two-piece Animation: unhandled stance: %s %d", dest.c_str(), StanceID);
	}
	if (Orient > 9) {
		dest += "e";
	}
}

void CharAnimations::AddLRSuffix( std::string& dest, unsigned char StanceID,
	unsigned char& Cycle, unsigned char Orient, EquipResRefData *&EquipData) const
{
	EquipData = new EquipResRefData;
	EquipData->Suffix[0] = 0;
	switch (StanceID) {
		case IE_ANI_ATTACK:
		case IE_ANI_ATTACK_BACKSLASH:
			dest += "g2";
			strcpy( EquipData->Suffix, "g2" );
			Cycle = Orient / 2;
			break;
		case IE_ANI_ATTACK_SLASH:
		case IE_ANI_CAST:
		case IE_ANI_CONJURE:
			dest += "g2";
			strcpy( EquipData->Suffix, "g2" );
			Cycle = 8 + Orient / 2;
			break;
		case IE_ANI_ATTACK_JAB:
		case IE_ANI_SHOOT:
			dest += "g2";
			strcpy( EquipData->Suffix, "g2" );
			Cycle = 16 + Orient / 2;
			break;
		case IE_ANI_WALK:
		case IE_ANI_HIDE: // unknown, just a guess
			dest += "g1";
			strcpy( EquipData->Suffix, "g1" );
			Cycle = Orient / 2;
			break;
		case IE_ANI_AWAKE:
			dest += "g1";
			strcpy( EquipData->Suffix, "g1" );
			Cycle = 8 + Orient / 2;
			break;
		case IE_ANI_READY:
		case IE_ANI_HEAD_TURN: //could be wrong
			dest += "g1";
			strcpy( EquipData->Suffix, "g1" );
			Cycle = 16 + Orient / 2;
			break;
		case IE_ANI_DAMAGE:
			dest += "g1";
			strcpy( EquipData->Suffix, "g1" );
			Cycle = 24 + Orient / 2;
			break;
		case IE_ANI_GET_UP:
		case IE_ANI_EMERGE:
		case IE_ANI_PST_START:
		case IE_ANI_DIE:
			dest += "g1";
			strcpy( EquipData->Suffix, "g1" );
			Cycle = 32 + Orient / 2;
			break;
		case IE_ANI_TWITCH:
		case IE_ANI_SLEEP:
			dest += "g1";
			strcpy( EquipData->Suffix, "g1" );
			Cycle = 40 + Orient / 2;
			break;
		default:
			error("CharAnimation", "LR Animation: unhandled stance: %s %d\n", dest.c_str(), StanceID);
	}
	if (Orient > 9) {
		dest += "e";
		strcat( EquipData->Suffix, "e");
	}
	EquipData->Cycle = Cycle;
}

void CharAnimations::GetLREquipmentRef(std::string& dest, unsigned char& Cycle,
			const char* equipRef, bool /*offhand*/,
			const EquipResRefData* equip) const
{
	Cycle = equip->Cycle;
	dest = ResRefBase.CString();
	dest += equipRef[0];
	dest += equip->Suffix;
}

//Only for the ogre animation (MOGR)
void CharAnimations::AddLR3Suffix( std::string& dest, unsigned char StanceID,
	unsigned char& Cycle, unsigned char Orient) const
{
	switch (StanceID) {
		case IE_ANI_ATTACK:
		case IE_ANI_ATTACK_BACKSLASH:
			dest += "g2";
			Cycle = Orient / 2;
			break;
		case IE_ANI_ATTACK_SLASH:
		case IE_ANI_ATTACK_JAB: //there is no third attack animation
			dest += "g2";
			Cycle = 8 + Orient / 2;
			break;
		case IE_ANI_CAST:
		case IE_ANI_CONJURE:
		case IE_ANI_SHOOT:
			dest += "g3";
			Cycle = Orient / 2;
			break;
		case IE_ANI_WALK:
			dest += "g1";
			Cycle = 16 + Orient / 2;
			break;
		case IE_ANI_READY:
			dest += "g1";
			Cycle = 8 + Orient / 2;
			break;
		case IE_ANI_HEAD_TURN: //could be wrong
		case IE_ANI_AWAKE:
		case IE_ANI_HIDE:
			dest += "g1";
			Cycle = Orient / 2;
			break;
		case IE_ANI_DAMAGE:
			dest += "g3";
			Cycle = 8 + Orient / 2;
			break;
		case IE_ANI_DIE:
		case IE_ANI_GET_UP:
		case IE_ANI_EMERGE:
		case IE_ANI_PST_START:
		case IE_ANI_SLEEP:
			dest += "g3";
			Cycle = 16 + Orient / 2;
			break;
		case IE_ANI_TWITCH:
			dest += "g3";
			Cycle = 24 + Orient / 2;
			break;
		default:
			error("CharAnimation", "LR3 Animation: unhandled stance: %s %d\n", dest.c_str(), StanceID);
	}
	if (Orient > 9) {
		dest += "e";
	}
}

void CharAnimations::AddMMR2Suffix(std::string& dest, unsigned char StanceID,
	unsigned char& Cycle, unsigned char Orient) const
{
	switch (StanceID) {
		case IE_ANI_ATTACK:
		case IE_ANI_ATTACK_SLASH:
		case IE_ANI_ATTACK_BACKSLASH:
		case IE_ANI_ATTACK_JAB:
		case IE_ANI_CONJURE:
		case IE_ANI_CAST:
			dest += "a1";
			Cycle = ( Orient / 2 );
			break;

		case IE_ANI_SHOOT:
			dest += "a4";
			Cycle = ( Orient / 2 );
			break;

		case IE_ANI_AWAKE:
		case IE_ANI_READY:
			dest += "sd";
			Cycle = ( Orient / 2 );
			break;

		case IE_ANI_HEAD_TURN:
			dest += "sc";
			Cycle = ( Orient / 2 );
			break;

		case IE_ANI_DAMAGE:
			dest += "gh";
			Cycle = ( Orient / 2 );
			break;

		case IE_ANI_DIE:
			dest += "de";
			Cycle = ( Orient / 2 );
			break;

		case IE_ANI_GET_UP:
		case IE_ANI_EMERGE:
		case IE_ANI_PST_START:
			dest += "gu";
			Cycle = ( Orient / 2 );
			break;

			//Unknown... maybe only a transparency effect apply
		case IE_ANI_HIDE:
			break;

		case IE_ANI_SLEEP:
			dest += "sl";
			Cycle = ( Orient / 2 );
			break;

		case IE_ANI_TWITCH:
			dest += "tw";
			Cycle = ( Orient / 2 );
			break;

		case IE_ANI_WALK:
			dest += "wk";
			Cycle = ( Orient / 2 );
			break;
		default:
			error("CharAnimation", "MMR Animation: unhandled stance: %s %d\n", dest.c_str(), StanceID);
	}
	if (Orient > 9) {
		dest += "e";
	}
}

void CharAnimations::AddMMRSuffix(std::string& dest, unsigned char StanceID,
	unsigned char& Cycle, unsigned char Orient, bool mirror) const
{
	if (mirror) {
		Cycle = SixteenToFive[Orient];
	} else {
		Cycle = Orient / 2;
	}
	switch (StanceID) {
		case IE_ANI_ATTACK:
		case IE_ANI_ATTACK_SLASH:
		case IE_ANI_ATTACK_BACKSLASH:
			dest += "a1";
			break;

		case IE_ANI_SHOOT:
			dest += "a4";
			break;

		case IE_ANI_ATTACK_JAB:
			dest += "a2";
			break;

		case IE_ANI_AWAKE:
		case IE_ANI_READY:
			dest += "sd";
			break;

		case IE_ANI_CONJURE:
			dest += "ca";
			break;

		case IE_ANI_CAST:
			dest += "sp";
			break;

		case IE_ANI_HEAD_TURN:
			dest += "sc";
			break;

		case IE_ANI_DAMAGE:
			dest += "gh";
			break;

		case IE_ANI_DIE:
			dest += "de";
			break;

		case IE_ANI_GET_UP:
		case IE_ANI_EMERGE:
		case IE_ANI_PST_START:
			dest += "gu";
			break;

			//Unknown... maybe only a transparency effect apply
		case IE_ANI_HIDE:
			break;

		case IE_ANI_SLEEP:
			dest += "sl";
			break;

		case IE_ANI_TWITCH:
			dest += "tw";
			break;

		case IE_ANI_WALK:
			dest += "wk";
			break;
		default:
			error("CharAnimation", "MMR Animation: unhandled stance: %s %d\n", dest.c_str(), StanceID);
	}
	if (!mirror && Orient > 9) {
		dest += "e";
	}
}

void CharAnimations::AddHLSuffix(std::string& dest, unsigned char StanceID,
	unsigned char& Cycle, unsigned char Orient) const
{
	//even orientations in 'h', odd in 'l', and since the WALK animation
	//with fewer orientations is first in h, all other stances in that
	//file need to be offset by those cycles
	unsigned char offset = ((Orient % 2) ^ 1) * 8;

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
			error("CharAnimation", "HL Animation: unhandled stance: %s %d", dest.c_str(), StanceID);
	}
	if (offset) {
		dest += "hg1";
	} else {
		dest += "lg1";
	}
	if (Orient > 9) {
		dest += "e";
	}
}

void CharAnimations::PulseRGBModifiers()
{
	tick_t time = GetTicks();

	if (time - lastModUpdate <= 40)
		return;

	if (time - lastModUpdate > 400) lastModUpdate = time - 40;

	tick_t inc = (time - lastModUpdate)/40;
	
	if (GlobalColorMod.type != RGBModifier::NONE &&
		GlobalColorMod.speed > 0)
	{
		GlobalColorMod.phase += inc;
		for (bool& c : change) {
			c = true;
		}

		// reset if done
		if (GlobalColorMod.phase > 2*GlobalColorMod.speed) {
			GlobalColorMod.type = RGBModifier::NONE;
			GlobalColorMod.phase = 0;
			GlobalColorMod.speed = 0;
			GlobalColorMod.locked = false;
		}
	}

	for (size_t i = 0; i < PAL_MAX * 8; ++i) {
		if (ColorMods[i].type != RGBModifier::NONE &&
			ColorMods[i].speed > 0)
		{
			change[i>>3] = true;
			ColorMods[i].phase += inc;
			if (ColorMods[i].phase > 2*ColorMods[i].speed) {
				ColorMods[i].type = RGBModifier::NONE;
				ColorMods[i].phase = 0;
				ColorMods[i].speed = 0;
				ColorMods[i].locked = false;
			}
		}
	}

	for (size_t i = 0; i < PAL_MAX; ++i) {
		if (change[i]) {
			change[i] = false;
			SetupColors((PaletteType) i);
		}
	}

	lastModUpdate += inc*40;
}

void CharAnimations::DebugDump() const
{
	Log (DEBUG, "CharAnimations", "Anim ID   : %04x", GetAnimationID() );
	Log (DEBUG, "CharAnimations", "BloodColor: %d", GetBloodColor() );
	Log (DEBUG, "CharAnimations", "Flags     : %04x", GetFlags() );
}

}
