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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

/**
 * @file Item.h
 * Declares Item, class for all things your PCs can pick, carry and wear
 * and many they can't.
 * @author The GemRB Project
 */

#ifndef ITEM_H
#define ITEM_H

#include "exports-core.h"
#include "ie_types.h"

#include "CharAnimations.h"
#include "EffectQueue.h"

#include <vector>

namespace GemRB {

class Projectile;

// Item Flags bits
// !!! Keep these synchronized with GUIDefines.h !!!
#define IE_ITEM_CRITICAL     0x00000001
#define IE_ITEM_TWO_HANDED   0x00000002
#define IE_ITEM_MOVABLE      0x00000004
#define IE_ITEM_DISPLAYABLE  0x00000008
#define IE_ITEM_CURSED       0x00000010
#define IE_ITEM_NOT_COPYABLE 0x00000020 // unused, ITMExtHeader dictates this
#define IE_ITEM_MAGICAL      0x00000040
#define IE_ITEM_BOW          0x00000080 // TODO: ee lefthanded (same?), disallow offhand use (similar IE_ITEM_NOT_OFFHAND)
#define IE_ITEM_SILVER       0x00000100
#define IE_ITEM_COLD_IRON    0x00000200
#define IE_ITEM_STOLEN       0x00000400 // TODO: ee offhanded
#define IE_ITEM_CONVERSABLE  0x00000800
#define IE_ITEM_PULSATING    0x00001000 // TODO: ee fake offhand
#define IE_ITEM_UNSELLABLE   (IE_ITEM_CRITICAL | IE_ITEM_STOLEN) // beware: StoreActionFlags::BUYCRITS may override the first half
// see IESDP for details
#define IE_ITEM_FORCE_2H    0x00002000 // TODO: ee, Force two-handed animation DISABLE_OFFHAND
#define IE_ITEM_NOT_OFFHAND 0x00004000 // ee, not usable in off-hand
#define IE_ITEM_INV_USABLE  0x00008000 // TODO: ee, only pstee: usable in inventory, shared with IE_ITEM_ADAMANTINE
#define IE_ITEM_ADAMANTINE  0x00008000 // bgee: adamantine (itemflag.ids)
//tobex modder extensions, please note, these are not copied into the local slot bits
#define IE_ITEM_NO_DISPEL    0x01000000 // disables destruction by dispelling; named BODYPART in ee
#define IE_ITEM_TOGGLE_CRITS 0x02000000 //toggles critical hit avertion
#define IE_ITEM_NO_INVIS     0x04000000


//Extended header recharge flags
#define IE_ITEM_USESTRENGTH     1 //weapon, EE splits it in two
#define IE_ITEM_BREAKABLE       2 //weapon
#define IE_ITEM_USESTRENGTH_DMG 4 // EE
#define IE_ITEM_USESTRENGTH_HIT 8 // EE
#define IE_ITEM_USEDEXTERITY    16 //gemrb weapon finesse (move this if tobex implements it elsewhere)
#define IE_ITEM_HOSTILE         0x400 //equipment
#define IE_ITEM_RECHARGE        0x800 //equipment
#define IE_ITEM_BYPASS          0x10000 //weapon (bypass shield and armor bonus)
#define IE_ITEM_KEEN            0x20000 //weapon

//modder extensions
#define IE_ITEM_BACKSTAB 0x01000000 //can used for backstab (ranged weapon)

//item use locations (weapons are not listed in equipment list)
#define ITEM_LOC_WEAPON    1 //this is a weapon slot (uses thac0 etc)
#define ITEM_LOC_EQUIPMENT 3 //this slot appears on equipment list
//other item locations appear useless

//attack types
#define ITEM_AT_MELEE      1
#define ITEM_AT_PROJECTILE 2
#define ITEM_AT_MAGIC      3
#define ITEM_AT_BOW        4

//target types
#define TARGET_INVALID 0 //all the rest (default)
#define TARGET_CREA    1 //single living creature
#define TARGET_INV     2 //inventory item (not used?)
#define TARGET_DEAD    3 //creature, item or point
#define TARGET_AREA    4 //point target
#define TARGET_SELF    5 //self
#define TARGET_UNKNOWN 6 //unknown (use default)
#define TARGET_NONE    7 //self

//projectile qualifiers
#define PROJ_ARROW  1
#define PROJ_BOLT   2
#define PROJ_BULLET 4

//charge depletion flags
#define CHG_NONE    0
#define CHG_BREAK   1
#define CHG_NOSOUND 2
#define CHG_DAY     3

struct DMGOpcodeInfo {
	String TypeName;
	int DiceThrown;
	int DiceSides;
	int DiceBonus;
	int Chance;
};

/**
 * @class ITMExtHeader
 * Header for special effects and uses of an Item.
 */

class GEM_EXPORT ITMExtHeader {
public:
	ITMExtHeader() noexcept = default;
	ITMExtHeader(const ITMExtHeader&) = delete;
	~ITMExtHeader();
	ITMExtHeader& operator=(const ITMExtHeader&) = delete;
	ieByte AttackType = 0;
	ieByte IDReq = 0; // in pst also holds the equivalent of spl friendly bit, but with only 2 senseless users
	ieByte Location = 0;
	ieByte AltDiceSides = 0;
	ResRef UseIcon;
	ieStrRef Tooltip = ieStrRef::INVALID;
	ieByte Target = 0;
	ieByte TargetNumber = 0;
	ieWord Range = 0;
	//this is partially redundant, but the original engine uses this value to
	//determine projectile type (ProjectileQualifier is almost always set too)
	//We use this field only when really needed, and resolve the redundancy
	//in the exporter. The reason: using bitfields is more flexible.
	//ieByte ProjectileType;
	ieByte AltDiceThrown = 0;
	ieByte Speed = 0;
	ieByte AltDamageBonus = 0;
	ieWord THAC0Bonus = 0;
	ieWord DiceSides = 0;
	ieWord DiceThrown = 0;
	ieWordSigned DamageBonus = 0; // this must be signed!!!
	ieWord DamageType = 0;
	ieWord FeatureOffset = 0;
	ieWord Charges = 0;
	ieWord ChargeDepletion = 0;
	ieDword RechargeFlags = 0; // this is a bitfield with many bits
	ieWord ProjectileAnimation = 0;
	std::array<ieWord, 3> MeleeAnimation;
	//this value is set in projectiles and launchers too
	int ProjectileQualifier = 0; // this is a derived value determined on load time
	std::vector<Effect*> features;
};

/**
 * @class Item
 * Class for all things your PCs can pick, carry and wear and many they can't.
 */

class GEM_EXPORT Item {
public:
	Item() noexcept = default;
	Item(const Item&) = delete;
	~Item();
	Item& operator=(const Item&) = delete;

