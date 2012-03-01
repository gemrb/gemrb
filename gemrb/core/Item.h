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

#include "exports.h"
#include "ie_types.h"

#include "EffectQueue.h"

namespace GemRB {

class Projectile;

// Item Flags bits
// !!! Keep these synchronized with GUIDefines.h !!!
#define IE_ITEM_CRITICAL     0x00000001
#define IE_ITEM_TWO_HANDED   0x00000002
#define IE_ITEM_MOVABLE      0x00000004
#define IE_ITEM_DISPLAYABLE  0x00000008
#define IE_ITEM_CURSED       0x00000010
#define IE_ITEM_NOT_COPYABLE 0x00000020
#define IE_ITEM_MAGICAL      0x00000040
#define IE_ITEM_BOW          0x00000080
#define IE_ITEM_SILVER       0x00000100
#define IE_ITEM_COLD_IRON    0x00000200
#define IE_ITEM_STOLEN       0x00000400
#define IE_ITEM_CONVERSABLE  0x00000800
#define IE_ITEM_PULSATING    0x00001000
#define IE_ITEM_UNSELLABLE   ( IE_ITEM_CRITICAL | IE_ITEM_STOLEN )
//tobex modder extensions, please note, these are not copied into the local slot bits
#define IE_ITEM_NO_DISPEL    0x01000000 //disables destruction by dispelling
#define IE_ITEM_TOGGLE_CRITS 0x02000000 //toggles critical hit avertion
#define IE_ITEM_NO_INVIS     0x04000000 //don't target invisible


//Extended header recharge flags
#define IE_ITEM_USESTRENGTH  1          //weapon
#define IE_ITEM_BREAKABLE    2          //weapon
#define IE_ITEM_USEDEXTERITY 4          //gemrb weapon (move this if tobex implements it elsewhere)
#define IE_ITEM_HOSTILE      0x400      //equipment
#define IE_ITEM_RECHARGE     0x800      //equipment
#define IE_ITEM_IGNORESHIELD 0x10000    //weapon
#define IE_ITEM_KEEN         0x20000    //weapon

//modder extensions
#define IE_ITEM_BACKSTAB     0x01000000 //can used for backstab (ranged weapon)

//item use locations (weapons are not listed in equipment list)
#define ITEM_LOC_WEAPON    1   //this is a weapon slot (uses thac0 etc)
#define ITEM_LOC_EQUIPMENT 3   //this slot appears on equipment list
//other item locations appear useless

//attack types
#define ITEM_AT_MELEE      1
#define ITEM_AT_PROJECTILE 2
#define ITEM_AT_MAGIC      3
#define ITEM_AT_BOW	4

//target types
#define TARGET_INVALID  0          //all the rest (default)
#define TARGET_CREA  1             //single living creature
#define TARGET_INV   2             //inventory item (not used?)
#define TARGET_DEAD  3             //creature, item or point
#define TARGET_AREA  4             //point target
#define TARGET_SELF  5             //self
#define TARGET_UNKNOWN 6           //unknown (use default)
#define TARGET_NONE  7             //self

//projectile qualifiers
#define PROJ_ARROW  1
#define PROJ_BOLT   2
#define PROJ_BULLET 4

//charge depletion flags
#define CHG_NONE    0
#define CHG_BREAK   1
#define CHG_NOSOUND 2
#define CHG_DAY     3

//items caster level is hardcoded to 10
#define ITEM_CASTERLEVEL   10
/**
 * @class ITMExtHeader
 * Header for special effects and uses of an Item.
 */

class GEM_EXPORT ITMExtHeader {
public:
	ITMExtHeader();
	~ITMExtHeader();
	ieByte AttackType;
	ieByte IDReq;
	ieByte Location;
	ieByte unknown1;
	ieResRef UseIcon;
	ieByte Target;
	ieByte TargetNumber;
	ieWord Range;
	//this is partially redundant, but the original engine uses this value to
	//determine projectile type (ProjectileQualifier is almost always set too)
	//We use this field only when really needed, and resolve the redundancy
	//in the exporter. The reason: using bitfields is more flexible.
	//ieWord ProjectileType;
	ieWord Speed;
	ieWord THAC0Bonus;
	ieWord DiceSides;
	ieWord DiceThrown;
	ieWordSigned DamageBonus; //this must be signed!!!
	ieWord DamageType;
	ieWord FeatureCount;
	ieWord FeatureOffset;
	ieWord Charges;
	ieWord ChargeDepletion;
	ieDword RechargeFlags; //this is a bitfield with many bits
	ieWord ProjectileAnimation;
	ieWord MeleeAnimation[3];
	//this value is set in projectiles and launchers too
	int ProjectileQualifier; //this is a derived value determined on load time
	Effect *features;
};

/**
 * @class Item
 * Class for all things your PCs can pick, carry and wear and many they can't.
 */

class GEM_EXPORT Item {
public:
	Item();
	~Item();

