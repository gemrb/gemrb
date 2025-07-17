/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003-2007 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

#include "defsounds.h"
#include "ie_stats.h"

#include "CharAnimations.h"
#include "DataFileMgr.h"
#include "DialogHandler.h"
#include "DisplayMessage.h"
#include "Game.h"
#include "GameData.h"
#include "GlobalTimer.h"
#include "IniSpawn.h"
#include "Interface.h"
#include "Item.h"
#include "Map.h"
#include "MusicMgr.h"
#include "Projectile.h" // for ProHeights
#include "RNG.h"
#include "SaveGameIterator.h"
#include "ScriptEngine.h"
#include "ScriptedAnimation.h"
#include "StringMgr.h"
#include "TileMap.h"
#include "WorldMap.h"

#include "GUI/EventMgr.h"
#include "GUI/GameControl.h"
#include "GUI/WindowManager.h"
#include "GameScript/GSUtils.h"
#include "GameScript/GameScript.h"
#include "GameScript/Matching.h"
#include "Scriptable/Container.h"
#include "Scriptable/Door.h"
#include "Scriptable/InfoPoint.h"

namespace GemRB {

//------------------------------------------------------------
// Action Functions
//-------------------------------------------------------------

void GameScript::SetExtendedNight(Scriptable* Sender, Action* parameters)
{
	Map* map = Sender->GetCurrentArea();
	if (parameters->int0Parameter) {
		map->AreaType |= AT_EXTENDED_NIGHT;
	} else {
		map->AreaType &= ~AT_EXTENDED_NIGHT;
	}
}

void GameScript::SetAreaRestFlag(Scriptable* Sender, Action* parameters)
{
	Map* map = Sender->GetCurrentArea();
	//sets the 'can rest other' bit
	if (parameters->int0Parameter) {
		map->AreaType |= AT_CAN_REST_INDOORS;
	} else {
		map->AreaType &= ~AT_CAN_REST_INDOORS;
	}
}

void GameScript::AddAreaFlag(Scriptable* Sender, Action* parameters)
{
	Map* map = Sender->GetCurrentArea();
	map->AreaFlags |= parameters->int0Parameter;
}

void GameScript::RemoveAreaFlag(Scriptable* Sender, Action* parameters)
{
	Map* map = Sender->GetCurrentArea();
	map->AreaFlags &= ~parameters->int0Parameter;
}

void GameScript::SetAreaFlags(Scriptable* Sender, Action* parameters)
{
	Map* map = Sender->GetCurrentArea();
	ieDword value = map->AreaFlags;
	HandleBitMod(value, parameters->int0Parameter, BitOp(parameters->int1Parameter));
	map->AreaFlags = value;
}

void GameScript::AddAreaType(Scriptable* Sender, Action* parameters)
{
	Map* map = Sender->GetCurrentArea();
	map->AreaType |= MapEnv(parameters->int0Parameter);
}

void GameScript::RemoveAreaType(Scriptable* Sender, Action* parameters)
{
	Map* map = Sender->GetCurrentArea();
	map->AreaType &= ~MapEnv(parameters->int0Parameter);
}

void GameScript::NoAction(Scriptable* /*Sender*/, Action* /*parameters*/)
{
	//thats all :)
}

void GameScript::SG(Scriptable* Sender, Action* parameters)
{
	SetVariable(Sender, parameters->string0Parameter, parameters->int0Parameter, "GLOBAL");
}

void GameScript::SetGlobal(Scriptable* Sender, Action* parameters)
{
	SetVariable(Sender, parameters->string0Parameter, parameters->int0Parameter);
}

void GameScript::SetGlobalRandom(Scriptable* Sender, Action* parameters)
{
	// in iwd2 all the calls but one (1, 6) are with 0, 1 — plain rolls would always give a 0
	// while SetGlobalRandomPlus supposedly rolls
	int max = parameters->int1Parameter - parameters->int0Parameter + 1;
	ieDword value = 0;
	if (parameters->int2Parameter) {
		value = core->Roll(parameters->int0Parameter, parameters->int1Parameter, parameters->int2Parameter);
	} else if (max > 0) {
		value = RandomNumValue % max + parameters->int0Parameter; // should be +1 instead?
	}
	SetVariable(Sender, parameters->string0Parameter, value, parameters->resref1Parameter);
}

void GameScript::StartTimer(Scriptable* Sender, Action* parameters)
{
	Sender->StartTimer(parameters->int0Parameter, parameters->int1Parameter);
}

void GameScript::StartRandomTimer(Scriptable* Sender, Action* parameters)
{
	ieDword value = core->Roll(1, parameters->int2Parameter - parameters->int1Parameter, parameters->int2Parameter - 1);
	Sender->StartTimer(parameters->int0Parameter, value);
}

void GameScript::SetGlobalTimer(Scriptable* Sender, Action* parameters)
{
	ieDword mytime;

	mytime = core->GetGame()->GameTime; //gametime (should increase it)
	SetVariable(Sender, parameters->string0Parameter,
		    parameters->int0Parameter * core->Time.defaultTicksPerSec + mytime);
}

void GameScript::SetGlobalTimerRandom(Scriptable* Sender, Action* parameters)
{
	ieDword mytime;
	int random;

	//This works both ways in the original engine
	if (parameters->int1Parameter > parameters->int0Parameter) {
		random = parameters->int1Parameter - parameters->int0Parameter + 1;
		//random cannot be 0, its minimal value is 1
		random = RandomNumValue % random + parameters->int0Parameter;
	} else {
		random = parameters->int0Parameter - parameters->int1Parameter + 1;
		random = RandomNumValue % random + parameters->int1Parameter;
	}
	mytime = core->GetGame()->GameTime; //gametime (should increase it)
	SetVariable(Sender, parameters->string0Parameter, random * core->Time.defaultTicksPerSec + mytime);
}

void GameScript::SetGlobalTimerOnce(Scriptable* Sender, Action* parameters)
{
	ieDword mytime = CheckVariable(Sender, parameters->string0Parameter);
	if (mytime != 0) {
		return;
	}
	mytime = core->GetGame()->GameTime; //gametime (should increase it)
	SetVariable(Sender, parameters->string0Parameter,
		    parameters->int0Parameter * core->Time.defaultTicksPerSec + mytime);
}

void GameScript::RealSetGlobalTimer(Scriptable* Sender, Action* parameters)
{
	ieDword mytime = core->GetGame()->RealTime;

	SetVariable(Sender, parameters->string0Parameter,
		    parameters->int0Parameter * core->Time.defaultTicksPerSec + mytime);
}

// buggy in all engines, even ees, temporarily wiping other IDS
// fields and permanently resetting IE_GENERAL to 0
// Bubb's description:
// These actions add/remove the creature to/from an internal list that keeps track of
// all familiars. Being in this list enables a majority of the engine's special casing
// regarding familiars. The actions exist because changing EA directly doesn't update
// the "familiar" list. The engine adds a creature with EA=3 to the familiar list on
// unmarshal, but not automatically during gameplay.
// We don't need such a list, since we can just check EA.
void GameScript::AddFamiliar(Scriptable* Sender, Action* /*parameters*/)
{
	Actor* actor = Scriptable::As<Actor>(Sender);
	if (!actor || !actor->Persistent()) {
		return;
	}
	actor->SetBase(IE_EA, EA_FAMILIAR);
}

void GameScript::RemoveFamiliar(Scriptable* Sender, Action* /*parameters*/)
{
	Actor* actor = Scriptable::As<Actor>(Sender);
	if (!actor) {
		return;
	}
	actor->SetBase(IE_EA, EA_NEUTRAL);
}

void GameScript::ChangeEnemyAlly(Scriptable* Sender, Action* parameters)
{
	Scriptable* scr = Sender;
	if (parameters->objects[1]) {
		scr = GetScriptableFromObject(Sender, parameters);
	}
	Actor* actor = Scriptable::As<Actor>(scr);
	if (!actor) {
		return;
	}

	actor->SetBase(IE_EA, parameters->int0Parameter);
}

void GameScript::ChangeGeneral(Scriptable* Sender, Action* parameters)
{
	Scriptable* scr = Sender;
	if (parameters->objects[1]) {
		scr = GetScriptableFromObject(Sender, parameters);
	}
	Actor* actor = Scriptable::As<Actor>(scr);
	if (!actor) {
		return;
	}

	actor->SetBase(IE_GENERAL, parameters->int0Parameter);
}

void GameScript::ChangeRace(Scriptable* Sender, Action* parameters)
{
	Scriptable* scr = Sender;
	if (parameters->objects[1]) {
		scr = GetScriptableFromObject(Sender, parameters);
	}
	Actor* actor = Scriptable::As<Actor>(scr);
	if (!actor) {
		return;
	}

	actor->SetBase(IE_RACE, parameters->int0Parameter);
}

void GameScript::ChangeClass(Scriptable* Sender, Action* parameters)
{
	Scriptable* scr = Sender;
	if (parameters->objects[1]) {
		scr = GetScriptableFromObject(Sender, parameters);
	}
	Actor* actor = Scriptable::As<Actor>(scr);
	if (!actor) {
		return;
	}

	actor->SetBase(IE_CLASS, parameters->int0Parameter);
}

void GameScript::SetNamelessClass(Scriptable* /*Sender*/, Action* parameters)
{
	//same as Protagonist
	Actor* actor = core->GetGame()->GetPC(0, false);
	actor->SetBase(IE_CLASS, parameters->int0Parameter);
	// dump newly incompatible items; perhaps this should go into pcf_class
	actor->inventory.EnforceUsability();
}

void GameScript::SetNamelessDisguise(Scriptable* Sender, Action* parameters)
{
	SetVariable(Sender, "APPEARANCE", parameters->int0Parameter, "GLOBAL");
	core->SetEventFlag(EF_UPDATEANIM);
}

void GameScript::ChangeSpecifics(Scriptable* Sender, Action* parameters)
{
	Scriptable* scr = Sender;
	if (parameters->objects[1]) {
		scr = GetScriptableFromObject(Sender, parameters);
	}
	Actor* actor = Scriptable::As<Actor>(scr);
	if (!actor) {
		return;
	}

	actor->SetBase(IE_SPECIFIC, parameters->int0Parameter);
}

inline void PermanentStatChangeFeedback(int stat, const Actor* actor)
{
	// figure out PC index (TNO, Morte, Annah, Dakkon, FFG, Nordom, Ignus, Vhailor)
	// then use it to calculate personalized feedback strings
	int spec2offset[] = { 0, 7, 5, 6, 4, 3, 2, 1 };
	int pcOffset = spec2offset[actor->GetStat(IE_SPECIFIC) - 2];

	ieStrRef ref = ieStrRef(int(ieStrRef::PST_STR_MOD) + pcOffset);
	if (stat == IE_STR) {
		displaymsg->DisplayString(ref, GUIColors::WHITE, STRING_FLAGS::SOUND);
		return;
	}

	// the strings follow the main stat order and are back to back (only ruined by IE_STREXTRA)
	// "Strength increased permanently" and so on until
	// @34889 = ~Charisma increased permanently [Nameless One]~
	if (stat >= IE_INT && stat <= IE_CHR) {
		ref = ieStrRef(int(ref) + 8 * (stat - IE_INT + 1));
		displaymsg->DisplayString(ref, GUIColors::WHITE, STRING_FLAGS::SOUND);
		return;
	}

	// these two have strrefs right after
	// @34897 = ~MaxHP increased permanently [Nameless One]~
	// @34905 = ~Armor Class increased permanently [Nameless One]~
	if (stat == IE_MAXHITPOINTS || stat == IE_ARMORCLASS) {
		ref = ieStrRef(int(ref) + 8 * (IE_CHR - IE_INT + 1 + stat - IE_MAXHITPOINTS + 1));
		displaymsg->DisplayString(ref, GUIColors::WHITE, STRING_FLAGS::SOUND);
		return;
	}

	// @34913 = ~Proficiency Points increased permanently [Nameless One]~
	if (stat == IE_FREESLOTS) {
		ref = ieStrRef(int(ref) + 8 * (IE_CHR - IE_INT + 1 + IE_ARMORCLASS + 1));
		displaymsg->DisplayString(ref, GUIColors::WHITE, STRING_FLAGS::SOUND);
		return;
	}
	// there's also this one, but it is unused, nothing calls the feedback function to print it:
	// @35621 = ~Characteristic Points increased permanently [Nameless One]~
}

void GameScript::PermanentStatChange(Scriptable* Sender, Action* parameters)
{
	Scriptable* scr = Sender;
	if (parameters->objects[1]) {
		scr = GetScriptableFromObject(Sender, parameters);
	}
	Actor* actor = Scriptable::As<Actor>(scr);
	if (!actor) {
		return;
	}

	ieDword value;
	int stat = parameters->int0Parameter;
	// int1Parameter is from delta.ids
	// int2Parameter is supposed to support also bones.ids, but nothing uses it like that
	// if we need it, take the implementation from GameScript::Damage
	switch (parameters->int1Parameter) {
		case DM_LOWER:
			value = actor->GetBase(stat);
			value -= parameters->int2Parameter;
			break;
		case DM_RAISE:
			value = actor->GetBase(stat);
			value += parameters->int2Parameter;
			if (actor->InParty) PermanentStatChangeFeedback(stat, actor);
			break;
		case DM_SET:
		default:
			value = parameters->int2Parameter;
			break;
	}
	actor->SetBase(stat, value);
}

void GameScript::ChangeStat(Scriptable* Sender, Action* parameters)
{
	Scriptable* scr = Sender;
	if (parameters->objects[1]) {
		scr = GetScriptableFromObject(Sender, parameters);
	}
	Actor* actor = Scriptable::As<Actor>(scr);
	if (!actor) {
		return;
	}

	ieDword value = parameters->int1Parameter;
	if (parameters->flags & ACF_PRECOMPILED) {
		value = OverrideStatsIDS(value);
	}
	if (parameters->int2Parameter == 1) { // basically statmod.ids entries, but there's only two
		value += actor->GetBase(parameters->int0Parameter);
	}
	actor->SetBase(parameters->int0Parameter, value);
}

void GameScript::ChangeStatGlobal(Scriptable* Sender, Action* parameters)
{
	Scriptable* scr = Sender;
	if (parameters->objects[1]) {
		scr = GetScriptableFromObject(Sender, parameters);
	}
	Actor* actor = Scriptable::As<Actor>(scr);
	if (!actor) {
		return;
	}
	ieDword value = CheckVariable(Sender, parameters->string0Parameter, parameters->string1Parameter);
	if (parameters->int1Parameter == 1) {
		value += actor->GetBase(parameters->int0Parameter);
	}
	actor->SetBase(parameters->int0Parameter, value);
}

void GameScript::ChangeGender(Scriptable* Sender, Action* parameters)
{
	Scriptable* scr = Sender;
	if (parameters->objects[1]) {
		scr = GetScriptableFromObject(Sender, parameters);
	}
	Actor* actor = Scriptable::As<Actor>(scr);
	if (!actor) {
		return;
	}

	actor->SetBase(IE_SEX, parameters->int0Parameter);
}

void GameScript::ChangeAlignment(Scriptable* Sender, Action* parameters)
{
	Scriptable* scr = Sender;
	if (parameters->objects[1]) {
		scr = GetScriptableFromObject(Sender, parameters);
	}
	Actor* actor = Scriptable::As<Actor>(scr);
	if (!actor) {
		return;
	}

	actor->SetBase(IE_ALIGNMENT, parameters->int0Parameter);
}

void GameScript::SetFaction(Scriptable* Sender, Action* parameters)
{
	Scriptable* scr = Sender;
	if (parameters->objects[1]) {
		scr = GetScriptableFromObject(Sender, parameters);
	}
	Actor* actor = Scriptable::As<Actor>(scr);
	if (!actor) {
		return;
	}

	actor->SetBase(IE_FACTION, parameters->int0Parameter);
}

void GameScript::SetHP(Scriptable* Sender, Action* parameters)
{
	Scriptable* scr = Sender;
	if (parameters->objects[1]) {
		scr = GetScriptableFromObject(Sender, parameters);
	}
	Actor* actor = Scriptable::As<Actor>(scr);
	if (!actor) {
		return;
	}

	actor->SetBase(IE_HITPOINTS, parameters->int0Parameter);
}

void GameScript::SetHPPercent(Scriptable* Sender, Action* parameters)
{
	Scriptable* scr = Sender;
	if (parameters->objects[1]) {
		scr = GetScriptableFromObject(Sender, parameters);
	}
	Actor* actor = Scriptable::As<Actor>(scr);
	if (!actor) {
		return;
	}

	// 0 - adjust to max hp, 1 - adjust to current
	if (parameters->int1Parameter) {
		actor->NewBase(IE_HITPOINTS, parameters->int0Parameter, MOD_PERCENT);
	} else {
		actor->NewBase(IE_HITPOINTS, actor->GetStat(IE_MAXHITPOINTS) * parameters->int0Parameter / 100, MOD_ABSOLUTE);
	}
}

void GameScript::AddHP(Scriptable* Sender, Action* parameters)
{
	Scriptable* scr = Sender;
	if (parameters->objects[1]) {
		scr = GetScriptableFromObject(Sender, parameters);
	}
	Actor* actor = Scriptable::As<Actor>(scr);
	if (!actor) {
		return;
	}

	actor->NewBase(IE_HITPOINTS, parameters->int0Parameter, MOD_ADDITIVE);
}

//this works on an object (pst)
//but can also work on actor itself (gemrb)
void GameScript::SetTeam(Scriptable* Sender, Action* parameters)
{
	Scriptable* scr = Sender;
	if (parameters->objects[1]) {
		scr = GetScriptableFromObject(Sender, parameters);
	}
	Actor* actor = Scriptable::As<Actor>(scr);
	if (!actor) {
		return;
	}

	actor->SetBase(IE_TEAM, parameters->int0Parameter);
}

//this works on an object (gemrb)
//or on Myself if object isn't given (iwd2)
void GameScript::SetTeamBit(Scriptable* Sender, Action* parameters)
{
	Scriptable* scr = Sender;
	if (parameters->objects[1]) {
		scr = GetScriptableFromObject(Sender, parameters);
	}
	Actor* actor = Scriptable::As<Actor>(scr);
	if (!actor) {
		return;
	}

	if (parameters->int1Parameter) {
		actor->SetBase(IE_TEAM, actor->GetStat(IE_TEAM) | parameters->int0Parameter);
	} else {
		actor->SetBase(IE_TEAM, actor->GetStat(IE_TEAM) & ~parameters->int0Parameter);
	}
}

void GameScript::TriggerActivation(Scriptable* Sender, Action* parameters)
{
	Scriptable* ip = Sender;
	StringParam& name = parameters->string0Parameter;

	if (parameters->objects[1]) {
		ip = GetScriptableFromObject(Sender, parameters);
		if (ip) name = parameters->objects[1]->objectName;
	}
	if (!ip || (ip->Type != ST_TRIGGER && ip->Type != ST_TRAVEL && ip->Type != ST_PROXIMITY)) {
		Log(WARNING, "Actions", "Script error: No Trigger Named \"{}\"", name);
		parameters->dump();
		return;
	}
	InfoPoint* trigger = static_cast<InfoPoint*>(ip);
	if (parameters->int0Parameter != 0) {
		trigger->Flags &= ~TRAP_DEACTIVATED;
		if (trigger->TrapResets()) {
			trigger->Trapped = 1;
			Sender->AddTrigger(TriggerEntry(trigger_reset, trigger->GetGlobalID()));
		}
	} else {
		trigger->Flags |= TRAP_DEACTIVATED;
	}
}

// not blocking in the originals, just sent a message
void GameScript::FadeToColor(Scriptable* Sender, Action* parameters)
{
	core->timer.SetFadeToColor(parameters->pointParameter.x);
	Sender->ReleaseCurrentAction();
}

void GameScript::FadeFromColor(Scriptable* Sender, Action* parameters)
{
	core->timer.SetFadeFromColor(parameters->pointParameter.x);
	Sender->ReleaseCurrentAction();
}

void GameScript::FadeToAndFromColor(Scriptable* Sender, Action* parameters)
{
	core->timer.SetFadeToColor(parameters->pointParameter.x);
	core->timer.SetFadeFromColor(parameters->pointParameter.x);
	Sender->ReleaseCurrentAction();
}

void GameScript::JumpToPoint(Scriptable* Sender, Action* parameters)
{
	Actor* actor = Scriptable::As<Actor>(Sender);
	if (!actor) {
		return;
	}
	actor->SetPosition(parameters->pointParameter, true);

	const Actor* protagonist = core->GetGame()->GetPC(0, false);
	// original: all familiars have JumpToPoint() (to protagonist) inserted at the front of their action queue
	// it didn't check they were in the same area
	// we just do it instantly for simplicity
	if (actor == protagonist) {
		core->GetGame()->MoveFamiliars(actor->AreaName, actor->Pos, -1);
	}
}

void GameScript::JumpToPointInstant(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetScriptableFromObject(Sender, parameters);
	Actor* actor = Scriptable::As<Actor>(tar);
	if (!actor) {
		return;
	}
	actor->SetPosition(parameters->pointParameter, true);
}

/** instant jump to location saved in stats */
/** default subject is the current actor */
void GameScript::JumpToSavedLocation(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetScriptableFromObject(Sender, parameters);
	if (!tar) {
		tar = Sender;
	}
	Actor* actor = Scriptable::As<Actor>(tar);
	if (!actor) {
		return;
	}
	Point p(actor->GetStat(IE_SAVEDXPOS), actor->GetStat(IE_SAVEDYPOS));
	actor->SetPosition(p, true);
	actor->SetOrientation(ClampToOrientation(actor->GetStat(IE_SAVEDFACE)), false);
}

void GameScript::JumpToObject(Scriptable* Sender, Action* parameters)
{
	Actor* actor = Scriptable::As<Actor>(Sender);
	if (!actor) {
		return;
	}

	const Scriptable* tar = GetScriptableFromObject(Sender, parameters);
	if (!tar) return;
	const Map* map = tar->GetCurrentArea();
	if (!map) return;

	if (!parameters->resref0Parameter.IsEmpty()) {
		CreateVisualEffectCore(Sender, Sender->Pos, parameters->resref0Parameter, 0);
	}

	if (actor->Persistent() || !CreateMovementEffect(actor, map->GetScriptRef(), tar->Pos, 0)) {
		MoveBetweenAreasCore(actor, map->GetScriptRef(), tar->Pos, -1, true);
	}
}

void GameScript::TeleportParty(Scriptable* /*Sender*/, Action* parameters)
{
	const Game* game = core->GetGame();
	game->MovePCs(parameters->resref0Parameter, parameters->pointParameter, parameters->int0Parameter);
	game->MoveFamiliars(parameters->resref0Parameter, parameters->pointParameter, parameters->int0Parameter);
}

//5 is the ToB value, but it might be useful to have multiple expansions
void GameScript::MoveToExpansion(Scriptable* Sender, Action* parameters)
{
	Game* game = core->GetGame();

	game->SetExpansion(parameters->int0Parameter);
	Sender->ReleaseCurrentAction();
}

//add some animation effects too?
void GameScript::ExitPocketPlane(Scriptable* /*Sender*/, Action* /*parameters*/)
{
	Point pos;
	ResRef area;

	Game* game = core->GetGame();
	int cnt = game->GetPartySize(false);
	for (int i = 0; i < cnt; i++) {
		Actor* act = game->GetPC(i, false);
		if (act) {
			const GAMLocationEntry* gle;
			if (game->GetPlaneLocationCount() <= (unsigned int) i) {
				// no location, meaning the actor joined the party after the save
				// reuse the last valid location
				gle = game->GetPlaneLocationEntry(game->GetPlaneLocationCount() - 1);
			} else {
				gle = game->GetPlaneLocationEntry(i);
			}

			// save player1 location for familiar movement
			if (!i) {
				pos = gle->Pos;
				area = gle->AreaResRef;
			}
			MoveBetweenAreasCore(act, gle->AreaResRef, gle->Pos, -1, true);
		}
	}

	// move familiars
	game->MoveFamiliars(area, pos, -1);

	// don't clear locations!
}

//moves pcs and npcs from an area to another area
void GameScript::MoveGlobalsTo(Scriptable* /*Sender*/, Action* parameters)
{
	const Game* game = core->GetGame();
	int i = game->GetPartySize(false);
	while (i--) {
		Actor* tar = game->GetPC(i, false);
		//if the actor isn't in the source area, we don't care
		if (tar->AreaName != parameters->resref0Parameter) {
			continue;
		}
		// no need of CreateMovementEffect, party members are always moved immediately
		MoveBetweenAreasCore(tar, parameters->resref1Parameter, parameters->pointParameter, -1, true);
	}
	i = game->GetNPCCount();
	while (i--) {
		Actor* tar = game->GetNPC(i);
		//if the actor isn't in the source area, we don't care
		if (tar->AreaName != parameters->resref0Parameter) {
			continue;
		}
		//if the actor is currently in a loaded area, remove it from there
		Map* map = tar->GetCurrentArea();
		if (map) {
			map->RemoveActor(tar);
		}
		//update the target's area to the destination
		tar->AreaName = parameters->resref1Parameter;
		//if the destination area is currently loaded, move the actor there now
		if (game->FindMap(tar->AreaName)) {
			MoveBetweenAreasCore(tar, parameters->resref1Parameter, parameters->pointParameter, -1, true);
		}
	}
}

void GameScript::MoveGlobal(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetScriptableFromObject(Sender, parameters);
	Actor* actor = Scriptable::As<Actor>(tar);
	if (!actor) {
		return;
	}

	if (actor->Persistent() || !CreateMovementEffect(actor, parameters->resref0Parameter, parameters->pointParameter, 0)) {
		MoveBetweenAreasCore(actor, parameters->resref0Parameter, parameters->pointParameter, -1, true);
	}
}

//we also allow moving to door, container
void GameScript::MoveGlobalObject(Scriptable* Sender, Action* parameters)
{
	MoveGlobalObjectCore(Sender, parameters, 0);
}

void GameScript::MoveGlobalObjectOffScreen(Scriptable* Sender, Action* parameters)
{
	MoveGlobalObjectCore(Sender, parameters, CC_OFFSCREEN);
}

//don't use offset from Sender
void GameScript::CreateCreature(Scriptable* Sender, Action* parameters)
{
	CreateCreatureCore(Sender, parameters, CC_CHECK_IMPASSABLE | CC_CHECK_OVERLAP | CC_SCRIPTNAME);
}

//another highly redundant action
void GameScript::CreateCreatureDoor(Scriptable* Sender, Action* parameters)
{
	//we hack this to death
	parameters->resref1Parameter = "SPDIMNDR";
	CreateCreatureCore(Sender, parameters, CC_CHECK_IMPASSABLE | CC_CHECK_OVERLAP | CC_PLAY_ANIM);
}

//another highly redundant action
void GameScript::CreateCreatureObjectDoor(Scriptable* Sender, Action* parameters)
{
	//we hack this to death
	parameters->resref1Parameter = "SPDIMNDR";
	CreateCreatureCore(Sender, parameters, CC_OBJECT | CC_CHECK_IMPASSABLE | CC_CHECK_OVERLAP | CC_PLAY_ANIM);
}

//don't use offset from Sender
void GameScript::CreateCreatureImpassable(Scriptable* Sender, Action* parameters)
{
	CreateCreatureCore(Sender, parameters, CC_CHECK_OVERLAP);
}

void GameScript::CreateCreatureImpassableAllowOverlap(Scriptable* Sender, Action* parameters)
{
	CreateCreatureCore(Sender, parameters, 0);
}

//use offset from Sender
void GameScript::CreateCreatureAtFeet(Scriptable* Sender, Action* parameters)
{
	CreateCreatureCore(Sender, parameters, CC_OFFSET | CC_CHECK_IMPASSABLE | CC_CHECK_OVERLAP);
}

void GameScript::CreateCreatureOffScreen(Scriptable* Sender, Action* parameters)
{
	CreateCreatureCore(Sender, parameters, CC_OFFSCREEN | CC_CHECK_IMPASSABLE | CC_CHECK_OVERLAP);
}

//creates copy at actor, plays animation
void GameScript::CreateCreatureObjectCopy(Scriptable* Sender, Action* parameters)
{
	CreateCreatureCore(Sender, parameters, CC_OBJECT | CC_CHECK_IMPASSABLE | CC_CHECK_OVERLAP | CC_COPY | CC_PLAY_ANIM);
}

//creates copy at absolute point
void GameScript::CreateCreatureCopyPoint(Scriptable* Sender, Action* parameters)
{
	CreateCreatureCore(Sender, parameters, CC_CHECK_IMPASSABLE | CC_CHECK_OVERLAP | CC_COPY | CC_PLAY_ANIM);
}

//this is the same, object + offset
//using this for simple createcreatureobject, (0 offsets)
//createcreatureobjecteffect may have animation
void GameScript::CreateCreatureObjectOffset(Scriptable* Sender, Action* parameters)
{
	CreateCreatureCore(Sender, parameters, CC_OBJECT | CC_CHECK_IMPASSABLE | CC_CHECK_OVERLAP | CC_PLAY_ANIM);
}

void GameScript::CreateCreatureObjectOffScreen(Scriptable* Sender, Action* parameters)
{
	CreateCreatureCore(Sender, parameters, CC_OFFSCREEN | CC_OBJECT | CC_CHECK_IMPASSABLE | CC_CHECK_OVERLAP);
}

//I think this simply removes the cursor and hides the gui without disabling scripts
//See Interface::SetCutSceneMode
void GameScript::SetCursorState(Scriptable* /*Sender*/, Action* parameters)
{
	int active = parameters->int0Parameter;

	Game* game = core->GetGame();
	game->SetControlStatus(CS_HIDEGUI, active ? BitOp::OR : BitOp::NAND);
	if (active) {
		core->GetWindowManager()->SetCursorFeedback(WindowManager::MOUSE_NONE);
	} else {
		core->GetWindowManager()->SetCursorFeedback(WindowManager::CursorFeedback(core->config.MouseFeedback));
	}
}

void GameScript::StartCutSceneMode(Scriptable* /*Sender*/, Action* /*parameters*/)
{
	core->SetCutSceneMode(true);
}

void GameScript::EndCutSceneMode(Scriptable* /*Sender*/, Action* /*parameters*/)
{
	core->SetCutSceneMode(false);
}

void GameScript::StartCutScene(Scriptable* Sender, Action* parameters)
{
	GameScript* gs = new GameScript(parameters->resref0Parameter, Sender);
	gs->EvaluateAllBlocks();
	delete gs;
}

// StartCutScene("my_nifty_cut_scene") = StartCutSceneEx("my_nifty_cut_scene",FALSE)
void GameScript::StartCutSceneEx(Scriptable* Sender, Action* parameters)
{
	GameScript* gs = new GameScript(parameters->resref0Parameter, Sender);
	gs->EvaluateAllBlocks(parameters->int0Parameter != 0);
	delete gs;
}

void GameScript::CutSceneID(Scriptable* Sender, Action* /*parameters*/)
{
	// shouldn't get called
	Log(DEBUG, "GameScript", "CutSceneID was called by {}!", Sender->GetScriptName());
}

// can the cutscene be skipped by pressing ESC?
// use eg. SetAreaScript("cutskip2",OVERRIDE) to define a "failsafe" script
// to execute when the cutscene is interrupted
void GameScript::SetCutSceneBreakable(Scriptable* /*Sender*/, Action* /*parameters*/)
{
	Log(ERROR, "GameScript", "SetCutSceneBreakable is not implemented yet!");
	// TODO: ee, breakable = bool(parameters->int0Parameter)
}

static EffectRef fx_charm_ref = { "State:Charmed", -1 };

void GameScript::Enemy(Scriptable* Sender, Action* /*parameters*/)
{
	Actor* actor = Scriptable::As<Actor>(Sender);
	if (!actor) {
		return;
	}

	actor->fxqueue.RemoveAllEffects(fx_charm_ref);
	actor->SetBase(IE_EA, EA_ENEMY);
}

void GameScript::Ally(Scriptable* Sender, Action* /*parameters*/)
{
	Actor* actor = Scriptable::As<Actor>(Sender);
	if (!actor || actor->GetBase(IE_EA) == EA_FAMILIAR) {
		return;
	}

	actor->fxqueue.RemoveAllEffects(fx_charm_ref);
	actor->SetBase(IE_EA, EA_ALLY);
}

/** GemRB extension: you can replace baldur.bcs */
void GameScript::ChangeAIScript(Scriptable* Sender, Action* parameters)
{
	if (parameters->int0Parameter >= MAX_SCRIPTS) {
		return;
	}
	//clearing the queue, and checking script level was intentionally removed
	Sender->SetScript(parameters->resref0Parameter, parameters->int0Parameter, false);
}

void GameScript::ForceAIScript(Scriptable* Sender, Action* parameters)
{
	if (parameters->int0Parameter >= MAX_SCRIPTS) {
		return;
	}
	Scriptable* tar = GetScriptableFromObject(Sender, parameters);
	Actor* actor = Scriptable::As<Actor>(tar);
	if (!actor) {
		return;
	}

	//clearing the queue, and checking script level was intentionally removed
	actor->SetScript(parameters->resref0Parameter, parameters->int0Parameter, false);
}

void GameScript::ResetPlayerAI(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetScriptableFromObject(Sender, parameters);
	Actor* actor = Scriptable::As<Actor>(tar);
	if (!actor) return;

	static AutoTable partyAI = gamedata->LoadTable("partyai", true);
	ResRef defaultAI = partyAI->QueryField(actor->GetScriptName(), "AI_SCRIPT");
	// should we set 0 (SCR_OVERRIDE) instead, now it's the class slot?
	// ees don't use .bs any more, so the false is always good
	actor->SetScript(defaultAI, AI_SCRIPT_LEVEL, false);
}

// basically ChangeAIScript, but anyone can run it and it affects the area instead
void GameScript::SetAreaScript(Scriptable* Sender, Action* parameters)
{
	Map* map = Sender->GetCurrentArea();
	if (!map) return;
	map->SetScript(parameters->resref0Parameter, parameters->int0Parameter, false);
}

// see sndslot.ids for strref1 meanings
void GameScript::SetPlayerSound(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetScriptableFromObject(Sender, parameters);
	Actor* actor = Scriptable::As<Actor>(tar);
	if (!actor) {
		return;
	}
	if (parameters->int1Parameter >= 100) {
		Log(WARNING, "GameScript", "Invalid index {} in SetPlayerSound.", parameters->int1Parameter);
		return;
	}

	actor->StrRefs[parameters->int1Parameter] = ieStrRef(parameters->int0Parameter);
}

//this one works only on real actors, they got constants (soundoff.ids)
void GameScript::VerbalConstantHead(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetScriptableFromObject(Sender, parameters);
	if (!tar || tar->Type != ST_ACTOR) {
		return;
	}
	DisplayStringCoreVC(tar, Verbal(parameters->int0Parameter), DS_HEAD | DS_CONSOLE);
}

void GameScript::VerbalConstant(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetScriptableFromObject(Sender, parameters);
	if (!tar || tar->Type != ST_ACTOR) {
		return;
	}
	DisplayStringCoreVC(tar, Verbal(parameters->int0Parameter), DS_CONSOLE);
}

//bg2 - variable
void GameScript::SaveLocation(Scriptable* Sender, Action* parameters)
{
	if (parameters->variable0Parameter.IsEmpty()) {
		parameters->variable0Parameter = "LOCALSsavedlocation";
	}
	SetPointVariable(Sender, parameters->string0Parameter, parameters->pointParameter);
}

//PST:has parameters, IWD2: no params
void GameScript::SetSavedLocation(Scriptable* Sender, Action* parameters)
{
	Actor* actor = Scriptable::As<Actor>(Sender);
	if (!actor) {
		return;
	}
	//iwd2
	if (parameters->pointParameter.IsZero()) {
		actor->SetBase(IE_SAVEDXPOS, actor->Pos.x);
		actor->SetBase(IE_SAVEDYPOS, actor->Pos.y);
		actor->SetBase(IE_SAVEDFACE, actor->GetOrientation());
		return;
	}
	//pst
	actor->SetBase(IE_SAVEDXPOS, parameters->pointParameter.x);
	actor->SetBase(IE_SAVEDYPOS, parameters->pointParameter.y);
	actor->SetBase(IE_SAVEDFACE, parameters->int0Parameter);
}
//IWD2, sets the homepoint int0,int1,int2
void GameScript::SetSavedLocationPoint(Scriptable* Sender, Action* parameters)
{
	Actor* actor = Scriptable::As<Actor>(Sender);
	if (!actor) {
		return;
	}
	actor->SetBase(IE_SAVEDXPOS, parameters->int0Parameter);
	actor->SetBase(IE_SAVEDYPOS, parameters->int1Parameter);
	actor->SetBase(IE_SAVEDFACE, parameters->int2Parameter);
}

// IWD2, sets the *homepoint* P — the iwd version of SetHomeLocation
// handle [-1.-1] specially, if needed; ar6200.bcs has interesting use
void GameScript::SetStartPos(Scriptable* Sender, Action* parameters)
{
	Actor* actor = Scriptable::As<Actor>(Sender);
	if (!actor) {
		return;
	}
	actor->HomeLocation = parameters->pointParameter;
}

void GameScript::SaveObjectLocation(Scriptable* Sender, Action* parameters)
{
	const Scriptable* tar = GetScriptableFromObject(Sender, parameters);
	if (!tar) {
		return;
	}
	if (parameters->variable0Parameter.IsEmpty()) {
		parameters->variable0Parameter = "LOCALSsavedlocation";
	}
	SetPointVariable(Sender, parameters->string0Parameter, tar->Pos);
}

/** you may omit the string0Parameter, in this case this will be a */
/** CreateCreatureAtSavedLocation */
void GameScript::CreateCreatureAtLocation(Scriptable* Sender, Action* parameters)
{
	if (parameters->variable0Parameter.IsEmpty()) {
		parameters->variable0Parameter = "LOCALSsavedlocation";
	}
	ieDword value = CheckVariable(Sender, parameters->string0Parameter);
	parameters->pointParameter.y = (ieWord) (value & 0xffff);
	parameters->pointParameter.x = (ieWord) (value >> 16);
	CreateCreatureCore(Sender, parameters, CC_CHECK_IMPASSABLE | CC_STRING1);
}

void GameScript::WaitRandom(Scriptable* Sender, Action* parameters)
{
	if (!Sender->CurrentActionState) {
		int width = parameters->int1Parameter - parameters->int0Parameter;
		if (width < 2) {
			width = parameters->int0Parameter;
		} else {
			width = RAND(0, width - 1) + parameters->int0Parameter;
		}
		Sender->CurrentActionState = width * core->Time.defaultTicksPerSec;
	} else {
		Sender->CurrentActionState--;
	}

	if (!Sender->CurrentActionState) {
		Sender->ReleaseCurrentAction();
		return;
	}

	assert(Sender->CurrentActionState >= 0);
}

void GameScript::Wait(Scriptable* Sender, Action* parameters)
{
	if (!Sender->CurrentActionState) {
		Sender->CurrentActionState = parameters->int0Parameter * core->Time.defaultTicksPerSec;
	} else {
		Sender->CurrentActionState--;
	}

	if (!Sender->CurrentActionState) {
		Sender->ReleaseCurrentAction();
		return;
	}

	assert(Sender->CurrentActionState >= 0);
}

void GameScript::SmallWait(Scriptable* Sender, Action* parameters)
{
	if (!Sender->CurrentActionState) {
		Sender->CurrentActionState = parameters->int0Parameter;
	} else {
		Sender->CurrentActionState--;
	}

	if (!Sender->CurrentActionState) {
		Sender->ReleaseCurrentAction();
	}

	assert(Sender->CurrentActionState >= 0);
}

void GameScript::SmallWaitRandom(Scriptable* Sender, Action* parameters)
{
	if (!Sender->CurrentActionState) {
		int random = parameters->int1Parameter - parameters->int0Parameter;
		if (random < 1) {
			random = 1;
		}
		Sender->CurrentActionState = RAND(0, random - 1) + parameters->int0Parameter;
	} else {
		Sender->CurrentActionState--;
	}

	if (!Sender->CurrentActionState) {
		Sender->ReleaseCurrentAction();
	}

	assert(Sender->CurrentActionState >= 0);
}

// these two don't appear to be blocking at all - also marked as instant in all but iwd2
// this test script for the bg1 start clearly doesn't wait for the move to finish
// IF
// 	HotKey(K)
// THEN
// 	RESPONSE #100
// 		MoveViewPoint([1400.500],10)
// 		MoveToPoint([1400.500])
// END
void GameScript::MoveViewPoint(Scriptable* /*Sender*/, Action* parameters)
{
	// disable centering if anything enabled it before us (eg. LeaveAreaLUA as in movie02a.bcs)
	GameControl* gc = core->GetGameControl();
	gc->SetScreenFlags(ScreenFlags::CenterOnActor, BitOp::NAND);
	core->timer.SetMoveViewPort(parameters->pointParameter, parameters->int0Parameter << 1, true);
}

void GameScript::MoveViewObject(Scriptable* Sender, Action* parameters)
{
	const Scriptable* scr = GetScriptableFromObject(Sender, parameters);
	if (!scr) {
		Sender->ReleaseCurrentAction();
		return;
	}
	core->timer.SetMoveViewPort(scr->Pos, parameters->int0Parameter << 1, true);
}

void GameScript::MoveViewPointUntilDone(Scriptable* Sender, Action* parameters)
{
	// skip after first run
	if (Sender->CurrentActionTicks) {
		if (core->timer.ViewportIsMoving()) {
			return;
		} else {
			Sender->ReleaseCurrentAction();
			return;
		}
	}

	// disable centering if anything enabled it before us (eg. LeaveAreaLUA as in movie02a.bcs)
	GameControl* gc = core->GetGameControl();
	gc->SetScreenFlags(ScreenFlags::CenterOnActor, BitOp::NAND);
	core->timer.SetMoveViewPort(parameters->pointParameter, parameters->int0Parameter << 1, true);
}

void GameScript::AddWayPoint(Scriptable* Sender, Action* parameters)
{
	Actor* actor = Scriptable::As<Actor>(Sender);
	if (!actor) {
		Sender->ReleaseCurrentAction();
		return;
	}

	actor->AddWayPoint(parameters->pointParameter);
	// this is marked as AF_BLOCKING (and indeed AddWayPoint causes moves),
	// but this probably needs more thought
	Sender->ReleaseCurrentAction();
}

void GameScript::MoveToPointNoRecticle(Scriptable* Sender, Action* parameters)
{
	Actor* actor = Scriptable::As<Actor>(Sender);
	if (!actor) {
		Sender->ReleaseCurrentAction();
		return;
	}
	if (!actor->InMove() || actor->Destination != parameters->pointParameter) {
		actor->WalkTo(parameters->pointParameter, IF_NORETICLE);
	}
	if (!actor->InMove()) {
		// we should probably instead keep retrying until we reach dest
		actor->ClearPath();
		Sender->ReleaseCurrentAction();
	}
}

void GameScript::MoveToPointNoInterrupt(Scriptable* Sender, Action* parameters)
{
	Actor* actor = Scriptable::As<Actor>(Sender);
	if (!actor) {
		Sender->ReleaseCurrentAction();
		return;
	}
	if (!actor->InMove() || actor->Destination != parameters->pointParameter) {
		actor->WalkTo(parameters->pointParameter, IF_NOINT);
	}
	// should we always force IF_NOINT here?
	if (!actor->InMove()) {
		// we should probably instead keep retrying until we reach dest
		actor->Interrupt();
		actor->ClearPath();
		Sender->ReleaseCurrentAction();
	}
}

void GameScript::RunToPointNoRecticle(Scriptable* Sender, Action* parameters)
{
	Actor* actor = Scriptable::As<Actor>(Sender);
	if (!actor) {
		Sender->ReleaseCurrentAction();
		return;
	}
	if (!actor->InMove() || actor->Destination != parameters->pointParameter) {
		actor->SetOrientation(actor->Pos, parameters->pointParameter, false);
		actor->WalkTo(parameters->pointParameter, IF_NORETICLE | IF_RUNNING);
	}
	if (!actor->InMove()) {
		// we should probably instead keep retrying until we reach dest
		actor->ClearPath();
		Sender->ReleaseCurrentAction();
	}
}

void GameScript::RunToPoint(Scriptable* Sender, Action* parameters)
{
	Actor* actor = Scriptable::As<Actor>(Sender);
	if (!actor) {
		Sender->ReleaseCurrentAction();
		return;
	}
	if (!actor->InMove() || actor->Destination != parameters->pointParameter) {
		actor->SetOrientation(actor->Pos, parameters->pointParameter, false);
		actor->WalkTo(parameters->pointParameter, IF_RUNNING);
	}
	if (!actor->InMove()) {
		// we should probably instead keep retrying until we reach dest
		actor->ClearPath();
		Sender->ReleaseCurrentAction();
	}
}

//movetopoint until timer is down or target reached
void GameScript::TimedMoveToPoint(Scriptable* Sender, Action* parameters)
{
	Actor* actor = Scriptable::As<Actor>(Sender);
	if (!actor) {
		Sender->ReleaseCurrentAction();
		return;
	}
	if (parameters->int0Parameter <= 0) {
		if (actor->Pos != parameters->pointParameter) {
			actor->SetPosition(parameters->pointParameter, true);
		}
		Sender->ReleaseCurrentAction();
		return;
	}

	if (!actor->InMove() || actor->Destination != parameters->pointParameter) {
		actor->WalkTo(parameters->pointParameter, 0, parameters->int1Parameter);
	}

	//hopefully this hack will prevent lockups
	if (!actor->InMove()) {
		// we should probably instead keep retrying until we reach dest
		actor->ClearPath();
		Sender->ReleaseCurrentAction();
		return;
	}

	//repeat movement...
	if (parameters->int0Parameter > 0) {
		Action* newaction = ParamCopyNoOverride(parameters);
		newaction->int0Parameter--;
		actor->AddActionInFront(newaction);
		Sender->SetWait(1);
	}

	Sender->ReleaseCurrentAction();
}

void GameScript::MoveToPoint(Scriptable* Sender, Action* parameters)
{
	Actor* actor = Scriptable::As<Actor>(Sender);
	if (!actor) {
		Sender->ReleaseCurrentAction();
		return;
	}

	// iwd2 is the only one with special handling:
	// -2 is used as HomeLocation; no other unusual values are used
	if (parameters->pointParameter.x < 0) {
		parameters->pointParameter = actor->HomeLocation;
	}

	// try the actual move, if we are not already moving there
	if (!actor->InMove() || actor->Destination != parameters->pointParameter) {
		actor->WalkTo(parameters->pointParameter, parameters->int0Parameter);
	}

	// give up if we can't move there (no path was found)
	if (!actor->InMove()) {
		// we should probably instead keep retrying until we reach dest
		actor->ClearPath();
		Sender->ReleaseCurrentAction();
	}
}

//bg2, jumps to saved location in variable
void GameScript::MoveToSavedLocation(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetScriptableFromObject(Sender, parameters);
	if (!tar) {
		tar = Sender;
	}
	Actor* actor = Scriptable::As<Actor>(tar);
	if (!actor) {
		Sender->ReleaseCurrentAction();
		return;
	}

	Point p = CheckPointVariable(Sender, parameters->string0Parameter);
	actor->SetPosition(p, true);
	Sender->ReleaseCurrentAction();
}
/** iwd2 returntosavedlocation (with stats) */
/** pst returntosavedplace */
/** use Sender as default subject */
void GameScript::ReturnToSavedLocation(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetScriptableFromObject(Sender, parameters, GA_NO_DEAD);
	if (!tar) {
		tar = Sender;
	}
	Actor* actor = Scriptable::As<Actor>(tar);
	if (!actor) {
		Sender->ReleaseCurrentAction();
		return;
	}
	Point p(actor->GetBase(IE_SAVEDXPOS), actor->GetBase(IE_SAVEDYPOS));
	if (p.IsZero()) {
		Sender->ReleaseCurrentAction();
		return;
	}
	if (!actor->InMove() || actor->Destination != p) {
		actor->WalkTo(p, 0);
	}
	if (!actor->InMove()) {
		// we should probably instead keep retrying until we reach dest
		actor->ClearPath();
		Sender->ReleaseCurrentAction();
	}
}

//PST
void GameScript::RunToSavedLocation(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetScriptableFromObject(Sender, parameters, GA_NO_DEAD);
	if (!tar) {
		tar = Sender;
	}
	Actor* actor = Scriptable::As<Actor>(tar);
	if (!actor) {
		Sender->ReleaseCurrentAction();
		return;
	}
	Point p(actor->GetBase(IE_SAVEDXPOS), actor->GetBase(IE_SAVEDYPOS));
	// the stat above is not set by default as in iwds
	if (p.IsZero()) {
		p = actor->HomeLocation;
	}
	if (p.IsZero()) {
		Sender->ReleaseCurrentAction();
		return;
	}
	if (!actor->InMove() || actor->Destination != p) {
		actor->WalkTo(p, IF_RUNNING);
	}
	if (!actor->InMove()) {
		// we should probably instead keep retrying until we reach dest
		actor->ClearPath();
		Sender->ReleaseCurrentAction();
	}
}

//iwd2
void GameScript::ReturnToSavedLocationDelete(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetScriptableFromObject(Sender, parameters, GA_NO_DEAD);
	if (!tar) {
		tar = Sender;
	}
	Actor* actor = Scriptable::As<Actor>(tar);
	if (!actor) {
		Sender->ReleaseCurrentAction();
		return;
	}
	Point p(actor->GetBase(IE_SAVEDXPOS), actor->GetBase(IE_SAVEDYPOS));
	actor->SetBase(IE_SAVEDXPOS, 0);
	actor->SetBase(IE_SAVEDYPOS, 0);
	if (p.IsZero()) {
		Sender->ReleaseCurrentAction();
		return;
	}
	if (!actor->InMove() || actor->Destination != p) {
		actor->WalkTo(p, 0);
	}
	//what else?
	if (!actor->InMove()) {
		// we should probably instead keep retrying until we reach dest
		actor->ClearPath();
		Sender->ReleaseCurrentAction();
	}
}

void GameScript::ReturnToStartLocation(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetScriptableFromObject(Sender, parameters, GA_NO_DEAD);
	if (!tar) {
		tar = Sender;
	}
	Actor* actor = Scriptable::As<Actor>(tar);
	if (!actor) {
		Sender->ReleaseCurrentAction();
		return;
	}
	Point p = actor->HomeLocation;
	if (p.IsZero()) {
		Sender->ReleaseCurrentAction();
		return;
	}
	if (!actor->InMove() || actor->Destination != p) {
		actor->WalkTo(p, 0, parameters->int0Parameter);
	}
	if (!actor->InMove()) {
		// we should probably instead keep retrying until we reach dest
		actor->ClearPath();
		Sender->ReleaseCurrentAction();
	}
}

void GameScript::TriggerWalkTo(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetScriptableFromObject(Sender, parameters, 0);
	if (!tar) {
		Sender->ReleaseCurrentAction();
		return;
	}
	MoveToObjectCore(Sender, parameters, 0, false);
	tar->AddTrigger(TriggerEntry(trigger_walkedtotrigger, Sender->GetGlobalID()));
}

void GameScript::MoveToObjectNoInterrupt(Scriptable* Sender, Action* parameters)
{
	MoveToObjectCore(Sender, parameters, IF_NOINT, false);
}

void GameScript::RunToObject(Scriptable* Sender, Action* parameters)
{
	MoveToObjectCore(Sender, parameters, IF_RUNNING, false);
}

void GameScript::MoveToObject(Scriptable* Sender, Action* parameters)
{
	MoveToObjectCore(Sender, parameters, 0, false);
}

void GameScript::MoveToObjectUntilSee(Scriptable* Sender, Action* parameters)
{
	MoveToObjectCore(Sender, parameters, 0, true);
}

void GameScript::MoveToObjectFollow(Scriptable* Sender, Action* parameters)
{
	Actor* actor = Scriptable::As<Actor>(Sender);
	if (!actor) {
		Sender->ReleaseCurrentAction();
		return;
	}
	const Scriptable* target = GetStoredActorFromObject(Sender, parameters);
	const Actor* tar = Scriptable::As<Actor>(target);
	if (!tar) {
		Sender->ReleaseCurrentAction();
		return;
	}
	//follow leader from a distance of 5
	//could also follow the leader with a point offset
	if (tar) {
		actor->SetLeader(tar, 5);
	}
	MoveNearerTo(Sender, target, MAX_OPERATING_DISTANCE);
}

void GameScript::StorePartyLocation(Scriptable* /*Sender*/, Action* /*parameters*/)
{
	Game* game = core->GetGame();
	for (int i = 0; i < game->GetPartySize(false); i++) {
		const Actor* act = game->GetPC(i, false);
		GAMLocationEntry* gle = game->GetSavedLocationEntry(i);
		if (act && gle) {
			gle->Pos = act->Pos;
			gle->AreaResRef = act->AreaName;
		}
	}
}

void GameScript::RestorePartyLocation(Scriptable* /*Sender*/, Action* /*parameters*/)
{
	Game* game = core->GetGame();
	for (int i = 0; i < game->GetPartySize(false); i++) {
		Actor* act = game->GetPC(i, false);
		if (act) {
			const GAMLocationEntry* gle;
			if (game->GetSavedLocationCount() <= (unsigned int) i) {
				// no location, meaning the actor joined the party after the save
				// reuse the last valid location
				gle = game->GetSavedLocationEntry(game->GetSavedLocationCount() - 1);
			} else {
				gle = game->GetSavedLocationEntry(i);
			}
			MoveBetweenAreasCore(act, gle->AreaResRef, gle->Pos, -1, true);
		}
	}

	// presumably this is correct
	game->ClearSavedLocations();
}

void GameScript::MoveToCenterOfScreen(Scriptable* Sender, Action* /*parameters*/)
{
	Actor* actor = Scriptable::As<Actor>(Sender);
	if (!actor) {
		Sender->ReleaseCurrentAction();
		return;
	}
	Region vp = core->GetGameControl()->Viewport();
	Point p = vp.Center();
	if (!actor->InMove() || actor->Destination != p) {
		actor->WalkTo(p, IF_NOINT);
	}
	if (!actor->InMove()) {
		// we should probably instead keep retrying until we reach dest
		actor->Interrupt();
		actor->ClearPath();
		Sender->ReleaseCurrentAction();
	}
}

void GameScript::MoveToOffset(Scriptable* Sender, Action* parameters)
{
	Actor* actor = Scriptable::As<Actor>(Sender);
	if (!actor) {
		Sender->ReleaseCurrentAction();
		return;
	}
	Point p = Sender->Pos + parameters->pointParameter;
	if (!actor->InMove() || actor->Destination != p) {
		actor->WalkTo(p, 0);
	}
	if (!actor->InMove()) {
		// we should probably instead keep retrying until we reach dest
		actor->ClearPath();
		Sender->ReleaseCurrentAction();
	}
}

void GameScript::RunAwayFrom(Scriptable* Sender, Action* parameters)
{
	RunAwayFromCore(Sender, parameters, RunAwayFlags::LeaveArea);
}

void GameScript::RunAwayFromNoLeaveArea(Scriptable* Sender, Action* parameters)
{
	RunAwayFromCore(Sender, parameters, 0);
}

void GameScript::RunAwayFromNoInterrupt(Scriptable* Sender, Action* parameters)
{
	RunAwayFromCore(Sender, parameters, RunAwayFlags::NoInterrupt | RunAwayFlags::LeaveArea);
}

void GameScript::RunAwayFromNoInterruptNoLeaveArea(Scriptable* Sender, Action* parameters)
{
	RunAwayFromCore(Sender, parameters, RunAwayFlags::NoInterrupt);
}

void GameScript::RunAwayFromPoint(Scriptable* Sender, Action* parameters)
{
	RunAwayFromCore(Sender, parameters, RunAwayFlags::LeaveArea | RunAwayFlags::UsePoint);
}

void GameScript::DisplayStringNoName(Scriptable* Sender, Action* parameters)
{
	Scriptable* target = GetScriptableFromObject(Sender, parameters);
	if (!target) {
		target = Sender;
	}
	if (Sender->Type == ST_ACTOR) {
		DisplayStringCore(target, ieStrRef(parameters->int0Parameter), DS_CONSOLE | DS_NONAME);
	} else {
		// Virtue calls this from the global script, but maybe Pos is ok for areas
		// set DS_CONSOLE only for ST_GLOBAL if it turns out areas don't care;
		// could also be dependent on the subtitle setting, see DisplayStringCore
		DisplayStringCore(target, ieStrRef(parameters->int0Parameter), DS_AREA | DS_CONSOLE | DS_NONAME);
	}
}

void GameScript::DisplayStringNoNameHead(Scriptable* Sender, Action* parameters)
{
	Scriptable* target = GetScriptableFromObject(Sender, parameters);
	if (!target) {
		target = Sender;
	}

	DisplayStringCore(target, ieStrRef(parameters->int0Parameter), DS_HEAD | DS_CONSOLE | DS_NONAME);
}

void GameScript::DisplayMessage(Scriptable* Sender, Action* parameters)
{
	DisplayStringCore(Sender, ieStrRef(parameters->int0Parameter), DS_CONSOLE | DS_NONAME);
}

//float message over target
void GameScript::DisplayStringHead(Scriptable* Sender, Action* parameters)
{
	Scriptable* target = GetScriptableFromObject(Sender, parameters);
	if (!target) {
		target = Sender;
		Log(WARNING, "Actions", "DisplayStringHead/FloatMessage got no target, assuming Sender!");
	}

	int flags = DS_HEAD | DS_SPEECH;
	int strRef = parameters->int0Parameter;
	if (core->HasFeature(GFFlags::ONSCREEN_TEXT) && strRef > 1000000) {
		flags |= DS_APPEND; // force append, so several messages can be queued
		strRef -= 1000000;
	}
	DisplayStringCore(target, ieStrRef(strRef), flags);
}

void GameScript::KillFloatMessage(Scriptable* Sender, Action* parameters)
{
	Scriptable* target = GetScriptableFromObject(Sender, parameters);
	if (!target) {
		target = Sender;
	}
	// we could kill the whole overhead queue, but no need has been shown so far
	target->overHead.Display(false, 0);
}

void GameScript::DisplayStringHeadOwner(Scriptable* /*Sender*/, Action* parameters)
{
	const Game* game = core->GetGame();

	int i = game->GetPartySize(true);
	while (i--) {
		Actor* actor = game->GetPC(i, true);
		if (actor->inventory.HasItem(parameters->resref0Parameter, 0)) {
			DisplayStringCore(actor, ieStrRef(parameters->int0Parameter), DS_CONSOLE | DS_HEAD);
		}
	}
}

static void FloatMessageAtPoint(Scriptable* Sender, const Point& pos, const ieStrRef& msgRef)
{
	Map* map = Sender->GetCurrentArea();
	if (!map) return;
	String msg = core->GetString(msgRef);
	map->overHead.SetText(std::move(msg));
	map->overHead.FixPos(pos);
}

// only used three times by cranium rats
void GameScript::FloatMessageFixed(Scriptable* Sender, Action* parameters)
{
	FloatMessageAtPoint(Sender, parameters->pointParameter, ieStrRef(parameters->int0Parameter));
}

// unused in the original
void GameScript::FloatMessageFixedRnd(Scriptable* Sender, Action* parameters)
{
	const SrcVector* strList = gamedata->SrcManager.GetSrc(parameters->resref0Parameter);
	if (strList->IsEmpty()) {
		Log(ERROR, "GameScript", "Cannot display resource!");
		return;
	}

	const ieStrRef& msgRef = strList->RandomRef();
	FloatMessageAtPoint(Sender, parameters->pointParameter, msgRef);
}

void GameScript::FloatMessageRnd(Scriptable* Sender, Action* parameters)
{
	Scriptable* target = GetScriptableFromObject(Sender, parameters);
	if (!target) {
		target = Sender;
		Log(ERROR, "GameScript", "DisplayStringHead/FloatMessage got no target, assuming Sender!");
	}

	const SrcVector* strList = gamedata->SrcManager.GetSrc(parameters->resref0Parameter);
	if (strList->IsEmpty()) {
		Log(ERROR, "GameScript", "Cannot display resource!");
		return;
	}
	DisplayStringCore(target, strList->RandomRef(), DS_CONSOLE | DS_HEAD);
}

void GameScript::DisplayStringHeadNoLog(Scriptable* Sender, Action* parameters)
{
	Scriptable* target = GetScriptableFromObject(Sender, parameters);
	if (!target) return;

	String msg = core->GetString(ieStrRef(parameters->int0Parameter));
	target->overHead.SetText(std::move(msg));
}

//apparently this should not display over head (for actors)
void GameScript::DisplayString(Scriptable* Sender, Action* parameters)
{
	Scriptable* target = GetScriptableFromObject(Sender, parameters);
	if (!target) {
		target = Sender;
	}

	int flags = DS_CONSOLE;

	if (Sender->Type != ST_ACTOR) {
		flags |= DS_AREA;
	}

	DisplayStringCore(target, ieStrRef(parameters->int0Parameter), flags);
}

//DisplayStringHead, but wait until done
void GameScript::DisplayStringWait(Scriptable* Sender, Action* parameters)
{
	ieDword gt = core->GetGame()->GameTime;
	if (Sender->CurrentActionState) {
		if (gt >= (ieDword) parameters->int2Parameter) {
			Sender->ReleaseCurrentAction();
		}
		return;
	}
	Scriptable* target = GetScriptableFromObject(Sender, parameters);
	if (!target) {
		target = Sender;
	}
	DisplayStringCore(target, ieStrRef(parameters->int0Parameter), DS_CONSOLE | DS_WAIT | DS_SPEECH | DS_HEAD);
	Sender->CurrentActionState = 1;
	// parameters->int2Parameter is unused here so hijack it to store the wait time
	// and make sure we wait at least one round, so strings without audio have some time to display
	unsigned long waitCounter = target->GetWait();
	parameters->int2Parameter = gt + (waitCounter > 0 ? waitCounter : core->Time.round_size);
}

void GameScript::ForceFacing(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetScriptableFromObject(Sender, parameters);
	Actor* actor = Scriptable::As<Actor>(tar);
	if (!actor) {
		Sender->ReleaseCurrentAction();
		return;
	}
	actor->SetOrientation(ClampToOrientation(parameters->int0Parameter), false);
}

/* A -1 means random facing? */
void GameScript::Face(Scriptable* Sender, Action* parameters)
{
	Actor* actor = Scriptable::As<Actor>(Sender);
	if (!actor) {
		Sender->ReleaseCurrentAction();
		return;
	}
	if (parameters->int0Parameter == -1) {
		actor->SetOrientation(RandomOrientation(), false);
	} else {
		actor->SetOrientation(ClampToOrientation(parameters->int0Parameter), false);
	}
	actor->SetWait(1);
	Sender->ReleaseCurrentAction(); // todo, blocking?
}

void GameScript::FaceObject(Scriptable* Sender, Action* parameters)
{
	Actor* actor = Scriptable::As<Actor>(Sender);
	if (!actor) {
		Sender->ReleaseCurrentAction();
		return;
	}
	const Scriptable* target = GetScriptableFromObject(Sender, parameters);
	if (!target) {
		Sender->ReleaseCurrentAction();
		return;
	}
	actor->SetOrientation(actor->Pos, target->Pos, false);
	actor->SetWait(1);
	Sender->ReleaseCurrentAction(); // todo, blocking?
}

void GameScript::FaceSavedLocation(Scriptable* Sender, Action* parameters)
{
	Scriptable* target = GetScriptableFromObject(Sender, parameters);
	Actor* actor = Scriptable::As<Actor>(target);
	if (!actor) {
		Sender->ReleaseCurrentAction();
		return;
	}
	if (parameters->variable0Parameter.IsEmpty()) {
		parameters->variable0Parameter = "LOCALSsavedlocation";
	}
	Point p = CheckPointVariable(target, parameters->string0Parameter);

	actor->SetOrientation(actor->Pos, p, false);
	actor->SetWait(1);
	Sender->ReleaseCurrentAction(); // todo, blocking?
}

// pst and bg2 can play a song designated by index (songs.ids in pst)
//actually pst has some extra params not currently implemented (never used - always the same)
//switchplaylist implements fade by simply scheduling the next
//music after the currently running one
//FIXME: This code is similar to PlayAreaSong, consider refactoring
void GameScript::StartSong(Scriptable* /*Sender*/, Action* parameters)
{
	//the force play logic should be handled by SwitchPlayList
	bool force;
	const ieVariable& poi = core->GetMusicPlaylist(parameters->int0Parameter);
	if (IsStar(poi)) return;

	// if fade parameter is force, force the music, otherwise just schedule it for next
	// NOTE: see songflag.ids for potential modes
	if (parameters->int1Parameter == 1) {
		force = true;
	} else {
		force = false;
	}
	int ret = core->GetMusicMgr()->SwitchPlayList(poi, force);
	if (ret) {
		core->DisableMusicPlaylist(parameters->int0Parameter);
	}
}

//starts the current area music (songtype is in int0Parameter)
//PlayAreaSong will set the CombatCounter to 150 if
//it is battlemusic (the Counter will tick back to 0)
void GameScript::StartMusic(Scriptable* Sender, Action* parameters)
{
	//don't break on bad values
	if (parameters->int0Parameter >= 10) return;
	const Map* map = Sender->GetCurrentArea();
	if (!map) return;
	bool force, restart;

	// values from mflags.ids
	// in the originals it's only used with QUICK_FADE, otherwise we'd have a few todos here
	switch (parameters->int1Parameter) {
		case 1: //force switch
			force = true;
			restart = true;
			break;
		case 3: // QUICK_FADE
			//force switch, but wait for previous music to end gracefully
			force = false;
			restart = true;
			break;
		case 2: // play
		case 4: // SLOW_FADE
		default:
			force = false;
			restart = false;
			break;
	}
	map->PlayAreaSong(parameters->int0Parameter, restart, force);
}

void GameScript::StartCombatCounter(Scriptable* Sender, Action* /*parameters*/)
{
	const Map* map = Sender->GetCurrentArea();
	if (!map) return;
	map->PlayAreaSong(SONG_BATTLE, true, true);
}

/*iwd2 can set an areasong slot*/
void GameScript::SetMusic(Scriptable* Sender, Action* parameters)
{
	// iwd2 allows setting all 10 slots (musics.ids), though there is no evidence they are used
	// int1Parameter is from music.ids or songlist.ids (ees)
	if (parameters->int0Parameter >= 10) return;
	Map* map = Sender->GetCurrentArea();
	if (!map) return;
	map->SongList[parameters->int0Parameter] = parameters->int1Parameter;
}

//optional integer parameter (isSpeech)
void GameScript::PlaySound(Scriptable* Sender, Action* parameters)
{
	Log(MESSAGE, "Actions", "PlaySound({})", parameters->string0Parameter);

	if (parameters->int0Parameter) {
		auto config = core->GetAudioSettings().ConfigPresetByChannel(SFXChannel::Char0, Sender->Pos);
		core->GetAudioPlayback().PlaySpeech(parameters->string0Parameter, config);
	} else {
		core->GetAudioPlayback().Play(parameters->string0Parameter, AudioPreset::Spatial, SFXChannel::Char0, Sender->Pos);
	}
}

void GameScript::PlaySoundPoint(Scriptable* /*Sender*/, Action* parameters)
{
	Log(MESSAGE, "Actions", "PlaySound({})", parameters->string0Parameter);
	core->GetAudioPlayback().Play(parameters->string0Parameter, AudioPreset::Spatial, SFXChannel::Actions, parameters->pointParameter);
}

void GameScript::PlaySoundNotRanged(Scriptable* /*Sender*/, Action* parameters)
{
	Log(MESSAGE, "Actions", "PlaySound({})", parameters->string0Parameter);
	core->GetAudioPlayback().Play(parameters->string0Parameter, AudioPreset::ScreenAction, SFXChannel::Actions);
}

void GameScript::Continue(Scriptable* /*Sender*/, Action* /*parameters*/)
{
}

// creates area vvc at position of object
void GameScript::CreateVisualEffectObject(Scriptable* Sender, Action* parameters)
{
	const Scriptable* tar = GetScriptableFromObject(Sender, parameters);
	if (!tar) {
		return;
	}
	CreateVisualEffectCore(tar, tar->Pos, parameters->resref0Parameter, parameters->int0Parameter);
}

// creates sticky vvc on actor or normal animation on object
void GameScript::CreateVisualEffectObjectSticky(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetScriptableFromObject(Sender, parameters);
	if (!tar) {
		return;
	}
	if (tar->Type == ST_ACTOR) {
		CreateVisualEffectCore(static_cast<Actor*>(tar), parameters->resref0Parameter, parameters->int0Parameter);
	} else {
		CreateVisualEffectCore(tar, tar->Pos, parameters->resref0Parameter, parameters->int0Parameter);
	}
}

// creates area effect at point
void GameScript::CreateVisualEffect(Scriptable* Sender, Action* parameters)
{
	CreateVisualEffectCore(Sender, parameters->pointParameter, parameters->resref0Parameter, parameters->int0Parameter);
}

void GameScript::DestroySelf(Scriptable* Sender, Action* /*parameters*/)
{
	Actor* actor = Scriptable::As<Actor>(Sender);
	if (!actor) {
		return;
	}
	actor->DestroySelf();
	// needed in pst #532, but softly breaks bg2 #1179
	if (actor == core->GetCutSceneRunner() && core->HasFeature(GFFlags::PST_STATE_FLAGS)) {
		core->SetCutSceneMode(false);
	}
}

void GameScript::ScreenShake(Scriptable* /*Sender*/, Action* parameters)
{
	if (parameters->int1Parameter) { //IWD2 has a different profile
		Point p(parameters->int1Parameter, parameters->int2Parameter);
		core->timer.SetScreenShake(p, parameters->int0Parameter);
	} else {
		core->timer.SetScreenShake(parameters->pointParameter, parameters->int0Parameter);
	}
}

void GameScript::UnhideGUI(Scriptable* /*Sender*/, Action* /*parameters*/)
{
	Game* game = core->GetGame();
	game->SetControlStatus(CS_HIDEGUI, BitOp::NAND);
}

void GameScript::HideGUI(Scriptable* /*Sender*/, Action* /*parameters*/)
{
	Game* game = core->GetGame();
	game->SetControlStatus(CS_HIDEGUI, BitOp::OR);
}

void GameScript::LockScroll(Scriptable* /*Sender*/, Action* /*parameters*/)
{
	GameControl* gc = core->GetGameControl();
	if (gc) {
		gc->SetScreenFlags(ScreenFlags::CenterOnActor, BitOp::OR);
		gc->SetScreenFlags(ScreenFlags::AlwaysCenter, BitOp::OR);
	}
}

void GameScript::UnlockScroll(Scriptable* /*Sender*/, Action* /*parameters*/)
{
	GameControl* gc = core->GetGameControl();
	if (gc) {
		gc->SetScreenFlags(ScreenFlags::CenterOnActor, BitOp::NAND);
		gc->SetScreenFlags(ScreenFlags::AlwaysCenter, BitOp::NAND);
	}
}

//no string, increase talkcount, no interrupt
void GameScript::Dialogue(Scriptable* Sender, Action* parameters)
{
	BeginDialog(Sender, parameters, BD_SOURCE | BD_TALKCOUNT | BD_CHECKDIST);
}

void GameScript::DialogueForceInterrupt(Scriptable* Sender, Action* parameters)
{
	BeginDialog(Sender, parameters, BD_SOURCE | BD_TALKCOUNT | BD_INTERRUPT);
}

// not in IESDP but this one should affect ambients
void GameScript::SoundActivate(Scriptable* /*Sender*/, Action* parameters)
{
	AmbientMgr& ambientmgr = core->GetAmbientManager();
	if (parameters->int0Parameter) {
		ambientmgr.Activate(parameters->objects[1]->objectName);
	} else {
		ambientmgr.Deactivate(parameters->objects[1]->objectName);
	}
}

// according to IESDP this action is about animations
//PST's SetCorpseEnabled also handles containers, but no one uses it
void GameScript::AmbientActivate(Scriptable* Sender, Action* parameters)
{
	AmbientActivateCore(Sender, parameters, parameters->int0Parameter);
}

void GameScript::ChangeTileState(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetScriptableFromObject(Sender, parameters, GA_NO_DEAD);
	Door* door = Scriptable::As<Door>(tar);
	if (!door) {
		return;
	}

	door->ToggleTiles(parameters->int0Parameter); // default is false for playsound
}

void GameScript::StaticStart(Scriptable* Sender, Action* parameters)
{
	AreaAnimation* anim = Sender->GetCurrentArea()->GetAnimation(parameters->objects[1]->objectNameVar);
	if (!anim) {
		Log(WARNING, "Actions", "Script error: No Animation Named \"{}\"", parameters->objects[1]->objectName);
		return;
	}
	anim->animFlags &= ~Animation::Flags::Once;
}

void GameScript::StaticStop(Scriptable* Sender, Action* parameters)
{
	AreaAnimation* anim = Sender->GetCurrentArea()->GetAnimation(parameters->objects[1]->objectNameVar);
	if (!anim) {
		Log(WARNING, "Actions", "Script error: No Animation Named \"{}\"", parameters->objects[1]->objectName);
		return;
	}
	anim->animFlags |= Animation::Flags::Once;
}

void GameScript::StaticPalette(Scriptable* Sender, Action* parameters)
{
	AreaAnimation* anim = Sender->GetCurrentArea()->GetAnimation(parameters->objects[1]->objectNameVar);
	if (!anim) {
		Log(WARNING, "Actions", "Script error: No Animation Named \"{}\"", parameters->objects[1]->objectName);
		return;
	}
	anim->SetPalette(parameters->resref0Parameter);
}

//this is a special case of PlaySequence (with wait time, not for area anims)
void GameScript::PlaySequenceTimed(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar;
	if (parameters->objects[1]) {
		tar = GetScriptableFromObject(Sender, parameters);
	} else {
		tar = Sender;
	}
	Actor* actor = Scriptable::As<Actor>(tar);
	if (!actor) {
		return;
	}

	actor->SetStance(parameters->int0Parameter);
	int delay = parameters->int1Parameter;
	actor->SetWait(delay);
}

//waitanimation: waiting while animation of target is of a certain type
void GameScript::WaitAnimation(Scriptable* Sender, Action* parameters)
{
	const Scriptable* tar = GetScriptableFromObject(Sender, parameters);
	if (!tar) {
		tar = Sender;
	}
	const Actor* actor = Scriptable::As<Actor>(tar);
	if (!actor) {
		return;
	}

	// HACK HACK: avoid too long waits due to buggy AI evaluation
	if (actor->GetStance() != parameters->int0Parameter || parameters->int1Parameter > (signed) core->Time.round_size) {
		Sender->ReleaseCurrentAction();
		return;
	}
	parameters->int1Parameter++;
}

// the spell target and attack target are different only in iwd2
void GameScript::SetMyTarget(Scriptable* Sender, Action* parameters)
{
	const Scriptable* tar = GetScriptableFromObject(Sender, parameters);
	if (!tar) {
		// we got called with Nothing to invalidate the target
		Sender->objects.MyTarget = 0;
		return;
	}
	Sender->objects.MyTarget = tar->GetGlobalID();
}

// PlaySequence without object parameter defaults to Sender
void GameScript::PlaySequence(Scriptable* Sender, Action* parameters)
{
	PlaySequenceCore(Sender, parameters, parameters->int0Parameter);
}

//same as PlaySequence, but the value comes from a variable
void GameScript::PlaySequenceGlobal(Scriptable* Sender, Action* parameters)
{
	ieDword value = CheckVariable(Sender, parameters->string0Parameter);
	PlaySequenceCore(Sender, parameters, value);
}

void GameScript::SetDialogue(Scriptable* Sender, Action* parameters)
{
	Actor* target = Scriptable::As<Actor>(Sender);
	if (!target) {
		return;
	}
	target->SetDialog(parameters->resref0Parameter);
}

void GameScript::ChangeDialogue(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetScriptableFromObject(Sender, parameters);
	Actor* target = Scriptable::As<Actor>(tar);
	if (!target) {
		return;
	}
	target->SetDialog(parameters->resref0Parameter);
}

//string0, no interrupt, talkcount increased
void GameScript::StartDialogue(Scriptable* Sender, Action* parameters)
{
	BeginDialog(Sender, parameters, BD_STRING0 | BD_TALKCOUNT | BD_SETDIALOG);
}

//string0, no interrupt, talkcount increased, don't set default
//optionally item name is used
void GameScript::StartDialogueOverride(Scriptable* Sender, Action* parameters)
{
	int flags = BD_STRING0 | BD_TALKCOUNT;

	if (parameters->int2Parameter) {
		flags |= BD_ITEM;
	}
	BeginDialog(Sender, parameters, flags);
}

//string0, no interrupt, talkcount increased, don't set default
//optionally item name is used
void GameScript::StartDialogueOverrideInterrupt(Scriptable* Sender,
						Action* parameters)
{
	int flags = BD_STRING0 | BD_TALKCOUNT | BD_INTERRUPT;

	if (parameters->int2Parameter) {
		flags |= BD_ITEM;
	}
	BeginDialog(Sender, parameters, flags);
}

//start talking to oneself
void GameScript::PlayerDialogue(Scriptable* Sender, Action* parameters)
{
	BeginDialog(Sender, parameters, BD_RESERVED | BD_OWN);
}

//we hijack this action for the player initiated dialogue
void GameScript::NIDSpecial1(Scriptable* Sender, Action* parameters)
{
	BeginDialog(Sender, parameters, BD_INTERRUPT | BD_TARGET /*| BD_NUMERIC*/ | BD_TALKCOUNT | BD_CHECKDIST);
}

void GameScript::NIDSpecial2(Scriptable* Sender, Action* /*parameters*/)
{
	const Actor* actor = Scriptable::As<Actor>(Sender);
	if (!actor) {
		Sender->ReleaseCurrentAction();
		return;
	}
	const Game* game = core->GetGame();
	if (!game->EveryoneStopped()) {
		//wait for a while
		Sender->SetWait(1 * core->Time.defaultTicksPerSec);
		return;
	}

	Map* map = actor->GetCurrentArea();
	if (!game->EveryoneNearPoint(map, actor->Pos, ENP::CanMove | ENP::Familars)) {
		//we abort the command, everyone should be here
		if (map->LastGoCloser < game->Ticks) {
			displaymsg->DisplayMsgCentered(HCStrings::WholeParty, FT_ANY, GUIColors::WHITE);
			map->LastGoCloser = game->Ticks + core->Time.round_size;
		}
		Sender->ReleaseCurrentAction();
		return;
	}

	//travel direction passed to guiscript
	WMPDirection direction = Sender->GetCurrentArea()->WhichEdge(actor->Pos);
	Log(MESSAGE, "Actions", "Travel direction returned: {}", static_cast<int>(direction));
	//this is notoriously flaky
	//if it doesn't work for the sender try other party members, too
	if (direction == WMPDirection::NONE) {
		int directions[4] = { -1, -1, -1, -1 };
		for (int i = 0; i < game->GetPartySize(false); i++) {
			actor = game->GetPC(i, false);
			if (actor == Sender) continue;

			WMPDirection partydir = actor->GetCurrentArea()->WhichEdge(actor->Pos);
			if (partydir != WMPDirection::NONE) {
				directions[static_cast<int>(partydir)]++;
			}
		}
		int best = 0;
		for (int i = 1; i < 4; ++i) {
			if (directions[i] > directions[best]) {
				best = i;
			}
		}
		if (directions[best] != -1) {
			direction = static_cast<WMPDirection>(best);
		}
		Log(DEBUG, "Actions", "Travel direction determined by party: {}", static_cast<int>(direction));
	}

	// pst enables worldmap travel only after visiting the lower ward
	bool keyAreaVisited = core->HasFeature(GFFlags::TEAM_MOVEMENT) && CheckVariable(Sender, "AR0500_Visited", "GLOBAL") == 1;
	if (direction == WMPDirection::NONE && !keyAreaVisited) {
		Sender->ReleaseCurrentAction();
		return;
	}
	// if we're now actually doing worldmap travel, pst ignores the direction and
	// just dumps you to the FROMMAP entrance. But we need to circumvent the usual logic here
	if (direction == WMPDirection::NONE && keyAreaVisited) {
		direction = WMPDirection::WEST;
	}
	core->GetGUIScriptEngine()->RunFunction("GUIMA", "OpenTravelWindow", direction);
	//sorry, i have absolutely no idea when i should do this :)
	Sender->ReleaseCurrentAction();
}

void GameScript::StartDialogueInterrupt(Scriptable* Sender, Action* parameters)
{
	BeginDialog(Sender, parameters,
		    BD_STRING0 | BD_INTERRUPT | BD_TALKCOUNT | BD_SETDIALOG);
}

//No string, flags:0
void GameScript::StartDialogueNoSet(Scriptable* Sender, Action* parameters)
{
	BeginDialog(Sender, parameters, BD_TALKCOUNT | BD_SOURCE);
}

void GameScript::StartDialogueNoSetInterrupt(Scriptable* Sender,
					     Action* parameters)
{
	BeginDialog(Sender, parameters, BD_TALKCOUNT | BD_SOURCE | BD_INTERRUPT);
}

//no talkcount, using banter dialogs
//probably banter dialogs are random, like rumours!
//no, they aren't, but they increase interactcount
void GameScript::Interact(Scriptable* Sender, Action* parameters)
{
	BeginDialog(Sender, parameters, BD_INTERACT | BD_NOEMPTY);
}

//this is an immediate action without checking Sender
void GameScript::DetectSecretDoor(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetScriptableFromObject(Sender, parameters, GA_NO_DEAD);
	Door* door = Scriptable::As<Door>(tar);
	if (!door) {
		return;
	}

	if (door->Flags & DOOR_SECRET) {
		door->Flags |= DOOR_FOUND;
	}
}

//this is an immediate action without checking Sender
void GameScript::Lock(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetScriptableFromObject(Sender, parameters);
	if (!tar) {
		return;
	}
	switch (tar->Type) {
		case ST_DOOR:
			static_cast<Door*>(tar)->SetDoorLocked(true, true);
			break;
		case ST_CONTAINER:
			static_cast<Container*>(tar)->SetContainerLocked(true);
			break;
		default:
			return;
	}
}

void GameScript::Unlock(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetScriptableFromObject(Sender, parameters);
	if (!tar) {
		return;
	}
	switch (tar->Type) {
		case ST_DOOR:
			static_cast<Door*>(tar)->SetDoorLocked(false, true);
			break;
		case ST_CONTAINER:
			static_cast<Container*>(tar)->SetContainerLocked(false);
			break;
		default:
			return;
	}
}

void GameScript::SetDoorLocked(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetScriptableFromObject(Sender, parameters);
	Door* door = Scriptable::As<Door>(tar);
	if (!door) {
		return;
	}

	// two dialog states in pst (and nothing else) use "FALSE" (yes, quoted)
	// they're on a critical path so we have to handle this data bug ourselves
	if (parameters->int0Parameter == -1) {
		parameters->int0Parameter = 0;
	}
	door->SetDoorLocked(parameters->int0Parameter != 0, false);
}

void GameScript::SetDoorFlag(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetScriptableFromObject(Sender, parameters);
	Door* door = Scriptable::As<Door>(tar);
	if (!door) {
		return;
	}

	ieDword flag = parameters->int0Parameter;

	//these are special flags
	if (flag & DOOR_LOCKED) {
		flag &= ~DOOR_LOCKED;
		door->SetDoorLocked(parameters->int1Parameter != 0, false);
	}
	if (flag & DOOR_OPEN) {
		flag &= ~DOOR_OPEN;
		door->SetDoorOpen(parameters->int1Parameter != 0, false, 0);
	}
	// take care of iwd2 flag bit differences as in AREIMporter's FixIWD2DoorFlags
	// ... it matters for exactly 1 user from the original data (20ctord3.bcs)
	if (core->HasFeature(GFFlags::RULES_3ED) && flag == DOOR_KEY) { // requesting SEETHROUGH
		flag = DOOR_TRANSPARENT;
	}

	if (parameters->int1Parameter) {
		door->Flags |= flag;
	} else {
		door->Flags &= ~flag;
	}
	if (flag & DOOR_TRANSPARENT) {
		// SetDoorOpen takes care of this for DOOR_OPEN and DOOR_LOCKED doesn't matter
		door->UpdateDoor();
	}
}

void GameScript::RemoveTraps(Scriptable* Sender, Action* parameters)
{
	//only actors may try to pick a lock
	Actor* actor = Scriptable::As<Actor>(Sender);
	if (!actor) {
		Sender->ReleaseCurrentAction();
		return;
	}
	Scriptable* tar = GetScriptableFromObject(Sender, parameters);
	if (!tar) {
		Sender->ReleaseCurrentAction();
		return;
	}

	unsigned int distance;
	const Point* p;
	Door* door = NULL;
	Container* container = NULL;
	InfoPoint* trigger = NULL;
	ScriptableType type = tar->Type;
	ieDword flags;

	switch (type) {
		case ST_DOOR:
			door = static_cast<Door*>(tar);
			if (door->IsOpen()) {
				//door is already open
				Sender->ReleaseCurrentAction();
				return;
			}

			p = door->GetClosestApproach(Sender, distance);
			flags = door->Trapped && door->TrapDetected;
			break;
		case ST_CONTAINER:
			container = static_cast<Container*>(tar);
			p = &container->Pos;
			distance = Distance(*p, Sender);
			flags = container->Trapped && container->TrapDetected;
			break;
		case ST_PROXIMITY:
			trigger = (InfoPoint*) tar;
			// this point is incorrect! will cause actor to enter trap
			// need to find a point using trigger->outline
			p = &trigger->Pos;
			distance = Distance(tar, Sender);
			flags = trigger->Trapped && trigger->TrapDetected && trigger->CanDetectTrap();
			actor->SetDisarmingTrap(trigger->GetGlobalID());
			break;
		default:
			Sender->ReleaseCurrentAction();
			return;
	}
	actor->SetOrientation(actor->Pos, *p, false);
	if (distance <= MAX_OPERATING_DISTANCE) {
		if (flags) {
			switch (type) {
				case ST_DOOR:
					door->TryDisarm(actor);
					break;
				case ST_CONTAINER:
					container->TryDisarm(actor);
					break;
				case ST_PROXIMITY:
					trigger->TryDisarm(actor);
					break;
				default:
					//not gonna happen!
					assert(false);
			}
		} else {
			//no trap here
			//displaymsg->DisplayString(HCStrings::NotTrapped);
		}
	} else {
		MoveNearerTo(Sender, *p, MAX_OPERATING_DISTANCE, 0);
		return;
	}
	Sender->SetWait(1);
	Sender->ReleaseCurrentAction();
}

void GameScript::PickLock(Scriptable* Sender, Action* parameters)
{
	//only actors may try to pick a lock
	Actor* actor = Scriptable::As<Actor>(Sender);
	if (!actor) {
		Sender->ReleaseCurrentAction();
		return;
	}
	Scriptable* tar = GetScriptableFromObject(Sender, parameters);
	if (!tar) {
		Sender->ReleaseCurrentAction();
		return;
	}
	unsigned int distance;
	const Point* p;
	const Door* door = nullptr;
	ScriptableType type = tar->Type;

	switch (type) {
		case ST_DOOR:
			door = static_cast<Door*>(tar);
			if (door->IsOpen()) {
				//door is already open
				Sender->ReleaseCurrentAction();
				return;
			}
			p = door->GetClosestApproach(Sender, distance);
			break;
		case ST_CONTAINER:
			p = &tar->Pos;
			distance = Distance(*p, Sender);
			break;
		default:
			Sender->ReleaseCurrentAction();
			return;
	}

	Highlightable* lockMount = static_cast<Highlightable*>(tar);
	bool locked = lockMount->IsLocked();
	actor->SetOrientation(actor->Pos, *p, false);
	if (distance <= MAX_OPERATING_DISTANCE) {
		if (locked) {
			lockMount->TryPickLock(actor);
		} else {
			//notlocked
			//displaymsg->DisplayString(HCStrings::NotLocked);
		}
	} else {
		MoveNearerTo(Sender, *p, MAX_OPERATING_DISTANCE, 0);
		return;
	}
	Sender->SetWait(1);
	Sender->ReleaseCurrentAction();
}

void GameScript::OpenDoor(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetScriptableFromObject(Sender, parameters);
	Door* door = Scriptable::As<Door>(tar);
	if (!door) {
		return;
	}
	int gid = Sender->GetGlobalID();
	// no idea if this is right, or whether OpenDoor/CloseDoor should allow opening
	// of all doors, or some doors, or whether it should still check for non-actors
	Actor* actor = Scriptable::As<Actor>(Sender);
	if (actor) {
		actor->SetModal(Modal::None);
		if (!door->TryUnlock(actor)) {
			return;
		}
	}
	door->SetDoorOpen(true, false, gid, false);
	Sender->ReleaseCurrentAction();
}

void GameScript::CloseDoor(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetScriptableFromObject(Sender, parameters);
	Door* door = Scriptable::As<Door>(tar);
	if (!door) {
		return;
	}
	// see comments in OpenDoor above
	Actor* actor = Scriptable::As<Actor>(Sender);
	if (actor) {
		// clear modal state like in OpenDoor?
		if (!door->TryUnlock(actor)) {
			return;
		}
	}
	door->SetDoorOpen(false, false, 0);
	Sender->ReleaseCurrentAction();
}

void GameScript::ToggleDoor(Scriptable* Sender, Action* parameters)
{
	Actor* actor = Scriptable::As<Actor>(Sender);
	if (!actor) {
		Sender->ReleaseCurrentAction();
		return;
	}
	actor->SetModal(Modal::None);

	Door* door = actor->GetCurrentArea()->GetDoorByGlobalID(parameters->int0Parameter);
	if (!door) {
		Sender->ReleaseCurrentAction();
		return;
	}
	unsigned int distance;
	const Point* p = door->GetClosestApproach(Sender, distance);
	if (distance <= MAX_OPERATING_DISTANCE) {
		actor->SetOrientation(actor->Pos, *p, false);
		if (!door->TryUnlock(actor)) {
			if (door->Flags & DOOR_LOCKEDINFOTEXT && door->LockedStrRef != ieStrRef::INVALID) {
				displaymsg->DisplayString(door->LockedStrRef, GUIColors::LIGHTGREY, STRING_FLAGS::SOUND | STRING_FLAGS::SPEECH);
			} else {
				displaymsg->DisplayMsgAtLocation(HCStrings::DoorLocked, FT_MISC, door, actor);
			}
			door->AddTrigger(TriggerEntry(trigger_failedtoopen, actor->GetGlobalID()));

			//playsound unsuccessful opening of door
			core->GetAudioPlayback().PlayDefaultSound(
				door->IsOpen() ? DS_CLOSE_FAIL : DS_OPEN_FAIL,
				core->GetAudioSettings().ConfigPresetByChannel(SFXChannel::Actions, *p));
			Sender->ReleaseCurrentAction();
			return; //don't open door
		}

		//trap scripts are triggered by SetDoorOpen
		door->SetDoorOpen(!door->IsOpen(), true, actor->GetGlobalID());
	} else {
		MoveNearerTo(Sender, *p, MAX_OPERATING_DISTANCE, 0);
		return;
	}
	Sender->SetWait(1);
	Sender->ReleaseCurrentAction();
}

void GameScript::ContainerEnable(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetScriptableFromObject(Sender, parameters);
	Container* cnt = Scriptable::As<Container>(tar);
	if (!cnt) {
		return;
	}

	if (parameters->int0Parameter) {
		cnt->Flags &= ~CONT_DISABLED;
	} else {
		cnt->Flags |= CONT_DISABLED;
	}
}

void GameScript::MoveBetweenAreas(Scriptable* Sender, Action* parameters)
{
	Actor* actor = Scriptable::As<Actor>(Sender);
	if (!actor) {
		return;
	}
	if (!parameters->resref1Parameter.IsEmpty()) {
		CreateVisualEffectCore(Sender, Sender->Pos, parameters->resref1Parameter, 0);
	}

	if (actor->Persistent() || !CreateMovementEffect(actor, parameters->resref0Parameter, parameters->pointParameter, parameters->int0Parameter)) {
		MoveBetweenAreasCore(actor, parameters->resref0Parameter, parameters->pointParameter, parameters->int0Parameter, true);
	}
}

//spell is depleted, casting time is calculated, interruptible
void GameScript::Spell(Scriptable* Sender, Action* parameters)
{
	SpellCore(Sender, parameters, SC_NO_DEAD | SC_RANGE_CHECK | SC_DEPLETE | SC_AURA_CHECK);
}

//spell is depleted, casting time is calculated, interruptible
void GameScript::SpellPoint(Scriptable* Sender, Action* parameters)
{
	SpellPointCore(Sender, parameters, SC_RANGE_CHECK | SC_DEPLETE | SC_AURA_CHECK);
}

//spell is not depleted (doesn't need to be memorised or known)
//casting time is calculated, interruptible
void GameScript::SpellNoDec(Scriptable* Sender, Action* parameters)
{
	SpellCore(Sender, parameters, SC_NO_DEAD | SC_RANGE_CHECK | SC_AURA_CHECK);
}

//spell is not depleted (doesn't need to be memorised or known)
//casting time is calculated, interruptible
void GameScript::SpellPointNoDec(Scriptable* Sender, Action* parameters)
{
	SpellPointCore(Sender, parameters, SC_RANGE_CHECK | SC_AURA_CHECK);
}

// this one has many signatures:
// ForceSpell(O:Target*,I:Spell*Spell)
// ForceSpellRES(S:RES*,O:Target*)
// ForceSpellRES(S:RES*,O:Target*,I:CastingLevel*)
// ForceSpellRESNoFeedback(S:RES*,O:Target*) - no logging or overhead text; add another SC_ flag once understood
//spell is not depleted (doesn't need to be memorised or known)
// casting time is calculated, not interruptible
void GameScript::ForceSpell(Scriptable* Sender, Action* parameters)
{
	SpellCore(Sender, parameters, SC_NOINTERRUPT | SC_SETLEVEL);
}

void GameScript::ForceSpellRange(Scriptable* Sender, Action* parameters)
{
	SpellCore(Sender, parameters, SC_NOINTERRUPT | SC_RANGE_CHECK);
}

//spell is not depleted (doesn't need to be memorised or known)
// casting time is calculated, not interruptible
void GameScript::ForceSpellPoint(Scriptable* Sender, Action* parameters)
{
	SpellPointCore(Sender, parameters, SC_NOINTERRUPT | SC_SETLEVEL);
}

void GameScript::ForceSpellPointRange(Scriptable* Sender, Action* parameters)
{
	SpellPointCore(Sender, parameters, SC_NOINTERRUPT | SC_RANGE_CHECK);
}

//ForceSpell with zero casting time
// zero casting time, no depletion, not interruptible
void GameScript::ReallyForceSpell(Scriptable* Sender, Action* parameters)
{
	SpellCore(Sender, parameters, SC_NOINTERRUPT | SC_SETLEVEL | SC_INSTANT);
}

//ForceSpellPoint with zero casting time
// zero casting time, no depletion (finish casting at point), not interruptible
//no CFB
void GameScript::ReallyForceSpellPoint(Scriptable* Sender, Action* parameters)
{
	SpellPointCore(Sender, parameters, SC_NOINTERRUPT | SC_SETLEVEL | SC_INSTANT);
}

// this differs from ReallyForceSpell that this one allows dead Sender casting
// zero casting time, no depletion
void GameScript::ReallyForceSpellDead(Scriptable* Sender, Action* parameters)
{
	// the difference from ReallyForceSpell is handled by the lack of AF_ALIVE being set
	SpellCore(Sender, parameters, SC_NOINTERRUPT | SC_SETLEVEL | SC_INSTANT);
}

void GameScript::Activate(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetScriptableFromObject(Sender, parameters);
	if (!tar) {
		//it could still be an area animation, PST allows deactivating them via Activate
		AmbientActivateCore(Sender, parameters, true);
		return;
	}
	if (tar->Type == ST_ACTOR) {
		tar->Unhide();
		return;
	}

	//PST allows activating of containers
	if (tar->Type == ST_CONTAINER) {
		static_cast<Container*>(tar)->Flags &= ~CONT_DISABLED;
		return;
	}

	//and regions
	if (tar->Type == ST_PROXIMITY || tar->Type == ST_TRAVEL || tar->Type == ST_TRIGGER) {
		static_cast<InfoPoint*>(tar)->Flags &= ~TRAP_DEACTIVATED;
		return;
	}
}

void GameScript::Deactivate(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetScriptableFromObject(Sender, parameters);
	if (!tar) {
		//it could still be an area animation, PST allows deactivating them via Deactivate
		AmbientActivateCore(Sender, parameters, false);
		return;
	}
	if (tar->Type == ST_ACTOR) {
		tar->Hide();
		return;
	}
	//PST allows deactivating of containers
	//but IWD doesn't, ar9705 chests rely on it (if this is changed, make sure they are all still selectable!)
	//FIXME: add a new game flag / differentiate more container flags
	if (tar->Type == ST_CONTAINER && !core->HasFeature(GFFlags::SPECIFIC_DMG_BONUS)) {
		static_cast<Container*>(tar)->Flags |= CONT_DISABLED;
		return;
	}

	//and regions
	if (tar->Type == ST_PROXIMITY || tar->Type == ST_TRAVEL || tar->Type == ST_TRIGGER) {
		static_cast<InfoPoint*>(tar)->Flags |= TRAP_DEACTIVATED;
		return;
	}
}

void GameScript::MakeGlobal(Scriptable* Sender, Action* /*parameters*/)
{
	Actor* act = Scriptable::As<Actor>(Sender);
	if (!act) {
		return;
	}
	core->GetGame()->AddNPC(act);
}

// will replace a global creature that has the same scriptname as the sender
void GameScript::MakeGlobalOverride(Scriptable* Sender, Action* /*parameters*/)
{
	Actor* act = Scriptable::As<Actor>(Sender);
	if (!act) {
		return;
	}

	Game* game = core->GetGame();
	bool alreadyGlobal = game->InStore(act) != -1;
	if (alreadyGlobal) return;

	// remove any previous global actor with the same script name first
	Actor* target = game->FindNPC(act->GetScriptName());
	if (target) {
		int slot = game->InStore(target);
		game->DelNPC(slot);
		target->SetPersistent(-1);
	}
	game->AddNPC(act);
}

void GameScript::UnMakeGlobal(Scriptable* Sender, Action* /*parameters*/)
{
	Actor* act = Scriptable::As<Actor>(Sender);
	if (!act) {
		return;
	}
	int slot;
	slot = core->GetGame()->InStore(act);
	if (slot >= 0) {
		core->GetGame()->DelNPC(slot);
		act->SetPersistent(-1);
	}
}

//this apparently doesn't check the gold, thus could be used from non actors
void GameScript::GivePartyGoldGlobal(Scriptable* Sender, Action* parameters)
{
	ieDword gold = CheckVariable(Sender, parameters->string0Parameter, parameters->string1Parameter);
	Actor* act = Scriptable::As<Actor>(Sender);
	if (act) {
		ieDword mygold = act->GetStat(IE_GOLD);
		if (mygold < gold) {
			gold = mygold;
		}
		//will get saved, not adjusted
		act->SetBase(IE_GOLD, act->GetBase(IE_GOLD) - gold);
	}
	core->GetGame()->AddGold(gold);
}

void GameScript::CreatePartyGold(Scriptable* /*Sender*/, Action* parameters)
{
	core->GetGame()->AddGold(parameters->int0Parameter);
}

void GameScript::GivePartyGold(Scriptable* Sender, Action* parameters)
{
	ieDword gold = (ieDword) parameters->int0Parameter;
	Actor* act = Scriptable::As<Actor>(Sender);
	if (act) {
		ieDword mygold = act->GetStat(IE_GOLD);
		if (mygold < gold) {
			gold = mygold;
		}
		//will get saved, not adjusted
		act->SetBase(IE_GOLD, act->GetBase(IE_GOLD) - gold);
	}
	core->GetGame()->AddGold(gold);
}

void GameScript::DestroyPartyGold(Scriptable* /*Sender*/, Action* parameters)
{
	int gold = core->GetGame()->PartyGold;
	if (gold > parameters->int0Parameter) {
		gold = parameters->int0Parameter;
	}
	core->GetGame()->AddGold(-gold);
}

void GameScript::TakePartyGold(Scriptable* Sender, Action* parameters)
{
	ieDword gold = core->GetGame()->PartyGold;
	if (gold > (ieDword) parameters->int0Parameter) {
		gold = (ieDword) parameters->int0Parameter;
	}
	core->GetGame()->AddGold((ieDword) - (int) gold);
	Actor* act = Scriptable::As<Actor>(Sender);
	// fixes PST limlim shop, partymembers don't receive the taken gold
	if (act && !act->InParty) {
		act->SetBase(IE_GOLD, act->GetBase(IE_GOLD) + gold);
	}
}

void GameScript::TakeObjectGoldGlobal(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetScriptableFromObject(Sender, parameters);
	Actor* actor = Scriptable::As<Actor>(tar);
	if (!actor) return;

	Actor::stat_t gold = actor->GetBase(IE_GOLD);
	actor->SetBase(IE_GOLD, 0);
	SetVariable(Sender, parameters->variable0Parameter, gold, parameters->resref1Parameter);
}

void GameScript::GiveObjectGoldGlobal(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetScriptableFromObject(Sender, parameters);
	Actor* actor = Scriptable::As<Actor>(tar);
	if (!actor) return;

	ieDword gold = CheckVariable(Sender, parameters->string0Parameter, parameters->resref1Parameter);
	actor->SetBase(IE_GOLD, actor->GetBase(IE_GOLD) + gold);
	// no need to nullify the var, it was done manually
}

void GameScript::AddXPObject(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetScriptableFromObject(Sender, parameters);
	Actor* actor = Scriptable::As<Actor>(tar);
	if (!actor) {
		return;
	}
	int xp = parameters->int0Parameter;
	SetTokenAsString("EXPERIENCEAMOUNT", xp);
	if (core->HasFeedback(FT_MISC)) {
		if (DisplayMessage::HasStringReference(HCStrings::GotQuestXP)) {
			displaymsg->DisplayConstantStringName(HCStrings::GotQuestXP, GUIColors::XPCHANGE, actor);
		} else {
			displaymsg->DisplayConstantStringValue(HCStrings::GotXP, GUIColors::XPCHANGE, (ieDword) xp);
		}
	}

	//normally the second parameter is 0, but it may be handy to have control over that (See SX_* flags)
	actor->AddExperience(xp, parameters->int1Parameter);
	core->GetAudioPlayback().PlayDefaultSound(DS_GOTXP, SFXChannel::Actions);
}

void GameScript::AddXP2DA(Scriptable* /*Sender*/, Action* parameters)
{
	AddXPCore(parameters);
}

void GameScript::AddXPVar(Scriptable* /*Sender*/, Action* parameters)
{
	AddXPCore(parameters, true);
}

void GameScript::AddXPWorth(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetScriptableFromObject(Sender, parameters);
	Actor* actor = Scriptable::As<Actor>(tar);
	if (!actor) return;

	int xp = actor->GetStat(IE_XPVALUE); // I guess
	if (parameters->int0Parameter) actor->SetBase(IE_XPVALUE, 0);
	core->GetGame()->ShareXP(xp, SX_DIVIDE);
	core->GetAudioPlayback().PlayDefaultSound(DS_GOTXP, SFXChannel::Actions);
}

void GameScript::AddExperienceParty(Scriptable* /*Sender*/, Action* parameters)
{
	core->GetGame()->ShareXP(parameters->int0Parameter, SX_DIVIDE);
	core->GetAudioPlayback().PlayDefaultSound(DS_GOTXP, SFXChannel::Actions);
}

//this needs moncrate.2da, but otherwise independent from GFFlags::CHALLENGERATING
void GameScript::AddExperiencePartyCR(Scriptable* /*Sender*/, Action* parameters)
{
	core->GetGame()->ShareXP(parameters->int0Parameter, SX_DIVIDE | SX_CR);
}

void GameScript::AddExperiencePartyGlobal(Scriptable* Sender, Action* parameters)
{
	ieDword xp = CheckVariable(Sender, parameters->string0Parameter, parameters->string1Parameter);
	core->GetGame()->ShareXP(xp, SX_DIVIDE);
	core->GetAudioPlayback().PlayDefaultSound(DS_GOTXP, SFXChannel::Actions);
}

// these two didn't work in the original (bg2, ee) and were unused
void GameScript::SetMoraleAI(Scriptable* Sender, Action* parameters)
{
	Actor* act = Scriptable::As<Actor>(Sender);
	if (!act) {
		return;
	}
	act->SetBase(IE_MORALE, parameters->int0Parameter);
}

void GameScript::IncMoraleAI(Scriptable* Sender, Action* parameters)
{
	Actor* act = Scriptable::As<Actor>(Sender);
	if (!act) {
		return;
	}
	act->SetBase(IE_MORALE, parameters->int0Parameter + act->GetBase(IE_MORALE));
}

// these three are present in all engines
void GameScript::MoraleSet(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetScriptableFromObject(Sender, parameters);
	Actor* act = Scriptable::As<Actor>(tar);
	if (!act) {
		return;
	}

	ieDword oldMorale = act->GetBase(IE_MORALE);
	act->SetBase(IE_MORALE, parameters->int0Parameter);
	if (core->HasFeature(GFFlags::PST_STATE_FLAGS)) {
		ScriptEngine::FunctionParameters params {
			ScriptEngine::Parameter(act->GetGlobalID()),
			ScriptEngine::Parameter(act->GetBase(IE_MORALE) - oldMorale),
			ScriptEngine::Parameter(0)
		};
		core->GetGUIScriptEngine()->RunFunction("Game", "CheckKarachUpgrade", params);
	}
}

void GameScript::MoraleInc(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetScriptableFromObject(Sender, parameters);
	Actor* act = Scriptable::As<Actor>(tar);
	if (!act) {
		return;
	}
	act->SetBase(IE_MORALE, act->GetBase(IE_MORALE) + parameters->int0Parameter);
	if (core->HasFeature(GFFlags::PST_STATE_FLAGS)) {
		ScriptEngine::FunctionParameters params {
			ScriptEngine::Parameter(act->GetGlobalID()),
			ScriptEngine::Parameter(parameters->int0Parameter),
			ScriptEngine::Parameter(0)
		};
		core->GetGUIScriptEngine()->RunFunction("Game", "CheckKarachUpgrade", params);
	}
}

void GameScript::MoraleDec(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetScriptableFromObject(Sender, parameters);
	Actor* act = Scriptable::As<Actor>(tar);
	if (!act) {
		return;
	}
	act->SetBase(IE_MORALE, act->GetBase(IE_MORALE) - parameters->int0Parameter);
	if (core->HasFeature(GFFlags::PST_STATE_FLAGS)) {
		ScriptEngine::FunctionParameters params {
			ScriptEngine::Parameter(act->GetGlobalID()),
			ScriptEngine::Parameter(-1 * parameters->int0Parameter),
			ScriptEngine::Parameter(0)
		};
		core->GetGUIScriptEngine()->RunFunction("Game", "CheckKarachUpgrade", params);
	}
}

// ee oddity
void GameScript::ResetMorale(Scriptable* Sender, Action* parameters)
{
	Actor* act = Scriptable::As<Actor>(Sender);
	if (!act) return;

	act->SetBase(IE_MORALEBREAK, 1);
	if (parameters->int0Parameter) {
		act->SetBase(IE_MORALE, 0);
	} else {
		act->SetBase(IE_MORALE, 10);
	}
}

void GameScript::JoinParty(Scriptable* Sender, Action* parameters)
{
	Actor* act = Scriptable::As<Actor>(Sender);
	if (!act) {
		return;
	}
	// make sure we're in the same area, otherwise Dynaheir joins when Minsc does
	// but she's in another area and needs to be rescued first!
	Game* game = core->GetGame();
	if (act->GetCurrentArea() != game->GetCurrentArea()) {
		return;
	}

	/* calling this, so it is simpler to change */
	/* i'm not sure this is required here at all */
	SetBeenInPartyFlags(Sender, parameters);
	act->SetBase(IE_EA, EA_PC);
	if (core->HasFeature(GFFlags::HAS_DPLAYER)) {
		/* we must reset various existing scripts */
		act->SetScript("DEFAULT", AI_SCRIPT_LEVEL, true);
		act->SetScript(ResRef(), SCR_RACE, true);
		act->SetScript(ResRef(), SCR_GENERAL, true);
		act->SetScript("DPLAYER2", SCR_DEFAULT, false);
	}
	AutoTable pdtable = gamedata->LoadTable("pdialog");
	if (pdtable) {
		const ieVariable& scriptname = act->GetScriptName();
		ResRef resRef;
		//set dialog only if we got a row
		if (pdtable->GetRowIndex(scriptname) != TableMgr::npos) {
			if (game->Expansion == GAME_TOB) {
				resRef = pdtable->QueryField(scriptname, "25JOIN_DIALOG_FILE");
			} else {
				resRef = pdtable->QueryField(scriptname, "JOIN_DIALOG_FILE");
			}
			act->SetDialog(resRef);
		}
	}
	int flags = JP_JOIN;
	if (parameters->actionID > 19) { // ugly way to distinguish JoinPartyOverride
		flags |= JP_OVERRIDE;
	}
	game->JoinParty(act, flags);
}

void GameScript::LeaveParty(Scriptable* Sender, Action* /*parameters*/)
{
	Actor* act = Scriptable::As<Actor>(Sender);
	if (!act) {
		return;
	}
	core->GetGame()->LeaveParty(act);
}

//HideCreature hides only the visuals of a creature
//(feet circle and avatar)
//the scripts of the creature are still running
// iwd2 stores this flag in the MC field (MC_ENABLED)
void GameScript::HideCreature(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetScriptableFromObject(Sender, parameters);
	Actor* actor = Scriptable::As<Actor>(tar);
	if (!actor) {
		return;
	}

	actor->SetBase(IE_AVATARREMOVAL, parameters->int0Parameter);
	const Map* map = actor->GetCurrentArea();
	if (!map || !actor->BlocksSearchMap()) return;

	if (parameters->int0Parameter == 0) {
		// move feet lice away — it can happen we "spawned" on others
		// Map::BlockSearchMapFor() is not enough
		int flags = GA_NO_DEAD | GA_NO_LOS | GA_NO_UNSCHEDULED | GA_NO_SELF;
		const auto& neighbours = map->GetAllActorsInRadius(actor->Pos, flags, actor->circleSize, actor);
		for (auto& neighbour : neighbours) {
			neighbour->SetPosition(neighbour->Pos, true);
		}
	} else {
		map->ClearSearchMapFor(actor);
	}
}

//i have absolutely no idea why this is needed when we have HideCreature
void GameScript::ForceHide(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetScriptableFromObject(Sender, parameters);
	if (!tar) {
		tar = Sender;
	}
	Actor* actor = Scriptable::As<Actor>(tar);
	if (!actor) {
		return;
	}
	actor->SetBase(IE_AVATARREMOVAL, 1);
}

void GameScript::ForceLeaveAreaLUA(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetScriptableFromObject(Sender, parameters);
	Actor* actor = Scriptable::As<Actor>(tar);
	if (!actor) {
		return;
	}

	//the LoadMos ResRef may be empty
	if (!parameters->resref1Parameter.IsEmpty()) {
		core->GetGame()->LoadMos = parameters->resref1Parameter;
	}
	if (actor->Persistent() || !CreateMovementEffect(actor, parameters->resref0Parameter, parameters->pointParameter, parameters->int0Parameter)) {
		MoveBetweenAreasCore(actor, parameters->resref0Parameter, parameters->pointParameter, parameters->int0Parameter, true);

		const Actor* protagonist = core->GetGame()->GetPC(0, false);
		if (actor == protagonist) {
			core->GetGame()->MoveFamiliars(parameters->resref0Parameter, parameters->pointParameter, parameters->int0Parameter);
		}
	}
}

void GameScript::LeaveAreaLUA(Scriptable* Sender, Action* parameters)
{
	Actor* actor = Scriptable::As<Actor>(Sender);
	if (!actor) {
		return;
	}
	//the LoadMos ResRef may be empty
	if (!parameters->resref1Parameter.IsEmpty()) {
		core->GetGame()->LoadMos = parameters->resref1Parameter;
	}
	if (actor->Persistent() || !CreateMovementEffect(actor, parameters->resref0Parameter, parameters->pointParameter, parameters->int0Parameter)) {
		MoveBetweenAreasCore(actor, parameters->resref0Parameter, parameters->pointParameter, parameters->int0Parameter, true);

		const Actor* protagonist = core->GetGame()->GetPC(0, false);
		if (actor == protagonist) {
			core->GetGame()->MoveFamiliars(parameters->resref0Parameter, parameters->pointParameter, parameters->int0Parameter);
		}
	}
}

//this is a blocking action, because we have to move to the Entry
void GameScript::LeaveAreaLUAEntry(Scriptable* Sender, Action* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		Sender->ReleaseCurrentAction();
		return;
	}
	Game* game = core->GetGame();
	if (!parameters->resref1Parameter.IsEmpty()) {
		game->LoadMos = parameters->resref1Parameter;
	}
	Point p = GetEntryPoint(parameters->resref0Parameter, parameters->resref1Parameter);
	if (p.IsInvalid()) {
		Sender->ReleaseCurrentAction();
		return;
	}
	parameters->pointParameter = p;
	parameters->string1Parameter.Reset();
	LeaveAreaLUA(Sender, parameters);
	Sender->ReleaseCurrentAction();
}

