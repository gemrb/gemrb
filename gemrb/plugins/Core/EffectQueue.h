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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/EffectQueue.h,v 1.24 2006/04/10 18:18:13 avenger_teambg Exp $
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
///** these effects stick but didn't trigger yet */
#define FX_PENDING    3

//remove level effects flags
#define RL_DISPELLABLE  1  //only dispellables
#define RL_MATCHSCHOOL  2  //match school
#define RL_MATCHSECTYPE 4  //match secondary type
#define RL_REMOVEFIRST  8  //remove only one spell (could be more effects)

#define BNC_PROJECTILE  1
#define BNC_OPCODE      2
#define BNC_LEVEL       4
#define BNC_SCHOOL      8
#define BNC_SECTYPE     0x10
#define BNC_RESOURCE    0x20
#define BNC_PROJECTILE_DEC 0x100
#define BNC_OPCODE_DEC  0x200
#define BNC_LEVEL_DEC   0x400
#define BNC_SCHOOL_DEC  0x800
#define BNC_SECTYPE_DEC 0x1000
#define BNC_RESOURCE_DEC 0x2000

// FIXME: Dice roll should be probably done just once, e.g. when equipping 
// the item, not each time the fx are applied
// <avenger> the dice values are actually level limits, except in 3 hp modifier functions
// the damage function is an instant (the other 2 functions might be tricky with random values)
#define DICE_ROLL(max_val) ((fx->DiceThrown && fx->DiceSides) ? ((max_val >=0) ? (MIN( core->Roll( fx->DiceThrown, fx->DiceSides, 0 ), max_val )) : (MAX( core->Roll( fx->DiceThrown, fx->DiceSides, 0 ), max_val ))) : max_val)

// often used stat modifications, usually Parameter2 types 0, 1 and 2
//these macros should work differently in permanent mode (modify base too)
#define STAT_GET(stat) (target->Modified[ stat ])
#define STAT_ADD(stat, mod) target->SetStat( stat, STAT_GET( stat ) + ( mod ), 0 )
#define STAT_BIT_OR(stat, mod) target->SetStat( stat, STAT_GET( stat ) | ( mod ), 0 )
#define STAT_SET(stat, mod) target->SetStat( stat,  ( mod ), 0 )
#define STAT_MUL(stat, mod) target->SetStat( stat, STAT_GET(stat) * (( mod ) / 100), 0 )
#define STATE_CURE( mod ) target->Modified[ IE_STATE_ID ] &= ~(ieDword) ( mod )
#define STATE_SET( mod ) target->Modified[ IE_STATE_ID ] |= (ieDword) ( mod )
#define STATE_GET( mod ) (target->Modified[ IE_STATE_ID ] & (ieDword) ( mod ) )
#define STAT_MOD( stat ) target->NewStat(stat, fx->Parameter1, fx->Parameter2)
#define BASE_SET(stat, mod) target->SetBase( stat,  ( mod ) )
#define BASE_MOD(stat) target->NewBase( stat, fx->Parameter1, fx->Parameter2)

/** Prototype of a function implementing a particular Effect opcode */
typedef int (* EffectFunction)(Actor*, Actor*, Effect*);


/** Links Effect name to a function implementing the effect */
struct EffectRef {
	const char* Name;
	EffectFunction Function;
	int EffText; //also opcode in another context
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
	//remove all effects of a given spell
	void RemoveAllEffects(ieResRef Removed);
	void RemoveAllEffects(EffectRef &effect_reference);
	void RemoveAllEffectsWithParam(EffectRef &effect_reference, ieDword param2);
	void RemoveLevelEffects(ieDword level, ieDword flags, ieDword match);
	Effect *GetEffect(ieDword idx) const;
	/* returns next saved effect, increases index */
	Effect *GetNextSavedEffect(ieDword &idx) const;
	/* returns the number of saved effects */
	ieDword GetSavedEffectsCount() const;
	size_t GetEffectsCount() const { return effects.size(); }

	//locating opcodes
	Effect *HasEffect(EffectRef &effect_reference);
	Effect *HasEffectWithParam(EffectRef &effect_reference, ieDword param2);
	Effect *HasEffectWithParamPair(EffectRef &effect_reference, ieDword param1, ieDword param2);
	Effect *HasEffectWithResource(EffectRef &effect_reference, ieResRef resource);
	bool HasAnyDispellableEffect() const;

	//getting summarised effects
	int BonusAgainstCreature(EffectRef &effect_reference, Actor *actor);

	// returns -1 if bounced, 0 if resisted, 1 if accepted spell
	int CheckImmunity(Actor *target);
	/** Lists contents of the queue on a terminal for debugging */
	void dump();
	//resolve effect
	static int ResolveEffect(EffectRef &effect_reference);
private:
	//use the effect reference style calls from outside
	void RemoveAllEffects(ieDword opcode);
	void RemoveAllEffectsWithParam(ieDword opcode, ieDword param2);
	Effect *HasOpcode(ieDword opcode) const;
	Effect *HasOpcodeWithParam(ieDword opcode, ieDword param2) const;
	Effect *HasOpcodeWithParamPair(ieDword opcode, ieDword param1, ieDword param2) const;
	Effect *HasOpcodeWithResource(ieDword opcode, ieResRef resource) const;
	int BonusAgainstCreature(ieDword opcode, Actor *actor) const;
};

#endif  // ! EFFECTQUEUE_H
