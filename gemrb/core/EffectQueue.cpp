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

#include "EffectQueue.h"

#include "ie_feats.h"
#include "opcode_params.h"
#include "overlays.h"
#include "strrefs.h"

#include "DisplayMessage.h"
#include "Effect.h"
#include "Game.h"
#include "GameData.h"
#include "Interface.h"
#include "Map.h"
#include "Spell.h" //needs for the source flags bitfield
#include "SymbolMgr.h"
#include "TableMgr.h"

#include "GameScript/GSUtils.h" // for DiffCore
#include "GameScript/GameScript.h" // only for ID_Allegiance
#include "Scriptable/Actor.h"

#include <cstdio>

namespace GemRB {

static std::vector<EffectDesc> effectnames;

void EffectQueue_RegisterOpcodes(int count, const EffectDesc* opcodes)
{
	size_t oldc = effectnames.size();
	effectnames.resize(effectnames.size() + count);

	std::copy(opcodes, opcodes + count, &effectnames[0] + oldc);

	//if we merge two effect lists, then we need to sort their effect tables
	//actually, we might always want to sort this list, so there is no
	//need to do it manually (sorted table is needed if we use bsearch)
	qsort(&effectnames[0], effectnames.size(), sizeof(EffectDesc), [](const void* a, const void* b) {
		return stricmp(((const EffectDesc*) a)->Name, ((const EffectDesc*) b)->Name);
	});
}

static EffectDesc* FindEffect(StringView effectname)
{
	if (effectname.empty() || effectnames.empty()) {
		return nullptr;
	}

	void* tmp = bsearch(effectname.c_str(), effectnames.data(), effectnames.size(), sizeof(EffectDesc), [](const void* a, const void* b) {
		return stricmp((const char*) a, ((const EffectDesc*) b)->Name);
	});

	if (!tmp) {
		Log(WARNING, "EffectQueue", "Couldn't assign effect: {}", effectname);
	}
	return (EffectDesc*) tmp;
}

/** Initializes table of available spell Effects used by all the queues. */
/** The available effects should already be registered by the effect plugins */

struct Globals {
	static constexpr int MAX_EFFECTS = 512;
	EffectDesc Opcodes[MAX_EFFECTS];

	int pstflags = false;
	bool iwd2fx = false;

	static const Globals& Get()
	{
		static Globals globs;
		return globs;
	}

	static void ResolveEffectRef(EffectRef& effectReference)
	{
		Get().ResolveEffectRefImp(effectReference);
	}

private:
	Globals()
	{
		pstflags = core->HasFeature(GFFlags::PST_STATE_FLAGS);
		iwd2fx = core->HasFeature(GFFlags::ENHANCED_EFFECTS);

		AutoTable efftextTable = gamedata->LoadTable("efftext");

		int eT = core->LoadSymbol("effects");
		if (eT < 0) {
			error("EffectQueue", "A critical scripting file is missing!");
		}
		auto effectsTable = core->GetSymbol(eT);
		if (!effectsTable) {
			error("EffectQueue", "A critical scripting file is damaged!");
		}

		for (int i = 0; i < MAX_EFFECTS; i++) {
			const auto& effectname = effectsTable->GetValue(i);
			if (effectname.empty()) continue; // past the table size or undefined effect

			EffectDesc* poi = FindEffect(effectname);
			assert(poi != nullptr);
			Opcodes[i] = *poi;

			// reverse linking opcode number
			// using this unused field
			if (poi->opcode != -1 && effectname[0] != '*') {
				error("EffectQueue", "Clashing Opcodes FN: {} vs. {}, {}", i, poi->opcode, effectname);
			}
			poi->opcode = i;

			if (!efftextTable) continue;
			TableMgr::index_t row = efftextTable->GetRowCount();
			while (row--) {
				const char* ret = efftextTable->GetRowName(row).c_str();
				int val;
				if (valid_signednumber(ret, val) && (i == val)) {
					Opcodes[i].Strref = efftextTable->QueryFieldAsStrRef(row, 1);
					break;
				} else {
					Opcodes[i].Strref = ieStrRef::INVALID;
				}
			}
		}
		core->DelSymbol(eT);
	}