//at this time it is unclear what the LeaveAreaLUAPanic* commands are used for
//since they are always followed by the non-panic version of the command in all
//games that use them (bg1 + bg2) we simply make them de-facto no-ops for now
// Taimon: in ToB these are used in multiplayer and do something different for
// the host than the external players
void GameScript::LeaveAreaLUAPanic(Scriptable* Sender, Action* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		return;
	}
	if (!parameters->resref1Parameter.IsEmpty()) {
		core->GetGame()->LoadMos = parameters->resref1Parameter;
	}
}

void GameScript::LeaveAreaLUAPanicEntry(Scriptable* Sender, Action* parameters)
{
	LeaveAreaLUAPanic(Sender, parameters);
}

void GameScript::SetToken(Scriptable* /*Sender*/, Action* parameters)
{
	//SetAt takes a newly created reference (no need of free/copy)
	String str = core->GetString(ieStrRef(parameters->int0Parameter));
	core->GetTokenDictionary()[parameters->variable0Parameter] = std::move(str);
}

//Assigns a numeric variable to the token
void GameScript::SetTokenGlobal(Scriptable* Sender, Action* parameters)
{
	ieDword value = CheckVariable(Sender, parameters->string0Parameter);
	SetTokenAsString(parameters->variable1Parameter, value);
}

