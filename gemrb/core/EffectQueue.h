/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
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

#include "exports.h"

#include "Effect.h"
#include "Region.h"

#include <cstdlib>
#include <list>

namespace GemRB {

class Actor;
class Map;
class Scriptable;
class StringBuffer;

/** Maximum number of different Effect opcodes */
#define MAX_EFFECTS 512

///** if the effect returns this, stop adding any other effect */
#define FX_ABORT 0
/** these effects don't stick around if used as permanent,
 * in that case they modify a base stat like charisma modifier */
#define FX_PERMANENT 2
/** these effects never stick around, use them for instant effects like damage */
#define FX_NOT_APPLIED 3
/** these effects always stick around when applied as permanent or duration */
#define FX_APPLIED 1	
///** insert the effect instead of push back */
#define FX_INSERT 4

//remove level effects flags
#define RL_DISPELLABLE  1  //only dispellables
#define RL_MATCHSCHOOL  2  //match school
#define RL_MATCHSECTYPE 4  //match secondary type
#define RL_REMOVEFIRST  8  //remove only one spell (could be more effects)

//bouncing immunities
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

//normal immunities
#define IMM_PROJECTILE  1
#define IMM_OPCODE      2
#define IMM_LEVEL       4
#define IMM_SCHOOL      8
#define IMM_SECTYPE     16
#define IMM_RESOURCE    32
#define IMM_PROJECTILE_DEC 0x100
#define IMM_OPCODE_DEC  0x200
#define IMM_LEVEL_DEC   0x400
#define IMM_SCHOOL_DEC  0x800
#define IMM_SECTYPE_DEC 0x1000
#define IMM_RESOURCE_DEC 0x2000

//pst immunities
#define IMM_GUARDIAN  0x80000000

// FIXME: Dice roll should be probably done just once, e.g. when equipping
// the item, not each time the fx are applied
// <avenger> the dice values are actually level limits, except in 3 hp modifier functions
// the damage function is an instant (the other 2 functions might be tricky with random values)
//#define DICE_ROLL(max_val) ((fx->DiceThrown && fx->DiceSides) ? ((max_val >=0) ? (MIN( core->Roll( fx->DiceThrown, fx->DiceSides, 0 ), max_val )) : (MAX( core->Roll( fx->DiceThrown, fx->DiceSides, 0 ), max_val ))) : max_val)

//sometimes damage doesn't comply with the calculated value
#define DICE_ROLL(adjustment) (core->Roll( fx->DiceThrown, fx->DiceSides, adjustment) )

// You will need to get GameTime somehow to use this macro
#define	PrepareDuration(fx) fx->Duration = (fx->Duration*AI_UPDATE_TIME + GameTime)

//return the caster object
#define GetCasterObject()  (core->GetGame()->GetActorByGlobalID(fx->CasterID))

// often used stat modifications, usually Parameter2 types 0, 1 and 2
//these macros should work differently in permanent mode (modify base too)
#define STAT_GET(stat) (target->Modified[ stat ])
#define STAT_ADD(stat, mod) target->SetStat( stat, STAT_GET( stat ) + ( mod ), 0 )
#define STAT_ADD_PCF(stat, mod) target->SetStat( stat, STAT_GET( stat ) + ( mod ), 1 )
#define STAT_SUB(stat, mod) target->SetStat( stat, STAT_GET( stat ) - ( mod ), 0 )
#define STAT_BIT_OR(stat, mod) target->SetStat( stat, STAT_GET( stat ) | ( mod ), 0 )
#define STAT_SET(stat, mod) target->SetStat( stat, ( mod ), 0 )
#define STAT_SET_PCF(stat, mod) target->SetStat( stat, ( mod ), 1 )
#define STAT_BIT_OR_PCF(stat, mod) target->SetStat( stat, STAT_GET( stat ) | ( mod ), 1 )
#define STAT_MUL(stat, mod) target->SetStat( stat, STAT_GET(stat) * ( mod ) / 100, 0 )
//if an effect sticks around
#define STATE_CURE( mod ) target->Modified[ IE_STATE_ID ] &= ~(ieDword) ( mod )
#define STATE_SET( mod ) target->Modified[ IE_STATE_ID ] |= (ieDword) ( mod )
#define EXTSTATE_SET( mod ) target->Modified[ IE_EXTSTATE_ID ] |= (ieDword) ( mod )
#define STATE_GET( mod ) (target->Modified[ IE_STATE_ID ] & (ieDword) ( mod ) )
#define EXTSTATE_GET( mod ) (target->Modified[ IE_EXTSTATE_ID ] & (ieDword) ( mod ) )
#define STAT_MOD( stat ) target->NewStat(stat, fx->Parameter1, fx->Parameter2)
#define STAT_MOD_VAR( stat, mod ) target->NewStat(stat, ( mod ) , fx->Parameter2 )
#define BASE_GET(stat) (target->BaseStats[ stat ])
#define BASE_SET(stat, mod) target->SetBase( stat, ( mod ) )
#define BASE_ADD(stat, mod) target->SetBase( stat, BASE_GET(stat)+ ( mod ) )
#define BASE_SUB(stat, mod) target->SetBase( stat, BASE_GET(stat)- ( mod ) )
#define BASE_MUL(stat, mod) target->SetBase( stat, BASE_GET(stat)* ( mod ) / 100 )
#define BASE_MOD(stat) target->NewBase( stat, fx->Parameter1, fx->Parameter2)
#define BASE_MOD_VAR(stat, mod) target->NewBase( stat, (mod), fx->Parameter2 )
//if an effect doesn't stick (and has permanent until cured effect) then
//it has to modify the base stat (which is saved)
//also use this one if the effect starts a cure effect automatically
#define BASE_STATE_SET( mod ) target->SetBaseBit( IE_STATE_ID, ( mod ), true )
#define BASE_STATE_CURE( mod ) target->SetBaseBit( IE_STATE_ID, ( mod ), false )

/** Prototype of a function implementing a particular Effect opcode */
typedef int (* EffectFunction)(Scriptable*, Actor*, Effect*);


/** Cached Effect -> opcode mapping */
struct EffectRef {
	const char* Name;
	int opcode;
};

/** Links Effect name to a function implementing the effect */
struct EffectDesc {
	const char* Name;
	EffectFunction Function;
	int Flags;
	int opcode;
};

enum EffectFlags {
	EFFECT_NORMAL = 0,
	EFFECT_DICED = 1,
	EFFECT_NO_LEVEL_CHECK = 2,
	EFFECT_NO_ACTOR = 4,
	EFFECT_REINIT_ON_LOAD = 8,
	EFFECT_PRESET_TARGET = 16
};

/** Initializes table of available spell Effects used by all the queues. */
/** The available effects should already be registered by the effect plugins */
bool Init_EffectQueue();

/** Registers opcodes implemented by an effect plugin */
void EffectQueue_RegisterOpcodes(int count, const EffectDesc *opcodes);

/** release effect list when Interface is destroyed */
void EffectQueue_ReleaseMemory();

/** Check if opcode is for an effect that takes a color slot as parameter. */
bool IsColorslotEffect(int opcode);

/**
 * @class EffectQueue
 * Class holding and processing spell Effects on a single Actor
 */

class GEM_EXPORT EffectQueue {
private:
	/** List of Effects applied on the Actor */
	std::list< Effect* > effects;
	/** Actor which is target of the Effects */
	Scriptable* Owner;

public:
	EffectQueue();
	virtual ~EffectQueue();

