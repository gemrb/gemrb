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

#ifndef CHARANIMATIONS_H
#define CHARANIMATIONS_H

#include "RGBAColor.h"
#include "exports.h"

#include "Animation.h"
#include "Palette.h"
#include "TableMgr.h"

#include <vector>

namespace GemRB {

#define AV_PREFIX1      0
#define AV_PREFIX2      1
#define AV_PREFIX3      2
#define AV_PREFIX4      3
#define AV_ANIMTYPE     4
#define AV_CIRCLESIZE   5
#define AV_USE_PALETTE  6
#define AV_SIZE         7

#define MAX_ANIMS				19

#define IE_ANI_ATTACK			0
#define IE_ANI_AWAKE			1
#define IE_ANI_CAST			2
#define IE_ANI_CONJURE			3
#define IE_ANI_DAMAGE			4
#define IE_ANI_DIE			5
#define IE_ANI_HEAD_TURN		6
#define IE_ANI_READY			7
#define IE_ANI_SHOOT			8
#define IE_ANI_TWITCH			9
#define IE_ANI_WALK			10
#define IE_ANI_ATTACK_SLASH		11
#define IE_ANI_ATTACK_BACKSLASH		12
#define IE_ANI_ATTACK_JAB		13
#define IE_ANI_EMERGE			14
#define IE_ANI_HIDE			15
#define IE_ANI_RUN			15 //pst has no hide, i hope
#define IE_ANI_SLEEP			16
#define IE_ANI_GET_UP			17
#define IE_ANI_PST_START		18

//BG2, IWD animation types
#define IE_ANI_CODE_MIRROR		0
#define IE_ANI_ONE_FILE			1
#define IE_ANI_FOUR_FILES		2
#define IE_ANI_TWO_FILES		3
#define IE_ANI_CODE_MIRROR_2		4
#define IE_ANI_SIX_FILES_2		5    //MOGR
#define IE_ANI_TWENTYTWO		6
#define IE_ANI_BIRD				7
#define IE_ANI_SIX_FILES		8    //MCAR/MWYV
#define IE_ANI_TWO_FILES_3		9    //iwd animations
#define IE_ANI_TWO_FILES_2		10   //low res bg1 anim
#define IE_ANI_FOUR_FRAMES		11   //wyvern anims
#define IE_ANI_NINE_FRAMES		12   //dragon anims
#define IE_ANI_FRAGMENT                 13   //fragment animation
#define IE_ANI_FOUR_FILES_2		14 //METT
#define IE_ANI_CODE_MIRROR_3	15 //MSPS
#define IE_ANI_TWO_FILES_3B		16    //iwd animations (eg. MBBM)
#define IE_ANI_TWO_PIECE		17    //MAKH
#define IE_ANI_FOUR_FILES_3		18    //mostly civilians
#define IE_ANI_TWO_FILES_4		19

//PST animation types
#define IE_ANI_PST_ANIMATION_1		56   //full animation
#define IE_ANI_PST_GHOST		57   //no orientations
#define IE_ANI_PST_STAND		58   //has orientations
#define IE_ANI_PST_ANIMATION_2		59   //full animation std-->stc
#define IE_ANI_PST_ANIMATION_3		60   //full animation stc-->std

//armour levels
#define IE_ANI_NO_ARMOR			0
#define IE_ANI_LIGHT_ARMOR		1
#define IE_ANI_MEDIUM_ARMOR		2
#define IE_ANI_HEAVY_ARMOR		3

#define IE_ANI_WEAPON_1H		0
#define IE_ANI_WEAPON_2H		1
#define IE_ANI_WEAPON_2W		2

#define IE_ANI_RANGED_BOW		0
#define IE_ANI_RANGED_XBOW		1
#define IE_ANI_RANGED_THROW		2

//special flags
#define AV_NO_BODY_HEAT                 1

enum PaletteType {
	PAL_MAIN,
	PAL_MAIN_2,
	PAL_MAIN_3,
	PAL_MAIN_4,
	PAL_MAIN_5,
	PAL_WEAPON,
	PAL_OFFHAND,
	PAL_HELMET,
	PAL_MAX
};

struct AvatarStruct {
	/* entries from avatars.2da */
	unsigned int AnimID;
	unsigned int PaletteType;
	ieResRef Prefixes[4];
	unsigned char AnimationType;
	unsigned char CircleSize;
	char Size;

	/* comes from bloodclr.2da */
	char BloodColor;
	unsigned int Flags;
	
	/* resdata.ini entries */
	unsigned int WalkScale; /* 1000 / walkscale */
	unsigned int RunScale; /* 1000 / runscale */
	int Bestiary;

	/* comes from walksnd.2da */
	ieResRef WalkSound;
	ieByte WalkSoundCount;

	/* comes from stances.2da */
	unsigned char StanceOverride[MAX_ANIMS];
};

struct EquipResRefData;

class GEM_EXPORT CharAnimations {
private:
	Animation** Anims[MAX_ANIMS][MAX_ORIENT];
	char HelmetRef[2];
	char WeaponRef[2];
	char OffhandRef[2];
public:
	const ieDword *Colors; //these are the custom color indices
	RGBModifier ColorMods[32]; // color modification effects
	unsigned long lastModUpdate;
	RGBModifier GlobalColorMod; // global color modification effect