//Assigns the target object's name (not scriptname) to the token
void GameScript::SetTokenObject(Scriptable* Sender, Action* parameters)
{
	const Scriptable* tar = GetScriptableFromObject(Sender, parameters);
	const Actor* actor = Scriptable::As<const Actor>(tar);
	if (!actor) {
		return;
	}
	core->GetTokenDictionary()[parameters->variable0Parameter] =
		actor->GetShortName();
}

void GameScript::PlayDead(Scriptable* Sender, Action* parameters)
{
	Actor* actor = Scriptable::As<Actor>(Sender);
	if (!actor) {
		Sender->ReleaseCurrentAction();
		return;
	}

	actor->CurrentActionInterruptible = false;
	if (!Sender->CurrentActionTicks && parameters->int0Parameter) {
		// set countdown on first run
		Sender->CurrentActionState = parameters->int0Parameter;
		actor->SetStance(IE_ANI_DIE);
	}
	if (Sender->CurrentActionState <= 0) {
		actor->SetStance(IE_ANI_GET_UP);
		Sender->ReleaseCurrentAction();
		return;
	}
	actor->CurrentActionState--;
}

void GameScript::PlayDeadInterruptible(Scriptable* Sender, Action* parameters)
{
	Actor* actor = Scriptable::As<Actor>(Sender);
	if (!actor) {
		Sender->ReleaseCurrentAction();
		return;
	}

	if (!Sender->CurrentActionTicks && parameters->int0Parameter) {
		// set countdown on first run
		Sender->CurrentActionState = parameters->int0Parameter;
		actor->SetStance(IE_ANI_DIE);
	}
	if (Sender->CurrentActionState <= 0) {
		actor->SetStance(IE_ANI_GET_UP);
		Sender->ReleaseCurrentAction();
		return;
	}
	actor->CurrentActionState--;
}