	/** Sets Actor which is affected by these effects */
	void SetOwner(Scriptable* act) { Owner = act; }
	/** Returns Actor affected by these effects */
	Scriptable* GetOwner() const { return Owner; }

	/** adds an effect to the queue, it could also insert it if flagged so
	 *  fx should be freed by the caller
	 */
	void AddEffect(Effect* fx, bool insert=false);
	/** Adds an Effect to the queue, subject to level and other checks.
	 * Returns FX_ABORT is unsuccessful. fx is just a reference, AddEffect()
	 * will malloc its own copy */
	int AddEffect(Effect* fx, Scriptable* self, Actor* pretarget, const Point &dest) const;
	/** Removes first Effect matching fx from the queue.
	 * Effects are matched based on their contents */
	bool RemoveEffect(Effect* fx);

	int AddAllEffects(Actor* target, const Point &dest) const;
	void ApplyAllEffects(Actor* target) const;
	/** remove effects marked for removal */
	void Cleanup();

	/* directly removes effects with specified opcode, use effect_reference when you can */
	void RemoveAllEffects(ieDword opcode) const;

	/* directly removes effects with specified opcode and resource (used by IWD) */
	void RemoveAllEffectsWithResource(ieDword opcode, const ieResRef resource) const;

	/* removes any effects (delayed or not) which were using projectile */
	void RemoveAllEffectsWithProjectile(ieDword projectile) const;

