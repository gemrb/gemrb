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
#include "Logging/Logging.h"
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
	ResRef Suffix;
	unsigned char Cycle;
};

CharAnimations::AvatarTableLoader::AvatarTableLoader() noexcept {
	AutoTable Avatars = gamedata->LoadTable("avatars");
	if (!Avatars) {
		error("CharAnimations", "A critical animation file is missing!");
	}
	TableMgr::index_t AvatarsCount = Avatars->GetRowCount();
	table.resize(AvatarsCount);
	const DataFileMgr *resdata = core->GetResDataINI();
	for (TableMgr::index_t i = 0; i < AvatarsCount; ++i) {
		table[i].AnimID = strtounsigned<unsigned int>(Avatars->GetRowName(i).c_str());
		table[i].Prefixes[0] = Avatars->QueryField(i, AV_PREFIX1);
		table[i].Prefixes[1] = Avatars->QueryField(i, AV_PREFIX2);
		table[i].Prefixes[2] = Avatars->QueryField(i, AV_PREFIX3);
		table[i].Prefixes[3] = Avatars->QueryField(i, AV_PREFIX4);
		table[i].AnimationType = Avatars->QueryFieldUnsigned<ieByte>(i,AV_ANIMTYPE);
		table[i].CircleSize = Avatars->QueryFieldUnsigned<ieByte>(i,AV_CIRCLESIZE);
		table[i].PaletteType = Avatars->QueryField(i, AV_USE_PALETTE);

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
			std::string section = fmt::to_string(i);
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
		TableMgr::index_t rows = blood->GetRowCount();
		for(TableMgr::index_t i = 0; i < rows; ++i) {
			char value = 0;
			unsigned int flags = 0;
			unsigned int rmin = 0;
			unsigned int rmax = 0xffff;

			valid_signednumber(blood->QueryField(i,0).c_str(), value);
			valid_unsignednumber(blood->QueryField(i,1).c_str(), rmin);
			valid_unsignednumber(blood->QueryField(i,2).c_str(), rmax);
			valid_unsignednumber(blood->QueryField(i,3).c_str(), flags);
			if (rmin > rmax || rmax > 0xffff) {
				Log(ERROR, "CharAnimations", "Invalid bloodclr entry: {:#x} {:#x}-{:#x} ", value, rmin, rmax);
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
		TableMgr::index_t rows = walk->GetRowCount();
		for (TableMgr::index_t i = 0; i < rows; ++i) {
			ResRef value;
			unsigned int rmin = 0;
			unsigned int rmax = 0xffff;
			ieByte range = 0;

			value = walk->QueryField(i, 0);
			valid_unsignednumber(walk->QueryField(i,1).c_str(), rmin);
			valid_unsignednumber(walk->QueryField(i,2).c_str(), rmax);
			valid_unsignednumber(walk->QueryField(i,3).c_str(), range);
			if (IsStar(value)) {
				value.Reset();
				range = 0;
			}
			if (rmin > rmax || rmax > 0xffff) {
				Log(ERROR, "CharAnimations", "Invalid walksnd entry: {:#x} {:#x}-{:#x} ", range, rmin, rmax);
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
		TableMgr::index_t rows = stances->GetRowCount();
		for (TableMgr::index_t i = 0; i < rows; i++) {
			unsigned int id = 0;
			unsigned int s1 = 0;
			unsigned int s2 = 0;
			valid_unsignednumber(stances->GetRowName(i).c_str(), id);
			valid_unsignednumber(stances->QueryField(i, 0).c_str(), s1);
			valid_unsignednumber(stances->QueryField(i, 1).c_str(), s2);

			if (s1 >= MAX_ANIMS || s2 >= MAX_ANIMS) {
				Log(ERROR, "CharAnimations", "Invalid stances entry: {:#x} {} {}", id, s1, s2);
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

	AutoTable avatarShadows = gamedata->LoadTable("shadows");
	if (avatarShadows) {
		TableMgr::index_t rows = avatarShadows->GetRowCount();
		for (TableMgr::index_t i = 0; i < rows; ++i) {
			unsigned int id = 0;
			valid_unsignednumber(avatarShadows->GetRowName(i).c_str(), id);

			for (auto& row : table) {
				if (id < row.AnimID) break;
				if (id == row.AnimID) {
					row.ShadowAnimation = avatarShadows->QueryField(i, 0);
					break;
				}
			}
		}
	}

	AutoTable wingBuffet = gamedata->LoadTable("wingbuff");
	if (wingBuffet) {
		TableMgr::index_t rows = wingBuffet->GetRowCount();
		for (TableMgr::index_t i = 0; i < rows; ++i) {
			unsigned int id = 0;
			unsigned int flags = wingBuffet->QueryFieldUnsigned<unsigned int>(i, 0);
			valid_unsignednumber(wingBuffet->GetRowName(i).c_str(), id);

			for (auto& row : table) {
				if (id < row.AnimID) break;
				if (id == row.AnimID) {
					row.Flags |= flags;
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

static const ResRef EmptyRef;
const ResRef& CharAnimations::GetPaletteType() const
{
	if (AvatarsRowNum == ~0U) return EmptyRef;
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
void CharAnimations::MaybeUpdateMainPalette(const Animation& anim) {
	if (previousStanceID != stanceID && GetAnimType() != IE_ANI_TWO_PIECE) {
		// Test if the palette in question is actually different to the one loaded.
		if (*PartPalettes[PAL_MAIN] != *anim.GetFrame(0)->GetPalette()) {
			PaletteResRef[PAL_MAIN].Reset();

			PartPalettes[PAL_MAIN] = MakeHolder<Palette>(*anim.GetFrame(0)->GetPalette());
			SetupColors(PAL_MAIN);
		}
	}
}

const ResRef &CharAnimations::GetWalkSound() const
{
	if (AvatarsRowNum == ~0U) return EmptyRef;
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
	RangedType = Clamp<ieByte>(rt, 0, 2);
}

void CharAnimations::SetWeaponType(unsigned char wt)
{
	if (wt != WeaponType) {
		WeaponType = wt;
		DropAnims();
	}
}

void CharAnimations::SetHelmetRef(AnimRef ref)
{
	HelmetRef = ref;

	// Only drop helmet anims?
	// Note: this doesn't happen "often", so this isn't a performance
	//       bottleneck. (wjp)
	DropAnims();
	PartPalettes[PAL_HELMET] = nullptr;
	ModPartPalettes[PAL_HELMET] = nullptr;
}

void CharAnimations::SetWeaponRef(AnimRef ref)
{
	WeaponRef = ref;

	// TODO: Only drop weapon anims?
	DropAnims();
	PartPalettes[PAL_WEAPON] = nullptr;
	ModPartPalettes[PAL_WEAPON] = nullptr;
}

void CharAnimations::SetOffhandRef(AnimRef ref)
{
	OffhandRef = ref;

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
	GetAnimation(stanceID, S);
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
	Holder<Palette> pal = PartPalettes[type];

	if (!pal) {
		return;
	}

	if (!Colors) {
		return;
	}
	
	const ResRef& paletteType = GetPaletteType();

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
			*ModPartPalettes[PAL_MAIN] = SetupGlobalRGBModification(PartPalettes[PAL_MAIN], GlobalColorMod);
		} else {
			ModPartPalettes[PAL_MAIN] = nullptr;
		}
	} else if (type <= PAL_MAIN_5 && paletteType != "0" && !paletteType.IsEmpty()) {
		//handling special palettes like MBER_BL (black bear)
		if (paletteType != "1") {
			if (GetAnimType()==IE_ANI_NINE_FRAMES) {
				PaletteResRef[type].Format("{:.4}_{:.2}{:c}", ResRefBase, paletteType, '1' + type);
			} else {
				if (ResRefBase == "MFIE") { // hack for magic golems
					PaletteResRef[type].Format("{:.4}{:.2}B", ResRefBase, paletteType);
				} else {
					PaletteResRef[type].Format("{:.4}_{:.2}", ResRefBase, paletteType);
				}
			}
			Holder<Palette> tmppal = gamedata->GetPalette(PaletteResRef[type]);
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
			*ModPartPalettes[type] = SetupGlobalRGBModification(PartPalettes[type], GlobalColorMod);
		} else {
			ModPartPalettes[type] = nullptr;
		}
	} else {
		*pal = SetupPaperdollColours(Colors, type);
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
				*ModPartPalettes[type] = SetupGlobalRGBModification(PartPalettes[type], GlobalColorMod);
			} else {
				*ModPartPalettes[type] = SetupRGBModification(PartPalettes[type],ColorMods, type);
			}
		} else {
			ModPartPalettes[type] = nullptr;
		}
	}
}

Holder<Palette> CharAnimations::GetPartPalette(int part) const
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

Holder<Palette> CharAnimations::GetShadowPalette() const {
	return shadowPalette;
}

CharAnimations::CharAnimations(unsigned int AnimID, ieDword ArmourLevel)
{
	for (bool& c : change) {
		c = true;
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
	if (core->HasFeature(GFFlags::ONE_BYTE_ANIMID) ) {
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
	Log(ERROR, "CharAnimations", "Invalid or nonexistent avatar entry: {:#x}", AnimID);
}

//we have to drop them when armourlevel changes
void CharAnimations::DropAnims()
{
	Anims.fill({});
}

CharAnimations::~CharAnimations(void)
{
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

const CharAnimations::PartAnim* CharAnimations::GetAnimation(unsigned char Stance, orient_t Orient)
{
	if (Stance >= MAX_ANIMS) {
		error("CharAnimation", "Illegal stance ID");
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
			Orient = S;
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
			Log(MESSAGE, "CharAnimation", "Invalid Stance: {}", stanceID);
			break;
	}

	// IE_ANI_RUN == IE_ANI_HIDE, so either exclude pst or limit to anim types where it matters
	if (AnimType == IE_ANI_TWO_PIECE && stanceID == IE_ANI_HIDE) {
		nextStanceID = IE_ANI_READY;
		autoSwitchOnEnd = true;
	}

	stanceID = MaybeOverrideStance(stanceID);

	bool lastFrameOnly = false;
	//pst (and some other) animations don't have separate animations for sleep/die
	if (Stance == IE_ANI_TWITCH &&
		(AnimType >= IE_ANI_PST_ANIMATION_1 || MaybeOverrideStance(IE_ANI_DIE) == stanceID))
	{
		lastFrameOnly = true;
	}

	PartAnim& anims = Anims[stanceID][Orient];
	if (!anims.empty()) {
		const SharedAnim& anim = anims[0];
		MaybeUpdateMainPalette(*anim);
		previousStanceID = stanceID;

		if (lastFrameOnly) {
			anim->SetFrame(anim->GetFrameCount() - 1);
		}

		return &anims;
	}

	int partCount = GetTotalPartCount();
	int actorPartCount = GetActorPartCount();
	if (partCount <= 0) return nullptr;
	
	PartAnim newparts(partCount);

	EquipResRefData equipment;
	for (int part = 0; part < partCount; ++part)
	{
		// NewResRef is based on the prefix ResRef and various suffixes
		ResRef NewResRef;
		unsigned char Cycle = 0;
		if (part < actorPartCount) {
			// Character animation parts

			equipment = EquipResRefData();

			//we need this long for special anims
			NewResRef = ResRefBase;
			GetAnimResRef(stanceID, Orient, NewResRef, Cycle, part, equipment);
		} else {
			// Equipment animation parts

			if (GetSize() == 0) continue;

			if (part == actorPartCount) {
				if (WeaponRef[0] == 0) continue;
				// weapon
				GetEquipmentResRef(WeaponRef, false, NewResRef, Cycle, equipment);
			} else if (part == actorPartCount+1) {
				if (OffhandRef[0] == 0) continue;
				if (WeaponType == IE_ANI_WEAPON_2H) continue;
				// off-hand
				if (WeaponType == IE_ANI_WEAPON_1H) {
					GetEquipmentResRef(OffhandRef, false, NewResRef, Cycle, equipment);
				} else { // IE_ANI_WEAPON_2W
					GetEquipmentResRef(OffhandRef, true, NewResRef, Cycle, equipment);
				}
			} else if (part == actorPartCount+2) {
				if (HelmetRef[0] == 0) continue;
				// helmet
				GetEquipmentResRef(HelmetRef, false, NewResRef, Cycle, equipment);
			}
		}

		auto af = gamedata->GetFactoryResourceAs<const AnimationFactory>(NewResRef, IE_BAM_CLASS_ID);

		if (!af) {
			if (part < actorPartCount) {
				Log(ERROR, "CharAnimations", "Couldn't create animationfactory: {} ({:#x})",
						NewResRef, GetAnimationID());
				return nullptr;
			} else {
				// not fatal if animation for equipment is missing
				continue;
			}
		}

		SharedAnim newanim(af->GetCycle(Cycle));

		if (!newanim) {
			if (part < actorPartCount) {
				Log(ERROR, "CharAnimations", "Couldn't load animation: {}, cycle {}", NewResRef, Cycle);
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
				PartPalettes[ptype] = MakeHolder<Palette>(*newanim->GetFrame(0)->GetPalette());
				// ...and setup the colours properly
				SetupColors(ptype);
			} else if (ptype == PAL_MAIN) {
				MaybeUpdateMainPalette(*newanim);
			}
		} else if (part == actorPartCount) {
			if (!PartPalettes[PAL_WEAPON]) {
				PartPalettes[PAL_WEAPON] = MakeHolder<Palette>(*newanim->GetFrame(0)->GetPalette());
				SetupColors(PAL_WEAPON);
			}
		} else if (part == actorPartCount+1) {
			if (!PartPalettes[PAL_OFFHAND]) {
				PartPalettes[PAL_OFFHAND] = MakeHolder<Palette>(*newanim->GetFrame(0)->GetPalette());
				SetupColors(PAL_OFFHAND);
			}
		} else if (part == actorPartCount+2) {
			if (!PartPalettes[PAL_HELMET]) {
				PartPalettes[PAL_HELMET] = MakeHolder<Palette>(*newanim->GetFrame(0)->GetPalette());
				SetupColors(PAL_HELMET);
			}
		}

		//animation is affected by game flags
		newanim->gameAnimation = true;
		if (lastFrameOnly) {
			newanim->SetFrame(newanim->GetFrameCount() - 1);
		} else {
			newanim->SetFrame(0);
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
				newanim->Flags |= A_ANI_PLAYONCE;
				break;
			case IE_ANI_EMERGE:
			case IE_ANI_GET_UP:
				newanim->playReversed = true;
				newanim->Flags |= A_ANI_PLAYONCE;
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
					newanim->MirrorAnimation(BlitFlags::MIRRORX);
				}
				break;
			default:
				break;
		}
		
		newparts[part] = newanim;

		// make animarea of part 0 encompass the animarea of the other parts
		if (part > 0)
			newparts[0]->AddAnimArea(newanim.get());

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
			Anims[stanceID][Orient].swap(newparts);
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
			Orient = ReduceToHalf(Orient);
			Anims[stanceID][Orient] = newparts;
			Anims[stanceID][NextOrientation(Orient)].swap(newparts);
			break;
		case IE_ANI_FOUR_FILES_3:
			//only 8 orientations for WALK
			if (stanceID == IE_ANI_WALK) {
				Orient = ReduceToHalf(Orient);
				Anims[stanceID][Orient] = newparts;
				Anims[stanceID][NextOrientation(Orient)].swap(newparts);
			} else {
				Anims[stanceID][Orient].swap(newparts);
			}
			break;
		case IE_ANI_TWO_FILES_4:
			for (auto& anim : Anims) {
				for (auto& orient : anim) {
					orient = newparts;
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
					Anims[stanceID][Orient].swap(newparts);
					break;
				default:
					Orient = ReduceToHalf(Orient);
					Anims[stanceID][Orient] = newparts;
					Anims[stanceID][NextOrientation(Orient)].swap(newparts);
					break;
			}
			break;
		default:
			error("CharAnimations", "Unknown animation type");
	}
	previousStanceID = stanceID;

	return &Anims[stanceID][Orient];
}

const CharAnimations::PartAnim* CharAnimations::GetShadowAnimation(unsigned char stance, orient_t orientation) {
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
		case IE_ANI_DAMAGE:
		case IE_ANI_TWITCH:
			break;
		default:
			return nullptr;
	}

	if (!shadowAnimations[newStanceID][orientation].empty()) {
		return &shadowAnimations[newStanceID][orientation];
	}

	if (AvatarTable[AvatarsRowNum].ShadowAnimation.IsEmpty()) {
		return nullptr;
	}

	int partCount = GetTotalPartCount();
	PartAnim newparts(partCount);

	ResRef shadowName = AvatarTable[AvatarsRowNum].ShadowAnimation;

	EquipResRefData dummy;
	unsigned char cycle = 0;
	AddMHRSuffix(shadowName, newStanceID, cycle, orientation, dummy);

	auto af = gamedata->GetFactoryResourceAs<const AnimationFactory>(shadowName, IE_BAM_CLASS_ID);
	if (!af) {
		return nullptr;
	}

	SharedAnim animation(af->GetCycle(cycle));
	if (!animation) {
		return nullptr;
	}
	
	newparts[0] = animation;

	if (!shadowPalette) {
		shadowPalette = MakeHolder<Palette>(*animation->GetFrame(0)->GetPalette());
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
	newparts[0]->AddAnimArea(animation.get());

	orientation = ReduceToHalf(orientation);
	shadowAnimations[newStanceID][orientation] = newparts;
	shadowAnimations[newStanceID][orientation + 1].swap(newparts);

	return &shadowAnimations[newStanceID][orientation];
}

static const int one_file[MAX_ANIMS] = {2, 1, 0, 0, 2, 3, 0, 1, 0, 4, 1, 0, 0, 0, 3, 1, 4, 4, 4};

void CharAnimations::GetAnimResRef(unsigned char StanceID,
					 orient_t Orient, ResRef& NewResRef, unsigned char& Cycle,
					 int Part, EquipResRefData& EquipData) const
{
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
			NewResRef.Append("g1");
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
			NewResRef.Format("{}STD{}{}{}{}", ResRefBase[0], ResRefBase[1], ResRefBase[2], ResRefBase[3], ResRefBase[4]);
			Cycle = SixteenToFive[Orient];
			break;
		case IE_ANI_PST_GHOST: // pst static animations
			//still doesn't handle the second cycle of the golem anim
			Cycle = 0;
			NewResRef = AvatarTable[AvatarsRowNum].Prefixes[Part];
			break;
		default:
			error("CharAnimations", "Unknown animation type in avatars.2da row: {}", AvatarsRowNum);
	}
}

void CharAnimations::GetEquipmentResRef(AnimRef equipRef, bool offhand,
										ResRef& dest, unsigned char& Cycle,
										const EquipResRefData& equip) const
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
			error("CharAnimations", "Unsupported animation type for equipment animation.");
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


void CharAnimations::AddPSTSuffix(ResRef& dest, unsigned char StanceID,
								  unsigned char& Cycle, orient_t Orient) const
{
	const char *Prefix;
	static const char prefixes[2][4] = {"sf2", "sf1"};

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

			for (int i = RandomFlip(); i < 2; ++i) {
				dest.Format("{}{}{}", ResRefBase[0], prefixes[i], ResRefBase.begin() + 1);
				if (gamedata->Exists(dest, IE_BAM_CLASS_ID)) {
					return;
				}
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
	dest.Format("{}{}{}", ResRefBase[0], Prefix, ResRefBase.begin() + 1);
}

void CharAnimations::AddVHR2Suffix(ResRef& dest, unsigned char StanceID,
								   unsigned char& Cycle, orient_t Orient) const
{
	Cycle=SixteenToNine[Orient];

	switch (StanceID) {
		case IE_ANI_ATTACK: //temporarily
		case IE_ANI_ATTACK_BACKSLASH:
			dest.Append("g21");
			Cycle+=9;
			break;

		case IE_ANI_ATTACK_SLASH:
			dest.Append("g2");
			break;

		case IE_ANI_ATTACK_JAB:
			dest.Append("g22");
			Cycle+=18;
			break;

		case IE_ANI_CAST: //looping
			dest.Append("g25");
			Cycle+=45;
			break;

		case IE_ANI_CONJURE://ending
			dest.Append("g26");
			Cycle+=54;
			break;

		case IE_ANI_SHOOT:
			dest.Append("g24");
			Cycle+=27;
			break;

		case IE_ANI_HEAD_TURN:
		case IE_ANI_AWAKE:
			dest.Append("g12");
			Cycle+=18;
			break;

		case IE_ANI_SLEEP:
		case IE_ANI_TWITCH:
			dest.Append("g15");
			Cycle+=45;
			break;

		case IE_ANI_DIE:
		case IE_ANI_EMERGE:
		case IE_ANI_GET_UP:
		case IE_ANI_PST_START:
			dest.Append("g14");
			Cycle+=36;
			break;

		case IE_ANI_DAMAGE:
			dest.Append("g13");
			Cycle+=27;
			break;

		case IE_ANI_READY:
			dest.Append("g1");
			Cycle+=9;
			break;

		case IE_ANI_WALK:
			dest.Append("g11");
			break;

		case IE_ANI_HIDE:
			dest.Append("g22");
			break;
		default:
			error("CharAnimation", "VHR2 Animation: unhandled stance: {} {}", dest, StanceID);
	}
}

void CharAnimations::AddVHR3Suffix(ResRef& dest, unsigned char StanceID,
								   unsigned char& Cycle, orient_t Orient) const
{
	Cycle=SixteenToNine[Orient];

	switch (StanceID) {
		case IE_ANI_ATTACK: //temporarily
		case IE_ANI_ATTACK_BACKSLASH:
			dest.Append("g21");
			Cycle+=9;
			break;

		case IE_ANI_ATTACK_SLASH:
			dest.Append("g2");
			break;

		case IE_ANI_ATTACK_JAB:
		case IE_ANI_CONJURE://ending
			dest.Append("g22");
			Cycle+=18;
			break;

		case IE_ANI_CAST: //looping
			dest.Append("g22");
			Cycle+=27;
			break;

		case IE_ANI_SHOOT:
			dest.Append("g23");
			Cycle+=27;
			break;

		case IE_ANI_HEAD_TURN:
		case IE_ANI_AWAKE:
		case IE_ANI_HIDE:
			dest.Append("g12");
			Cycle+=18;
			break;

		case IE_ANI_SLEEP:
		case IE_ANI_TWITCH:
			dest.Append("g15");
			Cycle+=45;
			break;

		case IE_ANI_DIE:
		case IE_ANI_EMERGE:
		case IE_ANI_GET_UP:
		case IE_ANI_PST_START:
			dest.Append("g14");
			Cycle+=36;
			break;

		case IE_ANI_DAMAGE:
			dest.Append("g13");
			Cycle+=27;
			break;

		case IE_ANI_READY:
			dest.Append("g1");
			Cycle+=9;
			break;

		case IE_ANI_WALK:
			dest.Append("g11");
			break;
		default:
			error("CharAnimation", "VHR3 Animation: unhandled stance: {} {}", dest, StanceID);
	}
}

// Note: almost like SixSuffix
void CharAnimations::AddFFSuffix(ResRef& dest, unsigned char StanceID,
								 unsigned char& Cycle, orient_t Orient, int Part) const
{
	Cycle=SixteenToNine[Orient];
	switch (StanceID) {
		case IE_ANI_WALK:
			dest.Append("g1");
			break;

		case IE_ANI_ATTACK:
		case IE_ANI_ATTACK_SLASH:
		case IE_ANI_SHOOT:
			dest.Append("g3");
			break;

		case IE_ANI_ATTACK_BACKSLASH:
			dest.Append("g3");
			Cycle += 16;
			break;

		case IE_ANI_ATTACK_JAB:
		case IE_ANI_CAST:
		case IE_ANI_CONJURE:
			dest.Append("g3");
			Cycle += 32;
			break;

		case IE_ANI_HEAD_TURN: //could be wrong
		case IE_ANI_HIDE: //could be wrong
		case IE_ANI_AWAKE:
			dest.Append("g2");
			break;

		case IE_ANI_READY:
			dest.Append("g2");
			Cycle += 16;
			break;

		case IE_ANI_DAMAGE:
			dest.Append("g2");
			Cycle += 32;
			break;

		case IE_ANI_DIE:
		case IE_ANI_GET_UP:
		case IE_ANI_EMERGE:
		case IE_ANI_PST_START:
			dest.Append("g2");
			Cycle += 48;
			break;

		case IE_ANI_SLEEP:
		case IE_ANI_TWITCH:
			dest.Append("g2");
			Cycle += 64;
			break;

		default:
			error("CharAnimation", "Four frames Animation: unhandled stance: {} {}", dest, StanceID);

	}
	dest[dest.length()] = static_cast<char>(Part + '1');
}

// demigorgon's 4-part animation
void CharAnimations::AddFF2Suffix(ResRef& dest, unsigned char StanceID,
								  unsigned char& Cycle, orient_t Orient, int Part) const
{
	Cycle = SixteenToNine[Orient];
	switch (StanceID) {
		case IE_ANI_HEAD_TURN:
			dest.Append("g101");
			break;

		case IE_ANI_READY:
		case IE_ANI_AWAKE:
			dest.Append("g102");
			Cycle += 9;
			break;

		case IE_ANI_WALK:
			dest.Append("g101");
			break;

		case IE_ANI_CAST:
		case IE_ANI_CONJURE:
			dest.Append("g205");
			Cycle += 45;
			break;

		case IE_ANI_ATTACK:
		case IE_ANI_ATTACK_SLASH:
			dest.Append("g206");
			Cycle += 54;
			break;

		case IE_ANI_ATTACK_BACKSLASH:
			dest.Append("g202");
			break;

		case IE_ANI_ATTACK_JAB:
			dest.Append("g203");
			Cycle += 18;
			break;

		case IE_ANI_DIE:
		case IE_ANI_GET_UP:
		case IE_ANI_EMERGE:
			dest.Append("g104");
			Cycle += 36;
			break;

		case IE_ANI_SLEEP:
		case IE_ANI_TWITCH:
		case IE_ANI_DAMAGE:
			dest.Append("g103");
			Cycle += 27;
			break;

		default:
			error("CharAnimation", "Four frames 2 Animation: unhandled stance: {} {}", dest, StanceID);

	}

	// change the placeholder 0 to a part number
	dest[dest.length() - 2] = static_cast<char>(Part + '1');
}

void CharAnimations::AddNFSuffix(ResRef& dest, unsigned char StanceID,
								 unsigned char& Cycle, orient_t Orient, int Part) const
{
	Cycle = SixteenToNine[Orient];
	
	std::string prefix = fmt::format("{}{}{}{}{}", dest, StancePrefix[StanceID], (Part + 1) % 100, CyclePrefix[StanceID], Cycle);
	if (prefix.length() > 8) {
		prefix.resize(8);
	}
	StringToLower(prefix.begin(), prefix.end(), dest.begin());
	Cycle=(ieByte) (Cycle+CycleOffset[StanceID]);
}

//Attack
//h1, h2, w2
//static const char *SlashPrefix[]={"a1","a4","a7"};
//static const char *BackPrefix[]={"a2","a5","a8"};
//static const char *JabPrefix[]={"a3","a6","a9"};
using prefix_t = char[3];
static const prefix_t SlashPrefix[] = { "a1", "a2", "a7" };
static const prefix_t BackPrefix[] = { "a3", "a4", "a8" };
static const prefix_t JabPrefix[] = { "a5", "a6", "a9" };
static const prefix_t RangedPrefix[] = { "sa", "sx", "ss" };
static const prefix_t RangedPrefixOld[] = { "sa", "sx", "a1" };

void CharAnimations::AddVHRSuffix(ResRef& dest, unsigned char StanceID,
								  unsigned char& Cycle, orient_t Orient, EquipResRefData& EquipData) const
{
	Cycle = SixteenToNine[Orient];
	switch (StanceID) {
		case IE_ANI_ATTACK:
		case IE_ANI_ATTACK_SLASH:
			dest.Append(SlashPrefix[WeaponType]);
			EquipData.Suffix = SlashPrefix[WeaponType];
			break;

		case IE_ANI_ATTACK_BACKSLASH:
			dest.Append(BackPrefix[WeaponType]);
			EquipData.Suffix = BackPrefix[WeaponType];
			break;

		case IE_ANI_ATTACK_JAB:
			dest.Append(JabPrefix[WeaponType]);
			EquipData.Suffix = JabPrefix[WeaponType];
			break;

		case IE_ANI_AWAKE:
			dest.Append("g17");
			EquipData.Suffix = "g1";
			Cycle += 63;
			break;

		case IE_ANI_CAST: //looping
			dest.Append("ca");
			EquipData.Suffix = "ca";
			break;

		case IE_ANI_CONJURE: //ending
			dest.Append("ca");
			EquipData.Suffix = "ca";
			Cycle += 9;
			break;

		case IE_ANI_DAMAGE:
			dest.Append("g14");
			EquipData.Suffix = "g1";
			Cycle += 36;
			break;

		case IE_ANI_DIE:
			dest.Append("g15");
			EquipData.Suffix = "g1";
			Cycle += 45;
			break;
			//I cannot find an emerge animation...
			//Maybe is Die reversed
		case IE_ANI_GET_UP:
		case IE_ANI_EMERGE:
		case IE_ANI_PST_START:
			dest.Append("g19");
			EquipData.Suffix = "g1";
			Cycle += 81;
			break;

		case IE_ANI_HEAD_TURN:
			if (RandomFlip()) {
				dest.Append("g12");
				Cycle += 18;
			} else {
				dest.Append("g18");
				Cycle += 72;
			}
			EquipData.Suffix = "g1";
			break;

			//Unknown... maybe only a transparency effect apply
		case IE_ANI_HIDE:
			break;

		case IE_ANI_READY:
			if ( WeaponType == IE_ANI_WEAPON_2H ) {
				dest.Append("g13");
				Cycle += 27;
			} else {
				dest.Append("g1");
				Cycle += 9;
			}
			EquipData.Suffix = "g1";
			break;
			//This depends on the ranged weapon equipped
		case IE_ANI_SHOOT:
			dest.Append(RangedPrefix[RangedType]);
			EquipData.Suffix = RangedPrefix[RangedType];
			break;

		case IE_ANI_SLEEP:
		case IE_ANI_TWITCH:
			dest.Append("g16");
			EquipData.Suffix = "g1";
			Cycle += 54;
			break;

		case IE_ANI_WALK:
			dest.Append("g11");
			EquipData.Suffix = "g1";
			break;

		default:
			error("CharAnimation", "VHR Animation: unhandled stance: {} {}", dest, StanceID);
	}
	EquipData.Cycle = Cycle;
}

void CharAnimations::GetVHREquipmentRef(ResRef& dest, unsigned char& Cycle,
										AnimRef equipRef, bool offhand,
										const EquipResRefData& equip) const
{
	Cycle = equip.Cycle;

	dest.Format("wq{}{}", GetSize(), equipRef);
	if (offhand) {
		dest.Append("o");
	}
	dest.Append(equip.Suffix);
}

void CharAnimations::AddSixSuffix(ResRef& dest, unsigned char StanceID,
								  unsigned char& Cycle, orient_t Orient) const
{
	switch (StanceID) {
		case IE_ANI_WALK:
			dest.Append("g1");
			Cycle = Orient;
			break;

		case IE_ANI_ATTACK:
		case IE_ANI_ATTACK_SLASH:
			dest.Append("g3");
			Cycle = Orient;
			break;

		case IE_ANI_ATTACK_BACKSLASH:
			dest.Append("g3");
			Cycle = 16 + Orient;
			break;

		case IE_ANI_ATTACK_JAB:
			dest.Append("g3");
			Cycle = 32 + Orient;
			break;

		case IE_ANI_HEAD_TURN: //could be wrong
		case IE_ANI_AWAKE:
		case IE_ANI_CAST: //could be wrong
		case IE_ANI_CONJURE:
			dest.Append("g2");
			Cycle = 0 + Orient;
			break;

		case IE_ANI_READY:
		case IE_ANI_HIDE: //could be wrong
			dest.Append("g2");
			Cycle = 16 + Orient;
			break;

		case IE_ANI_DAMAGE:
			dest.Append("g2");
			Cycle = 32 + Orient;
			break;

		case IE_ANI_DIE:
		case IE_ANI_GET_UP:
		case IE_ANI_EMERGE:
		case IE_ANI_PST_START:
			dest.Append("g2");
			Cycle = 48 + Orient;
			break;

		case IE_ANI_TWITCH:
		case IE_ANI_SLEEP:
			dest.Append("g2");
			Cycle = 64 + Orient;
			break;

		default:
			error("CharAnimation", "Six Animation: unhandled stance: {} {}", dest, StanceID);

	}
	if (Orient>9) {
		dest.Append("e");
	}
}

void CharAnimations::AddLR2Suffix(ResRef& dest, unsigned char StanceID,
								  unsigned char& Cycle, orient_t Orient) const
{
	Orient = orient_t(Orient / 2);

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
			error("CharAnimation", "LR2 Animation: unhandled stance: {} {}", dest, StanceID);
	}
	if (Orient>=4) {
		dest.Append("g1e");
	} else {
		dest.Append("g1");
	}
}

void CharAnimations::AddMHRSuffix(ResRef& dest, unsigned char StanceID,
								  unsigned char& Cycle, orient_t Orient, EquipResRefData& EquipData) const
{
	Orient = orient_t(Orient / 2);

	switch (StanceID) {
		case IE_ANI_ATTACK:
		case IE_ANI_ATTACK_SLASH:
			dest.Append(SlashPrefix[WeaponType]);
			EquipData.Suffix = SlashPrefix[WeaponType];
			Cycle = Orient;
			break;

		case IE_ANI_ATTACK_BACKSLASH:
			dest.Append(BackPrefix[WeaponType]);
			EquipData.Suffix = BackPrefix[WeaponType];
			Cycle = Orient;
			break;

		case IE_ANI_ATTACK_JAB:
			dest.Append(JabPrefix[WeaponType]);
			EquipData.Suffix = JabPrefix[WeaponType];
			Cycle = Orient;
			break;

		case IE_ANI_READY:
			dest.Append("g1");
			EquipData.Suffix = "g1";
			if ( WeaponType == IE_ANI_WEAPON_2W ) {
				Cycle = 24 + Orient;
			} else {
				Cycle = 8 + Orient;
			}
			break;

		case IE_ANI_CAST://looping
			dest.Append("ca");
			EquipData.Suffix = "ca";
			Cycle = 8 + Orient;
			break;

		case IE_ANI_CONJURE://ending
			dest.Append("ca");
			EquipData.Suffix = "ca";
			Cycle = Orient;
			break;

		case IE_ANI_DAMAGE:
			dest.Append("g1");
			EquipData.Suffix = "g1";
			Cycle = 40 + Orient;
			break;

		case IE_ANI_DIE:
		case IE_ANI_GET_UP:
		case IE_ANI_PST_START:
		case IE_ANI_EMERGE: // I cannot find an emerge animation... Maybe it is Die reversed
			dest.Append("g1");
			EquipData.Suffix = "g1";
			Cycle = 48 + Orient;
			break;
		case IE_ANI_HEAD_TURN:
			dest.Append("g1");
			EquipData.Suffix = "g1";
			Cycle = 32 + Orient;
			break;

			//Unknown... maybe only a transparency effect apply
		case IE_ANI_HIDE:
			break;

		case IE_ANI_AWAKE:
			dest.Append("g1");
			EquipData.Suffix = "g1";
			Cycle = 16 + Orient;
			break;

			//This depends on the ranged weapon equipped
		case IE_ANI_SHOOT:
			dest.Append(RangedPrefixOld[RangedType]);
			EquipData.Suffix = RangedPrefixOld[RangedType];
			Cycle = Orient;
			break;

		case IE_ANI_SLEEP:
			dest.Append("g1");
			EquipData.Suffix = "g1";
			Cycle = 64 + Orient;
			break;

		case IE_ANI_TWITCH:
			dest.Append("g1");
			EquipData.Suffix = "g1";
			Cycle = 56 + Orient;
			break;

		case IE_ANI_WALK:
			dest.Append("g1");
			EquipData.Suffix = "g1";
			Cycle = Orient;
			break;
		default:
			error("CharAnimation", "MHR Animation: unhandled stance: {} {}", dest, StanceID);
	}
	if (Orient>=5) {
		dest.Append("e");
		EquipData.Suffix.Append("e");
	}
	// NOTE: the two shadow animations (cshd, sshd) also have x-suffixed files,
	// but those are used (instead of the eastern ones) only if sprite
	// mirroring is on. "Mirror sprites" in bgee.lua, probably what was
	// SoftMirrorBlt in the original ini. Pretty useless.
	EquipData.Cycle = Cycle;
}

void CharAnimations::GetMHREquipmentRef(ResRef& dest, unsigned char& Cycle,
										AnimRef equipRef, bool offhand,
										const EquipResRefData& equip) const
{
	Cycle = equip.Cycle;
	if (offhand) {
		//i think there is no offhand stuff for bg1, lets use the bg2 equivalent here?
		dest.Format("wq{}{}o{}", GetSize(), equipRef, equip.Suffix);
	} else {
		dest.Format("wp{}{}{}", GetSize(), equipRef, equip.Suffix);
	}
}

void CharAnimations::AddTwoFileSuffix(ResRef& dest, unsigned char StanceID,
									  unsigned char& Cycle, orient_t Orient) const
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
	dest.Append("g1");
	if (Orient > 9) {
		dest.Append("e");
	}
}

void CharAnimations::AddTwoFiles5Suffix(ResRef& dest, unsigned char StanceID,
										unsigned char& Cycle, orient_t Orient) const
{
	Cycle=SixteenToNine[Orient];

	switch(StanceID) {
		case IE_ANI_WALK:
			dest.Append("g1");
			break;
		case IE_ANI_READY:
			Cycle += 9;
			dest.Append("g1");
			break;
		case IE_ANI_HEAD_TURN:
			Cycle += 18;
			dest.Append("g1");
			break;
		case IE_ANI_DAMAGE:
			Cycle += 27;
			dest.Append("g1");
			break;
		case IE_ANI_DIE:
			Cycle += 36;
			dest.Append("g1");
			break;
		case IE_ANI_SLEEP:
		case IE_ANI_TWITCH:
			Cycle += 45;
			dest.Append("g1");
			break;
		// dead but not quite dead...
		//	Cycle += 54;
		//	suffix = "g1";
		//	break;
		case IE_ANI_GET_UP:
		case IE_ANI_EMERGE:
			Cycle += 63;
			dest.Append("g1");
			break;
		case IE_ANI_ATTACK:
			dest.Append("g2");
			break;
		case IE_ANI_SHOOT:
			Cycle += 9;
			dest.Append("g2");
			break;
		// yet another attack
		//	Cycle += 18;
		//	suffix = "g2";
		//	break;
		case IE_ANI_ATTACK_BACKSLASH:
			Cycle += 27;
			dest.Append("g2");
			break;
		case IE_ANI_ATTACK_JAB:
			Cycle += 36;
			dest.Append("g2");
			break;
		case IE_ANI_CONJURE:
			Cycle += 45;
			dest.Append("g2");
			break;
		case IE_ANI_ATTACK_SLASH:
		case IE_ANI_CAST:
			Cycle += 54;
			dest.Append("g2");
			break;
		default:
			Cycle += 18;
			dest.Append("g1");
	}
}

void CharAnimations::AddLRSuffix2(ResRef& dest, unsigned char StanceID,
								  unsigned char& Cycle, orient_t Orient, EquipResRefData& EquipData) const
{
	switch (StanceID) {
		case IE_ANI_ATTACK:
		case IE_ANI_ATTACK_BACKSLASH:
		case IE_ANI_ATTACK_SLASH:
		case IE_ANI_ATTACK_JAB:
			dest.Append("g2");
			EquipData.Suffix = "g2";
			Cycle = Orient / 2;
			break;
		case IE_ANI_CAST:
		case IE_ANI_CONJURE:
		case IE_ANI_SHOOT:
			dest.Append("g2");
			EquipData.Suffix = "g2";
			Cycle = 8 + Orient / 2;
			break;
		case IE_ANI_WALK:
			dest.Append("g1");
			EquipData.Suffix = "g1";
			Cycle = Orient / 2;
			break;
		case IE_ANI_READY:
			dest.Append("g1");
			EquipData.Suffix = "g1";
			Cycle = 8 + Orient / 2;
			break;
		case IE_ANI_HEAD_TURN: //could be wrong
		case IE_ANI_AWAKE:
			dest.Append("g1");
			EquipData.Suffix = "g1";
			Cycle = 16 + Orient / 2;
			break;
		case IE_ANI_DAMAGE:
			dest.Append("g1");
			EquipData.Suffix = "g1";
			Cycle = 24 + Orient / 2;
			break;
		case IE_ANI_GET_UP:
		case IE_ANI_EMERGE:
		case IE_ANI_PST_START:
		case IE_ANI_DIE:
			dest.Append("g1");
			EquipData.Suffix = "g1";
			Cycle = 32 + Orient / 2;
			break;
		case IE_ANI_SLEEP:
		case IE_ANI_HIDE:
		case IE_ANI_TWITCH:
			dest.Append("g1");
			EquipData.Suffix = "g1";
			Cycle = 40 + Orient / 2;
			break;
		default:
			error("CharAnimation", "LRSuffix2 Animation: unhandled stance: {} {}", dest, StanceID);
	}
	if (Orient > 9) {
		dest.Append("e");
		EquipData.Suffix.Append("e");
	}
	EquipData.Cycle = Cycle;
}

void CharAnimations::AddTwoPieceSuffix(ResRef& dest, unsigned char StanceID,
									   unsigned char& Cycle, orient_t Orient, int Part) const
{
	if (Part == 1) {
		dest.Append("d");
	}

	switch (StanceID) {
		case IE_ANI_DIE:
			dest.Append("g1");
			Cycle = 8 + Orient / 2;
			break;
		case IE_ANI_TWITCH:
		case IE_ANI_SLEEP:
			dest.Append("g1");
			Cycle = 16 + Orient / 2;
			break;
		case IE_ANI_READY:
		case IE_ANI_HEAD_TURN:
		case IE_ANI_AWAKE:
		case IE_ANI_DAMAGE:
			dest.Append("g1");
			Cycle = 24 + Orient / 2;
			break;
		case IE_ANI_WALK: // this is more like IE_ANI_AWAKE / IE_ANI_READY when underground
			dest.Append("g2");
			Cycle = Orient / 2;
			break;
		case IE_ANI_GET_UP:
		case IE_ANI_EMERGE:
			dest.Append("g2");
			Cycle = 8 + Orient / 2;
			break;
		case IE_ANI_HIDE:
			dest.Append("g2");
			Cycle = 16 + Orient / 2;
			break;
		case IE_ANI_ATTACK:
		case IE_ANI_ATTACK_BACKSLASH:
		case IE_ANI_ATTACK_SLASH:
		case IE_ANI_ATTACK_JAB:
			dest.Append("g3");
			Cycle = Orient / 2;
			break;
		case IE_ANI_CAST:
		case IE_ANI_CONJURE:
		case IE_ANI_SHOOT:
			dest.Append("g3");
			Cycle = 8 + Orient / 2;
			break;
		default:
			error("CharAnimation", "Two-piece Animation: unhandled stance: {} {}", dest, StanceID);
	}
	if (Orient > 9) {
		dest.Append("e");
	}
}

void CharAnimations::AddLRSuffix(ResRef& dest, unsigned char StanceID,
								 unsigned char& Cycle, orient_t Orient, EquipResRefData& EquipData) const
{
	switch (StanceID) {
		case IE_ANI_ATTACK:
		case IE_ANI_ATTACK_BACKSLASH:
			dest.Append("g2");
			EquipData.Suffix = "g2";
			Cycle = Orient / 2;
			break;
		case IE_ANI_ATTACK_SLASH:
		case IE_ANI_CAST:
		case IE_ANI_CONJURE:
			dest.Append("g2");
			EquipData.Suffix = "g2";
			Cycle = 8 + Orient / 2;
			break;
		case IE_ANI_ATTACK_JAB:
		case IE_ANI_SHOOT:
			dest.Append("g2");
			EquipData.Suffix = "g2";
			Cycle = 16 + Orient / 2;
			break;
		case IE_ANI_WALK:
		case IE_ANI_HIDE: // unknown, just a guess
			dest.Append("g1");
			EquipData.Suffix = "g1";
			Cycle = Orient / 2;
			break;
		case IE_ANI_AWAKE:
			dest.Append("g1");
			EquipData.Suffix = "g1";
			Cycle = 8 + Orient / 2;
			break;
		case IE_ANI_READY:
		case IE_ANI_HEAD_TURN: //could be wrong
			dest.Append("g1");
			EquipData.Suffix = "g1";
			Cycle = 16 + Orient / 2;
			break;
		case IE_ANI_DAMAGE:
			dest.Append("g1");
			EquipData.Suffix = "g1";
			Cycle = 24 + Orient / 2;
			break;
		case IE_ANI_GET_UP:
		case IE_ANI_EMERGE:
		case IE_ANI_PST_START:
		case IE_ANI_DIE:
			dest.Append("g1");
			EquipData.Suffix = "g1";
			Cycle = 32 + Orient / 2;
			break;
		case IE_ANI_TWITCH:
		case IE_ANI_SLEEP:
			dest.Append("g1");
			EquipData.Suffix = "g1";
			Cycle = 40 + Orient / 2;
			break;
		default:
			error("CharAnimation", "LR Animation: unhandled stance: {} {}", dest, StanceID);
	}
	if (Orient > 9) {
		dest.Append("e");
		EquipData.Suffix.Append("e");
	}
	EquipData.Cycle = Cycle;
}

void CharAnimations::GetLREquipmentRef(ResRef& dest, unsigned char& Cycle,
									   AnimRef equipRef, bool /*offhand*/,
									   const EquipResRefData& equip) const
{
	Cycle = equip.Cycle;
	dest.Format("{}{}{}", ResRefBase, equipRef, equip.Suffix);
}

//Only for the ogre animation (MOGR)
void CharAnimations::AddLR3Suffix(ResRef& dest, unsigned char StanceID,
								  unsigned char& Cycle, orient_t Orient) const
{
	switch (StanceID) {
		case IE_ANI_ATTACK:
		case IE_ANI_ATTACK_BACKSLASH:
			dest.Append("g2");
			Cycle = Orient / 2;
			break;
		case IE_ANI_ATTACK_SLASH:
		case IE_ANI_ATTACK_JAB: //there is no third attack animation
			dest.Append("g2");
			Cycle = 8 + Orient / 2;
			break;
		case IE_ANI_CAST:
		case IE_ANI_CONJURE:
		case IE_ANI_SHOOT:
			dest.Append("g3");
			Cycle = Orient / 2;
			break;
		case IE_ANI_WALK:
			dest.Append("g1");
			Cycle = 16 + Orient / 2;
			break;
		case IE_ANI_READY:
			dest.Append("g1");
			Cycle = 8 + Orient / 2;
			break;
		case IE_ANI_HEAD_TURN: //could be wrong
		case IE_ANI_AWAKE:
		case IE_ANI_HIDE:
			dest.Append("g1");
			Cycle = Orient / 2;
			break;
		case IE_ANI_DAMAGE:
			dest.Append("g3");
			Cycle = 8 + Orient / 2;
			break;
		case IE_ANI_DIE:
		case IE_ANI_GET_UP:
		case IE_ANI_EMERGE:
		case IE_ANI_PST_START:
		case IE_ANI_SLEEP:
			dest.Append("g3");
			Cycle = 16 + Orient / 2;
			break;
		case IE_ANI_TWITCH:
			dest.Append("g3");
			Cycle = 24 + Orient / 2;
			break;
		default:
			error("CharAnimation", "LR3 Animation: unhandled stance: {} {}", dest, StanceID);
	}
	if (Orient > 9) {
		dest.Append("e");
	}
}

void CharAnimations::AddMMR2Suffix(ResRef& dest, unsigned char StanceID,
								   unsigned char& Cycle, orient_t Orient) const
{
	switch (StanceID) {
		case IE_ANI_ATTACK:
		case IE_ANI_ATTACK_SLASH:
		case IE_ANI_ATTACK_BACKSLASH:
		case IE_ANI_ATTACK_JAB:
		case IE_ANI_CONJURE:
		case IE_ANI_CAST:
			dest.Append("a1");
			Cycle = ( Orient / 2 );
			break;

		case IE_ANI_SHOOT:
			dest.Append("a4");
			Cycle = ( Orient / 2 );
			break;

		case IE_ANI_AWAKE:
		case IE_ANI_READY:
			dest.Append("sd");
			Cycle = ( Orient / 2 );
			break;

		case IE_ANI_HEAD_TURN:
			dest.Append("sc");
			Cycle = ( Orient / 2 );
			break;

		case IE_ANI_DAMAGE:
			dest.Append("gh");
			Cycle = ( Orient / 2 );
			break;

		case IE_ANI_DIE:
			dest.Append("de");
			Cycle = ( Orient / 2 );
			break;

		case IE_ANI_GET_UP:
		case IE_ANI_EMERGE:
		case IE_ANI_PST_START:
			dest.Append("gu");
			Cycle = ( Orient / 2 );
			break;

			//Unknown... maybe only a transparency effect apply
		case IE_ANI_HIDE:
			break;

		case IE_ANI_SLEEP:
			dest.Append("sl");
			Cycle = ( Orient / 2 );
			break;

		case IE_ANI_TWITCH:
			dest.Append("tw");
			Cycle = ( Orient / 2 );
			break;

		case IE_ANI_WALK:
			dest.Append("wk");
			Cycle = ( Orient / 2 );
			break;
		default:
			error("CharAnimation", "MMR Animation: unhandled stance: {} {}", dest, StanceID);
	}
	if (Orient > 9) {
		dest.Append("e");
	}
}

void CharAnimations::AddMMRSuffix(ResRef& dest, unsigned char StanceID,
								  unsigned char& Cycle, orient_t Orient, bool mirror) const
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
			dest.Append("a1");
			break;

		case IE_ANI_SHOOT:
			dest.Append("a4");
			break;

		case IE_ANI_ATTACK_JAB:
			dest.Append("a2");
			break;

		case IE_ANI_AWAKE:
		case IE_ANI_READY:
			dest.Append("sd");
			break;

		case IE_ANI_CONJURE:
			dest.Append("ca");
			break;

		case IE_ANI_CAST:
			dest.Append("sp");
			break;

		case IE_ANI_HEAD_TURN:
			dest.Append("sc");
			break;

		case IE_ANI_DAMAGE:
			dest.Append("gh");
			break;

		case IE_ANI_DIE:
			dest.Append("de");
			break;

		case IE_ANI_GET_UP:
		case IE_ANI_EMERGE:
		case IE_ANI_PST_START:
			dest.Append("gu");
			break;

			//Unknown... maybe only a transparency effect apply
		case IE_ANI_HIDE:
			break;

		case IE_ANI_SLEEP:
			dest.Append("sl");
			break;

		case IE_ANI_TWITCH:
			dest.Append("tw");
			break;

		case IE_ANI_WALK:
			dest.Append("wk");
			break;
		default:
			error("CharAnimation", "MMR Animation: unhandled stance: {} {}", dest, StanceID);
	}
	if (!mirror && Orient > 9) {
		dest.Append("e");
	}
}

void CharAnimations::AddHLSuffix(ResRef& dest, unsigned char StanceID,
								 unsigned char& Cycle, orient_t Orient) const
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
			error("CharAnimation", "HL Animation: unhandled stance: {} {}", dest, StanceID);
	}
	if (offset) {
		dest.Append("hg1");
	} else {
		dest.Append("lg1");
	}
	if (Orient > 9) {
		dest.Append("e");
	}
}

void CharAnimations::PulseRGBModifiers()
{
	tick_t time = GetMilliseconds();

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

	for (const PaletteType i : EnumIterator<PaletteType, PAL_MAIN, PAL_MAX>()) {
		if (change[i]) {
			change[i] = false;
			SetupColors(i);
		}
	}

	lastModUpdate += inc*40;
}

void CharAnimations::DebugDump() const
{
	Log (DEBUG, "CharAnimations", "Anim ID   : {:#x}", GetAnimationID());
	Log (DEBUG, "CharAnimations", "BloodColor: {}", GetBloodColor());
	Log (DEBUG, "CharAnimations", "Flags     : {:#x}", GetFlags());
}

static inline void applyMod(const Color& src, Color& dest, const RGBModifier& mod) noexcept
{
	if (mod.speed == -1) {
		if (mod.type == RGBModifier::TINT) {
			dest.r = ((unsigned int)src.r * mod.rgb.r)>>8;
			dest.g = ((unsigned int)src.g * mod.rgb.g)>>8;
			dest.b = ((unsigned int)src.b * mod.rgb.b)>>8;
		} else if (mod.type == RGBModifier::BRIGHTEN) {
			unsigned int r = (unsigned int)src.r * mod.rgb.r;
			if (r & (~0x7FF)) r = 0x7FF;
			dest.r = r >> 3;

			unsigned int g = (unsigned int)src.g * mod.rgb.g;
			if (g & (~0x7FF)) g = 0x7FF;
			dest.g = g >> 3;

			unsigned int b = (unsigned int)src.b * mod.rgb.b;
			if (b & (~0x7FF)) b = 0x7FF;
			dest.b = b >> 3;
		} else if (mod.type == RGBModifier::ADD) {
			unsigned int r = (unsigned int)src.r + mod.rgb.r;
			if (r & (~0xFF)) r = 0xFF;
			dest.r = r;

			unsigned int g = (unsigned int)src.g + mod.rgb.g;
			if (g & (~0xFF)) g = 0xFF;
			dest.g = g;

			unsigned int b = (unsigned int)src.b + mod.rgb.b;
			if (b & (~0xFF)) b = 0xFF;
			dest.b = b;
		} else {
			dest = src;
		}
	} else if (mod.speed > 0) {

		// TODO: a sinewave will probably look better
		int phase = (mod.phase % (2*mod.speed));
		if (phase > mod.speed) {
			phase = 512 - (256*phase)/mod.speed;
		} else {
			phase = (256*phase)/mod.speed;
		}

		if (mod.type == RGBModifier::TINT) {
			dest.r = ((unsigned int)src.r * (256*256 + phase*mod.rgb.r - 256*phase))>>16;
			dest.g = ((unsigned int)src.g * (256*256 + phase*mod.rgb.g - 256*phase))>>16;
			dest.b = ((unsigned int)src.b * (256*256 + phase*mod.rgb.b - 256*phase))>>16;
		} else if (mod.type == RGBModifier::BRIGHTEN) {
			unsigned int r = src.r + (256*256 + phase*mod.rgb.r - 256*phase);
			if (r & (~0x7FFFF)) r = 0x7FFFF;
			dest.r = r >> 11;

			unsigned int g = src.g * (256*256 + phase*mod.rgb.g - 256*phase);
			if (g & (~0x7FFFF)) g = 0x7FFFF;
			dest.g = g >> 11;

			unsigned int b = src.b * (256*256 + phase*mod.rgb.b - 256*phase);
			if (b & (~0x7FFFF)) b = 0x7FFFF;
			dest.b = b >> 11;
		} else if (mod.type == RGBModifier::ADD) {
			unsigned int r = src.r + ((phase*mod.rgb.r)>>8);
			if (r & (~0xFF)) r = 0xFF;
			dest.r = r;

			unsigned int g = src.g + ((phase*mod.rgb.g)>>8);
			if (g & (~0xFF)) g = 0xFF;
			dest.g = g;

			unsigned int b = src.b + ((phase*mod.rgb.b)>>8);
			if (b & (~0xFF)) b = 0xFF;
			dest.b = b;
		} else {
			dest = src;
		}
	} else {
		dest = src;
	}
}

Palette SetupRGBModification(const Holder<Palette>& src, const RGBModifier* mods,
	unsigned int type) noexcept
{
	Palette pal;

	const RGBModifier* tmods = mods+(8*type);
	int i;

	for (i = 0; i < 4; ++i)
		pal.col[i] = src->col[i];

	for (i = 0; i < 12; ++i)
		applyMod(src->col[0x04+i], pal.col[0x04+i], tmods[0]);
	for (i = 0; i < 12; ++i)
		applyMod(src->col[0x10+i], pal.col[0x10+i], tmods[1]);
	for (i = 0; i < 12; ++i)
		applyMod(src->col[0x1c+i], pal.col[0x1c+i], tmods[2]);
	for (i = 0; i < 12; ++i)
		applyMod(src->col[0x28+i], pal.col[0x28+i], tmods[3]);
	for (i = 0; i < 12; ++i)
		applyMod(src->col[0x34+i], pal.col[0x34+i], tmods[4]);
	for (i = 0; i < 12; ++i)
		applyMod(src->col[0x40+i], pal.col[0x40+i], tmods[5]);
	for (i = 0; i < 12; ++i)
		applyMod(src->col[0x4c+i], pal.col[0x4c+i], tmods[6]);
	for (i = 0; i < 8; ++i)
		applyMod(src->col[0x58+i], pal.col[0x58+i], tmods[1]);
	for (i = 0; i < 8; ++i)
		applyMod(src->col[0x60+i], pal.col[0x60+i], tmods[2]);
	for (i = 0; i < 8; ++i)
		applyMod(src->col[0x68+i], pal.col[0x68+i], tmods[1]);
	for (i = 0; i < 8; ++i)
		applyMod(src->col[0x70+i], pal.col[0x70+i], tmods[0]);
	for (i = 0; i < 8; ++i)
		applyMod(src->col[0x78+i], pal.col[0x78+i], tmods[4]);
	for (i = 0; i < 8; ++i)
		applyMod(src->col[0x80+i], pal.col[0x80+i], tmods[4]);
	for (i = 0; i < 8; ++i)
		applyMod(src->col[0x88+i], pal.col[0x88+i], tmods[1]);
	for (i = 0; i < 24; ++i)
		applyMod(src->col[0x90+i], pal.col[0x90+i], tmods[4]);

	for (i = 0; i < 8; ++i)
		pal.col[0xA8+i] = src->col[0xA8+i];

	for (i = 0; i < 8; ++i)
		applyMod(src->col[0xB0+i], pal.col[0xB0+i], tmods[3]);
	for (i = 0; i < 72; ++i)
		applyMod(src->col[0xB8+i], pal.col[0xB8+i], tmods[4]);
	
	return pal;
}

Palette SetupGlobalRGBModification(const Holder<Palette>& src, const RGBModifier& mod) noexcept
{
	Palette pal;
	// don't modify the transparency and shadow colour
	for (int i = 0; i < 2; ++i) {
		pal.col[i] = src->col[i];
	}

	for (int i = 2; i < 256; ++i) {
		applyMod(src->col[i], pal.col[i], mod);
	}

	return pal;
}

Palette SetupPaperdollColours(const ieDword* colors, unsigned int type) noexcept
{
	unsigned int s = Clamp<ieDword>(8*type, 0, 8*sizeof(ieDword)-1);
	constexpr uint8_t numCols = 12;
	Palette pal;

	enum PALETTES : uint8_t
	{
		METAL = 0, MINOR, MAJOR, SKIN, LEATHER, ARMOR, HAIR,
		END
	};

	for (uint8_t idx = METAL; idx < END; ++idx) {
		const auto& pal16 = core->GetPalette16(colors[idx]>>s);
		pal.CopyColorRange(&pal16[0], &pal16[numCols], 0x04 + (idx * 12));
	}
	
	//minor
	memcpy(&pal.col[0x58], &pal.col[0x11], 8 * sizeof(Color));
	//major
	memcpy(&pal.col[0x60], &pal.col[0x1d], 8 * sizeof(Color));
	//minor
	memcpy(&pal.col[0x68], &pal.col[0x11], 8 * sizeof(Color));
	//metal
	memcpy(&pal.col[0x70], &pal.col[0x05], 8 * sizeof(Color));
	//leather
	memcpy(&pal.col[0x78], &pal.col[0x35], 8 * sizeof(Color));
	//leather
	memcpy(&pal.col[0x80], &pal.col[0x35], 8 * sizeof(Color));
	//minor
	memcpy(&pal.col[0x88], &pal.col[0x11], 8 * sizeof(Color));

	for (int i = 0x90; i < 0xA8; i += 0x08) {
		//leather
		memcpy(&pal.col[i], &pal.col[0x35], 8 * sizeof(Color));
	}

	//skin
	memcpy(&pal.col[0xB0], &pal.col[0x29], 8 * sizeof(Color));

	for (int i = 0xB8; i < 0xFF; i += 0x08) {
		//leather
		memcpy(&pal.col[i], &pal.col[0x35], 8 * sizeof(Color));
	}

	pal.col[1] = Color(0, 0, 0, 128); // shadows are always half trans black
	return pal;
}

Holder<Sprite2D> GetPaperdollImage(const ResRef& resref, const ieDword* colors, Holder<Sprite2D>& picture2, unsigned int type)
{
	auto af = gamedata->GetFactoryResourceAs<const AnimationFactory>(resref, IE_BAM_CLASS_ID);
	if (!af) {
		Log(WARNING, "GetPaperdollImage", "BAM/PLT not found for ref: {}", resref);
		return nullptr;
	}

	if (af->GetFrameCount() < 2) {
		return nullptr;
	}
	
	// mod paperdolls can be unsorted (Longer Road Irenicus cycle: 1 1 0)
	Holder<Sprite2D> first;
	Holder<Sprite2D> second;
	for (AnimationFactory::index_t i = 0; i < af->GetCycleSize(0); ++i) {
		auto spr = af->GetFrame(i, 0);
		if (first == nullptr) {
			first = spr;
		} else if (second == nullptr && spr != first) {
			second = spr;
			break;
		}
	}
	assert(first && second);
	picture2 = second->copy();
	picture2->Frame.y -= 80;

	Holder<Sprite2D> spr = first->copy();

	if (colors) {
		Holder<Palette> pal = MakeHolder<Palette>(*spr->GetPalette());
		*pal = SetupPaperdollColours(colors, type);
		spr->SetPalette(pal);
		picture2->SetPalette(pal);
	}

	return spr;
}

}