/* this is not correct, see #92 */
void GameScript::Swing(Scriptable* Sender, Action* /*parameters*/)
{
	Actor* actor = Scriptable::As<Actor>(Sender);
	if (!actor) {
		return;
	}
	actor->SetStance(IE_ANI_ATTACK);
	actor->SetWait(core->Time.defaultTicksPerSec * 2);
}

/* this is not correct, see #92 */
void GameScript::SwingOnce(Scriptable* Sender, Action* /*parameters*/)
{
	Actor* actor = Scriptable::As<Actor>(Sender);
	if (!actor) {
		return;
	}
	actor->SetStance(IE_ANI_ATTACK);
	actor->SetWait(core->Time.defaultTicksPerSec);
}

void GameScript::Recoil(Scriptable* Sender, Action* /*parameters*/)
{
	Actor* actor = Scriptable::As<Actor>(Sender);
	if (!actor) {
		return;
	}
	actor->SetStance(IE_ANI_DAMAGE);
	actor->SetWait(1);
}

void GameScript::AnkhegEmerge(Scriptable* Sender, Action* /*parameters*/)
{
	Actor* actor = Scriptable::As<Actor>(Sender);
	if (!actor) {
		return;
	}
	if (actor->GetStance() != IE_ANI_EMERGE) {
		actor->SetStance(IE_ANI_EMERGE);
		actor->SetWait(1);
	}
}

void GameScript::AnkhegHide(Scriptable* Sender, Action* /*parameters*/)
{
	Actor* actor = Scriptable::As<Actor>(Sender);
	if (!actor) {
		return;
	}
	if (actor->GetStance() != IE_ANI_HIDE) {
		actor->SetStance(IE_ANI_HIDE);
		actor->SetWait(1);
	}
}

void GameScript::GlobalSetGlobal(Scriptable* Sender, Action* parameters)
{
	ieDword value = CheckVariable(Sender, parameters->string0Parameter);
	SetVariable(Sender, parameters->string1Parameter, value);
}

/* adding the second variable to the first, they must be GLOBAL */
void GameScript::AddGlobals(Scriptable* Sender, Action* parameters)
{
	ieDword value1 = CheckVariable(Sender, parameters->string0Parameter, "GLOBAL");
	ieDword value2 = CheckVariable(Sender, parameters->string1Parameter, "GLOBAL");
	SetVariable(Sender, parameters->string0Parameter, value1 + value2, "GLOBAL");
}

/* adding the second variable to the first, they could be area or locals */
void GameScript::GlobalAddGlobal(Scriptable* Sender, Action* parameters)
{
	ieDword value1 = CheckVariable(Sender,
				       parameters->string0Parameter);
	ieDword value2 = CheckVariable(Sender,
				       parameters->string1Parameter);
	SetVariable(Sender, parameters->string0Parameter, value1 + value2);
}

/* adding the number to the global, they could be area or locals */
void GameScript::IncrementGlobal(Scriptable* Sender, Action* parameters)
{
	ieDword value = CheckVariable(Sender, parameters->string0Parameter);
	SetVariable(Sender, parameters->string0Parameter,
		    value + parameters->int0Parameter);
}

// adding the number to the global ONLY if the first global is zero
// only user: 0901tria.baf:    IncrementGlobalOnce("Evil_Trias_2","GLOBAL","Good","GLOBAL",-1)
void GameScript::IncrementGlobalOnce(Scriptable* Sender, Action* parameters)
{
	ieDword value = CheckVariable(Sender, parameters->string0Parameter);
	if (value != 0) {
		return;
	}
	SetVariable(Sender, parameters->string0Parameter, 1);

	value = CheckVariable(Sender, parameters->string1Parameter);
	SetVariable(Sender, parameters->string1Parameter, ieDword(int(value) + parameters->int0Parameter));
}

void GameScript::GlobalSubGlobal(Scriptable* Sender, Action* parameters)
{
	ieDword value1 = CheckVariable(Sender,
				       parameters->string0Parameter);
	ieDword value2 = CheckVariable(Sender,
				       parameters->string1Parameter);
	SetVariable(Sender, parameters->string0Parameter, value1 - value2);
}

void GameScript::GlobalAndGlobal(Scriptable* Sender, Action* parameters)
{
	ieDword value1 = CheckVariable(Sender,
				       parameters->string0Parameter);
	ieDword value2 = CheckVariable(Sender,
				       parameters->string1Parameter);
	SetVariable(Sender, parameters->string0Parameter, value1 && value2);
}

void GameScript::GlobalOrGlobal(Scriptable* Sender, Action* parameters)
{
	ieDword value1 = CheckVariable(Sender,
				       parameters->string0Parameter);
	ieDword value2 = CheckVariable(Sender,
				       parameters->string1Parameter);
	SetVariable(Sender, parameters->string0Parameter, value1 || value2);
}

void GameScript::GlobalBOrGlobal(Scriptable* Sender, Action* parameters)
{
	ieDword value1 = CheckVariable(Sender,
				       parameters->string0Parameter);
	ieDword value2 = CheckVariable(Sender,
				       parameters->string1Parameter);
	SetVariable(Sender, parameters->string0Parameter, value1 | value2);
}

void GameScript::GlobalBAndGlobal(Scriptable* Sender, Action* parameters)
{
	ieDword value1 = CheckVariable(Sender,
				       parameters->string0Parameter);
	ieDword value2 = CheckVariable(Sender,
				       parameters->string1Parameter);
	SetVariable(Sender, parameters->string0Parameter, value1 & value2);
}

void GameScript::GlobalXorGlobal(Scriptable* Sender, Action* parameters)
{
	ieDword value1 = CheckVariable(Sender,
				       parameters->string0Parameter);
	ieDword value2 = CheckVariable(Sender,
				       parameters->string1Parameter);
	SetVariable(Sender, parameters->string0Parameter, value1 ^ value2);
}

void GameScript::GlobalBOr(Scriptable* Sender, Action* parameters)
{
	ieDword value1 = CheckVariable(Sender,
				       parameters->string0Parameter);
	SetVariable(Sender, parameters->string0Parameter,
		    value1 | parameters->int0Parameter);
}

void GameScript::GlobalBAnd(Scriptable* Sender, Action* parameters)
{
	ieDword value1 = CheckVariable(Sender,
				       parameters->string0Parameter);
	SetVariable(Sender, parameters->string0Parameter,
		    value1 & parameters->int0Parameter);
}

void GameScript::GlobalXor(Scriptable* Sender, Action* parameters)
{
	ieDword value1 = CheckVariable(Sender,
				       parameters->string0Parameter);
	SetVariable(Sender, parameters->string0Parameter,
		    value1 ^ parameters->int0Parameter);
}

void GameScript::GlobalMax(Scriptable* Sender, Action* parameters)
{
	int value1 = CheckVariable(Sender, parameters->string0Parameter);
	if (value1 > parameters->int0Parameter) {
		SetVariable(Sender, parameters->string0Parameter, value1);
	}
}

void GameScript::GlobalMin(Scriptable* Sender, Action* parameters)
{
	int value1 = CheckVariable(Sender, parameters->string0Parameter);
	if (value1 < parameters->int0Parameter) {
		SetVariable(Sender, parameters->string0Parameter, value1);
	}
}

void GameScript::BitClear(Scriptable* Sender, Action* parameters)
{
	ieDword value1 = CheckVariable(Sender,
				       parameters->string0Parameter);
	SetVariable(Sender, parameters->string0Parameter,
		    value1 & ~parameters->int0Parameter);
}

void GameScript::GlobalShL(Scriptable* Sender, Action* parameters)
{
	ieDword value1 = CheckVariable(Sender,
				       parameters->string0Parameter);
	ieDword value2 = parameters->int0Parameter;
	if (value2 > 31) {
		value1 = 0;
	} else {
		value1 <<= value2;
	}
	SetVariable(Sender, parameters->string0Parameter, value1);
}

void GameScript::GlobalShR(Scriptable* Sender, Action* parameters)
{
	ieDword value1 = CheckVariable(Sender,
				       parameters->string0Parameter);
	ieDword value2 = parameters->int0Parameter;
	if (value2 > 31) {
		value1 = 0;
	} else {
		value1 >>= value2;
	}
	SetVariable(Sender, parameters->string0Parameter, value1);
}

void GameScript::GlobalMaxGlobal(Scriptable* Sender, Action* parameters)
{
	ieDword value1 = CheckVariable(Sender, parameters->string0Parameter);
	ieDword value2 = CheckVariable(Sender, parameters->string1Parameter);
	if (value1 < value2) {
		SetVariable(Sender, parameters->string0Parameter, value2);
	}
}

void GameScript::GlobalMinGlobal(Scriptable* Sender, Action* parameters)
{
	ieDword value1 = CheckVariable(Sender, parameters->string0Parameter);
	ieDword value2 = CheckVariable(Sender, parameters->string1Parameter);
	if (value1 > value2) {
		SetVariable(Sender, parameters->string0Parameter, value2);
	}
}

void GameScript::GlobalShLGlobal(Scriptable* Sender, Action* parameters)
{
	ieDword value1 = CheckVariable(Sender, parameters->string0Parameter);
	ieDword value2 = CheckVariable(Sender, parameters->string1Parameter);
	if (value2 > 31) {
		value1 = 0;
	} else {
		value1 <<= value2;
	}
	SetVariable(Sender, parameters->string0Parameter, value1);
}
void GameScript::GlobalShRGlobal(Scriptable* Sender, Action* parameters)
{
	ieDword value1 = CheckVariable(Sender, parameters->string0Parameter);
	ieDword value2 = CheckVariable(Sender, parameters->string1Parameter);
	if (value2 > 31) {
		value1 = 0;
	} else {
		value1 >>= value2;
	}
	SetVariable(Sender, parameters->string0Parameter, value1);
}

void GameScript::ClearAllActions(Scriptable* Sender, Action* /*parameters*/)
{
	const Map* map = Sender->GetCurrentArea();
	int i = map->GetActorCount(true);
	while (i--) {
		Actor* act = map->GetActor(i, true);
		if (act && act != Sender && act->ValidTarget(GA_NO_DEAD)) {
			act->Stop(3);
			act->SetModal(Modal::None);
		}
	}

	// bg2 also sometimes cleared baldur.bcs, while area scripts were left untouched
	const Map* area = Sender->GetCurrentArea();
	if (!area) return;
	if (Sender->Type != ST_GLOBAL && area->MasterArea) {
		// clear game script if area is a master area
		// clear game script if Sender is in master area
		core->GetGame()->Stop();
	}
}

// clear the queue, leaving the current action intact if it is non-interruptible
void GameScript::ClearActions(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = Sender;
	if (parameters->objects[1]) {
		tar = GetScriptableFromObject(Sender, parameters);
		if (!tar) {
			Log(WARNING, "GameScript", "Couldn't find target for ClearActions!");
			parameters->objects[1]->dump();
			return;
		}
	}

	tar->Stop(3);

	if (tar->Type == ST_ACTOR) {
		Actor* actor = (Actor*) tar;
		actor->SetModal(Modal::None);
	}
}

void GameScript::SetNumTimesTalkedTo(Scriptable* Sender, Action* parameters)
{
	Actor* actor = Scriptable::As<Actor>(Sender);
	if (!actor) {
		return;
	}
	actor->TalkCount = parameters->int0Parameter;
}

void GameScript::StartMovie(Scriptable* Sender, Action* parameters)
{
	core->PlayMovie(parameters->resref0Parameter);
	Sender->ReleaseCurrentAction(); // should this be blocking?
}

void GameScript::SetLeavePartyDialogFile(Scriptable* Sender, Action* /*parameters*/)
{
	Actor* act = Scriptable::As<Actor>(Sender);
	if (!act) {
		return;
	}
	AutoTable pdtable = gamedata->LoadTable("pdialog");
	if (!pdtable) return;

	const ieVariable& scriptname = act->GetScriptName();
	if (pdtable->GetRowIndex(scriptname) != TableMgr::npos) {
		ResRef resRef;

		if (core->GetGame()->Expansion == GAME_TOB) {
			resRef = pdtable->QueryField(scriptname, "25POST_DIALOG_FILE");
		} else {
			resRef = pdtable->QueryField(scriptname, "POST_DIALOG_FILE");
		}
		act->SetDialog(resRef);
	}
}

void GameScript::TextScreen(Scriptable* /*Sender*/, Action* parameters)
{
	core->SetPause(PauseState::On, PF_QUIET);
	// bg2 sometimes calls IncrementChapter("") right after a TextScreen("sometable"),
	// so we make sure they don't cancel out
	if (!parameters->resref0Parameter.IsEmpty()) {
		core->GetGame()->TextScreen = parameters->resref0Parameter;
	}

	core->SetEventFlag(EF_TEXTSCREEN);
}

void GameScript::IncrementChapter(Scriptable* Sender, Action* parameters)
{
	core->GetGame()->IncrementChapter();
	TextScreen(Sender, parameters);
}

void GameScript::SetCriticalPathObject(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetScriptableFromObject(Sender, parameters);
	Actor* actor = Scriptable::As<Actor>(tar);
	if (!actor) {
		return;
	}

	if (parameters->int0Parameter) {
		actor->SetMCFlag(MC_PLOT_CRITICAL, BitOp::OR);
	} else {
		actor->SetMCFlag(MC_PLOT_CRITICAL, BitOp::NAND);
	}
}

void GameScript::SetBeenInPartyFlags(Scriptable* Sender, Action* /*parameters*/)
{
	Actor* actor = Scriptable::As<Actor>(Sender);
	if (!actor) {
		return;
	}
	//it is bit 15 of the multi-class flags (confirmed)
	actor->SetMCFlag(MC_BEENINPARTY, BitOp::OR);
}

/*iwd2 sets the high MC bits this way*/
void GameScript::SetCreatureAreaFlag(Scriptable* Sender, Action* parameters)
{
	Actor* actor = Scriptable::As<Actor>(Sender);
	if (!actor) {
		return;
	}
	if (parameters->int1Parameter) {
		actor->SetMCFlag(parameters->int0Parameter, BitOp::OR);
	} else {
		actor->SetMCFlag(parameters->int0Parameter, BitOp::NAND);
	}
}

// unused and referencing a non-existing color.ids file
// this is a global change, since it takes no target, but it's not clear which color it affected, if it worked
void GameScript::SetTextColor(Scriptable* /*Sender*/, Action* parameters)
{
	Color c = Color::FromABGR(parameters->int0Parameter);
	gamedata->ModifyColor(GUIColors::FLOAT_TXT_ACTOR, c);
	gamedata->ModifyColor(GUIColors::FLOAT_TXT_INFO, c);
	gamedata->ModifyColor(GUIColors::FLOAT_TXT_OTHER, c);
}

void GameScript::BitGlobal(Scriptable* Sender, Action* parameters)
{
	ieDword value = CheckVariable(Sender, parameters->string0Parameter);
	HandleBitMod(value, parameters->int0Parameter, BitOp(parameters->int1Parameter));
	SetVariable(Sender, parameters->string0Parameter, value);
}

void GameScript::GlobalBitGlobal(Scriptable* Sender, Action* parameters)
{
	ieDword value1 = CheckVariable(Sender, parameters->string0Parameter);
	ieDword value2 = CheckVariable(Sender, parameters->string1Parameter);
	HandleBitMod(value1, value2, BitOp(parameters->int1Parameter));
	SetVariable(Sender, parameters->string0Parameter, value1);
}

void GameScript::SetVisualRange(Scriptable* Sender, Action* parameters)
{
	Actor* actor = Scriptable::As<Actor>(Sender);
	if (!actor) {
		return;
	}
	int range = parameters->int0Parameter;
	// 0 means reset back to normal
	if (range == 0) {
		range = Scriptable::VOODOO_VISUAL_RANGE;
	}

	actor->SetBase(IE_VISUALRANGE, range);
	if (actor->GetStat(IE_EA) < EA_EVILCUTOFF) {
		actor->SetBase(IE_EXPLORE, 1);
	}
	// just in case, ensuring the update happens already this tick
	// (in iwd2 script use it's not blocking, just in dialog)
	Map* map = Sender->GetCurrentArea();
	if (map) map->UpdateFog();
}

void GameScript::MakeUnselectable(Scriptable* Sender, Action* parameters)
{
	Sender->UnselectableTimer = parameters->int0Parameter * core->Time.defaultTicksPerSec;
	// hidden EE mode option, not enabled by shipped action.ids
	Sender->UnselectableType = parameters->int1Parameter;

	//update color
	Actor* actor = Scriptable::As<Actor>(Sender);
	if (!actor) {
		return;
	}
	if (parameters->int0Parameter) {
		// flags may be wrong
		core->GetGame()->SelectActor(actor, false, SELECT_QUIET);
	}

	actor->SetCircleSize();
}

void GameScript::Debug(Scriptable* /*Sender*/, Action* parameters)
{
	SetDebugMode(DebugMode(parameters->int0Parameter));
	Log(WARNING, "GameScript", "DEBUG: {}", parameters->string0Parameter);
}

void GameScript::IncrementProficiency(Scriptable* Sender, Action* parameters)
{
	unsigned int idx = parameters->int0Parameter;
	if (idx > 31) {
		return;
	}
	Scriptable* tar = GetScriptableFromObject(Sender, parameters);
	Actor* target = Scriptable::As<Actor>(tar);
	if (!target) {
		return;
	}
	//start of the proficiency stats
	target->NewBase(IE_PROFICIENCYBASTARDSWORD + idx, parameters->int1Parameter, MOD_ADDITIVE);
}

void GameScript::IncrementExtraProficiency(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetScriptableFromObject(Sender, parameters);
	Actor* target = Scriptable::As<Actor>(tar);
	if (!target) {
		return;
	}
	target->SetBase(IE_FREESLOTS, target->GetBase(IE_FREESLOTS) + parameters->int0Parameter);
}

//the third parameter is a GemRB extension
void GameScript::AddJournalEntry(Scriptable* /*Sender*/, Action* parameters)
{
	core->GetGame()->AddJournalEntry(ieStrRef(parameters->int0Parameter), (JournalSection) parameters->int1Parameter, (ieByte) parameters->int2Parameter);
}

void GameScript::SetQuestDone(Scriptable* /*Sender*/, Action* parameters)
{
	Game* game = core->GetGame();
	game->DeleteJournalEntry(ieStrRef(parameters->int0Parameter));
	game->AddJournalEntry(ieStrRef(parameters->int0Parameter), JournalSection::Solved, (ieByte) parameters->int2Parameter);
}

void GameScript::RemoveJournalEntry(Scriptable* /*Sender*/, Action* parameters)
{
	core->GetGame()->DeleteJournalEntry(ieStrRef(parameters->int0Parameter));
}

void GameScript::SetInternal(Scriptable* Sender, Action* parameters)
{
	unsigned int idx = parameters->int0Parameter;
	if (idx > 15) {
		return;
	}
	Scriptable* tar = GetScriptableFromObject(Sender, parameters);
	Actor* target = Scriptable::As<Actor>(tar);
	if (!target) {
		return;
	}
	//start of the internal stats
	target->SetBase(IE_INTERNAL_0 + idx, parameters->int1Parameter);
}

void GameScript::IncInternal(Scriptable* Sender, Action* parameters)
{
	unsigned int idx = parameters->int0Parameter;
	if (idx > 15) {
		return;
	}
	Scriptable* tar = GetScriptableFromObject(Sender, parameters);
	Actor* target = Scriptable::As<Actor>(tar);
	if (!target) {
		return;
	}

	//start of the internal stats
	target->SetBase(IE_INTERNAL_0 + idx,
			target->GetBase(IE_INTERNAL_0 + idx) + parameters->int1Parameter);
}

void GameScript::DestroyAllEquipment(Scriptable* Sender, Action* /*parameters*/)
{
	Inventory* inv = NULL;

	switch (Sender->Type) {
		case ST_ACTOR:
			inv = &(static_cast<Actor*>(Sender)->inventory);
			break;
		case ST_CONTAINER:
			inv = &(static_cast<Container*>(Sender)->inventory);
			break;
		default:;
	}
	if (inv) {
		inv->DestroyItem("", 0, (ieDword) ~0); //destroy any and all
	}
}

void GameScript::DestroyItem(Scriptable* Sender, Action* parameters)
{
	Inventory* inv = NULL;

	switch (Sender->Type) {
		case ST_ACTOR:
			inv = &(static_cast<Actor*>(Sender)->inventory);
			break;
		case ST_CONTAINER:
			inv = &(static_cast<Container*>(Sender)->inventory);
			break;
		default:;
	}
	if (inv) {
		inv->DestroyItem(parameters->resref0Parameter, 0, 1); //destroy one (even indestructible?)
	}
}

//negative destroygold creates gold
void GameScript::DestroyGold(Scriptable* Sender, Action* parameters)
{
	Actor* act = Scriptable::As<Actor>(Sender);
	if (!act) {
		return;
	}
	int max = (int) act->GetStat(IE_GOLD);
	if (parameters->int0Parameter != 0 && max > parameters->int0Parameter) {
		max = parameters->int0Parameter;
	}
	act->SetBase(IE_GOLD, act->GetBase(IE_GOLD) - max);
}

void GameScript::DestroyPartyItem(Scriptable* /*Sender*/, Action* parameters)
{
	const Game* game = core->GetGame();
	int i = game->GetPartySize(false);
	ieDword count;
	if (parameters->int0Parameter) {
		count = 0;
	} else {
		count = 1;
	}
	while (i--) {
		Inventory* inv = &(game->GetPC(i, false)->inventory);
		int res = inv->DestroyItem(parameters->resref0Parameter, 0, count);
		if ((count == 1) && res) {
			break;
		}
	}
}

/* this is a gemrb extension */
void GameScript::DestroyPartyItemNum(Scriptable* /*Sender*/, Action* parameters)
{
	const Game* game = core->GetGame();
	int i = game->GetPartySize(false);
	ieDword count;
	count = parameters->int0Parameter;
	while (i--) {
		Inventory* inv = &(game->GetPC(i, false)->inventory);
		count -= inv->DestroyItem(parameters->resref0Parameter, 0, count);
		if (!count) {
			break;
		}
	}
}

