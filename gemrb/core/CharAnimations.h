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
#include "Orientation.h"
#include "Palette.h"
#include "Resource.h"
#include "TableMgr.h"

#include <array>
#include <memory>

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
// NOTE: update MAX_ANIMS if you add more!

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
#define IE_ANI_FOUR_FRAMES_2	20    //MDEM
#define IE_ANI_TWO_FILES_5		21 //MMEL
#define IE_ANI_TWO_FILES_3C		22    //iwd animations (eg. MWDR)

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
#define IE_ANI_WEAPON_INVALID		100

#define IE_ANI_RANGED_BOW		0
#define IE_ANI_RANGED_XBOW		1
#define IE_ANI_RANGED_THROW		2

//special flags
#define AV_NO_BODY_HEAT                 1
#define AV_BUFFET_IMMUNITY              0x1000

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
	ResRef PaletteType;
	ResRef Prefixes[4];
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
	ResRef WalkSound;
	ieByte WalkSoundCount;

	/* comes from stances.2da */
	unsigned char StanceOverride[MAX_ANIMS];

	ResRef ShadowAnimation;
};

struct EquipResRefData;

using AnimRef = FixedSizeString<2>;

class GEM_EXPORT CharAnimations {
public:
	// using shared_ptr<Animation> because several orientations can share the same animation
	using SharedAnim = std::shared_ptr<Animation>;
	using PartAnim = std::vector<SharedAnim>;
	using OrientAnim = std::array<PartAnim, MAX_ORIENT>;
	using StanceAnim = std::array<OrientAnim, MAX_ANIMS>;

	const ieDword *Colors = nullptr; // these are the custom color indices
	RGBModifier ColorMods[PAL_MAX*8]; // color modification effects
	tick_t lastModUpdate = 0;
	RGBModifier GlobalColorMod; // global color modification effect

	bool change[PAL_MAX];
	PaletteHolder PartPalettes[PAL_MAX];
	PaletteHolder ModPartPalettes[PAL_MAX];
	PaletteHolder shadowPalette;
	size_t AvatarsRowNum;
	unsigned char ArmorType = 0, WeaponType = 0, RangedType = 0;
	ResRef ResRefBase;
	ResRef PaletteResRef[5] = {};
	unsigned char previousStanceID = 0;
	unsigned char nextStanceID = 0;
	unsigned char stanceID = 0;
	bool autoSwitchOnEnd = false;
	bool lockPalette = false;

	CharAnimations(unsigned int AnimID, ieDword ArmourLevel);
	CharAnimations(const CharAnimations&) = delete;
	~CharAnimations();
	CharAnimations& operator=(const CharAnimations&) = delete;

	void SetArmourLevel(int ArmourLevel);
	void SetRangedType(int Ranged);
	void SetWeaponType(unsigned char WeaponType);
	void SetHelmetRef(AnimRef ref);
	void SetWeaponRef(AnimRef ref);
	void SetOffhandRef(AnimRef ref);
	void SetColors(const ieDword *Colors);
	void CheckColorMod();
	void SetupColors(PaletteType type);
	void LockPalette(const ieDword *Colors);

	// returns an array of animations of size GetTotalPartCount()
	const PartAnim* GetAnimation(unsigned char Stance, orient_t Orient);
	int GetTotalPartCount() const;
	const int* GetZOrder(unsigned char Orient) const;
	const PartAnim* GetShadowAnimation(unsigned char Stance, orient_t OXrient);

	// returns Palette for a given part (unlocked)
	PaletteHolder GetPartPalette(int part) const; // TODO: clean this up
	PaletteHolder GetShadowPalette() const;