	// nonstatic, this actually depends on Globals() indirectly
	void ResolveEffectRefImp(EffectRef& effectReference) const
	{
		if (effectReference.opcode == -1) {
			const EffectDesc* ref = FindEffect(StringView(effectReference.Name));
			if (ref && ref->opcode >= 0) {
				effectReference.opcode = ref->opcode;
				return;
			}
			effectReference.opcode = -2;
		}
	}
};

static EffectRef fx_unsummon_creature_ref = { "UnsummonCreature", -1 };
static EffectRef fx_ac_vs_creature_type_ref = { "ACVsCreatureType", -1 };
static EffectRef fx_spell_focus_ref = { "SpellFocus", -1 };
static EffectRef fx_spell_resistance_ref = { "SpellResistance", -1 };
static EffectRef fx_protection_from_display_string_ref = { "Protection:String", -1 };
static EffectRef fx_activate_spell_sequencer_ref = { "Sequencer:Activate", -1 };

// immunity effects (setters of IE_IMMUNITY)
static EffectRef fx_level_immunity_ref = { "Protection:SpellLevel", -1 };
static EffectRef fx_opcode_immunity_ref = { "Protection:Opcode", -1 }; //bg2
static EffectRef fx_opcode_immunity2_ref = { "Protection:Opcode2", -1 }; //iwd
static EffectRef fx_spell_immunity_ref = { "Protection:Spell", -1 }; //bg2
static EffectRef fx_spell_immunity2_ref = { "Protection:Spell2", -1 }; //iwd
static EffectRef fx_school_immunity_ref = { "Protection:School", -1 };
static EffectRef fx_secondary_type_immunity_ref = { "Protection:SecondaryType", -1 };
static EffectRef fx_projectile_immunity_ref = { "Protection:Projectile", -1 };

//decrementing immunity effects
static EffectRef fx_level_immunity_dec_ref = { "Protection:SpellLevelDec", -1 };
static EffectRef fx_spell_immunity_dec_ref = { "Protection:SpellDec", -1 };
static EffectRef fx_school_immunity_dec_ref = { "Protection:SchoolDec", -1 };
static EffectRef fx_secondary_type_immunity_dec_ref = { "Protection:SecondaryTypeDec", -1 };

//bounce effects
static EffectRef fx_projectile_bounce_ref = { "Bounce:Projectile", -1 };
static EffectRef fx_level_bounce_ref = { "Bounce:SpellLevel", -1 };
//static EffectRef fx_opcode_bounce_ref = { "Bounce:Opcode", -1 };
static EffectRef fx_spell_bounce_ref = { "Bounce:Spell", -1 };
static EffectRef fx_school_bounce_ref = { "Bounce:School", -1 };
static EffectRef fx_secondary_type_bounce_ref = { "Bounce:SecondaryType", -1 };

//decrementing bounce effects
static EffectRef fx_level_bounce_dec_ref = { "Bounce:SpellLevelDec", -1 };
static EffectRef fx_spell_bounce_dec_ref = { "Bounce:SpellDec", -1 };
static EffectRef fx_school_bounce_dec_ref = { "Bounce:SchoolDec", -1 };
static EffectRef fx_secondary_type_bounce_dec_ref = { "Bounce:SecondaryTypeDec", -1 };

//spelltrap (multiple decrementing immunity)
static EffectRef fx_spelltrap = { "SpellTrap", -1 };

//weapon immunity
static EffectRef fx_weapon_immunity_ref = { "Protection:Weapons", -1 };

bool EffectQueue::match_ids(const Actor* target, int table, ieDword value)
{
	if (value == 0) {
		return true;
	}

	int a, stat;

	switch (table) {
		case 0:
			stat = IE_FACTION;
			break;
		case 1:
			stat = IE_TEAM;
			break;
		case 2: //EA
			stat = IE_EA;
			return GameScript::ID_Allegiance(target, value);
		case 3: //GENERAL
			//this is a hack to support dead only projectiles in PST
			//if it interferes with something feel free to remove
			if (value == GEN_DEAD && target->GetStat(IE_STATE_ID) & STATE_DEAD) {
				return true;
			}
			stat = IE_GENERAL;
			break;
		case 4: //RACE
			stat = IE_RACE;
			break;
		case 5: //CLASS
			stat = IE_CLASS;
			break;
		case 6: //SPECIFIC
			stat = IE_SPECIFIC;
			break;
		case 7: //GENDER
			stat = IE_SEX;
			break;
		case 8: //ALIGNMENT
			stat = target->GetStat(IE_ALIGNMENT);
			a = value & 15;
			if (a && a != (stat & 15)) {
				return false;
			}
			a = value & 0xf0;
			if (a && a != (stat & 0xf0)) {
				return false;
			}
			return true;
		case 9:
			stat = target->GetClassMask();
			if (value & stat) return true;
			return false;
		default:
			return false;
	}

	if (stat == IE_CLASS) {
		return target->GetActiveClass() == value;
	}
	return target->GetStat(stat) == value;
}

/*
static const bool fx_instant[MAX_TIMING_MODE]={true,true,true,false,false,false,false,false,true,true,true,true};

static inline bool IsInstant(ieByte timingmode)
{
	if( timingmode>=MAX_TIMING_MODE) return false;
	return fx_instant[timingmode];
}*/

static const bool fx_equipped[MAX_TIMING_MODE] = { false, false, true, false, false, true, false, false, true, false, false, false };

static inline bool IsEquipped(ieWord timingmode)
{
	if (timingmode >= MAX_TIMING_MODE) return false;
	return fx_equipped[timingmode];
}

static const bool fx_relative[MAX_TIMING_MODE] = { true, false, false, true, true, true, false, false, false, false, true, false };

static inline bool NeedPrepare(ieWord timingmode)
{
	if (timingmode >= MAX_TIMING_MODE) return false;
	return fx_relative[timingmode];
}

enum class TimingType {
	Invalid = -1,
	Permanent,
	Delayed,
	Duration
};

static const TimingType fx_prepared[MAX_TIMING_MODE] = { TimingType::Duration, TimingType::Permanent, TimingType::Permanent, // 0-2
							 TimingType::Delayed, TimingType::Delayed, TimingType::Delayed, TimingType::Delayed, TimingType::Delayed, // 3-7
							 TimingType::Permanent, TimingType::Permanent, TimingType::Duration, TimingType::Permanent }; // 8-11

static inline TimingType DelayType(ieWord timingmode)
{
	if (timingmode >= MAX_TIMING_MODE) return TimingType::Invalid;
	return fx_prepared[timingmode];
}

//which effects are removable
static const bool fx_removable[MAX_TIMING_MODE] = { true, true, false, true, true, false, true, true, false, false, true, true };

static inline bool IsRemovable(ieWord timingmode)
{
	if (timingmode >= MAX_TIMING_MODE) return true;
	return fx_removable[timingmode];
}

//change the timing method after the effect triggered
static const ieByte fx_triggered[MAX_TIMING_MODE] = { FX_DURATION_JUST_EXPIRED, FX_DURATION_INSTANT_PERMANENT, //0,1
						      FX_DURATION_INSTANT_WHILE_EQUIPPED, FX_DURATION_INSTANT_LIMITED, //2,3
						      FX_DURATION_INSTANT_PERMANENT, FX_DURATION_PERMANENT_UNSAVED, //4,5
						      FX_DURATION_INSTANT_LIMITED, FX_DURATION_JUST_EXPIRED, FX_DURATION_PERMANENT_UNSAVED, //6,8
						      FX_DURATION_INSTANT_PERMANENT_AFTER_BONUSES, FX_DURATION_JUST_EXPIRED, //9,10
						      FX_DURATION_JUST_EXPIRED }; //11

static inline ieByte TriggeredEffect(ieByte timingmode)
{
	if (timingmode >= MAX_TIMING_MODE) return false;
	return fx_triggered[timingmode];
}

Effect* EffectQueue::CreateEffect(ieDword opcode, ieDword param1, ieDword param2, ieWord timing)
{
	if (opcode == 0xffffffff) {
		return nullptr;
	}
	Effect* fx = new Effect();
	if (!fx) {
		return nullptr;
	}

	fx->Target = FX_TARGET_SELF;
	fx->Opcode = opcode;
	fx->ProbabilityRangeMax = 100;
	fx->Parameter1 = param1;
	fx->Parameter2 = param2;
	fx->TimingMode = timing;
	fx->Pos.Invalidate();
	return fx;
}

//return the count of effects with matching parameters
//useful for effects where there is no separate stat to see
ieDword EffectQueue::CountEffects(EffectRef& effectReference, ieDword param1, ieDword param2, const ResRef& resource, const ResRef& source) const
{
	if (effectReference.Name[0]) {
		Globals::ResolveEffectRef(effectReference);
		if (effectReference.opcode < 0) {
			return 0;
		}
	}
	return CountEffects(effectReference.opcode, param1, param2, resource, source);
}

//Change the location of an existing effect
//this is used when some external code needs to adjust the effect's location
//used when the gui sets the effect's final target
void EffectQueue::ModifyEffectPoint(EffectRef& effectReference, ieDword x, ieDword y)
{
	Globals::ResolveEffectRef(effectReference);
	if (effectReference.opcode < 0) {
		return;
	}
	ModifyEffectPoint(effectReference.opcode, x, y);
}

void EffectQueue::ModifyAllEffectSources(const Point& source)
{
	for (auto& fx : effects) {
		fx.Source = source;
	}
}

Effect* EffectQueue::CreateEffect(EffectRef& effectReference, ieDword param1, ieDword param2, ieWord timing)
{
	Globals::ResolveEffectRef(effectReference);
	if (effectReference.opcode < 0) {
		return nullptr;
	}
	return CreateEffect(effectReference.opcode, param1, param2, timing);
}

//create a new effect with most of the characteristics of the old effect
//only opcode and parameters are changed
//This is used mostly inside effects, when an effect needs to spawn
//other effects with the same coordinates, source, duration, etc.
Effect* EffectQueue::CreateEffectCopy(const Effect* oldfx, ieDword opcode, ieDword param1, ieDword param2)
{
	if (opcode == 0xffffffff) {
		return nullptr;
	}
	Effect* fx = new Effect(*oldfx);
	if (!fx) return nullptr;

	fx->Opcode = opcode;
	fx->Parameter1 = param1;
	fx->Parameter2 = param2;
	return fx;
}

Effect* EffectQueue::CreateEffectCopy(const Effect* oldfx, EffectRef& effectReference, ieDword param1, ieDword param2)
{
	Globals::ResolveEffectRef(effectReference);
	if (effectReference.opcode < 0) {
		return nullptr;
	}
	return CreateEffectCopy(oldfx, effectReference.opcode, param1, param2);
}

Effect* EffectQueue::CreateUnsummonEffect(const Effect* fx)
{
	Effect* newfx = nullptr;
	if ((fx->TimingMode & 0xff) == FX_DURATION_INSTANT_LIMITED) {
		newfx = CreateEffectCopy(fx, fx_unsummon_creature_ref, 0, 0);
		newfx->TimingMode = FX_DURATION_DELAY_PERMANENT;
		newfx->Target = FX_TARGET_PRESET;
		newfx->Resource = newfx->Resource3.IsEmpty() ? "SPGFLSH1" : newfx->Resource3;
		if (fx->TimingMode == FX_DURATION_ABSOLUTE) {
			//unprepare duration
			newfx->Duration = (newfx->Duration - core->GetGame()->GameTime) / core->Time.defaultTicksPerSec;
		}
	}

	return newfx;
}

void EffectQueue::AddEffect(Effect* fx, bool insert)
{
	if (insert) {
		effects.push_front(std::move(*fx));
	} else {
		effects.push_back(std::move(*fx));
	}
	delete fx;
}

//This method can remove an effect described by a pointer to it, or
//an exact matching effect
bool EffectQueue::RemoveEffect(const Effect* fx)
{
	for (auto f = effects.begin(); f != effects.end(); ++f) {
		if (*fx == *f) {
			effects.erase(f);
			return true;
		}
	}
	return false;
}

//this is where we reapply all effects when loading a saved game
//The effects are already in the fxqueue of the target
//... but some require reinitialisation
void EffectQueue::ApplyAllEffects(Actor* target)
{
	const auto& Opcodes = Globals::Get().Opcodes;

	for (auto& fx : effects) {
		if (Opcodes[fx.Opcode].Flags & EFFECT_REINIT_ON_LOAD) {
			// pretend to be the first application (FirstApply==1)
			ApplyEffect(target, &fx, 1);
		} else {
			ApplyEffect(target, &fx, 0);
		}
	}
}

void EffectQueue::Cleanup()
{
	for (auto f = effects.begin(); f != effects.end();) {
		if (f->TimingMode == FX_DURATION_JUST_EXPIRED) {
			f = effects.erase(f);
		} else {
			++f;
		}
	}
}

//Handle the target flag when the effect is applied first
int EffectQueue::AddEffect(Effect* fx, Scriptable* self, Actor* pretarget, const Point& dest) const
{
	int i;
	const Game* game;
	const Map* map;
	int flg;
	ieDword spec = 0;
	Actor* st = Scriptable::As<Actor>(self);
	// HACK: 00p2229.baf in ar1006 does this silly thing, crashing later
	if (!st && self && (self->Type == ST_CONTAINER) && (fx->Target == FX_TARGET_SELF)) {
		fx->Target = FX_TARGET_PRESET;
	}

	if (self) {
		fx->CasterID = self->GetGlobalID();
		fx->SetSourcePosition(self->Pos);
	} else if (Owner) {
		fx->CasterID = Owner->GetGlobalID();
		fx->SetSourcePosition(Owner->Pos);
	}
	if (!fx->CasterLevel) {
		// happens for effects that we apply directly from within, not through a spell/item
		// for example through GemRB_ApplyEffect
		const Actor* caster = GetCasterObject();
		if (caster) {
			// FIXME: guessing, will be fine most of the time
			fx->CasterLevel = caster->GetAnyActiveCasterLevel();
		}
	}

	switch (fx->Target) {
		case FX_TARGET_ORIGINAL:
			assert(self != nullptr);
			fx->SetPosition(self->Pos);

			flg = ApplyEffect(st, fx, 1);
			if (fx->TimingMode != FX_DURATION_JUST_EXPIRED && st) {
				st->fxqueue.AddEffect(fx, flg == FX_INSERT);
			} else {
				delete fx;
			}
			break;
		case FX_TARGET_SELF:
			fx->SetPosition(dest);

			flg = ApplyEffect(st, fx, 1);
			if (fx->TimingMode != FX_DURATION_JUST_EXPIRED && st) {
				st->fxqueue.AddEffect(fx, flg == FX_INSERT);
			} else {
				delete fx;
			}
			break;

		case FX_TARGET_ALL_BUT_SELF:
			assert(self != nullptr);
			map = self->GetCurrentArea();
			i = map->GetActorCount(true);
			while (i--) {
				Actor* actor = map->GetActor(i, true);
				//don't pick ourselves
				if (st == actor) {
					continue;
				}
				Effect* new_fx = new Effect(*fx);
				new_fx->SetPosition(actor->Pos);

				flg = ApplyEffect(actor, new_fx, 1);
				if (new_fx->TimingMode != FX_DURATION_JUST_EXPIRED) {
					actor->fxqueue.AddEffect(new_fx, flg == FX_INSERT);
				} else {
					delete new_fx;
				}
			}
			delete fx;
			flg = FX_APPLIED;
			break;

		case FX_TARGET_OWN_SIDE:
			if (!st || st->InParty) {
				goto all_party;
			}
			map = self->GetCurrentArea();
			spec = st->GetStat(IE_SPECIFIC);

			//GetActorCount(false) returns all nonparty critters
			i = map->GetActorCount(false);
			while (i--) {
				Actor* actor = map->GetActor(i, false);
				if (actor->GetStat(IE_SPECIFIC) != spec) {
					continue;
				}
				Effect* new_fx = new Effect(*fx);
				new_fx->SetPosition(actor->Pos);

				flg = ApplyEffect(actor, new_fx, 1);
				if (new_fx->TimingMode != FX_DURATION_JUST_EXPIRED) {
					actor->fxqueue.AddEffect(new_fx, flg == FX_INSERT);
				} else {
					delete new_fx;
				}
			}
			delete fx;
			flg = FX_APPLIED;
			break;
		case FX_TARGET_OTHER_SIDE:
			if (!pretarget || pretarget->InParty) {
				goto all_party;
			}
			assert(self != nullptr);
			map = self->GetCurrentArea();
			spec = pretarget->GetStat(IE_SPECIFIC);

			//GetActorCount(false) returns all nonparty critters
			i = map->GetActorCount(false);
			while (i--) {
				Actor* actor = map->GetActor(i, false);
				if (actor->GetStat(IE_SPECIFIC) != spec) {
					continue;
				}
				Effect* new_fx = new Effect(*fx);
				new_fx->SetPosition(actor->Pos);

				flg = ApplyEffect(actor, new_fx, 1);
				//GetActorCount can now return all nonparty critters
				if (new_fx->TimingMode != FX_DURATION_JUST_EXPIRED) {
					actor->fxqueue.AddEffect(new_fx, flg == FX_INSERT);
				} else {
					delete new_fx;
				}
			}
			delete fx;
			flg = FX_APPLIED;
			break;
		case FX_TARGET_PRESET:
			//fx->SetPosition(pretarget->Pos);
			//knock needs this
			fx->SetPosition(dest);

			flg = ApplyEffect(pretarget, fx, 1);
			if (fx->TimingMode != FX_DURATION_JUST_EXPIRED && pretarget) {
				pretarget->fxqueue.AddEffect(fx, flg == FX_INSERT);
			} else {
				delete fx;
			}
			break;

		case FX_TARGET_PARTY:
all_party:
			game = core->GetGame();
			i = game->GetPartySize(false);
			while (i--) {
				Actor* actor = game->GetPC(i, false);
				Effect* new_fx = new Effect(*fx);
				new_fx->SetPosition(actor->Pos);

				flg = ApplyEffect(actor, new_fx, 1);
				if (new_fx->TimingMode != FX_DURATION_JUST_EXPIRED) {
					actor->fxqueue.AddEffect(new_fx, flg == FX_INSERT);
				} else {
					delete new_fx;
				}
			}
			delete fx;
			flg = FX_APPLIED;
			break;

		case FX_TARGET_ALL:
			assert(self != nullptr);
			map = self->GetCurrentArea();
			i = map->GetActorCount(true);
			while (i--) {
				Actor* actor = map->GetActor(i, true);
				Effect* new_fx = new Effect(*fx);
				new_fx->SetPosition(actor->Pos);

				flg = ApplyEffect(actor, new_fx, 1);
				if (new_fx->TimingMode != FX_DURATION_JUST_EXPIRED) {
					new_fx->Target = FX_TARGET_SELF;
					actor->fxqueue.AddEffect(new_fx, flg == FX_INSERT);
				} else {
					delete new_fx;
				}
			}
			delete fx;
			flg = FX_APPLIED;
			break;

		case FX_TARGET_ALL_BUT_PARTY:
			assert(self != nullptr);
			map = self->GetCurrentArea();
			i = map->GetActorCount(false);
			while (i--) {
				Actor* actor = map->GetActor(i, false);
				if (actor->GetBase(IE_EA) == EA_FAMILIAR) continue;
				Effect* new_fx = new Effect(*fx);
				new_fx->SetPosition(actor->Pos);

				flg = ApplyEffect(actor, new_fx, 1);
				//GetActorCount can now return all nonparty critters
				if (new_fx->TimingMode != FX_DURATION_JUST_EXPIRED) {
					actor->fxqueue.AddEffect(new_fx, flg == FX_INSERT);
				} else {
					delete new_fx;
				}
			}
			delete fx;
			flg = FX_APPLIED;
			break;

		case FX_TARGET_UNKNOWN:
		default:
			Log(MESSAGE, "EffectQueue", "Unknown FX target type: {}", fx->Target);
			flg = FX_ABORT;
			delete fx;
			break;
	}

	return flg;
}

//this is where effects from spells first get in touch with the target
//the effects are currently NOT in the target's fxqueue, those that stick
//will get copied (hence the fxqueue.AddEffect call)
//if this returns FX_NOT_APPLIED, then the whole stack was resisted
//or expired
int EffectQueue::AddAllEffects(Actor* target, const Point& destination)
{
	int res = FX_NOT_APPLIED;
	// pre-roll dice for fx needing them and stow them in the effect
	ieDword randomValue = core->Roll(1, 100, -1);

	if (target) {
		target->RollSaves();
	}
	for (auto& fx : effects) {
		//handle resistances and saving throws here
		fx.RandomValue = randomValue;
		//if applyeffect returns true, we stop adding the future effects
		//this is to simulate iwd2's on the fly spell resistance

		int tmp = AddEffect(new Effect(fx), Owner, target, destination);
		//lets try without Owner, any crash?
		//If yes, then try to fix the individual effect
		//If you use target for Owner here, the wand in chateau irenicus will work
		//the same way as Imoen's monster summoning, which is a BAD THING (TM)
		//int tmp = AddEffect(*f, Owner?Owner:target, target, destination);
		if (tmp == FX_ABORT) {
			res = FX_NOT_APPLIED;
			break;
		}
		if (tmp != FX_NOT_APPLIED) {
			res = FX_APPLIED;
		}
	}
	return res;
}

//resisted effect based on level
static inline bool CheckLevel(const Actor* target, Effect* fx)
{
	const auto& Opcodes = Globals::Get().Opcodes;
	//skip non level based effects
	//check if an effect has no level based resistance, but instead the dice sizes/count
	//adjusts Parameter1 (like a damage causing effect)
	if (Opcodes[fx->Opcode].Flags & EFFECT_DICED) {
		//add the caster level to the dice count
		if (fx->IsVariable) {
			fx->DiceThrown += fx->CasterLevel;
		}
		fx->Parameter1 = DICE_ROLL((signed) fx->Parameter1);
		//this is a hack for PST style diced effects
		if (core->HasFeature(GFFlags::SAVE_FOR_HALF)) {
			if (!fx->Resource.IsEmpty() && !fx->Resource.BeginsWith("NEG")) {
				fx->IsSaveForHalfDamage = 1;
			}
		} else if ((fx->Parameter2 & 3) == 3) {
			fx->IsSaveForHalfDamage = 1;
		}
		return false;
	}
	//there is no level based resistance, but Parameter1 cannot be precalculated
	//these effects use the Dice fields in a special way
	// also makes no sense to attempt resistance checks on actorless effects (eg. fx_tint_screen hit this due to bad targeting set)
	if (Opcodes[fx->Opcode].Flags & (EFFECT_NO_LEVEL_CHECK | EFFECT_NO_ACTOR)) {
		return false;
	}

	if (!target || fx->Target == FX_TARGET_SELF) {
		return false;
	}

	ieDword level = target->GetXPLevel(true);
	//return true if resisted
	if ((signed) fx->MinAffectedLevel > 0 && (signed) level < (signed) fx->MinAffectedLevel) {
		return true;
	}

	if ((signed) fx->MaxAffectedLevel > 0 && (signed) level > (signed) fx->MaxAffectedLevel) {
		return true;
	}
	return false;
}

// roll for the effect probability, there is a high and a low threshold, the d100
//roll should hit in the middle
static inline bool CheckProbability(const Effect* fx)
{
	//random value is 0-99
	if (fx->RandomValue < fx->ProbabilityRangeMin || fx->RandomValue > fx->ProbabilityRangeMax) {
		return false;
	}
	return true;
}

static int CheckOpcodeImmunity(const Effect* fx, const Actor* target)
{
	const auto& globals = Globals::Get();
	// opcode immunity
	// TODO: research, maybe the whole check_resistance should be skipped on caster != actor (selfapplication)
	if (target->fxqueue.HasEffectWithParam(fx_opcode_immunity_ref, fx->Opcode)) {
		Log(MESSAGE, "EffectQueue", "{} is immune to effect: {}", fmt::WideToChar { target->GetName() }, globals.Opcodes[fx->Opcode].Name);
		return FX_NOT_APPLIED;
	}
	if (target->fxqueue.HasEffectWithParam(fx_opcode_immunity2_ref, fx->Opcode)) {
		Log(MESSAGE, "EffectQueue", "{} is immune2 to effect: {}", fmt::WideToChar { target->GetName() }, globals.Opcodes[fx->Opcode].Name);
		// totlm's spin166 should be wholly blocked by spwi210, but only blocks its third effect, so make it fatal
		return FX_ABORT;
	}
	return -1;
}

//this is for whole spell immunity/bounce
static bool DecreaseEffect(Effect* fx)
{
	if (fx->Parameter1) {
		fx->Parameter1--;
		return true;
	}
	return false;
}

//lower decreasing immunities/bounces
static int check_type(Actor* actor, const Effect& fx)
{
	//the protective effect (if any)
	Effect* efx;

	const Actor* caster = core->GetGame()->GetActorByGlobalID(fx.CasterID);
	// Cannot resist own spells!  This even applies to bounced hostile spells, but notably excludes source immunity.
	bool self = (caster == actor);
	// MagicAttack: these spells pierce most generic magical defences (because they need to be able to dispel them).
	bool pierce = (fx.SecondaryType == 4);

	//spell level immunity
	if (fx.Power && actor->fxqueue.HasEffectWithParamPair(fx_level_immunity_ref, fx.Power, 0) && !self) {
		Log(DEBUG, "EffectQueue", "Resisted by level immunity");
		return 0;
	}

	//source immunity (spell name)
	//if source is unspecified, don't resist it
	if (!fx.SourceRef.IsEmpty()) {
		if (actor->fxqueue.HasEffectWithResource(fx_spell_immunity_ref, fx.SourceRef)) {
			Log(DEBUG, "EffectQueue", "Resisted by spell immunity ({})", fx.SourceRef);
			return 0;
		}
		if (actor->fxqueue.HasEffectWithResource(fx_spell_immunity2_ref, fx.SourceRef)) {
			if (fx.SourceRef != "detect") { // our secret door pervasive effect
				Log(DEBUG, "EffectQueue", "Resisted by spell immunity2 ({})", fx.SourceRef);
			}
			return 0;
		}
	}

	if (actor->fxqueue.HasEffectWithParam(fx_projectile_immunity_ref, fx.Projectile)) {
		Log(DEBUG, "EffectQueue", "Resisted by projectile");
		return 0;
	}

	//primary type immunity (school)
	if (fx.PrimaryType && !self && !pierce) {
		if (actor->fxqueue.HasEffectWithParam(fx_school_immunity_ref, fx.PrimaryType)) {
			Log(DEBUG, "EffectQueue", "Resisted by school/primary type");
			return 0;
		}
	}

	//secondary type immunity (usage)
	if (fx.SecondaryType && !self) {
		if (actor->fxqueue.HasEffectWithParam(fx_secondary_type_immunity_ref, fx.SecondaryType)) {
			Log(DEBUG, "EffectQueue", "Resisted by usage/secondary type");
			return 0;
		}
	}

	//decrementing immunity checks
	//decrementing level immunity
	if (fx.Power && fx.Resistance != FX_NO_RESIST_BYPASS_BOUNCE && !self && !pierce && actor->fxqueue.HasEffectWithParam(fx_level_immunity_dec_ref, fx.Power)) {
		if (actor->fxqueue.DecreaseParam1OfEffect(fx_level_immunity_dec_ref, fx.Power)) {
			Log(DEBUG, "EffectQueue", "Resisted by level immunity (decrementing)");
			return 0;
		}
	}

	//decrementing spell immunity
	if (!fx.SourceRef.IsEmpty()) {
		efx = const_cast<Effect*>(actor->fxqueue.HasEffectWithResource(fx_spell_immunity_dec_ref, fx.SourceRef));
		if (efx && DecreaseEffect(efx)) {
			Log(DEBUG, "EffectQueue", "Resisted by spell immunity (decrementing)");
			return 0;
		}
	}
	//decrementing primary type immunity (school)
	if (fx.PrimaryType && !self && !pierce) {
		efx = const_cast<Effect*>(actor->fxqueue.HasEffectWithParam(fx_school_immunity_dec_ref, fx.PrimaryType));
		if (efx && DecreaseEffect(efx)) {
			Log(DEBUG, "EffectQueue", "Resisted by school immunity (decrementing)");
			return 0;
		}
	}

	//decrementing secondary type immunity (usage)
	if (fx.SecondaryType && !self) {
		efx = const_cast<Effect*>(actor->fxqueue.HasEffectWithParam(fx_secondary_type_immunity_dec_ref, fx.SecondaryType));
		if (efx && DecreaseEffect(efx)) {
			Log(DEBUG, "EffectQueue", "Resisted by usage/sectype immunity (decrementing)");
			return 0;
		}
	}

	//spelltrap (absorb)
	if (fx.Power && fx.Resistance != FX_NO_RESIST_BYPASS_BOUNCE && !self && !pierce) {
		efx = const_cast<Effect*>(actor->fxqueue.HasEffectWithParamPair(fx_spelltrap, 0, fx.Power));
		if (efx) {
			//storing the absorbed spell level
			efx->Parameter3 += fx.Power;

			//instead of a single effect, they had to create an effect for each level
			//HOW DAMN LAME
			if (actor->fxqueue.DecreaseParam1OfEffect(fx_spelltrap, fx.Power)) {
				Log(DEBUG, "EffectQueue", "Absorbed by spelltrap");
				return 0;
			}
		}
	}

	// bounce checks; skip all if this is set, or if casting on oneself (obviously)
	if (fx.Resistance == FX_NO_RESIST_BYPASS_BOUNCE || self) {
		return 1;
	}

	ieDword bounce = actor->GetStat(IE_BOUNCE);
	if (fx.Power) {
		if ((bounce & BNC_LEVEL) && actor->fxqueue.HasEffectWithParamPair(fx_level_bounce_ref, 0, fx.Power)) {
			Log(DEBUG, "EffectQueue", "Bounced by level");
			return -1;
		}
	}

	if ((bounce & BNC_PROJECTILE) && actor->fxqueue.HasEffectWithParam(fx_projectile_bounce_ref, fx.Projectile)) {
		Log(DEBUG, "EffectQueue", "Bounced by projectile");
		return -1;
	}

	if (!fx.SourceRef.IsEmpty() && (bounce & BNC_RESOURCE) && actor->fxqueue.HasEffectWithResource(fx_spell_bounce_ref, fx.SourceRef)) {
		Log(DEBUG, "EffectQueue", "Bounced by resource");
		return -1;
	}

	if (fx.PrimaryType && (bounce & BNC_SCHOOL) && !pierce) {
		if (actor->fxqueue.HasEffectWithParam(fx_school_bounce_ref, fx.PrimaryType)) {
			Log(DEBUG, "EffectQueue", "Bounced by school");
			return -1;
		}
	}

	if (fx.SecondaryType && (bounce & BNC_SECTYPE)) {
		if (actor->fxqueue.HasEffectWithParam(fx_secondary_type_bounce_ref, fx.SecondaryType)) {
			Log(DEBUG, "EffectQueue", "Bounced by usage/sectype");
			return -1;
		}
	}
	//decrementing bounce checks

	//level decrementing bounce check
	if (fx.Power && (bounce & BNC_LEVEL_DEC) && !pierce && actor->fxqueue.HasEffectWithParamPair(fx_level_bounce_dec_ref, 0, fx.Power)) {
		if (actor->fxqueue.DecreaseParam1OfEffect(fx_level_bounce_dec_ref, fx.Power)) {
			Log(DEBUG, "EffectQueue", "Bounced by level (decrementing)");
			return -1;
		}
	}

	if (!fx.SourceRef.IsEmpty() && (bounce & BNC_RESOURCE_DEC)) {
		efx = const_cast<Effect*>(actor->fxqueue.HasEffectWithResource(fx_spell_bounce_dec_ref, fx.Resource));
		if (efx && DecreaseEffect(efx)) {
			Log(DEBUG, "EffectQueue", "Bounced by resource (decrementing)");
			return -1;
		}
	}

	if (fx.PrimaryType && (bounce & BNC_SCHOOL_DEC) && !pierce) {
		efx = const_cast<Effect*>(actor->fxqueue.HasEffectWithParam(fx_school_bounce_dec_ref, fx.PrimaryType));
		if (efx && DecreaseEffect(efx)) {
			Log(DEBUG, "EffectQueue", "Bounced by school (decrementing)");
			return -1;
		}
	}

	if (fx.SecondaryType && (bounce & BNC_SECTYPE_DEC)) {
		efx = const_cast<Effect*>(actor->fxqueue.HasEffectWithParam(fx_secondary_type_bounce_dec_ref, fx.SecondaryType));
		if (efx && DecreaseEffect(efx)) {
			Log(DEBUG, "EffectQueue", "Bounced by usage (decrementing)");
			return -1;
		}
	}

	return 1;
}

static inline int CheckMagicResistance(const Actor* actor, const Effect* fx, const Actor* caster)
{
	const auto& globals = Globals::Get();
	//don't resist self
	bool selective_mr = core->HasFeature(GFFlags::SELECTIVE_MAGIC_RES);
	if (fx->CasterID == actor->GetGlobalID() && selective_mr) {
		return -1;
	}

	//magic immunity
	ieDword val = actor->GetStat(IE_RESISTMAGIC);
	bool resisted = false;

	if (globals.iwd2fx) {
		// 3ed style check
		int roll = core->Roll(1, 20, 0);
		ieDword check = fx->CasterLevel + roll;
		int penetration = 0;
		if (caster) {
			// +2/+4 level bonus from the (greater) spell penetration feat
			int feat = caster->GetFeat(Feat::SpellPenetration);
			penetration += 2 * feat;
		}
		check += penetration;
		resisted = (signed) check < (signed) val;
		// ~Spell Resistance check (Spell resistance:) %d vs. (d20 + caster level + spell resistance mod)  = %d + %d + %d.~
		displaymsg->DisplayRollStringName(ieStrRef::ROLL16, GUIColors::LIGHTGREY, actor, val, roll, fx->CasterLevel, penetration);
	} else {
		// 2.5 style check
		resisted = (signed) fx->RandomValue < (signed) val;
	}
	if (resisted) {
		// we take care of irresistible spells a few checks above, so selective mr has no impact here anymore
		displaymsg->DisplayConstantStringName(HCStrings::MagicResisted, GUIColors::WHITE, actor);
		Log(MESSAGE, "EffectQueue", "{} resisted effect: {}", fmt::WideToChar { actor->GetName() }, globals.Opcodes[fx->Opcode].Name);
		return FX_NOT_APPLIED;
	}
	return -1;
}

// check saving throws
static int CheckSaves(Actor* actor, Effect* fx)
{
	if (!actor) return -1;

	const Actor* caster = Scriptable::As<const Actor>(GetCasterObject());
	const auto& globals = Globals::Get();

	// bonus can be improved by school specific bonus
	int bonus = fx->SavingThrowBonus + actor->fxqueue.BonusForParam2(fx_spell_resistance_ref, fx->PrimaryType);
	if (caster) {
		// bonus from generic ac&saves bonus opcode
		bonus += actor->fxqueue.BonusAgainstCreature(fx_ac_vs_creature_type_ref, caster);
		// saving throw could be made difficult by caster's school specific bonus
		bonus -= caster->fxqueue.BonusForParam2(fx_spell_focus_ref, fx->PrimaryType);
	}

	// handle modifiers of specialist mages
	if (!globals.pstflags && !globals.iwd2fx && fx->PrimaryType) {
		// specialist mage's enemies get a -2 penalty to saves vs the specialist's school
		if (caster) bonus -= caster->GetSpecialistSaveBonus(fx->PrimaryType);
		// specialist mages get a +2 bonus to saves to spells of the same school used against them
		bonus += actor->GetSpecialistSaveBonus(fx->PrimaryType);
	}

	static EffectRef fx_damage_ref = { "Damage", -1 };
	if (fx_damage_ref.opcode < 0) {
		Globals::ResolveEffectRef(fx_damage_ref);
	}

	bool saved = false;
	for (int i = 0; i < 5; i++) {
		if (fx->SavingThrowType & (1 << i)) {
			// FIXME: first bonus handling for iwd2 is just a guess
			if (globals.iwd2fx) {
				saved = actor->GetSavingThrow(i, bonus - fx->SavingThrowBonus, fx);
			} else {
				saved = actor->GetSavingThrow(i, bonus, fx);
			}
			if (saved) {
				break;
			}
		}
	}

	bool saveForHalf = fx->IsSaveForHalfDamage || ((int) fx->Opcode == fx_damage_ref.opcode && fx->IsVariable & DamageFlags::SaveForHalf);
	if (saved && saveForHalf) {
		// if we have evasion, we take no damage
		// sadly there's no feat or stat for it
		if (globals.iwd2fx && (actor->GetThiefLevel() > 1 || actor->GetMonkLevel())) {
			fx->Parameter1 = 0;
			// Evades effects from <RESOURCE>~
			if (fx->SourceRef.IsEmpty() || fx->SourceType != 2) {
				displaymsg->DisplayConstantStringName(HCStrings::Evaded2, GUIColors::WHITE, actor);
			} else {
				const Spell* spl = gamedata->GetSpell(fx->SourceRef, true);
				assert(spl);
				core->GetTokenDictionary()["RESOURCE"] = core->GetString(spl->SpellName);
				displaymsg->DisplayConstantStringName(HCStrings::Evaded1, GUIColors::WHITE, actor);
			}
			return FX_NOT_APPLIED;
		} else {
			fx->Parameter1 /= 2;
		}
	} else if (saved) {
		Log(MESSAGE, "EffectQueue", "{} saved against effect: {}", fmt::WideToChar { actor->GetName() }, globals.Opcodes[fx->Opcode].Name);
		return FX_NOT_APPLIED;
	} else {
		if ((int) fx->Opcode == fx_damage_ref.opcode && fx->IsVariable & DamageFlags::FailForHalf) {
			fx->Parameter1 /= 2;
		}
		// improved evasion: take only half damage even though we failed the save
		if (globals.iwd2fx && fx->IsSaveForHalfDamage && actor->HasFeat(Feat::ImprovedEvasion)) {
			fx->Parameter1 /= 2;
		}
	}
	return -1;
}

static int CheckResistances(Effect* fx, Actor* target)
{
	if (!CheckProbability(fx)) {
		return FX_NOT_APPLIED;
	}

	// the effect didn't pass the target level check
	if (CheckLevel(target, fx)) {
		return FX_NOT_APPLIED;
	}

	if (!target) return -1;

	// any localised antimagic at play?
	const auto& globals = Globals::Get();
	if (globals.pstflags && (target->GetSafeStat(IE_STATE_ID) & STATE_ANTIMAGIC)) {
		return -1;
	}
	// also check for SS_ANTIMAGIC for completeness (spell adds level immunities)
	if (target->HasSpellState(SS_ANTIMAGIC)) return -1;

	int immune = CheckOpcodeImmunity(fx, target);
	if (immune != -1) return immune;

	// check magic resistance if applicable
	const Actor* caster = Scriptable::As<const Actor>(GetCasterObject());
	// (note that no MR roll does not preclude saving throws -- see e.g. chromatic orb instakill)
	if (fx->Resistance == FX_CAN_RESIST_CAN_DISPEL && CheckMagicResistance(target, fx, caster) == FX_NOT_APPLIED) {
		// bg2 sequencer trigger spells have bad resistance set, so ignore them
		if (signed(fx->Opcode) != EffectQueue::ResolveEffect(fx_activate_spell_sequencer_ref)) {
			return FX_NOT_APPLIED;
		}
	}

	// the effect didn't pass saving throws
	int saved = CheckSaves(target, fx);
	return saved;
}

// this function is called two different ways
// when FirstApply is set, then the effect isn't stuck on the target
// this happens when a new effect comes in contact with the target.
// if the effect returns FX_DURATION_JUST_EXPIRED then it won't stick
// when first_apply is unset, the effect is already on the target
// this happens on load time too!
// returns FX_NOT_APPLIED if the process shouldn't be calling applyeffect anymore
// returns FX_ABORT if the whole spell this effect is in should be aborted
// it will disable all future effects of same source (only on first apply)

int EffectQueue::ApplyEffect(Actor* target, Effect* fx, ieDword first_apply, ieDword resistance) const
{
	if (fx->TimingMode == FX_DURATION_JUST_EXPIRED) {
		return FX_NOT_APPLIED;
	}

	const auto& globals = Globals::Get();

	if (fx->Opcode >= Globals::MAX_EFFECTS) {
		fx->TimingMode = FX_DURATION_JUST_EXPIRED;
		return FX_NOT_APPLIED;
	}

	ieDword GameTime = core->GetGame()->GameTime;

	if (first_apply) {
		fx->FirstApply = 1;
		// we do proper target vs targetless checks below
		if (target) fx->SetPosition(target->Pos);

		//gemrb specific, stat based chance
		const Actor* OwnerActor = Scriptable::As<Actor>(Owner);
		if (fx->ProbabilityRangeMin == 100 && OwnerActor) {
			fx->ProbabilityRangeMin = 0;
			fx->ProbabilityRangeMax = static_cast<ieWord>(OwnerActor->GetSafeStat(fx->ProbabilityRangeMax));
		}

		if (resistance) {
			int resisted = CheckResistances(fx, target);
			if (resisted != -1) {
				fx->TimingMode = FX_DURATION_JUST_EXPIRED;
				return resisted;
			}
		}

		//Same as in items and spells
		if (fx->SourceFlags & SF_HOSTILE) {
			if (target && target != Owner && OwnerActor) {
				target->AttackedBy(OwnerActor);
			}
		}

		if (NeedPrepare(fx->TimingMode)) {
			//save delay for later
			fx->SecondaryDelay = fx->Duration;
			bool inTicks = false;
			if (fx->TimingMode == FX_DURATION_INSTANT_LIMITED) {
				fx->TimingMode = FX_DURATION_ABSOLUTE;
			} else if (fx->TimingMode == FX_DURATION_DELAY_LIMITED && fx->IsVariable && globals.pstflags) {
				// override onset delay, useful for effects with scaled (simplified) duration
				fx->Duration = fx->IsVariable;
				inTicks = true;
				// also change TimingMode to FX_DURATION_DELAY_LIMITED_PENDING if needed
			}
			if (fx->TimingMode == FX_DURATION_INSTANT_LIMITED_TICKS) {
				fx->TimingMode = FX_DURATION_ABSOLUTE;
				inTicks = true;
			} else if (globals.pstflags && !(fx->SourceFlags & SF_SIMPLIFIED_DURATION) && fx->TimingMode != FX_DURATION_ABSOLUTE) {
				// pst stored the delay in ticks already, so we use a variant of PrepareDuration
				// ... only FX_DURATION_INSTANT_LIMITED, FX_DURATION_DELAY_UNSAVED and FX_DURATION_DELAY_LIMITED_PENDING used seconds
				// unless it's our unhardcoded spells which use iwd2-style simplified duration in rounds per level
				inTicks = true;
			}
			if (inTicks) {
				fx->Duration = (fx->Duration ? fx->Duration : 1) + GameTime;
			} else {
				fx->PrepareDuration(GameTime);
			}
		}
	}
	//check if the effect has triggered or expired
	switch (DelayType(fx->TimingMode & 0xff)) {
		case TimingType::Delayed:
			if (fx->Duration > GameTime) {
				return FX_NOT_APPLIED;
			}
			//effect triggered
			//delayed duration (3)
			if (NeedPrepare(fx->TimingMode)) {
				//prepare for delayed duration effects
				fx->Duration = fx->SecondaryDelay;
				fx->PrepareDuration(GameTime);
			}
			fx->TimingMode = TriggeredEffect(fx->TimingMode);
			break;
		case TimingType::Duration:
			if (fx->Duration <= GameTime) {
				fx->TimingMode = FX_DURATION_JUST_EXPIRED;
				//add a return here, if 0 duration effects shouldn't work
			}
			break;
		//permanent effect (so there is no warning)
		case TimingType::Permanent:
			break;
		//this shouldn't happen
		default:
			error("EffectQueue", "Unknown delay type: {} (from {})", int(DelayType(fx->TimingMode & 0xff)), fx->TimingMode);
	}

	int res = FX_ABORT;
	if (fx->Opcode >= Globals::MAX_EFFECTS) {
		return res;
	}

	const EffectDesc& ed = globals.Opcodes[fx->Opcode];
	if (!ed) {
		return res;
	}

	if (!target && !(ed.Flags & EFFECT_NO_ACTOR)) {
		Log(MESSAGE, "EffectQueue", "Targetless opcode without EFFECT_NO_ACTOR: {}, skipping!", fx->Opcode);
		return FX_NOT_APPLIED;
	}

	if (target && fx->FirstApply) {
		if (!target->fxqueue.HasEffectWithParamPair(fx_protection_from_display_string_ref, fx->Parameter1, 0)) {
			displaymsg->DisplayStringName(ed.Strref, GUIColors::WHITE, target, STRING_FLAGS::SOUND);
		}
	}

	res = ed(Owner, target, fx);
	fx->FirstApply = 0;

	switch (res) {
		case FX_APPLIED:
			// normal effect with duration
			break;
		case FX_ABORT:
		case FX_NOT_APPLIED:
			// instant effect, pending removal
			// for example, a damage effect
			fx->TimingMode = FX_DURATION_JUST_EXPIRED;
			break;
		case FX_INSERT:
			// put this effect in the beginning of the queue
			// all known insert effects are 'permanent' too
			// that is the AC effect only
			// actually, permanent effects seem to be
			// inserted by the game engine too
		case FX_PERMANENT:
			// don't stick around if it was executed permanently
			// for example, a permanent strength modifier effect
			if (fx->TimingMode == FX_DURATION_INSTANT_PERMANENT) {
				fx->TimingMode = FX_DURATION_JUST_EXPIRED;
			}
			break;
		default:
			error("EffectQueue", "Unknown effect result '{:#x}', aborting ...", res);
	}

	return res;
}

// looks for opcode with param2

#define MATCH_OPCODE() \
	if (fx.Opcode != opcode) { \
		continue; \
	}

// useful for: remove projectile type
#define MATCH_PROJECTILE() \
	if (fx.Projectile != projectile) { \
		continue; \
	}

static const bool fx_live[MAX_TIMING_MODE] = { true, true, true, false, false, false, false, false, true, true, true, false };
static inline bool IsLive(ieByte timingmode)
{
	if (timingmode >= MAX_TIMING_MODE) return false;
	return fx_live[timingmode];
}

#define MATCH_LIVE_FX() \
	if (!IsLive(fx.TimingMode)) { \
		continue; \
	}
#define MATCH_PARAM1() \
	if (fx.Parameter1 != param1) { \
		continue; \
	}
#define MATCH_PARAM2() \
	if (fx.Parameter2 != param2) { \
		continue; \
	}
#define MATCH_TIMING() \
	if (fx.TimingMode != timing) { \
		continue; \
	}

//call this from an applied effect, after it returns, these effects
//will be killed along with it
void EffectQueue::RemoveAllEffects(ieDword opcode)
{
	for (auto& fx : effects) {
		MATCH_OPCODE()
		MATCH_LIVE_FX()

		fx.TimingMode = FX_DURATION_JUST_EXPIRED;
	}
}

static bool RemoveMemo(Actor* ownerActor, unsigned int type, unsigned int idx, ieDword bonus = 0)
{
	CRESpellMemorization* sm = ownerActor->spellbook.GetSpellMemorization(type, idx);
	size_t diff = sm->SlotCountWithBonus - sm->SlotCount;
	if (diff == 0) return false;

	if (sm->SlotCount >= sm->memorized_spells.size()) return false;
	diff = std::min(diff, sm->memorized_spells.size());
	if (bonus) diff = std::min<size_t>(diff, bonus);
	for (size_t j = 0; j < diff; j++) {
		delete sm->memorized_spells.back();
		sm->memorized_spells.pop_back();
	}
	return true;
}

void EffectQueue::RemoveBonusMemorizations(const Effect& fx)
{
	static EffectRef fx_spell_bonus1_ref = { "WizardSpellSlotsModifier", -1 };
	static EffectRef fx_spell_bonus2_ref = { "PriestSpellSlotsModifier", -1 };
	if (fx_spell_bonus1_ref.opcode < 0) {
		Globals::ResolveEffectRef(fx_spell_bonus1_ref);
		Globals::ResolveEffectRef(fx_spell_bonus2_ref);
	}

	Actor* ownerActor = Scriptable::As<Actor>(Owner);
	if (!ownerActor) return;

	unsigned int type;
	if ((int) fx.Opcode == fx_spell_bonus1_ref.opcode) {
		type = IE_SPELL_TYPE_WIZARD;
	} else if ((int) fx.Opcode == fx_spell_bonus2_ref.opcode) {
		type = IE_SPELL_TYPE_PRIEST;
	} else {
		return;
	}

	// delete granted memorizations
	if (fx.Parameter2 == 0) {
		// doubled counts up to fx.Parameter1 level
		unsigned int level = std::min(fx.Parameter1, ownerActor->spellbook.GetSpellLevelCount(type));
		for (unsigned int i = 0; i < level; i++) {
			if (!RemoveMemo(ownerActor, type, i)) continue;
		}
	} else if (fx.Parameter2 == 0x200) {
		// doubled counts at fx.Parameter1 level
		unsigned int level = fx.Parameter1;
		if (level > ownerActor->spellbook.GetSpellLevelCount(type)) return;
		RemoveMemo(ownerActor, type, level);
	} else {
		// fx.Parameter1 bonus to all levels in param2 mask
		unsigned int level = ownerActor->spellbook.GetSpellLevelCount(type);
		int mask = 1;
		for (unsigned int i = 0; i < level; i++) {
			if (!(fx.Parameter2 & mask)) {
				mask <<= 1;
				continue;
			}

			RemoveMemo(ownerActor, type, i, fx.Parameter1);
			mask <<= 1;
		}
	}
}

//removes all equipping effects that match slotcode
bool EffectQueue::RemoveEquippingEffects(size_t slotCode)
{
	bool removed = false;
	for (auto& fx : effects) {
		if (!IsEquipped(fx.TimingMode)) continue;
		if (slotCode != 0xffffffff && fx.InventorySlot != (ieDwordSigned) slotCode) continue;

		fx.TimingMode = FX_DURATION_JUST_EXPIRED;
		RemoveBonusMemorizations(fx);
		removed = true;
	}
	return removed;
}

//removes all effects that match projectile
void EffectQueue::RemoveAllEffectsWithProjectile(ieDword projectile)
{
	for (auto& fx : effects) {
		MATCH_PROJECTILE()

		fx.TimingMode = FX_DURATION_JUST_EXPIRED;
	}
}

//remove effects belonging to a given spell
void EffectQueue::RemoveAllEffects(const ResRef& removed)
{
	for (auto& fx : effects) {
		MATCH_LIVE_FX()
		if (removed != fx.SourceRef) {
			continue;
		}

		fx.TimingMode = FX_DURATION_JUST_EXPIRED;
	}

	Actor* OwnerActor = Scriptable::As<Actor>(Owner);
	if (!OwnerActor) return;

	// we didn't catch effects that don't persist — they still need to be undone
	// FX_PERMANENT returners aren't part of the queue, so permanent stat mods can't be detected
	// good test case is the Oozemaster druid kit from Divine remix, which decreases charisma in its clab
	const Spell* spell = gamedata->GetSpell(removed, true);
	if (!spell) return; // can be hit until all the iwd2 clabs are implemented
	if (spell->ext_headers.size() > 1) {
		Log(WARNING, "EffectQueue", "Spell {} has more than one extended header, removing only first!", removed);
	}
	const SPLExtHeader* sph = spell->GetExtHeader(0);
	if (!sph) return; // some iwd2 clabs are only markers
	const auto& Opcodes = Globals::Get().Opcodes;
	for (const Effect& origfx : sph->features) {
		if (origfx.TimingMode != FX_DURATION_INSTANT_PERMANENT) continue;
		if (!(Opcodes[origfx.Opcode].Flags & EFFECT_SPECIAL_UNDO)) continue;

		// unapply the effect by applying the reverse — if feasible
		// but don't alter the spell itself or other users won't get what they asked for
		Effect* fx = CreateEffectCopy(&origfx, origfx.Opcode, origfx.Parameter1, origfx.Parameter2);

		// state setting effects are idempotent, so wouldn't cause problems during clab reapplication
		// ...they would during disabled dualclass levels, but it would be too annoying to try, since
		// not all have cure variants (eg. fx_set_blur_state) and if there were two sources, we'd kill
		// the effect just the same.

		// further ignore a few more complicated-to-undo opcodes
		// fx_ac_vs_damage_type_modifier, fx_ac_vs_damage_type_modifier_iwd2, fx_ids_modifier, fx_attacks_per_round_modifier

		// fx_pause_target is a one-tick shot pony, nothing to do

		fx->Parameter1 = 0xffffffff - fx->Parameter1; // over- or under-flow is intentional

		Log(DEBUG, "EffectQueue", "Manually removing effect {} (from {})", fx->Opcode, removed);
		ApplyEffect(OwnerActor, fx, 1, 0);
		delete fx;
	}
	gamedata->FreeSpell(spell, removed, false);
}

//remove effects belonging to a given spell, but only if they match timing method x
void EffectQueue::RemoveAllEffects(const ResRef& removed, ieByte timing)
{
	for (auto& fx : effects) {
		MATCH_TIMING()
		if (removed != fx.SourceRef) {
			continue;
		}

		fx.TimingMode = FX_DURATION_JUST_EXPIRED;
	}
}

//this will modify effect reference
void EffectQueue::RemoveAllEffects(EffectRef& effectReference)
{
	Globals::ResolveEffectRef(effectReference);
	if (effectReference.opcode < 0) {
		return;
	}
	RemoveAllEffects(effectReference.opcode);
}

//Removes all effects with a matching resource field
void EffectQueue::RemoveAllEffectsWithResource(ieDword opcode, const ResRef& resource)
{
	for (auto& fx : effects) {
		MATCH_OPCODE()
		MATCH_LIVE_FX()
		if (fx.Resource != resource) {
			continue;
		}

		fx.TimingMode = FX_DURATION_JUST_EXPIRED;
	}
}

//this will modify effect reference
void EffectQueue::RemoveAllEffectsWithResource(EffectRef& effectReference, const ResRef& resource)
{
	Globals::ResolveEffectRef(effectReference);
	RemoveAllEffectsWithResource(effectReference.opcode, resource);
}

//Removes all effects with a matching resource field
void EffectQueue::RemoveAllEffectsWithSource(ieDword opcode, const ResRef& source, int mode)
{
	for (auto& fx : effects) {
		MATCH_OPCODE()
		if (fx.SourceRef != source) continue;

		// equipping effects only
		// IsEquipped excludes to many to match ee's opcode 321
		if (mode == 1 && fx.TimingMode != FX_DURATION_INSTANT_WHILE_EQUIPPED) {
			continue;
		}

		// "timed" effects only
		if (mode == 2 && (fx.TimingMode == FX_DURATION_INSTANT_WHILE_EQUIPPED || fx.TimingMode == FX_DURATION_INSTANT_PERMANENT_AFTER_BONUSES)) {
			continue;
		}

		// mode 0 or anything else means remove all effects that match

		fx.TimingMode = FX_DURATION_JUST_EXPIRED;
	}
}

void EffectQueue::RemoveAllEffectsWithSource(EffectRef& effectReference, const ResRef& source, int mode)
{
	Globals::ResolveEffectRef(effectReference);
	RemoveAllEffectsWithSource(effectReference.opcode, source, mode);
}

//This method could be used to remove stat modifiers that would lower a stat
//(works only if a higher stat means good for the target)
void EffectQueue::RemoveAllDetrimentalEffects(ieDword opcode, ieDword current)
{
	for (auto& fx : effects) {
		MATCH_OPCODE()
		MATCH_LIVE_FX()

		switch (fx.Parameter2) {
			case 0:
			case 3:
				if ((signed) fx.Parameter1 >= 0) continue;
				break;
			case 1:
			case 4:
				if ((signed) fx.Parameter1 >= (signed) current) continue;
				break;
			case 2:
			case 5:
				if ((signed) fx.Parameter1 >= 100) continue;
				break;
			default:
				break;
		}
		fx.TimingMode = FX_DURATION_JUST_EXPIRED;
	}
}

//this will modify effect reference
void EffectQueue::RemoveAllDetrimentalEffects(EffectRef& effectReference, ieDword current)
{
	Globals::ResolveEffectRef(effectReference);
	RemoveAllDetrimentalEffects(effectReference.opcode, current);
}

//Removes all effects with a matching param2
//param2 is usually an effect's subclass (quality) while param1 is more like quantity.
//So opcode+param2 usually pinpoints an effect better when not all effects of a given
//opcode need to be removed (see removal of portrait icon)
void EffectQueue::RemoveAllEffectsWithParam(ieDword opcode, ieDword param, bool param1)
{
	for (auto& fx : effects) {
		MATCH_OPCODE()
		MATCH_LIVE_FX()
		if (param1) {
			if (fx.Parameter1 != param) continue;
		} else {
			if (fx.Parameter2 != param) continue;
		}

		fx.TimingMode = FX_DURATION_JUST_EXPIRED;
	}
}

//this will modify effect reference
void EffectQueue::RemoveAllEffectsWithParam(EffectRef& effectReference, ieDword param2)
{
	Globals::ResolveEffectRef(effectReference);
	RemoveAllEffectsWithParam(effectReference.opcode, param2);
}

void EffectQueue::RemoveAllEffectsWithParam1(EffectRef& effectReference, ieDword param1)
{
	Globals::ResolveEffectRef(effectReference);
	RemoveAllEffectsWithParam(effectReference.opcode, param1, true);
}

//Removes all effects with a matching resource field
void EffectQueue::RemoveAllEffectsWithParamAndResource(ieDword opcode, ieDword param2, const ResRef& resource)
{
	for (auto& fx : effects) {
		MATCH_OPCODE()
		MATCH_LIVE_FX()
		MATCH_PARAM2()

		if (!resource.IsEmpty() && fx.Resource != resource) continue;

		fx.TimingMode = FX_DURATION_JUST_EXPIRED;
	}
}

//this will modify effect reference
void EffectQueue::RemoveAllEffectsWithParamAndResource(EffectRef& effectReference, ieDword param2, const ResRef& resource)
{
	Globals::ResolveEffectRef(effectReference);
	RemoveAllEffectsWithParamAndResource(effectReference.opcode, param2, resource);
}

//this function is called by FakeEffectExpiryCheck
//probably also called by rest
void EffectQueue::RemoveExpiredEffects(ieDword futuretime)
{
	ieDword GameTime = core->GetGame()->GameTime;
	// prevent overflows, since we pass the max futuretime for guaranteed expiry
	if (GameTime + futuretime < GameTime) {
		GameTime = 0xffffffff;
	} else {
		GameTime += futuretime;
	}

	for (auto& fx : effects) {
		//FIXME: how this method handles delayed effects???
		//it should remove them as well, i think
		if (DelayType(fx.TimingMode) != TimingType::Permanent && fx.Duration <= GameTime) {
			fx.TimingMode = FX_DURATION_JUST_EXPIRED;
		}
	}
}

//this effect will expire all effects that are not truly permanent
//which i call permanent after death (iesdp calls it permanent after bonuses)
void EffectQueue::RemoveAllNonPermanentEffects()
{
	for (auto& fx : effects) {
		if (IsRemovable(fx.TimingMode)) {
			fx.TimingMode = FX_DURATION_JUST_EXPIRED;
		}
	}
}

//remove certain levels of effects, possibly matching school/secondary type
//this method removes whole spells (tied together by their source)
void EffectQueue::RemoveLevelEffects(ieDword level, ieDword Flags, ieDword match, const Scriptable* target)
{
	ResRef removed;
	for (auto& fx : effects) {
		if (fx.Power > level) {
			continue;
		}

		if (removed && removed != fx.SourceRef) {
			continue;
		}
		if (Flags & RL_MATCHSCHOOL && fx.PrimaryType != match) {
			continue;
		}
		if (Flags & RL_MATCHSECTYPE && fx.SecondaryType != match) {
			continue;
		}
		//if dispellable was not set, or the effect is dispellable
		//then remove it
		if (Flags & RL_DISPELLABLE && !(fx.Resistance & FX_CAN_DISPEL)) {
			continue;
		}
		fx.TimingMode = FX_DURATION_JUST_EXPIRED;
		if (Flags & RL_REMOVEFIRST) {
			removed = fx.SourceRef;
		}

		// provide feedback
		if (Flags & RL_MATCHSCHOOL) {
			AutoTable schoolTable = gamedata->LoadTable("mschool", true);
			if (!schoolTable) continue;
			ieStrRef msg = schoolTable->QueryFieldAsStrRef(fx.PrimaryType, 0);
			displaymsg->DisplayRollStringName(msg, GUIColors::WHITE, target);
		} else if (Flags & RL_MATCHSECTYPE) {
			AutoTable secTypeTable = gamedata->LoadTable("msectype", true);
			if (!secTypeTable) continue;
			ieStrRef msg = secTypeTable->QueryFieldAsStrRef(fx.SecondaryType, 0);
			displaymsg->DisplayRollStringName(msg, GUIColors::WHITE, target);
		}
	}
}

void EffectQueue::DispelEffects(const Effect* dispeller, ieDword level)
{
	for (auto& fx : effects) {
		if (&fx == dispeller) continue;

		// this should also ignore all equipping effects
		if (!(fx.Resistance & FX_CAN_DISPEL)) {
			continue;
		}

		if (!RollDispelChance(fx.CasterLevel, level)) continue;
		// finally dispel
		fx.TimingMode = FX_DURATION_JUST_EXPIRED;
	}
}

bool EffectQueue::RollDispelChance(ieDword casterLevel, ieDword level)
{
	// 50% base chance of success; always at least 1% chance of failure or success
	// positive level diff modifies the base chance by 5%, negative by -10%
	int diff = level - casterLevel;
	if (diff > 0) {
		diff *= 5;
	} else if (diff < 0) {
		diff *= 10;
	}
	diff += 50;

	int roll = core->Roll(1, 100, 0);
	if (roll == 1) return false;
	if (roll == 100 || roll < diff) return true;
	return false;
}

const Effect* EffectQueue::HasOpcode(ieDword opcode) const
{
	for (const auto& fx : effects) {
		MATCH_OPCODE()
		MATCH_LIVE_FX()

		return &fx;
	}
	return nullptr;
}

const Effect* EffectQueue::HasEffect(EffectRef& effectReference) const
{
	Globals::ResolveEffectRef(effectReference);
	if (effectReference.opcode < 0) {
		return nullptr;
	}
	return HasOpcode(effectReference.opcode);
}

Effect* EffectQueue::HasOpcode(ieDword opcode)
{
	for (auto& fx : effects) {
		MATCH_OPCODE()
		MATCH_LIVE_FX()

		return &fx;
	}
	return nullptr;
}

Effect* EffectQueue::HasEffect(EffectRef& effectReference)
{
	Globals::ResolveEffectRef(effectReference);
	if (effectReference.opcode < 0) {
		return nullptr;
	}
	return HasOpcode(effectReference.opcode);
}

const Effect* EffectQueue::HasOpcodeWithParam(ieDword opcode, ieDword param2) const
{
	for (auto& fx : effects) {
		MATCH_OPCODE()
		MATCH_LIVE_FX()
		MATCH_PARAM2()

		return &fx;
	}
	return nullptr;
}

//this will modify effect reference
const Effect* EffectQueue::HasEffectWithParam(EffectRef& effectReference, ieDword param2) const
{
	Globals::ResolveEffectRef(effectReference);
	if (effectReference.opcode < 0) {
		return nullptr;
	}
	return HasOpcodeWithParam(effectReference.opcode, param2);
}

//looks for opcode with pairs of parameters (useful for protection against creature, extra damage or extra thac0 against creature)
//generally an IDS targeting

const Effect* EffectQueue::HasOpcodeWithParamPair(ieDword opcode, ieDword param1, ieDword param2) const
{
	for (auto& fx : effects) {
		MATCH_OPCODE()
		MATCH_LIVE_FX()
		MATCH_PARAM2()
		//0 is always accepted as first parameter
		if (param1) {
			MATCH_PARAM1()
		}

		return &fx;
	}
	return nullptr;
}

//this will modify effect reference
const Effect* EffectQueue::HasEffectWithParamPair(EffectRef& effectReference, ieDword param1, ieDword param2) const
{
	Globals::ResolveEffectRef(effectReference);
	if (effectReference.opcode < 0) {
		return nullptr;
	}
	return HasOpcodeWithParamPair(effectReference.opcode, param1, param2);
}

//decreases all eligible effects at once!  returns false if all spent already
bool EffectQueue::DecreaseParam1OfEffect(ieDword opcode, ieDword amount)
{
	bool found = false;
	for (auto& fx : effects) {
		MATCH_OPCODE()
		MATCH_LIVE_FX()
		ieDword& amount_left = fx.Parameter1;
		if (amount_left > amount) {
			amount_left -= amount;
			found = true;
		} else if (amount_left > 0) {
			amount_left = 0;
			found = true;
		}
	}
	return found;
}

bool EffectQueue::DecreaseParam1OfEffect(EffectRef& effectReference, ieDword amount)
{
	Globals::ResolveEffectRef(effectReference);
	if (effectReference.opcode < 0) {
		return false;
	}
	return DecreaseParam1OfEffect(effectReference.opcode, amount);
}

//this is only used for Cloak of Warding Overlay in PST
//returns the damage amount NOT soaked
int EffectQueue::DecreaseParam3OfEffect(ieDword opcode, ieDword amount, ieDword param2)
{
	for (auto& fx : effects) {
		MATCH_OPCODE()
		MATCH_LIVE_FX()
		MATCH_PARAM2()
		ieDword value = fx.Parameter3;
		if (value > amount) {
			value -= amount;
			amount = 0;
		} else {
			amount -= value;
			value = 0;
		}
		fx.Parameter3 = value;
		if (value) {
			return 0;
		}
	}
	return amount;
}

//this is only used for Cloak of Warding Overlay in PST
//returns the damage amount NOT soaked
int EffectQueue::DecreaseParam3OfEffect(EffectRef& effectReference, ieDword amount, ieDword param2)
{
	Globals::ResolveEffectRef(effectReference);
	if (effectReference.opcode < 0) {
		return amount;
	}
	return DecreaseParam3OfEffect(effectReference.opcode, amount, param2);
}

//this function does IDS targeting for effects (extra damage/thac0 against creature)
//faction/team may be useful for grouping creatures differently, without messing with existing general/specific values
static const int ids_stats[9] = { IE_FACTION, IE_TEAM, IE_EA, IE_GENERAL, IE_RACE, IE_CLASS, IE_SPECIFIC, IE_SEX, IE_ALIGNMENT };

//0,1 and 9 are only in GemRB
int EffectQueue::BonusAgainstCreature(ieDword opcode, const Actor* actor) const
{
	ieDword sum = 0;
	for (const auto& fx : effects) {
		MATCH_OPCODE()
		MATCH_LIVE_FX()
		if (fx.Parameter1) {
			ieDword param1;
			ieDword ids = fx.Parameter2;
			switch (ids) {
				case 0:
				case 1:
				case 2:
				case 3:
				case 4:
				case 6:
				case 7:
				case 8:
					param1 = actor->GetStat(ids_stats[ids]);
					MATCH_PARAM1()
					break;
				case 5:
					param1 = actor->GetActiveClass();
					MATCH_PARAM1()
					break;
				case 9:
					//pseudo stat/classmask
					param1 = actor->GetClassMask() & fx.Parameter1;
					if (!param1) continue;
					break;
				default:
					break;
			}
		}
		ieDword val = fx.Parameter3;
		//we are really lucky with this, most of these boni are using +2 (including fiendslayer feat)
		//it would be much more inconvenient if we had to use v2 effect files
		if (!val) val = 2;
		sum += val;
	}
	return static_cast<int>(sum);
}

int EffectQueue::BonusAgainstCreature(EffectRef& effectReference, const Actor* actor) const
{
	Globals::ResolveEffectRef(effectReference);
	if (effectReference.opcode < 0) {
		return 0;
	}
	return BonusAgainstCreature(effectReference.opcode, actor);
}

int EffectQueue::BonusForParam2(ieDword opcode, ieDword param2) const
{
	int sum = 0;
	for (const auto& fx : effects) {
		MATCH_OPCODE()
		MATCH_LIVE_FX()
		MATCH_PARAM2()
		sum += fx.Parameter1;
	}
	return sum;
}

int EffectQueue::BonusForParam2(EffectRef& effectReference, ieDword param2) const
{
	Globals::ResolveEffectRef(effectReference);
	if (effectReference.opcode < 0) {
		return 0;
	}
	return BonusForParam2(effectReference.opcode, param2);
}

int EffectQueue::MaxParam1(ieDword opcode, bool positive) const
{
	int max = 0;
	ieDwordSigned param1 = 0;
	for (const auto& fx : effects) {
		MATCH_OPCODE()
		MATCH_LIVE_FX()

		param1 = signed(fx.Parameter1);
		if ((positive && param1 > max) || (!positive && param1 < max)) {
			max = param1;
		}
	}
	return max;
}

int EffectQueue::MaxParam1(EffectRef& effectReference, bool positive) const
{
	Globals::ResolveEffectRef(effectReference);
	if (effectReference.opcode < 0) {
		return 0;
	}
	return MaxParam1(effectReference.opcode, positive);
}

bool EffectQueue::WeaponImmunity(ieDword opcode, int enchantment, ieDword weapontype) const
{
	for (const auto& fx : effects) {
		MATCH_OPCODE()
		MATCH_LIVE_FX()

		int magic = (int) fx.Parameter1;
		ieDword mask = fx.Parameter3;
		ieDword value = fx.Parameter4;
		if (magic == 0 && enchantment) {
			continue;
		} else if (magic > 0 && enchantment > magic) {
			continue;
		}

		if ((weapontype & mask) != value) {
			continue;
		}
		return true;
	}
	return false;
}

bool EffectQueue::WeaponImmunity(int enchantment, ieDword weapontype) const
{
	Globals::ResolveEffectRef(fx_weapon_immunity_ref);
	if (fx_weapon_immunity_ref.opcode < 0) {
		return false;
	}
	return WeaponImmunity(fx_weapon_immunity_ref.opcode, enchantment, weapontype);
}

void EffectQueue::AddWeaponEffects(EffectQueue* fxqueue, EffectRef& fx_ref, ieDword param2) const
{
	Globals::ResolveEffectRef(fx_ref);
	if (fx_ref.opcode < 0) {
		return;
	}

	ieDword opcode = fx_ref.opcode;
	Point p(-1, -1);

	for (const auto& fx : effects) {
		MATCH_OPCODE()
		MATCH_LIVE_FX()
		if (!param2 && fx.Parameter2 != param2) continue;

		Effect* fx2 = core->GetEffect(fx.Resource, fx.Power, p);
		if (!fx2) continue;
		fx2->Target = FX_TARGET_PRESET;
		fxqueue->AddEffect(fx2, true);
	}
}

// figure out how much damage reduction applies for a given weapon enchantment and damage type
int EffectQueue::SumDamageReduction(EffectRef& effectReference, ieDword weaponEnchantment, int& total) const
{
	Globals::ResolveEffectRef(effectReference);
	ieDword opcode = effectReference.opcode;
	int remaining = 0;
	int count = 0;

	for (const auto& fx : effects) {
		MATCH_OPCODE()
		MATCH_LIVE_FX()

		count++;
		// add up if the effect has enough enchantment (or ignores it)
		if (!fx.Parameter2 || fx.Parameter2 > weaponEnchantment) {
			remaining += fx.Parameter1;
		}
		total += fx.Parameter1;
	}
	if (count) {
		return remaining;
	} else {
		return -1;
	}
}

//useful for immunity vs spell, can't use item, etc.
const Effect* EffectQueue::HasOpcodeWithResource(ieDword opcode, const ResRef& resource) const
{
	for (auto& fx : effects) {
		MATCH_OPCODE()
		MATCH_LIVE_FX()
		if (fx.Resource != resource) continue;

		return &fx;
	}
	return nullptr;
}

const Effect* EffectQueue::HasEffectWithResource(EffectRef& effectReference, const ResRef& resource) const
{
	Globals::ResolveEffectRef(effectReference);
	return HasOpcodeWithResource(effectReference.opcode, resource);
}

// for tobex bounce triggers
const Effect* EffectQueue::HasEffectWithPower(EffectRef& effectReference, ieDword power) const
{
	Globals::ResolveEffectRef(effectReference);
	return HasOpcodeWithPower(effectReference.opcode, power);
}

const Effect* EffectQueue::HasOpcodeWithPower(ieDword opcode, ieDword power) const
{
	for (const auto& fx : effects) {
		MATCH_OPCODE()
		MATCH_LIVE_FX()
		// NOTE: matching greater or equals!
		if (fx.Power < power) continue;

		return &fx;
	}
	return nullptr;
}

//returns the first effect with source 'Removed'
const Effect* EffectQueue::HasSource(const ResRef& removed) const
{
	for (const auto& fx : effects) {
		MATCH_LIVE_FX()
		if (removed != fx.SourceRef) {
			continue;
		}

		return &fx;
	}
	return nullptr;
}

//used in contingency/sequencer code (cannot have the same contingency twice)
const Effect* EffectQueue::HasOpcodeWithSource(ieDword opcode, const ResRef& removed) const
{
	for (auto& fx : effects) {
		MATCH_OPCODE()
		MATCH_LIVE_FX()
		if (removed != fx.SourceRef) {
			continue;
		}

		return &fx;
	}
	return nullptr;
}

const Effect* EffectQueue::HasEffectWithSource(EffectRef& effectReference, const ResRef& resource) const
{
	Globals::ResolveEffectRef(effectReference);
	return HasOpcodeWithSource(effectReference.opcode, resource);
}

bool EffectQueue::HasAnyDispellableEffect() const
{
	for (const Effect& fx : effects) {
		if (fx.Resistance & FX_CAN_DISPEL) {
			return true;
		}
	}
	return false;
}

std::string EffectQueue::dump(bool print) const
{
	std::string buffer("EFFECT QUEUE:\n");
	int i = 0;
	const auto& Opcodes = Globals::Get().Opcodes;
	for (const Effect& fx : effects) {
		if (fx.Opcode >= Globals::MAX_EFFECTS) {
			Log(FATAL, "EffectQueue", "Encountered opcode off the charts: {}! Report this immediately!", fx.Opcode);
			return buffer;
		}
		const auto& opcodeName = Opcodes[fx.Opcode].Name ? Opcodes[fx.Opcode].Name : "unknown opcode";
		AppendFormat(buffer, " {:2d}: 0x{:02x}: {} ({}, {}) S:{}\n", i++, fx.Opcode, opcodeName, fx.Parameter1, fx.Parameter2, fx.SourceRef);
	}
	if (print) Log(DEBUG, "EffectQueue", "{}", buffer);
	return buffer;
}

//alter the color effect in case the item is equipped in the shield slot
void EffectQueue::HackColorEffects(const Actor* Owner, Effect* fx)
{
	if (fx->InventorySlot != Owner->inventory.GetShieldSlot()) return;

	unsigned int gradienttype = fx->Parameter2 & 0xF0;
	if (gradienttype == 0x10) {
		gradienttype = 0x20; // off-hand
		fx->Parameter2 &= ~0xF0;
		fx->Parameter2 |= gradienttype;
	}
}

//iterate through saved effects
const Effect* EffectQueue::GetNextSavedEffect(queue_t::const_iterator& f) const
{
	while (f != effects.end()) {
		const Effect& effect = *f;
		f++;
		if (effect.Persistent()) {
			return &effect;
		}
	}
	return nullptr;
}

const Effect* EffectQueue::GetNextEffect(queue_t::const_iterator& f) const
{
	if (f != effects.end()) return &(*f++);
	return nullptr;
}

Effect* EffectQueue::GetNextEffect(queue_t::iterator& f)
{
	if (f != effects.end()) return &(*f++);
	return nullptr;
}

ieDword EffectQueue::CountEffects(ieDword opcode, ieDword param1, ieDword param2, const ResRef& resource, const ResRef& source) const
{
	ieDword cnt = 0;

	for (const auto& fx : effects) {
		if (opcode != 0xffffffff)
			MATCH_OPCODE()
		if (param1 != 0xffffffff)
			MATCH_PARAM1()
		if (param2 != 0xffffffff)
			MATCH_PARAM2()
		if (!resource.IsEmpty() && fx.Resource != resource) continue;
		if (!source.IsEmpty() && fx.SourceRef != source) continue;
		cnt++;
	}
	return cnt;
}

unsigned int EffectQueue::GetEffectOrder(EffectRef& effectReference, const Effect* fx2) const
{
	ieDword cnt = 1;
	ieDword opcode = ResolveEffect(effectReference);

	for (const auto& fx : effects) {
		MATCH_OPCODE()
		MATCH_LIVE_FX()
		if (&fx == fx2) break;
		cnt++;
	}
	return cnt;
}

void EffectQueue::ModifyEffectPoint(ieDword opcode, ieDword x, ieDword y)
{
	for (auto& fx : effects) {
		MATCH_OPCODE()
		fx.Pos = Point(x, y);
		fx.Parameter3 = 0;
		return;
	}
}

//count effects that get saved
ieDword EffectQueue::GetSavedEffectsCount() const
{
	ieDword cnt = 0;

	for (const auto& fx : effects) {
		if (fx.Persistent()) cnt++;
	}
	return cnt;
}

int EffectQueue::ResolveEffect(EffectRef& effectReference)
{
	Globals::ResolveEffectRef(effectReference);
	return effectReference.opcode;
}

// this check goes for the whole effect block, not individual effects
// But it takes the first effect of the block for the common fields

//returns 1 if effect block applicable
//returns 0 if effect block disabled
//returns -1 if effect block bounced
int EffectQueue::CheckImmunity(Actor* target) const
{
	//don't resist if target is non living
	if (!target) {
		return 1;
	}

	if (effects.empty()) {
		return 0;
	}

	// projectile immunity
	const Effect& fx = *effects.begin();
	if (target->ImmuneToProjectile(fx.Projectile)) {
		return 0;
	}

	// Allegedly, the book of infinite spells needed this, but irresistible by level
	// spells got fx->Power = 0, so i added those exceptions and removed returning here for fx->InventorySlot

	// check level resistances
	// check specific spell immunity
	// check school/sectype immunity
	int ret = check_type(target, fx);
	if (ret < 0 && target->Modified[IE_SANCTUARY] & (1 << OV_BOUNCE)) {
		target->Modified[IE_SANCTUARY] |= 1 << OV_BOUNCE2;
	}
	return ret;
}

void EffectQueue::AffectAllInRange(const Map* map, const Point& pos, int idstype, int idsvalue,
				   unsigned int range, const Actor* except)
{
	int cnt = map->GetActorCount(true);
	while (cnt--) {
		Actor* actor = map->GetActor(cnt, true);
		if (except == actor) {
			continue;
		}
		//distance
		if (!WithinRange(actor, pos, range)) {
			continue;
		}
		//ids targeting
		if (!match_ids(actor, idstype, idsvalue)) {
			continue;
		}
		//line of sight
		if (!map->IsVisibleLOS(actor->SMPos, SearchmapPoint(pos), actor)) {
			continue;
		}
		AddAllEffects(actor, actor->Pos);
	}
}

bool EffectQueue::OverrideTarget(const Effect* fx)
{
	if (!fx) return false;

	const auto& Opcodes = Globals::Get().Opcodes;
	return (Opcodes[fx->Opcode].Flags & EFFECT_PRESET_TARGET);
}

bool EffectQueue::HasHostileEffects() const
{
	bool hostile = false;

	for (const Effect& fx : effects) {
		if (fx.SourceFlags & SF_HOSTILE) {
			hostile = true;
			break;
		}
	}

	return hostile;
}

// iwd got a weird targeting system
// the opcode parameters are:
//   * param1 - optional value used only rarely
//   * param2 - a specific condition mostly based on target's stats
//     this is partly superior, partly inferior to the bioware ids targeting:
//       * superior because it can handle other stats and conditions
//       * inferior because it is not so readily moddable
//   * stat is usually a stat, but for special conditions it is a
//     function code (>=0x100).
//     If value is -1, then GemRB will use Param1, otherwise it is
//     compared to the target's stat using the relation function.
// The relation function is exactly the same as the extended
// diffmode for gemrb. (Thus scripts can use the very same relation
// functions).
// The hardcoded conditions are simulated via the IWDIDSEntry
// structure.

// returns true if the target matches iwd ids targeting
// usually this is used to restrict an effect to specific targets
bool EffectQueue::CheckIWDTargeting(const Scriptable* Owner, Actor* target, ieDword value, ieDword type, Effect* fx)
{
	const IWDIDSEntry& entry = gamedata->GetSpellProt(type);
	ieDword idx = entry.stat;
	ieDword val = entry.value;
	ieDword rel = entry.relation;
	const Actor* OwnerActor;
	if (idx == STI_INVALID) {
		// bad entry, don't match
		return false;
	}

	//if IDS value is 'anything' then the supplied value is in Parameter1
	if (val == 0xffffffff) {
		val = value;
	}
	switch (idx) {
		case STI_EA_RELATION:
			return DiffCore(EARelation(Owner, target), val, rel);
		case STI_ALLIES:
			return DiffCore(EARelation(Owner, target), EAR_FRIEND, rel);
		case STI_ENEMIES:
			return DiffCore(EARelation(Owner, target), EAR_HOSTILE, rel);
		case STI_DAYTIME:
			ieDword timeofday;
			timeofday = core->Time.GetHour(core->GetGame()->GameTime);
			// handle the clock jumping at midnight
			if (val > rel) {
				return timeofday >= val || timeofday <= rel;
			} else {
				return timeofday >= val && timeofday <= rel;
			}
		case STI_AREATYPE:
			Map* area;
			area = target->GetCurrentArea();
			return area && DiffCore((ieDword) area->AreaType, val, rel);
		case STI_MORAL_ALIGNMENT:
			OwnerActor = Scriptable::As<Actor>(Owner);
			if (OwnerActor) {
				return DiffCore(OwnerActor->GetStat(IE_ALIGNMENT) & AL_GE_MASK, STAT_GET(IE_ALIGNMENT) & AL_GE_MASK, rel);
			} else {
				return DiffCore(AL_TRUE_NEUTRAL, STAT_GET(IE_ALIGNMENT) & AL_GE_MASK, rel);
			}
		case STI_TWO_ROWS:
			//used in checks where any of two matches are ok (golem or undead etc)
			return CheckIWDTargeting(Owner, target, value, rel, fx) ||
				CheckIWDTargeting(Owner, target, value, val, fx);
		case STI_NOT_TWO_ROWS:
			//this should be the opposite as above
			return !(CheckIWDTargeting(Owner, target, value, rel, fx) ||
				 CheckIWDTargeting(Owner, target, value, val, fx));
		case STI_SOURCE_TARGET:
			return Owner == target;
		case STI_SOURCE_NOT_TARGET:
			return Owner != target;
		case STI_CIRCLESIZE:
			return DiffCore((ieDword) target->GetAnims()->GetCircleSize(), val, rel);
		case STI_SPELLSTATE:
			// only used with 1 and 5, so we don't need another accessor
			if (rel == EQUALS) {
				return target->HasSpellState(val);
			} else {
				return !target->HasSpellState(val);
			}
		case STI_SUMMONED_NUM:
			ieDword count;
			count = target->GetCurrentArea()->CountSummons(GA_NO_DEAD, SEX_SUMMON);
			return DiffCore(count, val, rel);
		case STI_CHAPTER_CHECK:
			{
				ieDword chapter = core->GetGame()->GetGlobal("CHAPTER", 0);
				return DiffCore(chapter, val, rel);
			}
		case STI_EVASION:
			if (core->HasFeature(GFFlags::ENHANCED_EFFECTS)) {
				// NOTE: no idea if this is used in iwd2 too (00misc32 has it set)
				// FIXME: check for evasion itself
				if (target->GetThiefLevel() < 2 && target->GetMonkLevel() < 1) {
					return false;
				}
				val = target->GetSavingThrow(4, 0, fx); // reflex
			} else {
				if (target->GetThiefLevel() < 7) {
					return false;
				}
				val = target->GetSavingThrow(1, 0, fx); // breath
			}

			return val;
		case STI_WATERY:
			// NOTE: this got reused in EEs for alignment matching, while the originals were about water.
			// Luckily the relation is unset in the later and the default 0 doesn't make sense for alignment
			if (rel == 0) {
				// hardcoded via animation id, so we can't use STI_TWO_ROWS
				// sahuagin x2, water elementals x2 (and water weirds)
				ieDword animID = target->GetSafeStat(IE_ANIMATION_ID);
				int ret = !val;
				if (animID == 0xf40b || animID == 0xf41b || animID == 0xe238 || animID == 0xe298 || animID == 0xe252) {
					ret = val;
				}
				return ret;
			} else {
				// alignment.ids check - see below
				idx = IE_ALIGNMENT;
			}
			// fall through
		case STI_EA:
		case STI_GENERAL:
		case STI_RACE:
		case STI_CLASS:
		case STI_SPECIFIC:
		case STI_GENDER:
		case STI_STATE:
		default:
			if (idx >= STI_EA && idx <= STI_STATE) {
				// the 0 will never be hit, since that is for STI_SUMMONED_NUM
				static const size_t fake2real[] = { IE_EA, IE_GENERAL, IE_RACE, IE_CLASS, IE_SPECIFIC, IE_SEX, 0, IE_STATE_ID };
				idx = fake2real[idx - STI_EA];
			}
			ieDword stat;
			stat = STAT_GET(idx);
			if (idx == IE_SUBRACE) {
				//subraces are not stand alone stats, actually, this hack should affect the CheckStat action too
				stat |= STAT_GET(IE_RACE) << 16;
			} else if (idx == IE_ALIGNMENT) {
				//alignment checks can be for good vs. evil, or chaotic vs. lawful, or both
				ieDword almask = 0;
				if (val & AL_GE_MASK) {
					almask |= AL_GE_MASK;
				}
				if (val & AL_LC_MASK) {
					almask |= AL_LC_MASK;
				}
				stat &= almask;
			}
			return DiffCore(stat, val, rel);
	}
}

}