void GameScript::DestroyAllDestructableEquipment(Scriptable* Sender, Action* /*parameters*/)
{
	Inventory* inv = NULL;

	switch (Sender->Type) {
		case ST_ACTOR:
			inv = &(static_cast<Actor*>(Sender)->inventory);
			break;
		case ST_CONTAINER:
			inv = &(static_cast<Container*>(Sender)->inventory);
			break;
		default:;
	}
	if (inv) {
		inv->DestroyItem("", IE_INV_ITEM_DESTRUCTIBLE, (ieDword) ~0);
	}
}

void GameScript::SetApparentName(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetScriptableFromObject(Sender, parameters);
	Actor* target = Scriptable::As<Actor>(tar);
	if (!target) {
		return;
	}
	target->SetName(ieStrRef(parameters->int0Parameter), 1);
}

void GameScript::SetRegularName(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetScriptableFromObject(Sender, parameters);
	Actor* target = Scriptable::As<Actor>(tar);
	if (!target) {
		return;
	}
	target->SetName(ieStrRef(parameters->int0Parameter), 2);
}

/** this is a gemrb extension */
void GameScript::UnloadArea(Scriptable* /*Sender*/, Action* parameters)
{
	int map = core->GetGame()->FindMap(parameters->resref0Parameter);
	if (map >= 0) {
		core->GetGame()->DelMap(map, parameters->int0Parameter);
	}
}

static EffectRef fx_death_ref = { "Death", -1 };
void GameScript::Kill(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetScriptableFromObject(Sender, parameters);
	Actor* target = Scriptable::As<Actor>(tar);
	if (!target) {
		return;
	}

	Effect* fx = EffectQueue::CreateEffect(fx_death_ref, 0, 0, FX_DURATION_INSTANT_PERMANENT);
	target->fxqueue.AddEffect(fx, false);
}

void GameScript::SetGabber(Scriptable* Sender, Action* parameters)
{
	const Scriptable* tar = GetScriptableFromObject(Sender, parameters);
	if (!tar || tar->Type != ST_ACTOR) {
		return;
	}
	const GameControl* gc = core->GetGameControl();
	if (gc->InDialog()) {
		gc->dialoghandler->SetSpeaker(tar);
	} else {
		Log(WARNING, "GameScript", "Can't set gabber!");
	}
}

void GameScript::ReputationSet(Scriptable* /*Sender*/, Action* parameters)
{
	core->GetGame()->SetReputation(parameters->int0Parameter * 10);
}

void GameScript::ReputationInc(Scriptable* /*Sender*/, Action* parameters)
{
	Game* game = core->GetGame();
	game->SetReputation((int) game->Reputation + parameters->int0Parameter * 10);
}

void GameScript::FullHeal(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetScriptableFromObject(Sender, parameters);
	Actor* scr = Scriptable::As<Actor>(tar);
	if (!scr) {
		return;
	}

	//0 means full healing
	//Heal() might contain curing of some conditions
	//if FullHeal doesn't do that, replace this with a SetBase
	//fullhealex might still be the curing action
	scr->Heal(0);
}

static EffectRef fx_disable_button_ref = { "DisableButton", -1 };
void GameScript::RemovePaladinHood(Scriptable* Sender, Action* /*parameters*/)
{
	Actor* act = Scriptable::As<Actor>(Sender);
	if (!act) {
		return;
	}
	act->ApplyKit(true, Actor::GetClassID(ISPALADIN));
	act->SetMCFlag(MC_FALLEN_PALADIN, BitOp::OR);
	Effect* fx = EffectQueue::CreateEffect(fx_disable_button_ref, 0, ACT_TURN, FX_DURATION_INSTANT_PERMANENT);
	act->fxqueue.AddEffect(fx, false);
	fx = EffectQueue::CreateEffect(fx_disable_button_ref, 0, ACT_CAST, FX_DURATION_INSTANT_PERMANENT);
	act->fxqueue.AddEffect(fx, false);
	if (act->InParty && core->HasFeedback(FT_STATES)) displaymsg->DisplayConstantStringName(HCStrings::PaladinFall, GUIColors::XPCHANGE, act);
}

void GameScript::RemoveRangerHood(Scriptable* Sender, Action* /*parameters*/)
{
	Actor* act = Scriptable::As<Actor>(Sender);
	if (!act) {
		return;
	}
	act->ApplyKit(true, Actor::GetClassID(ISRANGER));
	act->SetMCFlag(MC_FALLEN_RANGER, BitOp::OR);
	Effect* fx = EffectQueue::CreateEffect(fx_disable_button_ref, 0, ACT_STEALTH, FX_DURATION_INSTANT_PERMANENT);
	act->fxqueue.AddEffect(fx, false);
	fx = EffectQueue::CreateEffect(fx_disable_button_ref, 0, ACT_CAST, FX_DURATION_INSTANT_PERMANENT);
	act->fxqueue.AddEffect(fx, false);
	if (act->InParty && core->HasFeedback(FT_STATES)) displaymsg->DisplayConstantStringName(HCStrings::RangerFall, GUIColors::XPCHANGE, act);
}

void GameScript::RegainPaladinHood(Scriptable* Sender, Action* /*parameters*/)
{
	Actor* act = Scriptable::As<Actor>(Sender);
	if (!act) {
		return;
	}
	// prevent refalling on next rep update (the originals did the same)
	if (core->GetGame()->Reputation < 100) {
		core->GetGame()->SetReputation(100);
	}
	act->SetMCFlag(MC_FALLEN_PALADIN, BitOp::NAND);
	act->fxqueue.RemoveAllEffectsWithParam(fx_disable_button_ref, ACT_CAST);
	act->fxqueue.RemoveAllEffectsWithParam(fx_disable_button_ref, ACT_TURN);
	act->ApplyKit(false, Actor::GetClassID(ISPALADIN));
}

void GameScript::RegainRangerHood(Scriptable* Sender, Action* /*parameters*/)
{
	Actor* act = Scriptable::As<Actor>(Sender);
	if (!act) {
		return;
	}
	// prevent refalling on next rep update (the originals did the same)
	if (core->GetGame()->Reputation < 100) {
		core->GetGame()->SetReputation(100);
	}
	act->SetMCFlag(MC_FALLEN_RANGER, BitOp::NAND);
	act->fxqueue.RemoveAllEffectsWithParam(fx_disable_button_ref, ACT_CAST);
	act->fxqueue.RemoveAllEffectsWithParam(fx_disable_button_ref, ACT_STEALTH);
	act->ApplyKit(false, Actor::GetClassID(ISRANGER));
}

// transferring item from Sender to target, target must be an actor
//if target can't get it, it will be dropped at its feet
//a container or an actor can take an item from someone
void GameScript::GetItem(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetScriptableFromObject(Sender, parameters);
	if (!tar) {
		return;
	}
	MoveItemCore(tar, Sender, parameters->string0Parameter, 0, 0);
}

//getting one single item
void GameScript::TakePartyItem(Scriptable* Sender, Action* parameters)
{
	const Game* game = core->GetGame();
	int i = game->GetPartySize(false);
	while (i--) {
		Actor* pc = game->GetPC(i, false);
		MIC res = MoveItemCore(pc, Sender, parameters->string0Parameter, IE_INV_ITEM_UNDROPPABLE, IE_INV_ITEM_UNSTEALABLE);
		if (res != MIC::NoItem) return;
	}
}

//getting x single item
void GameScript::TakePartyItemNum(Scriptable* Sender, Action* parameters)
{
	int count = parameters->int0Parameter;
	const Game* game = core->GetGame();
	int i = game->GetPartySize(false);
	while (i-- && count) {
		Actor* pc = game->GetPC(i, false);
		MIC res = MoveItemCore(pc, Sender, parameters->string0Parameter, IE_INV_ITEM_UNDROPPABLE, IE_INV_ITEM_UNSTEALABLE, 1);
		if (res == MIC::GotItem) {
			i++;
			count--;
		}
	}
}

void GameScript::TakePartyItemRange(Scriptable* Sender, Action* parameters)
{
	const Game* game = core->GetGame();
	int i = game->GetPartySize(false);
	while (i--) {
		Actor* ac = game->GetPC(i, false);
		if (Distance(Sender, ac) < MAX_OPERATING_DISTANCE) {
			while (MoveItemCore(ac, Sender, parameters->string0Parameter, IE_INV_ITEM_UNDROPPABLE, IE_INV_ITEM_UNSTEALABLE) == MIC::GotItem) {}
		}
	}
}

void GameScript::TakePartyItemAll(Scriptable* Sender, Action* parameters)
{
	const Game* game = core->GetGame();
	int i = game->GetPartySize(false);
	while (i--) {
		while (MoveItemCore(game->GetPC(i, false), Sender, parameters->string0Parameter, IE_INV_ITEM_UNDROPPABLE, IE_INV_ITEM_UNSTEALABLE) == MIC::GotItem) {}
	}
}

//an actor can 'give' an item to a container or another actor
void GameScript::GiveItem(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetScriptableFromObject(Sender, parameters);
	MoveItemCore(Sender, tar, parameters->string0Parameter, 0, 0);
}

//this action creates an item in a container or a creature
//if there is an object it works as GiveItemCreate
//otherwise it creates the item on the Sender
void GameScript::CreateItem(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar;
	if (parameters->objects[1]) {
		tar = GetScriptableFromObject(Sender, parameters);
		// originals fell back to Player1 if Player1-6 tried to match and failed
		// one of the reasons for nodecode.ids #2233
		if (!tar && parameters->objects[1]->objectFilters[0] >= 21 && parameters->objects[1]->objectFilters[0] < 27) {
			tar = core->GetGame()->GetPC(0, false);
		}
	} else {
		tar = Sender;
	}
	if (!tar)
		return;
	Inventory* myinv;

	switch (tar->Type) {
		case ST_ACTOR:
			myinv = &(static_cast<Actor*>(tar)->inventory);
			break;
		case ST_CONTAINER:
			myinv = &(static_cast<Container*>(tar)->inventory);
			break;
		default:
			return;
	}

	CREItem* item = new CREItem();
	if (!CreateItemCore(item, parameters->resref0Parameter, parameters->int0Parameter, parameters->int1Parameter, parameters->int2Parameter)) {
		delete item;
		return;
	}
	if (tar->Type == ST_CONTAINER) {
		myinv->AddItem(item);
		return;
	}

	const Actor* act = static_cast<const Actor*>(tar);
	if (ASI_SUCCESS != myinv->AddSlotItem(item, SLOT_ONLYINVENTORY)) {
		Map* map = tar->GetCurrentArea();
		// drop it at my feet
		map->AddItemToLocation(tar->Pos, item);
		if (act->InParty) {
			act->VerbalConstant(Verbal::InventoryFull);
			displaymsg->DisplayMsgCentered(HCStrings::InventoryFullItemDrop, FT_MISC, GUIColors::XPCHANGE);
		}
	} else if (act->InParty) {
		displaymsg->DisplayMsgCentered(HCStrings::GotItem, FT_MISC, GUIColors::XPCHANGE);
	}
}

void GameScript::CreateItemNumGlobal(Scriptable* Sender, Action* parameters)
{
	Inventory* myinv;

	switch (Sender->Type) {
		case ST_ACTOR:
			myinv = &(static_cast<Actor*>(Sender)->inventory);
			break;
		case ST_CONTAINER:
			myinv = &(static_cast<Container*>(Sender)->inventory);
			break;
		default:
			return;
	}
	int value = CheckVariable(Sender, parameters->string0Parameter);
	CREItem* item = new CREItem();
	if (!CreateItemCore(item, parameters->resref1Parameter, value, 0, 0)) {
		delete item;
		return;
	}
	if (Sender->Type == ST_CONTAINER) {
		myinv->AddItem(item);
		return;
	}

	const Actor* act = static_cast<const Actor*>(Sender);
	if (ASI_SUCCESS != myinv->AddSlotItem(item, SLOT_ONLYINVENTORY)) {
		Map* map = Sender->GetCurrentArea();
		// drop it at my feet
		map->AddItemToLocation(Sender->Pos, item);
		if (act->InParty) {
			act->VerbalConstant(Verbal::InventoryFull);
			displaymsg->DisplayMsgCentered(HCStrings::InventoryFullItemDrop, FT_MISC, GUIColors::XPCHANGE);
		}
	} else if (act->InParty) {
		displaymsg->DisplayMsgCentered(HCStrings::GotItem, FT_MISC, GUIColors::XPCHANGE);
	}
}

// supports invitem.ids in ees
void GameScript::SetItemFlags(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar;
	if (parameters->objects[1]) {
		tar = GetScriptableFromObject(Sender, parameters);
	} else {
		tar = Sender;
	}
	if (!tar) return;

	const Inventory* myinv;
	switch (tar->Type) {
		case ST_ACTOR:
			myinv = &(static_cast<Actor*>(tar)->inventory);
			break;
		case ST_CONTAINER:
			myinv = &(static_cast<Container*>(tar)->inventory);
			break;
		default:
			return;
	}

	int slot = myinv->FindItem(parameters->resref0Parameter, 0);
	if (slot == -1) {
		Log(ERROR, "GameScript", "Item {} not found in inventory of {}", parameters->string0Parameter, tar->GetScriptName());
		return;
	}

	BitOp op = BitOp::NAND;
	if (parameters->int1Parameter) op = BitOp::OR;
	myinv->ChangeItemFlag(slot, parameters->int0Parameter, op);
}

void GameScript::TakeItemReplace(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetScriptableFromObject(Sender, parameters);
	Actor* scr = Scriptable::As<Actor>(tar);
	if (!scr) {
		return;
	}

	CREItem* item;
	int slot = scr->inventory.RemoveItem(parameters->string1Parameter, IE_INV_ITEM_UNDROPPABLE, &item);
	if (!item) {
		item = new CREItem();
	}
	if (!CreateItemCore(item, parameters->resref0Parameter, -1, 0, 0)) {
		delete item;
		return;
	}
	if (ASI_SUCCESS != scr->inventory.AddSlotItem(item, slot)) {
		Map* map = scr->GetCurrentArea();
		map->AddItemToLocation(Sender->Pos, item);
	}
}

void GameScript::TakeCreatureItems(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetScriptableFromObject(Sender, parameters);
	Actor* victim = Scriptable::As<Actor>(tar);
	Actor* taker = Scriptable::As<Actor>(Sender);
	if (!victim || !taker) return;

	// slot types are special, see takeitm.ids
	// 2 EQUIPPED has to be handled separately
	std::vector<ieDword> takeItm = { SLOT_ALL, SLOT_INVENTORY, 0, SLOT_WEAPON, SLOT_ITEM };
	if (parameters->int0Parameter == 2) {
		int eqs = victim->inventory.GetEquippedSlot();
		const CREItem* slot = victim->inventory.GetSlotItem(eqs);
		if (!slot) return;
		MoveItemCore(victim, taker, slot->ItemResRef, 0, 0);
		return;
	}

	int maxCount = static_cast<int>(core->SlotTypes);
	for (int i = 0; i < maxCount; i++) {
		ieDword id = core->QuerySlot(i);
		ieDword sType = core->QuerySlotType(id);
		if (!(sType & takeItm[parameters->int0Parameter])) {
			continue;
		}

		const CREItem* slot = victim->inventory.GetSlotItem(id);
		if (!slot) continue;
		MoveItemCore(victim, taker, slot->ItemResRef, 0, 0);
	}
}

//same as equipitem, but with additional slots parameter, and object to perform action
// XEquipItem("00Troll1",Myself,SLOT_RING_LEFT,TRUE)
void GameScript::XEquipItem(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetScriptableFromObject(Sender, parameters);
	Actor* actor = Scriptable::As<Actor>(tar);
	if (!actor) {
		return;
	}
	int slot = actor->inventory.FindItem(parameters->resref0Parameter, IE_INV_ITEM_UNDROPPABLE);
	if (slot < 0) {
		return;
	}

	int slot2 = parameters->int0Parameter;
	bool equip = parameters->int1Parameter;

	if (equip) {
		if (slot != slot2) {
			// swap them first, so we equip to the desired slot
			CREItem* si = actor->inventory.RemoveItem(slot);
			CREItem* si2 = actor->inventory.RemoveItem(slot2);
			if (actor->inventory.AddSlotItem(si, slot2) != ASI_SUCCESS) {
				// should never happen, since we just made room
				error("Actions", "XEquip: suddenly no slots left!");
			}
			if (si2) {
				actor->inventory.AddSlotItem(si2, slot);
			}
		}
		actor->inventory.EquipItem(slot2);
	} else {
		CREItem* si = actor->inventory.RemoveItem(slot);
		if (si && actor->inventory.AddSlotItem(si, SLOT_ONLYINVENTORY) == ASI_FAILED) {
			Map* map = Sender->GetCurrentArea();
			if (map) {
				//drop item at the feet of the character instead of destroying it
				map->AddItemToLocation(Sender->Pos, si);
			} else {
				delete si;
			}
		}
	}

	actor->ReinitQuickSlots();
}

//GemRB extension: if int1Parameter is nonzero, don't destroy existing items
void GameScript::FillSlot(Scriptable* Sender, Action* parameters)
{
	Actor* actor = Scriptable::As<Actor>(Sender);
	if (!actor) {
		return;
	}

	//free up target slot
	int slot = parameters->int0Parameter;
	CREItem* tmp = actor->inventory.RemoveItem(slot);

	actor->inventory.TryEquipAll(slot);

	if (tmp) {
		if (!actor->inventory.IsSlotEmpty(slot)) {
			slot = SLOT_ONLYINVENTORY;
		}

		//reequip original item
		if (actor->inventory.AddSlotItem(tmp, slot) != ASI_SUCCESS) {
			delete tmp;
		}
	}
}

// iwd2 also has a flag for unequip (it collides with the original!)
// it's basically EquipItemEx(S:Object*,I:EquipUnEquip*) from the ees,
// with xequip.ids having 1 for equipping, 0 for unequipping
// luckily iwd2 always uses both params
void GameScript::EquipItem(Scriptable* Sender, Action* parameters)
{
	Actor* actor = Scriptable::As<Actor>(Sender);
	if (!actor) {
		return;
	}
	int slot = actor->inventory.FindItem(parameters->resref0Parameter, IE_INV_ITEM_UNDROPPABLE);
	if (slot < 0) {
		return;
	}

	int slot2;
	bool zeroEquips = !core->HasFeature(GFFlags::RULES_3ED);
	if (parameters->int0Parameter == zeroEquips) {
		//unequip item, and move it to the inventory
		slot2 = SLOT_ONLYINVENTORY;
	} else {
		//equip item if possible
		slot2 = SLOT_AUTOEQUIP;
	}
	CREItem* si = actor->inventory.RemoveItem(slot);
	if (!si) {
		actor->ReinitQuickSlots();
		return;
	}

	// try the same slot first if it would be eligible
	if (slot2 == SLOT_AUTOEQUIP) {
		if (actor->inventory.AddSlotItem(si, slot) != ASI_FAILED) {
			actor->ReinitQuickSlots();
			return;
		}
	}

	if (actor->inventory.AddSlotItem(si, slot2) == ASI_FAILED) {
		Map* map = Sender->GetCurrentArea();
		if (map) {
			//drop item at the feet of the character instead of destroying it
			map->AddItemToLocation(Sender->Pos, si);
		} else {
			delete si;
		}
	}
	actor->ReinitQuickSlots();
}

void GameScript::DropItem(Scriptable* Sender, Action* parameters)
{
	Actor* scr = Scriptable::As<Actor>(Sender);
	if (!scr) {
		Sender->ReleaseCurrentAction();
		return;
	}

	// iwd2 has two uses with [-1.-1]
	if (parameters->pointParameter.x == -1) {
		parameters->pointParameter = Sender->Pos;
	}

	if (Distance(parameters->pointParameter, Sender) > 10) {
		MoveNearerTo(Sender, parameters->pointParameter, 10, 0);
		return;
	}
	Map* map = Sender->GetCurrentArea();

	if (!parameters->resref0Parameter.IsEmpty()) {
		//dropping location isn't exactly our place, this is why i didn't use a simple DropItem
		scr->inventory.DropItemAtLocation(parameters->resref0Parameter,
						  0, map, parameters->pointParameter);
	} else {
		//this should be converted from scripting slot to physical slot
		scr->inventory.DropItemAtLocation(parameters->int0Parameter, 0, map, parameters->pointParameter);
	}

	Sender->ReleaseCurrentAction();
}

void GameScript::DropInventory(Scriptable* Sender, Action* /*parameters*/)
{
	Actor* scr = Scriptable::As<Actor>(Sender);
	if (!scr) {
		return;
	}

	scr->DropItem("", 0);
}

//this should work on containers!
//using the same code for DropInventoryEXExclude
void GameScript::DropInventoryEX(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetScriptableFromObject(Sender, parameters);
	if (!tar) {
		return;
	}
	Inventory* inv = NULL;
	switch (tar->Type) {
		case ST_ACTOR:
			inv = &(static_cast<Actor*>(tar)->inventory);
			break;
		case ST_CONTAINER:
			inv = &(static_cast<Container*>(tar)->inventory);
			break;
		default:;
	}
	if (!inv) return;

	int x = inv->GetSlotCount();
	Map* area = tar->GetCurrentArea();
	while (x--) {
		if (!parameters->resref0Parameter.IsEmpty()) {
			const ResRef& itemRef = inv->GetSlotItem(x)->ItemResRef;
			if (itemRef == parameters->resref0Parameter) {
				continue;
			}
		}
		inv->DropItemAtLocation(x, 0, area, tar->Pos);
	}
}

void GameScript::GivePartyAllEquipment(Scriptable* Sender, Action* /*parameters*/)
{
	const Actor* scr = Scriptable::As<Actor>(Sender);
	if (!scr) {
		return;
	}
	const Game* game = core->GetGame();
	// pick the first actor first
	for (int i = 0; i < game->GetPartySize(false); i++) {
		Actor* tar = game->GetPC(i, false);
		//don't try to give self, it would be an infinite loop
		if (tar == scr)
			continue;
		while (MoveItemCore(Sender, tar, "", 0, 0) != MIC::NoItem) {}
	}
}

//This is unsure, Plunder could be just handling ground piles and not dead actors
void GameScript::Plunder(Scriptable* Sender, Action* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		Sender->ReleaseCurrentAction();
		return;
	}
	Scriptable* tar = GetStoredActorFromObject(Sender, parameters);
	if (!tar) {
		Sender->ReleaseCurrentAction();
		return;
	}

	//you must be joking
	if (tar == Sender) {
		Sender->ReleaseCurrentAction();
		return;
	}

	// can plunder only dead actors
	const Actor* scr = Scriptable::As<Actor>(tar);
	if (scr && !(scr->BaseStats[IE_STATE_ID] & STATE_DEAD)) {
		Sender->ReleaseCurrentAction();
		return;
	}
	if (PersonalDistance(Sender, tar) > MAX_OPERATING_DISTANCE) {
		MoveNearerTo(Sender, tar->Pos, MAX_OPERATING_DISTANCE, 0);
		return;
	}
	//move all movable item from the target to the Sender
	//the rest will be dropped at the feet of Sender
	while (MoveItemCore(tar, Sender, "", 0, 0) != MIC::NoItem) {}
	Sender->ReleaseCurrentAction();
}

void GameScript::MoveInventory(Scriptable* Sender, Action* parameters)
{
	Scriptable* src = GetScriptableFromObject(Sender, parameters);
	if (!src || src->Type != ST_ACTOR) {
		return;
	}
	Scriptable* tar = GetScriptableFromObject2(Sender, parameters);
	if (!tar) {
		return;
	}
	//don't try to move to self, it would create infinite loop
	if (src == tar)
		return;
	//move all movable item from the target to the Sender
	//the rest will be dropped at the feet of Sender
	while (MoveItemCore(src, tar, "", 0, 0) != MIC::NoItem) {}
}

void GameScript::PickPockets(Scriptable* Sender, Action* parameters)
{
	Actor* snd = Scriptable::As<Actor>(Sender);
	if (!snd) {
		Sender->ReleaseCurrentAction();
		return;
	}
	Scriptable* tar = GetStoredActorFromObject(Sender, parameters);
	Actor* scr = Scriptable::As<Actor>(tar);
	if (!scr) {
		Sender->ReleaseCurrentAction();
		return;
	}

	//for PP one must go REALLY close
	Map* map = Sender->GetCurrentArea();
	if (!map) {
		Sender->ReleaseCurrentAction();
		return;
	}

	if (PersonalDistance(Sender, tar) > 10) {
		MoveNearerTo(Sender, tar, 10);
		return;
	}

	static bool turnHostile = core->HasFeature(GFFlags::STEAL_IS_ATTACK);
	static bool reportFailure = core->HasFeedback(FT_MISC);
	static bool breakInvisibility = true;
	AutoTable ppBehave = gamedata->LoadTable("ppbehave");
	if (ppBehave) {
		turnHostile = ppBehave->QueryFieldSigned<int>("TURN_HOSTILE", "VALUE") == 1;
		reportFailure &= ppBehave->QueryFieldSigned<int>("REPORT_FAILURE", "VALUE") == 1;
		breakInvisibility = ppBehave->QueryFieldSigned<int>("BREAK_INVISIBILITY", "VALUE") == 1;
	}

	if (scr->GetStat(IE_EA) > EA_EVILCUTOFF) {
		if (reportFailure) displaymsg->DisplayMsgAtLocation(HCStrings::PickpocketEvil, FT_ANY, Sender, Sender, GUIColors::WHITE);
		Sender->ReleaseCurrentAction();
		return;
	}

	// determine slot to steal from and potentially adjust difficulty
	int slot = -1;
	if ((RandomNumValue & 3) || scr->GetStat(IE_GOLD) <= 0) {
		slot = scr->inventory.FindStealableItem();
	}
	int checkDC = 50;
	AutoTable slotStealDC = gamedata->LoadTable("sltsteal", true);
	if (slotStealDC && slot != -1) {
		int realSlot = core->QuerySlot(slot);
		checkDC = slotStealDC->QueryFieldSigned<int>(realSlot, 0);
	}

	int skill = snd->GetStat(IE_PICKPOCKET);
	int tgt = scr->GetStat(IE_PICKPOCKET);
	int check;
	if (core->HasFeature(GFFlags::RULES_3ED)) {
		skill = snd->GetSkill(IE_PICKPOCKET);
		int roll = core->Roll(1, 20, 0);
		int level = scr->GetXPLevel(true);
		int wismod = scr->GetAbilityBonus(IE_WIS);
		// ~Pick pocket check. (10 + skill w/Dex bonus) %d vs. ((d20 + target's level) + Wisdom modifier) %d + %d.~
		displaymsg->DisplayRollStringName(ieStrRef::ROLL12, GUIColors::LIGHTGREY, snd, 10 + skill, roll + level, wismod);
		check = (10 + skill) > (roll + level + wismod);
		if (skill == 0) { // a trained skill, make sure we fail
			check = 1;
		}
	} else {
		//the original engine has no random here
		if (tgt != 255) {
			skill -= tgt;
			//if you want original behaviour: remove this
			skill += snd->LuckyRoll(1, 100, 0);
		} else {
			skill = 0;
		}
		//and change this 50 to 0.
		check = skill < checkDC;
	}
	if (check) {
		//noticed attempt
		if (reportFailure) displaymsg->DisplayMsgAtLocation(HCStrings::PickpocketFail, FT_ANY, Sender, Sender, GUIColors::WHITE);
		if (breakInvisibility) {
			snd->SetModal(Modal::None);
			snd->CureInvisibility();
		}
		if (turnHostile && !core->HasFeature(GFFlags::HAS_EE_EFFECTS)) {
			scr->AttackedBy(snd);
		} else {
			//pickpocket failed trigger
			tar->AddTrigger(TriggerEntry(trigger_pickpocketfailed, snd->GetGlobalID()));
		}
		Sender->ReleaseCurrentAction();
		return;
	}

	MIC ret = MIC::NoItem;
	if (slot != -1) {
		CREItem* item = scr->inventory.RemoveItem(slot);
		int rc = snd->inventory.AddSlotItem(item, SLOT_ONLYINVENTORY);
		if (rc != ASI_SUCCESS) {
			map->AddItemToLocation(snd->Pos, item);
			ret = MIC::Full;
		}
	}

	if (ret == MIC::NoItem) {
		int money = 0;
		//go for money too
		if (scr->GetStat(IE_GOLD) > 0) {
			money = (RandomNumValue % scr->GetStat(IE_GOLD)) + 1;
		}
		if (!money) {
			//no stuff to steal
			if (reportFailure) displaymsg->DisplayMsgAtLocation(HCStrings::PickpocketNone, FT_ANY, Sender, Sender, GUIColors::WHITE);
			Sender->ReleaseCurrentAction();
			return;
		}
		CREItem* item = new CREItem();
		if (!CreateItemCore(item, core->GoldResRef, money, 0, 0)) {
			error("GameScript", "Failed to create {} of pick-pocketed gold '{}'!", money, core->GoldResRef);
		}
		scr->SetBase(IE_GOLD, scr->GetBase(IE_GOLD) - money);
		if (ASI_SUCCESS != snd->inventory.AddSlotItem(item, SLOT_ONLYINVENTORY)) {
			// drop it at my feet
			map->AddItemToLocation(snd->Pos, item);
			ret = MIC::Full;
		}
	}

	if (core->HasFeedback(FT_MISC)) displaymsg->DisplayMsgAtLocation(HCStrings::PickpocketDone, FT_ANY, Sender, Sender, GUIColors::WHITE);
	DisplayStringCoreVC(snd, Verbal::PickedPocket, DS_CONSOLE);

	int xp = gamedata->GetXPBonus(XP_PICKPOCKET, scr->GetXPLevel(1));
	core->GetGame()->ShareXP(xp, SX_DIVIDE);

	if (ret == MIC::Full && snd->InParty) {
		if (!core->HasFeature(GFFlags::PST_STATE_FLAGS)) snd->VerbalConstant(Verbal::InventoryFull);
		if (reportFailure) displaymsg->DisplayMsgAtLocation(HCStrings::PickpocketInventoryFull, FT_ANY, Sender, Sender, GUIColors::WHITE);
	}
	Sender->ReleaseCurrentAction();
}

void GameScript::TakeItemList(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetScriptableFromObject(Sender, parameters);
	if (!tar || tar->Type != ST_ACTOR) {
		return;
	}
	AutoTable tab = gamedata->LoadTable(parameters->resref0Parameter);
	if (!tab) {
		return;
	}

	TableMgr::index_t rows = tab->GetRowCount();
	for (TableMgr::index_t i = 0; i < rows; ++i) {
		MoveItemCore(tar, Sender, tab->QueryField(i, 0), 0, IE_INV_ITEM_UNSTEALABLE);
	}
}

void GameScript::TakeItemListParty(Scriptable* Sender, Action* parameters)
{
	AutoTable tab = gamedata->LoadTable(parameters->resref0Parameter);
	if (!tab) {
		return;
	}
	const Game* game = core->GetGame();
	TableMgr::index_t rows = tab->GetRowCount();
	for (TableMgr::index_t i = 0; i < rows; ++i) {
		int j = game->GetPartySize(false);
		while (j--) {
			Actor* tar = game->GetPC(j, false);
			MoveItemCore(tar, Sender, tab->QueryField(i, 0), 0, IE_INV_ITEM_UNSTEALABLE);
		}
	}
}

void GameScript::TakeItemListPartyNum(Scriptable* Sender, Action* parameters)
{
	AutoTable tab = gamedata->LoadTable(parameters->resref0Parameter);
	if (!tab) {
		return;
	}
	const Game* game = core->GetGame();
	TableMgr::index_t rows = tab->GetRowCount();
	int count = parameters->int0Parameter;
	for (TableMgr::index_t i = 0; i < rows; ++i) {
		int j = game->GetPartySize(false);
		while (j--) {
			Actor* tar = game->GetPC(j, false);
			MIC res = MoveItemCore(tar, Sender, tab->QueryField(i, 0), 0, IE_INV_ITEM_UNSTEALABLE);
			if (res == MIC::GotItem) {
				j++;
				count--;
			}
			if (!count) break;
		}
	}
	if (count == 1) {
		// grant the default table item to the Sender in regular games
		Action* params = new Action(true);
		params->resref0Parameter = tab->QueryDefault();
		CreateItem(Sender, params);
		delete params;
	}
}

//bg2
void GameScript::SetRestEncounterProbabilityDay(Scriptable* Sender, Action* parameters)
{
	Map* map = Sender->GetCurrentArea();
	map->RestHeader.DayChance = (ieWord) parameters->int0Parameter;
}

void GameScript::SetRestEncounterProbabilityNight(Scriptable* Sender, Action* parameters)
{
	Map* map = Sender->GetCurrentArea();
	map->RestHeader.NightChance = (ieWord) parameters->int0Parameter;
}

//iwd
void GameScript::SetRestEncounterChance(Scriptable* Sender, Action* parameters)
{
	Map* map = Sender->GetCurrentArea();
	map->RestHeader.DayChance = (ieWord) parameters->int0Parameter;
	map->RestHeader.NightChance = (ieWord) parameters->int1Parameter;
}

//easily hardcoded end sequence
void GameScript::EndCredits(Scriptable* Sender, Action* parameters)
{
	if (gamedata->Exists("25ecred", IE_2DA_CLASS_ID, true)) {
		/* ToB */
		ExecuteString(Sender, "TextScreen(\"25ecred\")");
	} else {
		core->PlayMovie("credits");
		QuitGame(Sender, parameters);
	}
}

//easily hardcoded end sequence
void GameScript::ExpansionEndCredits(Scriptable* Sender, Action* parameters)
{
	core->PlayMovie("ecredit");

	// end the game for HoW-only runs, but teleport back to Kuldahar for full iwd runs
	bool howOnly = CheckVariable(Sender, "JOIN_POSSE", "GLOBAL") == 0; // 0 how only, >0 for full run
	if (howOnly) {
		QuitGame(Sender, parameters);
	} else {
		static const ResRef area = "ar2109";
		Point dest(275, 235);
		const Game* game = core->GetGame();
		game->MovePCs(area, dest, -1);
		game->MoveFamiliars(area, dest, -1);
	}
}

//always quits game, but based on game it can play end animation, or display
//death text, etc
//this covers:
//QuitGame (play two of 3 movies in PST, display death screen with strref; names also in movval.ids)
//EndGame (display death screen with strref)
void GameScript::QuitGame(Scriptable* Sender, Action* parameters)
{
	auto& vars = core->GetDictionary();
	ClearAllActions(Sender, parameters);
	vars.Set("QuitGame1", parameters->int0Parameter);
	vars.Set("QuitGame2", parameters->int1Parameter);
	vars.Set("QuitGame3", parameters->int2Parameter);
	core->SetNextScript("QuitGame");
}

