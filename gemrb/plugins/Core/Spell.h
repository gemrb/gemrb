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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/Spell.h,v 1.2 2004/02/15 23:56:05 edheldil Exp $
 *
 */

#ifndef SPELL_H
#define SPELL_H

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


typedef struct SPLFeature {
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
} SPLFeature;

typedef struct SPLExtHeader {
        ieByte SpellForm;
        ieByte unknown1;
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
        ieWord Enchanted;
        ieWord FeatureCount;
        ieWord FeatureOffset;
        ieWord Charges;
        ieWord ChargeDepletion;
        ieWord Projectile;
	std::vector<SPLFeature *> features;
} SPLExtHeader;


class GEM_EXPORT Spell {
public:
        Spell ();
	~Spell ();

	std::vector<SPLExtHeader *> ext_headers;
	std::vector<SPLFeature *> casting_features;

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
	ieResRef SpellBookIcon;
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
	char unknown13[16];

	AnimationMgr *SpellIconBAM;
};

#endif  // ! SPELL_H
