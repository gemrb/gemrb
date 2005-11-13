/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/EffectQueue.h,v 1.15 2005/11/13 11:23:50 avenger_teambg Exp $
 *
 */

/**
 * @file EffectQueue.h
 * Declares EffectQueue class holding and processing all spell effects 
 * on a single Actor
 * @author The GemRB Project
 */

#ifndef EFFECTQUEUE_H
#define EFFECTQUEUE_H


#include <vector>
#include "Effect.h"

class Actor;


/** Maximum number of different Effect opcodes */
#define MAX_EFFECTS 512

/** these effects never stick around, use them for instant effects like damage */
#define FX_NOT_APPLIED 0 
/** these effects always stick around when applied as permanent or duration */
#define FX_APPLIED 1	
/** these effects don't stick around if used as permanent, 
 * in that case they modify a base stat like charisma modifier */
#define FX_PERMANENT 2
/** these effects stick and also repeatedly trigger like poison */
#define FX_CYCLIC    3


// FIXME: Dice roll should be probably done just once, e.g. when equipping 
// the item, not each time the fx are applied
// <avenger> the dice values are actually level limits, except in 3 hp modifier functions
// the damage function is an instant (the other 2 functions might be tricky with random values)
#define DICE_ROLL(max_val) ((fx->DiceThrown && fx->DiceSides) ? ((max_val >=0) ? (MIN( core->Roll( fx->DiceThrown, fx->DiceSides, 0 ), max_val )) : (MAX( core->Roll( fx->DiceThrown, fx->DiceSides, 0 ), max_val ))) : max_val)

// often used stat modifications, usually Parameter2 types 0, 1 and 2
//#define STAT_ADD(stat, mod) target->SetStat( ( stat ), (ieDword)(target->GetStat( stat ) + ( mod )))
//#define STAT_SET(stat, mod) target->SetStat( ( stat ), (ieDword)( mod ))
//#define STAT_MUL(stat, mod) target->SetStat( ( stat ), (ieDword)(target->GetStat( stat ) * (( mod ) / 100.0)))

//these macros should work differently in permanent mode (modify base too)
#define STAT_GET(stat) (target->Modified[ stat ])
#define STAT_ADD(stat, mod) target->Modified[ stat ] = (ieDword)(target->Modified[ stat ] + ( mod ))
#define STAT_SET(stat, mod) target->Modified[ stat ] = (ieDword)( mod )
#define STAT_MUL(stat, mod) target->Modified[ stat ] = (ieDword)(target->Modified[ stat ] * (( mod ) / 100.0))
#define STATE_CURE( mod ) target->Modified[ IE_STATE_ID ] &= ~(ieDword) ( mod )
#define STATE_SET( mod ) target->Modified[ IE_STATE_ID ] |= (ieDword) ( mod )
#define STATE_GET( mod ) (target->Modified[ IE_STATE_ID ] & (ieDword) ( mod ) )
#define STAT_MOD( stat ) target->NewStat(stat, fx->Parameter1, fx->Parameter2)



/** Prototype of a function implementing a particular Effect opcode */
typedef int (* EffectFunction)(Actor*, Actor*, Effect*);


/** Links Effect name to a function implementing the effect */
struct EffectRef {
	const char* Name;
	EffectFunction Function;
	int EffText;
};

/** Initializes table of available spell Effects used by all the queues */
bool Init_EffectQueue();

/** Registers opcodes implemented by a plugin */
void EffectQueue_RegisterOpcodes(int count, EffectRef *opcodes);


bool match_ids(Actor *target, int table, ieDword value);


/**
 * @class EffectQueue
 * Class holding and processing spell Effects on a single Actor
 */

class GEM_EXPORT EffectQueue {
	/** List of Effects applied on the Actor */
	std::vector< Effect* >  effects;
	/** Actor which is target of the Effects */
	Actor* Owner;

public:
	EffectQueue();
	virtual ~EffectQueue();

	/** Sets Actor which is affected by these effects */
	void SetOwner(Actor* act) { Owner = act; }
	/** Returns Actor affected by these effects */
	Actor* GetOwner() { return Owner; }

	/** Adds an Effect to the queue, subject to level and other checks.
	 * Returns true is successful. fx is just a reference, AddEffect()
	 * will malloc its own copy */
	bool AddEffect(Effect* fx);
	/** Removes first Effect matching fx from the queue. 
	 * Effects are matched based on their contents */
	bool RemoveEffect(Effect* fx);

	void AddAllEffects(Actor* target);
	void ApplyAllEffects(Actor* target);
	void ApplyEffect(Actor* target, Effect* fx, bool first_apply);
	void PrepareDuration(Effect* fx);
	void RemoveAllEffects(ieDword opcode);
	void RemoveLevelEffects(ieDword level, bool dispellable);

	//locating opcodes
	Effect *HasOpcodeWithParam(ieDword opcode, ieDword param2);
	Effect *HasOpcodeWithParamPair(ieDword opcode, ieDword param1, ieDword param2);
	Effect *HasOpcodeMatchingCreature(ieDword opcode, Actor *actor);
	Effect *HasOpcodeWithResource(ieDword opcode, ieResRef resource);

	/** Lists contents of the queue on a terminal for debugging */
	void dump();
};

#endif  // ! EFFECTQUEUE_H