	ITMExtHeader *ext_headers;
	Effect *equipping_features;
	ieResRef Name; //the resref of the item itself!

	ieStrRef ItemName;
	ieStrRef ItemNameIdentified;
	ieResRef ReplacementItem;
	ieDword Flags;
	ieWord ItemType;
	ieDword UsabilityBitmask;
	char AnimationType[2];
	ieByte MinLevel;
	ieByte unknown1;
	ieByte MinStrength;
	ieByte unknown2;
	ieByte MinStrengthBonus;
	//kit1
	ieByte MinIntelligence;
	//kit2
	ieByte MinDexterity;
	//kit3
	ieByte MinWisdom;
	//kit4
	ieByte MinConstitution;
	ieByte WeaProf;
	ieByte MinCharisma;
	ieByte unknown3;
	ieDword KitUsability;
	ieDword Price;
	ieWord MaxStackAmount;
	ieResRef ItemIcon;
	ieWord LoreToID;
	ieResRef GroundIcon;
	ieDword Weight;
	ieStrRef ItemDesc;
	ieStrRef ItemDescIdentified;
	ieResRef DescriptionIcon;
	ieDword Enchantment;
	ieDword ExtHeaderOffset;
	ieWord ExtHeaderCount;
	ieDword FeatureBlockOffset;
	ieWord EquippingFeatureOffset;
	ieWord EquippingFeatureCount;

	// PST only
	ieResRef Dialog;
	ieStrRef DialogName;
	ieWord WieldColor;

	// PST and IWD2 only
	char unknown[26];
	// flag items to mutually exclusive to equip
	ieDword ItemExcl;
public:
	ieStrRef GetItemName(bool identified) const
	{
		if(identified) {
			if((int) ItemNameIdentified>=0) return ItemNameIdentified;
			return ItemName;
		}
		if((int) ItemName>=0) {
			return ItemName;
		}
		return ItemNameIdentified;
	}
	ieStrRef GetItemDesc(bool identified) const
	{
		if(identified) {
			if((int) ItemDescIdentified>=0) return ItemDescIdentified;
			return ItemDesc;
		}
		if((int) ItemDesc>=0) {
			return ItemDesc;
		}
		return ItemDescIdentified;
	}

	//returns if the item is usable, expend will also expend a charge
	int UseCharge(ieWord *Charges, int header, bool expend) const;

	//returns the requested extended header
	//-1 will return melee weapon header, -2 the ranged one
	ITMExtHeader *GetExtHeader(int which) const
	{
		if(which < 0)
			return GetWeaponHeader(which == -2) ;
		if(ExtHeaderCount<=which) {
			return NULL;
		}
		return ext_headers+which;
	}
	ieDword GetWieldedGradient() const
	{
		return WieldColor;
	}

	//-1 will return the equipping feature block
	EffectQueue *GetEffectBlock(Scriptable *self, const Point &pos, int header, ieDwordSigned invslot, ieDword pro) const;
	//returns a projectile created from an extended header
	//if miss is non-zero, then no effects will be loaded
	Projectile *GetProjectile(Scriptable *self, int header, const Point &target, ieDwordSigned invslot, int miss) const;
	//Builds an equipping glow effect from gradient colour
	//this stuff is not item specific, could be moved elsewhere
	Effect *BuildGlowEffect(int gradient) const;
	//returns the average damage of the weapon (doesn't check for special effects)
	int GetDamagePotential(bool ranged, ITMExtHeader *&header) const;
	//returns the weapon header
	ITMExtHeader *GetWeaponHeader(bool ranged) const;
	int GetWeaponHeaderNumber(bool ranged) const;
	int GetEquipmentHeaderNumber(int cnt) const;
	unsigned int GetCastingDistance(int header) const;
private:
};

}

#endif // ! ITEM_H
