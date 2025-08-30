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

#include "ie_stats.h"
#include "strrefs.h"

#include "DialogHandler.h"
#include "DisplayMessage.h"
#include "Game.h"
#include "GameData.h"
#include "Interface.h"
#include "Map.h"
#include "Projectile.h"
#include "Spell.h"

#include "GUI/GameControl.h"
#include "GameScript/GSUtils.h"
#include "GameScript/Matching.h" // MatchActor
#include "Scriptable/Highlightable.h"

#include <utility>

namespace GemRB {

// we start this at a non-zero value to make debugging easier
static ieDword globalActorCounter = 10000;
static bool startActive = false;
static bool third = false;
static bool pst_flags = false;
static const unsigned short ClearActionsID = 133; // same for all games
unsigned int Scriptable::VOODOO_VISUAL_RANGE = 28;

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
	ClearActions(4);
	for (auto& script : Scripts) {
		delete script;
	}
}

ieDword Scriptable::GetLocal(const ieVariable& key, ieDword fallback) const
{
	auto lookup = locals.find(key);
	if (lookup != locals.cend()) {
		return lookup->second;
	}

	return fallback;
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

void Scriptable::SetDialog(const ResRef& resref)
{
	if (!resref.IsEmpty() && gamedata->Exists(resref, IE_DLG_CLASS_ID, true)) {
		Dialog = resref;
	}
}

Map* Scriptable::GetCurrentArea() const
{
	//this could be NULL, always check it
	return area;
}

void Scriptable::SetMap(Map* map)
{
	if (map && (map->GetCurrentArea() != map)) {
		//a map always points to itself (if it is a real map)
		error("Scriptable", "Invalid map set!");
	}
	area = map;
}

//ai is nonzero if this is an actor currently in the party
//if the script level is AI_SCRIPT_LEVEL, then we need to
//load an AI script (.bs) instead of (.bcs)
void Scriptable::SetScript(const ResRef& aScript, int idx, bool ai)
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
	// NONE is an 'invalid' script name, seldom used to reset the slot, which we do above
	// This check is to prevent flooding of the console
	if (!aScript.IsEmpty() && aScript != "NONE") {
		if (idx != AI_SCRIPT_LEVEL) ai = false;
		Scripts[idx] = new GameScript(aScript, this, idx, ai);
	}
}

void Scriptable::SetSpellResRef(const ResRef& resref)
{
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
	return ((const Actor*) this)->GetStat(IE_EA) <= EA_CHARMED;
}

void Scriptable::Update()
{
	Ticks++;
	AdjustedTicks++;
	if (AuraCooldown) AuraCooldown--;

	if (UnselectableTimer) {
		UnselectableTimer--;
		if (!UnselectableTimer && Type == ST_ACTOR) {
			Actor* actor = (Actor*) this;
			actor->SetCircleSize();
			if (actor->InParty) {
				core->GetGame()->SelectActor(actor, true, SELECT_QUIET);
				core->SetEventFlag(EF_PORTRAIT);
			}
		}
		if (!UnselectableTimer) UnselectableType = 0;
	}

	TickScripting();

	ProcessActions();

	InterruptCasting = false;
}