//BG2 demo end, shows some pictures then goes to main screen
void GameScript::DemoEnd(Scriptable* Sender, Action* parameters)
{
	auto& vars = core->GetDictionary();
	ClearAllActions(Sender, parameters);
	vars.Set("QuitGame1", 0);
	vars.Set("QuitGame2", 0);
	vars.Set("QuitGame3", -1);
	core->SetNextScript("QuitGame");
}

void GameScript::StopMoving(Scriptable* Sender, Action* /*parameters*/)
{
	Actor* actor = Scriptable::As<Actor>(Sender);
	if (!actor) {
		return;
	}

	actor->ClearPath();
}

void GameScript::ApplyDamage(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetScriptableFromObject(Sender, parameters);
	Actor* damagee = Scriptable::As<Actor>(tar);
	if (!damagee) {
		return;
	}
	Actor* damager = Scriptable::As<Actor>(Sender);
	if (!damager) {
		damager = damagee;
	}

	damagee->Damage(parameters->int0Parameter, parameters->int1Parameter >> 16, damager);
}

void GameScript::ApplyDamagePercent(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetScriptableFromObject(Sender, parameters);
	Actor* damagee = Scriptable::As<Actor>(tar);
	if (!damagee) {
		return;
	}
	Actor* damager = Scriptable::As<Actor>(Sender);
	if (!damager) {
		damager = damagee;
	}

	//this, if the percent is calculated from the current hp
	damagee->Damage((parameters->int0Parameter * damagee->Modified[IE_HITPOINTS]) / 100, parameters->int1Parameter >> 16, damager);
	//this, if the percent is calculated from the max hp
	//damagee->Damage(parameters->int0Parameter, parameters->int1Parameter >> 16, damager, MOD_PERCENT);
}

void GameScript::Damage(Scriptable* Sender, Action* parameters)
{
	Scriptable* damager = Sender;
	Scriptable* tar = GetScriptableFromObject(Sender, parameters);
	Actor* damagee = Scriptable::As<Actor>(tar);
	if (!damagee) {
		return;
	}

	// bones.ids handling
	int diceNum = (parameters->int1Parameter >> 12) & 15;
	int diceSize = (parameters->int1Parameter >> 4) & 255;
	int diceAdd = parameters->int1Parameter & 15;
	int damage = 0;
	const Actor* damager2 = Scriptable::As<Actor>(Sender);

	if (damager2 && damager2 != damagee) {
		damage = damager2->LuckyRoll(diceNum, diceSize, diceAdd, LR_DAMAGELUCK, damagee);
	} else {
		damage = core->Roll(diceNum, diceSize, diceAdd);
	}
	int type = MOD_ADDITIVE;
	// delta.ids
	switch (parameters->int0Parameter) {
		case DM_LOWER: // lower
			break;
		case DM_RAISE: //raise
			damage = -damage;
			break;
		case DM_SET: //set
			type = MOD_ABSOLUTE;
			break;
		case 4: // GemRB extension
			type = MOD_PERCENT;
			break;
		// NOTE: forge.d has a bunch of calls missing a parameter, eg. Damage(Protagonist, 15)
		// it's unclear if it worked, but let's accommodate it
		default:
			damage = parameters->int0Parameter;
			break;
	}
	//damagetype seems to be always 0
	damagee->Damage(damage, 0, damager, type);
}

void GameScript::SetHomeLocation(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetScriptableFromObject(Sender, parameters);
	Actor* movable = Scriptable::As<Actor>(tar);
	if (!movable) {
		return;
	}

	movable->HomeLocation = parameters->pointParameter;
	//no movement should be started here, i think
}

void GameScript::SetMasterArea(Scriptable* /*Sender*/, Action* parameters)
{
	core->GetGame()->SetMasterArea(parameters->resref0Parameter);
}

void GameScript::Berserk(Scriptable* Sender, Action* /*parameters*/)
{
	const Actor* act = Scriptable::As<Actor>(Sender);
	if (!act) {
		return;
	}

	const Map* map = Sender->GetCurrentArea();
	if (!map) {
		return;
	}

	const Actor* target;
	if (!act->GetStat(IE_BERSERKSTAGE2) && RAND(0, 1)) {
		//anyone
		target = GetNearestEnemyOf(map, act, ORIGIN_SEES_ENEMY);
	} else {
		target = GetNearestOf(map, act, ORIGIN_SEES_ENEMY);
	}

	if (!target) {
		Sender->SetWait(6);
	} else {
		//generate attack action
		Action* newaction = GenerateActionDirect("NIDSpecial3()", target);
		if (newaction) {
			Sender->AddActionInFront(newaction);
		}
	}
	Sender->ReleaseCurrentAction();
}

void GameScript::Panic(Scriptable* Sender, Action* /*parameters*/)
{
	Actor* act = Scriptable::As<Actor>(Sender);
	if (!act) {
		return;
	}
	act->Panic(nullptr, PanicMode::RandomWalk);
}

// All Calm() does is apply op4 on the target as if it was applied by the script runner
// NB: if berserking was caused by panic, we don't also cure the panic or stop any attacks
void GameScript::Calm(Scriptable* Sender, Action* /*parameters*/)
{
	static EffectRef fx_cure_berserk_state_ref = { "Cure:Berserk", -1 };
	Actor* act = Scriptable::As<Actor>(Sender);
	if (!act) {
		return;
	}
	Effect* fx = EffectQueue::CreateEffect(fx_cure_berserk_state_ref, 0, 0, FX_DURATION_INSTANT_PERMANENT);
	core->ApplyEffect(fx, act, Sender);
}

void GameScript::RevealAreaOnMap(Scriptable* /*Sender*/, Action* parameters)
{
	WorldMap* worldmap = core->GetWorldMap();
	if (!worldmap) {
		error("GameScript", "Can't find worldmap!");
	}
	// WMP_ENTRY_ADJACENT because otherwise revealed bg2 areas are unreachable from city gates
	worldmap->SetAreaStatus(parameters->resref0Parameter, WMP_ENTRY_VISIBLE | WMP_ENTRY_ADJACENT, BitOp::OR);
	displaymsg->DisplayConstantString(HCStrings::WorldmapChange, GUIColors::XPCHANGE);
}

void GameScript::HideAreaOnMap(Scriptable* /*Sender*/, Action* parameters)
{
	WorldMap* worldmap = core->GetWorldMap();
	if (!worldmap) {
		error("GameScript", "Can't find worldmap!");
	}
	// WMP_ENTRY_ADJACENT because otherwise revealed bg2 areas are unreachable from city gates
	worldmap->SetAreaStatus(parameters->resref0Parameter, WMP_ENTRY_VISIBLE | WMP_ENTRY_ADJACENT, BitOp::NAND);
}

void GameScript::AddWorldmapAreaFlag(Scriptable* /*Sender*/, Action* parameters)
{
	WorldMap* worldmap = core->GetWorldMap();
	if (!worldmap) {
		error("GameScript", "Can't find worldmap!");
	}
	worldmap->SetAreaStatus(parameters->resref0Parameter, parameters->int0Parameter, BitOp::OR);
}

void GameScript::RemoveWorldmapAreaFlag(Scriptable* /*Sender*/, Action* parameters)
{
	WorldMap* worldmap = core->GetWorldMap();
	if (!worldmap) {
		error("GameScript", "Can't find worldmap!");
	}
	worldmap->SetAreaStatus(parameters->resref0Parameter, parameters->int0Parameter, BitOp::NAND);
}

void GameScript::SendTrigger(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetScriptableFromObject(Sender, parameters, GA_NO_DEAD);
	if (!tar) {
		return;
	}
	tar->AddTrigger(TriggerEntry(trigger_trigger, parameters->int0Parameter));
}

void GameScript::Shout(Scriptable* Sender, Action* parameters)
{
	const Actor* actor = Scriptable::As<Actor>(Sender);
	// skip dead ones or the paladin ogres turn Garren hostile
	if (!actor || actor->GetStat(IE_STATE_ID) & STATE_DEAD) {
		return;
	}

	const Map* map = Sender->GetCurrentArea();
	map->Shout(actor, parameters->int0Parameter, false);
}

void GameScript::GlobalShout(Scriptable* Sender, Action* parameters)
{
	const Actor* actor = Scriptable::As<Actor>(Sender);
	if (!actor || actor->GetStat(IE_STATE_ID) & STATE_DEAD) {
		return;
	}

	const Map* map = Sender->GetCurrentArea();
	// true means global, unlimited, shout distance
	map->Shout(actor, parameters->int0Parameter, true);
}

void GameScript::Help(Scriptable* Sender, Action* /*parameters*/)
{
	const Actor* actor = Scriptable::As<Actor>(Sender);
	if (!actor) {
		return;
	}

	const Map* map = Sender->GetCurrentArea();
	map->Shout(actor, 0, false);
}

void GameScript::GiveOrder(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetScriptableFromObject(Sender, parameters);
	if (tar) {
		tar->AddTrigger(TriggerEntry(trigger_receivedorder, Sender->GetGlobalID(), parameters->int0Parameter));
	}
}

// ees support mapnotes.ids
void GameScript::AddMapnote(Scriptable* Sender, Action* parameters)
{
	Map* map = Sender->GetCurrentArea();
	map->AddMapNote(parameters->pointParameter, parameters->int1Parameter, ieStrRef(parameters->int0Parameter));
}

void GameScript::RemoveMapnote(Scriptable* Sender, Action* parameters)
{
	Map* map = Sender->GetCurrentArea();
	map->RemoveMapNote(parameters->pointParameter);
}

void GameScript::AttackOneRound(Scriptable* Sender, Action* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		Sender->ReleaseCurrentAction();
		return;
	}
	//using auto target!
	Scriptable* tar = GetStoredActorFromObject(Sender, parameters, GA_NO_DEAD);
	if (!tar || (tar->Type != ST_ACTOR && tar->Type != ST_DOOR && tar->Type != ST_CONTAINER)) {
		Sender->ReleaseCurrentAction();
		return;
	}

	//actor is already incapable of attack
	if (Sender->GetInternalFlag() & IF_STOPATTACK) {
		Sender->ReleaseCurrentAction();
		return;
	}

	if (!Sender->CurrentActionState) {
		Sender->CurrentActionState = core->Time.round_size;
	}

	AttackCore(Sender, tar, 0);

	if (Sender->CurrentActionState <= 1) {
		Sender->ReleaseCurrentAction();
	} else {
		Sender->CurrentActionState--;
	}
}

void GameScript::RunningAttackNoSound(Scriptable* Sender, Action* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		Sender->ReleaseCurrentAction();
		return;
	}
	//using auto target!
	Scriptable* tar = GetStoredActorFromObject(Sender, parameters, GA_NO_DEAD);
	if (!tar || (tar->Type != ST_ACTOR && tar->Type != ST_DOOR && tar->Type != ST_CONTAINER)) {
		Sender->ReleaseCurrentAction();
		return;
	}

	//actor is already incapable of attack
	if (Sender->GetInternalFlag() & IF_STOPATTACK) {
		Sender->ReleaseCurrentAction();
		return;
	}

	AttackCore(Sender, tar, AC_NO_SOUND | AC_RUNNING);
}

void GameScript::AttackNoSound(Scriptable* Sender, Action* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		Sender->ReleaseCurrentAction();
		return;
	}
	//using auto target!
	Scriptable* tar = GetStoredActorFromObject(Sender, parameters, GA_NO_DEAD);
	if (!tar || (tar->Type != ST_ACTOR && tar->Type != ST_DOOR && tar->Type != ST_CONTAINER)) {
		Sender->ReleaseCurrentAction();
		return;
	}

	//actor is already incapable of attack
	if (Sender->GetInternalFlag() & IF_STOPATTACK) {
		Sender->ReleaseCurrentAction();
		return;
	}

	AttackCore(Sender, tar, AC_NO_SOUND);
}

void GameScript::RunningAttack(Scriptable* Sender, Action* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		Sender->ReleaseCurrentAction();
		return;
	}
	//using auto target!
	Scriptable* tar = GetStoredActorFromObject(Sender, parameters, GA_NO_DEAD);
	if (!tar || (tar->Type != ST_ACTOR && tar->Type != ST_DOOR && tar->Type != ST_CONTAINER)) {
		Sender->ReleaseCurrentAction();
		return;
	}

	//actor is already incapable of attack
	if (Sender->GetInternalFlag() & IF_STOPATTACK) {
		Sender->ReleaseCurrentAction();
		return;
	}

	AttackCore(Sender, tar, AC_RUNNING);
}

void GameScript::Attack(Scriptable* Sender, Action* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		Sender->ReleaseCurrentAction();
		return;
	}
	//using auto target!
	Scriptable* tar = GetStoredActorFromObject(Sender, parameters, GA_NO_DEAD);

	if (!tar || (tar->Type != ST_ACTOR && tar->Type != ST_DOOR && tar->Type != ST_CONTAINER) || tar == Sender) {
		Sender->ReleaseCurrentAction();
		return;
	}

	//actor is already incapable of attack
	if (Sender->GetInternalFlag() & IF_STOPATTACK) {
		Sender->ReleaseCurrentAction();
		return;
	}

	AttackCore(Sender, tar, 0);
}

void GameScript::ForceAttack(Scriptable* Sender, Action* parameters)
{
	Scriptable* scr = GetScriptableFromObject(Sender, parameters, GA_NO_DEAD);
	if (!scr || scr->Type != ST_ACTOR) {
		return;
	}
	Scriptable* tar = GetScriptableFromObject2(Sender, parameters, GA_NO_DEAD);
	if (!tar || (tar->Type != ST_ACTOR && tar->Type != ST_DOOR && tar->Type != ST_CONTAINER)) {
		return;
	}
	//this is a hack, we use a gui variable for our own hideous reasons?
	if (tar->Type == ST_ACTOR) {
		const GameControl* gc = core->GetGameControl();
		if (gc) {
			//saving the target object ID from the gui variable
			scr->AddAction(GenerateActionDirect("NIDSpecial3()", static_cast<Actor*>(tar)));
		}
	} else {
		scr->AddAction(fmt::format("BashDoor({})", tar->GetScriptName()));
	}
}

void GameScript::AttackReevaluate(Scriptable* Sender, Action* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		Sender->ReleaseCurrentAction();
		return;
	}

	if (Sender->CurrentActionState) {
		// check if our target is still ok, otherwise try to reevaluate it
		const Scriptable* curTar = core->GetGame()->GetActorByGlobalID(Sender->CurrentActionTarget);
		const Actor* curAct = Scriptable::As<Actor>(curTar);
		if (!curTar || (curAct && !curAct->ValidTarget(GA_NO_DEAD | GA_NO_UNSCHEDULED | GA_NO_HIDDEN))) {
			Sender->CurrentActionTarget = 0; // grab the target object anew below
		}
	} else {
		Sender->CurrentActionState = parameters->int0Parameter;
	}

	Scriptable* tar = GetStoredActorFromObject(Sender, parameters, GA_NO_DEAD);
	if (!tar || (tar->Type != ST_ACTOR && tar->Type != ST_DOOR && tar->Type != ST_CONTAINER)) {
		Sender->ReleaseCurrentAction();
		return;
	}

	//actor is already incapable of attack
	if (Sender->GetInternalFlag() & IF_STOPATTACK) {
		Sender->ReleaseCurrentAction();
		return;
	}

	// if same target as before, don't play the war cry again, as they'd pop up too often
	int flags = 0;
	if (Sender->objects.LastTargetPersistent == tar->GetGlobalID()) {
		flags = AC_NO_SOUND;
	}

	AttackCore(Sender, tar, flags);
	parameters->int2Parameter = 1;

	// we are trying to move between areas, so stop the normal flow
	if (Sender->GetInternalFlag() & IF_USEEXIT) {
		return;
	}

	Sender->CurrentActionState--;
	if (Sender->CurrentActionState <= 0) {
		Sender->ReleaseCurrentAction();
	}
}

// attack creatures with the same specific value as the target creature
void GameScript::GroupAttack(Scriptable* Sender, Action* parameters)
{
	Scriptable* scr = GetScriptableFromObject(Sender, parameters, GA_NO_DEAD);
	if (!scr || scr->Type != ST_ACTOR) {
		return;
	}

	Actor* actor = Scriptable::As<Actor>(scr);
	int specific = actor->GetStat(IE_SPECIFIC);
	Sender->ReleaseCurrentAction(); // it's not an instant
	Action* attack = GenerateAction("Attack()");
	attack->objects[1]->objectFields[4] = specific;
	actor->AddActionInFront(attack);
}

void GameScript::Explore(Scriptable* Sender, Action* /*parameters*/)
{
	Sender->GetCurrentArea()->FillExplored(true);
}

void GameScript::UndoExplore(Scriptable* Sender, Action* /*parameters*/)
{
	Sender->GetCurrentArea()->FillExplored(false);
}

void GameScript::ExploreMapChunk(Scriptable* Sender, Action* parameters)
{
	Map* map = Sender->GetCurrentArea();
	/*
	There is a mode flag in int1Parameter, but i don't know what is it,
	ExploreMapChunk will reveal both visibility/explored map, but the
	visibility will fade in the next update cycle (which is quite frequent).
	The action is an instant, so it will only run once.
	*/
	map->ExploreMapChunk(SearchmapPoint(parameters->pointParameter), parameters->int1Parameter, 0);
}

void GameScript::StartStore(Scriptable* Sender, Action* parameters)
{
	if (core->GetCurrentStore()) {
		return;
	}
	core->SetCurrentStore(parameters->resref0Parameter, Sender->GetGlobalID());
	core->SetEventFlag(EF_OPENSTORE);
	//sorry, i have absolutely no idea when i should do this :)
	Sender->ReleaseCurrentAction();
}

//The integer parameter is a GemRB extension, if set to 1, the player
//gains experience for learning the spell
void GameScript::AddSpecialAbility(Scriptable* Sender, Action* parameters)
{
	Actor* actor = Scriptable::As<Actor>(Sender);
	if (!actor) {
		return;
	}

	actor->LearnSpell(parameters->resref0Parameter, parameters->int0Parameter | LS_MEMO | LS_LEARN);
	core->SetEventFlag(EF_ACTION);
}

//actually this just depletes a spell, doesn't remove it from the book
//GemRB extension: the first/second int parameter can also make it removed
//from the spell memorization schedule (also from the spellbook)
void GameScript::RemoveSpell(Scriptable* Sender, Action* parameters)
{
	ResRef spellRes;
	int type;
	Actor* actor = Scriptable::As<Actor>(Sender);
	if (!actor) {
		return;
	}
	if (!ResolveSpellName(spellRes, parameters)) {
		return;
	}

	if (!parameters->resref0Parameter.IsEmpty()) {
		//the spell resref is in the string parameter
		type = parameters->int0Parameter;
	} else {
		//the spell number is in the int0 parameter
		type = parameters->int1Parameter;
	}
	if (type == 2) {
		//remove spell from both book and memorization
		actor->spellbook.RemoveSpell(spellRes);
		return;
	}
	//type == 1 remove spell only from memorization
	//type == 0 original behaviour: deplete only
	actor->spellbook.UnmemorizeSpell(spellRes, !type, 2);
}

void GameScript::SetScriptName(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetScriptableFromObject(Sender, parameters);
	if (!tar || tar->Type != ST_ACTOR) {
		return;
	}
	tar->SetScriptName(parameters->variable0Parameter);
}

//iwd2
//advance time with a constant
//This is in seconds according to IESDP
void GameScript::AdvanceTime(Scriptable* /*Sender*/, Action* parameters)
{
	core->GetGame()->AdvanceTime(parameters->int0Parameter * core->Time.defaultTicksPerSec);
	core->GetGame()->ResetPartyCommentTimes();
}

// advance at the beginning of the specified hour (minus one tick? unsure)
// the parameter is HOURS (time.ids, 0 to 23)
// never advance a full day or more (in fact, duplicating this action does nothing)
void GameScript::DayNight(Scriptable* /*Sender*/, Action* parameters)
{
	int delta = parameters->int0Parameter * core->Time.hour_size - core->GetGame()->GameTime % core->Time.day_size;
	if (delta < 0) {
		delta += core->Time.day_size;
	}
	core->GetGame()->AdvanceTime(delta, false);
}

// most games take no parameters: RestParty()
// pst style parameters: RestParty(I:SuggestedDream*,I:HP*,I:Renting*)
// - suggested dream: unused and always -1 in the original data
//   (compatibility: if suggested dream is 0, then area flags determine the 'movie')
// - hp: number of hps healed
// - renting: crashes pst, we simply ignore it
void GameScript::RestParty(Scriptable* Sender, Action* parameters)
{
	Game* game = core->GetGame();
	game->RestParty(RestChecks::NoCheck, parameters->int0Parameter, parameters->int1Parameter);
	Sender->ReleaseCurrentAction();
}

//doesn't advance game time, just removes fatigue & refreshes spells of target
//this is a non-blocking action
static EffectRef fx_fatigue_ref = { "FatigueModifier", -1 };

void GameScript::Rest(Scriptable* Sender, Action* /*parameters*/)
{
	Actor* actor = Scriptable::As<Actor>(Sender);
	if (!actor) {
		return;
	}
	actor->spellbook.ChargeAllSpells();
	actor->fxqueue.RemoveAllEffects(fx_fatigue_ref);
	actor->SetBase(IE_FATIGUE, 0);
}

//doesn't advance game time, just removes fatigue
void GameScript::RestNoSpells(Scriptable* Sender, Action* /*parameters*/)
{
	Actor* actor = Scriptable::As<Actor>(Sender);
	if (!actor) {
		return;
	}
	actor->fxqueue.RemoveAllEffects(fx_fatigue_ref);
	actor->SetBase(IE_FATIGUE, 0);
}

//this most likely advances time and heals whole party
void GameScript::RestUntilHealed(Scriptable* Sender, Action* /*parameters*/)
{
	core->GetGame()->RestParty(RestChecks::NoCheck, 0, 0);
	Sender->ReleaseCurrentAction();
}

//iwd2
//removes all delayed/duration/semi permanent effects (like a ctrl-r)
void GameScript::ClearPartyEffects(Scriptable* /*Sender*/, Action* /*parameters*/)
{
	const Game* game = core->GetGame();
	int i = game->GetPartySize(false);
	while (i--) {
		Actor* tar = game->GetPC(i, false);
		tar->fxqueue.RemoveExpiredEffects(0xffffffff);
	}
}

//iwd2 removes effects from a single sprite
void GameScript::ClearSpriteEffects(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetScriptableFromObject(Sender, parameters);
	Actor* actor = Scriptable::As<Actor>(tar);
	if (!actor) {
		return;
	}
	actor->fxqueue.RemoveExpiredEffects(0xffffffff);
}

//IWD2 special, can mark only actors, hope it is enough
void GameScript::MarkObject(Scriptable* Sender, Action* parameters)
{
	Actor* actor = Scriptable::As<Actor>(Sender);
	if (!actor) {
		return;
	}
	//unsure, could mark dead objects?
	const Scriptable* tar = GetScriptableFromObject(Sender, parameters, GA_NO_DEAD);
	if (!tar || tar->Type != ST_ACTOR) {
		return;
	}

	actor->objects.LastMarked = tar->GetGlobalID();
}

void GameScript::MarkSpellAndObject(Scriptable* Sender, Action* parameters)
{
	Actor* me = Scriptable::As<Actor>(Sender);
	if (!me) {
		return;
	}
	if (me->objects.LastMarkedSpell) {
		return;
	}

	const Scriptable* tar = GetScriptableFromObject(Sender, parameters);
	if (!tar) {
		// target died on us
		return;
	}
	const Actor* actor = Scriptable::As<Actor>(tar);

	int flags = parameters->int0Parameter;
	if (!(flags & MSO_IGNORE_NULL) && !actor) {
		return;
	}
	if (!(flags & MSO_IGNORE_INVALID) && actor && actor->InvalidSpellTarget()) {
		return;
	}
	if (!(flags & MSO_IGNORE_SEE) && actor && !CanSee(Sender, actor, true, 0)) {
		return;
	}
	uint8_t len = parameters->string0Parameter.length();
	// only allow multiples of 4
	if (!len || len & 3) {
		return;
	}
	len /= 4;
	size_t max = len;
	size_t pos = 0;
	if (flags & MSO_RANDOM_SPELL) {
		pos = core->Roll(1, len, -1);
	}
	while (len--) {
		ResRef spl = SubStr(parameters->string0Parameter, static_cast<uint8_t>(pos * 4), 4);
		spl[4] = '\0';
		int splnum = atoi(spl.c_str());

		if (!(flags & MSO_IGNORE_HAVE) && !me->spellbook.HaveSpell(splnum, 0)) {
			goto end_mso_loop;
		}
		int range;
		if ((flags & MSO_IGNORE_RANGE) || !actor) {
			range = 0;
		} else {
			range = Distance(me, actor);
		}
		if (!(flags & MSO_IGNORE_INVALID) && actor && actor->InvalidSpellTarget(splnum, me, range)) {
			goto end_mso_loop;
		}
		//mark spell and target
		me->objects.LastMarkedSpell = splnum;
		me->objects.LastSpellTarget = tar->GetGlobalID();
		break;
end_mso_loop:
		pos++;
		if (pos == max) {
			pos = 0;
		}
	}
}

void GameScript::ForceMarkedSpell(Scriptable* Sender, Action* parameters)
{
	Actor* actor = Scriptable::As<Actor>(Sender);
	if (!actor) {
		return;
	}
	actor->objects.LastMarkedSpell = parameters->int0Parameter;
}

void GameScript::SetMarkedSpell(Scriptable* Sender, Action* parameters)
{
	Actor* actor = Scriptable::As<Actor>(Sender);
	if (!actor) {
		return;
	}
	if (parameters->int0Parameter) {
		if (actor->objects.LastMarkedSpell) {
			return;
		}
		if (!actor->spellbook.HaveSpell(parameters->int0Parameter, 0)) {
			return;
		}
	}

	actor->objects.LastMarkedSpell = parameters->int0Parameter;
}

void GameScript::SetDialogueRange(Scriptable* Sender, Action* parameters)
{
	Actor* actor = Scriptable::As<Actor>(Sender);
	if (!actor) {
		return;
	}
	actor->SetBase(IE_DIALOGRANGE, parameters->int0Parameter);
}

void GameScript::SetGlobalTint(Scriptable* /*Sender*/, Action* parameters)
{
	Color c(parameters->int0Parameter, parameters->int1Parameter, parameters->int2Parameter, 0xff);
	core->GetWindowManager()->FadeColor = c;
}

void GameScript::SetArmourLevel(Scriptable* Sender, Action* parameters)
{
	const Scriptable* tar = GetScriptableFromObject(Sender, parameters);
	if (!tar || tar->Type != ST_ACTOR) {
		return;
	}
	Actor* actor = static_cast<Actor*>(Sender);
	actor->SetBase(IE_ARMOR_TYPE, parameters->int0Parameter);
}

void GameScript::RandomWalk(Scriptable* Sender, Action* /*parameters*/)
{
	Actor* actor = Scriptable::As<Actor>(Sender);
	if (!actor) {
		Sender->ReleaseCurrentAction();
		return;
	}
	actor->RandomWalk(true, false);
}

void GameScript::RandomRun(Scriptable* Sender, Action* /*parameters*/)
{
	Actor* actor = Scriptable::As<Actor>(Sender);
	if (!actor) {
		Sender->ReleaseCurrentAction();
		return;
	}
	actor->RandomWalk(true, true);
}

void GameScript::RandomWalkContinuous(Scriptable* Sender, Action* /*parameters*/)
{
	Actor* actor = Scriptable::As<Actor>(Sender);
	if (!actor || !actor->GetCurrentArea()) {
		Sender->ReleaseCurrentAction();
		return;
	}

	// unlike other randomwalk actions, this one queues its payload, so it can get interrupted;
	// it just queues MoveToPoint and itself again
	// ... that's why we don't use Movable::RandomWalk
	const Map* area = actor->GetCurrentArea();
	if (actor->BlocksSearchMap()) {
		area->ClearSearchMapFor(actor);
	}
	const auto randomStep = area->RandomWalk(actor->Pos, actor->circleSize, std::max<int>(5, actor->maxWalkDistance), actor);
	if (actor->BlocksSearchMap()) {
		area->BlockSearchMapFor(actor);
	}
	if (!randomStep.point.IsZero()) {
		Action* moveAction = GenerateAction("MoveToPoint()");
		moveAction->pointParameter = randomStep.point;
		Action* randomWalk = GenerateAction("RandomWalkContinuous()");
		actor->AddActionInFront(randomWalk);
		actor->AddActionInFront(moveAction);
	}

	actor->ReleaseCurrentAction();
}

void GameScript::RandomFly(Scriptable* Sender, Action* /*parameters*/)
{
	Actor* actor = Scriptable::As<Actor>(Sender);
	if (!actor) {
		Sender->ReleaseCurrentAction();
		return;
	}
	int x = RAND(0, 31);
	if (x < 10) {
		actor->SetOrientation(PrevOrientation(actor->GetOrientation()), false);
	} else if (x > 20) {
		actor->SetOrientation(NextOrientation(actor->GetOrientation()), false);
	}
	//fly in this direction for 20 steps
	actor->MoveLine(20, actor->GetOrientation());
}

//UseContainer uses the predefined target (like Nidspecial1 dialog hack)
void GameScript::UseContainer(Scriptable* Sender, Action* parameters)
{
	Actor* actor = Scriptable::As<Actor>(Sender);
	if (!actor) {
		Sender->ReleaseCurrentAction();
		return;
	}

	if (core->InCutSceneMode()) {
		//cannot use container in dialog or cutscene
		Sender->ReleaseCurrentAction();
		return;
	}

	Container* container = core->GetCurrentContainer();
	if (!container) {
		Log(WARNING, "GameScript", "No container selected!");
		Sender->ReleaseCurrentAction();
		return;
	}
	if (parameters->int2Parameter > 20) {
		Log(WARNING, "GameScript", "Could not get close enough to container!");
		Sender->ReleaseCurrentAction();
		return;
	}

	ieDword distance = PersonalDistance(Sender, container);
	ieDword needed = MAX_OPERATING_DISTANCE;
	// give up the strictness after 10 retries from the same spot
	if (!parameters->int2Parameter) {
		parameters->int1Parameter = distance;
		parameters->int2Parameter = 1;
	} else {
		if (parameters->int1Parameter == (signed) distance) {
			parameters->int2Parameter++;
		} else {
			parameters->int1Parameter = distance;
		}
	}
	if (container->containerType == IE_CONTAINER_PILE && parameters->int2Parameter < 10) {
		needed = 0; // less than a search square (width)
	}
	if (distance <= needed) {
		//check if the container is unlocked
		if (!container->TryUnlock(actor)) {
			//playsound can't open container
			//display string, etc
			displaymsg->DisplayMsgAtLocation(HCStrings::ContLocked, FT_MISC, container, actor);
			Sender->ReleaseCurrentAction();
			return;
		}
		actor->SetModal(Modal::None);
		if (container->Trapped) {
			container->AddTrigger(TriggerEntry(trigger_opened, actor->GetGlobalID()));
		} else {
			container->AddTrigger(TriggerEntry(trigger_harmlessopened, actor->GetGlobalID()));
		}
		container->TriggerTrap(0, actor->GetGlobalID());
		core->SetCurrentContainer(actor, container, true);
		Sender->ReleaseCurrentAction();
		return;
	}
	MoveNearerTo(Sender, container, needed, 1);
}

//call the usecontainer action in target (not used)
void GameScript::ForceUseContainer(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetScriptableFromObject(Sender, parameters);
	if (!tar || tar->Type != ST_ACTOR) {
		Sender->ReleaseCurrentAction(); //why blocking???
		return;
	}
	Action* newaction = GenerateAction("UseContainer()");
	tar->AddActionInFront(newaction);
	Sender->ReleaseCurrentAction(); //why blocking???
}

//these actions directly manipulate a game variable (as the original engine)
void GameScript::SetMazeEasier(Scriptable* Sender, Action* /*parameters*/)
{
	int value = CheckVariable(Sender, "MAZEDIFFICULTY", "GLOBAL");
	if (value > 0) {
		SetVariable(Sender, "MAZEDIFFICULTY", value - 1, "GLOBAL");
	}
}

void GameScript::SetMazeHarder(Scriptable* Sender, Action* /*parameters*/)
{
	int value = CheckVariable(Sender, "MAZEDIFFICULTY", "GLOBAL");
	if (value < 2) {
		SetVariable(Sender, "MAZEDIFFICULTY", value + 1, "GLOBAL");
	}
}

void GameScript::GenerateMaze(Scriptable* /*Sender*/, Action* /*parameters*/)
{
	core->SetEventFlag(EF_CREATEMAZE);
}

void GameScript::FixEngineRoom(Scriptable* Sender, Action* /*parameters*/)
{
	int value = CheckVariable(Sender, "EnginInMaze", "GLOBAL");
	if (value) {
		SetVariable(Sender, "EnginInMaze", 0, "GLOBAL");
		//this works only because the engine room exit depends only on the EnginInMaze variable
		auto sE = core->GetGUIScriptEngine();
		sE->RunFunction("Maze", "CustomizeArea");
	}
}

void GameScript::StartRainNow(Scriptable* /*Sender*/, Action* /*parameters*/)
{
	core->GetGame()->StartRainOrSnow(false, WB_RAIN | WB_RARELIGHTNING);
}

void GameScript::Weather(Scriptable* /*Sender*/, Action* parameters)
{
	Game* game = core->GetGame();
	switch (parameters->int0Parameter & WB_FOG) {
		case WB_NORMAL:
			game->StartRainOrSnow(false, 0);
			break;
		case WB_RAIN:
			game->StartRainOrSnow(true, WB_RAIN | WB_RARELIGHTNING);
			break;
		case WB_SNOW:
			game->StartRainOrSnow(true, WB_SNOW);
			break;
		case WB_FOG:
			game->StartRainOrSnow(true, WB_FOG);
			break;
	}
}

// Pos could be [-1,-1] in which case it copies the ground piles to their
// original position in the second area
void GameScript::CopyGroundPilesTo(Scriptable* Sender, Action* parameters)
{
	const Map* map = Sender->GetCurrentArea();
	Map* othermap = core->GetGame()->GetMap(parameters->resref0Parameter, false);
	if (!othermap) {
		return;
	}

	for (const auto& pile : map->GetTileMap()->GetContainers()) {
		if (pile->containerType != IE_CONTAINER_PILE) continue;

		// creating (or grabbing) the container in the other map at the given position
		Container* otherPile;
		if (parameters->pointParameter.IsInvalid()) {
			otherPile = othermap->GetPile(pile->Pos);
		} else {
			otherPile = othermap->GetPile(parameters->pointParameter);
		}

		// transfer the pile to the other container
		unsigned int i = pile->inventory.GetSlotCount();
		while (i--) {
			CREItem* item = pile->RemoveItem(i, 0);
			otherPile->AddItem(item);
		}
	}
}

