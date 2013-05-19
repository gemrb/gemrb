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

#include "win32def.h"
#include "strrefs.h"
#include "ie_cursors.h"

#include "Audio.h"
#include "DisplayMessage.h"
#include "Game.h"
#include "GameData.h"
#include "Interface.h"
#include "Item.h"
#include "Font.h"
#include "Map.h"
#include "Projectile.h"
#include "Spell.h"
#include "SpriteCover.h"
#include "TileMap.h"
#include "Video.h"
#include "GameScript/GSUtils.h"
#include "GameScript/Matching.h" // MatchActor
#include "GUI/GameControl.h"
//#include "GUI/Window.h"
#include "Scriptable/InfoPoint.h"

#include <cassert>
#include <cmath>

namespace GemRB {

#define YESNO(x) ( (x)?"Yes":"No")

// we start this at a non-zero value to make debugging easier
static ieDword globalActorCounter = 10000;
static bool startActive = false;
static bool third = false;

/***********************
 *  Scriptable Class   *
 ***********************/
Scriptable::Scriptable(ScriptableType type)
{
	Type = type;
	for (int i = 0; i < MAX_SCRIPTS; i++) {
		Scripts[i] = NULL;
	}
	overHeadText = NULL;
	overHeadTextPos.empty();
	textDisplaying = 0;
	timeStartDisplaying = 0;
	scriptName[0] = 0;

	LastAttacker = 0;
	LastCommander = 0;
	LastProtector = 0;
	LastProtectee = 0;
	LastTargetedBy = 0;
	LastHitter = 0;
	LastHelp = 0;
	LastTrigger = 0;
	LastSeen = 0;
	LastTalker = 0;
	LastHeard = 0;
	LastSummoner = 0;
	LastFollowed = 0;
	LastMarked = 0;
	LastMarkedSpell = 0;

	DialogName = 0;
	CurrentAction = NULL;
	CurrentActionState = 0;
	CurrentActionTarget = 0;
	CurrentActionInterruptable = true;
	CurrentActionTicks = 0;
	UnselectableTimer = 0;
	Ticks = 0;
	AdjustedTicks = 0;
	ScriptTicks = 0;
	IdleTicks = 0;
	AuraTicks = 100;
	TriggerCountdown = 0;
	Dialog[0] = 0;

	globalID = ++globalActorCounter;
	if (globalActorCounter == 0) {
		error("Scriptable", "GlobalID overflowed, quitting due to too many actors.");
	}

	WaitCounter = 0;
	if (Type == ST_ACTOR) {
		InternalFlags = IF_VISIBLE | IF_USEDSAVE;
		if (startActive) {
			InternalFlags |= IF_ACTIVE;
		}
	} else {
		InternalFlags = IF_ACTIVE | IF_VISIBLE | IF_NOINT;
	}
	area = 0;
	Pos.x = 0;
	Pos.y = 0;

	LastSpellOnMe = 0xffffffff;
	ResetCastingState(NULL);
	InterruptCasting = false;
	locals = new Variables();
	locals->SetType( GEM_VARIABLES_INT );
	locals->ParseKey( 1 );
	InitTriggers();
	AddTrigger(TriggerEntry(trigger_oncreation));

	memset( script_timers,0, sizeof(script_timers));
	startActive = core->HasFeature(GF_START_ACTIVE);
	third = core->HasFeature(GF_3ED_RULES);
}

Scriptable::~Scriptable(void)
{
	if (CurrentAction) {
		ReleaseCurrentAction();
	}
	ClearActions();
	for (int i = 0; i < MAX_SCRIPTS; i++) {
		if (Scripts[i]) {
			delete( Scripts[i] );
		}
	}
	if (overHeadText) {
		core->FreeString( overHeadText );
	}
	if (locals) {
		delete( locals );
	}
}

void Scriptable::SetScriptName(const char* text)
{
	//if (text && text[0]) { //this leaves some uninitialized bytes
	//lets hope this won't break anything
	if (text) {
		strnspccpy( scriptName, text, 32 );
	}
}

/** Gets the DeathVariable */
const char* Scriptable::GetScriptName(void) const
{
	return scriptName;
}

void Scriptable::SetDialog(const char *resref) {
		if (gamedata->Exists(resref, IE_DLG_CLASS_ID) ) {
			strnuprcpy(Dialog, resref, 8);
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
		error("Scriptable", "Invalid map set!\n");
	}
	area = map;
}

//ai is nonzero if this is an actor currently in the party
//if the script level is AI_SCRIPT_LEVEL, then we need to
//load an AI script (.bs) instead of (.bcs)
void Scriptable::SetScript(const ieResRef aScript, int idx, bool ai)
{
	if (idx >= MAX_SCRIPTS) {
		error("Scriptable", "Invalid script index!\n");
	}
	if (Scripts[idx]) {
		delete Scripts[idx];
	}
	Scripts[idx] = NULL;
	// NONE is an 'invalid' script name, never used seriously
	// This hack is to prevent flooding of the console
	if (aScript[0] && stricmp(aScript, "NONE") ) {
		if (idx!=AI_SCRIPT_LEVEL) ai = false;
		Scripts[idx] = new GameScript( aScript, this, idx, ai );
	}
}

void Scriptable::SetScript(int index, GameScript* script)
{
	if (index >= MAX_SCRIPTS) {
		Log(ERROR, "Scriptable", "Invalid script index!");
		return;
	}
	if (Scripts[index] ) {
		delete Scripts[index];
	}
	Scripts[index] = script;
}

void Scriptable::SetSpellResRef(ieResRef resref) {
	strnuprcpy(SpellResRef, resref, 8);
}

void Scriptable::DisplayHeadText(const char* text)
{
	if (overHeadText) {
		core->FreeString( overHeadText );
	}
	overHeadTextPos.empty();
	if (text) {
		overHeadText = strdup(text);
		timeStartDisplaying = core->GetGame()->Ticks;
		textDisplaying = 1;
	}
	else {
		timeStartDisplaying = 0;
		textDisplaying = 0;
	}
}

/* 'fix' the current overhead text in the current position */
void Scriptable::FixHeadTextPos()
{
	overHeadTextPos = Pos;
}

#define MAX_DELAY  6000
static const Color black={0,0,0,0};

void Scriptable::DrawOverheadText(const Region &screen)
{
	unsigned long time = core->GetGame()->Ticks;
	Palette *palette = NULL;

	if (!textDisplaying)
		return;

	time -= timeStartDisplaying;

	Font* font = core->GetFont( 1 );
	if (time >= MAX_DELAY) {
		textDisplaying = 0;
		return;
	} else {
		time = (MAX_DELAY-time)/10;
		if (time<256) {
			ieByte time2 = time; // shut up narrowing warnings
			const Color overHeadColor = {time2,time2,time2,time2};
			palette = core->CreatePalette(overHeadColor, black);
		}
	}

	int cs = 100;
	if (Type == ST_ACTOR) {
		cs = ((Selectable *) this)->size*50;
	}

	short x, y;
	if (overHeadTextPos.isempty()) {
		x = Pos.x;
		y = Pos.y;
	} else {
		x = overHeadTextPos.x;
		y = overHeadTextPos.y;
	}

	Region rgn( x-100+screen.x, y - cs + screen.y, 200, 400 );
	font->Print( rgn, ( unsigned char * ) overHeadText,
		palette?palette:core->InfoTextPalette, IE_FONT_ALIGN_CENTER | IE_FONT_ALIGN_TOP, false );
	gamedata->FreePalette(palette);
}

void Scriptable::ImmediateEvent()
{
	InternalFlags |= IF_FORCEUPDATE;
}

bool Scriptable::IsPC() const
{
	if(Type == ST_ACTOR) {
		if (((Actor *) this)->GetStat(IE_EA) <= EA_CHARMED) {
			return true;
		}
	}
	return false;
}

void Scriptable::Update()
{
	Ticks++;
	AdjustedTicks++;
	AuraTicks++;

	Actor *act = NULL;
	if (Type == ST_ACTOR) {
		act = (Actor *) this;
	}

	if (UnselectableTimer) {
		UnselectableTimer--;
		if (!UnselectableTimer) {
			if (act) {
				act->SetCircleSize();
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
	if (Ticks % 16 != globalID % 16)
		return;

	ieDword actorState = 0;
	if (Type == ST_ACTOR)
		actorState = ((Actor *)this)->Modified[IE_STATE_ID];

	// Dead actors only get one chance to run a new script.
	if ( (InternalFlags& (IF_REALLYDIED|IF_JUSTDIED))==IF_REALLYDIED)
		return;

	ScriptTicks++;

	// If no action is running, we've had triggers set recently or we haven't checked recently, do a script update.
	bool needsUpdate = (!CurrentAction) || (TriggerCountdown > 0) || (IdleTicks > 15);

	// Also do a script update if one was forced..
	if (InternalFlags & IF_FORCEUPDATE) {
		needsUpdate = true;
		InternalFlags &= ~IF_FORCEUPDATE;
	}
	// TODO: force for all on-screen actors

	// Charmed actors don't get frequent updates.
	if ((actorState & STATE_CHARMED) && (IdleTicks < 5))
		needsUpdate = false;

	if (!needsUpdate) {
		IdleTicks++;
		return;
	}

	if (triggers.size())
		TriggerCountdown = 5;
	IdleTicks = 0;
	InternalFlags &= ~IF_JUSTDIED;
	if (TriggerCountdown > 0)
		TriggerCountdown--;
	// TODO: set TriggerCountdown once we have real triggers

	ExecuteScript(MAX_SCRIPTS);
}

void Scriptable::ExecuteScript(int scriptCount)
{
	// area scripts still run for at least the current area, in bg1 (see ar2631, confirmed by testing)
	// but not in bg2 (kill Abazigal in ar6005)
	if (core->GetGameControl()->GetScreenFlags()&SF_CUTSCENE) {
		if (! (core->HasFeature(GF_CUTSCENE_AREASCRIPTS) && Type == ST_AREA)) {
			return;
		}
	}

	// Don't abort if there is a running non-interruptable action.
	if ((InternalFlags & IF_NOINT) && (CurrentAction || GetNextAction())) {
		return;
	}
	if (!CurrentActionInterruptable) {
		// sanity check
		if (!CurrentAction && !GetNextAction())
			error("Scriptable", "No current action and no next action.\n");
		return;
	}

	bool changed = false;

	// if party AI is disabled, don't run non-override scripts
	if (Type == ST_ACTOR && ((Actor *) this)->InParty && (core->GetGame()->ControlStatus & CS_PARTY_AI))
		scriptCount = 1;
	// TODO: hardcoded action hacks
	if (Type == ST_ACTOR) {
		//TODO: add stuff here that overrides actions (like Panic, etc)
		changed |= ((Actor *) this)->OverrideActions();
	}

	bool continuing = false, done = false;
	for (scriptlevel = 0;scriptlevel<scriptCount;scriptlevel++) {
		GameScript *Script = Scripts[scriptlevel];
		if (Script) {
			changed |= Script->Update(&continuing, &done);
		}

		/* scripts are not concurrent, see WAITPC override script for example */
		if (done) break;
	}

	if (changed)
		InitTriggers();

	if (Type == ST_ACTOR) {
		//TODO: add stuff here about idle actions
		((Actor *) this)->IdleActions(CurrentAction!=NULL);
	}
}

void Scriptable::AddAction(Action* aC)
{
	if (!aC) {
		print("[GameScript]: NULL action encountered for %s!", scriptName);
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
	// FIXME: area check hack until fuzzie fixes scripts here
	if (!CurrentAction && !GetNextAction() && area) {
		if (actionflags[aC->actionID] & AF_INSTANT) {
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
		print("[GameScript]: NULL action encountered for %s!", scriptName);
		return;
	}
	InternalFlags|=IF_ACTIVE;
	actionQueue.push_front( aC );
	aC->IncRef();
}

Action* Scriptable::GetNextAction() const
{
	if (actionQueue.size() == 0) {
		return NULL;
	}
	return actionQueue.front();
}

Action* Scriptable::PopNextAction()
{
	if (actionQueue.size() == 0) {
		return NULL;
	}
	Action* aC = actionQueue.front();
	actionQueue.pop_front();
	return aC;
}

void Scriptable::ClearActions()
{
	ReleaseCurrentAction();
	for (unsigned int i = 0; i < actionQueue.size(); i++) {
		Action* aC = actionQueue.front();
		actionQueue.pop_front();
		aC->Release();
	}
	actionQueue.clear();
	WaitCounter = 0;
	LastTarget = 0;

	if (Type == ST_ACTOR) {
		Interrupt();
	} else {
		NoInterrupt();
	}
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
				print("Last action: %d", lastAction);
			}
			assert(CurrentActionTicks == 0 && CurrentActionState == 0);
			CurrentAction = PopNextAction();
		} else {
			CurrentActionTicks++;
		}
		if (!CurrentAction) {
			ClearActions();
			lastAction = -1;
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
	return me->GetNextStep()!=NULL;
}

void Scriptable::SetWait(unsigned long time)
{
	WaitCounter = time;
}

unsigned long Scriptable::GetWait() const
{
	return WaitCounter;
}

void Scriptable::LeaveDialog()
{
	AddTrigger(TriggerEntry(trigger_wasindialog));
}

void Scriptable::Hide()
{
	InternalFlags &=~(IF_VISIBLE);
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

void Scriptable::InitTriggers()
{
	triggers.clear();
}

void Scriptable::AddTrigger(TriggerEntry trigger)
{
	triggers.push_back(trigger);
	ImmediateEvent();

	assert(trigger.triggerID < MAX_TRIGGERS);
	if (triggerflags[trigger.triggerID] & TF_SAVED) {
		//TODO: if LastTrigger is still overwritten by script action blocks, store this in a separate field and copy it back when the block ends
		//Log(WARNING, "Scriptable", "%s: Added LastTrigger: %d for trigger %d\n", scriptName, trigger.param1, trigger.triggerID);
		LastTrigger = trigger.param1;
	}
}

bool Scriptable::MatchTrigger(unsigned short id, ieDword param) {
	for (std::list<TriggerEntry>::iterator m = triggers.begin(); m != triggers.end (); m++) {
		TriggerEntry &trigger = *m;
		if (trigger.triggerID != id)
			continue;
		if (param && trigger.param1 != param)
			continue;
		return true;
	}

	return false;
}

bool Scriptable::MatchTriggerWithObject(unsigned short id, class Object *obj, ieDword param) {
	for (std::list<TriggerEntry>::iterator m = triggers.begin(); m != triggers.end (); m++) {
		TriggerEntry &trigger = *m;
		if (trigger.triggerID != id)
			continue;
		if (param && trigger.param2 != param)
			continue;
		if (!MatchActor(this, trigger.param1, obj))
			continue;
		return true;
	}

	return false;
}

const TriggerEntry *Scriptable::GetMatchingTrigger(unsigned short id, unsigned int notflags) {
	for (std::list<TriggerEntry>::iterator m = triggers.begin(); m != triggers.end (); m++) {
		TriggerEntry &trigger = *m;
		if (trigger.triggerID != id)
			continue;
		if (notflags & trigger.flags)
			continue;
		return &*m;
	}

	return NULL;
}

void Scriptable::CreateProjectile(const ieResRef SpellResRef, ieDword tgt, int level, bool fake)
{
	Spell* spl = gamedata->GetSpell( SpellResRef );
	Actor *caster = NULL;

	//PST has a weird effect, called Enoll Eva's duplication
	//it creates every projectile of the affected actor twice
	int projectileCount = 1;
	if (Type == ST_ACTOR) {
		caster = (Actor *) this;
		if (spl->Flags&SF_HOSTILE) {
			caster->CureSanctuary();
		}

		// check if a wild surge ordered us to replicate the projectile
		projectileCount = caster->wildSurgeMods.num_castings;
		if (!projectileCount) {
			projectileCount = 1;
		}
	}

	if (core->HasFeature(GF_PST_STATE_FLAGS) && (Type == ST_ACTOR)) {
		if (caster->GetStat(IE_STATE_ID)&STATE_EE_DUPL) {
			//seriously, wild surges and EE in the same game?
			//anyway, it would be too many duplications
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
			// check for target (type) change
			int count, i;
			Actor *newact = NULL;
			SPLExtHeader *seh = NULL;
			Effect *fx = NULL;
			switch (caster->wildSurgeMods.target_change_type) {
				case WSTC_SETTYPE:
					seh = &spl->ext_headers[SpellHeader];
					for (i=0; i < seh->FeatureCount; i++) {
						seh->features[i].Target = caster->wildSurgeMods.target_type;
					}
					// we need to fetch the projectile, so the effect queue is created
					// (skipped above)
					pro = spl->GetProjectile(this, SpellHeader, level, LastTargetPos);
					pro->SetCaster(GetGlobalID(), level);
					break;
				case WSTC_ADDTYPE:
					// TODO: unhardcode to allow for mixing all the target types
					// caster gets selftargeting fx when the projectile is fetched above
					seh = &spl->ext_headers[SpellHeader];
					for (i=0; i < seh->FeatureCount; i++) {
						if (seh->features[i].Target == FX_TARGET_SELF) {
							seh->features[i].Target = caster->wildSurgeMods.target_type;
						} else {
							// also apply to the caster
							fx = seh->features+i;
							core->ApplyEffect(fx, caster, caster);
						}
					}
					// we need to refetch the projectile, so the effect queue is created
					pro = spl->GetProjectile(this, SpellHeader, level, LastTargetPos);
					pro->SetCaster(GetGlobalID(), level);
					break;
				case WSTC_RANDOMIZE:
					count = area->GetActorCount(false);
					newact = area->GetActor(core->Roll(1,count,-1), false);
					if (count > 1 && newact == caster) {
						while (newact == caster) {
							newact = area->GetActor(core->Roll(1,count,-1), false);
						}
					}
					if (tgt) {
						LastTarget = newact->GetGlobalID();
						LastTargetPos = newact->Pos;
					} else {
						// no better idea; I wonder if the original randomized point targets at all
						LastTargetPos = newact->Pos;
					}

					// make it also work for self-targeting spells:
					// change the payload or this was all in vain
					seh = &spl->ext_headers[SpellHeader];
					for (i=0; i < seh->FeatureCount; i++) {
						if (seh->features[i].Target == FX_TARGET_SELF) {
							seh->features[i].Target = FX_TARGET_PRESET;
						}
					}
					// we need to fetch the projectile, so the effect queue is created
					// (skipped above)
					pro = spl->GetProjectile(this, SpellHeader, level, LastTargetPos);
					pro->SetCaster(GetGlobalID(), level);
					break;
				default: //0 - do nothing
					break;
			}

			// apply the saving throw mod
			if (caster->wildSurgeMods.saving_throw_mod) {
				seh = &spl->ext_headers[SpellHeader];
				for (i=0; i < seh->FeatureCount; i++) {
					seh->features[i].SavingThrowBonus += caster->wildSurgeMods.saving_throw_mod;
				}
			}

			// change the projectile
			if (caster->wildSurgeMods.projectile_id) {
				spl->ext_headers[SpellHeader].ProjectileAnimation = caster->wildSurgeMods.projectile_id;
				// make it also work for self-targeting spells:
				// change the payload or this was all in vain
				seh = &spl->ext_headers[SpellHeader];
				for (i=0; i < seh->FeatureCount; i++) {
					if (seh->features[i].Target == FX_TARGET_SELF) {
						seh->features[i].Target = FX_TARGET_PRESET;
					}
				}
				// we need to refetch the projectile, so the new one is used
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

		if (tgt) {
			area->AddProjectile(pro, origin, tgt, fake);
		} else {
			area->AddProjectile(pro, origin, LastTargetPos);
		}
	}

	ieDword spellnum=ResolveSpellNumber( SpellResRef );
	if (spellnum!=0xffffffff) {
		area->SeeSpellCast(this, spellnum);

		// spellcasting feedback
		if (third) {
			// only display it for party friendly creatures - enemies require a successful spellcraft check
			if (Type == ST_ACTOR) {
				Actor *caster = ((Actor *) this);
				if (caster->GetStat(IE_EA) <= EA_CONTROLLABLE) {
					DisplaySpellCastMessage(tgt, spl);
				}
			}
		} else {
			DisplaySpellCastMessage(tgt, spl);
		}
	}
	// only trigger the autopause when in combat or buffing gets very annoying
	if (core->GetGame()->CombatCounter) {
		core->Autopause(AP_SPELLCAST, this);
	}

	gamedata->FreeSpell(spl, SpellResRef, false);
}

void Scriptable::DisplaySpellCastMessage(ieDword tgt, Spell *spl)
{
	// caster - Casts spellname : target OR
	// caster - spellname : target (repeating spells)
	Scriptable *target = NULL;
	if (tgt) {
		target = area->GetActorByGlobalID(tgt);
		if (!target) {
			target=core->GetGame()->GetActorByGlobalID(tgt);
		}
	}
	char* spell = core->GetString(spl->SpellName);
	if (stricmp(spell, "") && Type == ST_ACTOR) {
		char* msg = core->GetString(displaymsg->GetStringReference(STR_ACTION_CAST), 0);
		char *tmp;
		if (target) {
			tmp = (char *) malloc(strlen(msg)+strlen(spell)+strlen(target->GetName(-1))+5);
			sprintf(tmp, "%s %s : %s", msg, spell, target->GetName(-1));
		} else {
			tmp = (char *) malloc(strlen(spell)+strlen(GetName(-1))+4);
			sprintf(tmp, "%s : %s", spell, GetName(-1));
		}
		displaymsg->DisplayStringName(tmp, DMC_WHITE, this);
		core->FreeString(msg);
		free(tmp);
	}
	core->FreeString(spell);
}

void Scriptable::SendTriggerToAll(TriggerEntry entry)
{
	Actor** nearActors = area->GetAllActorsInRadius(Pos, GA_NO_DEAD, 15*10);
	int i=0;
	while(nearActors[i]!=NULL) {
		nearActors[i]->AddTrigger(entry);
		++i;
	}
	area->AddTrigger(entry);
	free(nearActors);
}

inline void Scriptable::ResetCastingState(Actor *caster) {
	SpellHeader = -1;
	SpellResRef[0] = 0;
	LastTarget = 0;
	LastTargetPos.empty();
	if (caster) {
		memset(&(caster->wildSurgeMods), 0, sizeof(caster->wildSurgeMods));
	}
}

void Scriptable::CastSpellPointEnd(int level, int no_stance)
{
	Actor *caster = NULL;
	Spell* spl = gamedata->GetSpell(SpellResRef); // this was checked before we got here
	if (!spl) {
		return;
	}
	int nSpellType = spl->SpellType;
	gamedata->FreeSpell(spl, SpellResRef, false);

	if (Type == ST_ACTOR) {
		caster = ((Actor *) this);
		if (!no_stance) {
			caster->SetStance(IE_ANI_CONJURE);
			caster->CureInvisibility();
		}
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
		LastTargetPos.empty();
		return;
	}

	if (LastTargetPos.isempty()) {
		SpellHeader = -1;
		return;
	}

	if (!SpellResRef[0]) {
		return;
	}
	if (!area) {
		Log(ERROR, "Scriptable", "CastSpellPointEnd: lost area, skipping %s!", SpellResRef);
		ResetCastingState(caster);
		return;
	}

	if (caster && caster->PCStats) {
		caster->PCStats->RegisterFavourite(SpellResRef, FAV_SPELL);
	}

	CreateProjectile(SpellResRef, 0, level, false);
	//FIXME: this trigger affects actors whom the caster sees, not just the caster itself
	// the original engine saves lasttrigger only in case of SpellCast, so we have to differentiate
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

	ResetCastingState(caster);
}

void Scriptable::CastSpellEnd(int level, int no_stance)
{
	Actor *caster = NULL;
	Spell* spl = gamedata->GetSpell(SpellResRef); // this was checked before we got here
	if (!spl) {
		return;
	}
	int nSpellType = spl->SpellType;
	gamedata->FreeSpell(spl, SpellResRef, false);
	if (Type == ST_ACTOR) {
		caster = ((Actor *) this);
		if (!no_stance) {
			caster->SetStance(IE_ANI_CONJURE);
			caster->CureInvisibility();
		}
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
		LastTarget = 0;
		return;
	}
	if (!LastTarget) {
		SpellHeader = -1;
		return;
	}
	if (!SpellResRef[0]) {
		return;
	}
	if (!area) {
		Log(ERROR, "Scriptable", "CastSpellEnd: lost area, skipping %s!", SpellResRef);
		ResetCastingState(caster);
		return;
	}

	if (caster && caster->PCStats) {
		caster->PCStats->RegisterFavourite(SpellResRef, FAV_SPELL);
	}

	//if the projectile doesn't need to follow the target, then use the target position
	CreateProjectile(SpellResRef, LastTarget, level, GetSpellDistance(SpellResRef, this)==0xffffffff);
	//FIXME: this trigger affects actors whom the caster sees, not just the caster itself
	// the original engine saves lasttrigger only in case of SpellCast, so we have to differentiate
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

	// TODO: maybe it should be set on effect application, since the data uses it with dispel magic and true sight a lot
	Actor *target = area->GetActorByGlobalID(LastTarget);
	if (target) {
		target->AddTrigger(TriggerEntry(trigger_spellcastonme, GetGlobalID(), spellID));
		target->LastSpellOnMe = spellID;
	}

	ResetCastingState(caster);
}

// check for input sanity and good casting conditions
int Scriptable::CanCast(const ieResRef SpellResRef, bool verbose) {
	Spell* spl = gamedata->GetSpell(SpellResRef);
	if (!spl) {
		SpellHeader = -1;
		Log(ERROR, "Scriptable", "Spell not found, aborting cast!");
		return 0;
	}

	// check for area dead magic
	// tob AR3004 is a dead magic area, but using a script with personal dead magic
	if (area->GetInternalFlag()&AF_DEADMAGIC) {
		// TODO: display fizzling animation
		displaymsg->DisplayConstantStringName(STR_DEADMAGIC_FAIL, DMC_WHITE, this);
		return 0;
	}

	if (spl->Flags&SF_NOT_INDOORS && !(area->AreaType&AT_OUTDOOR)) {
		displaymsg->DisplayConstantStringName(STR_INDOOR_FAIL, DMC_WHITE, this);
		return 0;
	}

	// various individual checks
	if (Type == ST_ACTOR) {
		Actor *actor = (Actor *) this;

		// check for silence
		// only a handful of spells don't have a verbal component -
		// the original hardcoded vocalize and a few more
		// we (also) ignore nonmagic spells
		if (actor->CheckSilenced()) {
			if (!(core->GetSpecialSpell(spl->Name)&SP_SILENCE) && !(spl->Flags&SF_HLA)) {
				Log(WARNING, "Scriptable", "Tried to cast while silenced!");
				return 0;
			}
		}

		// check for personal dead magic
		if (actor->Modified[IE_DEADMAGIC]) {
			// TODO: display fizzling animation
			displaymsg->DisplayConstantStringName(STR_DEADMAGIC_FAIL, DMC_WHITE, this);
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
			if (chance >= roll) {
				failed = true;
			}
			break;
		case IE_SPL_WIZARD:
			chance = actor->GetSpellFailure(true);
			if (chance >= roll) {
				failed = true;
			}
			break;
		case IE_SPL_INNATE:
			chance = actor->Modified[IE_SPELLFAILUREINNATE];
			if (chance >= roll) {
				failed = true;
			}
			break;
		}
		if (verbose && chance && third) {
			// ~Spell Failure check: Roll d100 %d vs. Spell failure chance %d~
			displaymsg->DisplayRollStringName(40955, DMC_LIGHTGREY, actor, roll, chance);
		}
		if (failed) {
			// TODO: display fizzling animation
			displaymsg->DisplayConstantStringName(STR_MISCASTMAGIC, DMC_WHITE, this);
			return 0;
		}
	}

	return 1;
}

// checks if a party-friendly actor is nearby and if so, if it reckognizes the spell
// this enemy just started casting
void Scriptable::SpellcraftCheck(const Actor *caster, const ieResRef SpellResRef)
{
	if (!third || !caster || caster->GetStat(IE_EA) <= EA_CONTROLLABLE || !area) {
		return;
	}

	Spell* spl = gamedata->GetSpell(SpellResRef);
	assert(spl); // only a bad surge could make this fail and we want to catch it
	int AdjustedSpellLevel = spl->SpellLevel + 15;
	Actor **neighbours = area->GetAllActorsInRadius(caster->Pos, GA_NO_DEAD|GA_NO_ENEMY|GA_NO_SELF|GA_NO_UNSCHEDULED, 10*caster->GetBase(IE_VISUALRANGE));
	Actor **poi = neighbours;
	while (*poi) {
		Actor *detective = *poi;
		// disallow neutrals from helping the party
		if (detective->GetStat(IE_EA) > EA_CONTROLLABLE) {
			poi++;
			continue;
		}
		if ((signed)detective->GetStat(IE_SPELLCRAFT) <= 0) {
			poi++;
			continue;
		}

		// ~Spellcraft check (d20 roll + Spellcraft level + int mod) %d vs. (spell level + 15)  = %d.   (Int mod = %d)~
		int Spellcraft = core->Roll(1, 20, 0) + detective->GetStat(IE_SPELLCRAFT);
		int IntMod = detective->GetAbilityBonus(IE_INT);

		if ((Spellcraft + IntMod) > AdjustedSpellLevel) {
			char tmpstr[100];
			memset(tmpstr, 0, sizeof(tmpstr));
			// eg. .:Casts Word of Recall:.
			snprintf(tmpstr, sizeof(tmpstr), ".:%s %s:.", core->GetString(displaymsg->GetStringReference(STR_CASTS)), core->GetString(spl->SpellName));
			DisplayHeadText(tmpstr);
			displaymsg->DisplayRollStringName(39306, DMC_LIGHTGREY, detective, Spellcraft+IntMod, AdjustedSpellLevel, IntMod);
			break;
		}
		poi++;
	}
	gamedata->FreeSpell(spl, SpellResRef, false);
	free(neighbours);
}

//set target as point
//if spell needs to be depleted, do it
//if spell is illegal stop casting
int Scriptable::CastSpellPoint( const Point &target, bool deplete, bool instant, bool nointerrupt )
{
	LastTarget = 0;
	LastTargetPos.empty();
	Actor *actor = NULL;
	if (Type == ST_ACTOR) {
		actor = (Actor *) this;
		if (actor->HandleCastingStance(SpellResRef, deplete, instant) ) {
			Log(ERROR, "Scriptable", "Spell not known or memorized, aborting cast!");
			return -1;
		}
	}
	if(!nointerrupt && !CanCast(SpellResRef)) {
		SpellResRef[0] = 0;
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
	}
	return SpellCast(instant);
}

//set target as actor (if target isn't actor, use its position)
//if spell needs to be depleted, do it
//if spell is illegal stop casting
int Scriptable::CastSpell( Scriptable* target, bool deplete, bool instant, bool nointerrupt )
{
	LastTarget = 0;
	LastTargetPos.empty();
	Actor *actor = NULL;
	if (Type == ST_ACTOR) {
		actor = (Actor *) this;
		if (actor->HandleCastingStance(SpellResRef, deplete, instant) ) {
			Log(ERROR, "Scriptable", "Spell not known or memorized, aborting cast!");
			return -1;
		}
	}

	assert(target);

	if(!nointerrupt && !CanCast(SpellResRef)) {
		SpellResRef[0] = 0;
		if (actor) {
			actor->SetStance(IE_ANI_READY);
		}
		return -1;
	}

	LastTargetPos = target->Pos;
	if (target->Type==ST_ACTOR) {
		LastTarget = target->GetGlobalID();
	}

	if(!CheckWildSurge()) {
		return -1;
	}

	if (!instant) {
		SpellcraftCheck(actor, SpellResRef);
	}
	return SpellCast(instant);
}

static EffectRef fx_force_surge_modifier_ref = { "ForceSurgeModifier", -1 };

//start spellcasting (common part)
int Scriptable::SpellCast(bool instant)
{
	Spell* spl = gamedata->GetSpell(SpellResRef); // this was checked before we got here
	Actor *actor = NULL;
	int level = 0;
	if (Type == ST_ACTOR) {
		actor = (Actor *) this;

		//The ext. index is here to calculate the casting time
		level = actor->GetCasterLevel(spl->SpellType);
		SpellHeader = spl->GetHeaderIndexFromLevel(level);
	} else {
		SpellHeader = 0;
	}

	SPLExtHeader *header = spl->GetExtHeader(SpellHeader);
	int casting_time = (int)header->CastingTime;
	// how does this work for non-actors exactly?
	if (actor) {
		// The mental speed effect can shorten or lengthen the casting time.
		casting_time -= (int)actor->Modified[IE_MENTALSPEED];
		// maybe also add a random lucky roll as for weapon speed / initiative
		if (casting_time < 0) {
			casting_time = 0;
		} else if (casting_time > 10) {
			casting_time = 10;
		}
	}
	// this is a guess which seems approximately right so far (same as in the bg2 manual, except that it may be a combat round instead)
	int duration = (casting_time*core->Time.round_size) / 10;
	if (instant) {
		duration = 0;
	}
	if (actor) {
		//cfb
		EffectQueue *fxqueue = spl->GetEffectBlock(this, this->Pos, -1, level);
		fxqueue->SetOwner(actor);
		if (!(actor->Modified[IE_AVATARREMOVAL] || instant)) {
			ieDword gender = actor->GetCGGender();
			spl->AddCastingGlow(fxqueue, duration, gender);
		}
		fxqueue->AddAllEffects(actor, actor->Pos);
		delete fxqueue;
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
		ieResRef OldSpellResRef;
		memcpy(OldSpellResRef, SpellResRef, sizeof(OldSpellResRef));
		Spell *spl = gamedata->GetSpell( OldSpellResRef ); // this was checked before we got here
		// ignore non-magic "spells"
		if (!(spl->Flags&(SF_HLA|SF_TRIGGER) )) {
			int check = roll + caster->GetCasterLevel(spl->SpellType) + caster->Modified[IE_SURGEMOD];
			if (caster->Modified[IE_CHAOSSHIELD]) {
				//avert the surge and decrease the chaos shield counter
				check = 0;
				caster->fxqueue.DecreaseParam1OfEffect(fx_chaosshield_ref,1);
				displaymsg->DisplayConstantStringName(STR_CHAOSSHIELD,DMC_LIGHTGREY,caster);
			}

			// hundred or more means a normal cast; same for negative values (for absurd antisurge modifiers)
			if ((check > 0) && (check < 100) ) {
				// display feedback: Wild Surge: bla bla
				char *s1 = core->GetString(displaymsg->GetStringReference(STR_WILDSURGE), 0);
				char *s2 = core->GetString(core->SurgeSpells[check-1].message, 0);
				char *s3 = (char *) malloc(strlen(s1)+strlen(s2)+2);
				sprintf(s3, "%s %s", s1, s2);
				core->FreeString(s1);
				core->FreeString(s2);
				displaymsg->DisplayStringName(s3, DMC_WHITE, this);
				free(s3);

				// lookup the spell in the "check" row of wildmag.2da
				ieResRef surgeSpellRef;
				CopyResRef(surgeSpellRef, core->SurgeSpells[check-1].spell);

				if (!gamedata->Exists(surgeSpellRef, IE_SPL_CLASS_ID)) {
					// handle the hardcoded cases - they'll also fail here
					if (!HandleHardcodedSurge(surgeSpellRef, spl, caster)) {
						//free the spell handle because we need to return
						gamedata->FreeSpell(spl, OldSpellResRef, false);
						return 0;
					}
				} else {
					// finally change the spell
					// the hardcoded bunch does it on its own when needed
					CopyResRef(SpellResRef, surgeSpellRef);
				}
			}
		}

		//free the spell handle
		gamedata->FreeSpell(spl, OldSpellResRef, false);
	}

	return 1;
}

bool Scriptable::HandleHardcodedSurge(ieResRef surgeSpellRef, Spell *spl, Actor *caster)
{
	// format: ID or ID.param1 or +SPELLREF
	int types = caster->spellbook.GetTypes();
	int lvl = spl->SpellLevel-1;
	int count, i, tmp, tmp2, tmp3;
	Scriptable *target = NULL;
	Point targetpos(-1, -1);
	ieResRef newspl;

	int level = caster->GetCasterLevel(spl->SpellType);
	switch (surgeSpellRef[0]) {
		case '+': // cast normally, but also cast SPELLREF first
			core->ApplySpell(surgeSpellRef+1, caster, caster, level);
			break;
		case '0': // cast spell param1 times
			strtok(surgeSpellRef,".");
			count = strtol(strtok(NULL,"."), NULL, 0);
			caster->wildSurgeMods.num_castings = count;
			break;
		case '1': // change projectile (id) to param1
			strtok(surgeSpellRef,".");
			count = strtol(strtok(NULL,"."), NULL, 0);
			caster->wildSurgeMods.projectile_id = count;
			break;
		case '2': // also target target type param1
			strtok(surgeSpellRef,".");
			count = strtol(strtok(NULL,"."), NULL, 0);
			caster->wildSurgeMods.target_type = count;
			caster->wildSurgeMods.target_change_type = WSTC_ADDTYPE;
			break;
		case '3': // (wild surge) roll param1 more times
			strtok(surgeSpellRef,".");
			count = strtol(strtok(NULL,"."), NULL, 0);
			// force surge and then cast
			// force the surge roll to be < 100, so we cast a spell from the surge table
			tmp = caster->Modified[IE_FORCESURGE];
			tmp2 = caster->Modified[IE_SURGEMOD];
			tmp3 = caster->WMLevelMod; // also save caster level; the original didn't reroll the bonus
			caster->Modified[IE_FORCESURGE] = 7;
			caster->Modified[IE_SURGEMOD] = - caster->GetCasterLevel(spl->SpellType); // nulify the bonus
			if (LastTarget) {
				target = area->GetActorByGlobalID(LastTarget);
				if (!target) {
					target = core->GetGame()->GetActorByGlobalID(LastTarget);
				}
			}
			if (!LastTargetPos.isempty()) {
				targetpos = LastTargetPos;
			} else if (target) {
				targetpos = target->Pos;
			}
			// SpellResRef still contains the original spell and we need to keep it that way
			// as any of the rerolls could result in a "spell cast normally" (non-surge)
			for (i=0; i<count; i++) {
				if (target) {
					caster->CastSpell(target, false, true);
					CopyResRef(newspl, SpellResRef);
					caster->WMLevelMod = tmp3;
					caster->CastSpellEnd(level, 1);
				} else {
					caster->CastSpellPoint(targetpos, false, true);
					CopyResRef(newspl, SpellResRef);
					caster->WMLevelMod = tmp3;
					caster->CastSpellPointEnd(level, 1);
				}
				// reset the ref, since CastSpell*End destroyed it
				CopyResRef(SpellResRef, newspl);
			}
			caster->Modified[IE_FORCESURGE] = tmp;
			caster->Modified[IE_SURGEMOD] = tmp2;
			break;
		case '4': // change the target type to param1
			strtok(surgeSpellRef,".");
			count = strtol(strtok(NULL,"."), NULL, 0);
			caster->wildSurgeMods.target_type = count;
			caster->wildSurgeMods.target_change_type = WSTC_SETTYPE;
			break;
		case '5': // change the target to a random actor
			caster->wildSurgeMods.target_change_type = WSTC_RANDOMIZE;
			break;
		case '6': // change saving throw (+param1)
			strtok(surgeSpellRef,".");
			count = strtol(strtok(NULL,"."), NULL, 0);
			caster->wildSurgeMods.saving_throw_mod = count;
			break;
		case '7': // random spell of the same level (FIXME: make an effect out of this?)
			// change this if we ever want the surges to respect the original type
			for (i=0; i<types; i++) {
				unsigned int spellCount = caster->spellbook.GetKnownSpellsCount(i, lvl);
				if (!spellCount) continue;
				int id = core->Roll(1, spellCount, -1);
				CREKnownSpell *ck = caster->spellbook.GetKnownSpell(i, lvl, id);
				if (ck) {
					CopyResRef(SpellResRef, ck->SpellResRef);
					break;
				}
			}
			break;
		case '8': // set projectile speed to param1 %
			strtok(surgeSpellRef,".");
			count = strtol(strtok(NULL,"."), NULL, 0);
			caster->wildSurgeMods.projectile_speed_mod = count;
			break;
		default:
			SpellHeader = -1;
			SpellResRef[0] = 0;
			Log(ERROR, "Scriptable", "New spell not found, aborting cast mid-surge!");
			caster->SetStance(IE_ANI_READY);
			return false;
	}
	return true;
}

bool Scriptable::AuraPolluted()
{
	if (Type != ST_ACTOR) {
		return false;
	}

	// aura pollution happens automatically
	// aura cleansing the usual and magical way
	if (AuraTicks >= core->Time.attack_round_size) {
		AuraTicks = -1;
		return false;
	} else if (CurrentActionTicks == 0 && AuraTicks != 1 && Type == ST_ACTOR) {
		Actor *act = (Actor *) this;
		if (act->GetStat(IE_AURACLEANSING)) {
			AuraTicks = -1;
			displaymsg->DisplayConstantStringName(STR_AURACLEANSED, DMC_WHITE, this);
			return false;
		}
	}

	if (AuraTicks > 0) {
		// sorry, you'll have to recover first
		return true;
	}
	return false;
}

bool Scriptable::TimerActive(ieDword ID)
{
	if (ID>=MAX_TIMER) {
		return false;
	}
	if (script_timers[ID]) {
		return true;
	}
	return false;
}

bool Scriptable::TimerExpired(ieDword ID)
{
	if (ID>=MAX_TIMER) {
		return false;
	}
	if (script_timers[ID] && script_timers[ID] < core->GetGame()->GameTime) {
		// expired timers become inactive after being checked
		script_timers[ID] = 0;
		return true;
	}
	return false;
}

void Scriptable::StartTimer(ieDword ID, ieDword expiration)
{
	if (ID>=MAX_TIMER) {
		Log(ERROR, "Scriptable", "Timer id %d exceeded MAX_TIMER %d",
			ID, MAX_TIMER);
		return;
	}
	script_timers[ID]= core->GetGame()->GameTime + expiration*AI_UPDATE_TIME;
}

/********************
 * Selectable Class *
 ********************/

Selectable::Selectable(ScriptableType type)
	: Scriptable( type )
{
	Selected = false;
	Over = false;
	size = 0;
	cover = NULL;
	circleBitmap[0] = NULL;
	circleBitmap[1] = NULL;
}

void Selectable::SetSpriteCover(SpriteCover* c)
{
	delete cover;
	cover = c;
}

Selectable::~Selectable(void)
{
	delete cover;
}

void Selectable::SetBBox(const Region &newBBox)
{
	BBox = newBBox;
}

void Selectable::DrawCircle(const Region &vp)
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

	if (size<=0) {
		return;
	}
	Color mix;
	Color* col = &selectedColor;
	Sprite2D* sprite = circleBitmap[0];

	if (Selected) {
		sprite = circleBitmap[1];
	} else if (Over) {
		//doing a time dependent flashing of colors
		//if it is too fast, increase the 6 to 7
		unsigned long step;
		step = GetTickCount();
		step = tp_steps [(step >> 7) & 7]*2;
		mix.a = overColor.a;
		mix.r = (overColor.r*step+selectedColor.r*(8-step))/8;
		mix.g = (overColor.g*step+selectedColor.g*(8-step))/8;
		mix.b = (overColor.b*step+selectedColor.b*(8-step))/8;
		col = &mix;
	} else if (IsPC()) {
		col = &overColor;
	}

	if (sprite) {
		core->GetVideoDriver()->BlitSprite( sprite, Pos.x - vp.x, Pos.y - vp.y, true );
	} else {
		// for size >= 2, radii are (size-1)*16, (size-1)*12
		// for size == 1, radii are 12, 9
		int csize = (size - 1) * 4;
		if (csize < 4) csize = 3;
		core->GetVideoDriver()->DrawEllipse( (ieWord) (Pos.x - vp.x), (ieWord) (Pos.y - vp.y),
		(ieWord) (csize * 4), (ieWord) (csize * 3), *col );
	}
}

// Check if P is over our ground circle
bool Selectable::IsOver(const Point &P) const
{
	int csize = size;
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
	//forcing regeneration of the cover
	SetSpriteCover(NULL);
}

void Selectable::SetCircle(int circlesize, const Color &color, Sprite2D* normal_circle, Sprite2D* selected_circle)
{
	size = circlesize;
	selectedColor = color;
	overColor.r = color.r >> 1;
	overColor.g = color.g >> 1;
	overColor.b = color.b >> 1;
	overColor.a = color.a;
	circleBitmap[0] = normal_circle;
	circleBitmap[1] = selected_circle;
}

//used for creatures
int Selectable::WantDither() const
{
	//if dithering is disabled globally, don't do it
	if (core->FogOfWar&FOG_DITHERSPRITES) {
		return 0;
	}
	//if actor is dead, dither it if polygon wants
	if (Selected&0x80) {
		return 1;
	}
	//if actor is selected dither it
	if (Selected) {
		return 2;
	}
	return 1;
}

/***********************
 * Highlightable Class *
 ***********************/

Highlightable::Highlightable(ScriptableType type)
	: Scriptable( type )
{
	outline = NULL;
	Highlight = false;
	Cursor = IE_CURSOR_NORMAL;
	KeyResRef[0] = 0;
	EnterWav[0] = 0;
}

Highlightable::~Highlightable(void)
{
	if (outline) {
		delete( outline );
	}
}

bool Highlightable::IsOver(const Point &Pos) const
{
	if (!outline) {
		return false;
	}
	return outline->PointIn( Pos );
}

void Highlightable::DrawOutline() const
{
	if (!outline) {
		return;
	}
	core->GetVideoDriver()->DrawPolyline( outline, outlineColor, true );
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
	if (!Scripts[0] && !EnterWav[0]) {
		return false;
	}
	AddTrigger(TriggerEntry(trigger_entered, ID));
	if (!TrapResets()) {
		Trapped = false;
	}
	return true;
}

bool Highlightable::TryUnlock(Actor *actor, bool removekey) {
	const char *Key = GetKey();
	Actor *haskey = NULL;

	if (Key && actor->InParty) {
		Game *game = core->GetGame();
		//allow unlock when the key is on any partymember
		for (int idx = 0; idx < game->GetPartySize(false); idx++) {
			Actor *pc = game->FindPC(idx + 1);
			if (!pc) continue;

			if (pc->inventory.HasItem(Key,0) ) {
				haskey = pc;
				break;
			}
		}
	} else if (Key) {
		//actor is not in party, check only actor
		if (actor->inventory.HasItem(Key,0) ) {
			haskey = actor;
		}
	}

	if (!haskey) {
		return false;
	}

	if (removekey) {
		CREItem *item = NULL;
		haskey->inventory.RemoveItem(Key,0,&item);
		//the item should always be existing!!!
		if (item) {
			delete item;
		}
	}

	return true;
}

//detect this trap, using a skill, skill could be set to 256 for 'sure'
//skill is the all around modified trap detection skill
//a trapdetectiondifficulty of 100 means impossible detection short of a spell
void Highlightable::DetectTrap(int skill, ieDword actorID)
{
	if (!CanDetectTrap()) return;
	if (!Scripts[0]) return;
	if ((skill>=100) && (skill!=256) ) skill = 100;
	int check = 0;
	if (third) {
		//~Search (detect traps) check. Search skill %d vs. trap's difficulty %d (searcher's %d INT bonus).~
		Actor *detective = core->GetGame()->GetActorByGlobalID(actorID);
		int bonus = 0;
		if (detective) {
			bonus = detective->GetAbilityBonus(IE_INT);
			displaymsg->DisplayRollStringName(39303, DMC_LIGHTGREY, detective, skill-bonus, TrapDetectionDiff, bonus);
		}
		check = (skill + bonus)*7;
	} else {
		check = skill/2 + core->Roll(1, skill/2, 0);
	}
	if (check > TrapDetectionDiff) {
		SetTrapDetected(1); //probably could be set to the player #?
		AddTrigger(TriggerEntry(trigger_detected, actorID));
	}
}

bool Highlightable::PossibleToSeeTrap() const
{
	return CanDetectTrap();
}

/*****************
 * Movable Class *
 *****************/

Movable::Movable(ScriptableType type)
	: Selectable( type )
{
	Destination = Pos;
	Orientation = 0;
	NewOrientation = 0;
	StanceID = 0;
	path = NULL;
	step = NULL;
	timeStartStep = 0;
	lastFrame = NULL;
	Area[0] = 0;
	AttackMovements[0] = 100;
	AttackMovements[1] = 0;
	AttackMovements[2] = 0;
}

Movable::~Movable(void)
{
	if (path) {
		ClearPath();
	}
}

int Movable::GetPathLength()
{
	PathNode *node = GetNextStep(0);
	if (!node) return 0;

	int i = 0;
	while (node->Next) {
		i++;
		node = node->Next;
	}
	return i;
}

PathNode *Movable::GetNextStep(int x)
{
	if (!step) {
		DoStep((unsigned int) ~0);
	}
	PathNode *node = step;
	while(node && x--) {
		node = node->Next;
	}
	return node;
}

Point Movable::GetMostLikelyPosition()
{
	if (!path) {
		return Pos;
	}

//actually, sometimes middle path would be better, if
//we stand in Destination already
	int halfway = GetPathLength()/2;
	PathNode *node = GetNextStep(halfway);
	if (node) {
		return Point((ieWord) ((node->x*16)+8), (ieWord) ((node->y*12)+6) );
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

	if (StanceID == IE_ANI_CONJURE && StanceID != arg && Type ==ST_ACTOR) {
		Actor *caster = (Actor *) this;
		if (caster->casting_sound) {
			caster->casting_sound->Stop();
			caster->casting_sound.release();
		}
	}

	if (arg<MAX_ANIMS) {
		StanceID=(unsigned char) arg;

		if (StanceID == IE_ANI_ATTACK) {
			// Set stance to a random attack animation

			int random = rand()%100;
			if (random < AttackMovements[0]) {
				StanceID = IE_ANI_ATTACK_BACKSLASH;
			} else if (random < AttackMovements[0] + AttackMovements[1]) {
				StanceID = IE_ANI_ATTACK_SLASH;
			} else {
				StanceID = IE_ANI_ATTACK_JAB;
			}
		}

	} else {
		StanceID=IE_ANI_AWAKE; //
		print("Tried to set invalid stance id(%u)", arg);
	}
}

void Movable::SetAttackMoveChances(ieWord *amc)
{
	AttackMovements[0]=amc[0];
	AttackMovements[1]=amc[1];
	AttackMovements[2]=amc[2];
}

//this could be used for WingBuffet as well
void Movable::MoveLine(int steps, int Pass, ieDword orient)
{
	if (path || !steps) {
		return;
	}
	Point p = Pos;
	p.x/=16;
	p.y/=12;
	path = area->GetLine( p, steps, orient, Pass );
}

void AdjustPositionTowards(Point &Pos, ieDword time_diff, unsigned int walk_speed, short srcx, short srcy, short destx, short desty) {
	if (destx > srcx)
		Pos.x += ( unsigned short )
			( ( ( ( ( destx * 16 ) + 8 ) - Pos.x ) * ( time_diff ) ) / walk_speed );
	else
		Pos.x -= ( unsigned short )
			( ( ( Pos.x - ( ( destx * 16 ) + 8 ) ) * ( time_diff ) ) / walk_speed );
	if (desty > srcy)
		Pos.y += ( unsigned short )
			( ( ( ( ( desty * 12 ) + 6 ) - Pos.y ) * ( time_diff ) ) / walk_speed );
	else
		Pos.y -= ( unsigned short )
			( ( ( Pos.y - ( ( desty * 12 ) + 6 ) ) * ( time_diff ) ) / walk_speed );
}

unsigned char Movable::GetNextFace()
{
	//slow turning
	if (timeStartStep==core->GetGame()->Ticks) {
		return Orientation;
	}
	if (Orientation != NewOrientation) {
		if ( ( (NewOrientation-Orientation) & (MAX_ORIENT-1) ) <= MAX_ORIENT/2) {
			Orientation++;
		} else {
			Orientation--;
		}
		Orientation = Orientation&(MAX_ORIENT-1);
	}

	return Orientation;
}

// returns whether we made all pending steps (so, false if we must be called again this tick)
// we can't just do them all here because the caller might have to update searchmap etc
bool Movable::DoStep(unsigned int walk_speed, ieDword time)
{
	if (!path) {
		return true;
	}
	if (!time) time = core->GetGame()->Ticks;
	if (!walk_speed) {
		// zero speed: no movement
		timeStartStep = time;
		StanceID = IE_ANI_READY;
		return true;
	}
	if (!step) {
		step = path;
		timeStartStep = time;
	} else if (step->Next && (( time - timeStartStep ) >= walk_speed)) {
		//print("[New Step] : Orientation = %d", step->orient);
		step = step->Next;
		timeStartStep = timeStartStep + walk_speed;
	}
	SetOrientation (step->orient, true);
	StanceID = IE_ANI_WALK;
	if ((Type == ST_ACTOR) && (InternalFlags & IF_RUNNING)) {
		StanceID = IE_ANI_RUN;
	}
	Pos.x = ( step->x * 16 ) + 8;
	Pos.y = ( step->y * 12 ) + 6;
	if (!step->Next) {
		// we reached our destination, we are done
		ClearPath();
		NewOrientation = Orientation;
		//since clearpath no longer sets currentaction to NULL
		//we set it here
		//no we don't, action is responsible for releasing itself
		//ReleaseCurrentAction();
		return true;
	}
	if (( time - timeStartStep ) >= walk_speed) {
		// we didn't finish all pending steps, yet
		return false;
	}
	AdjustPositionTowards(Pos, time - timeStartStep, walk_speed, step->x, step->y, step->Next->x, step->Next->y);
	return true;
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
	PathNode *endNode = path;
	while(endNode->Next) {
		endNode = endNode->Next;
	}
	Point p(endNode->x, endNode->y);
	area->ClearSearchMapFor(this);
	PathNode *path2 = area->FindPath( p, Des, size );
	endNode->Next = path2;
	//probably it is wise to connect it both ways?
	path2->Parent = endNode;
}

void Movable::FixPosition()
{
	if (Type!=ST_ACTOR) {
		return;
	}
	Actor *actor = (Actor *) this;
	if (actor->GetStat(IE_DONOTJUMP)&DNJ_BIRD ) {
		return;
	}
	//before fixposition, you should remove own shadow
	area->ClearSearchMapFor(this);
	Pos.x/=16;
	Pos.y/=12;
	GetCurrentArea()->AdjustPosition(Pos);
	Pos.x=Pos.x*16+8;
	Pos.y=Pos.y*12+6;
}

void Movable::WalkTo(const Point &Des, int distance)
{
	Point from;

	// maybe caller should be responsible for this
	if ((Des.x/16 == Pos.x/16) && (Des.y/12 == Pos.y/12)) {
		ClearPath();
		return;
	}

	// the prev_step stuff is a naive attempt to allow re-pathing while moving
	PathNode *prev_step = NULL;
	unsigned char old_stance = StanceID;
	if (step && step->Next) {
		// don't interrupt in the middle of a step; path from the next one
		prev_step = new PathNode(*step);
		from.x = ( step->Next->x * 16 ) + 8;
		from.y = ( step->Next->y * 12 ) + 6;
	}

	ClearPath();
	if (!prev_step) {
		FixPosition();
		from = Pos;
	}
	area->ClearSearchMapFor(this);
	if (distance) {
		path = area->FindPathNear( from, Des, size, distance );
	} else {
		path = area->FindPath( from, Des, size, distance );
	}
	//ClearPath sets destination, so Destination must be set after it
	//also we should set Destination only if there is a walkable path
	if (path) {
		Destination = Des;

		if (prev_step) {
			// we want to smoothly continue, please
			// this all needs more thought! but it seems to work okay
			StanceID = old_stance;

			if (path->Next) {
				// this is a terrible hack to make up for the
				// pathfinder orienting the first node wrong
				// should be fixed in pathfinder and not here!
				Point next, follow;
				next.x = path->x; next.y = path->y;
				follow.x = path->Next->x;
				follow.y = path->Next->y;
				path->orient = GetOrient(follow, next);
			}

			// then put the prev_step at the beginning of the path
			prev_step->Next = path;
			path->Parent = prev_step;
			path = prev_step;

			step = path;
		}
	} else {
		// pathing failed
		if (prev_step) {
			delete( prev_step );
			FixPosition();
		}
	}
}

void Movable::RunAwayFrom(const Point &Des, int PathLength, int flags)
{
	ClearPath();
	area->ClearSearchMapFor(this);
	path = area->RunAway( Pos, Des, size, PathLength, flags );
}

void Movable::RandomWalk(bool can_stop, bool run)
{
	if (path) {
		return;
	}
	//if not continous random walk, then stops for a while
	if (can_stop && (rand()&3) ) {
		SetWait((rand()&7)+7);
		return;
	}
	if (run) {
		InternalFlags|=IF_RUNNING;
	}
	//the commenting-out of the clear search map call was removed in 0.4.0
	//if you want to put it back for some reason, check
	//if the searchmap is not eaten up
	area->ClearSearchMapFor(this);
	Point p = Pos;

	//selecting points around a circle's edge around actor (didn't work better)
	//int x = core->Roll(1,100,-50);
	//p.x+=x;
	//p.y+=(int) sqrt(100-x*x);

	//selecting points in a square around actor (-24 to +24)
	p.x+=core->Roll(1,49,-25);
	p.y+=core->Roll(1,49,-25);
	//the 5th parameter is controlling the orientation of the actor
	//0 - back away, 1 - face direction
	path = area->RunAway( Pos, p, size, 50, 1 );
}

void Movable::MoveTo(const Point &Des)
{
	area->ClearSearchMapFor(this);
	Pos = Des;
	Destination = Des;
	if (BlocksSearchMap()) {
		area->BlockSearchMap( Pos, size, IsPC()?PATH_MAP_PC:PATH_MAP_NPC);
	}
}

void Movable::ClearPath()
{
	//this is to make sure attackers come to us
	//make sure ClearPath doesn't screw Destination (in the rare cases Destination
	//is set before ClearPath
	Destination = Pos;
	if (StanceID==IE_ANI_WALK || StanceID==IE_ANI_RUN) {
		StanceID = IE_ANI_AWAKE;
	}
	InternalFlags&=~IF_NORETICLE;
	PathNode* thisNode = path;
	while (thisNode) {
		PathNode* nextNode = thisNode->Next;
		delete( thisNode );
		thisNode = nextNode;
	}
	path = NULL;
	step = NULL;
	//don't call ReleaseCurrentAction
}

/**********************
 * Tiled Object Class *
 **********************/

TileObject::TileObject()
{
	opentiles = NULL;
	opencount = 0;
	closedtiles = NULL;
	closedcount = 0;
	Flags = 0;
}

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
