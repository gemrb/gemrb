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

class Projectile;

//values for Spell usability Flags

#define SF_HOSTILE	0x400
#define SF_NO_LOS	0x800
#define SF_NOT_INDOORS	0x2000
#define SF_HLA  	0x4000 // probably this means a nonmagical ability
#define SF_TRIGGER	0x8000
#define SF_NOT_IN_COMBAT 0x10000
//this is a relocated bit (used in iwd2 as 0x4000)
#define SF_SIMPLIFIED_DURATION 0x40

//spelltypes in spells
#define  IE_SPL_ITEM   0
#define  IE_SPL_WIZARD 1
#define  IE_SPL_PRIEST 2
#define  IE_SPL_PSION  3
#define  IE_SPL_INNATE 4
#define  IE_SPL_SONG   5

//this is not the same as the book types which is 3 or 11)
#define NUM_SPELL_TYPES 6

#define SPEC_IDENTIFY  1  //spells that don't appear in the casting bar
#define SPEC_SILENCE   2  //spells that can be cast when silenced
#define SPEC_DEAD      4  //spells that can target dead actors despite their target type is 1 (pst hack)
/**
 * @class SPLExtHeader
 * Header for Spell special effects
 */

class GEM_EXPORT SPLExtHeader {
public:
	SPLExtHeader();
	~SPLExtHeader();

	ieByte SpellForm;
	ieByte Hostile;
	ieByte Location;
	ieByte unknown2;
	ieResRef MemorisedIcon;
	ieByte Target;
	ieByte TargetNumber;
	ieWord Range;
	ieWord RequiredLevel;
	ieDword CastingTime;
	ieWord DiceSides;
	ieWord DiceThrown;
	ieWord DamageBonus;
	ieWord DamageType;
	ieWord FeatureCount;
	ieWord FeatureOffset;
	ieWord Charges;
	ieWord ChargeDepletion;
	ieWord ProjectileAnimation;
	Effect* features;
};

/**
 * @class Spell
 * Class for magic incantations, cleric prayers, 
 * bardic songs and innate abilities.
 */

class GEM_EXPORT Spell {
public:
	Spell();
	~Spell();

	SPLExtHeader *ext_headers;
	Effect* casting_features;

	/** Resref of the spell itself */
	ieResRef Name;
	ieStrRef SpellName;
	ieStrRef SpellNameIdentified;
	ieResRef CompletionSound;
	ieDword Flags;
	ieWord SpellType;
	ieWord ExclusionSchool;
	ieWord PriestType;
	ieWord CastingGraphics;
	ieByte unknown1;
	ieWord PrimaryType;
	ieByte SecondaryType;
	ieDword unknown2;
	ieDword unknown3;
	ieDword unknown4;
	ieDword SpellLevel;
	ieWord unknown5;
	ieResRef SpellbookIcon;
	ieWord unknown6;
	ieDword unknown7;
	ieDword unknown8;
	ieDword unknown9;
	ieStrRef SpellDesc;
	ieStrRef SpellDescIdentified;
	ieDword unknown10;
	ieDword unknown11;
	ieDword unknown12;
	ieDword ExtHeaderOffset;
	ieWord ExtHeaderCount;
	ieDword FeatureBlockOffset;
	ieWord CastingFeatureOffset;
	ieWord CastingFeatureCount;

	// IWD2 only
	ieDword TimePerLevel;
	ieDword TimeConstant;
	char unknown13[8];
	//derived values
	int CastingSound;

public:
	//returns the requested extended header
	inline SPLExtHeader *GetExtHeader(unsigned int which) const
	{
		if (Flags & SF_SIMPLIFIED_DURATION) {
			which = 0;
		}

		if(ExtHeaderCount<=which) {
			return NULL;
		}
		return ext_headers+which;
	}
	//converts a wanted level to block index count
	int GetHeaderIndexFromLevel(int level) const;
	//-1 will return the cfb
	EffectQueue *GetEffectBlock(Scriptable *self, const Point &pos, int block_index, int level, ieDword pro=0) const;
	// add appropriate casting glow effect
	void AddCastingGlow(EffectQueue *fxqueue, ieDword duration, int gender);
	//returns a projectile created from an extended header
	Projectile *GetProjectile(Scriptable *self, int headerindex, int level, const Point &pos) const;
	unsigned int GetCastingDistance(Scriptable *Sender) const;
	bool ContainsDamageOpcode() const;
};

#endif  // ! SPELL_H
