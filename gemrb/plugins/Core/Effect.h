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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/Effect.h,v 1.2 2005/05/17 13:52:01 avenger_teambg Exp $
 *
 */

#ifndef EFFECT_H
#define EFFECT_H

//#include <vector>
#include "../../includes/ie_types.h"
class Actor;


// Effect target types
#define FX_TARGET_UNKNOWN    0
#define FX_TARGET_SELF       1
#define FX_TARGET_PRESET     2
#define FX_TARGET_PARTY      3
#define FX_TARGET_GLOBAL_INCL_PARTY   4
#define FX_TARGET_GLOBAL_EXCL_PARTY   5

// Effect duration/timing types
#define FX_DURATION_INSTANT_LIMITED          0
#define FX_DURATION_INSTANT_PERMANENT        1
#define FX_DURATION_INSTANT_WHILE_EQUIPPED   2
#define FX_DURATION_DELAY_LIMITED            3
#define FX_DURATION_DELAY_PERMANENT          4
#define FX_DURATION_UNKNOWN5                 5
#define FX_DURATION_UNKNOWN6                 6
#define FX_DURATION_AFTER_EXPIRES            7
#define FX_DURATION_UNKNOWN8                 8
#define FX_DURATION_INSTANT_PERMANENT_AFTER_BONUSES   9

// Effect resistance types
#define FX_RESIST_NO_DISPELL_BYPASS      0
#define FX_RESIST_DISPELL_NO_BYPASS      1
#define FX_RESIST_NO_DISPELL_NO_BYPASS   2
#define FX_RESIST_DISPELL_BYPASS         3



// the same as ITMFeature and SPLFeature
typedef struct Effect {
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
	//keep these four in one bunch, VariableName will
	//spread across them
	ieResRef Resource;
	ieResRef Resource2;
	ieResRef Resource3;
	ieResRef Resource4;
	ieDword DiceThrown;
	ieDword DiceSides;
	ieDword SavingThrowType;
	ieDword SavingThrowBonus;
	ieDword unknown;

	// These are not in the IE files, but are our precomputed values
	int  random_value;
} Effect;

// FIXME: what about area spells? They can have map & coordinates as target
void AddEffect(Effect* fx, Actor* self, Actor* pretarget);


#endif  // ! EFFECT_H
