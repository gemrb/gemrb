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

#include "Logging/Logging.h"

#include <cstdlib>
#include <list>

namespace GemRB {

class Actor;
class Map;
class Scriptable;

/** if the effect returns this, stop adding any other effect */
#define FX_ABORT 0
/** these effects don't stick around if used as permanent,
 * in that case they modify a base stat like charisma modifier */
#define FX_PERMANENT 2
/** these effects never stick around, use them for instant effects like damage */
#define FX_NOT_APPLIED 3
/** these effects always stick around when applied as permanent or duration */
#define FX_APPLIED 1	
/** insert the effect instead of push back */
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

//sometimes damage doesn't comply with the calculated value
#define DICE_ROLL(adjustment) (core->Roll( fx->DiceThrown, fx->DiceSides, adjustment) )

// You will need to get GameTime somehow to use this macro
// we add +1 so we can handle effects with 0 duration (apply once only)
#define	PrepareDuration(fx) fx->Duration = ((fx->Duration? fx->Duration*AI_UPDATE_TIME : 1) + GameTime)

//return the caster object
#define GetCasterObject()  (core->GetGame()->GetActorByGlobalID(fx->CasterID))

// often used stat modifications, usually Parameter2 types 0, 1 and 2
//these macros should work differently in permanent mode (modify base too)
#define STAT_GET(stat) (target->Modified[ stat ])
#define STAT_ADD(stat, mod) target->SetStat( stat, STAT_GET( stat ) + ( mod ), 0 )
#define STAT_SUB(stat, mod) target->SetStat( stat, STAT_GET( stat ) - ( mod ), 0 )
#define STAT_BIT_OR(stat, mod) target->SetStat( stat, STAT_GET( stat ) | ( mod ), 0 )
#define STAT_SET(stat, mod) target->SetStat( stat, ( mod ), 0 )
#define STAT_SET_PCF(stat, mod) target->SetStat( stat, ( mod ), 1 )
#define STAT_BIT_OR_PCF(stat, mod) target->NewStat(stat, (mod), MOD_BITOR)
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
using EffectFunction = int (*)(Scriptable*, Actor*, Effect*);

/** Links Effect name to a function implementing the effect */
class EffectDesc {
	EffectFunction Function = nullptr;

public:
	const char* Name = nullptr; // FIXME: shouldn't we presume ownership of the name? original implementation didn't
	int Flags = 0;
	int opcode = -1;
	ieStrRef Strref = ieStrRef::INVALID;
	
	EffectDesc() = default;
	
	EffectDesc(const char* name, EffectFunction fn, int flags, int data) :
		Function(fn), Name(name), Flags(flags), opcode(data) {};

	explicit operator bool() const {
		return Function != nullptr;
	}
	
	int operator()(Scriptable* s, Actor* a, Effect* fx) const {
		return Function(s, a, fx);
	}

};
	
enum EffectFlags {
	EFFECT_NORMAL = 0,
	EFFECT_DICED = 1,
	EFFECT_NO_LEVEL_CHECK = 2,
	EFFECT_NO_ACTOR = 4,
	EFFECT_REINIT_ON_LOAD = 8,
	EFFECT_PRESET_TARGET = 16,
	EFFECT_SPECIAL_UNDO = 32
};

// unusual SpellProt types which need hacking (fake stats)
enum STITypes {
	STI_SOURCE_TARGET = 0x100,
	STI_SOURCE_NOT_TARGET,
	STI_CIRCLESIZE,
	STI_TWO_ROWS,
	STI_NOT_TWO_ROWS,
	STI_MORAL_ALIGNMENT,
	STI_AREATYPE,
	STI_DAYTIME,
	STI_EA_RELATION,
	STI_EVASION,
	STI_EA,
	STI_GENERAL,
	STI_RACE,
	STI_CLASS,
	STI_SPECIFIC,
	STI_GENDER,
	STI_WATERY, // ALIGNMENT in EE
	STI_STATE,
	STI_SPELLSTATE,
	STI_ALLIES, // just STI_EA_RELATION, but enables negation
	STI_ENEMIES, // just exact STI_EA_RELATION, since it would also return true for neutrals
	STI_SUMMONED_NUM,
	STI_CHAPTER_CHECK,

	STI_INVALID = 0xffff
};

/** Registers opcodes implemented by an effect plugin */
void EffectQueue_RegisterOpcodes(int count, const EffectDesc *opcodes);

/** Check if opcode is for an effect that takes a color slot as parameter. */
bool IsColorslotEffect(int opcode);

/**
 * @class EffectQueue
 * Class holding and processing spell Effects on a single Actor
 */

class GEM_EXPORT EffectQueue {
private:
	/** List of Effects applied on the Actor */
	using queue_t = std::list<Effect>;
	queue_t effects;
	/** Actor which is target of the Effects */
	Scriptable* Owner = nullptr;

public:
	EffectQueue() noexcept {};
	