	/* removes equipping effects with specified inventory slot code */
	void RemoveEquippingEffects(ieDwordSigned slotcode) const;

	/* removes all effects of a given spell */
	void RemoveAllEffects(const ieResRef Removed) const;
	void RemoveAllEffects(const ieResRef Removed, ieByte timing) const;
	/* removes all effects of type */
	void RemoveAllEffects(EffectRef &effect_reference) const;
	/* removes expired or to be expired effects */
	void RemoveExpiredEffects(ieDword futuretime) const;
	/* removes all effects except timing mode 9 */
	void RemoveAllNonPermanentEffects() const;
	void RemoveAllDetrimentalEffects(EffectRef &effect_reference, ieDword current) const;
	void RemoveAllEffectsWithParam(EffectRef &effect_reference, ieDword param2) const;
	void RemoveAllEffectsWithResource(EffectRef &effect_reference, const ieResRef resource) const;
	void RemoveAllEffectsWithParamAndResource(EffectRef &effect_reference, ieDword param2, const ieResRef resource) const;
	void RemoveLevelEffects(ieResRef &Removed, ieDword level, ieDword flags, ieDword match) const;

	/* returns true if the timing method supports simplified duration */
	static bool HasDuration(Effect *fx);
	/* returns true if the effect should be saved */
	static bool Persistent(Effect* fx);
	/* returns next saved effect, increases index */
	std::list< Effect* >::const_iterator GetFirstEffect() const
	{
		return effects.begin();
	}
	const Effect *GetNextSavedEffect(std::list< Effect* >::const_iterator &f) const;
	Effect *GetNextEffect(std::list< Effect* >::const_iterator &f) const;
	ieDword CountEffects(EffectRef &effect_reference, ieDword param1, ieDword param2, const char *ResRef) const;
	void ModifyEffectPoint(EffectRef &effect_reference, ieDword x, ieDword y) const;
	/* returns the number of saved effects */
	ieDword GetSavedEffectsCount() const;
	size_t GetEffectsCount() const { return effects.size(); }
	/* this method hacks the offhand weapon color effects */
	static void HackColorEffects(Actor *Owner, Effect *fx);
	static Effect *CreateEffect(EffectRef &effect_reference, ieDword param1, ieDword param2, ieWord timing);
	EffectQueue *CopySelf() const;
	static Effect *CreateEffectCopy(Effect *oldfx, EffectRef &effect_reference, ieDword param1, ieDword param2);
	static Effect *CreateUnsummonEffect(Effect *fx);
	//locating opcodes
	Effect *HasEffect(EffectRef &effect_reference) const;
	Effect *HasEffectWithParam(EffectRef &effect_reference, ieDword param2) const;
	Effect *HasEffectWithParamPair(EffectRef &effect_reference, ieDword param1, ieDword param2) const;
	Effect *HasEffectWithResource(EffectRef &effect_reference, const ieResRef resource) const;
	Effect *HasEffectWithPower(EffectRef &effect_reference, ieDword power) const;
	Effect *HasSource(const ieResRef source) const;
	Effect *HasEffectWithSource(EffectRef &effect_reference, const ieResRef source) const;
	void DecreaseParam1OfEffect(EffectRef &effect_reference, ieDword amount) const;
	int DecreaseParam3OfEffect(EffectRef &effect_reference, ieDword amount, ieDword param2) const;
	//int SpecificDamageBonus(ieDword damage_type) const;
	int BonusForParam2(EffectRef &effect_reference, ieDword param2) const;
	bool HasAnyDispellableEffect() const;
	//getting summarised effects
	int BonusAgainstCreature(EffectRef &effect_reference, Actor *actor) const;
	//getting weapon immunity flag
	bool WeaponImmunity(int enchantment, ieDword weapontype) const;
	int SumDamageReduction(EffectRef &effect_reference, ieDword weaponEnchantment, int &total) const;
	//melee and ranged effects
	void AddWeaponEffects(EffectQueue *fxqueue, EffectRef &fx_ref) const;
	// checks if spells of type "types" are disabled (usually by armor)
	// returns a bitfield of disabled spelltypes
	// it is no longer used
	//int DisabledSpellcasting(int types) const;

