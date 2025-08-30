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
 * @file Spell.h
 * Declares Spell, class for magic incantations, cleric prayers, 
 * bardic songs and innate abilities
 * @author The GemRB Project
 */

#ifndef SPELL_H
#define SPELL_H

#include "exports.h"
#include "ie_types.h"

#include "EffectQueue.h"

namespace GemRB {

class Projectile;

//values for Spell usability Flags

#define SF_BREAK_SANCTUARY 0x200 // EE bit to force the removal of any fx_set_sanctuary_state effects
#define SF_HOSTILE         0x400 // bit 18
#define SF_NO_LOS          0x800
// unknown Allow spotting  0x1000
#define SF_NOT_INDOORS   0x2000
#define SF_HLA           0x4000 // this means a nonmagical ability (also ignores dead-magic and wild surge effect)
#define SF_TRIGGER       0x8000
#define SF_NOT_IN_COMBAT 0x10000 // bit 24 unused
// 7 unknowns, likely unused
#define SF_TARGETS_INVISIBLE 0x1000000 // tobex/ee: can target invisible creatures
#define SF_IGNORES_SILENCE   0x2000000 // tobex/ee: can be cast while silenced
//this is a relocated bit (used in iwd2 as 0x4000)
#define SF_SIMPLIFIED_DURATION 0x40

//spelltypes in spells
#define IE_SPL_ITEM   0
#define IE_SPL_WIZARD 1
#define IE_SPL_PRIEST 2
#define IE_SPL_PSION  3
#define IE_SPL_INNATE 4
#define IE_SPL_SONG   5

// this is not the same as the book types (which is 3 or 11)
#define NUM_SPELL_TYPES 6

#define SPEC_IDENTIFY 1 //spells that don't appear in the casting bar
#define SPEC_SILENCE  2 //spells that can be cast when silenced
#define SPEC_DEAD     4 //spells that can target dead actors despite their target type is 1 (pst hack)
#define SPEC_AREA     32 // spells that can target the area despite their target type being 1

/**
 * @class SPLExtHeader
 * Header for Spell special effects
 */

class GEM_EXPORT SPLExtHeader {
public:
	SPLExtHeader() noexcept = default;

	ieByte SpellForm = 0;
	ieByte Hostile = 0;
	ieByte Location = 0;
	ieByte unknown2 = 0;
	ResRef memorisedIcon;
	ieByte Target = 0;
	ieByte TargetNumber = 0;
	ieWord Range = 0;
	ieWord RequiredLevel = 0;
	ieDword CastingTime = 0;
	ieWord DiceSides = 0;
	ieWord DiceThrown = 0;
	ieWord DamageBonus = 0;
	ieWord DamageType = 0;
	ieWord FeatureOffset = 0;
	ieWord Charges = 0;
	ieWord ChargeDepletion = 0;
	ieWord ProjectileAnimation = 0;
	std::vector<Effect> features;
};

/**
 * @class Spell
 * Class for magic incantations, cleric prayers, 
 * bardic songs and innate abilities.
 */

class GEM_EXPORT Spell {
public:
	Spell() noexcept = default;

	std::vector<SPLExtHeader> ext_headers;
	std::vector<Effect> casting_features;

	/** Resref of the spell itself */
	ResRef Name;
	ieStrRef SpellName = ieStrRef::INVALID;
	ieStrRef SpellNameIdentified = ieStrRef::INVALID;
	ResRef CompletionSound;
	ieDword Flags = 0;
	ieWord SpellType = 0;
	ieWord ExclusionSchool = 0;
	ieWord PriestType = 0;
	ieWord CastingGraphics = 0;
	ieByte unknown1 = 0;
	ieWord PrimaryType = 0;
	ieByte SecondaryType = 0;
	ieDword unknown2 = 0;
	ieDword unknown3 = 0;
	ieDword unknown4 = 0;
	ieDword SpellLevel = 0;
	ieWord unknown5 = 0;
	ResRef SpellbookIcon;
	ieWord unknown6 = 0;
	ieDword unknown7 = 0;
	ieDword unknown8 = 0;
	ieDword unknown9 = 0;
	ieStrRef SpellDesc = ieStrRef::INVALID;
	ieStrRef SpellDescIdentified = ieStrRef::INVALID;
	ieDword unknown10 = 0;
	ieDword unknown11 = 0;
	ieDword unknown12 = 0;
	ieDword ExtHeaderOffset = 0;
	ieDword FeatureBlockOffset = 0;
	ieWord CastingFeatureOffset = 0;
	ieWord CastingFeatureCount = 0;

	// IWD2 only
	ieByte TimePerLevel = 0;
	ieByte TimeConstant = 0;
	char unknown13[14];
	//derived values
	int CastingSound = 0;

public:
	//returns the requested extended header
	inline const SPLExtHeader* GetExtHeader(size_t which) const
	{
		if (Flags & SF_SIMPLIFIED_DURATION) {
			which = 0;
		}

		if (ext_headers.size() <= which) {
			return NULL;
		}
		return &ext_headers[which];
	}
	//converts a wanted level to block index count
	int GetHeaderIndexFromLevel(int level) const;
	//-1 will return the cfb
	EffectQueue GetEffectBlock(Scriptable* self, const Point& pos, int block_index, int level, ieDword pro = 0);
	// add appropriate casting glow effect
	void AddCastingGlow(EffectQueue* fxqueue, ieDword duration, int gender) const;
	//returns a projectile created from an extended header
	Projectile* GetProjectile(Scriptable* self, int headerindex, int level, const Point& pos);
	unsigned int GetCastingDistance(Scriptable* Sender) const;
	bool ContainsDamageOpcode() const;
	bool ContainsTamingOpcode() const;
};

}

#endif // ! SPELL_H
