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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/EffectQueue.h,v 1.13 2005/10/20 20:39:14 edheldil Exp $
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

/** Initializes table of available spell Effects used by all the queues */
bool Init_EffectQueue();


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

/** Prototype of a function implementing a particular Effect opcode */
typedef int (* EffectFunction)(Actor*, Actor*, Effect*);

/** Links Effect name to a function implementing the effect */
struct EffectLink {
	const char* Name;
	EffectFunction Function;
};


#endif  // ! EFFECTQUEUE_H