void GameScript::MoveContainerContents(Scriptable* Sender, Action* parameters)
{
	const Map* map1 = Sender->GetCurrentArea();
	const Map* map2 = map1;
	ieVariable con1;
	ieVariable con2;
	Game* game = core->GetGame();
	// do the container names have area name prefixes? Eg. BD0103.BCS:
	// MoveContainerContents("BD0120*Imoen_import_eq","BD0103*Imoen_equipment")
	auto strParam = Explode<StringParam, ieVariable>(parameters->string0Parameter, '*');
	if (strParam.size() > 1) {
		map1 = game->GetMap(strParam[0], false);
		if (!map1) return;
		con1 = strParam[1];
	} else {
		con1 = parameters->variable0Parameter;
	}

	strParam = Explode<StringParam, ieVariable>(parameters->string1Parameter, '*');
	if (strParam.size() > 1) {
		map2 = game->GetMap(strParam[0], false);
		if (!map2) return;
		con2 = strParam[1];
	} else {
		con2 = parameters->variable1Parameter;
	}

	Container* cont1 = map1->GetTileMap()->GetContainer(con1);
	Container* cont2 = map2->GetTileMap()->GetContainer(con2);
	unsigned int i = cont1->inventory.GetSlotCount();
	while (i--) {
		CREItem* item = cont1->RemoveItem(i, 0);
		cont2->AddItem(item);
	}
}

// iwd specific, but unused in original data
// bardsong.ids mapping clearly does not follow regular spell naming:
const ResRef songs[6] = { "SPIN151", "SPIN144", "SPIN145", "SPIN146", "SPIN147", "SPIN148" };
void GameScript::PlayBardSong(Scriptable* Sender, Action* parameters)
{
	Actor* actor = Scriptable::As<Actor>(Sender);
	int songIdx = parameters->int0Parameter;
	if (!actor || songIdx < 0 || songIdx > 5) {
		return;
	}

	actor->SetModalSpell(Modal::BattleSong, songs[songIdx]);
	actor->SetModal(Modal::BattleSong);
}

void GameScript::BattleSong(Scriptable* Sender, Action* /*parameters*/)
{
	Actor* actor = Scriptable::As<Actor>(Sender);
	if (!actor) {
		return;
	}
	actor->SetModal(Modal::BattleSong);
}

void GameScript::FindTraps(Scriptable* Sender, Action* /*parameters*/)
{
	Actor* actor = Scriptable::As<Actor>(Sender);
	if (!actor) {
		return;
	}
	actor->SetModal(Modal::DetectTraps);
}

void GameScript::Hide(Scriptable* Sender, Action* /*parameters*/)
{
	Actor* actor = Scriptable::As<Actor>(Sender);
	if (!actor) {
		return;
	}

	if (actor->TryToHide()) {
		actor->SetModal(Modal::Stealth);
	}
	//TODO: expiry isn't instant (skill based transition?)
}

static EffectRef fx_set_invisible_state_ref = { "State:Invisible", -1 };
void GameScript::Unhide(Scriptable* Sender, Action* /*parameters*/)
{
	Actor* actor = Scriptable::As<Actor>(Sender);
	if (!actor) {
		return;
	}

	if (actor->Modal.State == Modal::Stealth) {
		actor->SetModal(Modal::None);
	}
	actor->fxqueue.RemoveAllEffects(fx_set_invisible_state_ref);
}

void GameScript::Turn(Scriptable* Sender, Action* /*parameters*/)
{
	Actor* actor = Scriptable::As<Actor>(Sender);
	if (!actor) {
		return;
	}

	if (actor->Modified[IE_DISABLEDBUTTON] & (1 << ACT_TURN)) {
		return;
	}

	int skill = actor->GetStat(IE_TURNUNDEADLEVEL);
	if (skill < 1) return;

	actor->SetModal(Modal::TurnUndead);
}

void GameScript::TurnAMT(Scriptable* Sender, Action* parameters)
{
	Actor* actor = Scriptable::As<Actor>(Sender);
	if (!actor) {
		Sender->ReleaseCurrentAction();
		return;
	}
	actor->SetOrientation(NextOrientation(actor->GetOrientation(), parameters->int0Parameter), true);
	actor->SetWait(1);
	Sender->ReleaseCurrentAction(); // todo, blocking?
}

void GameScript::RandomTurn(Scriptable* Sender, Action* parameters)
{
	Actor* actor = Scriptable::As<Actor>(Sender);
	if (!actor) {
		Sender->ReleaseCurrentAction();
		return;
	}
	// it doesn't take parameters, but we used them internally for one-shot runs
	if (parameters->int0Parameter > 1) parameters->int0Parameter--;
	if (parameters->int0Parameter == 1) {
		Sender->ReleaseCurrentAction();
		return;
	}

	actor->SetOrientation(RandomOrientation(), true);
	// the original waited more if the actor was offscreen, perhaps as an optimization
	int diceSides = 40;
	Region vp = core->GetGameControl()->Viewport();
	if (vp.PointInside(actor->Pos)) diceSides = 10;
	actor->SetWait(core->Time.defaultTicksPerSec * core->Roll(1, diceSides, 0));
}

void GameScript::AttachTransitionToDoor(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetScriptableFromObject(Sender, parameters);
	Door* door = Scriptable::As<Door>(tar);
	if (!door) {
		return;
	}

	door->LinkedInfo = MakeVariable(parameters->variable0Parameter);
}

/*getting a handle of a temporary actor resource to copy its selected attributes*/
void GameScript::ChangeAnimation(Scriptable* Sender, Action* parameters)
{
	Actor* actor = Scriptable::As<Actor>(Sender);
	if (!actor) {
		return;
	}
	ChangeAnimationCore(actor, parameters->resref0Parameter, true);
}

void GameScript::ChangeAnimationNoEffect(Scriptable* Sender, Action* parameters)
{
	Actor* actor = Scriptable::As<Actor>(Sender);
	if (!actor) {
		return;
	}
	ChangeAnimationCore(actor, parameters->resref0Parameter, false);
}

void GameScript::Polymorph(Scriptable* Sender, Action* parameters)
{
	Actor* act = Scriptable::As<Actor>(Sender);
	if (!act) {
		return;
	}
	act->SetBase(IE_ANIMATION_ID, parameters->int0Parameter);
}

void GameScript::PolymorphCopy(Scriptable* Sender, Action* parameters)
{
	Actor* actor = Scriptable::As<Actor>(Sender);
	if (!actor) {
		return;
	}
	const Scriptable* tar = GetScriptableFromObject(Sender, parameters);
	const Actor* target = Scriptable::As<Actor>(tar);
	if (!target) {
		return;
	}
	PolymorphCopyCore(target, actor);
}

/* according to IESDP this only copies the animation ID */
void GameScript::PolymorphCopyBase(Scriptable* Sender, Action* parameters)
{
	Actor* act = Scriptable::As<Actor>(Sender);
	if (!act) {
		return;
	}
	const Scriptable* tar = GetScriptableFromObject(Sender, parameters);
	const Actor* actor = Scriptable::As<Actor>(tar);
	if (!actor) {
		return;
	}

	act->SetBase(IE_ANIMATION_ID, actor->GetBase(IE_ANIMATION_ID));
}

void GameScript::ExportParty(Scriptable* /*Sender*/, Action* parameters)
{
	const Game* game = core->GetGame();
	int i = game->GetPartySize(false);
	while (i--) {
		const Actor* actor = game->GetPC(i, false);
		std::string fname = fmt::format("{}{}", parameters->string0Parameter, i + 1);
		core->WriteCharacter(fname, actor);
	}
	displaymsg->DisplayConstantString(HCStrings::Exported, GUIColors::XPCHANGE);
}

void GameScript::SaveGame(Scriptable* /*Sender*/, Action* parameters)
{
	if (core->HasFeature(GFFlags::STRREF_SAVEGAME)) {
		String basename = u"Auto-Save";
		AutoTable tab = gamedata->LoadTable("savegame");
		if (tab) {
			basename = StringFromTLK(tab->QueryDefault());
		}
		String str = core->GetString(ieStrRef(parameters->int0Parameter), STRING_FLAGS::STRREFOFF);
		String FolderName = fmt::format(u"{} - {}", basename, str);
		auto saveGame = core->GetSaveGameIterator()->GetSaveGame(FolderName);
		core->GetSaveGameIterator()->CreateSaveGame(std::move(saveGame), FolderName);
	} else {
		core->GetSaveGameIterator()->CreateSaveGame(parameters->int0Parameter);
	}
}

static bool FindEscapePosition(Point& pos, const Scriptable* escapee)
{
	const Map* map = escapee->GetCurrentArea();
	if (!map) {
		return false;
	}

	const auto found = map->TMap->AdjustNearestTravel(pos);
	Region vp = core->GetGameControl()->Viewport();
	if (!found && vp.PointInside(pos)) {
		// no travel region found, so look for alternative
		// we don't need to bother if the actor isn't visible — immediate selfdestruction is fine
		// the obvious direction would be to the closest map's edge, but that's not what the originals did
		// it's on a timer, so we can simplify and direct to viewport edge (or only if it's closer)
		// ... but even this simpler approach works good enough
		pos = vp.Intercept(pos);
	}
	return true;
}

/*EscapeAreaMove(S:Area*,I:X*,I:Y*,I:Face*)*/
void GameScript::EscapeArea(Scriptable* Sender, Action* parameters)
{
	ScriptDebugLog(DebugMode::ACTIONS, "EscapeArea/EscapeAreaMove");

	if (Sender->Type != ST_ACTOR) {
		Sender->ReleaseCurrentAction();
		return;
	}

	Point p = Sender->Pos;
	if (!FindEscapePosition(p, Sender)) {
		Sender->ReleaseCurrentAction();
		return;
	}

	if (!parameters->resref0Parameter.IsEmpty()) {
		Point q(parameters->int0Parameter, parameters->int1Parameter);
		EscapeAreaCore(Sender, p, parameters->resref0Parameter, q, EscapeArea::None, parameters->int2Parameter);
	} else {
		EscapeAreaCore(Sender, p, parameters->resref0Parameter, p, EscapeArea::Destroy, parameters->int0Parameter);
	}
	//EscapeAreaCore will do its ReleaseCurrentAction
	//Sender->ReleaseCurrentAction();
}

void GameScript::EscapeAreaNoSee(Scriptable* Sender, Action* parameters)
{
	ScriptDebugLog(DebugMode::ACTIONS, "EscapeAreaNoSee");

	if (Sender->Type != ST_ACTOR) {
		Sender->ReleaseCurrentAction();
		return;
	}

	Point p = Sender->Pos;
	if (!FindEscapePosition(p, Sender)) {
		Sender->ReleaseCurrentAction();
		return;
	}

	if (!parameters->resref0Parameter.IsEmpty()) {
		Point q(parameters->int0Parameter, parameters->int1Parameter);
		EscapeAreaCore(Sender, p, parameters->resref0Parameter, q, EscapeArea::None, parameters->int2Parameter);
	} else {
		EscapeAreaCore(Sender, p, parameters->resref0Parameter, p, EscapeArea::DestroyNoSee, parameters->int0Parameter);
	}
	//EscapeAreaCore will do its ReleaseCurrentAction
}

void GameScript::EscapeAreaDestroy(Scriptable* Sender, Action* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		Sender->ReleaseCurrentAction();
		return;
	}

	Point p = Sender->Pos;
	if (!FindEscapePosition(p, Sender)) {
		Sender->ReleaseCurrentAction();
		return;
	}

	//EscapeAreaCore will do its ReleaseCurrentAction
	EscapeAreaCore(Sender, p, parameters->resref0Parameter, p, EscapeArea::Destroy, parameters->int0Parameter);
}

/*EscapeAreaObjectMove(S:Area*,I:X*,I:Y*,I:Face*)*/
void GameScript::EscapeAreaObject(Scriptable* Sender, Action* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		Sender->ReleaseCurrentAction();
		return;
	}
	const Map* map = Sender->GetCurrentArea();
	if (!map) {
		Sender->ReleaseCurrentAction();
		return;
	}

	const Scriptable* tar = GetScriptableFromObject(Sender, parameters);
	if (!tar) {
		Sender->ReleaseCurrentAction();
		return;
	}
	Point p = tar->Pos;
	if (!parameters->resref0Parameter.IsEmpty()) {
		Point q(parameters->int0Parameter, parameters->int1Parameter);
		EscapeAreaCore(Sender, p, parameters->resref0Parameter, q, EscapeArea::None, parameters->int2Parameter);
	} else {
		EscapeAreaCore(Sender, p, {}, p, EscapeArea::Destroy, parameters->int0Parameter);
	}
	//EscapeAreaCore will do its ReleaseCurrentAction
}

//This one doesn't require the object to be seen?
//We don't have that feature yet, so this is the same as EscapeAreaObject
void GameScript::EscapeAreaObjectNoSee(Scriptable* Sender, Action* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		Sender->ReleaseCurrentAction();
		return;
	}
	const Map* map = Sender->GetCurrentArea();
	if (!map) {
		Sender->ReleaseCurrentAction();
		return;
	}

	const Scriptable* tar = GetScriptableFromObject(Sender, parameters);
	if (!tar) {
		Sender->ReleaseCurrentAction();
		return;
	}
	Point p = tar->Pos;
	Sender->SetWait(parameters->int0Parameter);
	if (!parameters->resref0Parameter.IsEmpty()) {
		Point q(parameters->int0Parameter, parameters->int1Parameter);
		EscapeAreaCore(Sender, p, parameters->resref0Parameter, q, EscapeArea::None, parameters->int2Parameter);
	} else {
		EscapeAreaCore(Sender, p, {}, p, EscapeArea::DestroyNoSee, parameters->int0Parameter);
	}
	//EscapeAreaCore will do its ReleaseCurrentAction
}

//takes first fitting item from container at feet, doesn't seem to be working in the original engines
void GameScript::PickUpItem(Scriptable* Sender, Action* parameters)
{
	Actor* scr = Scriptable::As<Actor>(Sender);
	if (!scr) {
		return;
	}
	Map* map = scr->GetCurrentArea();
	Container* c = map->GetPile(scr->Pos);
	if (!c) { //this shouldn't happen, but lets prepare for the worst
		return;
	}

	// the following part is coming from GUIScript.cpp with trivial changes
	int Slot = c->inventory.FindItem(parameters->resref0Parameter, 0);
	if (Slot < 0) {
		return;
	}
	int res = core->CanMoveItem(c->inventory.GetSlotItem(Slot));
	if (!res) { //cannot move
		return;
	}
	CREItem* item = c->RemoveItem(Slot, 0);
	if (!item) {
		return;
	}
	if (res != -1) { // it is gold and we got the party pool!
		if (scr->IsPartyMember()) {
			core->GetGame()->PartyGold += res;
			// if you want message here then use core->GetGame()->AddGold(res);
		} else {
			scr->SetBase(IE_GOLD, scr->GetBase(IE_GOLD) + res);
		}
		delete item;
		return;
	}

	Actor* receiver = scr;
	if (scr->GetBase(IE_EA) == EA_FAMILIAR) {
		receiver = core->GetGame()->FindPC(1);
	}
	res = receiver->inventory.AddSlotItem(item, SLOT_ONLYINVENTORY);
	if (res != ASI_SUCCESS) { //putting it back
		c->AddItem(item);
	}
	return;
}

void GameScript::ChangeStoreMarkup(Scriptable* /*Sender*/, Action* parameters)
{
	bool has_current = false;
	ResRef current;
	ieDword owner;

	Store* store = core->GetCurrentStore();
	if (!store) {
		store = core->SetCurrentStore(parameters->resref0Parameter, 0);
	} else {
		if (store->Name != parameters->resref0Parameter) {
			//not the current store, we need some dirty hack
			has_current = true;
			current = store->Name;
			owner = store->GetOwnerID();
			store = core->SetCurrentStore(parameters->resref0Parameter, 0);
		}
	}
	store->SellMarkup = parameters->int0Parameter;
	store->BuyMarkup = parameters->int1Parameter;
	// additional markup, is this depreciation? Yes
	store->DepreciationRate = parameters->int2Parameter;
	core->CloseCurrentStore(); // will also save it
	if (has_current) {
		//setting back old store (this will save our current store)
		core->SetCurrentStore(current, owner);
	}
}

void GameScript::SetEncounterProbability(Scriptable* /*Sender*/, Action* parameters)
{
	WorldMap* wmap = core->GetWorldMap(parameters->resref0Parameter);
	if (!wmap) {
		//no such starting area
		return;
	}
	WMPAreaLink* link = wmap->GetLink(parameters->resref0Parameter, parameters->resref1Parameter);
	if (!link) {
		return;
	}
	link->EncounterChance = parameters->int0Parameter;
}

void GameScript::SpawnPtActivate(Scriptable* Sender, Action* parameters)
{
	if (parameters->objects[1]) {
		const Map* map = Sender->GetCurrentArea();
		Spawn* spawn = map->GetSpawn(parameters->objects[1]->objectName);
		if (spawn) {
			spawn->Enabled = 1;
		}
	}
}

void GameScript::SpawnPtDeactivate(Scriptable* Sender, Action* parameters)
{
	if (parameters->objects[1]) {
		const Map* map = Sender->GetCurrentArea();
		Spawn* spawn = map->GetSpawn(parameters->objects[1]->objectName);
		if (spawn) {
			spawn->Enabled = 0;
		}
	}
}

void GameScript::SpawnPtSpawn(Scriptable* Sender, Action* parameters)
{
	if (parameters->objects[1]) {
		Map* map = Sender->GetCurrentArea();
		Spawn* spawn = map->GetSpawn(parameters->objects[1]->objectName);
		if (spawn) {
			spawn->Enabled = 1; //??? maybe use an unconditionality flag
			map->TriggerSpawn(spawn);
		}
	}
}

void GameScript::ApplySpell(Scriptable* Sender, Action* parameters)
{
	ResRef spellRes;
	if (!ResolveSpellName(spellRes, parameters)) {
		return;
	}

	Scriptable* tar = GetScriptableFromObject(Sender, parameters);
	if (!tar) {
		return;
	}
	if (tar->Type == ST_ACTOR) {
		//apply spell on target
		core->ApplySpell(spellRes, static_cast<Actor*>(tar), Sender, parameters->int1Parameter);
	} else {
		//apply spell on point
		Point d;
		GetPositionFromScriptable(tar, d, false);
		core->ApplySpellPoint(spellRes, tar->GetCurrentArea(), d, Sender, parameters->int1Parameter);
	}
}

void GameScript::ApplySpellPoint(Scriptable* Sender, Action* parameters)
{
	ResRef spellRes;
	if (!ResolveSpellName(spellRes, parameters)) {
		return;
	}

	core->ApplySpellPoint(spellRes, Sender->GetCurrentArea(), parameters->pointParameter, Sender, parameters->int1Parameter);
}

//this is a gemrb extension
//sets a variable to the stat value
void GameScript::GetStat(Scriptable* Sender, Action* parameters)
{
	ieDword value = 0;

	const Scriptable* tar = GetScriptableFromObject(Sender, parameters);
	const Actor* actor = Scriptable::As<Actor>(tar);
	if (actor) {
		value = actor->GetStat(parameters->int0Parameter);
	}
	SetVariable(Sender, parameters->string0Parameter, value);
}

void GameScript::BreakInstants(Scriptable* Sender, Action* /*parameters*/)
{
	//don't do anything, apparently the point of this action is to
	//delay the execution of further actions to the next AI cycle
	//Sender->SetWait(1);
	Sender->ReleaseCurrentAction(); // this doesn't really need to block
}

//an interesting improvement would be to pause game for a given duration
void GameScript::PauseGame(Scriptable* Sender, Action* /*parameters*/)
{
	GameControl* gc = core->GetGameControl();
	if (gc) {
		gc->SetDialogueFlags(DF_FREEZE_SCRIPTS, BitOp::OR);
		displaymsg->DisplayConstantString(HCStrings::ScriptPaused, GUIColors::RED);
	}
	// releasing this action allows actions to continue executing,
	// so we force a wait
	Sender->SetWait(1);
	Sender->ReleaseCurrentAction(); // does this need to block?
}

void GameScript::SetNoOneOnTrigger(Scriptable* Sender, Action* parameters)
{
	Scriptable* ip;
	ieVariable name = "null";
	if (parameters->objects[1]) {
		name = parameters->objects[1]->objectName;
		ip = Sender->GetCurrentArea()->TMap->GetInfoPoint(name);
	} else {
		ip = Sender;
	}
	if (!ip || (ip->Type != ST_TRIGGER && ip->Type != ST_TRAVEL && ip->Type != ST_PROXIMITY)) {
		Log(WARNING, "Actions", "Script error: No trigger named \"{}\"", name);
		parameters->dump();
		return;
	}

	ip->ClearTriggers();
	// we also need to reset the IF_INTRAP bit for any actors that are inside or subsequent triggers will be skipped
	// there are only two users of this action, so we can be a bit sloppy and skip the geometry checks
	std::vector<Actor*> nearActors = Sender->GetCurrentArea()->GetAllActorsInRadius(ip->Pos, GA_NO_LOS | GA_NO_DEAD | GA_NO_UNSCHEDULED, MAX_OPERATING_DISTANCE);
	for (const auto& candidate : nearActors) {
		candidate->SetInTrap(false);
	}
}

void GameScript::UseDoor(Scriptable* Sender, Action* parameters)
{
	GameControl* gc = core->GetGameControl();
	if (!gc) {
		Sender->ReleaseCurrentAction();
		return;
	}

	gc->ResetTargetMode();
	OpenDoor(Sender, parameters);

	Sender->ReleaseCurrentAction(); // this is blocking, OpenDoor is not
}

//this will force bashing the door, if bend bars check is successful,
//it will unlock the door and sets the broken flag
void GameScript::BashDoor(Scriptable* Sender, Action* parameters)
{
	GameControl* gc = core->GetGameControl();
	if (!gc) {
		Sender->ReleaseCurrentAction();
		return;
	}
	Actor* actor = Scriptable::As<Actor>(Sender);
	if (!actor) {
		Sender->ReleaseCurrentAction();
		return;
	}

	Scriptable* target = GetScriptableFromObject(Sender, parameters);
	if (!target) {
		Sender->ReleaseCurrentAction();
		return;
	}

	const Point* pos;
	unsigned int distance;
	if (target->Type == ST_DOOR) {
		const Door* door = static_cast<Door*>(target);
		pos = door->GetClosestApproach(Sender, distance);
	} else if (target->Type == ST_CONTAINER) {
		pos = &target->Pos;
		distance = PersonalDistance(*pos, Sender);
	} else {
		Sender->ReleaseCurrentAction();
		return;
	}

	if (distance > MAX_OPERATING_DISTANCE) {
		MoveNearerTo(Sender, *pos, MAX_OPERATING_DISTANCE, 0);
		return;
	}

	//bashing makes the actor visible
	actor->CureInvisibility();
	gc->SetTargetMode(TargetMode::Attack); // for bashing doors too

	// try to bash it
	static_cast<Highlightable*>(target)->TryBashLock(actor);

	Sender->ReleaseCurrentAction();
}

//pst action
void GameScript::ActivatePortalCursor(Scriptable* Sender, Action* parameters)
{
	Scriptable* ip;
	if (!parameters->objects[1]) {
		ip = Sender;
	} else {
		ip = Sender->GetCurrentArea()->TMap->GetInfoPoint(parameters->objects[1]->objectNameVar);
	}
	if (!ip) {
		return;
	}
	if (ip->Type != ST_PROXIMITY && ip->Type != ST_TRAVEL) {
		return;
	}
	InfoPoint* tar = static_cast<InfoPoint*>(ip);
	if (parameters->int0Parameter) {
		tar->Trapped |= PORTAL_CURSOR;
	} else {
		tar->Trapped &= ~PORTAL_CURSOR;
	}
}

//pst action
void GameScript::EnablePortalTravel(Scriptable* Sender, Action* parameters)
{
	Scriptable* ip;
	if (!parameters->objects[1]) {
		ip = Sender;
	} else {
		ip = Sender->GetCurrentArea()->TMap->GetInfoPoint(parameters->objects[1]->objectNameVar);
	}
	if (!ip) {
		return;
	}
	if (ip->Type != ST_PROXIMITY && ip->Type != ST_TRAVEL) {
		return;
	}
	InfoPoint* tar = static_cast<InfoPoint*>(ip);
	if (parameters->int0Parameter) {
		tar->Trapped |= PORTAL_TRAVEL;
	} else {
		tar->Trapped &= ~PORTAL_TRAVEL;
	}
}

//unhardcoded iwd action (for the forge entrance change)
void GameScript::ChangeDestination(Scriptable* Sender, Action* parameters)
{
	InfoPoint* ip = Sender->GetCurrentArea()->TMap->GetInfoPoint(parameters->objects[1]->objectNameVar);
	if (ip && (ip->Type == ST_TRAVEL)) {
		//alter the destination area, don't touch the entrance variable link
		ip->Destination = parameters->resref0Parameter;
	}
}

void GameScript::MoveCursorPoint(Scriptable* /*Sender*/, Action* /*parameters*/)
{
	// according to IESDP this does nothing...
	// the other cursor actions we implement affect only GameControl
	// in GemRB you wouldnt need to move the mouse before scripting a click etc, so this is probably not needed.
}

// false means no talk
void GameScript::DialogueInterrupt(Scriptable* Sender, Action* parameters)
{
	Actor* actor = Scriptable::As<Actor>(Sender);
	if (!actor) {
		return;
	}
	if (parameters->int0Parameter != 0) {
		actor->SetMCFlag(MC_NO_TALK, BitOp::NAND);
	} else {
		actor->SetMCFlag(MC_NO_TALK, BitOp::OR);
	}
}

void GameScript::EquipMostDamagingMelee(Scriptable* Sender, Action* /*parameters*/)
{
	Actor* actor = Scriptable::As<Actor>(Sender);
	if (!actor) {
		return;
	}
	actor->inventory.EquipBestWeapon(EQUIP_MELEE);
}

void GameScript::EquipRanged(Scriptable* Sender, Action* /*parameters*/)
{
	Actor* actor = Scriptable::As<Actor>(Sender);
	if (!actor) {
		return;
	}
	actor->inventory.EquipBestWeapon(EQUIP_RANGED);
}

//will equip best weapon regardless of range considerations
void GameScript::EquipWeapon(Scriptable* Sender, Action* /*parameters*/)
{
	Actor* actor = Scriptable::As<Actor>(Sender);
	if (!actor) {
		return;
	}
	actor->inventory.EquipBestWeapon(EQUIP_MELEE | EQUIP_RANGED);
}

void GameScript::SetBestWeapon(Scriptable* Sender, Action* parameters)
{
	Actor* actor = Scriptable::As<Actor>(Sender);
	if (!actor) {
		return;
	}

	const Scriptable* tar = GetScriptableFromObject(Sender, parameters);
	const Actor* target = Scriptable::As<Actor>(tar);
	if (!target) {
		return;
	}

	if (PersonalDistance(actor, target) > (unsigned int) parameters->int0Parameter) {
		actor->inventory.EquipBestWeapon(EQUIP_RANGED);
	} else {
		actor->inventory.EquipBestWeapon(EQUIP_MELEE);
	}
}

void GameScript::FakeEffectExpiryCheck(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetScriptableFromObject(Sender, parameters);
	Actor* target = Scriptable::As<Actor>(tar);
	if (!target) {
		return;
	}

	target->fxqueue.RemoveExpiredEffects(parameters->int0Parameter * core->Time.defaultTicksPerSec);
}

void GameScript::SetInterrupt(Scriptable* Sender, Action* parameters)
{
	if (parameters->int0Parameter) {
		Sender->Interrupt();
	} else {
		Sender->NoInterrupt();
	}
}

void GameScript::SelectWeaponAbility(Scriptable* Sender, Action* parameters)
{
	Actor* scr = Scriptable::As<Actor>(Sender);
	if (!scr) {
		return;
	}

	// weapon
	int slot = parameters->int0Parameter;
	if (core->QuerySlotType(slot) & SLOT_WEAPON) {
		slot = Inventory::GetWeaponQuickSlot(slot);
		if (slot < 0 || slot >= MAX_QUICKWEAPONSLOT) {
			return;
		}
		// nothing to do? Then don't disrupt by reequipping
		if (slot == scr->inventory.GetEquipped() && parameters->int1Parameter == scr->inventory.GetEquippedHeader()) return;

		scr->SetEquippedQuickSlot(slot, parameters->int1Parameter);
		core->SetEventFlag(EF_ACTION);
		return;
	}

	//quick item
	if (core->QuerySlotType(slot) & SLOT_ITEM) {
		slot -= Inventory::GetQuickSlot();
		if (slot < 0 || slot >= MAX_QUICKITEMSLOT) {
			return;
		}
		if (scr->PCStats) {
			scr->PCStats->QuickItemHeaders[slot] = (ieWord) parameters->int1Parameter;
		}
	}
}

void GameScript::UseItem(Scriptable* Sender, Action* parameters)
{
	Actor* act = Scriptable::As<Actor>(Sender);
	if (!act) {
		Sender->ReleaseCurrentAction();
		return;
	}
	const Scriptable* tar = GetStoredActorFromObject(Sender, parameters);
	const Actor* target = Scriptable::As<const Actor>(tar);
	if (!tar) {
		Sender->ReleaseCurrentAction();
		return;
	}
	int Slot;
	int header;
	ieDword flags = 0; // aura pollution is on for everyone
	ResRef itemres;

	// cater to all 4 signatures:
	//   UseItem(S:Object*,O:Target*)
	//   UseItemAbility(S:Object*,O:Target*,I:Slot*,I:Ability*)
	//   UseItemSlot(O:Target*,I:Slot*Slots)
	//   UseItemSlotAbility(O:Target*,I:Slot*Slots,I:Ability*)
	if (!parameters->resref0Parameter.IsEmpty()) {
		Slot = act->inventory.FindItem(parameters->resref0Parameter, IE_INV_ITEM_UNDROPPABLE);
		// UseItemAbility passes both the resref and slot, so we need to be stricter to deplete the right item
		if (parameters->int0Parameter) {
			int skip = 0;
			while (Slot != -1 && Slot != parameters->int0Parameter) {
				Slot = act->inventory.FindItem(parameters->resref0Parameter, IE_INV_ITEM_UNDROPPABLE, ++skip);
			}
		}
		//this IS in the original game code (ability)
		header = parameters->int0Parameter;
		flags |= parameters->int1Parameter;
	} else {
		Slot = parameters->int0Parameter;
		//this is actually not in the original game code
		header = parameters->int1Parameter;
		flags |= parameters->int2Parameter;
	}

	if (Slot == -1) {
		Sender->ReleaseCurrentAction();
		return;
	}

	if (!ResolveItemName(itemres, act, Slot)) {
		Sender->ReleaseCurrentAction();
		return;
	}

	// make sure we can still see the target
	const Item* itm = gamedata->GetItem(itemres, true);
	if (Sender != tar && !(itm->Flags & IE_ITEM_NO_INVIS) && target && target->IsInvisibleTo(Sender)) {
		Sender->ReleaseCurrentAction();
		Sender->AddTrigger(TriggerEntry(trigger_targetunreachable, tar->GetGlobalID()));
		displaymsg->DisplayConstantStringName(HCStrings::NoSeeNoCast, GUIColors::RED, Sender);
		core->Autopause(AUTOPAUSE::NOTARGET, Sender);
		gamedata->FreeItem(itm, itemres, false);
		return;
	}
	const ITMExtHeader* hh = itm->GetExtHeader(header);
	if (!hh) {
		Sender->ReleaseCurrentAction();
		return;
	}
	bool inactive = (tar->GetInternalFlag() & (IF_ACTIVE | IF_VISIBLE)) != (IF_ACTIVE | IF_VISIBLE);
	bool removed = target && target->GetStat(IE_AVATARREMOVAL);
	if (hh->Target == TARGET_DEAD) {
		if (!target || target->ValidTarget(GA_NO_DEAD) ||
		    (target->InParty == 0 && (inactive || removed)) ||
		    (target->InParty && removed)) {
			Sender->ReleaseCurrentAction();
			return;
		}
	} else {
		if ((target && target->GetStat(IE_AVATARREMOVAL)) || inactive) {
			Sender->ReleaseCurrentAction();
			return;
		}
	}
	gamedata->FreeItem(itm, itemres, false);

	float_t angle = AngleFromPoints(Sender->Pos, tar->Pos);
	unsigned int dist = GetItemDistance(itemres, header, angle);
	if (PersonalDistance(Sender, tar) > dist) {
		MoveNearerTo(Sender, tar, dist);
		return;
	}

	// only one use per round; skip for our internal attack projectile
	if (!(flags & UI_NOAURA) && act->AuraPolluted()) {
		return;
	}

	Sender->ReleaseCurrentAction();
	act->UseItem(Slot, header, tar, flags);
}

void GameScript::UseItemPoint(Scriptable* Sender, Action* parameters)
{
	Actor* act = Scriptable::As<Actor>(Sender);
	if (!act) {
		Sender->ReleaseCurrentAction();
		return;
	}
	int Slot;
	int header;
	ResRef itemres;
	ieDword flags;

	if (!parameters->resref0Parameter.IsEmpty()) {
		Slot = act->inventory.FindItem(parameters->resref0Parameter, 0);
		//this IS in the original game code (ability)
		header = parameters->int0Parameter;
		flags = parameters->int1Parameter;
	} else {
		Slot = parameters->int0Parameter;
		//this is actually not in the original game code
		header = parameters->int1Parameter;
		flags = parameters->int2Parameter;
	}

	if (Slot == -1) {
		Sender->ReleaseCurrentAction();
		return;
	}

	if (!ResolveItemName(itemres, act, Slot)) {
		Sender->ReleaseCurrentAction();
		return;
	}

	float_t angle = AngleFromPoints(Sender->Pos, parameters->pointParameter);
	unsigned int dist = GetItemDistance(itemres, header, angle);
	if (PersonalDistance(parameters->pointParameter, Sender) > dist) {
		MoveNearerTo(Sender, parameters->pointParameter, dist, 0);
		return;
	}

	// only one use per round; skip for our internal attack projectile
	if (!(flags & UI_NOAURA) && act->AuraPolluted()) {
		return;
	}

	Point targetPos = parameters->pointParameter;

	Sender->ReleaseCurrentAction();
	act->UseItemPoint(Slot, header, targetPos, flags);
}

//addfeat will be able to remove feats too
//(the second int parameter is a value to add to the feat)
void GameScript::AddFeat(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetScriptableFromObject(Sender, parameters);
	Actor* actor = Scriptable::As<Actor>(tar);
	if (!actor) {
		return;
	}
	//value to add to the feat
	int value = parameters->int1Parameter;
	//default is increase by 1
	if (!value) value = 1;
	Feat feat = static_cast<Feat>(parameters->int0Parameter);
	value += actor->GetFeat(feat);
	//SetFeatValue will handle edges
	actor->SetFeatValue(feat, value);
}