void Scriptable::TickScripting()
{
	// Stagger script updates.
	// but not for just loaded area scripts, ensuring they run first
	if (Ticks % 16 != globalID % 16 && (Type != ST_AREA || Ticks > 1)) {
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
	// we potentially delay a forced update for 1 tick, since TriggerCountdown is only set later (like the original)
	// NOTE: however the original also had another bool for when a condition returned true, avoiding this
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
	const GameControl* gc = core->GetGameControl();

	// area scripts still run for at least the current area, in bg1 (see ar2631, confirmed by testing)
	// but not in bg2 (kill Abazigal in ar6005)
	if (gc->GetScreenFlags().Test(ScreenFlags::Cutscene)) {
		if (!(core->HasFeature(GFFlags::CUTSCENE_AREASCRIPTS) && (Type == ST_AREA || Type == ST_PROXIMITY))) {
			return;
		}
	}

	// Don't abort if there is a running non-interruptible action.
	// NOTE: the original didn't check for the next action, causing odd timing bugs
	if ((InternalFlags & IF_NOINT) && (CurrentAction || GetNextAction())) {
		return;
	}
	if (!CurrentActionInterruptible) {
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
	if (act && (act->GetStat(IE_CASTERHOLD) || act->GetStat(IE_SUMMONDISABLE))) return;

	// (EE-only?) bg2 special casing, probably fine elsewhere, so not ifdefing to bg2/bg2ee just yet
	// don't touch if we're on autopilot for everything but the party, except for Edwin
	// TODO: check if the Edwin hack is really needed (his quests once he joins, other MakeUnselectable calls)
	if (UnselectableTimer && (UnselectableType & 1) == 0) {
		if (!act || !act->InParty || act->GetName() != u"Edwin") return;
	}

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
		Game* game = core->GetGame();
		if (act->InParty && game->nextBored > 100 && // throtle a bit, since it could get expensive
		    (CurrentAction || act->Modal.State == Modal::BattleSong || act->Modal.State == Modal::ShamanDance) &&
		    act->ValidTarget(GA_SELECT)) {
			game->nextBored = 0;
		}

		// if party AI is disabled, don't run non-override scripts
		if (act->InParty && !(game->ControlStatus & CS_PARTY_AI)) scriptCount = 1;

		// hardcoded action overrides like charm, confusion, panic and berserking
		if (act->OverrideActions()) {
			// we just need to execute, no more processing needed
			ClearTriggers();
			return;
		}
	}

	bool continuing = false, done = false;
	for (scriptLevel = 0; scriptLevel < scriptCount; scriptLevel++) {
		GameScript* script = Scripts[scriptLevel];
		if (script) {
			changed |= script->Update(&continuing, &done);
			if (script->dead) {
				delete script;
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
		act->IdleActions(CurrentAction != NULL);
	}
}

void Scriptable::AddAction(std::string actStr)
{
	Action* aC = GenerateAction(std::move(actStr));
	AddAction(aC);
}

void Scriptable::AddAction(Action* aC)
{
	if (!aC) {
		Log(WARNING, "Scriptable", "AA: NULL action encountered for {}!", scriptName);
		return;
	}

	InternalFlags |= IF_ACTIVE;
	if (startActive) {
		InternalFlags &= ~IF_IDLE;
	}
	aC->IncRef();
	if (actionflags[aC->actionID] & AF_SCRIPTLEVEL) {
		aC->int0Parameter = scriptLevel;
	}

	// attempt to handle 'instant' actions, from instant.ids, which run immediately
	// when added if the action queue is empty, even on actors which are Held/etc
	// but try to ignore iwd2 ActionOverride for 41pstail.bcs
	// FIXME: area check hack until fuzzie fixes scripts here
	const Action* nextAction = GetNextAction();
	bool ignoreQueue = !nextAction || (third && nextAction->objects[0]);
	if (!CurrentAction && ignoreQueue && area) {
		int instant = AF_SCR_INSTANT;
		if (core->GetGameControl()->InDialog()) {
			instant = AF_DLG_INSTANT;
		}
		if (actionflags[aC->actionID] & instant) {
			CurrentAction = aC;
			GameScript::ExecuteAction(this, CurrentAction);
			return;
		}
	}

	actionQueue.push_back(aC);
}

void Scriptable::AddActionInFront(Action* aC)
{
	if (!aC) {
		Log(WARNING, "Scriptable", "AAIF: null action encountered for {}!", scriptName);
		return;
	}
	InternalFlags |= IF_ACTIVE;
	actionQueue.push_front(aC);
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
	// clear dialog target if it was us that wanted to talk
	// this is here just to clear the target reticle back to a circle
	if (CurrentAction && CurrentAction->actionID == 70) { // NIDSpecial1
		core->GetGameControl()->dialoghandler->SetTarget(nullptr);
	}

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
		} else if (skipFlags == 3 && (CurrentActionInterruptible == false || InternalFlags & IF_NOINT)) {
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
			i--;
			aC->Release();
		}
	}
	if (savedCurrentAction) return;

	WaitCounter = 0;
	objects.LastTarget = 0;
	objects.LastTargetPos.Invalidate();
	// intentionally not resetting LastTargetPersistent
	objects.LastSpellTarget = 0;

	if (Type == ST_ACTOR) {
		Interrupt();
		if (skipFlags != 4) As<Actor>(this)->ResetAttackProjectile();
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
	CurrentActionInterruptible = true;
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
		CurrentActionInterruptible = true;
		if (!CurrentAction) {
			if (!(CurrentActionTicks == 0 && CurrentActionState == 0)) {
				Log(ERROR, "Scriptable", "Last action: {}", lastAction);
			}
			assert(CurrentActionTicks == 0 && CurrentActionState == 0);
			CurrentAction = PopNextAction();
		} else {
			CurrentActionTicks++;
		}
		if (!CurrentAction) {
			ClearActions(4);
			// clear lastAction here if you'll ever need it after exiting the loop
			break;
		}
		lastAction = CurrentAction->actionID;
		GameScript::ExecuteAction(this, CurrentAction);
		//break execution in case of a Wait flag
		if (WaitCounter) {
			break;
		}
		// break execution in case of blocking action (AF_BLOCKING)
		if (CurrentAction) {
			break;
		}
		// break execution if a fade was started (it pauses script processing, so this can only be true once here)
		if (core->timer.IsFading()) {
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
	if (Type != ST_ACTOR) {
		return false;
	}
	return !As<Movable>(this)->GetPath().Empty();
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
	InternalFlags &= ~(IF_ACTIVE | IF_IDLE);
}

void Scriptable::Activate()
{
	InternalFlags |= IF_ACTIVE;
	InternalFlags &= ~IF_IDLE;
}

void Scriptable::PartyRested()
{
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
		objects.LastTrigger = scriptableID;
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

bool Scriptable::MatchTriggerWithObject(unsigned short id, const Object* obj, ieDword param) const
{
	for (auto& trigger : triggers) {
		if (trigger.triggerID != id) continue;
		if (param && trigger.param2 != param) continue;
		if (!MatchActor(this, trigger.param1, obj)) continue;
		return true;
	}

	return false;
}

const TriggerEntry* Scriptable::GetMatchingTrigger(unsigned short id, unsigned int notflags) const
{
	for (auto& trigger : triggers) {
		if (trigger.triggerID != id) continue;
		if (notflags & trigger.flags) continue;
		return &trigger;
	}

	return NULL;
}

// handle wild surge projectile modifiers
void Scriptable::ModifyProjectile(Projectile*& pro, Spell* spl, ieDword tgt, int level)
{
	Actor* caster = Scriptable::As<Actor>(this);
	assert(caster);

	int count;
	const Actor* newact = nullptr;
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
			pro = spl->GetProjectile(this, SpellHeader, level, objects.LastTargetPos);
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
			pro = spl->GetProjectile(this, SpellHeader, level, objects.LastTargetPos);
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
				objects.LastSpellTarget = newact->GetGlobalID();
				objects.LastTargetPos = newact->Pos;
			} else {
				// no better idea; I wonder if the original randomized point targets at all
				objects.LastTargetPos = newact->Pos;
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
			pro = spl->GetProjectile(this, SpellHeader, level, objects.LastTargetPos);
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
		pro = spl->GetProjectile(this, SpellHeader, level, objects.LastTargetPos);
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

	while (projectileCount--) {
		Projectile* pro = NULL;
		// jump through hoops to skip applying selftargeting spells to the caster
		// if we'll be changing the target
		int tct = 0;
		if (caster) {
			tct = caster->wildSurgeMods.target_change_type;
		}
		if (!caster || !tct || tct == WSTC_ADDTYPE || !caster->wildSurgeMods.projectile_id) {
			pro = spl->GetProjectile(this, SpellHeader, level, objects.LastTargetPos);
			if (!pro) {
				return;
			}
			pro->SetCaster(GetGlobalID(), level);
		}

		Point origin = Pos;
		if (Type == ST_TRIGGER || Type == ST_PROXIMITY || Type == ST_DOOR) {
			// try and make projectiles start from the right trap position
			// see the traps in the duergar/assassin battle in bg2 dungeon
			// see also function below - maybe we should fix Pos instead
			// iwd2 ar6050 doors need the same (the closed outline is moved to the map corner), same for traps in ar6100
			origin = As<const Highlightable>(this)->TrapLaunch;
		}

		if (caster) {
			ModifyProjectile(pro, spl, tgt, level);
		}

		if (tgt) {
			area->AddProjectile(pro, origin, tgt, fake);
		} else {
			area->AddProjectile(pro, origin, objects.LastTargetPos);
		}
	}

	ieDword spellnum = ResolveSpellNumber(spellResRef);
	if (spellnum != 0xffffffff) {
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
	const Scriptable* target = nullptr;
	if (tgt) {
		target = area->GetActorByGlobalID(tgt);
		if (!target) {
			target = core->GetGame()->GetActorByGlobalID(tgt);
		}
	}

	const String& spell = core->GetString(spl->SpellName);
	if (!spell.empty() && Type == ST_ACTOR) {
		String str;
		if (target) {
			// no "Casts " prefix for special powers (the originals erroneously left the space in)
			if (spl->SpellType == IE_SPL_INNATE) {
				str = fmt::format(u"{} : {}", spell, target->GetName());
			} else {
				const String msg = core->GetString(HCStrings::ActionCast, STRING_FLAGS::NONE);
				str = fmt::format(u"{} {} : {}", msg, spell, target->GetName());
			}
		} else {
			str = fmt::format(u"{} : {}", spell, GetName());
		}
		displaymsg->DisplayStringName(std::move(str), GUIColors::WHITE, this);
	}
}

// NOTE: currently includes the sender
void Scriptable::SendTriggerToAll(TriggerEntry entry, int extraFlags)
{
	int flags = GA_NO_DEAD | GA_NO_UNSCHEDULED | extraFlags;
	std::vector<Actor*> nearActors = area->GetAllActorsInRadius(Pos, flags, 15);
	for (const auto& neighbour : nearActors) {
		neighbour->AddTrigger(entry);
	}

	std::vector<Scriptable*> nearOthers = area->GetScriptablesInRect(Pos, 15);
	for (const auto& neighbour : nearOthers) {
		neighbour->AddTrigger(entry);
	}

	area->AddTrigger(entry);
}

inline void Scriptable::ResetCastingState(Actor* caster)
{
	SpellHeader = -1;
	SpellResRef.Reset();
	objects.LastTargetPos.Invalidate();
	objects.LastSpellTarget = 0;
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
		objects.LastTargetPos.Invalidate();
		return;
	}

	if (objects.LastTargetPos.IsInvalid()) {
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
		core->GetAudioPlayback().Play(spl->CompletionSound, AudioPreset::Spatial, SFXChannel::Missile, Pos);
	}

	CreateProjectile(SpellResRef, 0, level, false);

	// the original engine saves lasttrigger only in case of SpellCast, so we have to differentiate
	ieDword oldLastTrigger = objects.LastTrigger;
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

	Scriptable* target = area->GetScriptable(objects.LastTargetPos, GA_NO_UNSCHEDULED | GA_NO_HIDDEN);
	if (target) {
		target->AddTrigger(TriggerEntry(trigger_spellcastonme, GetGlobalID(), spellID));
		target->objects.LastSpellOnMe = spellID;
	}
	// restore LastTrigger as bg2 otygrate.bcs relies on it; mangles trigger_spellcast for the caster
	objects.LastTrigger = oldLastTrigger;

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
		objects.LastSpellTarget = 0;
		return;
	}
	if (!objects.LastSpellTarget) {
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
		core->GetAudioPlayback().Play(spl->CompletionSound, AudioPreset::Spatial, SFXChannel::Missile, Pos);
	}

	//if the projectile doesn't need to follow the target, then use the target position
	CreateProjectile(SpellResRef, objects.LastSpellTarget, level, GetSpellDistance(SpellResRef, this) == 0x7fffffff);

	// the original engine saves lasttrigger only in case of SpellCast, so we have to differentiate
	ieDword oldLastTrigger = objects.LastTrigger;
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

	Scriptable* target = area->GetScriptableByGlobalID(objects.LastSpellTarget);
	if (target) {
		target->AddTrigger(TriggerEntry(trigger_spellcastonme, GetGlobalID(), spellID));
		target->objects.LastSpellOnMe = spellID;
	}
	// restore LastTrigger as bg2 otygrate.bcs relies on it; mangles trigger_spellcast for the caster
	objects.LastTrigger = oldLastTrigger;

	ResetCastingState(caster);
}

// check for input sanity and good casting conditions
int Scriptable::CanCast(const ResRef& SpellRef, bool verbose)
{
	const Spell* spl = gamedata->GetSpell(SpellRef);
	if (!spl) {
		SpellHeader = -1;
		Log(ERROR, "Scriptable", "Spell not found, aborting cast!");
		return 0;
	}

	// check for area dead magic
	// tob AR3004 is a dead magic area, but using a script with personal dead magic
	if (area->GetInternalFlag() & AF_DEADMAGIC && !(spl->Flags & SF_HLA)) {
		displaymsg->DisplayConstantStringName(HCStrings::DeadmagicFail, GUIColors::WHITE, this);
		return 0;
	}

	if (spl->Flags & SF_NOT_INDOORS && !(area->AreaType & AT_OUTDOOR)) {
		displaymsg->DisplayConstantStringName(HCStrings::IndoorFail, GUIColors::WHITE, this);
		return 0;
	}

	// various individual checks
	if (Type != ST_ACTOR) {
		return 1;
	}
	const Actor* actor = (const Actor*) this;

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
	if (actor->Modified[IE_DEADMAGIC] && !(spl->Flags & SF_HLA)) {
		displaymsg->DisplayConstantStringName(HCStrings::DeadmagicFail, GUIColors::WHITE, this);
		return 0;
	}

	// check for miscast magic and similar
	ieDword roll = actor->LuckyRoll(1, 100, 0, 0);
	bool failed = false;
	ieDword chance = 1000;
	switch (spl->SpellType) {
		case IE_SPL_PRIEST:
			chance = actor->GetSpellFailure(false);
			break;
		case IE_SPL_WIZARD:
			chance = actor->GetSpellFailure(true);
			break;
		case IE_SPL_INNATE:
			chance = actor->Modified[IE_SPELLFAILUREINNATE];
			break;
		default:
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
void Scriptable::SpellcraftCheck(const Actor* caster, const ResRef& spellRef)
{
	if (!third || !caster || caster->GetStat(IE_EA) <= EA_CONTROLLABLE || !area) {
		return;
	}

	const Spell* spl = gamedata->GetSpell(spellRef);
	assert(spl); // only a bad surge could make this fail and we want to catch it
	int AdjustedSpellLevel = spl->SpellLevel + 15;
	std::vector<Actor*> neighbours = area->GetAllActorsInRadius(caster->Pos, GA_NO_DEAD | GA_NO_ENEMY | GA_NO_SELF | GA_NO_UNSCHEDULED, caster->GetVisualRange(), this);
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
			const String castmsg = core->GetString(HCStrings::Casts);
			const String spellname = core->GetString(spl->SpellName);
			overHead.SetText(fmt::format(u".:{} {}:.", castmsg, spellname));
			displaymsg->DisplayRollStringName(ieStrRef::ROLL15, GUIColors::LIGHTGREY, detective, Spellcraft + IntMod, AdjustedSpellLevel, IntMod);
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
	Point TmpPos = objects.LastTargetPos;
	ieDword TmpTarget = objects.LastSpellTarget;
	int TmpHeader = SpellHeader;

	SetSpellResRef(spellRef);
	CastSpellPoint(target, deplete, true, true, level);
	CastSpellPointEnd(level, keepStance);

	objects.LastTargetPos = TmpPos;
	objects.LastSpellTarget = TmpTarget;
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
	Point TmpPos = objects.LastTargetPos;
	ieDword TmpTarget = objects.LastSpellTarget;
	int TmpHeader = SpellHeader;

	SetSpellResRef(spellRef);
	CastSpell(target, deplete, true, true, level);
	CastSpellEnd(level, keepStance);

	objects.LastTargetPos = TmpPos;
	objects.LastSpellTarget = TmpTarget;
	SpellHeader = TmpHeader;
}

//set target as point
//if spell needs to be depleted, do it
//if spell is illegal stop casting
int Scriptable::CastSpellPoint(const Point& target, bool deplete, bool instant, bool noInterrupt, int level)
{
	objects.LastSpellTarget = 0;
	objects.LastTargetPos.Invalidate();
	Actor* actor = Scriptable::As<Actor>(this);
	if (actor && actor->HandleCastingStance(SpellResRef, deplete, instant)) {
		Log(ERROR, "Scriptable", "Spell {} not known or memorized, aborting cast!", SpellResRef);
		return -1;
	}
	if (!instant && !noInterrupt) {
		AuraCooldown = core->Time.attack_round_size;
	}
	if (!noInterrupt && !CanCast(SpellResRef)) {
		SpellResRef.Reset();
		if (actor) {
			actor->SetStance(IE_ANI_READY);
		}
		return -1;
	}

	objects.LastTargetPos = target;

	if (!CheckWildSurge()) {
		return -1;
	}

	int duration = SpellCast(instant, nullptr, level);
	if (!instant && duration) {
		SpellcraftCheck(actor, SpellResRef);
		if (actor) actor->CureInvisibility();
	}
	return duration;
}

//set target as actor (if target isn't actor, use its position)
//if spell needs to be depleted, do it
//if spell is illegal stop casting
int Scriptable::CastSpell(Scriptable* target, bool deplete, bool instant, bool noInterrupt, int level)
{
	objects.LastSpellTarget = 0;
	objects.LastTargetPos.Invalidate();
	Actor* actor = Scriptable::As<Actor>(this);
	if (actor && actor->HandleCastingStance(SpellResRef, deplete, instant)) {
		Log(ERROR, "Scriptable", "Spell {} not known or memorized, aborting cast!", SpellResRef);
		return -1;
	}

	assert(target);

	if (!instant && !noInterrupt) {
		AuraCooldown = core->Time.attack_round_size;
	}
	if (!noInterrupt && !CanCast(SpellResRef)) {
		SpellResRef.Reset();
		if (actor) {
			actor->SetStance(IE_ANI_READY);
		}
		return -1;
	}

	objects.LastTargetPos = target->Pos;
	if (target->Type == ST_ACTOR || target->Type == ST_CONTAINER || target->Type == ST_DOOR) {
		objects.LastSpellTarget = target->GetGlobalID();
	}

	if (!CheckWildSurge()) {
		return -1;
	}

	int duration = SpellCast(instant, target, level);
	if (!instant && duration) {
		SpellcraftCheck(actor, SpellResRef);
		// self-targeted spells that are not hostile maintain invisibility
		if (actor && target != this) actor->CureInvisibility();
	}
	return duration;
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

	const SPLExtHeader* header = spl->GetExtHeader(SpellHeader);
	int casting_time = (int) header->CastingTime;
	// how does this work for non-actors exactly?
	if (actor) {
		// The mental speed effect can shorten or lengthen the casting time.
		// But first check if a special maximum is set
		const Effect* fx = actor->fxqueue.HasEffectWithParam(fx_castingspeed_modifier_ref, 2);
		int max = 1000;
		if (fx) {
			max = fx->Parameter1;
		}
		if (max < 10 && casting_time > max) {
			casting_time = max;
		} else {
			casting_time -= (int) actor->Modified[IE_MENTALSPEED];
		}
		casting_time = Clamp(casting_time, 0, 10);
	}

	// this is a guess which seems approximately right so far (same as in the bg2 manual, except that it may be a combat round instead)
	int duration = (casting_time * core->Time.round_size) / 10;
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
			fxqueue.AddAllEffects((Actor*) target, target->Pos);
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
	core->SetEventFlag(EF_ACTION); // in case it was cast from a quickspell slot, so we update the count
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

	Actor* caster = (Actor*) this;

	int roll = core->Roll(1, 100, 0);
	if (!((roll <= 5 && caster->Modified[IE_SURGEMOD]) || caster->Modified[IE_FORCESURGE])) {
		return 1;
	}

	ResRef oldSpellResRef;
	oldSpellResRef = SpellResRef;
	const Spell* spl = gamedata->GetSpell(oldSpellResRef); // this was checked before we got here
	// ignore non-magic "spells"
	if (spl->Flags & (SF_HLA | SF_TRIGGER)) {
		gamedata->FreeSpell(spl, oldSpellResRef, false);
		return 1;
	}

	// some effects, if present, disable wild surges for the whole spell
	if (spl->ContainsTamingOpcode()) {
		return 1;
	}

	int check = roll + caster->Modified[IE_SURGEMOD];
	if (caster->Modified[IE_FORCESURGE] != 7) {
		// skip the caster level bonus if we're already in a complicated surge
		check += caster->GetCasterLevel(spl->SpellType);
	}
	if (caster->Modified[IE_CHAOSSHIELD]) {
		// avert the surge and decrease the chaos shield counter
		check = 0;
		caster->fxqueue.DecreaseParam1OfEffect(fx_chaosshield_ref, 1);
		displaymsg->DisplayConstantStringName(HCStrings::ChaosShield, GUIColors::LIGHTGREY, caster);
	}

	// hundred or more means a normal cast; same for negative values (for absurd antisurge modifiers)
	if ((check > 0) && (check < 100)) {
		// display feedback: Wild Surge: bla bla
		// look up the spell in the "check" row of wildmag.2da
		const SurgeSpell& surgeSpell = gamedata->GetSurgeSpell(check - 1);
		const String s1 = core->GetString(HCStrings::WildSurge, STRING_FLAGS::NONE);
		const String s2 = core->GetString(surgeSpell.message, STRING_FLAGS::NONE);
		displaymsg->DisplayStringName(s1 + u" " + s2, GUIColors::WHITE, this);

		if (!gamedata->Exists(surgeSpell.spell, IE_SPL_CLASS_ID)) {
			// handle the hardcoded cases - they'll also fail here
			if (!HandleHardcodedSurge(surgeSpell.spell, spl, caster)) {
				// free the spell handle because we need to return
				gamedata->FreeSpell(spl, oldSpellResRef, false);
				return 0;
			}
		} else {
			// finally change the spell
			// the hardcoded bunch does it on its own when needed
			SpellResRef = surgeSpell.spell;
		}
	}

	gamedata->FreeSpell(spl, oldSpellResRef, false);
	return 1;
}

bool Scriptable::HandleHardcodedSurge(const ResRef& surgeSpell, const Spell* spl, Actor* caster)
{
	// format: ID or ID.param1 or +SPELLREF
	int types = caster->spellbook.GetTypes();
	int lvl = spl->SpellLevel - 1;
	int count, i, tmp, tmp3;
	Scriptable* target = NULL;
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
			if (objects.LastSpellTarget) {
				target = area->GetActorByGlobalID(objects.LastSpellTarget);
				if (!target) {
					target = core->GetGame()->GetActorByGlobalID(objects.LastSpellTarget);
				}
			}
			if (objects.LastTargetPos.IsInvalid()) {
				targetpos = objects.LastTargetPos;
			} else if (target) {
				targetpos = target->Pos;
			}
			// SpellResRef still contains the original spell and we need to keep it that way
			// as any of the rerolls could result in a "spell cast normally" (non-surge)
			for (i = 0; i < count; i++) {
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
			for (i = 0; i < types; i++) {
				unsigned int spellCount = caster->spellbook.GetKnownSpellsCount(i, lvl);
				if (!spellCount) continue;
				int id = core->Roll(1, spellCount, -1);
				const CREKnownSpell* ck = caster->spellbook.GetKnownSpell(i, lvl, id);
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
		case ST_PROXIMITY:
			return u"Proximity";
		case ST_TRIGGER:
			return u"Trigger";
		case ST_TRAVEL:
			return u"Travel";
		case ST_DOOR:
			return u"Door";
		case ST_CONTAINER:
			return u"Container";
		case ST_AREA:
			return u"Area";
		case ST_GLOBAL:
			return u"Global";
		case ST_ACTOR:
			return As<const Actor>()->GetLongName();
		default:
			return u"NONE";
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
	const Actor* actor = static_cast<const Actor*>(this);
	if (actor->GetStat(IE_AURACLEANSING)) {
		AuraCooldown = 0;
		if (core->HasFeedback(FT_STATES)) displaymsg->DisplayConstantStringName(HCStrings::AuraCleansed, GUIColors::WHITE, this);
		return false;
	}

	// sorry, you'll have to recover first
	return true;
}

unsigned int Scriptable::GetVisualRange() const
{
	if (pst_flags || Type != ST_ACTOR) {
		// everyone uses the same range
		return VOODOO_VISUAL_RANGE;
	}
	const Actor* actor = static_cast<const Actor*>(this);
	return actor->GetStat(IE_VISUALRANGE);
}

bool Scriptable::TimerActive(ieDword ID)
{
	const auto& tit = scriptTimers.find(ID);
	if (tit == scriptTimers.end()) {
		return false;
	}
	return tit->second > core->GetGame()->GameTime;
}

bool Scriptable::TimerExpired(ieDword ID)
{
	const auto& tit = scriptTimers.find(ID);
	if (tit == scriptTimers.end()) {
		return false;
	}
	if (tit->second <= core->GetGame()->GameTime) {
		// expired timers become inactive after being checked
		scriptTimers.erase(tit);
		return true;
	}
	return false;
}

void Scriptable::StartTimer(ieDword ID, ieDword expiration)
{
	ieDword newTime = core->GetGame()->GameTime + expiration * core->Time.defaultTicksPerSec;
	const auto& tit = scriptTimers.find(ID);
	if (tit != scriptTimers.end()) {
		tit->second = newTime;
		return;
	}
	scriptTimers.emplace(ID, newTime);
}
}
