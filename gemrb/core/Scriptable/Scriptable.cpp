/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003-2005 The GemRB Project
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
 */

#include "Scriptable/Scriptable.h"

#include "strrefs.h"
#include "voodooconst.h"

#include "DialogHandler.h"
#include "DisplayMessage.h"
#include "Game.h"
#include "GameData.h"
#include "Interface.h"
#include "Projectile.h"
#include "Spell.h"
#include "Sprite2D.h"
#include "Video/Video.h"
#include "GameScript/GSUtils.h"
#include "GameScript/Matching.h" // MatchActor
#include "GUI/GameControl.h"
#include "GUI/TextSystem/Font.h"
#include "RNG.h"
#include "Scriptable/InfoPoint.h"
 
#include <utility>

namespace GemRB {

// we start this at a non-zero value to make debugging easier
static ieDword globalActorCounter = 10000;
static bool startActive = false;
static bool third = false;
static bool pst_flags = false;
static const unsigned short ClearActionsID = 133; // same for all games

/***********************
 *  Scriptable Class   *
 ***********************/
Scriptable::Scriptable(ScriptableType type)
{
	startActive = core->HasFeature(GFFlags::START_ACTIVE);
	third = core->HasFeature(GFFlags::RULES_3ED);
	pst_flags = core->HasFeature(GFFlags::PST_STATE_FLAGS);

	globalID = ++globalActorCounter;
	if (globalActorCounter == 0) {
		error("Scriptable", "GlobalID overflowed, quitting due to too many actors.");
	}

	Type = type;
	if (Type == ST_ACTOR) {
		InternalFlags = IF_VISIBLE | IF_USEDSAVE;
		if (startActive) {
			InternalFlags |= IF_ACTIVE;
		}
	} else {
		InternalFlags = IF_ACTIVE | IF_VISIBLE | IF_NOINT;
	}

	ResetCastingState(nullptr);
	ClearTriggers();
	AddTrigger(TriggerEntry(trigger_oncreation));
}

Scriptable::~Scriptable(void)
{
	if (CurrentAction) {
		ReleaseCurrentAction();
	}
	ClearActions();
	for (auto& script : Scripts) {
		delete script;
	}
}

ieDword Scriptable::GetLocal(const ieVariable& key, ieDword fallback) const {
	auto lookup = locals.find(key);
	if (lookup != locals.cend()) {
		return lookup->second;
	}

	return fallback;
}

void Scriptable::DumpLocals() const {
	Log(DEBUG, "Scriptable", "Locals item count: {}", locals.size());
	for (const auto& entry : locals) {
		Log(DEBUG, "Scriptable", "{} = {}", entry.first, entry.second);
	}
}

void Scriptable::SetScriptName(const ieVariable& text)
{
	// recreate to remove internal spaces
	scriptName = MakeVariable(text);
}

/** Gets the DeathVariable */
const ieVariable& Scriptable::GetScriptName(void) const
{
	return scriptName;
}

void Scriptable::SetDialog(const ResRef &resref) {
	if (gamedata->Exists(resref, IE_DLG_CLASS_ID) ) {
		Dialog = resref;
	}
}

Map* Scriptable::GetCurrentArea() const
{
	//this could be NULL, always check it
	return area;
}

void Scriptable::SetMap(Map *map)
{
	if (map && (map->GetCurrentArea()!=map)) {
		//a map always points to itself (if it is a real map)
		error("Scriptable", "Invalid map set!");
	}
	area = map;
}

//ai is nonzero if this is an actor currently in the party
//if the script level is AI_SCRIPT_LEVEL, then we need to
//load an AI script (.bs) instead of (.bcs)
void Scriptable::SetScript(const ResRef &aScript, int idx, bool ai)
{
	if (idx >= MAX_SCRIPTS) {
		error("Scriptable", "Invalid script index!");
	}
	if (Scripts[idx] && Scripts[idx]->running) {
		Scripts[idx]->dead = true;
	} else {
		delete Scripts[idx];
	}
	Scripts[idx] = NULL;
	// NONE is an 'invalid' script name, seldomly used to reset the slot, which we do above
	// This check is to prevent flooding of the console
	if (!aScript.IsEmpty() && aScript != "NONE") {
		if (idx!=AI_SCRIPT_LEVEL) ai = false;
		Scripts[idx] = new GameScript(aScript, this, idx, ai);
	}
}

void Scriptable::SetSpellResRef(const ResRef& resref) {
	SpellResRef = resref;
}

Region Scriptable::DrawingRegion() const
{
	return BBox;
}

void Scriptable::ImmediateEvent()
{
	InternalFlags |= IF_FORCEUPDATE;
}

bool Scriptable::IsPC() const
{
	if (Type != ST_ACTOR) return false;
	return ((const Actor *) this)->GetStat(IE_EA) <= EA_CHARMED;
}

void Scriptable::Update()
{
	Ticks++;
	AdjustedTicks++;
	if (AuraCooldown) AuraCooldown--;

	if (UnselectableTimer) {
		UnselectableTimer--;
		if (!UnselectableTimer && Type == ST_ACTOR) {
			Actor *actor = (Actor *) this;
			actor->SetCircleSize();
			if (actor->InParty) {
				core->GetGame()->SelectActor(actor, true, SELECT_QUIET);
				core->SetEventFlag(EF_PORTRAIT);
			}
		}
	}

	TickScripting();

	ProcessActions();

	InterruptCasting = false;
}

void Scriptable::TickScripting()
{
	// Stagger script updates.
	if (Ticks % 16 != globalID % 16) {
		return;
	}

	ieDword actorState = 0;
	if (Type == ST_ACTOR) {
		actorState = static_cast<Actor*>(this)->Modified[IE_STATE_ID];
	}

	// Dead actors only get one chance to run a new script.
	if ((InternalFlags & (IF_REALLYDIED | IF_JUSTDIED)) == IF_REALLYDIED) {
		return;
	}

	ScriptTicks++;

	// If no action is running, we've had triggers set recently or we haven't checked recently, do a script update.
	bool needsUpdate = (!CurrentAction) || (TriggerCountdown > 0) || (IdleTicks > 15);

	// Also do a script update if one was forced..
	if (InternalFlags & IF_FORCEUPDATE) {
		needsUpdate = true;
		InternalFlags &= ~IF_FORCEUPDATE;
	}
	// also force it for on-screen actors
	Region vp = core->GetGameControl()->Viewport();
	if (!needsUpdate && vp.PointInside(Pos)) {
		needsUpdate = true;
	}

	// Charmed actors don't get frequent updates.
	if ((actorState & STATE_CHARMED) && IdleTicks < 5) {
		needsUpdate = false;
	}

	if (!needsUpdate) {
		IdleTicks++;
		return;
	}

	if (!triggers.empty()) {
		TriggerCountdown = 5;
	}
	IdleTicks = 0;
	InternalFlags &= ~IF_JUSTDIED;
	if (TriggerCountdown > 0) {
		TriggerCountdown--;
	}

	ExecuteScript(MAX_SCRIPTS);
}

void Scriptable::ExecuteScript(int scriptCount)
{
	const GameControl *gc = core->GetGameControl();

	// area scripts still run for at least the current area, in bg1 (see ar2631, confirmed by testing)
	// but not in bg2 (kill Abazigal in ar6005)
	if (gc->GetScreenFlags() & SF_CUTSCENE) {
		if (! (core->HasFeature(GFFlags::CUTSCENE_AREASCRIPTS) && Type == ST_AREA)) {
			return;
		}
	}

	// Don't abort if there is a running non-interruptable action.
	if ((InternalFlags & IF_NOINT) && (CurrentAction || GetNextAction())) {
		return;
	}
	if (!CurrentActionInterruptable) {
		// sanity check
		if (!CurrentAction && !GetNextAction()) {
			error("Scriptable", "No current action and no next action.");
		}
		return;
	}
	// don't abort ActionOverride spawned actions
	if (CurrentAction && CurrentAction->flags & ACF_OVERRIDE) {
		return;
	}

	bool changed = false;
	Actor* act = Scriptable::As<Actor>(this);

	// don't run if the final dialog action queue is still playing out (we're already out of dialog!)
	// currently limited with GFFlags::CUTSCENE_AREASCRIPTS and to area scripts, to minimize risk into known test cases
	if (Type == ST_AREA && !core->HasFeature(GFFlags::CUTSCENE_AREASCRIPTS) && gc->GetDialogueFlags() & DF_POSTPONE_SCRIPTS) {
		return;
	}

	// don't run scripts if we're in dialog, regardless of DF_FREEZE_SCRIPTS
	if (gc->InDialog() && gc->dialoghandler->InDialog(this) &&
		(!act || act->Modified[IE_IGNOREDIALOGPAUSE] == 0)) {
		return;
	}

	if (act) {
		// if party AI is disabled, don't run non-override scripts
		if (act->InParty && !(core->GetGame()->ControlStatus & CS_PARTY_AI))
			scriptCount = 1;
		// hardcoded action overrides like charm, confusion, panic and berserking
		// TODO: check why everything else but charm is handled separately and unify if possible
		changed |= act->OverrideActions();
	}

	bool continuing = false, done = false;
	for (scriptlevel = 0;scriptlevel<scriptCount;scriptlevel++) {
		GameScript *Script = Scripts[scriptlevel];
		if (Script) {
			changed |= Script->Update(&continuing, &done);
			if (Script->dead) {
				delete Script;
			}
		}

		/* scripts are not concurrent, see WAITPC override script for example */
		if (done) break;
	}

	if (changed) {
		ClearTriggers();
	}

	if (act) {
		// if nothing is happening, look around, check if we're bored and so on
		act->IdleActions(CurrentAction!=NULL);
	}
}

void Scriptable::AddAction(Action* aC)
{
	if (!aC) {
		Log(WARNING, "Scriptable", "AA: NULL action encountered for {}!", scriptName);
		return;
	}

	InternalFlags|=IF_ACTIVE;
	if (startActive) {
		InternalFlags &= ~IF_IDLE;
	}
	aC->IncRef();
	if (actionflags[aC->actionID] & AF_SCRIPTLEVEL) {
		aC->int0Parameter = scriptlevel;
	}

	// attempt to handle 'instant' actions, from instant.ids, which run immediately
	// when added if the action queue is empty, even on actors which are Held/etc
	// but try to ignore iwd2 ActionOverride for 41pstail.bcs
	// FIXME: area check hack until fuzzie fixes scripts here
	const Action *nextAction = GetNextAction();
	bool ignoreQueue = !nextAction || (third && nextAction->objects[0]);
	if (!CurrentAction && ignoreQueue && area) {
		int instant = AF_SCR_INSTANT;
		if (core->GetGameControl()->InDialog()) {
			instant = AF_DLG_INSTANT;
		}
		if (actionflags[aC->actionID] & instant) {
			CurrentAction = aC;
			GameScript::ExecuteAction( this, CurrentAction );
			return;
		}
	}

	actionQueue.push_back( aC );
}

void Scriptable::AddActionInFront(Action* aC)
{
	if (!aC) {
		Log(WARNING, "Scriptable", "AAIF: null action encountered for {}!", scriptName);
		return;
	}
	InternalFlags|=IF_ACTIVE;
	actionQueue.push_front( aC );
	aC->IncRef();
}

Action* Scriptable::GetNextAction() const
{
	if (actionQueue.empty()) return nullptr;
	return actionQueue.front();
}

Action* Scriptable::PopNextAction()
{
	if (actionQueue.empty()) return nullptr;

	Action* aC = actionQueue.front();
	actionQueue.pop_front();
	return aC;
}

// clear all actions, unless some are marked to be preserved
void Scriptable::ClearActions(int skipFlags)
{
	// pst sometimes uses clearactions in the middle of a cutscene (eg. 1203cd21)
	// and expect it to clear only the previous actions, not the whole queue
	bool savedCurrentAction = false;
	if (pst_flags && CurrentAction && CurrentAction->actionID == ClearActionsID) {
		ReleaseCurrentAction();
	} else {
		if (skipFlags == 1 && CurrentAction && CurrentAction->flags & ACF_OVERRIDE) {
			savedCurrentAction = true;
		} else if (skipFlags == 2 && CurrentAction && actionflags[CurrentAction->actionID] & AF_IWD2_OVERRIDE) {
			savedCurrentAction = true;
		} else if (skipFlags == 3 && (CurrentActionInterruptable == false || InternalFlags & IF_NOINT)) {
			savedCurrentAction = true;
		} else {
			ReleaseCurrentAction();
		}

		for (unsigned int i = 0; i < actionQueue.size(); i++) {
			Action* aC = actionQueue.front();
			if (skipFlags == 1 && aC->flags & ACF_OVERRIDE) continue;
			if (skipFlags == 2 && actionflags[aC->actionID] & AF_IWD2_OVERRIDE) continue;
			if (skipFlags == 3 && aC == CurrentAction && savedCurrentAction) continue;

			actionQueue.pop_front();
			aC->Release();
		}
	}
	if (savedCurrentAction) return;

	WaitCounter = 0;
	LastTarget = 0;
	LastTargetPos.Invalidate();
	// intentionally not resetting LastTargetPersistent
	LastSpellTarget = 0;

	if (Type == ST_ACTOR) {
		Interrupt();
	} else {
		NoInterrupt();
	}
}

void Scriptable::Stop(int flags)
{
	ClearActions(flags);
}

void Scriptable::ReleaseCurrentAction()
{
	if (CurrentAction) {
		CurrentAction->Release();
		CurrentAction = NULL;
	}

	CurrentActionState = 0;
	CurrentActionTarget = 0;
	CurrentActionInterruptable = true;
	CurrentActionTicks = 0;
}

void Scriptable::ProcessActions()
{
	if (WaitCounter) {
		WaitCounter--;
		if (WaitCounter) return;
	}

	int lastAction = -1;
	while (true) {
		CurrentActionInterruptable = true;
		if (!CurrentAction) {
			if (! (CurrentActionTicks == 0 && CurrentActionState == 0)) {
				Log(ERROR, "Scriptable", "Last action: {}", lastAction);
			}
			assert(CurrentActionTicks == 0 && CurrentActionState == 0);
			CurrentAction = PopNextAction();
		} else {
			CurrentActionTicks++;
		}
		if (!CurrentAction) {
			ClearActions();
			// clear lastAction here if you'll ever need it after exiting the loop
			break;
		}
		lastAction = CurrentAction->actionID;
		GameScript::ExecuteAction( this, CurrentAction );
		//break execution in case of a Wait flag
		if (WaitCounter) {
			break;
		}
		//break execution in case of blocking action
		if (CurrentAction) {
			break;
		}
		//break execution in case of movement
		//we should not actually break here, or else fix waypoints
		if (InMove()) {
			break;
		}
	}
	// FIXME
	/*if (InternalFlags&IF_IDLE) {
		Deactivate();
	}*/
}

bool Scriptable::InMove() const
{
	if (Type!=ST_ACTOR) {
		return false;
	}
	Movable *me = (Movable *) this;
	return me->GetStep() != NULL;
}

void Scriptable::SetWait(tick_t time)
{
	WaitCounter = time;
}

tick_t Scriptable::GetWait() const
{
	return WaitCounter;
}

void Scriptable::LeftDialog()
{
	AddTrigger(TriggerEntry(trigger_wasindialog));
}

void Scriptable::Hide()
{
	InternalFlags &= ~IF_VISIBLE;
}

void Scriptable::Unhide()
{
	InternalFlags |= IF_VISIBLE;
}

void Scriptable::Interrupt()
{
	InternalFlags &= ~IF_NOINT;
}

void Scriptable::NoInterrupt()
{
	InternalFlags |= IF_NOINT;
}

//also turning off the idle flag so it won't run continuously
void Scriptable::Deactivate()
{
	InternalFlags &=~(IF_ACTIVE|IF_IDLE);
}

//turning off the not interruptable flag, actions should reenable it themselves
//also turning off the idle flag
//heh, no, i wonder why did i touch the interruptable flag here
void Scriptable::Activate()
{
	InternalFlags |= IF_ACTIVE;
	InternalFlags &= ~IF_IDLE;
}

void Scriptable::PartyRested()
{
	//InternalFlags |=IF_PARTYRESTED;
	AddTrigger(TriggerEntry(trigger_partyrested));
}

ieDword Scriptable::GetInternalFlag() const
{
	return InternalFlags;
}

void Scriptable::SetInternalFlag(unsigned int value, BitOp mode)
{
	SetBits(InternalFlags, value, mode);
}

void Scriptable::ClearTriggers()
{
	triggers.clear();
}

void Scriptable::AddTrigger(TriggerEntry trigger)
{
	triggers.push_back(trigger);
	ImmediateEvent();
	SetLastTrigger(trigger.triggerID, trigger.param1);
}

// plenty of triggers in svitrobj don't send trigger messages and so never see the code in AddTrigger
void Scriptable::SetLastTrigger(ieDword triggerID, ieDword scriptableID)
{
	assert(triggerID < MAX_TRIGGERS);
	if (triggerflags[triggerID] & TF_SAVED) {
		//TODO: if LastTrigger is still overwritten by script action blocks, store this in a separate field and copy it back when the block ends
		ieVariable name = "none";
		if (area) {
			const Scriptable* scr = area->GetScriptableByGlobalID(scriptableID);
			if (scr) {
				name = scr->GetScriptName();
			}
		}
		ScriptDebugLog(DebugMode::TRIGGERS, "Scriptable", "{}: Added LastTrigger: {} ({}) for trigger {}", scriptName, scriptableID, name, triggerID);
		LastTrigger = scriptableID;
	}
}

bool Scriptable::MatchTrigger(unsigned short id, ieDword param) const
{
	for (const auto& trigger : triggers) {
		if (trigger.triggerID != id)
			continue;
		if (param && trigger.param1 != param)
			continue;
		return true;
	}

	return false;
}

bool Scriptable::MatchTriggerWithObject(unsigned short id, const Object *obj, ieDword param) const
{
	for (auto& trigger : triggers) {
		if (trigger.triggerID != id) continue;
		if (param && trigger.param2 != param) continue;
		if (!MatchActor(this, trigger.param1, obj)) continue;
		return true;
	}

	return false;
}

const TriggerEntry *Scriptable::GetMatchingTrigger(unsigned short id, unsigned int notflags) const
{
	for (auto& trigger : triggers) {
		if (trigger.triggerID != id) continue;
		if (notflags & trigger.flags) continue;
		return &trigger;
	}

	return NULL;
}

// handle wild surge projectile modifiers
void Scriptable::ModifyProjectile(Projectile* &pro, Spell* spl, ieDword tgt, int level)
{
	Actor* caster = Scriptable::As<Actor>(this);
	assert(caster);

	int count;
	const Actor *newact = nullptr;
	SPLExtHeader* seh = nullptr;
	// check for target (type) change
	switch (caster->wildSurgeMods.target_change_type) {
		case WSTC_SETTYPE:
			seh = &spl->ext_headers[SpellHeader];
			for (Effect& feature : seh->features) {
				feature.Target = caster->wildSurgeMods.target_type;
			}
			// we need to fetch the projectile, so the effect queue is created
			// (skipped above)
			delete pro;
			pro = spl->GetProjectile(this, SpellHeader, level, LastTargetPos);
			pro->SetCaster(GetGlobalID(), level);
			break;
		case WSTC_ADDTYPE:
			// caster gets selftargeting fx when the projectile is fetched above
			seh = &spl->ext_headers[SpellHeader];
			for (Effect& feature : seh->features) {
				if (feature.Target == FX_TARGET_SELF) {
					feature.Target = caster->wildSurgeMods.target_type;
				} else {
					// also apply to the caster
					core->ApplyEffect(new Effect(feature), caster, caster);
				}
			}
			// we need to refetch the projectile, so the effect queue is created
			delete pro; // don't leak the original one
			pro = spl->GetProjectile(this, SpellHeader, level, LastTargetPos);
			pro->SetCaster(GetGlobalID(), level);
			break;
		case WSTC_RANDOMIZE:
			count = area->GetActorCount(false);
			newact = area->GetActor(core->Roll(1, count, -1), false);
			if (count > 1 && newact == caster) {
				while (newact == caster) {
					newact = area->GetActor(core->Roll(1, count, -1), false);
				}
			}
			if (tgt) {
				LastSpellTarget = newact->GetGlobalID();
				LastTargetPos = newact->Pos;
			} else {
				// no better idea; I wonder if the original randomized point targets at all
				LastTargetPos = newact->Pos;
			}

			// make it also work for self-targeting spells:
			// change the payload or this was all in vain
			seh = &spl->ext_headers[SpellHeader];
			for (Effect& feature : seh->features) {
				if (feature.Target == FX_TARGET_SELF) {
					feature.Target = FX_TARGET_PRESET;
				}
			}
			// we need to fetch the projectile, so the effect queue is created
			// (skipped above)
			delete pro;
			pro = spl->GetProjectile(this, SpellHeader, level, LastTargetPos);
			pro->SetCaster(GetGlobalID(), level);
			break;
		default: //0 - do nothing
			break;
	}

	// apply the saving throw mod
	if (caster->wildSurgeMods.saving_throw_mod) {
		seh = &spl->ext_headers[SpellHeader];
		for (Effect& feature : seh->features) {
			feature.SavingThrowBonus += caster->wildSurgeMods.saving_throw_mod;
		}
	}

	// change the projectile
	if (caster->wildSurgeMods.projectile_id) {
		spl->ext_headers[SpellHeader].ProjectileAnimation = caster->wildSurgeMods.projectile_id;
		// make it also work for self-targeting spells:
		// change the payload or this was all in vain
		seh = &spl->ext_headers[SpellHeader];
		for (Effect& feature : seh->features) {
			if (feature.Target == FX_TARGET_SELF) {
				feature.Target = FX_TARGET_PRESET;
			}
		}
		// we need to refetch the projectile, so the new one is used
		delete pro; // don't leak the original one
		pro = spl->GetProjectile(this, SpellHeader, level, LastTargetPos);
		pro->SetCaster(GetGlobalID(), level);
	}

	// check for the speed mod
	if (caster->wildSurgeMods.projectile_speed_mod) {
		pro->Speed = (pro->Speed * caster->wildSurgeMods.projectile_speed_mod) / 100;
		if (!pro->Speed) {
			pro->Speed = 1;
		}
	}
}

void Scriptable::CreateProjectile(const ResRef& spellResRef, ieDword tgt, int level, bool fake)
{
	Spell* spl = gamedata->GetSpell(spellResRef);
	Actor* caster = Scriptable::As<Actor>(this);

	//PST has a weird effect, called Enoll Eva's duplication
	//it creates every projectile of the affected actor twice
	int projectileCount = 1;
	if (caster) {
		if (spl->Flags & (SF_HOSTILE | SF_BREAK_SANCTUARY)) {
			caster->CureSanctuary();
		}

		// check if a wild surge ordered us to replicate the projectile
		projectileCount = std::max(1U, caster->wildSurgeMods.num_castings);

		if (pst_flags && caster->GetStat(IE_STATE_ID) & STATE_EE_DUPL) {
			// seriously, wild surges and EE in the same game?
			// anyway, it would be too many duplications
			projectileCount = 2;
		}
	}

	while(projectileCount --) {
		Projectile *pro = NULL;
		// jump through hoops to skip applying selftargeting spells to the caster
		// if we'll be changing the target
		int tct = 0;
		if (caster) {
			tct = caster->wildSurgeMods.target_change_type;
		}
		if (!caster || !tct || tct == WSTC_ADDTYPE || !caster->wildSurgeMods.projectile_id) {
			pro = spl->GetProjectile(this, SpellHeader, level, LastTargetPos);
			if (!pro) {
				return;
			}
			pro->SetCaster(GetGlobalID(), level);
		}

		Point origin = Pos;
		if (Type == ST_TRIGGER || Type == ST_PROXIMITY) {
			// try and make projectiles start from the right trap position
			// see the traps in the duergar/assassin battle in bg2 dungeon
			// see also function below - maybe we should fix Pos instead
			origin = ((InfoPoint *)this)->TrapLaunch;
		}

		if (caster) {
			ModifyProjectile(pro, spl, tgt, level);
		}

		if (tgt) {
			area->AddProjectile(pro, origin, tgt, fake);
		} else {
			area->AddProjectile(pro, origin, LastTargetPos);
		}
	}

	ieDword spellnum = ResolveSpellNumber(spellResRef);
	if (spellnum!=0xffffffff) {
		area->SeeSpellCast(this, spellnum);

		// spellcasting feedback
		// iwd2: only display it for party friendly creatures - enemies require a successful spellcraft check
		if (!third || (caster && caster->GetStat(IE_EA) <= EA_CONTROLLABLE)) {
			DisplaySpellCastMessage(tgt, spl);
		}
	}
	// only trigger the autopause when in combat or buffing gets very annoying
	if (core->GetGame()->CombatCounter && caster && caster->IsPartyMember()) {
		core->Autopause(AUTOPAUSE::SPELLCAST, this);
	}

	gamedata->FreeSpell(spl, spellResRef, false);
}

void Scriptable::DisplaySpellCastMessage(ieDword tgt, const Spell* spl) const
{
	if (!core->HasFeedback(FT_CASTING)) return;

	// caster - Casts spellname : target OR
	// caster - spellname : target (repeating spells)
	const Scriptable *target = nullptr;
	if (tgt) {
		target = area->GetActorByGlobalID(tgt);
		if (!target) {
			target=core->GetGame()->GetActorByGlobalID(tgt);
		}
	}

	const String& spell = core->GetString(spl->SpellName);
	if (!spell.empty() && Type == ST_ACTOR) {
		String str;
		if (target) {
			// no "Casts " prefix for special powers (the originals erroneously left the space in)
			if (spl->SpellType == IE_SPL_INNATE) {
				str = fmt::format(L"{} : {}", spell, target->GetName());
			} else {
				const String msg = core->GetString(DisplayMessage::GetStringReference(HCStrings::ActionCast), STRING_FLAGS::NONE);
				str = fmt::format(L"{} {} : {}", msg, spell, target->GetName());
			}
		} else {
			str = fmt::format(L"{} : {}", spell, GetName());
		}
		displaymsg->DisplayStringName(std::move(str), GUIColors::WHITE, this);
	}
}

// NOTE: currently includes the sender
void Scriptable::SendTriggerToAll(TriggerEntry entry)
{
	std::vector<Actor *> nearActors = area->GetAllActorsInRadius(Pos, GA_NO_DEAD|GA_NO_UNSCHEDULED, 15);
	for (const auto& neighbour : nearActors) {
		neighbour->AddTrigger(entry);
	}
	area->AddTrigger(entry);
}

inline void Scriptable::ResetCastingState(Actor *caster) {
	SpellHeader = -1;
	SpellResRef.Reset();
	LastTargetPos.Invalidate();
	LastSpellTarget = 0;
	if (caster) {
		memset(&(caster->wildSurgeMods), 0, sizeof(caster->wildSurgeMods));
	}
}

void Scriptable::CastSpellPointEnd(int level, bool keepStance)
{
	const Spell* spl = gamedata->GetSpell(SpellResRef); // this was checked before we got here
	if (!spl) {
		return;
	}
	int nSpellType = spl->SpellType;
	gamedata->FreeSpell(spl, SpellResRef, false);

	Actor* caster = Scriptable::As<Actor>(this);
	if (caster && !keepStance) {
		caster->SetStance(IE_ANI_CONJURE);
	}
	if (level == 0) {
		if (caster) {
			level = caster->GetCasterLevel(nSpellType);
		} else {
			//default caster level is 1
			level = 1;
		}
	}

	if (SpellHeader == -1) {
		LastTargetPos.Invalidate();
		return;
	}

	if (LastTargetPos.IsInvalid()) {
		SpellHeader = -1;
		return;
	}

	if (!SpellResRef[0]) {
		return;
	}
	if (!area) {
		Log(ERROR, "Scriptable", "CastSpellPointEnd: lost area, skipping {}!", SpellResRef);
		ResetCastingState(caster);
		return;
	}

	// a candidate for favourite spell? Not if we're forced cast (eg. from fx_cast_spell)
	if (caster && caster->PCStats && !keepStance) {
		caster->PCStats->RegisterFavourite(SpellResRef, FAV_SPELL);
	}

	if (!keepStance) {
		// yep, the original didn't use the casting channel for this!
		core->GetAudioDrv()->Play(spl->CompletionSound, SFX_CHAN_MISSILE, Pos);
	}

	CreateProjectile(SpellResRef, 0, level, false);

	// the original engine saves lasttrigger only in case of SpellCast, so we have to differentiate
	// NOTE: unused in iwd2, so the fact that it has no stored spelltype is of no consequence
	ieDword spellID = ResolveSpellNumber(SpellResRef);
	switch (nSpellType) {
	case 1:
		SendTriggerToAll(TriggerEntry(trigger_spellcast, GetGlobalID(), spellID));
		break;
	case 2:
		SendTriggerToAll(TriggerEntry(trigger_spellcastpriest, GetGlobalID(), spellID));
		break;
	default:
		SendTriggerToAll(TriggerEntry(trigger_spellcastinnate, GetGlobalID(), spellID));
		break;
	}

	Actor *target = area->GetActor(LastTargetPos, GA_NO_UNSCHEDULED|GA_NO_HIDDEN);
	if (target) {
		target->AddTrigger(TriggerEntry(trigger_spellcastonme, GetGlobalID(), spellID));
		target->LastSpellOnMe = spellID;
	}

	ResetCastingState(caster);
}

void Scriptable::CastSpellEnd(int level, bool keepStance)
{
	const Spell* spl = gamedata->GetSpell(SpellResRef); // this was checked before we got here
	if (!spl) {
		return;
	}
	int nSpellType = spl->SpellType;
	gamedata->FreeSpell(spl, SpellResRef, false);

	Actor* caster = Scriptable::As<Actor>(this);
	if (caster && !keepStance) {
		caster->SetStance(IE_ANI_CONJURE);
	}
	if (level == 0) {
		if (caster) {
			level = caster->GetCasterLevel(nSpellType);
		} else {
			//default caster level is 1
			level = 1;
		}
	}

	if (SpellHeader == -1) {
		LastSpellTarget = 0;
		return;
	}
	if (!LastSpellTarget) {
		SpellHeader = -1;
		return;
	}
	if (!SpellResRef[0]) {
		return;
	}
	if (!area) {
		Log(ERROR, "Scriptable", "CastSpellEnd: lost area, skipping {}!", SpellResRef);
		ResetCastingState(caster);
		return;
	}

	if (caster && caster->PCStats && !keepStance) {
		caster->PCStats->RegisterFavourite(SpellResRef, FAV_SPELL);
	}

	if (!keepStance) {
		core->GetAudioDrv()->Play(spl->CompletionSound, SFX_CHAN_MISSILE, Pos);
	}

	//if the projectile doesn't need to follow the target, then use the target position
	CreateProjectile(SpellResRef, LastSpellTarget, level, GetSpellDistance(SpellResRef, this)==0xffffffff);

	// the original engine saves lasttrigger only in case of SpellCast, so we have to differentiate
	// NOTE: unused in iwd2, so the fact that it has no stored spelltype is of no consequence
	ieDword spellID = ResolveSpellNumber(SpellResRef);
	switch (nSpellType) {
	case 1:
		SendTriggerToAll(TriggerEntry(trigger_spellcast, GetGlobalID(), spellID));
		break;
	case 2:
		SendTriggerToAll(TriggerEntry(trigger_spellcastpriest, GetGlobalID(), spellID));
		break;
	default:
		SendTriggerToAll(TriggerEntry(trigger_spellcastinnate, GetGlobalID(), spellID));
		break;
	}

	Actor *target = area->GetActorByGlobalID(LastSpellTarget);
	if (target) {
		target->AddTrigger(TriggerEntry(trigger_spellcastonme, GetGlobalID(), spellID));
		target->LastSpellOnMe = spellID;
	}

	ResetCastingState(caster);
}

// check for input sanity and good casting conditions
int Scriptable::CanCast(const ResRef& SpellRef, bool verbose) {
	const Spell* spl = gamedata->GetSpell(SpellRef);
	if (!spl) {
		SpellHeader = -1;
		Log(ERROR, "Scriptable", "Spell not found, aborting cast!");
		return 0;
	}

	// check for area dead magic
	// tob AR3004 is a dead magic area, but using a script with personal dead magic
	if (area->GetInternalFlag()&AF_DEADMAGIC && !(spl->Flags&SF_HLA)) {
		displaymsg->DisplayConstantStringName(HCStrings::DeadmagicFail, GUIColors::WHITE, this);
		return 0;
	}

	if (spl->Flags&SF_NOT_INDOORS && !(area->AreaType&AT_OUTDOOR)) {
		displaymsg->DisplayConstantStringName(HCStrings::IndoorFail, GUIColors::WHITE, this);
		return 0;
	}

	// various individual checks
	if (Type != ST_ACTOR) {
		return 1;
	}
	const Actor *actor = (const Actor *) this;

	// check for silence
	// only a handful of spells don't have a verbal component -
	// the original hardcoded vocalize and a few more
	// we (also) ignore tobex modded spells
	if (actor->CheckSilenced()) {
		if (!(gamedata->GetSpecialSpell(spl->Name) & SpecialSpell::Silence) && !(spl->Flags & SF_IGNORES_SILENCE)) {
			Log(WARNING, "Scriptable", "Tried to cast while silenced!");
			return 0;
		}
	}

	// check for personal dead magic
	if (actor->Modified[IE_DEADMAGIC] && !(spl->Flags&SF_HLA)) {
		displaymsg->DisplayConstantStringName(HCStrings::DeadmagicFail, GUIColors::WHITE, this);
		return 0;
	}

	// check for miscast magic and similar
	ieDword roll = actor->LuckyRoll(1, 100, 0, 0);
	bool failed = false;
	ieDword chance = 0;
	switch(spl->SpellType)
	{
	case IE_SPL_PRIEST:
		chance = actor->GetSpellFailure(false);
		break;
	case IE_SPL_WIZARD:
		chance = actor->GetSpellFailure(true);
		break;
	case IE_SPL_INNATE:
		chance = actor->Modified[IE_SPELLFAILUREINNATE];
		break;
	}
	if (chance >= roll) {
		failed = true;
	}
	if (verbose && chance && third) {
		// ~Spell Failure check: Roll d100 %d vs. Spell failure chance %d~
		displaymsg->DisplayRollStringName(ieStrRef::ROLL21, GUIColors::LIGHTGREY, actor, roll, chance);
	}
	if (failed) {
		displaymsg->DisplayConstantStringName(HCStrings::MiscastMagic, GUIColors::WHITE, this);
		return 0;
	}

	// iwd2: make a concentration check if needed
	if (!actor->ConcentrationCheck()) {
		return 0;
	}

	return 1;
}

// checks if a party-friendly actor is nearby and if so, if it recognizes the spell
// this enemy just started casting
void Scriptable::SpellcraftCheck(const Actor *caster, const ResRef& spellRef)
{
	if (!third || !caster || caster->GetStat(IE_EA) <= EA_CONTROLLABLE || !area) {
		return;
	}

	const Spell* spl = gamedata->GetSpell(spellRef);
	assert(spl); // only a bad surge could make this fail and we want to catch it
	int AdjustedSpellLevel = spl->SpellLevel + 15;
	std::vector<Actor *> neighbours = area->GetAllActorsInRadius(caster->Pos, GA_NO_DEAD|GA_NO_ENEMY|GA_NO_SELF|GA_NO_UNSCHEDULED, caster->GetBase(IE_VISUALRANGE), this);
	for (const auto& detective : neighbours) {
		// disallow neutrals from helping the party
		if (detective->GetStat(IE_EA) > EA_CONTROLLABLE) {
			continue;
		}
		if (detective->GetSkill(IE_SPELLCRAFT) <= 0) {
			continue;
		}

		// ~Spellcraft check (d20 roll + Spellcraft level + int mod) %d vs. (spell level + 15)  = %d.   (Int mod = %d)~
		int Spellcraft = core->Roll(1, 20, 0) + detective->GetStat(IE_SPELLCRAFT);
		int IntMod = detective->GetAbilityBonus(IE_INT);

		if ((Spellcraft + IntMod) > AdjustedSpellLevel) {
			// eg. .:Casts Word of Recall:.
			const String castmsg = core->GetString(DisplayMessage::GetStringReference(HCStrings::Casts));
			const String spellname = core->GetString(spl->SpellName);
			overHead.SetText(fmt::format(L".:{} {}:.", castmsg, spellname));
			displaymsg->DisplayRollStringName(ieStrRef::ROLL15, GUIColors::LIGHTGREY, detective, Spellcraft+IntMod, AdjustedSpellLevel, IntMod);
			break;
		}
	}
	gamedata->FreeSpell(spl, spellRef, false);
}

// shortcut for internal use when there is no wait
// if any user needs casting time support, they should use Spell* actions directly
void Scriptable::DirectlyCastSpellPoint(const Point& target, const ResRef& spellRef, int level, bool keepStance, bool deplete)
{
	if (!gamedata->Exists(spellRef, IE_SPL_CLASS_ID)) {
		return;
	}

	// save and restore the casting targets, so we don't interrupt any gui triggered casts with spells like true seeing (repeated fx_cast_spell)
	Point TmpPos = LastTargetPos;
	ieDword TmpTarget = LastSpellTarget;
	int TmpHeader = SpellHeader;

	SetSpellResRef(spellRef);
	CastSpellPoint(target, deplete, true, true, level);
	CastSpellPointEnd(level, keepStance);

	LastTargetPos = TmpPos;
	LastSpellTarget = TmpTarget;
	SpellHeader = TmpHeader;
}

// shortcut for internal use
// if any user needs casting time support, they should use Spell* actions directly
void Scriptable::DirectlyCastSpell(Scriptable* target, const ResRef& spellRef, int level, bool keepStance, bool deplete)
{
	if (!gamedata->Exists(spellRef, IE_SPL_CLASS_ID)) {
		return;
	}

	// save and restore the casting targets, so we don't interrupt any gui triggered casts with spells like true seeing (repeated fx_cast_spell)
	Point TmpPos = LastTargetPos;
	ieDword TmpTarget = LastSpellTarget;
	int TmpHeader = SpellHeader;

	SetSpellResRef(spellRef);
	CastSpell(target, deplete, true, true, level);
	CastSpellEnd(level, keepStance);

	LastTargetPos = TmpPos;
	LastSpellTarget = TmpTarget;
	SpellHeader = TmpHeader;
}

//set target as point
//if spell needs to be depleted, do it
//if spell is illegal stop casting
int Scriptable::CastSpellPoint(const Point& target, bool deplete, bool instant, bool noInterrupt, int level)
{
	LastSpellTarget = 0;
	LastTargetPos.Invalidate();
	Actor* actor = Scriptable::As<Actor>(this);
	if (actor && actor->HandleCastingStance(SpellResRef, deplete, instant)) {
		Log(ERROR, "Scriptable", "Spell {} not known or memorized, aborting cast!", SpellResRef);
		return -1;
	}
	if (!instant && !noInterrupt) {
		AuraCooldown = core->Time.attack_round_size;
	}
	if(!noInterrupt && !CanCast(SpellResRef)) {
		SpellResRef.Reset();
		if (actor) {
			actor->SetStance(IE_ANI_READY);
		}
		return -1;
	}

	LastTargetPos = target;

	if(!CheckWildSurge()) {
		return -1;
	}
	if (!instant) {
		SpellcraftCheck(actor, SpellResRef);
		if (actor) actor->CureInvisibility();
	}
	return SpellCast(instant, nullptr, level);
}

//set target as actor (if target isn't actor, use its position)
//if spell needs to be depleted, do it
//if spell is illegal stop casting
int Scriptable::CastSpell(Scriptable* target, bool deplete, bool instant, bool noInterrupt, int level)
{
	LastSpellTarget = 0;
	LastTargetPos.Invalidate();
	Actor* actor = Scriptable::As<Actor>(this);
	if (actor && actor->HandleCastingStance(SpellResRef, deplete, instant)) {
		Log(ERROR, "Scriptable", "Spell {} not known or memorized, aborting cast!", SpellResRef);
		return -1;
	}

	assert(target);

	if (!instant && !noInterrupt) {
		AuraCooldown = core->Time.attack_round_size;
	}
	if(!noInterrupt && !CanCast(SpellResRef)) {
		SpellResRef.Reset();
		if (actor) {
			actor->SetStance(IE_ANI_READY);
		}
		return -1;
	}

	LastTargetPos = target->Pos;
	if (target->Type==ST_ACTOR) {
		LastSpellTarget = target->GetGlobalID();
	}

	if(!CheckWildSurge()) {
		return -1;
	}

	if (!instant) {
		SpellcraftCheck(actor, SpellResRef);
		// self-targetted spells that are not hostile maintain invisibility
		if (actor && target != this) actor->CureInvisibility();
	}
	return SpellCast(instant, target, level);
}

static EffectRef fx_force_surge_modifier_ref = { "ForceSurgeModifier", -1 };
static EffectRef fx_castingspeed_modifier_ref = { "CastingSpeedModifier", -1 };

//start spellcasting (common part)
int Scriptable::SpellCast(bool instant, Scriptable* target, int level)
{
	Spell* spl = gamedata->GetSpell(SpellResRef); // this was checked before we got here
	Actor* actor = Scriptable::As<Actor>(this);
	if (actor) {
		//The ext. index is here to calculate the casting time
		if (!level) level = actor->GetCasterLevel(spl->SpellType);
		SpellHeader = spl->GetHeaderIndexFromLevel(level);
	} else {
		SpellHeader = 0;
	}

	const SPLExtHeader *header = spl->GetExtHeader(SpellHeader);
	int casting_time = (int)header->CastingTime;
	// how does this work for non-actors exactly?
	if (actor) {
		// The mental speed effect can shorten or lengthen the casting time.
		// But first check if a special maximum is set
		const Effect *fx = actor->fxqueue.HasEffectWithParam(fx_castingspeed_modifier_ref, 2);
		int max = 1000;
		if (fx) {
			max = fx->Parameter1;
		}
		if (max < 10 && casting_time > max) {
			casting_time = max;
		} else {
			casting_time -= (int)actor->Modified[IE_MENTALSPEED];
		}
		casting_time = Clamp(casting_time, 0, 10);
	}

	// this is a guess which seems approximately right so far (same as in the bg2 manual, except that it may be a combat round instead)
	int duration = (casting_time*core->Time.round_size) / 10;
	if (instant) {
		duration = 0;
	}
	if (actor) {
		//cfb
		EffectQueue fxqueue;
		// casting glow is always on the caster
		if (!(actor->Modified[IE_AVATARREMOVAL] || instant)) {
			ieDword gender = actor->GetCGGender();
			fxqueue.SetOwner(actor);
			spl->AddCastingGlow(&fxqueue, duration, gender);
			fxqueue.AddAllEffects(actor, Point());
		}

		// actual cfb
		fxqueue = spl->GetEffectBlock(this, this->Pos, -1, level);
		fxqueue.SetOwner(actor);
		if (target && target->Type == ST_ACTOR) {
			fxqueue.AddAllEffects((Actor *)target, target->Pos);
		} else {
			fxqueue.AddAllEffects(actor, actor->Pos);
		}
		actor->WMLevelMod = 0;
		if (actor->Modified[IE_FORCESURGE] == 1) {
			// affects only the next spell cast, but since the timing is permanent,
			// we have to remove it manually
			actor->fxqueue.RemoveAllEffectsWithParam(fx_force_surge_modifier_ref, 1);
		}
		actor->ResetCommentTime();
	}

	gamedata->FreeSpell(spl, SpellResRef, false);
	return duration;
}

// Anyone with some wildness has 5% chance of getting a wild surge when casting,
// but most innates are excluded, due to being nonmagic.
// A d100 roll is made, some stat boni are added, then either:
// 1. the spell is cast normally (score of 100 or more)
// 2. one or more wild surges happen and something else is cast
// 2.1. this can loop, since some surges cause rerolls
static EffectRef fx_chaosshield_ref = { "ChaosShieldModifier", -1 };

int Scriptable::CheckWildSurge()
{
	//no need to check for 3rd ed rules, because surgemod or forcesurge need to be nonzero to get a surge
	if (Type != ST_ACTOR) {
		return 1;
	}
	if (core->InCutSceneMode()) {
		return 1;
	}

	Actor *caster = (Actor *) this;

	int roll = core->Roll(1, 100, 0);
	if ((roll <= 5 && caster->Modified[IE_SURGEMOD]) || caster->Modified[IE_FORCESURGE]) {
		ResRef oldSpellResRef;
		oldSpellResRef = SpellResRef;
		const Spell *spl = gamedata->GetSpell(oldSpellResRef); // this was checked before we got here
		// ignore non-magic "spells"
		if (spl->Flags&(SF_HLA|SF_TRIGGER)) {
			gamedata->FreeSpell(spl, oldSpellResRef, false);
			return 1;
		}

		int check = roll + caster->Modified[IE_SURGEMOD];
		if (caster->Modified[IE_FORCESURGE] != 7) {
			// skip the caster level bonus if we're already in a complicated surge
			check += caster->GetCasterLevel(spl->SpellType);
		}
		if (caster->Modified[IE_CHAOSSHIELD]) {
			//avert the surge and decrease the chaos shield counter
			check = 0;
			caster->fxqueue.DecreaseParam1OfEffect(fx_chaosshield_ref,1);
			displaymsg->DisplayConstantStringName(HCStrings::ChaosShield,GUIColors::LIGHTGREY,caster);
		}

		// hundred or more means a normal cast; same for negative values (for absurd antisurge modifiers)
		if ((check > 0) && (check < 100) ) {
			// display feedback: Wild Surge: bla bla
			// look up the spell in the "check" row of wildmag.2da
			const SurgeSpell& surgeSpell = gamedata->GetSurgeSpell(check - 1);
			const String s1 = core->GetString(DisplayMessage::GetStringReference(HCStrings::WildSurge), STRING_FLAGS::NONE);
			const String s2 = core->GetString(surgeSpell.message, STRING_FLAGS::NONE);
			displaymsg->DisplayStringName(s1 + L" " + s2, GUIColors::WHITE, this);

			if (!gamedata->Exists(surgeSpell.spell, IE_SPL_CLASS_ID)) {
				// handle the hardcoded cases - they'll also fail here
				if (!HandleHardcodedSurge(surgeSpell.spell, spl, caster)) {
					//free the spell handle because we need to return
					gamedata->FreeSpell(spl, oldSpellResRef, false);
					return 0;
				}
			} else {
				// finally change the spell
				// the hardcoded bunch does it on its own when needed
				SpellResRef = surgeSpell.spell;
			}
		}

		//free the spell handle
		gamedata->FreeSpell(spl, oldSpellResRef, false);
	}

	return 1;
}

bool Scriptable::HandleHardcodedSurge(const ResRef& surgeSpell, const Spell *spl, Actor *caster)
{
	// format: ID or ID.param1 or +SPELLREF
	int types = caster->spellbook.GetTypes();
	int lvl = spl->SpellLevel-1;
	int count, i, tmp, tmp3;
	Scriptable *target = NULL;
	Point targetpos(-1, -1);
	ResRef newSpell;
	auto parts = Explode<ResRef, ResRef>(surgeSpell, '.', 2);
	
	int level = caster->GetCasterLevel(spl->SpellType);
	switch (surgeSpell[0]) {
		case '+': // cast normally, but also cast SPELLREF first
			core->ApplySpell(SubStr(surgeSpell, 1), caster, caster, level);
			break;
		case '0': // cast spell param1 times
			count = strtosigned<int>(parts[1].c_str());
			caster->wildSurgeMods.num_castings = count;
			break;
		case '1': // change projectile (id) to param1
			count = strtosigned<int>(parts[1].c_str());
			caster->wildSurgeMods.projectile_id = count;
			break;
		case '2': // also target target type param1
			count = strtosigned<int>(parts[1].c_str());
			caster->wildSurgeMods.target_type = count;
			caster->wildSurgeMods.target_change_type = WSTC_ADDTYPE;
			break;
		case '3': // (wild surge) roll param1 more times
			count = strtosigned<int>(parts[1].c_str());
			// force surge and then cast
			// force the surge roll to be < 100, so we cast a spell from the surge table
			tmp = caster->Modified[IE_FORCESURGE];
			tmp3 = caster->WMLevelMod; // also save caster level; the original didn't reroll the bonus
			caster->Modified[IE_FORCESURGE] = 7;
			if (LastSpellTarget) {
				target = area->GetActorByGlobalID(LastSpellTarget);
				if (!target) {
					target = core->GetGame()->GetActorByGlobalID(LastSpellTarget);
				}
			}
			if (LastTargetPos.IsInvalid()) {
				targetpos = LastTargetPos;
			} else if (target) {
				targetpos = target->Pos;
			}
			// SpellResRef still contains the original spell and we need to keep it that way
			// as any of the rerolls could result in a "spell cast normally" (non-surge)
			for (i=0; i<count; i++) {
				if (target) {
					caster->CastSpell(target, false, true, false, level);
					newSpell = SpellResRef;
					caster->WMLevelMod = tmp3;
					caster->CastSpellEnd(level, true);
				} else {
					caster->CastSpellPoint(targetpos, false, true, false, level);
					newSpell = SpellResRef;
					caster->WMLevelMod = tmp3;
					caster->CastSpellPointEnd(level, true);
				}
				// reset the ref, since CastSpell*End destroyed it
				SpellResRef = newSpell;
			}
			caster->Modified[IE_FORCESURGE] = tmp;
			break;
		case '4': // change the target type to param1
			count = strtosigned<int>(parts[1].c_str());
			caster->wildSurgeMods.target_type = count;
			caster->wildSurgeMods.target_change_type = WSTC_SETTYPE;
			break;
		case '5': // change the target to a random actor
			caster->wildSurgeMods.target_change_type = WSTC_RANDOMIZE;
			break;
		case '6': // change saving throw (+param1)
			count = strtosigned<int>(parts[1].c_str());
			caster->wildSurgeMods.saving_throw_mod = count;
			break;
		case '7': // random spell of the same level
			// change this if we ever want the surges to respect the original type
			for (i=0; i<types; i++) {
				unsigned int spellCount = caster->spellbook.GetKnownSpellsCount(i, lvl);
				if (!spellCount) continue;
				int id = core->Roll(1, spellCount, -1);
				const CREKnownSpell *ck = caster->spellbook.GetKnownSpell(i, lvl, id);
				if (ck) {
					SpellResRef = ck->SpellResRef;
					break;
				}
			}
			break;
		case '8': // set projectile speed to param1 %
			count = strtosigned<int>(parts[1].c_str());
			caster->wildSurgeMods.projectile_speed_mod = count;
			break;
		default:
			SpellHeader = -1;
			SpellResRef.Reset();
			Log(ERROR, "Scriptable", "New spell not found, aborting cast mid-surge!");
			caster->SetStance(IE_ANI_READY);
			return false;
	}
	return true;
}

String Scriptable::GetName() const
{
	switch (Type) {
		case ST_PROXIMITY:	return L"Proximity";
		case ST_TRIGGER:	return L"Trigger";
		case ST_TRAVEL:		return L"Travel";
		case ST_DOOR:		return L"Door";
		case ST_CONTAINER:	return L"Container";
		case ST_AREA:		return L"Area";
		case ST_GLOBAL:		return L"Global";
		case ST_ACTOR:		return As<const Actor>()->GetLongName();
		default:	return L"NONE";
	}
}

// aura pollution happens on cast or item use
// aura cleansing automatically or magically
bool Scriptable::AuraPolluted()
{
	if (Type != ST_ACTOR || AuraCooldown == 0) {
		return false;
	}

	// check for improved alacrity
	const Actor *actor = static_cast<const Actor*>(this);
	if (actor->GetStat(IE_AURACLEANSING)) {
		AuraCooldown = 0;
		if (core->HasFeedback(FT_STATES)) displaymsg->DisplayConstantStringName(HCStrings::AuraCleansed, GUIColors::WHITE, this);
		return false;
	}

	// sorry, you'll have to recover first
	return true;
}

bool Scriptable::TimerActive(ieDword ID)
{
	std::map<ieDword,ieDword>::iterator tit = script_timers.find(ID);
	if (tit == script_timers.end()) {
		return false;
	}
	return tit->second > core->GetGame()->GameTime;
}

bool Scriptable::TimerExpired(ieDword ID)
{
	std::map<ieDword,ieDword>::iterator tit = script_timers.find(ID);
	if (tit == script_timers.end()) {
		return false;
	}
	if (tit->second <= core->GetGame()->GameTime) {
		// expired timers become inactive after being checked
		script_timers.erase(tit);
		return true;
	}
	return false;
}

void Scriptable::StartTimer(ieDword ID, ieDword expiration)
{
	ieDword newTime = core->GetGame()->GameTime + expiration * core->Time.defaultTicksPerSec;
	std::map<ieDword,ieDword>::iterator tit = script_timers.find(ID);
	if (tit != script_timers.end()) {
		tit->second = newTime;
		return;
	}
	script_timers.insert(std::pair<ieDword,ieDword>(ID, newTime));
}

/********************
 * Selectable Class *
 ********************/

void Selectable::SetBBox(const Region &newBBox)
{
	BBox = newBBox;
}

// NOTE: still need to multiply by 4 or 3 to get full pixel radii
int Selectable::CircleSize2Radius() const
{
	// for size >= 2, radii are (size-1)*16, (size-1)*12
	// for size == 1, radii are 12, 9
	int adjustedSize = (circleSize - 1) * 4;
	if (adjustedSize < 4) adjustedSize = 3;
	return adjustedSize;
}

void Selectable::DrawCircle(const Point& p) const
{
	/* BG2 colours ground circles as follows:
	dark green for unselected party members
	bright green for selected party members
	flashing green/white for a party member the mouse is over
	bright red for enemies
	yellow for panicked actors
	flashing red/white for enemies the mouse is over
	flashing cyan/white for neutrals the mouse is over
	*/

	if (circleSize <= 0) {
		return;
	}

	Color mix;
	const Color* col = &selectedColor;
	Holder<Sprite2D> sprite = circleBitmap[0];

	if (Selected && !Over) {
		sprite = circleBitmap[1];
	} else if (Over) {
		mix = GlobalColorCycle.Blend(overColor, selectedColor);
		col = &mix;
	} else if (IsPC()) {
		col = &overColor;
	}

	if (sprite) {
		VideoDriver->BlitSprite(sprite, Pos - p);
	} else {
		float baseSize = CircleSize2Radius() * sizeFactor;
		const Size s(baseSize * 8, baseSize * 6);
		const Region r(Pos - p - s.Center(), s);
		VideoDriver->DrawEllipse(r, *col);
	}
}

// Check if P is over our ground circle
bool Selectable::IsOver(const Point &P) const
{
	int csize = circleSize;
	if (csize < 2) csize = 2;

	int dx = P.x - Pos.x;
	int dy = P.y - Pos.y;

	// check rectangle first
	if (dx < -(csize-1)*16 || dx > (csize-1)*16) return false;
	if (dy < -(csize-1)*12 || dy > (csize-1)*12) return false;

	// then check ellipse
	int r = 9*dx*dx + 16*dy*dy; // 48^2 * ( (dx/16)^2 + (dy/12)^2 )

	return (r <= 48*48*(csize-1)*(csize-1));
}

bool Selectable::IsSelected() const
{
	return Selected == 1;
}

void Selectable::SetOver(bool over)
{
	Over = over;
}

//don't call this function after rendering the cover and before the
//blitting of the sprite or bad things will happen :)
void Selectable::Select(int Value)
{
	if (Selected!=0x80 || Value!=1) {
		Selected = (ieWord) Value;
	}
}

void Selectable::SetCircle(int circlesize, float factor, const Color &color, Holder<Sprite2D> normal_circle, Holder<Sprite2D> selected_circle)
{
	circleSize = circlesize;
	sizeFactor = factor;
	selectedColor = color;
	overColor.r = color.r >> 1;
	overColor.g = color.g >> 1;
	overColor.b = color.b >> 1;
	overColor.a = color.a;
	circleBitmap[0] = std::move(normal_circle);
	circleBitmap[1] = std::move(selected_circle);
}

/***********************
 * Highlightable Class *
 ***********************/

bool Highlightable::IsOver(const Point &Place) const
{
	if (!outline) {
		return false;
	}
	return outline->PointIn(Place);
}

void Highlightable::DrawOutline(Point origin) const
{
	if (!outline) {
		return;
	}
	origin = outline->BBox.origin - origin;

	if (core->HasFeature(GFFlags::PST_STATE_FLAGS)) {
		VideoDriver->DrawPolygon(outline.get(), origin, outlineColor, true, BlitFlags::MOD | BlitFlags::HALFTRANS);
	} else {
		VideoDriver->DrawPolygon( outline.get(), origin, outlineColor, true, BlitFlags::BLENDED|BlitFlags::HALFTRANS );
		VideoDriver->DrawPolygon( outline.get(), origin, outlineColor, false );
	}
}

void Highlightable::SetCursor(unsigned char CursorIndex)
{
	Cursor = CursorIndex;
}

//trap that will fire now
bool Highlightable::TriggerTrap(int /*skill*/, ieDword ID)
{
	if (!Trapped) {
		return false;
	}
	//actually this could be script name[0]
	if (!Scripts[0] && EnterWav.IsEmpty()) {
		return false;
	}
	AddTrigger(TriggerEntry(trigger_entered, ID));
	AddTrigger(TriggerEntry(trigger_traptriggered, ID)); // for that one user in bg2

	// the second part is a hack to deal with bg2's ar1401 lava floor trap ("muck"), which doesn't have the repeating bit set
	// just testing TrapDetectionDiff/TrapRemovalDiff is not good enough as DeadThiefProx in the initial bg2 chamber exit is
	// almost identical and would retrigger instead of being a one-off
	// at the same time iwd2 Saablic_Greeting in ar6200 shouldn't repeat, while being practically identical
	// should we always send Entered instead, also when !Trapped? Does not appear so, see history of InfoPoint::TriggerTrap
	if (TrapResets()) {
		AddTrigger(TriggerEntry(trigger_reset, GetGlobalID()));
	} else if (third || scriptName != "muck") {
		Trapped = false;
	}
	return true;
}

bool Highlightable::TryUnlock(Actor *actor, bool removekey) const {
	if (KeyResRef.IsEmpty()) {
		return false;
	}

	Actor* haskey = nullptr;
	if (actor->InParty) {
		const Game *game = core->GetGame();
		//allow unlock when the key is on any partymember
		for (int idx = 0; idx < game->GetPartySize(false); idx++) {
			Actor *pc = game->FindPC(idx + 1);
			if (!pc) continue;
			if (HasItemCore(&pc->inventory, KeyResRef, 0)) {
				haskey = pc;
				break;
			}
		}
	// actor is not in party, check only actor
	} else if (HasItemCore(&actor->inventory, KeyResRef, 0)) {
		haskey = actor;
	}

	if (!haskey) {
		return false;
	}

	if (removekey) {
		CREItem* item = nullptr;
		int result = haskey->inventory.RemoveItem(KeyResRef, 0, &item);
		// check also in bags if nothing was found
		if (result == -1) {
			int i= haskey->inventory.GetSlotCount();
			while (i--) {
				// maybe we could speed this up if we mark bag items with a flags bit
				const CREItem* itemSlot = haskey->inventory.GetSlotItem(i);
				if (!itemSlot) continue;
				const Item* itemStore = gamedata->GetItem(itemSlot->ItemResRef);
				if (!itemStore) continue;
				if (core->CheckItemType(itemStore, SLOT_BAG)) {
					//the store is the same as the item's name
					RemoveStoreItem(itemSlot->ItemResRef, KeyResRef);
				}
				gamedata->FreeItem(itemStore, itemSlot->ItemResRef);
			}
		}
		//the item should always be existing!!!
		delete item;
	}

	return true;
}

//detect this trap, using a skill, skill could be set to 256 for 'sure'
//skill is the all around modified trap detection skill
//a trapdetectiondifficulty of 100 means impossible detection short of a spell
void Highlightable::DetectTrap(int skill, ieDword actorID)
{
	if (!CanDetectTrap()) return;
	if (TrapDetected) return;
	if (!Scripts[0]) return;
	if (skill >= 100 && skill != 256) skill = 100;

	int check = 0;
	Actor* detective = core->GetGame()->GetActorByGlobalID(actorID);
	assert(detective);
	if (third) {
		//~Search (detect traps) check. Search skill %d vs. trap's difficulty %d (searcher's %d INT bonus).~
		int bonus = detective->GetAbilityBonus(IE_INT);
		displaymsg->DisplayRollStringName(ieStrRef::ROLL13, GUIColors::LIGHTGREY, detective, skill - bonus, TrapDetectionDiff, bonus);
		check = (skill + bonus) * 7;
	} else {
		check = skill/2 + core->Roll(1, skill/2, 0);
	}
	if (check > TrapDetectionDiff) {
		SetTrapDetected(1); //probably could be set to the player #?
		AddTrigger(TriggerEntry(trigger_detected, actorID));
		displaymsg->DisplayMsgAtLocation(HCStrings::TrapFound, FT_ANY, detective, detective, GUIColors::WHITE);
	}
}

bool Highlightable::PossibleToSeeTrap() const
{
	return CanDetectTrap();
}

/*****************
 * Movable Class *
 *****************/

Movable::~Movable(void)
{
	if (path) {
		ClearPath(true);
	}
}

int Movable::GetPathLength() const
{
	const PathListNode *node = GetNextStep(0);
	if (!node) return 0;

	int i = 0;
	while (node->Next) {
		i++;
		node = node->Next;
	}
	return i;
}

PathListNode *Movable::GetNextStep(int x) const
{
	if (!step) {
		error("GetNextStep", "Hit with step = null");
	}
	PathListNode *node = step;
	while(node && x--) {
		node = node->Next;
	}
	return node;
}

Point Movable::GetMostLikelyPosition() const
{
	if (!path) {
		return Pos;
	}

//actually, sometimes middle path would be better, if
//we stand in Destination already
	int halfway = GetPathLength()/2;
	const PathListNode *node = GetNextStep(halfway);
	if (node) {
		return Map::ConvertCoordFromTile(node->point) + Point(8, 6);
	}
	return Destination;
}

void Movable::SetStance(unsigned int arg)
{
	//don't modify stance from dead back to anything if the actor is dead
	if ((StanceID==IE_ANI_TWITCH || StanceID==IE_ANI_DIE) && (arg!=IE_ANI_TWITCH) ) {
		if (GetInternalFlag()&IF_REALLYDIED) {
			Log(WARNING, "Movable", "Stance overridden by death");
			return;
		}
	}

	if (arg >= MAX_ANIMS) {
		StanceID = IE_ANI_AWAKE;
		Log(ERROR, "Movable", "Tried to set invalid stance id({})", arg);
		return;
	}

	Actor* caster = Scriptable::As<Actor>(this);
	if (StanceID == IE_ANI_CONJURE && StanceID != arg) {
		if (caster && caster->casting_sound) {
			caster->casting_sound->Stop();
			caster->casting_sound.release();
		}
	}

	StanceID = (unsigned char) arg;

	if (StanceID == IE_ANI_ATTACK) {
		// Set stance to a random attack animation
		int random = RAND(0, 99);
		if (random < AttackMovements[0]) {
			StanceID = IE_ANI_ATTACK_BACKSLASH;
		} else if (random < AttackMovements[0] + AttackMovements[1]) {
			StanceID = IE_ANI_ATTACK_SLASH;
		} else {
			StanceID = IE_ANI_ATTACK_JAB;
		}
	}

	// this doesn't get hit on movement, since movement overrides the stance manually
	// but it is needed for the twang/clank when an actor stops moving
	// a lot of other stances would get skipped later, since we check we're out of combat
	if (caster) {
		caster->PlayArmorSound();
	}
}

void Movable::SetOrientation(orient_t value, bool slow) {
	NewOrientation = value;
	if (NewOrientation != Orientation && Type == ST_ACTOR) {
		const Actor *actor = (Actor *) this;
		actor->PlayArmorSound();
	}
	if (!slow) {
		Orientation = NewOrientation;
	}
}

void Movable::SetOrientation(const Point& from, const Point& to, bool slow)
{
	SetOrientation(GetOrient(from, to), slow);
}

void Movable::SetAttackMoveChances(const ieWord *amc)
{
	AttackMovements[0]=amc[0];
	AttackMovements[1]=amc[1];
	AttackMovements[2]=amc[2];
}

//this could be used for WingBuffet as well
void Movable::MoveLine(int steps, orient_t orient)
{
	if (path || !steps) {
		return;
	}
	// DoStep takes care of stopping on walls if necessary
	path = area->GetLine(Pos, steps, orient);
}

orient_t Movable::GetNextFace() const
{
	//slow turning
	if (timeStartStep==core->GetGame()->Ticks) {
		return Orientation;
	}
	return GemRB::GetNextFace(Orientation, NewOrientation);
}


void Movable::Backoff()
{
	StanceID = IE_ANI_READY;
	if (InternalFlags & IF_RUNNING) {
		randomBackoff = RAND(MAX_PATH_TRIES * 2 / 3, MAX_PATH_TRIES * 4 / 3);
	} else {
		randomBackoff = RAND(MAX_PATH_TRIES, MAX_PATH_TRIES * 2);
	}
}


void Movable::BumpAway()
{
	area->ClearSearchMapFor(this);
	if (!IsBumped()) oldPos = Pos;
	bumped = true;
	bumpBackTries = 0;
	area->AdjustPositionNavmap(Pos);
}

void Movable::BumpBack()
{
	if (Type != ST_ACTOR) return;
	const Actor *actor = (const Actor*) this;
	area->ClearSearchMapFor(this);
	PathMapFlags oldPosBlockStatus = area->GetBlocked(oldPos);
	if (!(oldPosBlockStatus & PathMapFlags::PASSABLE)) {
		// Do bump back if the actor is "blocking" itself
		if (!((oldPosBlockStatus & PathMapFlags::ACTOR) == PathMapFlags::ACTOR && area->GetActor(oldPos, GA_NO_DEAD|GA_NO_UNSCHEDULED) == actor)) {
			area->BlockSearchMapFor(this);
			if (actor->GetStat(IE_EA) < EA_GOODCUTOFF) {
				bumpBackTries++;
				if (bumpBackTries > MAX_BUMP_BACK_TRIES && SquaredDistance(Pos, oldPos) < unsigned(circleSize * 32 * circleSize * 32)) {
					oldPos = Pos;
					bumped = false;
					bumpBackTries = 0;
					if (SquaredDistance(Pos, Destination) < unsigned(circleSize * 32 * circleSize * 32)) {
						ClearPath(true);
					}

				}
			}
			return;
		}
	}
	bumped = false;
	MoveTo(oldPos);
	bumpBackTries = 0;
}

// Takes care of movement and actor bumping, i.e. gently pushing blocking actors out of the way
// The movement logic is a proportional regulator: the displacement/movement vector has a
// fixed radius, based on actor walk speed, and its direction heads towards the next waypoint.
// The bumping logic checks if there would be a collision if the actor was to move according to this
// displacement vector and then, if that is the case, checks if that actor can be bumped
// In that case, it bumps it and goes on with its step, otherwise it either stops and waits
// for a random time (inspired by network media access control algorithms) or just stops if
// the goal is close enough.
void Movable::DoStep(unsigned int walkScale, ieDword time) {
	// Only bump back if not moving
	// Actors can be bumped while moving if they are backing off
	if (!path) {
		if (IsBumped()) {
			BumpBack();
		}
		return;
	}
	if (!time) time = core->GetGame()->Ticks;
	if (!walkScale) {
		// zero speed: no movement
		StanceID = IE_ANI_READY;
		timeStartStep = time;
		return;
	}
	if (!step) {
		step = path;
		timeStartStep = time;
		return;
	}

	if (time > timeStartStep) {
		Point nmptStep = step->point;
		double dx = nmptStep.x - Pos.x;
		double dy = nmptStep.y - Pos.y;
		Map::NormalizeDeltas(dx, dy, double(gamedata->GetStepTime()) / double(walkScale));
		Actor *actorInTheWay = nullptr;
		// We can't use GetActorInRadius because we want to only check directly along the way
		// and not be blocked by actors who are on the sides
		int collisionLookaheadRadius = ((circleSize < 3 ? 3 : circleSize) - 1) * 3;
		for (int r = collisionLookaheadRadius; r > 0 && !actorInTheWay; r--) {
			double xCollision = Pos.x + dx * r;
			double yCollision = Pos.y + dy * r * 0.75;
			Point nmptCollision(xCollision, yCollision);
			actorInTheWay = area->GetActor(nmptCollision, GA_NO_DEAD|GA_NO_UNSCHEDULED|GA_NO_SELF, this);
		}

		const Actor* actor = Scriptable::As<Actor>(this);
		bool blocksSearch = BlocksSearchMap();
		if (actorInTheWay && blocksSearch && actorInTheWay->BlocksSearchMap()) {
			// Give up instead of bumping if you are close to the goal
			if (!(step->Next) && PersonalDistance(nmptStep, this) < MAX_OPERATING_DISTANCE) {
				ClearPath(true);
				NewOrientation = Orientation;
				// Do not call ReleaseCurrentAction() since other actions
				// than MoveToPoint can cause movement
				Log(DEBUG, "PathFinderWIP", "Abandoning because I'm close to the goal");
				pathAbandoned = true;
				return;
			}
			if (actor && actor->ValidTarget(GA_CAN_BUMP) && actorInTheWay->ValidTarget(GA_ONLY_BUMPABLE)) {
				actorInTheWay->BumpAway();
			} else {
				Backoff();
				return;
			}
		}
		// Stop if there's a door in the way
		if (blocksSearch && bool(area->GetBlocked(Pos + Point(dx, dy)) & PathMapFlags::SIDEWALL)) {
			ClearPath(true);
			NewOrientation = Orientation;
			return;
		}
		if (blocksSearch) {
			area->ClearSearchMapFor(this);
		}
		StanceID = IE_ANI_WALK;
		if (InternalFlags & IF_RUNNING) {
			StanceID = IE_ANI_RUN;
		}
		Pos.x += dx;
		Pos.y += dy;
		oldPos = Pos;
		if (actor && blocksSearch) {
			auto flag = actor->IsPartyMember() ? PathMapFlags::PC : PathMapFlags::NPC;
			area->tileProps.PaintSearchMap(Map::ConvertCoordToTile(Pos), circleSize, flag);
		}

		SetOrientation(step->orient, false);
		timeStartStep = time;
		if (Pos == nmptStep) {
			if (step->Next) {
				step = step->Next;
			} else {
				ClearPath(true);
				NewOrientation = Orientation;
				pathfindingDistance = circleSize;
			}
		}
	}
}

void Movable::AdjustPosition()
{
	area->AdjustPosition(Pos);
	ImpedeBumping();
}

void Movable::AddWayPoint(const Point &Des)
{
	if (!path) {
		WalkTo(Des);
		return;
	}
	Destination = Des;
	//it is tempting to use 'step' here, as it could
	//be about half of the current path already
	PathListNode *endNode = path;
	while(endNode->Next) {
		endNode = endNode->Next;
	}
	Point p = endNode->point;
	area->ClearSearchMapFor(this);
	PathListNode* path2 = area->FindPath(p, Des, circleSize);
	// if the waypoint is too close to the current position, no path is generated
	if (!path2) {
		if (BlocksSearchMap()) {
			area->BlockSearchMapFor(this);
		}
		return;
	}
	endNode->Next = path2;
	//probably it is wise to connect it both ways?
	path2->Parent = endNode;
}

// This function is called at each tick if an actor is following another actor
// Therefore it's rate-limited to avoid actors being stuck as they keep pathfinding
void Movable::WalkTo(const Point &Des, int distance)
{
	// Only rate-limit when moving
	if ((GetPath() || InMove()) && prevTicks && Ticks < prevTicks + 2) {
		return;
	}

	const Actor* actor = Scriptable::As<Actor>(this);

	prevTicks = Ticks;
	Destination = Des;
	if (pathAbandoned) {
		Log(DEBUG, "WalkTo", "{}: Path was just abandoned", fmt::WideToChar{actor->GetShortName()});
		ClearPath(true);
		return;
	}

	if (Pos.x / 16 == Des.x / 16 && Pos.y / 12 == Des.y / 12) {
		ClearPath(true);
		SetStance(IE_ANI_HEAD_TURN);
		return;
	}

	if (BlocksSearchMap()) area->ClearSearchMapFor(this);
	PathListNode* newPath = area->FindPath(Pos, Des, circleSize, distance, PF_SIGHT | PF_ACTORS_ARE_BLOCKING, actor);
	if (!newPath && actor && actor->ValidTarget(GA_CAN_BUMP)) {
		Log(DEBUG, "WalkTo", "{} re-pathing ignoring actors", fmt::WideToChar{actor->GetShortName()});
		newPath = area->FindPath(Pos, Des, circleSize, distance, PF_SIGHT, actor);
	}

	if (newPath) {
		ClearPath(false);
		path = newPath;
		step = path;
		HandleAnkhegStance(false);
	}  else {
		pathfindingDistance = std::max(circleSize, distance);
		if (BlocksSearchMap()) {
			area->BlockSearchMapFor(this);
		}
	}
}

void Movable::RunAwayFrom(const Point &Source, int PathLength, bool noBackAway)
{
	ClearPath(true);
	area->ClearSearchMapFor(this);
	path = area->RunAway(Pos, Source, circleSize, PathLength, !noBackAway, As<Actor>());
	HandleAnkhegStance(false);
}

void Movable::RandomWalk(bool can_stop, bool run)
{
	if (path) {
		return;
	}
	//if not continous random walk, then stops for a while
	if (can_stop) {
		Region vp = core->GetGameControl()->Viewport();
		if (!vp.PointInside(Pos)) {
			SetWait(core->Time.defaultTicksPerSec * core->Roll(1, 40, 0));
			return;
		}
		// a 50/50 chance to move or do a spin (including its own wait)
		if (RandomFlip()) {
			Action *me = ParamCopy(CurrentAction);
			Action *turnAction = GenerateAction("RandomTurn()");
			// only spin once before relinquishing control back
			turnAction->int0Parameter = 3;
			// remove and readd ourselves, so the turning gets a chance to run
			ReleaseCurrentAction();
			AddActionInFront(me);
			AddActionInFront(turnAction);
			return;
		}
	}

	// handle the RandomWalkTime variants, which only count moves
	if (CurrentAction->int0Parameter && !CurrentAction->int1Parameter) {
		// first run only
		CurrentAction->int1Parameter = 1;
		CurrentAction->int0Parameter++;
	}
	if (CurrentAction->int0Parameter) {
		CurrentAction->int0Parameter--;
	}
	if (CurrentAction->int1Parameter && !CurrentAction->int0Parameter) {
		ReleaseCurrentAction();
		return;
	}

	randomWalkCounter++;
	if (randomWalkCounter > MAX_RAND_WALK) {
		randomWalkCounter = 0;
		WalkTo(HomeLocation);
		return;
	}

	if (run) {
		InternalFlags|=IF_RUNNING;
	}

	if (BlocksSearchMap()) {
		area->ClearSearchMapFor(this);
	}

	//the 5th parameter is controlling the orientation of the actor
	//0 - back away, 1 - face direction
	path = area->RandomWalk(Pos, circleSize, maxWalkDistance ? maxWalkDistance : 5, As<Actor>());
	if (BlocksSearchMap()) {
		area->BlockSearchMapFor(this);
	}
	if (path) {
		Destination = path->point;
	} else {
		randomWalkCounter = 0;
		WalkTo(HomeLocation);
		return;
	}

}

void Movable::MoveTo(const Point &Des)
{
	area->ClearSearchMapFor(this);
	Pos = Des;
	oldPos = Des;
	Destination = Des;
	if (BlocksSearchMap()) {
		area->BlockSearchMapFor(this);
	}
}

void Movable::Stop(int flags)
{
	Scriptable::Stop(flags);
	ClearPath(true);
}

void Movable::ClearPath(bool resetDestination)
{
	pathAbandoned = false;

	if (resetDestination) {
		//this is to make sure attackers come to us
		//make sure ClearPath doesn't screw Destination (in the rare cases Destination
		//is set before ClearPath
		Destination = Pos;

		if (StanceID == IE_ANI_WALK || StanceID == IE_ANI_RUN) {
			StanceID = IE_ANI_AWAKE;
		}
		HandleAnkhegStance(true);
		InternalFlags &= ~IF_NORETICLE;
	}
	PathListNode* thisNode = path;
	while (thisNode) {
		PathListNode* nextNode = thisNode->Next;
		delete thisNode;
		thisNode = nextNode;
	}
	path = nullptr;
	step = nullptr;
	//don't call ReleaseCurrentAction
}

// (un)hide ankhegs when they stop/start moving
void Movable::HandleAnkhegStance(bool emerge)
{
	const Actor* actor = As<Actor>();
	int nextStance = emerge ? IE_ANI_EMERGE : IE_ANI_HIDE;
	if (actor && path && StanceID != nextStance && actor->GetAnims()->GetAnimType() == IE_ANI_TWO_PIECE) {
		SetStance(nextStance);
		SetWait(15); // both stances have 15 frames, at 15 fps
	}
}

/**********************
 * Tiled Object Class *
 **********************/

TileObject::~TileObject()
{
	if (opentiles) {
		free( opentiles );
	}
	if (closedtiles) {
		free( closedtiles );
	}
}

void TileObject::SetOpenTiles(unsigned short* Tiles, int cnt)
{
	if (opentiles) {
		free( opentiles );
	}
	opentiles = Tiles;
	opencount = cnt;
}

void TileObject::SetClosedTiles(unsigned short* Tiles, int cnt)
{
	if (closedtiles) {
		free( closedtiles );
	}
	closedtiles = Tiles;
	closedcount = cnt;
}


}
