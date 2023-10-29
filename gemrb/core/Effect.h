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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

/**
 * @file Effect.h
 * Declares Effect class implementing spell and spell-like effects
 * and related defines
 */

#ifndef EFFECT_H
#define EFFECT_H

#include "ie_types.h"

#include "Region.h"
#include "Resource.h"

namespace GemRB {

//local variables in creatures are stored in fake opcodes
#define FAKE_VARIABLE_OPCODE 187
#define FAKE_VARIABLE_MARKER 1

// Effect target types
#define FX_TARGET_UNKNOWN    0
#define FX_TARGET_SELF       1
#define FX_TARGET_PRESET     2
#define FX_TARGET_PARTY      3
#define FX_TARGET_ALL        4
#define FX_TARGET_ALL_BUT_PARTY   5
#define FX_TARGET_OWN_SIDE   6
#define FX_TARGET_OTHER_SIDE 7
#define FX_TARGET_ALL_BUT_SELF 8
#define FX_TARGET_ORIGINAL   9

// Effect duration/timing types
#define FX_DURATION_INSTANT_LIMITED          0
#define FX_DURATION_INSTANT_PERMANENT        1
#define FX_DURATION_INSTANT_WHILE_EQUIPPED   2
#define FX_DURATION_DELAY_LIMITED            3 //this contains a relative onset time (delay) also used as duration, transforms to 0 when applied
#define FX_DURATION_DELAY_PERMANENT          4 //this transforms to 9 (i guess)
#define FX_DURATION_DELAY_UNSAVED            5 //this transforms to 8
#define FX_DURATION_DELAY_LIMITED_PENDING    6 //this contains an absolute onset time and a duration
#define FX_DURATION_AFTER_EXPIRES            7 //this is a delayed non permanent effect (resolves to JUST_EXPIRED)
#define FX_DURATION_PERMANENT_UNSAVED        8
#define FX_DURATION_INSTANT_PERMANENT_AFTER_BONUSES   9//this is a special permanent
#define FX_DURATION_INSTANT_LIMITED_TICKS    10 // same as 0, but in ticks instead of seconds
#define FX_DURATION_JUST_EXPIRED             (FX_DURATION_INSTANT_LIMITED_TICKS + 1) // internal
#define MAX_TIMING_MODE                      (FX_DURATION_JUST_EXPIRED + 1)
#define FX_DURATION_ABSOLUTE                 0x1000

// Effect resistance types
#define FX_NO_RESIST_NO_DISPEL      0
#define FX_CAN_DISPEL               1
#define FX_CAN_RESIST_CAN_DISPEL     1
//#define FX_CAN_RESIST_NO_DISPEL     2   // same as 0 (not resistible, not dispellable)
#define FX_NO_RESIST_CAN_DISPEL    3
#define FX_NO_RESIST_BYPASS_BOUNCE 4 // EE bit to bypass deflection/reflection/trap opcodes
#define FX_NO_RESIST_SELF_TARGETED 8 // TODO: EE bit to fix an exploit, see IESDP
// unused gap
#define FX_SET_BY_ITEM 0x80000000 // TODO: EE bit for opcode 324, perhaps not needed (use SourceType?)

// Effect save flags (ToBEx)
#define SF_BYPASS_MIRROR_IMAGE 0x1000000
#define SF_IGNORE_DIFFICULTY   0x2000000

/**
 * @class Effect
 * Structure holding information about single spell or spell-like effect.
 */

/** Cached Effect -> opcode mapping */
struct EffectRef {
	const char* Name;
	int opcode;
};

struct ResourceGroup {
	//keep these four in one bunch, VariableName will
	//spread across them
	ResRef Resource;
	ResRef Resource2; //vvc in a lot of effects
	ResRef Resource3;
	ResRef Resource4;
};

// the same as ITMFeature and SPLFeature
struct GEM_EXPORT Effect {
	ieDword Opcode = 0;
	ieDword Target = 0;
	ieDword Power = 0; // the effect level
	ieDword Parameter1 = 0;
	ieDword Parameter2 = 0;
	ieWord TimingMode = 0;   //0x1000 -- no need of conversion
	ieWord unknown2 = 0;
	ieDword Resistance = 0;
	ieDword Duration = 0;
	ieWord ProbabilityRangeMax = 0;
	ieWord ProbabilityRangeMin = 0;
	
	union {
		ResourceGroup resources; // keep largest type first to 0 fill everything
		ieVariable VariableName;
	};

	ResRef& Resource = resources.Resource;
	ResRef& Resource2 = resources.Resource2; //vvc in a lot of effects
	ResRef& Resource3 = resources.Resource3;
	ResRef& Resource4 = resources.Resource4;

	ieDword DiceThrown = 0;
	ieDword DiceSides = 0;
	ieDword SavingThrowType = 0;
	ieDword SavingThrowBonus = 0;
	// IsVariable is the "Special" field in EEs (use depends on opcode) and has two meanings in PST:
	// - personalized critical type (Actor::GetCriticalType)
	// - onset delay override for FX_DURATION_DELAY_LIMITED
	ieWord IsVariable = 0;
	ieWord IsSaveForHalfDamage = 0;

	// EFF V2.0 fields:
	ieDword PrimaryType = 0; //school
	ieDword MinAffectedLevel = 0;
	ieDword MaxAffectedLevel = 0;
	ieDword Parameter3 = 0;
	ieDword Parameter4 = 0;
	ieDword Parameter5 = 0;
	ieDword Parameter6 = 0;
	Point Source;
	Point Pos;
	ieDword SourceType = 0; //1-item, 2-spell
	ResRef SourceRef;
	ieDword SourceFlags = 0;
	ieDword Projectile = 0;          //9c
	ieDwordSigned InventorySlot = 0; //a0
	//Original engine had a VariableName here, but it is stored in the resource fields
	ieDword CasterLevel = 0;  //c4 in both
	ieDword FirstApply = 0;   //c8 in bg2, cc in iwd2
	ieDword SecondaryType = 0;
	ieDword SecondaryDelay = 0; //still not sure about this
	ieDword CasterID = 0;       //10c in bg2 (not saved?)
	// These are not in the IE files, but are our precomputed values
	ieDword RandomValue = 0;

	ieDword SpellLevel = 0; // Power does not always contain the Source level, which is needed in iwd2; items will be left at 0
public:
	//don't modify position in case it was already set
	void SetPosition(const Point &p);
	void SetSourcePosition(const Point &p);
	
	Effect() noexcept
	: resources()
	{
		// must define our own empty ctor due to union type
		// union is already 0 initialized so there is nothing else to do
	}
	
	Effect(const Effect& rhs) noexcept;

	Effect& operator=(const Effect& rhs) noexcept;

	~Effect() noexcept = default;

	bool operator==(const Effect& rhs) const noexcept;
	
	//returns true if the effect supports simplified duration
	bool HasDuration() const;
	
	//returns true if the effect must be saved
	bool Persistent() const;

	void PrepareDuration(ieDword gameTime);
};

}

#endif  // ! EFFECT_H