	// returns -1 if bounced, 0 if resisted, 1 if accepted spell
	int CheckImmunity(Actor *target) const;
	// apply this effectqueue on all actors matching ids targeting
	// from pos, in range (no cone size yet)
	void AffectAllInRange(Map *map, const Point &pos, int idstype, int idsvalue, unsigned int range, Actor *except);
	/** Lists contents of the queue on a terminal for debugging */
	void dump() const;
	void dump(StringBuffer&) const;
	//resolve effect
	static int ResolveEffect(EffectRef &effect_reference);
	static bool match_ids(Actor *target, int table, ieDword value);
	/** returns true if the process should abort applying a stack of effects */
	int ApplyEffect(Actor* target, Effect* fx, ieDword first_apply, ieDword resistance=1) const;
	/** just checks if it is a particularly stupid effect that needs its target reset */
	static bool OverrideTarget(Effect *fx);
private:
	/** counts effects of specific opcode, parameters and resource */
	ieDword CountEffects(ieDword opcode, ieDword param1, ieDword param2, const char *ResRef) const;
	void ModifyEffectPoint(ieDword opcode, ieDword x, ieDword y) const;
	//use the effect reference style calls from outside
	static Effect *CreateEffect(ieDword opcode, ieDword param1, ieDword param2, ieWord timing);
	static Effect *CreateEffectCopy(Effect *oldfx, ieDword opcode, ieDword param1, ieDword param2);
	void RemoveAllDetrimentalEffects(ieDword opcode, ieDword current) const;
	void RemoveAllEffectsWithParam(ieDword opcode, ieDword param2) const;
	void RemoveAllEffectsWithParamAndResource(ieDword opcode, ieDword param2, const ieResRef resource) const;
	Effect *HasOpcode(ieDword opcode) const;
	Effect *HasOpcodeWithParam(ieDword opcode, ieDword param2) const;
	Effect *HasOpcodeWithParamPair(ieDword opcode, ieDword param1, ieDword param2) const;
	Effect *HasOpcodeWithResource(ieDword opcode, const ieResRef resource) const;
	Effect *HasOpcodeWithPower(ieDword opcode, ieDword power) const;
	Effect *HasOpcodeWithSource(ieDword opcode, const ieResRef source) const;
	void DecreaseParam1OfEffect(ieDword opcode, ieDword amount) const;
	int DecreaseParam3OfEffect(ieDword opcode, ieDword amount, ieDword param2) const;
	int BonusForParam2(ieDword opcode, ieDword param2) const;
	int BonusAgainstCreature(ieDword opcode, Actor *actor) const;
	bool WeaponImmunity(ieDword opcode, int enchantment, ieDword weapontype) const;
};

}

#endif // ! EFFECTQUEUE_H
