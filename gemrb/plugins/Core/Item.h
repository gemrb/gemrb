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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/Item.h,v 1.1 2004/02/15 14:26:54 edheldil Exp $
 *
 */

#ifndef ITEM_H
#define ITEM_H

#include <vector>
#include "../../includes/ie_types.h"

#include "AnimationMgr.h"


#ifdef WIN32

#ifdef GEM_BUILD_DLL
#define GEM_EXPORT __declspec(dllexport)
#else
#define GEM_EXPORT __declspec(dllimport)
#endif

#else
#define GEM_EXPORT
#endif

// the same as SPLFeature
typedef struct ITMFeature {
        ieWord Opcode;
        ieByte Target;
        ieByte Power;
        ieDword Parameter1;
        ieDword Parameter2;
        ieByte TimingMode;
        ieByte Resistance;
        ieDword Duration;
        ieByte Probability1;
        ieByte Probability2;
        ieResRef Resource;
        ieDword DiceThrown;
        ieWord DiceSides;
        ieDword SavingThrowType;
        ieDword SavingThrowBonus;
        ieDword unknown;
} ITMFeature;

typedef struct ITMExtHeader {
        ieByte AttackType;
        ieByte IDReq;
        ieByte Location;
        ieByte unknown1;
        ieResRef UseIcon;
        ieByte Target;
        ieByte TargetNumber;
        ieWord Range;
        ieWord ProjectileType;
        ieWord Speed;
        ieWord THAC0Bonus;
        ieWord DiceSides;
        ieWord DiceThrown;
        ieWord DamageBonus;
        ieWord DamageType;
        ieWord FeatureCount;
        ieWord FeatureOffset;
        ieWord Charges;
        ieWord ChargeDepletion;
        ieByte UseStrengthBonus;
        ieByte Recharge;
        ieWord unknown2;
        ieWord ProjectileAnimation;
        ieWord MeleeAnimation[3];
        ieWord BowArrowQualifier;
        ieWord CrossbowBoltQualifier;
        ieWord MiscProjectileQualifier;

	std::vector<ITMFeature *> features;
} ITMExtHeader;


class GEM_EXPORT Item {
public:
        Item ();
	~Item ();

	std::vector<ITMExtHeader *> ext_headers;
	std::vector<ITMFeature *> equipping_features;

	ieStrRef ItemName;
	ieStrRef ItemNameIdentified;
	ieResRef ReplacementItem;
	ieDword Flags;
	ieWord ItemType;
	ieDword UsabilityBitmask;
	char InventoryIconType[2];
	ieWord MinLevel;
	ieWord MinStrength;
	ieWord MinStrengthBonus;
	ieWord MinIntelligence;
	ieWord MinDexterity;
	ieWord MinWisdom;
	ieWord MinConstitution;
	ieWord MinCharisma;
	ieDword Price;
	ieWord StackAmount;
	ieResRef ItemIcon;
	ieWord LoreToID;
	ieResRef GroundIcon;
	ieDword Weight;
	ieStrRef ItemDesc;
	ieStrRef ItemDescIdentified;
	ieResRef CarriedIcon;
	ieDword Enchantment;
	ieDword ExtHeaderOffset;
	ieWord ExtHeaderCount;
	ieDword FeatureBlockOffset;
	ieWord EquippingFeatureOffset;
	ieWord EquippingFeatureCount;
	ieDword unknown[7];

	AnimationMgr *ItemIconBAM;
	AnimationMgr *GroundIconBAM;
	AnimationMgr *CarriedIconBAM;
};

#endif  // ! ITEM_H
