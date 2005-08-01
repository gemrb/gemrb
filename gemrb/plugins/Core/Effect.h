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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/Effect.h,v 1.7 2005/08/01 20:52:13 avenger_teambg Exp $
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
#define FX_DURATION_DELAY_LIMITED            3 //this contains a relative onset time (delay) also used as duration, transforms to 6 when applied
#define FX_DURATION_DELAY_PERMANENT          4 //this transforms to 9 (i guess)
#define FX_DURATION_UNKNOWN5                 5 //this transforms to 8, but unknown
#define FX_DURATION_DELAY_LIMITED_PENDING    6 //this contains an absolute onset time and a duration
#define FX_DURATION_AFTER_EXPIRES            7
#define FX_DURATION_UNKNOWN8                 8
#define FX_DURATION_INSTANT_PERMANENT_AFTER_BONUSES   9//this is a special permanent
#define FX_DURATION_JUST_EXPIRED             10

// Effect resistance types
#define FX_NO_RESIST_NO_DISPEL      0
#define FX_CAN_RESIST_CAN_DISPEL    1
#define FX_CAN_RESIST_NO_DISPEL     2
#define FX_NO_RESIST_CAN_DISPEL     3
#define FX_CAN_DISPEL               1

// the same as ITMFeature and SPLFeature
typedef struct Effect {
	ieDword Opcode;
	ieDword Target;
	ieDword Power;
	ieDword Parameter1;
	ieDword Parameter2;
	ieDword TimingMode;
	ieDword Resistance;
	ieDword Duration;
	ieWord Probability1;
	ieWord Probability2;
	//keep these four in one bunch, VariableName will
	//spread across them
	ieResRef Resource;
	ieResRef Resource2; //vvc in a lot of effects
	ieResRef Resource3;
	ieResRef Resource4;
	ieDword DiceThrown;
	ieDword DiceSides;
	ieDword SavingThrowType;
	ieDword SavingThrowBonus;
	ieDword unknown;

	// EFF V2.0 fields:
	ieDword PrimaryType;
	ieDword Parameter3;
	ieDword Parameter4;
	//ieResRef VVCResource; //this is actually Resource2
	ieResRef Source;
	//char Variable[32]; //resources will cover this
	ieDword SecondaryType;

	// These are not in the IE files, but are our precomputed values
	int  random_value;
} Effect;

// FIXME: what about area spells? They can have map & coordinates as target
void AddEffect(Effect* fx, Actor* self, Actor* pretarget);


#endif  // ! EFFECT_H