	explicit operator bool() const {
		return !effects.empty();
	}

	/** Sets Actor which is affected by these effects */
	void SetOwner(Scriptable* act) { Owner = act; }
	/** Returns Actor affected by these effects */
	Scriptable* GetOwner() const { return Owner; }

	/** adds an effect to the queue, */
	void AddEffect(Effect* fx, bool insert=false);
	/** Adds an Effect to the queue, subject to level and other checks.
	 * Returns FX_ABORT if unsuccessful. */
	int AddEffect(Effect* fx, Scriptable* self, Actor* pretarget, const Point &dest) const;
	/** Removes first Effect matching fx from the queue.
	 * Effects are matched based on their contents */
	bool RemoveEffect(const Effect* fx);

	int AddAllEffects(Actor* target, const Point &dest);
	void ApplyAllEffects(Actor* target);
	/** remove effects marked for removal */
	void Cleanup();

	/* directly removes effects with specified opcode, use effect_reference when you can */
	void RemoveAllEffects(ieDword opcode);

	/* directly removes effects with specified opcode and resource (used by IWD) */
	void RemoveAllEffectsWithResource(ieDword opcode, const ResRef &resource);

	/* removes any effects (delayed or not) which were using projectile */
	void RemoveAllEffectsWithProjectile(ieDword projectile);

	// removes any effects with matching source and chosen mode
	void RemoveAllEffectsWithSource(ieDword opcode, const ResRef &source, int mode);

	/* removes equipping effects with specified inventory slot code */
	bool RemoveEquippingEffects(ieDwordSigned slotcode);

	/* removes all effects of a given spell */
	void RemoveAllEffects(const ResRef &Removed);
	void RemoveAllEffects(const ResRef &Removed, ieByte timing);
	/* removes all effects of type */
	void RemoveAllEffects(EffectRef &effect_reference);
	/* removes expired or to be expired effects */
	void RemoveExpiredEffects(ieDword futuretime);
	/* removes all effects except timing mode 9 */
	void RemoveAllNonPermanentEffects();
	void RemoveAllDetrimentalEffects(EffectRef &effect_reference, ieDword current);
	void RemoveAllEffectsWithParam(EffectRef &effect_reference, ieDword param2);
	void RemoveAllEffectsWithParam(ieDword opcode, ieDword param2);
	void RemoveAllEffectsWithResource(EffectRef &effect_reference, const ResRef &resource);
	void RemoveAllEffectsWithParamAndResource(EffectRef &effect_reference, ieDword param2, const ResRef &resource);
	void RemoveAllEffectsWithSource(EffectRef &effectReference, const ResRef &source, int mode);
	void RemoveLevelEffects(ieDword level, ieDword flags, ieDword match, const Scriptable* target);
	static bool RollDispelChance(ieDword casterLevel, ieDword level);
	void DispelEffects(const Effect *dispeller, ieDword level);

	/* returns next saved effect, increases index */
	queue_t::iterator GetFirstEffect()
	{
		return effects.begin();
	}
	
	queue_t::const_iterator GetFirstEffect() const
	{
		return effects.begin();
	}
	