	bool change[4];
	Palette* palette[PAL_MAX];
	Palette* modifiedPalette[PAL_MAX];
	unsigned int AvatarsRowNum;
	unsigned char ArmorType, WeaponType, RangedType;
	ieResRef ResRef;
	ieResRef PaletteResRef[5];
	unsigned char nextStanceID, StanceID;
	bool autoSwitchOnEnd;
	bool lockPalette;
public:
	CharAnimations(unsigned int AnimID, ieDword ArmourLevel);
	~CharAnimations(void);
	static void ReleaseMemory();
	void SetArmourLevel(int ArmourLevel);
	void SetRangedType(int Ranged);
	void SetWeaponType(int WeaponType);
	void SetHelmetRef(const char* ref);
	void SetWeaponRef(const char* ref);
	void SetOffhandRef(const char* ref);
	void SetColors(const ieDword *Colors);
	void CheckColorMod();
	void SetupColors(PaletteType type);
	void LockPalette(const ieDword *Colors);

	// returns an array of animations of size GetTotalPartCount()
	Animation** GetAnimation(unsigned char Stance, unsigned char Orient);
	int GetTotalPartCount() const;
	const int* GetZOrder(unsigned char Orient);

	// returns Palette for a given part (unlocked)
	Palette* GetPartPalette(int part); // TODO: clean this up

public: //attribute functions
	static int GetAvatarsCount();
	static AvatarStruct *GetAvatarStruct(int RowNum);
	unsigned int GetAnimationID() const;
	int GetCircleSize() const;
	int NoPalette() const;
	int GetAnimType() const;
	int GetSize() const;
	int GetBloodColor() const;
	unsigned int GetFlags() const;
	const ieResRef &GetWalkSound() const;
	int GetWalkSoundCount() const;
	const ieResRef &GetArmourLevel(int ArmourLevel) const;
	void PulseRGBModifiers();
	void DebugDump();
private:
	void DropAnims();
	void InitAvatarsTable();
	int GetActorPartCount() const;
	void AddPSTSuffix(char* ResRef, unsigned char AnimID,
		unsigned char& Cycle, unsigned char Orient);
	void AddFFSuffix(char* ResRef, unsigned char AnimID,
		unsigned char& Cycle, unsigned char Orient, int Part);
	void AddHLSuffix(char* ResRef, unsigned char AnimID,
		unsigned char& Cycle, unsigned char Orient);
	void AddNFSuffix(char* ResRef, unsigned char AnimID,
		unsigned char& Cycle, unsigned char Orient, int Part);
	void AddVHR2Suffix(char* ResRef, unsigned char AnimID,
		unsigned char& Cycle, unsigned char Orient);
	void AddVHRSuffix(char* ResRef, unsigned char AnimID,
		unsigned char& Cycle, unsigned char Orient, EquipResRefData*& equip);
	void AddVHR3Suffix(char* ResRef, unsigned char AnimID,
		unsigned char& Cycle, unsigned char Orient);
	void GetVHREquipmentRef(char* ResRef, unsigned char& Cycle,
		const char* equipRef, bool offhand, EquipResRefData* equip);
	void AddSixSuffix(char* ResRef, unsigned char AnimID,
		unsigned char& Cycle, unsigned char Orient);
	void AddTwoPieceSuffix(char* ResRef, unsigned char AnimID,
		unsigned char& Cycle, unsigned char Orient, int Part);
	void AddMHRSuffix(char* ResRef, unsigned char AnimID,
		unsigned char& Cycle, unsigned char Orient, EquipResRefData*& equip);
	void GetMHREquipmentRef(char* ResRef, unsigned char& Cycle,
		const char* equipRef, bool offhand, EquipResRefData* equip);
	void AddMMRSuffix(char* ResRef, unsigned char AnimID,
		unsigned char& Cycle, unsigned char Orient);
	void AddMMR2Suffix(char* ResRef, unsigned char AnimID,
		unsigned char& Cycle, unsigned char Orient);
	void AddTwoFileSuffix(char* ResRef, unsigned char AnimID,
		unsigned char& Cycle, unsigned char Orient);
	void AddLRSuffix(char* ResRef, unsigned char AnimID,
		unsigned char& Cycle, unsigned char Orient, EquipResRefData*& equip);
	void AddLRSuffix2( char* ResRef, unsigned char StanceID,
		unsigned char& Cycle, unsigned char Orient, EquipResRefData *&EquipData);
	void GetLREquipmentRef(char* ResRef, unsigned char& Cycle,
		const char* equipRef, bool offhand, EquipResRefData* equip);
	void AddLR2Suffix(char* ResRef, unsigned char AnimID,
		unsigned char& Cycle, unsigned char Orient);
	void AddLR3Suffix(char* ResRef, unsigned char AnimID,
		unsigned char& Cycle, unsigned char Orient);
	void GetAnimResRef(unsigned char AnimID, unsigned char Orient,
		char* ResRef, unsigned char& Cycle, int Part, EquipResRefData*& equip);
	void GetEquipmentResRef(const char* equipRef, bool offhand,
		char* ResRef, unsigned char& Cycle, EquipResRefData* equip);
	unsigned char MaybeOverrideStance(unsigned char stance) const;
};

}

#endif