	static size_t GetAvatarsCount();
	static const AvatarStruct &GetAvatarStruct(size_t RowNum);
	unsigned int GetAnimationID() const;
	int GetCircleSize() const;
	const ResRef& GetPaletteType() const;
	int GetAnimType() const;
	char GetSize() const;
	int GetBloodColor() const;
	unsigned int GetFlags() const;
	const ResRef &GetWalkSound() const;
	int GetWalkSoundCount() const;
	const ResRef &GetArmourLevel(int ArmourLevel) const;
	void PulseRGBModifiers();
	void DebugDump() const;

private:
	void DropAnims();
	void InitAvatarsTable() const;
	int GetActorPartCount() const;
	void AddPSTSuffix(ResRef& dest, unsigned char AnimID,
		unsigned char& Cycle, orient_t Orient) const;
	void AddFFSuffix(ResRef& dest, unsigned char AnimID,
		unsigned char& Cycle, orient_t Orient, int Part) const;
	void AddFF2Suffix(ResRef& dest, unsigned char AnimID,
		unsigned char& Cycle, orient_t Orient, int Part) const;
	void AddHLSuffix(ResRef& dest, unsigned char AnimID,
		unsigned char& Cycle, orient_t Orient) const;
	void AddNFSuffix(ResRef& dest, unsigned char AnimID,
		unsigned char& Cycle, orient_t Orient, int Part) const;
	void AddVHR2Suffix(ResRef& dest, unsigned char AnimID,
		unsigned char& Cycle, orient_t Orient) const;
	void AddVHRSuffix(ResRef& dest, unsigned char AnimID,
		unsigned char& Cycle, orient_t Orient, EquipResRefData& equip) const;
	void AddVHR3Suffix(ResRef& dest, unsigned char AnimID,
		unsigned char& Cycle, orient_t Orient) const;
	void GetVHREquipmentRef(ResRef& dest, unsigned char& Cycle,
							AnimRef equipRef, bool offhand, const EquipResRefData& equip) const;
	void AddSixSuffix(ResRef& dest, unsigned char AnimID,
		unsigned char& Cycle, orient_t Orient) const;
	void AddTwoPieceSuffix(ResRef& dest, unsigned char AnimID,
		unsigned char& Cycle, orient_t Orient, int Part) const;
	void AddMHRSuffix(ResRef& dest, unsigned char AnimID,
		unsigned char& Cycle, orient_t Orient, EquipResRefData& equip) const;
	void GetMHREquipmentRef(ResRef& dest, unsigned char& Cycle,
							AnimRef equipRef, bool offhand, const EquipResRefData& equip) const;
	void AddMMRSuffix(ResRef& dest, unsigned char AnimID,
		unsigned char& Cycle, orient_t Orient, bool mirror) const;
	void AddMMR2Suffix(ResRef& dest, unsigned char AnimID,
		unsigned char& Cycle, orient_t Orient) const;
	void AddTwoFileSuffix(ResRef& dest, unsigned char AnimID,
		unsigned char& Cycle, orient_t Orient) const;
	void AddTwoFiles5Suffix(ResRef& dest, unsigned char AnimID,
		unsigned char& Cycle, orient_t Orient) const;
	void AddLRSuffix(ResRef& dest, unsigned char AnimID,
		unsigned char& Cycle, orient_t Orient, EquipResRefData& equip) const;
	void AddLRSuffix2( ResRef& dest, unsigned char StanceID,
		unsigned char& Cycle, orient_t Orient, EquipResRefData& EquipData) const;
	void GetLREquipmentRef(ResRef& dest, unsigned char& Cycle,
						   AnimRef equipRef, bool offhand, const EquipResRefData& equip) const;
	void AddLR2Suffix(ResRef& dest, unsigned char AnimID,
		unsigned char& Cycle, orient_t Orient) const;
	void AddLR3Suffix(ResRef& dest, unsigned char AnimID,
		unsigned char& Cycle, orient_t Orient) const;
	void GetAnimResRef(unsigned char AnimID, orient_t Orient,
		ResRef& dest, unsigned char& Cycle, int Part, EquipResRefData& equip) const;
	void GetEquipmentResRef(AnimRef equipRef, bool offhand,
		ResRef& dest, unsigned char& Cycle, const EquipResRefData& equip) const;
	unsigned char MaybeOverrideStance(unsigned char stance) const;
	void MaybeUpdateMainPalette(const Animation&);
	
	using AvatarTable_t = std::vector<AvatarStruct>;
	struct AvatarTableLoader final {
		AvatarTable_t table;
		
		static const AvatarTable_t& Get() {
			static AvatarTableLoader loader;
			return loader.table;
		}
		
	private:
		AvatarTableLoader() noexcept;
	};
	
	const AvatarTable_t& AvatarTable = AvatarTableLoader::Get();
	
	
	StanceAnim Anims;
	StanceAnim shadowAnimations;

	AnimRef HelmetRef;
	AnimRef WeaponRef;
	AnimRef OffhandRef;
};

}

#endif