	std::vector<ITMExtHeader> ext_headers;
	std::vector<Effect*> equipping_features;
	ResRef Name; //the resref of the item itself!

	ieStrRef ItemName = ieStrRef::INVALID;
	ieStrRef ItemNameIdentified = ieStrRef::INVALID;
	ResRef ReplacementItem;
	ieDword Flags = 0;
	ieWord ItemType = 0;
	ieDword UsabilityBitmask = 0;
	AnimRef AnimationType {};
	ieByte MinLevel = 0;
	ieByte unknown1 = 0; // ee docs say it's part of MinLevel read as a word, so useless
	ieByte MinStrength = 0;
	ieByte unknown2 = 0; // ee docs say it's part of MinStrength read as a word, so useless
	ieByte MinStrengthBonus = 0;
	//kit1
	ieByte MinIntelligence = 0;
	//kit2
	ieByte MinDexterity = 0;
	//kit3
	ieByte MinWisdom = 0;
	//kit4
	ieByte MinConstitution = 0;
	ieByte WeaProf = 0;
	ieByte MinCharisma = 0;
	ieByte unknown3 = 0; // ee docs say it's part of MinCharisma read as a word, so useless
	ieDword KitUsability = 0;
	ieDword Price = 0;
	ieWord MaxStackAmount = 0;
	ResRef ItemIcon;
	ieWord LoreToID = 0;
	ResRef GroundIcon;
	ieDword Weight = 0;
	ieStrRef ItemDesc = ieStrRef::INVALID;
	ieStrRef ItemDescIdentified = ieStrRef::INVALID;
	ResRef DescriptionIcon;
	ieDword Enchantment = 0;
	ieDword ExtHeaderOffset = 0;
	ieDword FeatureBlockOffset = 0;
	ieWord EquippingFeatureOffset = 0;
	ieWord EquippingFeatureCount = 0;

	// PST and BG2 only
	ResRef Dialog;
	ieStrRef DialogName = ieStrRef::INVALID;

	// PST only
	ieWord WieldColor = 0;

	// PST and IWD2 only
	char unknown[26] {};
	// flag items to mutually exclusive to equip
	ieDword ItemExcl = 0;

public:
	ieStrRef GetItemName(bool identified) const;
	ieStrRef GetItemDesc(bool identified) const;

	//returns if the item is usable, expend will also expend a charge
	int UseCharge(std::array<ieWord, CHARGE_COUNTERS>& charges, int header, bool expend) const;

	ieDword GetWieldedGradient() const
	{
		return WieldColor;
	}

	//-1 will return the equipping feature block
	EffectQueue GetEffectBlock(Scriptable* self, const Point& pos, int header, ieDwordSigned invslot, ieDword pro) const;
	//returns a projectile created from an extended header
	//if miss is non-zero, then no effects will be loaded
	Projectile* GetProjectile(Scriptable* self, int header, const Point& target, ieDwordSigned invslot, int miss) const;
	//Builds an equipping glow effect from gradient colour
	//this stuff is not item specific, could be moved elsewhere
	Effect* BuildGlowEffect(int gradient) const;
	//returns the average damage of the weapon (doesn't check for special effects)
	int GetDamagePotential(bool ranged, const ITMExtHeader*& header) const;
	//returns the weapon header
	const ITMExtHeader* GetWeaponHeader(bool ranged) const;
	int GetWeaponHeaderNumber(bool ranged) const;
	int GetEquipmentHeaderNumber(int cnt) const;
	const ITMExtHeader* GetExtHeader(int which) const;
	size_t GetExtHeaderCount() const { return ext_headers.size(); };
	unsigned int GetCastingDistance(int header) const;
	// returns  a vector with details about any extended headers containing fx_damage with a 100% probability
	std::vector<DMGOpcodeInfo> GetDamageOpcodesDetails(const ITMExtHeader* header) const;
};

}

#endif // ! ITEM_H