void GameScript::MatchHP(Scriptable* Sender, Action* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		return;
	}
	const Actor* scr = (const Actor*) Sender;
	Scriptable* tar = GetScriptableFromObject(Sender, parameters);
	Actor* actor = Scriptable::As<Actor>(tar);
	if (!actor) {
		return;
	}
	switch (parameters->int0Parameter) {
		case 1: //sadly the hpflags are not the same as stats
			actor->SetBase(IE_HITPOINTS, scr->GetBase(IE_HITPOINTS));
			break;
		case 0:
			actor->SetBase(IE_MAXHITPOINTS, scr->GetBase(IE_MAXHITPOINTS));
			break;
		default: //this is gemrb extension
			actor->SetBase(parameters->int0Parameter, scr->GetBase(parameters->int0Parameter));
			break;
	}
}

void GameScript::ChangeColor(Scriptable* Sender, Action* parameters)
{
	Actor* scr = Scriptable::As<Actor>(Sender);
	if (!scr) {
		return;
	}
	ieDword stat = parameters->int0Parameter; // clownrge.ids
	if (stat < 9 || stat > 14) {
		return;
	}
	stat += IE_COLORS - 9;
	scr->SetBase(stat, (scr->GetBase(stat) & ~255) | (parameters->int1Parameter & 255)); // clownclr.ids
}

//removes previous kit, adds new
void GameScript::AddKit(Scriptable* Sender, Action* parameters)
{
	Actor* scr = Scriptable::As<Actor>(Sender);
	if (!scr) {
		return;
	}
	//remove previous kit stuff
	scr->ApplyKit(true);
	//this adds the current level abilities
	scr->SetBase(IE_KIT, parameters->int0Parameter);
	scr->ApplyKit(false);
}

//doesn't remove old kit
void GameScript::AddSuperKit(Scriptable* Sender, Action* parameters)
{
	Actor* scr = Scriptable::As<Actor>(Sender);
	if (!scr) {
		return;
	}
	scr->SetBase(IE_KIT, parameters->int0Parameter);
	scr->ApplyKit(false);
}

void GameScript::SetSelection(Scriptable* /*Sender*/, Action* parameters)
{
	GameControl* gc = core->GetGameControl();
	if (!gc) {
		return;
	}
	gc->SelectActor(parameters->int0Parameter, parameters->int1Parameter);
}

//this action is weird in the original game, because it overwrites ALL
//IDS stats.
//in this version, if a stat is set to 0, it won't change
//it will alter only the main IDS stats
// (unused in the originals, but if that changes, make sure all ids files are handled; eg. see iwd2 script.2da)
void GameScript::ChangeAIType(Scriptable* Sender, Action* parameters)
{
	Actor* scr = Scriptable::As<Actor>(Sender);
	if (!scr) {
		return;
	}
	const Object* ob = parameters->objects[1];
	if (!ob) {
		return;
	}

	for (int i = 0; i < MAX_OBJECT_FIELDS; i++) {
		int val = ob->objectFields[i];
		if (!val) continue;
		if (ObjectIDSTableNames[i] == "ea") {
			scr->SetBase(IE_EA, val);
			continue;
		}
		if (ObjectIDSTableNames[i] == "general") {
			scr->SetBase(IE_GENERAL, val);
			continue;
		}
		if (ObjectIDSTableNames[i] == "race") {
			scr->SetBase(IE_RACE, val);
			continue;
		}
		if (ObjectIDSTableNames[i] == "class") {
			scr->SetBase(IE_CLASS, val);
			continue;
		}
		if (ObjectIDSTableNames[i] == "gender") {
			scr->SetBase(IE_SEX, val);
			continue;
		}
		if (ObjectIDSTableNames[i] == "specific") {
			scr->SetBase(IE_SPECIFIC, val);
			continue;
		}
		if (ObjectIDSTableNames[i] == "align") {
			scr->SetBase(IE_ALIGNMENT, val);
			continue;
		}
	}
}

//same as MoveToPoint, but not blocking
void GameScript::Leader(Scriptable* Sender, Action* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		return;
	}

	std::string tmp = fmt::format("MoveToPoint([{}.{}])", parameters->pointParameter.x, parameters->pointParameter.y);
	Action* newact = GenerateAction(std::move(tmp));
	Sender->AddAction(newact);
}

//same as MoveToPointNoRecticle, but not blocking
void GameScript::Follow(Scriptable* Sender, Action* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		return;
	}

	std::string tmp = fmt::format("MoveToPointNoRecticle([{}.{}])", parameters->pointParameter.x, parameters->pointParameter.y);
	Action* newact = GenerateAction(std::move(tmp));
	Sender->AddAction(newact);
}

void GameScript::FollowCreature(Scriptable* Sender, Action* parameters)
{
	Actor* scr = Scriptable::As<Actor>(Sender);
	if (!scr) {
		Sender->ReleaseCurrentAction();
		return;
	}

	const Scriptable* tar = GetStoredActorFromObject(Sender, parameters);
	const Actor* actor = Scriptable::As<Actor>(tar);
	if (!actor) {
		Sender->ReleaseCurrentAction();
		return;
	}

	scr->objects.LastFollowed = actor->GetGlobalID();
	scr->FollowOffset.Invalidate();
	if (!scr->InMove() || scr->Destination != actor->Pos) {
		scr->WalkTo(actor->Pos, 0, 1);
	}
}

void GameScript::RunFollow(Scriptable* Sender, Action* parameters)
{
	Actor* scr = Scriptable::As<Actor>(Sender);
	if (!scr) {
		Sender->ReleaseCurrentAction();
		return;
	}

	const Scriptable* tar = GetStoredActorFromObject(Sender, parameters);
	const Actor* actor = Scriptable::As<Actor>(tar);
	if (!actor) {
		Sender->ReleaseCurrentAction();
		return;
	}

	scr->objects.LastFollowed = actor->GetGlobalID();
	scr->FollowOffset.Invalidate();
	if (!scr->InMove() || scr->Destination != actor->Pos) {
		scr->WalkTo(actor->Pos, IF_RUNNING, 1);
	}
}

void GameScript::ProtectPoint(Scriptable* Sender, Action* parameters)
{
	Actor* scr = Scriptable::As<Actor>(Sender);
	if (!scr) {
		Sender->ReleaseCurrentAction();
		return;
	}

	if (!scr->InMove() || scr->Destination != parameters->pointParameter) {
		scr->WalkTo(parameters->pointParameter, 0, 1);
	}
	// we should handle 'Protect' here rather than just unblocking
	Sender->ReleaseCurrentAction();
}

void GameScript::ProtectObject(Scriptable* Sender, Action* parameters)
{
	Actor* scr = Scriptable::As<Actor>(Sender);
	if (!scr) {
		Sender->ReleaseCurrentAction();
		return;
	}

	Scriptable* tar = GetStoredActorFromObject(Sender, parameters);
	Actor* actor = Scriptable::As<Actor>(tar);
	if (!actor) {
		Sender->ReleaseCurrentAction();
		return;
	}

	scr->objects.LastFollowed = actor->GetGlobalID();
	scr->objects.LastProtectee = actor->GetGlobalID();
	actor->objects.LastProtector = scr->GetGlobalID();
	//not exactly range
	scr->FollowOffset.x = parameters->int0Parameter;
	scr->FollowOffset.y = parameters->int0Parameter;
	if (!scr->InMove() || scr->Destination != tar->Pos) {
		scr->WalkTo(tar->Pos, 0, MAX_OPERATING_DISTANCE);
	}
	// we should handle 'Protect' here rather than just unblocking
	Sender->ReleaseCurrentAction();
}

//keeps following the object in formation
void GameScript::FollowObjectFormation(Scriptable* Sender, Action* parameters)
{
	const GameControl* gc = core->GetGameControl();
	if (!gc) {
		Sender->ReleaseCurrentAction();
		return;
	}
	Actor* scr = Scriptable::As<Actor>(Sender);
	if (!scr) {
		Sender->ReleaseCurrentAction();
		return;
	}

	const Scriptable* tar = GetStoredActorFromObject(Sender, parameters);
	const Actor* actor = Scriptable::As<Actor>(tar);
	if (!actor) {
		Sender->ReleaseCurrentAction();
		return;
	}

	scr->objects.LastFollowed = actor->GetGlobalID();
	ieDword formation = parameters->int0Parameter;
	ieDword pos = parameters->int1Parameter;
	scr->FollowOffset = gc->GetFormationOffset(formation, pos);
	if (!scr->InMove() || scr->Destination != tar->Pos) {
		scr->WalkTo(tar->Pos, 0, 1);
	}
	Sender->ReleaseCurrentAction();
}

//walks to a specific offset of target (quite like movetoobject)
void GameScript::Formation(Scriptable* Sender, Action* parameters)
{
	const GameControl* gc = core->GetGameControl();
	if (!gc) {
		Sender->ReleaseCurrentAction();
		return;
	}
	Actor* scr = Scriptable::As<Actor>(Sender);
	if (!scr) {
		Sender->ReleaseCurrentAction();
		return;
	}
	const Scriptable* tar = GetStoredActorFromObject(Sender, parameters);
	if (!tar) {
		Sender->ReleaseCurrentAction();
		return;
	}

	ieDword formation = parameters->int0Parameter;
	ieDword pos = parameters->int1Parameter;
	Point FollowOffset = gc->GetFormationOffset(formation, pos) + tar->Pos;
	if (!scr->InMove() || scr->Destination != FollowOffset) {
		scr->WalkTo(FollowOffset, 0, 1);
	}
}

void GameScript::TransformItem(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetScriptableFromObject(Sender, parameters);
	if (!tar || tar->Type != ST_ACTOR) {
		return;
	}
	TransformItemCore((Actor*) tar, parameters, true);
}

void GameScript::TransformPartyItem(Scriptable* /*Sender*/, Action* parameters)
{
	const Game* game = core->GetGame();
	int i = game->GetPartySize(false);
	while (i--) {
		Actor* tar = game->GetPC(i, false);
		TransformItemCore(tar, parameters, true);
	}
}

void GameScript::TransformItemAll(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetScriptableFromObject(Sender, parameters);
	if (!tar || tar->Type != ST_ACTOR) {
		return;
	}
	TransformItemCore((Actor*) tar, parameters, false);
}

void GameScript::TransformPartyItemAll(Scriptable* /*Sender*/, Action* parameters)
{
	const Game* game = core->GetGame();
	int i = game->GetPartySize(false);
	while (i--) {
		Actor* tar = game->GetPC(i, false);
		TransformItemCore(tar, parameters, false);
	}
}

// pst spawning
void GameScript::GeneratePartyMember(Scriptable* /*Sender*/, Action* parameters)
{
	AutoTable pcs = gamedata->LoadTable("bios");
	if (!pcs) {
		return;
	}
	ieVariable string = pcs->GetRowName(parameters->int0Parameter);
	Actor* actor = core->GetGame()->FindNPC(string);
	if (!actor) {
		return;
	}
	if (!actor->GetCurrentArea()) {
		core->GetGame()->GetCurrentArea()->AddActor(actor, true);
	}
	actor->SetOrientation(ClampToOrientation(parameters->int1Parameter), false);
	actor->MoveTo(parameters->pointParameter);
	actor->Die(nullptr);
	actor->SetBaseBit(IE_STATE_ID, STATE_DEAD, true);
}

void GameScript::EnableFogDither(Scriptable* /*Sender*/, Action* /*parameters*/)
{
	GameControl::DebugFlags |= DEBUG_SHOW_FOG_ALL;
}

void GameScript::DisableFogDither(Scriptable* /*Sender*/, Action* /*parameters*/)
{
	GameControl::DebugFlags &= ~DEBUG_SHOW_FOG_ALL;
}

void GameScript::EnableSpriteDither(Scriptable* /*Sender*/, Action* /*parameters*/)
{
	core->DitherSprites = true;
}

void GameScript::DisableSpriteDither(Scriptable* /*Sender*/, Action* /*parameters*/)
{
	core->DitherSprites = false;
}

void GameScript::FloatRebus(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetScriptableFromObject(Sender, parameters);
	Actor* actor = Scriptable::As<Actor>(tar);
	if (!actor) {
		return;
	}
	//the PST crew apparently loved hardcoding stuff
	static ResRef RebusResRef { "DABUS1" };
	RebusResRef[5] = (char) core->Roll(1, 5, '0');
	ScriptedAnimation* vvc = gamedata->GetScriptedAnimation(RebusResRef, false);
	if (vvc) {
		//setting the height
		vvc->ZOffset = actor->circleSize * 20;
		vvc->PlayOnce();
		//maybe this needs setting up some time
		vvc->SetDefaultDuration(20);
		actor->AddVVCell(vvc);
	}
}

void GameScript::IncrementKillStat(Scriptable* Sender, Action* parameters)
{
	const DataFileMgr* ini = core->GetBeastsINI();
	if (!ini) {
		return;
	}

	const std::string& key = fmt::format("{}", parameters->int0Parameter);
	StringView cstr = ini->GetKeyAsString(key, "killvar");
	if (!cstr) {
		return;
	}
	StringParam variable(cstr);
	ieDword value = CheckVariable(Sender, variable, "GLOBAL") + 1;
	SetVariable(Sender, variable, value, "GLOBAL");
}

static Effect* GetEffect(ieDword opcode)
{
	if (opcode == 0xffffffff) {
		return nullptr;
	}
	Effect* fx = new Effect();
	if (!fx) {
		return nullptr;
	}

	fx->Opcode = opcode;
	return fx;
}

//this action uses the sceffect.ids (which should be covered by our cgtable.2da)
//The original engines solved cg by hardcoding either vvcs or sparkles
//so either we include sparkles as possible CG or just simulate all of these with projectiles
//in any case, this action just creates an opcode (0xeb) and plays sound
static EffectRef fx_iwd_casting_glow_ref = { "CastingGlow2", -1 };

void GameScript::SpellCastEffect(Scriptable* Sender, Action* parameters)
{
	Scriptable* src = GetScriptableFromObject(Sender, parameters);
	Actor* actor = Scriptable::As<Actor>(src);
	if (!actor) {
		return;
	}

	ieDword sparkle = parameters->int0Parameter;

	int opcode = EffectQueue::ResolveEffect(fx_iwd_casting_glow_ref);
	Effect* fx = GetEffect(opcode);
	if (!fx) {
		//invalid effect name didn't resolve to opcode
		return;
	}

	SFXChannel channel = SFXChannel::Dialog;
	if (actor->InParty > 0) {
		channel = SFXChannel(ieByte(SFXChannel::Char0) + actor->InParty - 1);
	} else if (actor->GetStat(IE_EA) >= EA_EVILCUTOFF) {
		channel = SFXChannel::Monster;
	}

	// voice
	core->GetAudioPlayback().Play(parameters->string0Parameter, AudioPreset::SpatialVoice, channel, Sender->Pos);
	// string1Parameter has the starting sound, played at the same time, but on SFXChannel::Casting
	// NOTE: only a few uses have also an ending sound that plays when the effect ends (also stopping Sound1)
	// but we don't even read all three string parameters, as Action stores just two
	// seems like a waste of memory to impose it on everyone, just for these few users
	// (ok, many actions take three, but one is the variable context, so gets merged; here we actually end up with 1 and 3)

	const Actor* caster = Scriptable::As<Actor>(Sender);
	int adjustedDuration = 0;
	if (caster) {
		adjustedDuration = parameters->int1Parameter - caster->GetStat(IE_MENTALSPEED);
		if (adjustedDuration < 0) adjustedDuration = 0;
	}
	adjustedDuration *= 10; // it's really not core->Time.defaultTicksPerSec

	// tell the effect to not use the main casting glow
	fx->Parameter4 = 1;
	fx->ProbabilityRangeMax = 100;
	fx->ProbabilityRangeMin = 0;
	fx->Parameter2 = sparkle; //animation type
	fx->TimingMode = FX_DURATION_INSTANT_LIMITED_TICKS;
	fx->Duration = adjustedDuration;
	fx->Target = FX_TARGET_PRESET;
	fx->Resource = parameters->string1Parameter;
	//int2param isn't actually used in the original engine

	core->ApplyEffect(fx, actor, src);

	// SpellCastEffect tries to keep the action alive until the effect is over
	Sender->SetWait(adjustedDuration);
}

//this action plays a vvc animation over target
//we simply apply the appropriate opcode on the target (see iwdopcodes)
//the list of vvcs is in iwdshtab.2da (sheffect.ids)
static EffectRef fx_iwd_visual_spell_hit_ref = { "IWDVisualSpellHit", -1 };

void GameScript::SpellHitEffectSprite(Scriptable* Sender, Action* parameters)
{
	Scriptable* src = GetScriptableFromObject(Sender, parameters);
	if (!src) {
		return;
	}
	Scriptable* tar = GetScriptableFromObject2(Sender, parameters);
	Actor* target = Scriptable::As<Actor>(tar);
	if (!target) {
		return;
	}
	int opcode = EffectQueue::ResolveEffect(fx_iwd_visual_spell_hit_ref);
	Effect* fx = GetEffect(opcode);
	if (!fx) {
		//invalid effect name didn't resolve to opcode
		return;
	}

	// int1Parameter was supposed to be height ... but was unused
	// these projectiles hardcoded a -100 y offset:
	// 69 RIGHTEOUS_WRATH_OF_THE_FAITHFUL_GROUND
	// 74 BLADE_BARRIER_BACK
	// 76 CIRCLE_OF_BONES_BACK
	// 98 UNDEAD_WARD
	// 104 MONSTER_SUMMONING_CIRCLE
	// 105 ANIMAL_SUMMONING_CIRCLE
	// 106 EARTH_SUMMONING_CIRCLE
	// 107 FIRE_SUMMONING_CIRCLE
	// 108 WATER_SUMMONING_CIRCLE
	std::array<int, 9> exceptions = { 69, 74, 76, 98, 104, 105, 106, 107, 108 };
	fx->Pos = tar->Pos;
	fx->Pos.y += int(ProHeights::Normal); // negate future ZPos
	for (const auto& pro : exceptions) {
		if (parameters->int0Parameter == pro) {
			fx->Pos.y -= 100;
			break;
		}
	}

	//vvc type
	fx->Parameter2 = parameters->int0Parameter + 0x1001;
	fx->Parameter4 = 1; // mark for special treatment
	fx->ProbabilityRangeMax = 100;
	fx->ProbabilityRangeMin = 0;
	fx->TimingMode = FX_DURATION_INSTANT_PERMANENT_AFTER_BONUSES;
	fx->Target = FX_TARGET_PRESET;
	core->ApplyEffect(fx, target, src);

	// see note in SpellHitEffectPoint, helps with #2109 efreeti all spawning at the same time
	Sender->SetWait(3);
}

void GameScript::SpellHitEffectPoint(Scriptable* Sender, Action* parameters)
{
	Scriptable* src = GetScriptableFromObject(Sender, parameters);
	if (!src) {
		return;
	}

	int opcode = EffectQueue::ResolveEffect(fx_iwd_visual_spell_hit_ref);
	Effect* fx = GetEffect(opcode);
	if (!fx) {
		//invalid effect name didn't resolve to opcode
		return;
	}

	//vvc type
	fx->Parameter2 = parameters->int0Parameter;
	//height (not sure if this is in the opcode, but seems acceptable)
	fx->Parameter1 = parameters->int1Parameter;
	fx->Parameter4 = 1; // mark for special treatment
	fx->ProbabilityRangeMax = 100;
	fx->ProbabilityRangeMin = 0;
	fx->TimingMode = FX_DURATION_INSTANT_PERMANENT_AFTER_BONUSES;
	// iwd2 with [-1.-1] again
	if (parameters->pointParameter.x == -1) {
		fx->Pos = src->Pos;
	} else {
		fx->Pos = parameters->pointParameter;
	}
	fx->Pos.y += int(ProHeights::Normal); // negate future ZPos
	fx->Target = FX_TARGET_PRESET;
	core->ApplyEffect(fx, NULL, src);

	// it should probably wait until projectile payload, but a single tick works well for the use in 41cnatew
	Sender->SetWait(1);
}


void GameScript::ClickLButtonObject(Scriptable* Sender, Action* parameters)
{
	const Scriptable* tar = GetScriptableFromObject(Sender, parameters);
	if (!tar) {
		Sender->ReleaseCurrentAction(); // this is blocking for some reason?
		return;
	}
	Event e = EventMgr::CreateMouseBtnEvent(tar->Pos, GEM_MB_ACTION, true);
	ClickCore(Sender, e.mouse, parameters->int0Parameter);
}

void GameScript::ClickLButtonPoint(Scriptable* Sender, Action* parameters)
{
	Event e = EventMgr::CreateMouseBtnEvent(parameters->pointParameter, GEM_MB_ACTION, true);
	ClickCore(Sender, e.mouse, parameters->int0Parameter);
}

void GameScript::ClickRButtonObject(Scriptable* Sender, Action* parameters)
{
	const Scriptable* tar = GetScriptableFromObject(Sender, parameters);
	if (!tar) {
		Sender->ReleaseCurrentAction(); // this is blocking for some reason?
		return;
	}
	Event e = EventMgr::CreateMouseBtnEvent(tar->Pos, GEM_MB_MENU, true);
	ClickCore(Sender, e.mouse, parameters->int0Parameter);
}

void GameScript::ClickRButtonPoint(Scriptable* Sender, Action* parameters)
{
	Event e = EventMgr::CreateMouseBtnEvent(parameters->pointParameter, GEM_MB_MENU, true);
	ClickCore(Sender, e.mouse, parameters->int0Parameter);
}

void GameScript::DoubleClickLButtonObject(Scriptable* Sender, Action* parameters)
{
	const Scriptable* tar = GetScriptableFromObject(Sender, parameters);
	if (!tar) {
		Sender->ReleaseCurrentAction(); // this is blocking for some reason?
		return;
	}
	Event e = EventMgr::CreateMouseBtnEvent(tar->Pos, GEM_MB_ACTION, true);
	e.mouse.repeats = 2;
	ClickCore(Sender, e.mouse, parameters->int0Parameter);
}

void GameScript::DoubleClickLButtonPoint(Scriptable* Sender, Action* parameters)
{
	Event e = EventMgr::CreateMouseBtnEvent(parameters->pointParameter, GEM_MB_ACTION, true);
	e.mouse.repeats = 2;
	ClickCore(Sender, e.mouse, parameters->int0Parameter);
}

void GameScript::DoubleClickRButtonObject(Scriptable* Sender, Action* parameters)
{
	const Scriptable* tar = GetScriptableFromObject(Sender, parameters);
	if (!tar) {
		Sender->ReleaseCurrentAction(); // this is blocking for some reason?
		return;
	}
	Event e = EventMgr::CreateMouseBtnEvent(tar->Pos, GEM_MB_MENU, true);
	e.mouse.repeats = 2;
	ClickCore(Sender, e.mouse, parameters->int0Parameter);
}

void GameScript::DoubleClickRButtonPoint(Scriptable* Sender, Action* parameters)
{
	Event e = EventMgr::CreateMouseBtnEvent(parameters->pointParameter, GEM_MB_MENU, true);
	e.mouse.repeats = 2;
	ClickCore(Sender, e.mouse, parameters->int0Parameter);
}

//Picks 5 lines from wish.2da
//Gets the 5 values (column is int0parameter) from the table.
//Sets the five wishpowerNN to 1, while resets the rest to 0.
void GameScript::SetupWish(Scriptable* Sender, Action* parameters)
{
	SetupWishCore(Sender, parameters->int0Parameter, parameters->int1Parameter);
}

//The same as the previous action, except that the column parameter comes from
//the target object's wisdom directly (this action is not used in the original)
void GameScript::SetupWishObject(Scriptable* Sender, Action* parameters)
{
	const Scriptable* tar = GetScriptableFromObject(Sender, parameters);
	const Actor* target = Scriptable::As<Actor>(tar);
	if (!target) {
		return;
	}
	SetupWishCore(Sender, target->GetStat(IE_WIS), parameters->int0Parameter);
}

//GemRB specific action
//Sets up multiple tokens randomly (one per 2da row)
//the row label column sets the token names
void GameScript::SetToken2DA(Scriptable* /*Sender*/, Action* parameters)
{
	AutoTable tm = gamedata->LoadTable(parameters->resref0Parameter);
	if (!tm) {
		Log(ERROR, "Actions", "Cannot find {}.2da.", parameters->string0Parameter);
		return;
	}

	TableMgr::index_t count = tm->GetRowCount();
	for (TableMgr::index_t i = 0; i < count; i++) {
		//roll a random number between 0 and column #
		TableMgr::index_t j = RAND<TableMgr::index_t>(0, tm->GetColumnCount(i) - 1);
		ieVariable tokenname = tm->GetRowName(i);

		core->GetTokenDictionary()[tokenname] = StringFromTLK(tm->QueryField(i, j));
	}
}

//this is a gemrb extension for scriptable tracks
void GameScript::SetTrackString(Scriptable* Sender, Action* parameters)
{
	Map* map = Sender->GetCurrentArea();
	if (!map) return;
	map->SetTrackString(ieStrRef(parameters->int0Parameter), parameters->int1Parameter, parameters->int2Parameter);
}

void GameScript::StateOverrideFlag(Scriptable* /*Sender*/, Action* parameters)
{
	core->GetGame()->StateOverrideFlag = parameters->int0Parameter;
}

void GameScript::StateOverrideTime(Scriptable* /*Sender*/, Action* parameters)
{
	core->GetGame()->StateOverrideTime = parameters->int0Parameter;
}

void GameScript::BanterBlockFlag(Scriptable* /*Sender*/, Action* parameters)
{
	core->GetGame()->BanterBlockFlag = parameters->int0Parameter;
}

void GameScript::BanterBlockTime(Scriptable* /*Sender*/, Action* parameters)
{
	core->GetGame()->BanterBlockTime = parameters->int0Parameter;
}

void GameScript::SetNamelessDeath(Scriptable* Sender, Action* parameters)
{
	ResRef area;
	area.Format("AR{:04d}", parameters->int0Parameter);
	IniSpawn* sp = Sender->GetCurrentArea()->INISpawn;
	if (!sp) {
		return;
	}
	sp->SetNamelessDeath(area, parameters->pointParameter, parameters->int1Parameter);
}

void GameScript::SetNamelessDeathParty(Scriptable* Sender, Action* parameters)
{
	IniSpawn* sp = Sender->GetCurrentArea()->INISpawn;
	if (!sp) {
		return;
	}
	sp->SetNamelessDeathParty(parameters->pointParameter, parameters->int0Parameter);
}

// like GameScript::Kill, but forces chunking damage (disabling resurrection)
void GameScript::ChunkCreature(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetScriptableFromObject(Sender, parameters);
	Actor* target = Scriptable::As<Actor>(tar);
	if (!target) {
		return;
	}

	Effect* fx = EffectQueue::CreateEffect(fx_death_ref, 0, 8, FX_DURATION_INSTANT_PERMANENT);
	target->fxqueue.AddEffect(fx, false);
}

void GameScript::MultiPlayerSync(Scriptable* Sender, Action* /*parameters*/)
{
	Sender->SetWait(2);
}

void GameScript::DestroyAllFragileEquipment(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetScriptableFromObject(Sender, parameters);
	Actor* actor = Scriptable::As<Actor>(tar);
	if (!actor) {
		return;
	}

	// bits are from itemflag.ids and are identical to our Item flags, not CREItem
	// handling the only known user for now
	ieDword flags = parameters->int0Parameter;
	if (flags & IE_ITEM_ADAMANTINE) flags = (flags | IE_INV_ITEM_ADAMANTINE) & ~IE_ITEM_ADAMANTINE;
	actor->inventory.DestroyItem("", flags, ~0);
}

void GameScript::SetOriginalClass(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetScriptableFromObject(Sender, parameters);
	Actor* actor = Scriptable::As<Actor>(tar);
	int classBit = parameters->int0Parameter & MC_WAS_ANY;
	if (!actor || !classBit) {
		return;
	}

	if (parameters->int1Parameter == 0) {
		// only reset the class bits
		actor->SetMCFlag(MC_WAS_ANY, BitOp::NAND);
		parameters->int1Parameter = int(BitOp::OR);
	}
	actor->SetMCFlag(classBit, BitOp(parameters->int1Parameter));
}

void GameScript::SetPCStatsTokens(Scriptable* Sender, Action* parameters)
{
	const Scriptable* tar = GetScriptableFromObject(Sender, parameters);
	const Actor* actor = Scriptable::As<const Actor>(tar);
	if (!actor || !actor->PCStats) {
		return;
	}

	static auto setfav = [](const auto& favs, ieVariable favtok, ieVariable cnttok) {
		String token = u"/";
		auto fav = std::max_element(favs.begin(), favs.end(), [](const auto& lhs, const auto& rhs) {
			return lhs.second < rhs.second;
		});
		if (fav->second != 0) token = StringFromTLK(fav->first);
		core->GetTokenDictionary()[favtok] = std::move(token);
		SetTokenAsString(cnttok, fav->second);
	};

	setfav(actor->PCStats->FavouriteSpells, "FAVOURITESPELL", "FAVOURITESPELLNUM");
	setfav(actor->PCStats->FavouriteWeapons, "FAVOURITEWEAPON", "FAVOURITEWEAPONNUM");

	// kill stats
	SetTokenAsString("KILLCOUNT", actor->PCStats->KillsTotalCount);
	SetTokenAsString("KILLCOUNTCHAPTER", actor->PCStats->KillsChapterCount);
	core->GetTokenDictionary()["BESTKILL"] = core->GetString(actor->PCStats->BestKilledName);
}

void GameScript::ForceRandomEncounter(Scriptable* Sender, Action* parameters)
{
	WorldMap* worldMap = core->GetWorldMap();
	// potentially wrong area, should we postpone until the worldmap is brought up for traveling?
	WMPAreaLink* link = worldMap->GetLink(Sender->GetCurrentArea()->GetScriptName(), parameters->resref0Parameter);
	if (!link) {
		return;
	}
	if (!parameters->variable1Parameter.IsEmpty()) {
		link->DestEntryPoint = parameters->variable1Parameter;
		core->GetGame()->RandomEncounterEntry = parameters->variable1Parameter;
	}
	worldMap->SetEncounterArea(parameters->resref0Parameter, link);
	core->GetGame()->RandomEncounterArea = parameters->resref0Parameter;
}

void GameScript::RemoveStoreItem(Scriptable* /*Sender*/, Action* parameters)
{
	::GemRB::RemoveStoreItem(parameters->resref0Parameter, parameters->resref1Parameter, parameters->int0Parameter);
}

void GameScript::AddStoreItem(Scriptable* /*Sender*/, Action* parameters)
{
	Store* store = gamedata->GetStore(parameters->resref0Parameter);
	if (!store) {
		Log(ERROR, "GameScript", "AddStoreItem: store {} cannot be opened!", parameters->resref0Parameter);
		return;
	}

	CREItem* itm = new CREItem();
	if (!CreateItemCore(itm, parameters->resref1Parameter, 1, 0, 0)) {
		delete itm;
		return;
	}
	itm->Flags |= parameters->int1Parameter; // should we just set instead?

	// we could complicate and take item->MaxStackAmount into account, but AddItem can take care of everything
	while (parameters->int0Parameter--) {
		store->AddItem(itm);
	}
	delete itm;
	// store changed, save it
	gamedata->SaveStore(store);
	return;
}

void GameScript::DestroyGroundPiles(Scriptable* Sender, Action* /*parameters*/)
{
	const Map* map = Sender->GetCurrentArea();
	if (!map) return;
	TileMap* tm = map->GetTileMap();
	size_t containerCount = tm->GetContainerCount();
	while (containerCount--) {
		Container* pile = tm->GetContainer(containerCount);
		if (pile->containerType != IE_CONTAINER_PILE) continue;

		pile->inventory.DestroyItem("", 0, (ieDword) ~0); //destroy any and all
		pile->RemoveItem(0, 0); // force ground icon refresh
		tm->CleanupContainer(pile);
	}
}

void GameScript::SetWorldmap(Scriptable* /*Sender*/, Action* parameters)
{
	core->UpdateWorldMap(parameters->resref0Parameter);
}

// pretty ineffectual action, due to this being important in the importer
// only the CheckAreaDiffLevel trigger would pick this change up
void GameScript::OverrideAreaDifficulty(Scriptable* /*Sender*/, Action* parameters)
{
	Map* map = core->GetGame()->GetMap(parameters->resref0Parameter, false);
	if (!map) return;
	map->AreaDifficulty = parameters->int0Parameter;
}

// TODO: ee, this action reinitializes important default values and resource
// references based on definitions from campaign.2da, such as world scripts,
// save folder name or starting area. Campaign refers to the name defined in
// the first column of that table. Used eg. to switch from BGEE to SOD.
// this approach complements our start.2da and should be unified
// also check MoveToExpansion, Game::CurrentCampaign
void GameScript::MoveToCampaign(Scriptable* /*Sender*/, Action* parameters)
{
	Log(ERROR, "GameScript", "MoveToCampaign is not implemented yet!");
	core->UpdateWorldMap(parameters->resref0Parameter);
}

// SetNoWaitX(I:SetReset*Boolean) was a stub
// TODO: ee, ContinueGame(I:State*Boolean)
//       IESDP: If "State" is `FALSE` then this action clears the "Last Save" definition in `baldur.lua` for the current
//       campaign that would otherwise enable the player to automatically load the latest save from the start menu of the game.
//       Specifying `TRUE` does nothing.
// TODO: ee, unclear if really useful DisableAI(O:Object*,I:Disable*Boolean) used in cutscenes
// IESDP says: this action activates or deactivates all creature scripts of the given target depending on the second parameter.
// ... but they're already disabled; is this used more to enable them despite cutscene logic?

// TODO: ee, zoom actions
// 412 ZoomLock(I:Lock*Boolean)
// 	This action can be used to set zoom to 100%. When set to TRUE zoom factor is locked at 100% and can not be changed by user input. Setting it to FALSE restores the original zoom factor. The zoom lock state is not saved.
//
// 463 SetZoomViewport(P:Point*)
// 	Changes the current zoom level to match the viewport size specified by the point parameter. The action has no effect if Zoom Lock has been enabled in the game options.
//
// 464 StoreZoomLevel()
// 	Stores the current zoom level internally. It can be restored with RestoreZoomLevel(). The stored zoom level is not saved.
//
// 465 RestoreZoomLevel()
// 	Restores the zoom level stored by a previous call of StoreZoomLevel(). The action has no effect if Zoom Lock has been enabled in the game options.

// TODO: ee, voice channels actions
// 470 WaitForVoiceChannel()
// 	Delays actions until the voice channel of the active creature is ready to play a new sound.
//
// 471 PlaySoundThroughVoice(S:Sound*)
// 	Plays the specified sound through the actor's voice channel.

}