	const Effect *GetNextSavedEffect(queue_t::const_iterator &f) const;
	const Effect *GetNextEffect(queue_t::const_iterator &f) const;
	Effect *GetNextEffect(queue_t::iterator &f);
	ieDword CountEffects(EffectRef &effect_reference, ieDword param1, ieDword param2, const ResRef& = ResRef()) const;
	void ModifyEffectPoint(EffectRef &effect_reference, ieDword x, ieDword y);
	void ModifyAllEffectSources(const Point &source);
	/* returns the number of saved effects */
	ieDword GetSavedEffectsCount() const;
	size_t GetEffectsCount() const { return effects.size(); }
	unsigned int GetEffectOrder(EffectRef &effect_reference, const Effect *fx2) const;
	/* this method hacks the offhand weapon color effects */
	static void HackColorEffects(const Actor *Owner, Effect *fx);
	static Effect *CreateEffect(EffectRef &effect_reference, ieDword param1, ieDword param2, ieWord timing);
	static Effect *CreateEffectCopy(const Effect *oldfx, EffectRef &effect_reference, ieDword param1, ieDword param2);
	static Effect *CreateUnsummonEffect(const Effect *fx);
	//locating opcodes
	Effect *HasEffect(EffectRef &effect_reference);
	const Effect *HasEffect(EffectRef &effect_reference) const;
	const Effect *HasEffectWithParam(EffectRef &effect_reference, ieDword param2) const;
	const Effect *HasEffectWithParamPair(EffectRef &effect_reference, ieDword param1, ieDword param2) const;
	const Effect *HasEffectWithResource(EffectRef &effect_reference, const ResRef &resource) const;
	const Effect *HasEffectWithPower(EffectRef &effect_reference, ieDword power) const;
	const Effect *HasSource(const ResRef &source) const;
	const Effect *HasEffectWithSource(EffectRef &effect_reference, const ResRef &source) const;
	bool DecreaseParam1OfEffect(EffectRef &effect_reference, ieDword amount);
	int DecreaseParam3OfEffect(EffectRef &effect_reference, ieDword amount, ieDword param2);
	int BonusForParam2(EffectRef &effect_reference, ieDword param2) const;
	int MaxParam1(EffectRef &effect_reference, bool positive) const;
	bool HasAnyDispellableEffect() const;
	//getting summarised effects
	int BonusAgainstCreature(EffectRef &effect_reference, const Actor *actor) const;
	//getting weapon immunity flag
	bool WeaponImmunity(int enchantment, ieDword weapontype) const;
	int SumDamageReduction(EffectRef &effect_reference, ieDword weaponEnchantment, int &total) const;
	//melee and ranged effects
	void AddWeaponEffects(EffectQueue* fxqueue, EffectRef& fx_ref, ieDword param2 = 1) const;

	// returns -1 if bounced, 0 if resisted, 1 if accepted spell
	int CheckImmunity(Actor *target) const;
	// apply this effectqueue on all actors matching ids targeting
	// from pos, in range (no cone size yet)
	void AffectAllInRange(const Map *map, const Point &pos, int idstype, int idsvalue, unsigned int range, const Actor *except);
	/** Lists contents of the queue on a terminal for debugging */
	std::string dump(bool print = true) const;
	//resolve effect
	static int ResolveEffect(EffectRef &effect_reference);
	static bool match_ids(const Actor *target, int table, ieDword value);
	/** returns true if the process should abort applying a stack of effects */
	int ApplyEffect(Actor* target, Effect* fx, ieDword first_apply, ieDword resistance=1) const;
	/** just checks if it is a particularly stupid effect that needs its target reset */
	static bool OverrideTarget(const Effect *fx);
	bool HasHostileEffects() const;
	static bool CheckIWDTargeting(Scriptable* Owner, Actor* target, ieDword value, ieDword type, Effect *fx = nullptr);
private:
	/** counts effects of specific opcode, parameters and resource */
	ieDword CountEffects(ieDword opcode, ieDword param1, ieDword param2, const ResRef& = ResRef()) const;
	void ModifyEffectPoint(ieDword opcode, ieDword x, ieDword y);
	//use the effect reference style calls from outside
	static Effect *CreateEffect(ieDword opcode, ieDword param1, ieDword param2, ieWord timing);
	static Effect *CreateEffectCopy(const Effect *oldfx, ieDword opcode, ieDword param1, ieDword param2);
	void RemoveAllDetrimentalEffects(ieDword opcode, ieDword current);
	void RemoveAllEffectsWithParamAndResource(ieDword opcode, ieDword param2, const ResRef &resource);
	Effect *HasOpcode(ieDword opcode);
	const Effect *HasOpcode(ieDword opcode) const;
	const Effect *HasOpcodeWithParam(ieDword opcode, ieDword param2) const;
	const Effect *HasOpcodeWithParamPair(ieDword opcode, ieDword param1, ieDword param2) const;
	const Effect *HasOpcodeWithResource(ieDword opcode, const ResRef &resource) const;
	const Effect *HasOpcodeWithPower(ieDword opcode, ieDword power) const;
	const Effect *HasOpcodeWithSource(ieDword opcode, const ResRef &source) const;
	bool DecreaseParam1OfEffect(ieDword opcode, ieDword amount);
	int DecreaseParam3OfEffect(ieDword opcode, ieDword amount, ieDword param2);
	int BonusForParam2(ieDword opcode, ieDword param2) const;
	int MaxParam1(ieDword opcode, bool positive) const;
	int BonusAgainstCreature(ieDword opcode, const Actor *actor) const;
	bool WeaponImmunity(ieDword opcode, int enchantment, ieDword weapontype) const;
};

}

#endif // ! EFFECTQUEUE_H
