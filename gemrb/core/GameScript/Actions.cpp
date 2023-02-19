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

#include "GameScript/GameScript.h"

#include "GameScript/GSUtils.h"
#include "GameScript/Matching.h"

#include "voodooconst.h"

#include "AmbientMgr.h"
#include "CharAnimations.h"
#include "DataFileMgr.h"
#include "DialogHandler.h"
#include "DisplayMessage.h"
#include "Game.h"
#include "GameData.h"
#include "GlobalTimer.h"
#include "IniSpawn.h"
#include "Item.h"
#include "Map.h"
#include "MusicMgr.h"
#include "SaveGameIterator.h"
#include "ScriptEngine.h"
#include "TileMap.h"
#include "Video/Video.h"
#include "WorldMap.h"
#include "GUI/GameControl.h"
#include "GUI/EventMgr.h"
#include "RNG.h"
#include "Scriptable/Container.h"
#include "Scriptable/Door.h"
#include "Scriptable/InfoPoint.h"
#include "ScriptedAnimation.h"
#include "StringMgr.h"

namespace GemRB {

//------------------------------------------------------------
// Action Functions
//-------------------------------------------------------------

void GameScript::SetExtendedNight(Scriptable* Sender, Action* parameters)
{
	Map *map=Sender->GetCurrentArea();
	//sets the 'can rest other' bit
	if (parameters->int0Parameter) {
		map->AreaType|=AT_EXTENDED_NIGHT;
	} else {
		map->AreaType&=~AT_EXTENDED_NIGHT;
	}
}

void GameScript::SetAreaRestFlag(Scriptable* Sender, Action* parameters)
{
	Map *map=Sender->GetCurrentArea();
	//sets the 'can rest other' bit
	if (parameters->int0Parameter) {
		map->AreaType |= AT_CAN_REST_INDOORS;
	} else {
		map->AreaType &= ~AT_CAN_REST_INDOORS;
	}
}

void GameScript::AddAreaFlag(Scriptable* Sender, Action* parameters)
{
	Map *map=Sender->GetCurrentArea();
	map->AreaFlags|=parameters->int0Parameter;
}

void GameScript::RemoveAreaFlag(Scriptable* Sender, Action* parameters)
{
	Map *map=Sender->GetCurrentArea();
	map->AreaFlags&=~parameters->int0Parameter;
}

void GameScript::SetAreaFlags(Scriptable* Sender, Action* parameters)
{
	Map *map=Sender->GetCurrentArea();
	ieDword value = map->AreaFlags;
	HandleBitMod( value, parameters->int0Parameter, BitOp(parameters->int1Parameter));
	map->AreaFlags=value;
}

void GameScript::AddAreaType(Scriptable* Sender, Action* parameters)
{
	Map *map=Sender->GetCurrentArea();
	map->AreaType |= MapEnv(parameters->int0Parameter);
}

void GameScript::RemoveAreaType(Scriptable* Sender, Action* parameters)
{
	Map *map=Sender->GetCurrentArea();
	map->AreaType &= ~MapEnv(parameters->int0Parameter);
}

void GameScript::NoAction(Scriptable* /*Sender*/, Action* /*parameters*/)
{
	//thats all :)
}

void GameScript::SG(Scriptable* Sender, Action* parameters)
{
	SetVariable( Sender, parameters->string0Parameter, parameters->int0Parameter, "GLOBAL");
}

void GameScript::SetGlobal(Scriptable* Sender, Action* parameters)
{
	SetVariable( Sender, parameters->string0Parameter, parameters->int0Parameter );
}

void GameScript::SetGlobalRandom(Scriptable* Sender, Action* parameters)
{
	int max=parameters->int1Parameter-parameters->int0Parameter+1;
	if (max>0) {
		SetVariable( Sender, parameters->string0Parameter, RandomNumValue%max+parameters->int0Parameter );
	} else {
		SetVariable( Sender, parameters->string0Parameter, 0);
	}
}

void GameScript::StartTimer(Scriptable* Sender, Action* parameters)
{
	Sender->StartTimer(parameters->int0Parameter, parameters->int1Parameter);
}

void GameScript::StartRandomTimer(Scriptable* Sender, Action* parameters)
{
	ieDword value = core->Roll(1, parameters->int2Parameter-parameters->int1Parameter, parameters->int2Parameter-1);
	Sender->StartTimer(parameters->int0Parameter, value);
}

void GameScript::SetGlobalTimer(Scriptable* Sender, Action* parameters)
{
	ieDword mytime;

	mytime=core->GetGame()->GameTime; //gametime (should increase it)
	SetVariable( Sender, parameters->string0Parameter,
		parameters->int0Parameter * core->Time.ai_update_time + mytime);
}

void GameScript::SetGlobalTimerRandom(Scriptable* Sender, Action* parameters)
{
	ieDword mytime;
	int random;

	//This works both ways in the original engine
	if (parameters->int1Parameter>parameters->int0Parameter) {
		random = parameters->int1Parameter-parameters->int0Parameter+1;
		//random cannot be 0, its minimal value is 1
		random = RandomNumValue % random + parameters->int0Parameter;
	} else {
		random = parameters->int0Parameter-parameters->int1Parameter+1;
		random = RandomNumValue % random + parameters->int1Parameter;
	}
	mytime=core->GetGame()->GameTime; //gametime (should increase it)
	SetVariable(Sender, parameters->string0Parameter, random * core->Time.ai_update_time + mytime);
}

void GameScript::SetGlobalTimerOnce(Scriptable* Sender, Action* parameters)
{
	ieDword mytime = CheckVariable( Sender, parameters->string0Parameter );
	if (mytime != 0) {
		return;
	}
	mytime=core->GetGame()->GameTime; //gametime (should increase it)
	SetVariable( Sender, parameters->string0Parameter,
		parameters->int0Parameter * core->Time.ai_update_time + mytime);
}

void GameScript::RealSetGlobalTimer(Scriptable* Sender, Action* parameters)
{
	ieDword mytime=core->GetGame()->RealTime;

	SetVariable( Sender, parameters->string0Parameter,
		parameters->int0Parameter * core->Time.ai_update_time + mytime);
}

void GameScript::ChangeAllegiance(Scriptable* Sender, Action* parameters)
{
	Scriptable *scr = Sender;
	if (parameters->objects[1]) {
		scr = GetScriptableFromObject(Sender, parameters->objects[1]);
	}
	Actor* actor = Scriptable::As<Actor>(scr);
	if (!actor) {
		return;
	}

	actor->SetBase( IE_EA, parameters->int0Parameter );
}

void GameScript::ChangeGeneral(Scriptable* Sender, Action* parameters)
{
	Scriptable *scr = Sender;
	if (parameters->objects[1]) {
		scr = GetScriptableFromObject(Sender, parameters->objects[1]);
	}
	Actor* actor = Scriptable::As<Actor>(scr);
	if (!actor) {
		return;
	}

	actor->SetBase( IE_GENERAL, parameters->int0Parameter );
}

void GameScript::ChangeRace(Scriptable* Sender, Action* parameters)
{
	Scriptable *scr = Sender;
	if (parameters->objects[1]) {
		scr = GetScriptableFromObject(Sender, parameters->objects[1]);
	}
	Actor* actor = Scriptable::As<Actor>(scr);
	if (!actor) {
		return;
	}

	actor->SetBase( IE_RACE, parameters->int0Parameter );
}

void GameScript::ChangeClass(Scriptable* Sender, Action* parameters)
{
	Scriptable *scr = Sender;
	if (parameters->objects[1]) {
		scr = GetScriptableFromObject(Sender, parameters->objects[1]);
	}
	Actor* actor = Scriptable::As<Actor>(scr);
	if (!actor) {
		return;
	}

	actor->SetBase( IE_CLASS, parameters->int0Parameter );
}

void GameScript::SetNamelessClass(Scriptable* /*Sender*/, Action* parameters)
{
	//same as Protagonist
	Actor* actor = core->GetGame()->GetPC(0, false);
	actor->SetBase( IE_CLASS, parameters->int0Parameter );
}

void GameScript::SetNamelessDisguise(Scriptable* Sender, Action* parameters)
{
	SetVariable(Sender, "APPEARANCE", parameters->int0Parameter, "GLOBAL");
	core->SetEventFlag(EF_UPDATEANIM);
}

void GameScript::ChangeSpecifics(Scriptable* Sender, Action* parameters)
{
	Scriptable *scr = Sender;
	if (parameters->objects[1]) {
		scr = GetScriptableFromObject(Sender, parameters->objects[1]);
	}
	Actor* actor = Scriptable::As<Actor>(scr);
	if (!actor) {
		return;
	}

	actor->SetBase( IE_SPECIFIC, parameters->int0Parameter );
}

inline void PermanentStatChangeFeedback(int stat, const Actor* actor)
{
	// figure out PC index (TNO, Morte, Annah, Dakkon, FFG, Nordom, Ignus, Vhailor)
	// then use it to calculate presonalized feedback strings
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
	// there's also this one, but there's no stat, perhaps used for level up, where you get the attribute increase window?
	// @35621 = ~Characteristic Points increased permanently [Nameless One]~
}

void GameScript::PermanentStatChange(Scriptable* Sender, Action* parameters)
{
	Scriptable *scr = Sender;
	if (parameters->objects[1]) {
		scr = GetScriptableFromObject(Sender, parameters->objects[1]);
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
	Scriptable *scr = Sender;
	if (parameters->objects[1]) {
		scr = GetScriptableFromObject(Sender, parameters->objects[1]);
	}
	Actor* actor = Scriptable::As<Actor>(scr);
	if (!actor) {
		return;
	}

	ieDword value = parameters->int1Parameter;
	if (parameters->int2Parameter==1) { // basically statmod.ids entries, but there's only two
		value+=actor->GetBase(parameters->int0Parameter);
	}
	actor->SetBase( parameters->int0Parameter, value);
}

void GameScript::ChangeStatGlobal(Scriptable* Sender, Action* parameters)
{
	Scriptable *scr = Sender;
	if (parameters->objects[1]) {
		scr = GetScriptableFromObject(Sender, parameters->objects[1]);
	}
	Actor* actor = Scriptable::As<Actor>(scr);
	if (!actor) {
		return;
	}
	ieDword value = CheckVariable(Sender, parameters->string0Parameter, parameters->string1Parameter);
	if (parameters->int1Parameter==1) {
		value+=actor->GetBase(parameters->int0Parameter);
	}
	actor->SetBase( parameters->int0Parameter, value);
}

void GameScript::ChangeGender(Scriptable* Sender, Action* parameters)
{
	Scriptable *scr = Sender;
	if (parameters->objects[1]) {
		scr = GetScriptableFromObject(Sender, parameters->objects[1]);
	}
	Actor* actor = Scriptable::As<Actor>(scr);
	if (!actor) {
		return;
	}

	actor->SetBase( IE_SEX, parameters->int0Parameter );
}

void GameScript::ChangeAlignment(Scriptable* Sender, Action* parameters)
{
	Scriptable *scr = Sender;
	if (parameters->objects[1]) {
		scr = GetScriptableFromObject(Sender, parameters->objects[1]);
	}
	Actor* actor = Scriptable::As<Actor>(scr);
	if (!actor) {
		return;
	}

	actor->SetBase( IE_ALIGNMENT, parameters->int0Parameter );
}

void GameScript::SetFaction(Scriptable* Sender, Action* parameters)
{
	Scriptable *scr = Sender;
	if (parameters->objects[1]) {
		scr = GetScriptableFromObject(Sender, parameters->objects[1]);
	}
	Actor* actor = Scriptable::As<Actor>(scr);
	if (!actor) {
		return;
	}

	actor->SetBase( IE_FACTION, parameters->int0Parameter );
}

void GameScript::SetHP(Scriptable* Sender, Action* parameters)
{
	Scriptable *scr = Sender;
	if (parameters->objects[1]) {
		scr = GetScriptableFromObject(Sender, parameters->objects[1]);
	}
	Actor* actor = Scriptable::As<Actor>(scr);
	if (!actor) {
		return;
	}

	actor->SetBase( IE_HITPOINTS, parameters->int0Parameter );
}

void GameScript::SetHPPercent(Scriptable* Sender, Action* parameters)
{
	Scriptable *scr = Sender;
	if (parameters->objects[1]) {
		scr = GetScriptableFromObject(Sender, parameters->objects[1]);
	}
	Actor* actor = Scriptable::As<Actor>(scr);
	if (!actor) {
		return;
	}

	// 0 - adjust to max hp, 1 - adjust to current
	if (parameters->int1Parameter) {
		actor->NewBase(IE_HITPOINTS, parameters->int0Parameter, MOD_PERCENT);
	} else {
		actor->NewBase(IE_HITPOINTS, actor->GetStat(IE_MAXHITPOINTS) * parameters->int0Parameter/100, MOD_ABSOLUTE);
	}
}

void GameScript::AddHP(Scriptable* Sender, Action* parameters)
{
	Scriptable *scr = Sender;
	if (parameters->objects[1]) {
		scr = GetScriptableFromObject(Sender, parameters->objects[1]);
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
	Scriptable *scr = Sender;
	if (parameters->objects[1]) {
		scr = GetScriptableFromObject(Sender, parameters->objects[1]);
	}
	Actor* actor = Scriptable::As<Actor>(scr);
	if (!actor) {
		return;
	}

	actor->SetBase( IE_TEAM, parameters->int0Parameter );
}

//this works on an object (gemrb)
//or on Myself if object isn't given (iwd2)
void GameScript::SetTeamBit(Scriptable* Sender, Action* parameters)
{
	Scriptable *scr = Sender;
	if (parameters->objects[1]) {
		scr = GetScriptableFromObject(Sender, parameters->objects[1]);
	}
	Actor* actor = Scriptable::As<Actor>(scr);
	if (!actor) {
		return;
	}

	if (parameters->int1Parameter) {
		actor->SetBase( IE_TEAM, actor->GetStat(IE_TEAM) | parameters->int0Parameter );
	} else {
		actor->SetBase( IE_TEAM, actor->GetStat(IE_TEAM) & ~parameters->int0Parameter );
	}
}

void GameScript::TriggerActivation(Scriptable* Sender, Action* parameters)
{
	Scriptable* ip = Sender;
	StringParam& name = parameters->string0Parameter;

	if (parameters->objects[1]) {
		ip = GetScriptableFromObject(Sender, parameters->objects[1]);
		if (ip) name = parameters->objects[1]->objectName;
	}
	if (!ip || (ip->Type!=ST_TRIGGER && ip->Type!=ST_TRAVEL && ip->Type!=ST_PROXIMITY)) {
		Log(WARNING, "Actions", "Script error: No Trigger Named \"{}\"", name);
		parameters->dump();
		return;
	}
	InfoPoint *trigger = static_cast<InfoPoint*>(ip);
	if ( parameters->int0Parameter != 0 ) {
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
	// fallback matches SetFadeToColor fallback
	Sender->SetWait(parameters->pointParameter.x ? parameters->pointParameter.x * 3 / 4 : 30);
	Sender->ReleaseCurrentAction();
}

void GameScript::FadeFromColor(Scriptable* Sender, Action* parameters)
{
	core->timer.SetFadeFromColor(parameters->pointParameter.x);
	Sender->SetWait(parameters->pointParameter.x ? parameters->pointParameter.x : 30);
	Sender->ReleaseCurrentAction();
}

void GameScript::FadeToAndFromColor(Scriptable* Sender, Action* parameters)
{
	core->timer.SetFadeToColor(parameters->pointParameter.x);
	core->timer.SetFadeFromColor(parameters->pointParameter.x);
	Sender->SetWait(parameters->pointParameter.x ? 2 * parameters->pointParameter.x : 60);
	Sender->ReleaseCurrentAction();
}

void GameScript::JumpToPoint(Scriptable* Sender, Action* parameters)
{
	Actor* actor = Scriptable::As<Actor>(Sender);
	if (!actor) {
		return;
	}
	actor->SetPosition(parameters->pointParameter, true);
}

void GameScript::JumpToPointInstant(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetScriptableFromObject(Sender, parameters->objects[1]);
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
	Scriptable* tar = GetScriptableFromObject(Sender, parameters->objects[1]);
	if (!tar) {
		tar = Sender;
	}
	Actor* actor = Scriptable::As<Actor>(tar);
	if (!actor) {
		return;
	}
	Point p(actor->GetStat(IE_SAVEDXPOS), actor->GetStat(IE_SAVEDYPOS));
	actor->SetPosition(p, true );
	actor->SetOrientation(ClampToOrientation(actor->GetStat(IE_SAVEDFACE)), false);
}

void GameScript::JumpToObject(Scriptable* Sender, Action* parameters)
{
	Actor* actor = Scriptable::As<Actor>(Sender);
	if (!actor) {
		return;
	}

	const Scriptable* tar = GetScriptableFromObject(Sender, parameters->objects[1]);
	if (!tar) return;
	const Map *map = tar->GetCurrentArea();
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
	const Game *game = core->GetGame();
	game->MovePCs(parameters->resref0Parameter, parameters->pointParameter, parameters->int0Parameter);
	game->MoveFamiliars(parameters->resref0Parameter, parameters->pointParameter, parameters->int0Parameter);
}

//5 is the ToB value, but it might be useful to have multiple expansions
void GameScript::MoveToExpansion(Scriptable* Sender, Action* parameters)
{
	Game *game = core->GetGame();

	game->SetExpansion(parameters->int0Parameter);
	Sender->ReleaseCurrentAction();
}

//add some animation effects too?
void GameScript::ExitPocketPlane(Scriptable* /*Sender*/, Action* /*parameters*/)
{
	Point pos;
	ResRef area;

	Game *game = core->GetGame();
	int cnt = game->GetPartySize(false);
	for (int i = 0; i < cnt; i++) {
		Actor* act = game->GetPC( i, false );
		if (act) {
			const GAMLocationEntry *gle;
			if (game->GetPlaneLocationCount() <= (unsigned int)i) {
				// no location, meaning the actor joined the party after the save
				// reuse the last valid location
				gle = game->GetPlaneLocationEntry(game->GetPlaneLocationCount()-1);
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
	const Game *game = core->GetGame();
	int i = game->GetPartySize(false);
	while (i--) {
		Actor *tar = game->GetPC(i, false);
		//if the actor isn't in the source area, we don't care
		if (tar->Area != parameters->resref0Parameter) {
			continue;
		}
		// no need of CreateMovementEffect, party members are always moved immediately
		MoveBetweenAreasCore(tar, parameters->resref1Parameter, parameters->pointParameter, -1, true);
	}
	i = game->GetNPCCount();
	while (i--) {
		Actor *tar = game->GetNPC(i);
		//if the actor isn't in the source area, we don't care
		if (tar->Area != parameters->resref0Parameter) {
			continue;
		}
		//if the actor is currently in a loaded area, remove it from there
		Map *map = tar->GetCurrentArea();
		if (map) {
			map->RemoveActor(tar);
		}
		//update the target's area to the destination
		tar->Area = parameters->resref1Parameter;
		//if the destination area is currently loaded, move the actor there now
		if (game->FindMap(tar->Area)) {
			MoveBetweenAreasCore(tar, parameters->resref1Parameter, parameters->pointParameter, -1, true);
		}
	}
}

void GameScript::MoveGlobal(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetScriptableFromObject(Sender, parameters->objects[1]);
	Actor* actor = Scriptable::As<Actor>(tar);
	if (!actor) {
		return;
	}

	if (actor->Persistent() || !CreateMovementEffect(actor, parameters->resref0Parameter, parameters->pointParameter, 0)) {
		MoveBetweenAreasCore( actor, parameters->resref0Parameter, parameters->pointParameter, -1, true);
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
	CreateCreatureCore( Sender, parameters, CC_CHECK_IMPASSABLE|CC_CHECK_OVERLAP|CC_SCRIPTNAME );
}

//another highly redundant action
void GameScript::CreateCreatureDoor(Scriptable* Sender, Action* parameters)
{
	//we hack this to death
	parameters->resref1Parameter = "SPDIMNDR";
	CreateCreatureCore( Sender, parameters, CC_CHECK_IMPASSABLE|CC_CHECK_OVERLAP | CC_PLAY_ANIM );
}

//another highly redundant action
void GameScript::CreateCreatureObjectDoor(Scriptable* Sender, Action* parameters)
{
	//we hack this to death
	parameters->resref1Parameter = "SPDIMNDR";
	CreateCreatureCore( Sender, parameters, CC_OBJECT | CC_CHECK_IMPASSABLE|CC_CHECK_OVERLAP | CC_PLAY_ANIM );
}

//don't use offset from Sender
void GameScript::CreateCreatureImpassable(Scriptable* Sender, Action* parameters)
{
	CreateCreatureCore( Sender, parameters, CC_CHECK_OVERLAP );
}

void GameScript::CreateCreatureImpassableAllowOverlap(Scriptable* Sender, Action* parameters)
{
	CreateCreatureCore( Sender, parameters, 0 );
}

//use offset from Sender
void GameScript::CreateCreatureAtFeet(Scriptable* Sender, Action* parameters)
{
	CreateCreatureCore( Sender, parameters, CC_OFFSET | CC_CHECK_IMPASSABLE | CC_CHECK_OVERLAP);
}

void GameScript::CreateCreatureOffScreen(Scriptable* Sender, Action* parameters)
{
	CreateCreatureCore( Sender, parameters, CC_OFFSCREEN | CC_CHECK_IMPASSABLE | CC_CHECK_OVERLAP );
}

//creates copy at actor, plays animation
void GameScript::CreateCreatureObjectCopy(Scriptable* Sender, Action* parameters)
{
	CreateCreatureCore( Sender, parameters, CC_OBJECT | CC_CHECK_IMPASSABLE | CC_CHECK_OVERLAP | CC_COPY | CC_PLAY_ANIM );
}

//creates copy at absolute point
void GameScript::CreateCreatureCopyPoint(Scriptable* Sender, Action* parameters)
{
	CreateCreatureCore( Sender, parameters, CC_CHECK_IMPASSABLE | CC_CHECK_OVERLAP | CC_COPY | CC_PLAY_ANIM );
}

//this is the same, object + offset
//using this for simple createcreatureobject, (0 offsets)
//createcreatureobjecteffect may have animation
void GameScript::CreateCreatureObjectOffset(Scriptable* Sender, Action* parameters)
{
	CreateCreatureCore( Sender, parameters, CC_OBJECT | CC_CHECK_IMPASSABLE | CC_CHECK_OVERLAP | CC_PLAY_ANIM);
}

void GameScript::CreateCreatureObjectOffScreen(Scriptable* Sender, Action* parameters)
{
	CreateCreatureCore( Sender, parameters, CC_OFFSCREEN | CC_OBJECT | CC_CHECK_IMPASSABLE | CC_CHECK_OVERLAP );
}

//I think this simply removes the cursor and hides the gui without disabling scripts
//See Interface::SetCutSceneMode
void GameScript::SetCursorState(Scriptable* /*Sender*/, Action* parameters)
{
	int active = parameters->int0Parameter;

	Game *game = core->GetGame();
	game->SetControlStatus(CS_HIDEGUI, active ? BitOp::OR : BitOp::NAND);
	if (active) {
		core->GetWindowManager()->SetCursorFeedback(WindowManager::MOUSE_NONE);
	} else {
		core->GetWindowManager()->SetCursorFeedback(WindowManager::CursorFeedback(core->config.MouseFeedback));
	}
}

void GameScript::StartCutSceneMode(Scriptable* /*Sender*/, Action* /*parameters*/)
{
	core->SetCutSceneMode( true );
}

void GameScript::EndCutSceneMode(Scriptable* /*Sender*/, Action* /*parameters*/)
{
	core->SetCutSceneMode( false );
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

void GameScript::CutSceneID(Scriptable *Sender, Action* /*parameters*/)
{
	// shouldn't get called
	Log(DEBUG, "GameScript", "CutSceneID was called by {}!", Sender->GetScriptName());
}

static EffectRef fx_charm_ref = { "State:Charmed", -1 };

void GameScript::Enemy(Scriptable* Sender, Action* /*parameters*/)
{
	Actor* actor = Scriptable::As<Actor>(Sender);
	if (!actor) {
		return;
	}

	actor->fxqueue.RemoveAllEffects(fx_charm_ref);
	actor->SetBase( IE_EA, EA_ENEMY );
}

void GameScript::Ally(Scriptable* Sender, Action* /*parameters*/)
{
	Actor* actor = Scriptable::As<Actor>(Sender);
	if (!actor) {
		return;
	}

	actor->fxqueue.RemoveAllEffects(fx_charm_ref);
	actor->SetBase( IE_EA, EA_ALLY );
}

/** GemRB extension: you can replace baldur.bcs */
void GameScript::ChangeAIScript(Scriptable* Sender, Action* parameters)
{
	if (parameters->int0Parameter>=MAX_SCRIPTS) {
		return;
	}
	//clearing the queue, and checking script level was intentionally removed
	Sender->SetScript( parameters->resref0Parameter, parameters->int0Parameter, false );
}

void GameScript::ForceAIScript(Scriptable* Sender, Action* parameters)
{
	if (parameters->int0Parameter>=MAX_SCRIPTS) {
		return;
	}
	Scriptable* tar = GetScriptableFromObject(Sender, parameters->objects[1]);
	Actor* actor = Scriptable::As<Actor>(tar);
	if (!actor) {
		return;
	}

	//clearing the queue, and checking script level was intentionally removed
	actor->SetScript( parameters->resref0Parameter, parameters->int0Parameter, false );
}

// see sndslot.ids for strref1 meanings
void GameScript::SetPlayerSound(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetScriptableFromObject(Sender, parameters->objects[1]);
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
	Scriptable* tar = GetScriptableFromObject(Sender, parameters->objects[1]);
	if (!tar || tar->Type != ST_ACTOR) {
		return;
	}
	DisplayStringCoreVC(tar, parameters->int0Parameter, DS_HEAD|DS_CONSOLE);
}

void GameScript::VerbalConstant(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetScriptableFromObject(Sender, parameters->objects[1]);
	if (!tar || tar->Type != ST_ACTOR) {
		return;
	}
	DisplayStringCoreVC(tar, parameters->int0Parameter, DS_CONSOLE);
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
	const Scriptable* tar = GetScriptableFromObject(Sender, parameters->objects[1]);
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
	CreateCreatureCore(Sender, parameters, CC_CHECK_IMPASSABLE|CC_STRING1);
}

void GameScript::WaitRandom(Scriptable* Sender, Action* parameters)
{
	if (!Sender->CurrentActionState) {
		int width = parameters->int1Parameter-parameters->int0Parameter;
		if (width<2) {
			width = parameters->int0Parameter;
		} else {
			width = RAND(0, width-1) + parameters->int0Parameter;
		}
		Sender->CurrentActionState = width * core->Time.ai_update_time;
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
		Sender->CurrentActionState = parameters->int0Parameter * core->Time.ai_update_time;
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
		if (random<1) {
			random = 1;
		}
		Sender->CurrentActionState = RAND(0, random-1) + parameters->int0Parameter;
	} else {
		Sender->CurrentActionState--;
	}

	if (!Sender->CurrentActionState) {
		Sender->ReleaseCurrentAction();
	}

	assert(Sender->CurrentActionState >= 0);
}

void GameScript::MoveViewPoint(Scriptable* Sender, Action* parameters)
{
	// disable centering if anything enabled it before us (eg. LeaveAreaLUA as in movie02a.bcs)
	GameControl *gc = core->GetGameControl();
	gc->SetScreenFlags(SF_CENTERONACTOR, BitOp::NAND);
	core->timer.SetMoveViewPort( parameters->pointParameter, parameters->int0Parameter<<1, true );
	Sender->SetWait(1); // todo, blocking?
	Sender->ReleaseCurrentAction(); // todo, blocking?
}

void GameScript::MoveViewObject(Scriptable* Sender, Action* parameters)
{
	const Scriptable* scr = GetScriptableFromObject(Sender, parameters->objects[1]);
	if (!scr) {
		Sender->ReleaseCurrentAction();
		return;
	}
	core->timer.SetMoveViewPort( scr->Pos, parameters->int0Parameter<<1, true );
	Sender->SetWait(1); // todo, blocking?
	Sender->ReleaseCurrentAction(); // todo, blocking?
}

void GameScript::AddWayPoint(Scriptable* Sender, Action* parameters)
{
	Actor* actor = Scriptable::As<Actor>(Sender);
	if (!actor) {
		Sender->ReleaseCurrentAction();
		return;
	}

	actor->AddWayPoint( parameters->pointParameter );
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
		actor->SetOrientation(GetOrient(parameters->pointParameter, actor->Pos), false);
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
		actor->SetOrientation(GetOrient(parameters->pointParameter, actor->Pos), false);
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
	if (parameters->int0Parameter<=0) {
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
	if (parameters->int0Parameter>0) {
		Action *newaction = ParamCopyNoOverride(parameters);
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
		actor->WalkTo( parameters->pointParameter, 0 );
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
	Scriptable* tar = GetScriptableFromObject(Sender, parameters->objects[1]);
	if (!tar) {
		tar = Sender;
	}
	Actor* actor = Scriptable::As<Actor>(tar);
	if (!actor) {
		Sender->ReleaseCurrentAction();
		return;
	}

	Point p = CheckPointVariable(Sender, parameters->string0Parameter);
	actor->SetPosition(p, true );
	Sender->ReleaseCurrentAction();
}
/** iwd2 returntosavedlocation (with stats) */
/** pst returntosavedplace */
/** use Sender as default subject */
void GameScript::ReturnToSavedLocation(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetScriptableFromObject(Sender, parameters->objects[1], GA_NO_DEAD);
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
	Scriptable* tar = GetScriptableFromObject(Sender, parameters->objects[1], GA_NO_DEAD);
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
	Scriptable* tar = GetScriptableFromObject(Sender, parameters->objects[1], GA_NO_DEAD);
	if (!tar) {
		tar = Sender;
	}
	Actor* actor = Scriptable::As<Actor>(tar);
	if (!actor) {
		Sender->ReleaseCurrentAction();
		return;
	}
	Point p(actor->GetBase(IE_SAVEDXPOS), actor->GetBase(IE_SAVEDYPOS));
	actor->SetBase(IE_SAVEDXPOS,0);
	actor->SetBase(IE_SAVEDYPOS,0);
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
	Scriptable* tar = GetScriptableFromObject(Sender, parameters->objects[1], GA_NO_DEAD);
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
	Scriptable* tar = GetScriptableFromObject(Sender, parameters->objects[1], 0);
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
	const Scriptable* target = GetStoredActorFromObject(Sender, parameters->objects[1]);
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
	Game *game = core->GetGame();
	for (int i = 0; i < game->GetPartySize(false); i++) {
		const Actor* act = game->GetPC(i, false);
		GAMLocationEntry *gle = game->GetSavedLocationEntry(i);
		if (act && gle) {
			gle->Pos = act->Pos;
			gle->AreaResRef = act->Area;
		}
	}
}

void GameScript::RestorePartyLocation(Scriptable* /*Sender*/, Action* /*parameters*/)
{
	Game *game = core->GetGame();
	for (int i = 0; i < game->GetPartySize(false); i++) {
		Actor* act = game->GetPC( i, false );
		if (act) {
			const GAMLocationEntry *gle;
			if (game->GetSavedLocationCount() <= (unsigned int)i) {
				// no location, meaning the actor joined the party after the save
				// reuse the last valid location
				gle = game->GetSavedLocationEntry(game->GetSavedLocationCount()-1);
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
	Scriptable* target = GetScriptableFromObject(Sender, parameters->objects[1]);
	if (!target) {
		target=Sender;
	}
	if (Sender->Type==ST_ACTOR) {
		DisplayStringCore(target, ieStrRef(parameters->int0Parameter), DS_CONSOLE|DS_NONAME);
	} else {
		// Virtue calls this from the global script, but maybe Pos is ok for areas
		// set DS_CONSOLE only for ST_GLOBAL if it turns out areas don't care;
		// could also be dependent on the subtitle setting, see DisplayStringCore
		DisplayStringCore(target, ieStrRef(parameters->int0Parameter), DS_AREA|DS_CONSOLE|DS_NONAME);
	}
}

void GameScript::DisplayStringNoNameHead(Scriptable* Sender, Action* parameters)
{
	Scriptable* target = GetScriptableFromObject(Sender, parameters->objects[1]);
	if (!target) {
		target=Sender;
	}

	DisplayStringCore(target, ieStrRef(parameters->int0Parameter), DS_HEAD|DS_CONSOLE|DS_NONAME);
}

void GameScript::DisplayMessage(Scriptable* Sender, Action* parameters)
{
	DisplayStringCore(Sender, ieStrRef(parameters->int0Parameter), DS_CONSOLE|DS_NONAME);
}

//float message over target
void GameScript::DisplayStringHead(Scriptable* Sender, Action* parameters)
{
	Scriptable* target = GetScriptableFromObject(Sender, parameters->objects[1]);
	if (!target) {
		target=Sender;
		Log(WARNING, "Actions", "DisplayStringHead/FloatMessage got no target, assuming Sender!");
	}

	DisplayStringCore(target, ieStrRef(parameters->int0Parameter), DS_HEAD | DS_SPEECH);
}

void GameScript::KillFloatMessage(Scriptable* Sender, Action* parameters)
{
	Scriptable* target = GetScriptableFromObject(Sender, parameters->objects[1]);
	if (!target) {
		target=Sender;
	}
	target->overHead.Display(false);
}

void GameScript::DisplayStringHeadOwner(Scriptable* /*Sender*/, Action* parameters)
{
	const Game *game = core->GetGame();

	int i = game->GetPartySize(true);
	while(i--) {
		Actor *actor = game->GetPC(i, true);
		if (actor->inventory.HasItem(parameters->resref0Parameter, 0)) {
			DisplayStringCore(actor, ieStrRef(parameters->int0Parameter), DS_CONSOLE|DS_HEAD );
		}
	}
}

static void FloatMessageAtPoint(Scriptable* Sender, const Point& pos, const ieStrRef& msgRef)
{
	// create invisible and invicible actor to host the text (lifted from Map::GetItemByDialog)
	// replace once we have a generic solution, so we don't crowd the area
	Actor *surrogate = gamedata->GetCreature("dmhead");
	assert(surrogate);
	Map *map = Sender->GetCurrentArea();
	if (!map) return;
	map->AddActor(surrogate, true);
	surrogate->SetBase(IE_DONOTJUMP, DNJ_BIRD);
	surrogate->SetPosition(pos, 0);
	String msg = core->GetString(msgRef);
	surrogate->overHead.SetText(msg);
	surrogate->overHead.FixPos();
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
	Scriptable* target = GetScriptableFromObject(Sender, parameters->objects[1]);
	if (!target) {
		target=Sender;
		Log(ERROR, "GameScript", "DisplayStringHead/FloatMessage got no target, assuming Sender!");
	}

	const SrcVector* strList = gamedata->SrcManager.GetSrc(parameters->resref0Parameter);
	if (strList->IsEmpty()) {
		Log(ERROR, "GameScript", "Cannot display resource!");
		return;
	}
	DisplayStringCore(target, strList->RandomRef(), DS_CONSOLE | DS_HEAD);
}

//apparently this should not display over head (for actors)
void GameScript::DisplayString(Scriptable* Sender, Action* parameters)
{
	Scriptable* target = GetScriptableFromObject(Sender, parameters->objects[1]);
	if (!target) {
		target=Sender;
	}
	if (Sender->Type==ST_ACTOR) {
		DisplayStringCore(target, ieStrRef(parameters->int0Parameter), DS_CONSOLE);
	} else {
		DisplayStringCore(target, ieStrRef(parameters->int0Parameter), DS_AREA);
	}
}

//DisplayStringHead, but wait until done
void GameScript::DisplayStringWait(Scriptable* Sender, Action* parameters)
{
	ieDword gt = core->GetGame()->GameTime;
	if (Sender->CurrentActionState) {
		if (gt >= (ieDword)parameters->int2Parameter) {
			Sender->ReleaseCurrentAction();
		}
		return;
	}
	Scriptable* target = GetScriptableFromObject(Sender, parameters->objects[1]);
	if (!target) {
		target=Sender;
	}
	DisplayStringCore(target, ieStrRef(parameters->int0Parameter), DS_CONSOLE|DS_WAIT|DS_SPEECH|DS_HEAD);
	Sender->CurrentActionState = 1;
	// parameters->int2Parameter is unused here so hijack it to store the wait time
	// and make sure we wait at least one round, so strings without audio have some time to display
	unsigned long waitCounter = target->GetWait();
	parameters->int2Parameter = gt + (waitCounter > 0 ? waitCounter : core->Time.round_size);
}

void GameScript::ForceFacing(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetScriptableFromObject(Sender, parameters->objects[1]);
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
	if (parameters->int0Parameter==-1) {
		actor->SetOrientation(RandomOrientation(), false);
	} else {
		actor->SetOrientation(ClampToOrientation(parameters->int0Parameter), false);
	}
	actor->SetWait( 1 );
	Sender->ReleaseCurrentAction(); // todo, blocking?
}

void GameScript::FaceObject(Scriptable* Sender, Action* parameters)
{
	Actor* actor = Scriptable::As<Actor>(Sender);
	if (!actor) {
		Sender->ReleaseCurrentAction();
		return;
	}
	const Scriptable* target = GetScriptableFromObject(Sender, parameters->objects[1]);
	if (!target) {
		Sender->ReleaseCurrentAction();
		return;
	}
	actor->SetOrientation( GetOrient( target->Pos, actor->Pos ), false);
	actor->SetWait( 1 );
	Sender->ReleaseCurrentAction(); // todo, blocking?
}

void GameScript::FaceSavedLocation(Scriptable* Sender, Action* parameters)
{
	Scriptable* target = GetScriptableFromObject(Sender, parameters->objects[1]);
	Actor* actor = Scriptable::As<Actor>(target);
	if (!actor) {
		Sender->ReleaseCurrentAction();
		return;
	}
	if (parameters->variable0Parameter.IsEmpty()) {
		parameters->variable0Parameter = "LOCALSsavedlocation";
	}
	Point p = CheckPointVariable(target, parameters->string0Parameter);

	actor->SetOrientation ( GetOrient( p, actor->Pos ), false);
	actor->SetWait( 1 );
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
	if (parameters->int1Parameter==1) {
		force = true;
	} else {
		force = false;
	}
	int ret = core->GetMusicMgr()->SwitchPlayList( poi, force );
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
	const Map *map = Sender->GetCurrentArea();
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
	const Map *map = Sender->GetCurrentArea();
	if (!map) return;
	map->PlayAreaSong(3, true, true);
}

/*iwd2 can set an areasong slot*/
void GameScript::SetMusic(Scriptable* Sender, Action* parameters)
{
	// iwd2 allows setting all 10 slots (musics.ids), though there is no evidence they are used
	// int1Parameter is from music.ids or songlist.ids (ees)
	if (parameters->int0Parameter >= 10) return;
	Map *map = Sender->GetCurrentArea();
	if (!map) return;
	map->SongList[parameters->int0Parameter] = parameters->int1Parameter;
}

//optional integer parameter (isSpeech)
void GameScript::PlaySound(Scriptable* Sender, Action* parameters)
{
	Log(MESSAGE, "Actions", "PlaySound({})", parameters->string0Parameter);
	core->GetAudioDrv()->Play(parameters->string0Parameter, SFX_CHAN_CHAR0, Sender->Pos, parameters->int0Parameter ? GEM_SND_SPEECH : 0);
}

void GameScript::PlaySoundPoint(Scriptable* /*Sender*/, Action* parameters)
{
	Log(MESSAGE, "Actions", "PlaySound({})", parameters->string0Parameter);
	core->GetAudioDrv()->Play(parameters->string0Parameter, SFX_CHAN_ACTIONS,
		parameters->pointParameter);
}

void GameScript::PlaySoundNotRanged(Scriptable* /*Sender*/, Action* parameters)
{
	Log(MESSAGE, "Actions", "PlaySound({})", parameters->string0Parameter);
	core->GetAudioDrv()->Play(parameters->string0Parameter, SFX_CHAN_ACTIONS, Point(), GEM_SND_RELATIVE);
}

void GameScript::Continue(Scriptable* /*Sender*/, Action* /*parameters*/)
{
}

// creates area vvc at position of object
void GameScript::CreateVisualEffectObject(Scriptable* Sender, Action* parameters)
{
	const Scriptable* tar = GetScriptableFromObject(Sender, parameters->objects[1]);
	if (!tar) {
		return;
	}
	CreateVisualEffectCore(tar, tar->Pos, parameters->resref0Parameter, parameters->int0Parameter);
}

// creates sticky vvc on actor or normal animation on object
void GameScript::CreateVisualEffectObjectSticky(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetScriptableFromObject(Sender, parameters->objects[1]);
	if (!tar) {
		return;
	}
	if (tar->Type==ST_ACTOR) {
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
	// needeed in pst #532, but softly breaks bg2 #1179
	if (actor == core->GetCutSceneRunner() && core->HasFeature(GF_PST_STATE_FLAGS)) {
		core->SetCutSceneMode(false);
	}
}

void GameScript::ScreenShake(Scriptable* Sender, Action* parameters)
{
	if (parameters->int1Parameter) { //IWD2 has a different profile
		Point p(parameters->int1Parameter, parameters->int2Parameter);
		core->timer.SetScreenShake(p, parameters->int0Parameter);
	} else {
		core->timer.SetScreenShake(parameters->pointParameter, parameters->int0Parameter);
	}
	Sender->SetWait( parameters->int0Parameter );
	Sender->ReleaseCurrentAction(); // todo, blocking?
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
		gc->SetScreenFlags(SF_CENTERONACTOR|SF_ALWAYSCENTER, BitOp::OR);
	}
}

void GameScript::UnlockScroll(Scriptable* /*Sender*/, Action* /*parameters*/)
{
	GameControl* gc = core->GetGameControl();
	if (gc) {
		gc->SetScreenFlags(SF_CENTERONACTOR|SF_ALWAYSCENTER, BitOp::NAND);
	}
}

//no string, increase talkcount, no interrupt
void GameScript::Dialogue(Scriptable* Sender, Action* parameters)
{
	BeginDialog( Sender, parameters, BD_SOURCE | BD_TALKCOUNT | BD_CHECKDIST );
}

void GameScript::DialogueForceInterrupt(Scriptable* Sender, Action* parameters)
{
	BeginDialog( Sender, parameters, BD_SOURCE | BD_TALKCOUNT | BD_INTERRUPT );
}

// not in IESDP but this one should affect ambients
void GameScript::SoundActivate(Scriptable* /*Sender*/, Action* parameters)
{
	AmbientMgr * ambientmgr = core->GetAudioDrv()->GetAmbientMgr();
	if (parameters->int0Parameter) {
		ambientmgr->Activate(parameters->objects[1]->objectName);
	} else {
		ambientmgr->Deactivate(parameters->objects[1]->objectName);
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
	Scriptable* tar = GetScriptableFromObject(Sender, parameters->objects[1], GA_NO_DEAD);
	Door* door = Scriptable::As<Door>(tar);
	if (!door) {
		return;
	}

	int state = parameters->int0Parameter;
	if(door) {
		door->ToggleTiles(state); /* default is false for playsound */
	}
}

void GameScript::StaticStart(Scriptable* Sender, Action* parameters)
{
	AreaAnimation *anim = Sender->GetCurrentArea()->GetAnimation(parameters->objects[1]->objectNameVar);
	if (!anim) {
		Log(WARNING, "Actions", "Script error: No Animation Named \"{}\"", parameters->objects[1]->objectName);
		return;
	}
	anim->Flags &=~A_ANI_PLAYONCE;
}

void GameScript::StaticStop(Scriptable* Sender, Action* parameters)
{
	AreaAnimation *anim = Sender->GetCurrentArea()->GetAnimation(parameters->objects[1]->objectNameVar);
	if (!anim) {
		Log(WARNING, "Actions", "Script error: No Animation Named \"{}\"", parameters->objects[1]->objectName);
		return;
	}
	anim->Flags |= A_ANI_PLAYONCE;
}

void GameScript::StaticPalette(Scriptable* Sender, Action* parameters)
{
	AreaAnimation *anim = Sender->GetCurrentArea()->GetAnimation(parameters->objects[1]->objectNameVar);
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
		tar = GetScriptableFromObject(Sender, parameters->objects[1]);
	} else {
		tar=Sender;
	}
	Actor* actor = Scriptable::As<Actor>(tar);
	if (!actor) {
		return;
	}

	actor->SetStance( parameters->int0Parameter );
	int delay = parameters->int1Parameter;
	actor->SetWait( delay );
}

//waitanimation: waiting while animation of target is of a certain type
void GameScript::WaitAnimation(Scriptable* Sender, Action* parameters)
{
	const Scriptable* tar = GetScriptableFromObject(Sender, parameters->objects[1]);
	if (!tar) {
		tar=Sender;
	}
	const Actor* actor = Scriptable::As<Actor>(tar);
	if (!actor) {
		return;
	}

	// HACK HACK: avoid too long waits due to buggy AI evaluation
	if (actor->GetStance() != parameters->int0Parameter || parameters->int1Parameter > (signed)core->Time.round_size) {
		Sender->ReleaseCurrentAction();
		return;
	}
	parameters->int1Parameter++;
}

// the spell target and attack target are different only in iwd2
void GameScript::SetMyTarget(Scriptable* Sender, Action* parameters)
{
	const Scriptable* tar = GetScriptableFromObject(Sender, parameters->objects[1]);
	if (!tar) {
		// we got called with Nothing to invalidate the target
		Sender->MyTarget = 0;
		return;
	}
	Sender->MyTarget = tar->GetGlobalID();
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
	Scriptable* tar = GetScriptableFromObject(Sender, parameters->objects[1]);
	Actor* target = Scriptable::As<Actor>(tar);
	if (!target) {
		return;
	}
	target->SetDialog(parameters->resref0Parameter);
}

//string0, no interrupt, talkcount increased
void GameScript::StartDialogue(Scriptable* Sender, Action* parameters)
{
	BeginDialog( Sender, parameters, BD_STRING0 | BD_TALKCOUNT | BD_SETDIALOG );
}

//string0, no interrupt, talkcount increased, don't set default
//optionally item name is used
void GameScript::StartDialogueOverride(Scriptable* Sender, Action* parameters)
{
	int flags = BD_STRING0 | BD_TALKCOUNT;

	if (parameters->int2Parameter) {
		flags|=BD_ITEM;
	}
	BeginDialog( Sender, parameters, flags );
}

//string0, no interrupt, talkcount increased, don't set default
//optionally item name is used
void GameScript::StartDialogueOverrideInterrupt(Scriptable* Sender,
	Action* parameters)
{
	int flags = BD_STRING0 | BD_TALKCOUNT | BD_INTERRUPT;

	if (parameters->int2Parameter) {
		flags|=BD_ITEM;
	}
	BeginDialog( Sender, parameters, flags );
}

//start talking to oneself
void GameScript::PlayerDialogue(Scriptable* Sender, Action* parameters)
{
	BeginDialog( Sender, parameters, BD_RESERVED | BD_OWN );
}

//we hijack this action for the player initiated dialogue
void GameScript::NIDSpecial1(Scriptable* Sender, Action* parameters)
{
	BeginDialog( Sender, parameters, BD_INTERRUPT | BD_TARGET /*| BD_NUMERIC*/ | BD_TALKCOUNT | BD_CHECKDIST );
}

void GameScript::NIDSpecial2(Scriptable* Sender, Action* /*parameters*/)
{
	const Actor* actor = Scriptable::As<Actor>(Sender);
	if (!actor) {
		Sender->ReleaseCurrentAction();
		return;
	}
	const Game *game = core->GetGame();
	if (!game->EveryoneStopped() ) {
		//wait for a while
		Sender->SetWait(1 * core->Time.ai_update_time);
		return;
	}

	Map* map = actor->GetCurrentArea();
	if (!game->EveryoneNearPoint(map, actor->Pos, ENP_CANMOVE)) {
		//we abort the command, everyone should be here
		if (map->LastGoCloser < game->Ticks) {
			displaymsg->DisplayConstantString(STR_WHOLEPARTY, GUIColors::WHITE);
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
	bool keyAreaVisited = core->HasFeature(GF_TEAM_MOVEMENT) && CheckVariable(Sender, "AR0500_Visited", "GLOBAL") == 1;
	if (direction == WMPDirection::NONE && !keyAreaVisited) {
		Sender->ReleaseCurrentAction();
		return;
	}
	if (direction == WMPDirection::NONE && keyAreaVisited) {
		// FIXME: not ideal, pst uses the infopoint links (ip->EntranceName), so direction doesn't matter
		// but we're not travelling through them (the whole point of the world map), so how to pick a good entrance?
		// DestEntryPoint is all zeroes, pst just didn't use it
		direction = WMPDirection::WEST;
	}
	core->GetDictionary()->SetAt("Travel", (ieDword) direction);
	core->GetGUIScriptEngine()->RunFunction( "GUIMA", "OpenTravelWindow" );
	//sorry, i have absolutely no idea when i should do this :)
	Sender->ReleaseCurrentAction();
}

void GameScript::StartDialogueInterrupt(Scriptable* Sender, Action* parameters)
{
	BeginDialog( Sender, parameters,
		BD_STRING0 | BD_INTERRUPT | BD_TALKCOUNT | BD_SETDIALOG );
}

//No string, flags:0
void GameScript::StartDialogueNoSet(Scriptable* Sender, Action* parameters)
{
	BeginDialog( Sender, parameters, BD_TALKCOUNT | BD_SOURCE );
}

void GameScript::StartDialogueNoSetInterrupt(Scriptable* Sender,
	Action* parameters)
{
	BeginDialog( Sender, parameters, BD_TALKCOUNT | BD_SOURCE | BD_INTERRUPT );
}

//no talkcount, using banter dialogs
//probably banter dialogs are random, like rumours!
//no, they aren't, but they increase interactcount
void GameScript::Interact(Scriptable* Sender, Action* parameters)
{
	BeginDialog( Sender, parameters, BD_INTERACT | BD_NOEMPTY );
}

static unsigned int FindNearPoint(Scriptable* Sender, Point *&p1, Point *&p2)
{
	unsigned int distance1 = Distance(*p1, Sender);
	unsigned int distance2 = Distance(*p2, Sender);
	if (distance1 <= distance2) {
		return distance1;
	} else {
		Point *tmp = p1;
		p1 = p2;
		p2 = tmp;
		return distance2;
	}
}

//this is an immediate action without checking Sender
void GameScript::DetectSecretDoor(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetScriptableFromObject(Sender, parameters->objects[1], GA_NO_DEAD);
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
	Scriptable* tar = GetScriptableFromObject(Sender, parameters->objects[1]);
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
	Scriptable* tar = GetScriptableFromObject(Sender, parameters->objects[1]);
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
	Scriptable* tar = GetScriptableFromObject(Sender, parameters->objects[1]);
	Door* door = Scriptable::As<Door>(tar);
	if (!door) {
		return;
	}

	// two dialog states in pst (and nothing else) use "FALSE" (yes, quoted)
	// they're on a critical path so we have to handle this data bug ourselves
	if (parameters->int0Parameter == -1) {
		parameters->int0Parameter = 0;
	}
	door->SetDoorLocked( parameters->int0Parameter!=0, false);
}

void GameScript::SetDoorFlag(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetScriptableFromObject(Sender, parameters->objects[1]);
	Door* door = Scriptable::As<Door>(tar);
	if (!door) {
		return;
	}

	ieDword flag = parameters->int0Parameter;

	//these are special flags
	if (flag&DOOR_LOCKED) {
		flag&=~DOOR_LOCKED;
		door->SetDoorLocked(parameters->int1Parameter!=0, false);
	}
	if (flag&DOOR_OPEN) {
		flag&=~DOOR_OPEN;
		door->SetDoorOpen(parameters->int1Parameter!=0, false, 0);
	}
	// take care of iwd2 flag bit differences as in AREIMporter's FixIWD2DoorFlags
	// ... it matters for exactly 1 user from the original data (20ctord3.bcs)
	if (core->HasFeature(GF_3ED_RULES) && flag == DOOR_KEY) {
		flag = DOOR_TRANSPARENT;
	}

	if (parameters->int1Parameter) {
		door->Flags|=flag;
	} else {
		door->Flags&=~flag;
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
	Scriptable* tar = GetScriptableFromObject(Sender, parameters->objects[1]);
	if (!tar) {
		Sender->ReleaseCurrentAction();
		return;
	}

	unsigned int distance;
	Point *p, *otherp;
	Door *door = NULL;
	Container *container = NULL;
	InfoPoint *trigger = NULL;
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
		p = door->toOpen;
		otherp = door->toOpen+1;
		distance = FindNearPoint( Sender, p, otherp);
		flags = door->Trapped && door->TrapDetected;
		break;
	case ST_CONTAINER:
		container = static_cast<Container*>(tar);
		p = &container->Pos;
		otherp = p;
		distance = Distance(*p, Sender);
		flags = container->Trapped && container->TrapDetected;
		break;
	case ST_PROXIMITY:
		trigger = (InfoPoint *) tar;
		// this point is incorrect! will cause actor to enter trap
		// need to find a point using trigger->outline
		p = &trigger->Pos;
		otherp = p;
		distance = Distance(tar, Sender);
		flags = trigger->Trapped && trigger->TrapDetected && trigger->CanDetectTrap();
		actor->SetDisarmingTrap(trigger->GetGlobalID());
		break;
	default:
		Sender->ReleaseCurrentAction();
		return;
	}
	actor->SetOrientation( GetOrient( *otherp, actor->Pos ), false);
	if (distance <= MAX_OPERATING_DISTANCE) {
		if (flags) {
			switch(type) {
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
			//displaymsg->DisplayString(STR_NOT_TRAPPED);
		}
	} else {
		MoveNearerTo(Sender, *p, MAX_OPERATING_DISTANCE,0);
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
	Scriptable* tar = GetScriptableFromObject(Sender, parameters->objects[1]);
	if (!tar) {
		Sender->ReleaseCurrentAction();
		return;
	}
	unsigned int distance;
	Point *p, *otherp;
	Door *door = NULL;
	Container *container = NULL;
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
		p = door->toOpen;
		otherp = door->toOpen+1;
		distance = FindNearPoint( Sender, p, otherp);
		flags = door->Flags&DOOR_LOCKED;
		break;
	case ST_CONTAINER:
		container = static_cast<Container*>(tar);
		p = &container->Pos;
		otherp = p;
		distance = Distance(*p, Sender);
		flags = container->Flags&CONT_LOCKED;
		break;
	default:
		Sender->ReleaseCurrentAction();
		return;
	}

	actor->SetOrientation( GetOrient( *otherp, actor->Pos ), false);
	if (distance <= MAX_OPERATING_DISTANCE) {
		if (flags) {
			if (type==ST_DOOR) {
				door->TryPickLock(actor);
			} else {
				container->TryPickLock(actor);
			}
		} else {
			//notlocked
			//displaymsg->DisplayString(STR_NOT_LOCKED);
		}
	} else {
		MoveNearerTo(Sender, *p, MAX_OPERATING_DISTANCE,0);
		return;
	}
	Sender->SetWait(1);
	Sender->ReleaseCurrentAction();
}

void GameScript::OpenDoor(Scriptable* Sender, Action* parameters) {
	Scriptable* tar = GetScriptableFromObject(Sender, parameters->objects[1]);
	Door* door = Scriptable::As<Door>(tar);
	if (!door) {
		return;
	}
	int gid = Sender->GetGlobalID();
	// no idea if this is right, or whether OpenDoor/CloseDoor should allow opening
	// of all doors, or some doors, or whether it should still check for non-actors
	Actor* actor = Scriptable::As<Actor>(Sender);
	if (actor) {
		actor->SetModal(MS_NONE);
		if (!door->TryUnlock(actor)) {
			return;
		}
	}
	door->SetDoorOpen(true, false, gid, false);
	Sender->ReleaseCurrentAction();
}

void GameScript::CloseDoor(Scriptable* Sender, Action* parameters) {
	Scriptable* tar = GetScriptableFromObject(Sender, parameters->objects[1]);
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

void GameScript::ToggleDoor(Scriptable* Sender, Action* /*parameters*/)
{
	Actor* actor = Scriptable::As<Actor>(Sender);
	if (!actor) {
		Sender->ReleaseCurrentAction();
		return;
	}
	actor->SetModal(MS_NONE);

	Door* door = actor->GetCurrentArea()->GetDoorByGlobalID(actor->TargetDoor);
	if (!door) {
		Sender->ReleaseCurrentAction();
		return;
	}
	unsigned int distance;
	Point *p = door->toOpen;
	Point *otherp = door->toOpen+1;
	distance = FindNearPoint( Sender, p, otherp);
	if (distance <= MAX_OPERATING_DISTANCE) {
		actor->SetOrientation( GetOrient( *otherp, actor->Pos ), false);
		if (!door->TryUnlock(actor)) {
			displaymsg->DisplayConstantString(STR_DOORLOCKED, GUIColors::LIGHTGREY, door);
			door->AddTrigger(TriggerEntry(trigger_failedtoopen, actor->GetGlobalID()));

			//playsound unsuccessful opening of door
			core->PlaySound(door->IsOpen() ? DS_CLOSE_FAIL : DS_OPEN_FAIL, SFX_CHAN_ACTIONS);
			Sender->ReleaseCurrentAction();
			actor->TargetDoor = 0;
			return; //don't open door
		}

		//trap scripts are triggered by SetDoorOpen
		door->SetDoorOpen(!door->IsOpen(), false, actor->GetGlobalID());
	} else {
		MoveNearerTo(Sender, *p, MAX_OPERATING_DISTANCE,0);
		return;
	}
	Sender->SetWait(1);
	Sender->ReleaseCurrentAction();
	actor->TargetDoor = 0;
}

void GameScript::ContainerEnable(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetScriptableFromObject(Sender, parameters->objects[1]);
	Container* cnt = Scriptable::As<Container>(tar);
	if (!cnt) {
		return;
	}

	if (parameters->int0Parameter) {
		cnt->Flags&=~CONT_DISABLED;
	} else {
		cnt->Flags|=CONT_DISABLED;
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

	if (actor->Persistent() || !CreateMovementEffect(actor, parameters->resref0Parameter, parameters->pointParameter, parameters->int0Parameter) ) {
		MoveBetweenAreasCore(actor, parameters->resref0Parameter, parameters->pointParameter, parameters->int0Parameter, true);
	}
}

//spell is depleted, casting time is calculated, interruptible
//FIXME The caster must meet the level requirements as set in the spell file
void GameScript::Spell(Scriptable* Sender, Action* parameters)
{
	SpellCore(Sender, parameters, SC_NO_DEAD|SC_RANGE_CHECK|SC_DEPLETE|SC_AURA_CHECK);
}

//spell is depleted, casting time is calculated, interruptible
//FIXME The caster must meet the level requirements as set in the spell file
void GameScript::SpellPoint(Scriptable* Sender, Action* parameters)
{
	SpellPointCore(Sender, parameters, SC_RANGE_CHECK|SC_DEPLETE|SC_AURA_CHECK);
}

//spell is not depleted (doesn't need to be memorised or known)
//casting time is calculated, interruptible
//FIXME The caster must meet the level requirements as set in the spell file
void GameScript::SpellNoDec(Scriptable* Sender, Action* parameters)
{
	SpellCore(Sender, parameters, SC_NO_DEAD|SC_RANGE_CHECK|SC_AURA_CHECK);
}

//spell is not depleted (doesn't need to be memorised or known)
//casting time is calculated, interruptible
//FIXME The caster must meet the level requirements as set in the spell file
void GameScript::SpellPointNoDec(Scriptable* Sender, Action* parameters)
{
	SpellPointCore(Sender, parameters, SC_RANGE_CHECK|SC_AURA_CHECK);
}

//spell is not depleted (doesn't need to be memorised or known)
//casting time is calculated, not interruptable
//FIXME The caster must meet the level requirements as set in the spell file
void GameScript::ForceSpell(Scriptable* Sender, Action* parameters)
{
	// gemrb extension for internal use
	if (parameters->int1Parameter) {
		SpellCore(Sender, parameters, SC_NOINTERRUPT | SC_SETLEVEL);
	} else {
		SpellCore(Sender, parameters, SC_NOINTERRUPT);
	}
}

void GameScript::ForceSpellRange(Scriptable* Sender, Action* parameters)
{
	SpellCore(Sender, parameters, SC_NOINTERRUPT|SC_RANGE_CHECK);
}

//spell is not depleted (doesn't need to be memorised or known)
//casting time is calculated, not interruptable
//FIXME The caster must meet the level requirements as set in the spell file
void GameScript::ForceSpellPoint(Scriptable* Sender, Action* parameters)
{
	// gemrb extension for internal use
	if (parameters->int1Parameter) {
		SpellPointCore(Sender, parameters, SC_NOINTERRUPT | SC_SETLEVEL);
	} else {
		SpellPointCore(Sender, parameters, SC_NOINTERRUPT);
	}
}

void GameScript::ForceSpellPointRange(Scriptable* Sender, Action* parameters)
{
	SpellPointCore(Sender, parameters, SC_NOINTERRUPT|SC_RANGE_CHECK);
}

//ForceSpell with zero casting time
//zero casting time, no depletion, not interruptable
//FIXME The caster must meet the level requirements as set in the spell file
void GameScript::ReallyForceSpell(Scriptable* Sender, Action* parameters)
{
	SpellCore(Sender, parameters, SC_NOINTERRUPT|SC_SETLEVEL|SC_INSTANT);
}

//ForceSpellPoint with zero casting time
//zero casting time, no depletion (finish casting at point), not interruptable
//no CFB
//FIXME The caster must meet the level requirements as set in the spell file
void GameScript::ReallyForceSpellPoint(Scriptable* Sender, Action* parameters)
{
	SpellPointCore(Sender, parameters, SC_NOINTERRUPT|SC_SETLEVEL|SC_INSTANT);
}

// this differs from ReallyForceSpell that this one allows dead Sender casting
// zero casting time, no depletion
void GameScript::ReallyForceSpellDead(Scriptable* Sender, Action* parameters)
{
	// the difference from ReallyForceSpell is handled by the lack of AF_ALIVE being set
	SpellCore(Sender, parameters, SC_NOINTERRUPT|SC_SETLEVEL|SC_INSTANT);
}

void GameScript::Activate(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetScriptableFromObject(Sender, parameters->objects[1]);
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
	if (tar->Type == ST_PROXIMITY || tar->Type == ST_TRAVEL || tar->Type==ST_TRIGGER) {
		static_cast<InfoPoint*>(tar)->Flags &= ~TRAP_DEACTIVATED;
		return;
	}
}

void GameScript::Deactivate(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetScriptableFromObject(Sender, parameters->objects[1]);
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
	if (tar->Type == ST_CONTAINER && !core->HasFeature(GF_SPECIFIC_DMG_BONUS)) {
		static_cast<Container*>(tar)->Flags |= CONT_DISABLED;
		return;
	}

	//and regions
	if (tar->Type == ST_PROXIMITY || tar->Type == ST_TRAVEL || tar->Type==ST_TRIGGER) {
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
	core->GetGame()->AddNPC( act );
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
	slot = core->GetGame()->InStore( act );
	if (slot >= 0) {
		core->GetGame()->DelNPC( slot );
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
		act->SetBase(IE_GOLD, act->GetBase(IE_GOLD)-gold);
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
		act->SetBase(IE_GOLD, act->GetBase(IE_GOLD)-gold);
	}
	core->GetGame()->AddGold(gold);
}

void GameScript::DestroyPartyGold(Scriptable* /*Sender*/, Action* parameters)
{
	int gold = core->GetGame()->PartyGold;
	if (gold>parameters->int0Parameter) {
		gold=parameters->int0Parameter;
	}
	core->GetGame()->AddGold(-gold);
}

void GameScript::TakePartyGold(Scriptable* Sender, Action* parameters)
{
	ieDword gold = core->GetGame()->PartyGold;
	if (gold>(ieDword) parameters->int0Parameter) {
		gold=(ieDword) parameters->int0Parameter;
	}
	core->GetGame()->AddGold((ieDword) -(int) gold);
	Actor* act = Scriptable::As<Actor>(Sender);
	// fixes PST limlim shop, partymembers don't receive the taken gold
	if (act && !act->InParty) {
		act->SetBase(IE_GOLD, act->GetBase(IE_GOLD) + gold);
	}
}

void GameScript::AddXPObject(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetScriptableFromObject(Sender, parameters->objects[1]);
	Actor* actor = Scriptable::As<Actor>(tar);
	if (!actor) {
		return;
	}
	int xp = parameters->int0Parameter;
	core->GetTokenDictionary()->SetAtAsString("EXPERIENCEAMOUNT", xp);
	if (core->HasFeedback(FT_MISC)) {
		if (DisplayMessage::HasStringReference(STR_GOTQUESTXP)) {
			displaymsg->DisplayConstantStringName(STR_GOTQUESTXP, GUIColors::XPCHANGE, actor);
		} else {
			displaymsg->DisplayConstantStringValue(STR_GOTXP, GUIColors::XPCHANGE, (ieDword)xp);
		}
	}

	//normally the second parameter is 0, but it may be handy to have control over that (See SX_* flags)
	actor->AddExperience(xp, parameters->int1Parameter);
	core->PlaySound(DS_GOTXP, SFX_CHAN_ACTIONS);
}

void GameScript::AddXP2DA(Scriptable* /*Sender*/, Action* parameters)
{
	AddXPCore(parameters);
}

void GameScript::AddXPVar(Scriptable* /*Sender*/, Action* parameters)
{
	AddXPCore(parameters, true);
}

void GameScript::AddExperienceParty(Scriptable* /*Sender*/, Action* parameters)
{
	core->GetGame()->ShareXP(parameters->int0Parameter, SX_DIVIDE);
	core->PlaySound(DS_GOTXP, SFX_CHAN_ACTIONS);
}

//this needs moncrate.2da, but otherwise independent from GF_CHALLENGERATING
void GameScript::AddExperiencePartyCR(Scriptable* /*Sender*/, Action* parameters)
{
	core->GetGame()->ShareXP(parameters->int0Parameter, SX_DIVIDE|SX_CR);
}

void GameScript::AddExperiencePartyGlobal(Scriptable* Sender, Action* parameters)
{
	ieDword xp = CheckVariable( Sender, parameters->string0Parameter, parameters->string1Parameter );
	core->GetGame()->ShareXP(xp, SX_DIVIDE);
	core->PlaySound(DS_GOTXP, SFX_CHAN_ACTIONS);
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
	Scriptable* tar = GetScriptableFromObject(Sender, parameters->objects[1]);
	Actor* act = Scriptable::As<Actor>(tar);
	if (!act) {
		return;
	}
	act->SetBase(IE_MORALE, parameters->int0Parameter);
}

void GameScript::MoraleInc(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetScriptableFromObject(Sender, parameters->objects[1]);
	Actor* act = Scriptable::As<Actor>(tar);
	if (!act) {
		return;
	}
	act->SetBase(IE_MORALE, act->GetBase(IE_MORALE) + parameters->int0Parameter);
}

void GameScript::MoraleDec(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetScriptableFromObject(Sender, parameters->objects[1]);
	Actor* act = Scriptable::As<Actor>(tar);
	if (!act) {
		return;
	}
	act->SetBase(IE_MORALE, act->GetBase(IE_MORALE) - parameters->int0Parameter);
}

void GameScript::JoinParty(Scriptable* Sender, Action* parameters)
{
	Actor* act = Scriptable::As<Actor>(Sender);
	if (!act) {
		return;
	}
	// make sure we're in the same area, otherwise Dynaheir joins when Minsc does
	// but she's in another area and needs to be rescued first!
	Game *game = core->GetGame();
	if (act->GetCurrentArea() != game->GetCurrentArea()) {
		return;
	}

	/* calling this, so it is simpler to change */
	/* i'm not sure this is required here at all */
	SetBeenInPartyFlags(Sender, parameters);
	act->SetBase( IE_EA, EA_PC );
	if (core->HasFeature( GF_HAS_DPLAYER )) {
		/* we must reset various existing scripts */
		act->SetScript( "DEFAULT", AI_SCRIPT_LEVEL, true );
		act->SetScript(ResRef(), SCR_RACE, true);
		act->SetScript(ResRef(), SCR_GENERAL, true);
		act->SetScript( "DPLAYER2", SCR_DEFAULT, false );
	}
	AutoTable pdtable = gamedata->LoadTable("pdialog");
	if (pdtable) {
		const ieVariable& scriptname = act->GetScriptName();
		ResRef resRef;
		//set dialog only if we got a row
		if (pdtable->GetRowIndex( scriptname ) != TableMgr::npos) {
			if (game->Expansion == GAME_TOB) {
				resRef = pdtable->QueryField(scriptname, "25JOIN_DIALOG_FILE");
			} else {
				resRef = pdtable->QueryField(scriptname, "JOIN_DIALOG_FILE");
			}
			act->SetDialog(resRef);
		}
	}
	game->JoinParty( act, JP_JOIN );
}

void GameScript::LeaveParty(Scriptable* Sender, Action* /*parameters*/)
{
	Actor* act = Scriptable::As<Actor>(Sender);
	if (!act) {
		return;
	}
	core->GetGame()->LeaveParty( act );
}

//HideCreature hides only the visuals of a creature
//(feet circle and avatar)
//the scripts of the creature are still running
//iwd2 stores this flag in the MC field (MC_HIDDEN)
void GameScript::HideCreature(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetScriptableFromObject(Sender, parameters->objects[1]);
	Actor* actor = Scriptable::As<Actor>(tar);
	if (!actor) {
		return;
	}

	actor->SetBase(IE_AVATARREMOVAL, parameters->int0Parameter);
}

//i have absolutely no idea why this is needed when we have HideCreature
void GameScript::ForceHide(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetScriptableFromObject(Sender, parameters->objects[1]);
	if (!tar) {
		tar=Sender;
	}
	Actor* actor = Scriptable::As<Actor>(tar);
	if (!actor) {
		return;
	}
	actor->SetBase(IE_AVATARREMOVAL, 1);
}

void GameScript::ForceLeaveAreaLUA(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetScriptableFromObject(Sender, parameters->objects[1]);
	Actor* actor = Scriptable::As<Actor>(tar);
	if (!actor) {
		return;
	}

	//the LoadMos ResRef may be empty
	if (!parameters->resref1Parameter.IsEmpty()) {
		core->GetGame()->LoadMos = parameters->resref1Parameter;
	}
	if (actor->Persistent() || !CreateMovementEffect(actor, parameters->resref0Parameter, parameters->pointParameter, parameters->int0Parameter) ) {
		MoveBetweenAreasCore(actor, parameters->resref0Parameter, parameters->pointParameter, parameters->int0Parameter, true);
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
	if (actor->Persistent() || !CreateMovementEffect(actor, parameters->resref0Parameter, parameters->pointParameter, parameters->int0Parameter) ) {
		MoveBetweenAreasCore(actor, parameters->resref0Parameter, parameters->pointParameter, parameters->int0Parameter, true);
	}
}

//this is a blocking action, because we have to move to the Entry
void GameScript::LeaveAreaLUAEntry(Scriptable* Sender, Action* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		Sender->ReleaseCurrentAction();
		return;
	}
	Game *game = core->GetGame();
	if (!parameters->resref1Parameter.IsEmpty()) {
		game->LoadMos = parameters->resref1Parameter;
	}
	Point p = GetEntryPoint(parameters->resref0Parameter, parameters->resref1Parameter);
	if (p.IsInvalid()) {
		Sender->ReleaseCurrentAction();
		return;
	}
	parameters->pointParameter=p;
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
	core->GetTokenDictionary()->SetAt(parameters->string0Parameter, str);
}

//Assigns a numeric variable to the token
void GameScript::SetTokenGlobal(Scriptable* Sender, Action* parameters)
{
	ieDword value = CheckVariable( Sender, parameters->string0Parameter );
	core->GetTokenDictionary()->SetAtAsString(parameters->string1Parameter, value);
}

//Assigns the target object's name (not scriptname) to the token
void GameScript::SetTokenObject(Scriptable* Sender, Action* parameters)
{
	const Scriptable* tar = GetScriptableFromObject(Sender, parameters->objects[1]);
	const Actor* actor = Scriptable::As<const Actor>(tar);
	if (!actor) {
		return;
	}
	core->GetTokenDictionary()->SetAt(parameters->string0Parameter, actor->GetShortName());
}

void GameScript::PlayDead(Scriptable* Sender, Action* parameters)
{
	Actor* actor = Scriptable::As<Actor>(Sender);
	if (!actor) {
		Sender->ReleaseCurrentAction();
		return;
	}

	actor->CurrentActionInterruptable = false;
	if (!Sender->CurrentActionTicks && parameters->int0Parameter) {
		// set countdown on first run
		Sender->CurrentActionState = parameters->int0Parameter;
		actor->SetStance( IE_ANI_DIE );
	}
	if (Sender->CurrentActionState <= 0) {
		actor->SetStance( IE_ANI_GET_UP );
		Sender->ReleaseCurrentAction();
		return;
	}
	actor->CurrentActionState--;
}

void GameScript::PlayDeadInterruptable(Scriptable* Sender, Action* parameters)
{
	Actor* actor = Scriptable::As<Actor>(Sender);
	if (!actor) {
		Sender->ReleaseCurrentAction();
		return;
	}

	if (!Sender->CurrentActionTicks && parameters->int0Parameter) {
		// set countdown on first run
		Sender->CurrentActionState = parameters->int0Parameter;
		actor->SetStance( IE_ANI_DIE );
	}
	if (Sender->CurrentActionState <= 0) {
		actor->SetStance( IE_ANI_GET_UP );
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
	actor->SetStance( IE_ANI_ATTACK );
	actor->SetWait(core->Time.ai_update_time * 2);
}

/* this is not correct, see #92 */
void GameScript::SwingOnce(Scriptable* Sender, Action* /*parameters*/)
{
	Actor* actor = Scriptable::As<Actor>(Sender);
	if (!actor) {
		return;
	}
	actor->SetStance( IE_ANI_ATTACK );
	actor->SetWait(core->Time.ai_update_time);
}

void GameScript::Recoil(Scriptable* Sender, Action* /*parameters*/)
{
	Actor* actor = Scriptable::As<Actor>(Sender);
	if (!actor) {
		return;
	}
	actor->SetStance( IE_ANI_DAMAGE );
	actor->SetWait( 1 );
}

void GameScript::AnkhegEmerge(Scriptable* Sender, Action* /*parameters*/)
{
	Actor* actor = Scriptable::As<Actor>(Sender);
	if (!actor) {
		return;
	}
	if (actor->GetStance()!=IE_ANI_EMERGE) {
		actor->SetStance( IE_ANI_EMERGE );
		actor->SetWait( 1 );
	}
}

void GameScript::AnkhegHide(Scriptable* Sender, Action* /*parameters*/)
{
	Actor* actor = Scriptable::As<Actor>(Sender);
	if (!actor) {
		return;
	}
	if (actor->GetStance()!=IE_ANI_HIDE) {
		actor->SetStance( IE_ANI_HIDE );
		actor->SetWait( 1 );
	}
}

void GameScript::GlobalSetGlobal(Scriptable* Sender, Action* parameters)
{
	ieDword value = CheckVariable( Sender, parameters->string0Parameter );
	SetVariable( Sender, parameters->string1Parameter, value );
}

/* adding the second variable to the first, they must be GLOBAL */
void GameScript::AddGlobals(Scriptable* Sender, Action* parameters)
{
	ieDword value1 = CheckVariable( Sender, parameters->string0Parameter, "GLOBAL");
	ieDword value2 = CheckVariable( Sender, parameters->string1Parameter, "GLOBAL");
	SetVariable( Sender, parameters->string0Parameter, value1 + value2, "GLOBAL");
}

/* adding the second variable to the first, they could be area or locals */
void GameScript::GlobalAddGlobal(Scriptable* Sender, Action* parameters)
{
	ieDword value1 = CheckVariable( Sender,
		parameters->string0Parameter );
	ieDword value2 = CheckVariable( Sender,
		parameters->string1Parameter );
	SetVariable( Sender, parameters->string0Parameter, value1 + value2 );
}

/* adding the number to the global, they could be area or locals */
void GameScript::IncrementGlobal(Scriptable* Sender, Action* parameters)
{
	ieDword value = CheckVariable( Sender, parameters->string0Parameter );
	SetVariable( Sender, parameters->string0Parameter,
		value + parameters->int0Parameter );
}

/* adding the number to the global ONLY if the first global is zero */
void GameScript::IncrementGlobalOnce(Scriptable* Sender, Action* parameters)
{
	ieDword value = CheckVariable( Sender, parameters->string0Parameter );
	if (value != 0) {
		return;
	}
	//todo:the actual behaviour of this opcode may need to be verified, as this is
	//just a best guess at how the two parameters are changed, and could
	//well be more complex; the original usage of this function is currently
	//not well understood (relates to hardcoded alignment changes)
	SetVariable( Sender, parameters->string0Parameter, 1 );

	value = CheckVariable( Sender, parameters->string1Parameter );
	SetVariable( Sender, parameters->string1Parameter,
		value + parameters->int0Parameter );
}

void GameScript::GlobalSubGlobal(Scriptable* Sender, Action* parameters)
{
	ieDword value1 = CheckVariable( Sender,
		parameters->string0Parameter );
	ieDword value2 = CheckVariable( Sender,
		parameters->string1Parameter );
	SetVariable( Sender, parameters->string0Parameter, value1 - value2 );
}

void GameScript::GlobalAndGlobal(Scriptable* Sender, Action* parameters)
{
	ieDword value1 = CheckVariable( Sender,
		parameters->string0Parameter );
	ieDword value2 = CheckVariable( Sender,
		parameters->string1Parameter );
	SetVariable( Sender, parameters->string0Parameter, value1 && value2 );
}

void GameScript::GlobalOrGlobal(Scriptable* Sender, Action* parameters)
{
	ieDword value1 = CheckVariable( Sender,
		parameters->string0Parameter );
	ieDword value2 = CheckVariable( Sender,
		parameters->string1Parameter );
	SetVariable( Sender, parameters->string0Parameter, value1 || value2 );
}

void GameScript::GlobalBOrGlobal(Scriptable* Sender, Action* parameters)
{
	ieDword value1 = CheckVariable( Sender,
		parameters->string0Parameter );
	ieDword value2 = CheckVariable( Sender,
		parameters->string1Parameter );
	SetVariable( Sender, parameters->string0Parameter, value1 | value2 );
}

void GameScript::GlobalBAndGlobal(Scriptable* Sender, Action* parameters)
{
	ieDword value1 = CheckVariable( Sender,
		parameters->string0Parameter );
	ieDword value2 = CheckVariable( Sender,
		parameters->string1Parameter );
	SetVariable( Sender, parameters->string0Parameter, value1 & value2 );
}

void GameScript::GlobalXorGlobal(Scriptable* Sender, Action* parameters)
{
	ieDword value1 = CheckVariable( Sender,
		parameters->string0Parameter );
	ieDword value2 = CheckVariable( Sender,
		parameters->string1Parameter );
	SetVariable( Sender, parameters->string0Parameter, value1 ^ value2 );
}

void GameScript::GlobalBOr(Scriptable* Sender, Action* parameters)
{
	ieDword value1 = CheckVariable( Sender,
		parameters->string0Parameter );
	SetVariable( Sender, parameters->string0Parameter,
		value1 | parameters->int0Parameter );
}

void GameScript::GlobalBAnd(Scriptable* Sender, Action* parameters)
{
	ieDword value1 = CheckVariable( Sender,
		parameters->string0Parameter );
	SetVariable( Sender, parameters->string0Parameter,
		value1 & parameters->int0Parameter );
}

void GameScript::GlobalXor(Scriptable* Sender, Action* parameters)
{
	ieDword value1 = CheckVariable( Sender,
		parameters->string0Parameter );
	SetVariable( Sender, parameters->string0Parameter,
		value1 ^ parameters->int0Parameter );
}

void GameScript::GlobalMax(Scriptable* Sender, Action* parameters)
{
	int value1 = CheckVariable( Sender, parameters->string0Parameter );
	if (value1 > parameters->int0Parameter) {
		SetVariable( Sender, parameters->string0Parameter, value1 );
	}
}

void GameScript::GlobalMin(Scriptable* Sender, Action* parameters)
{
	int value1 = CheckVariable( Sender, parameters->string0Parameter );
	if (value1 < parameters->int0Parameter) {
		SetVariable( Sender, parameters->string0Parameter, value1 );
	}
}

void GameScript::BitClear(Scriptable* Sender, Action* parameters)
{
	ieDword value1 = CheckVariable( Sender,
		parameters->string0Parameter );
	SetVariable( Sender, parameters->string0Parameter,
		value1 & ~parameters->int0Parameter );
}

void GameScript::GlobalShL(Scriptable* Sender, Action* parameters)
{
	ieDword value1 = CheckVariable( Sender,
		parameters->string0Parameter );
	ieDword value2 = parameters->int0Parameter;
	if (value2 > 31) {
		value1 = 0;
	} else {
		value1 <<= value2;
	}
	SetVariable( Sender, parameters->string0Parameter, value1 );
}

void GameScript::GlobalShR(Scriptable* Sender, Action* parameters)
{
	ieDword value1 = CheckVariable( Sender,
		parameters->string0Parameter );
	ieDword value2 = parameters->int0Parameter;
	if (value2 > 31) {
		value1 = 0;
	} else {
		value1 >>= value2;
	}
	SetVariable( Sender, parameters->string0Parameter, value1 );
}

void GameScript::GlobalMaxGlobal(Scriptable* Sender, Action* parameters)
{
	ieDword value1 = CheckVariable( Sender, parameters->string0Parameter );
	ieDword value2 = CheckVariable( Sender, parameters->string1Parameter );
	if (value1 < value2) {
		SetVariable( Sender, parameters->string0Parameter, value2 );
	}
}

void GameScript::GlobalMinGlobal(Scriptable* Sender, Action* parameters)
{
	ieDword value1 = CheckVariable( Sender, parameters->string0Parameter );
	ieDword value2 = CheckVariable( Sender, parameters->string1Parameter );
	if (value1 > value2) {
		SetVariable( Sender, parameters->string0Parameter, value2 );
	}
}

void GameScript::GlobalShLGlobal(Scriptable* Sender, Action* parameters)
{
	ieDword value1 = CheckVariable( Sender, parameters->string0Parameter );
	ieDword value2 = CheckVariable( Sender, parameters->string1Parameter );
	if (value2 > 31) {
		value1 = 0;
	} else {
		value1 <<= value2;
	}
	SetVariable( Sender, parameters->string0Parameter, value1 );
}
void GameScript::GlobalShRGlobal(Scriptable* Sender, Action* parameters)
{
	ieDword value1 = CheckVariable( Sender, parameters->string0Parameter );
	ieDword value2 = CheckVariable( Sender, parameters->string1Parameter );
	if (value2 > 31) {
		value1 = 0;
	} else {
		value1 >>= value2;
	}
	SetVariable( Sender, parameters->string0Parameter, value1 );
}

void GameScript::ClearAllActions(Scriptable* Sender, Action* /*parameters*/)
{
	const Map *map = Sender->GetCurrentArea();
	int i = map->GetActorCount(true);
	while(i--) {
		Actor* act = map->GetActor(i,true);
		if (act && act != Sender && act->ValidTarget(GA_NO_DEAD)) {
			act->Stop(3);
			act->SetModal(MS_NONE);
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
		tar = GetScriptableFromObject(Sender, parameters->objects[1]);
		if (!tar) {
			Log(WARNING, "GameScript", "Couldn't find target for ClearActions!");
			parameters->objects[1]->dump();
			return;
		}
	}

	tar->Stop(3);

	if (tar->Type == ST_ACTOR) {
		Actor *actor = (Actor *) tar;
		actor->SetModal(MS_NONE);
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
	if (pdtable->GetRowIndex( scriptname ) != TableMgr::npos) {
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
	core->SetPause(PAUSE_ON, PF_QUIET);
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
	Scriptable* tar = GetScriptableFromObject(Sender, parameters->objects[1]);
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

//this will be a global change, fixme if it should be local
void GameScript::SetTextColor(Scriptable* /*Sender*/, Action* parameters)
{
	Color c = Color::FromABGR(parameters->int0Parameter);
	core->SetInfoTextColor(c);
}

void GameScript::BitGlobal(Scriptable* Sender, Action* parameters)
{
	ieDword value = CheckVariable(Sender, parameters->string0Parameter );
	HandleBitMod(value, parameters->int0Parameter, BitOp(parameters->int1Parameter));
	SetVariable(Sender, parameters->string0Parameter, value);
}

void GameScript::GlobalBitGlobal(Scriptable* Sender, Action* parameters)
{
	ieDword value1 = CheckVariable(Sender, parameters->string0Parameter );
	ieDword value2 = CheckVariable(Sender, parameters->string1Parameter );
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
		range = VOODOO_VISUAL_RANGE / 2;
	}

	actor->SetBase(IE_VISUALRANGE, range);
	if (actor->GetStat(IE_EA) < EA_EVILCUTOFF) {
		actor->SetBase(IE_EXPLORE, 1);
	}
	// just in case, ensuring the update happens already this tick
	// (in iwd2 script use it's not blocking, just in dialog)
	Map *map = Sender->GetCurrentArea();
	if (map) map->UpdateFog();
}

void GameScript::MakeUnselectable(Scriptable* Sender, Action* parameters)
{
	Sender->UnselectableTimer = parameters->int0Parameter * core->Time.ai_update_time;

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
	core->SetDebugMode(parameters->int0Parameter);
	Log(WARNING, "GameScript", "DEBUG: {}", parameters->string0Parameter);
}

void GameScript::IncrementProficiency(Scriptable* Sender, Action* parameters)
{
	unsigned int idx = parameters->int0Parameter;
	if (idx>31) {
		return;
	}
	Scriptable* tar = GetScriptableFromObject(Sender, parameters->objects[1]);
	Actor* target = Scriptable::As<Actor>(tar);
	if (!target) {
		return;
	}
	//start of the proficiency stats
	target->SetBase(IE_PROFICIENCYBASTARDSWORD+idx,
		target->GetBase(IE_PROFICIENCYBASTARDSWORD+idx)+parameters->int1Parameter);
}

void GameScript::IncrementExtraProficiency(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetScriptableFromObject(Sender, parameters->objects[1]);
	Actor* target = Scriptable::As<Actor>(tar);
	if (!target) {
		return;
	}
	target->SetBase(IE_FREESLOTS, target->GetBase(IE_FREESLOTS)+parameters->int0Parameter);
}

//the third parameter is a GemRB extension
void GameScript::AddJournalEntry(Scriptable* /*Sender*/, Action* parameters)
{
	core->GetGame()->AddJournalEntry(ieStrRef(parameters->int0Parameter), parameters->int1Parameter, parameters->int2Parameter);
}

void GameScript::SetQuestDone(Scriptable* /*Sender*/, Action* parameters)
{
	Game *game = core->GetGame();
	game->DeleteJournalEntry(ieStrRef(parameters->int0Parameter));
	game->AddJournalEntry(ieStrRef(parameters->int0Parameter), IE_GAM_QUEST_DONE, parameters->int2Parameter);

}

void GameScript::RemoveJournalEntry(Scriptable* /*Sender*/, Action* parameters)
{
	core->GetGame()->DeleteJournalEntry(ieStrRef(parameters->int0Parameter));
}

void GameScript::SetInternal(Scriptable* Sender, Action* parameters)
{
	unsigned int idx = parameters->int0Parameter;
	if (idx>15) {
		return;
	}
	Scriptable* tar = GetScriptableFromObject(Sender, parameters->objects[1]);
	Actor* target = Scriptable::As<Actor>(tar);
	if (!target) {
		return;
	}
	//start of the internal stats
	target->SetBase(IE_INTERNAL_0+idx, parameters->int1Parameter);
}

void GameScript::IncInternal(Scriptable* Sender, Action* parameters)
{
	unsigned int idx = parameters->int0Parameter;
	if (idx>15) {
		return;
	}
	Scriptable* tar = GetScriptableFromObject(Sender, parameters->objects[1]);
	Actor* target = Scriptable::As<Actor>(tar);
	if (!target) {
		return;
	}

	//start of the internal stats
	target->SetBase(IE_INTERNAL_0+idx,
		target->GetBase(IE_INTERNAL_0+idx)+parameters->int1Parameter);
}

void GameScript::DestroyAllEquipment(Scriptable* Sender, Action* /*parameters*/)
{
	Inventory *inv=NULL;

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
		inv->DestroyItem("",0,(ieDword) ~0); //destroy any and all
	}
}

void GameScript::DestroyItem(Scriptable* Sender, Action* parameters)
{
	Inventory *inv=NULL;

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
		inv->DestroyItem(parameters->resref0Parameter,0,1); //destroy one (even indestructible?)
	}
}

//negative destroygold creates gold
void GameScript::DestroyGold(Scriptable* Sender, Action* parameters)
{
	Actor* act = Scriptable::As<Actor>(Sender);
	if (!act) {
		return;
	}
	int max=(int) act->GetStat(IE_GOLD);
	if (parameters->int0Parameter != 0 && max > parameters->int0Parameter) {
		max = parameters->int0Parameter;
	}
	act->SetBase(IE_GOLD, act->GetBase(IE_GOLD)-max);
}

void GameScript::DestroyPartyItem(Scriptable* /*Sender*/, Action* parameters)
{
	const Game *game = core->GetGame();
	int i = game->GetPartySize(false);
	ieDword count;
	if (parameters->int0Parameter) {
		count=0;
	} else {
		count=1;
	}
	while (i--) {
		Inventory *inv = &(game->GetPC( i,false )->inventory);
		int res=inv->DestroyItem(parameters->resref0Parameter,0,count);
		if ( (count == 1) && res) {
			break;
		}
	}
}

/* this is a gemrb extension */
void GameScript::DestroyPartyItemNum(Scriptable* /*Sender*/, Action* parameters)
{
	const Game *game = core->GetGame();
	int i = game->GetPartySize(false);
	ieDword count;
	count = parameters->int0Parameter;
	while (i--) {
		Inventory *inv = &(game->GetPC( i,false )->inventory);
		count -= inv->DestroyItem(parameters->resref0Parameter,0,count);
		if (!count ) {
			break;
		}
	}
}

void GameScript::DestroyAllDestructableEquipment(Scriptable* Sender, Action* /*parameters*/)
{
	Inventory *inv=NULL;

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
	Scriptable* tar = GetScriptableFromObject(Sender, parameters->objects[1]);
	Actor* target = Scriptable::As<Actor>(tar);
	if (!target) {
		return;
	}
	target->SetName(ieStrRef(parameters->int0Parameter), 1);
}

void GameScript::SetRegularName(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetScriptableFromObject(Sender, parameters->objects[1]);
	Actor* target = Scriptable::As<Actor>(tar);
	if (!target) {
		return;
	}
	target->SetName(ieStrRef(parameters->int0Parameter), 2);
}

/** this is a gemrb extension */
void GameScript::UnloadArea(Scriptable* /*Sender*/, Action* parameters)
{
	int map=core->GetGame()->FindMap(parameters->resref0Parameter);
	if (map>=0) {
		core->GetGame()->DelMap(map, parameters->int0Parameter);
	}
}

static EffectRef fx_death_ref = { "Death", -1 };
void GameScript::Kill(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetScriptableFromObject(Sender, parameters->objects[1]);
	Actor* target = Scriptable::As<Actor>(tar);
	if (!target) {
		return;
	}

	Effect *fx = EffectQueue::CreateEffect(fx_death_ref, 0, 0, FX_DURATION_INSTANT_PERMANENT);
	target->fxqueue.AddEffect(fx, false);
}

void GameScript::SetGabber(Scriptable* Sender, Action* parameters)
{
	const Scriptable* tar = GetScriptableFromObject(Sender, parameters->objects[1]);
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
	core->GetGame()->SetReputation(parameters->int0Parameter*10);
}

void GameScript::ReputationInc(Scriptable* /*Sender*/, Action* parameters)
{
	Game *game = core->GetGame();
	game->SetReputation( (int) game->Reputation + parameters->int0Parameter*10);
}

void GameScript::FullHeal(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetScriptableFromObject(Sender, parameters->objects[1]);
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
	Effect *fx = EffectQueue::CreateEffect(fx_disable_button_ref, 0, ACT_TURN, FX_DURATION_INSTANT_PERMANENT);
	act->fxqueue.AddEffect(fx, false);
	fx = EffectQueue::CreateEffect(fx_disable_button_ref, 0, ACT_CAST, FX_DURATION_INSTANT_PERMANENT);
	act->fxqueue.AddEffect(fx, false);
	if (act->InParty && core->HasFeedback(FT_STATES)) displaymsg->DisplayConstantStringName(STR_PALADIN_FALL, GUIColors::XPCHANGE, act);
}

void GameScript::RemoveRangerHood(Scriptable* Sender, Action* /*parameters*/)
{
	Actor* act = Scriptable::As<Actor>(Sender);
	if (!act) {
		return;
	}
	act->ApplyKit(true, Actor::GetClassID(ISRANGER));
	act->SetMCFlag(MC_FALLEN_RANGER, BitOp::OR);
	Effect *fx = EffectQueue::CreateEffect(fx_disable_button_ref, 0, ACT_STEALTH, FX_DURATION_INSTANT_PERMANENT);
	act->fxqueue.AddEffect(fx, false);
	fx = EffectQueue::CreateEffect(fx_disable_button_ref, 0, ACT_CAST, FX_DURATION_INSTANT_PERMANENT);
	act->fxqueue.AddEffect(fx, false);
	if (act->InParty && core->HasFeedback(FT_STATES)) displaymsg->DisplayConstantStringName(STR_RANGER_FALL, GUIColors::XPCHANGE, act);
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

//transfering item from Sender to target, target must be an actor
//if target can't get it, it will be dropped at its feet
//a container or an actor can take an item from someone
void GameScript::GetItem(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetScriptableFromObject(Sender, parameters->objects[1]);
	if (!tar) {
		return;
	}
	MoveItemCore(tar, Sender, parameters->string0Parameter,0,0);
}

//getting one single item
void GameScript::TakePartyItem(Scriptable* Sender, Action* parameters)
{
	const Game *game = core->GetGame();
	int i=game->GetPartySize(false);
	while (i--) {
		int res=MoveItemCore(game->GetPC(i,false), Sender, parameters->string0Parameter,IE_INV_ITEM_UNDROPPABLE,IE_INV_ITEM_UNSTEALABLE);
		if (res!=MIC_NOITEM) return;
	}
}

//getting x single item
void GameScript::TakePartyItemNum(Scriptable* Sender, Action* parameters)
{
	int count = parameters->int0Parameter;
	const Game *game = core->GetGame();
	int i=game->GetPartySize(false);
	while (i-- && count) {
		Actor *pc = game->GetPC(i, false);
		int res = MoveItemCore(pc, Sender, parameters->string0Parameter, IE_INV_ITEM_UNDROPPABLE, IE_INV_ITEM_UNSTEALABLE, 1);
		if (res == MIC_GOTITEM) {
			i++;
			count--;
		}
	}
}

void GameScript::TakePartyItemRange(Scriptable* Sender, Action* parameters)
{
	const Game *game = core->GetGame();
	int i=game->GetPartySize(false);
	while (i--) {
		Actor *ac = game->GetPC(i,false);
		if (Distance(Sender, ac)<MAX_OPERATING_DISTANCE) {
			while (MoveItemCore(ac, Sender, parameters->string0Parameter,IE_INV_ITEM_UNDROPPABLE,IE_INV_ITEM_UNSTEALABLE)==MIC_GOTITEM) { }
		}
	}
}

void GameScript::TakePartyItemAll(Scriptable* Sender, Action* parameters)
{
	const Game *game = core->GetGame();
	int i=game->GetPartySize(false);
	while (i--) {
		while (MoveItemCore(game->GetPC(i,false), Sender, parameters->string0Parameter,IE_INV_ITEM_UNDROPPABLE, IE_INV_ITEM_UNSTEALABLE)==MIC_GOTITEM) { }
	}
}

//an actor can 'give' an item to a container or another actor
void GameScript::GiveItem(Scriptable *Sender, Action* parameters)
{
	Scriptable* tar = GetScriptableFromObject(Sender, parameters->objects[1]);
	MoveItemCore(Sender, tar, parameters->string0Parameter,0,0);
}

//this action creates an item in a container or a creature
//if there is an object it works as GiveItemCreate
//otherwise it creates the item on the Sender
void GameScript::CreateItem(Scriptable *Sender, Action* parameters)
{
	Scriptable* tar;
	if (parameters->objects[1]) {
		tar = GetScriptableFromObject(Sender, parameters->objects[1]);
	} else {
		tar = Sender;
	}
	if (!tar)
		return;
	Inventory *myinv;

	switch(tar->Type) {
		case ST_ACTOR:
			myinv = &(static_cast<Actor*>(tar)->inventory);
			break;
		case ST_CONTAINER:
			myinv = &(static_cast<Container*>(tar)->inventory);
			break;
		default:
			return;
	}

	CREItem *item = new CREItem();
	if (!CreateItemCore(item, parameters->resref0Parameter, parameters->int0Parameter, parameters->int1Parameter, parameters->int2Parameter)) {
		delete item;
		return;
	}
	if (tar->Type==ST_CONTAINER) {
		myinv->AddItem(item);
		return;
	}

	const Actor *act = static_cast<const Actor*>(tar);
	if (ASI_SUCCESS != myinv->AddSlotItem(item, SLOT_ONLYINVENTORY)) {
		Map *map = tar->GetCurrentArea();
		// drop it at my feet
		map->AddItemToLocation(tar->Pos, item);
		if (act->InParty) {
			act->VerbalConstant(VB_INVENTORY_FULL);
			if (core->HasFeedback(FT_MISC)) displaymsg->DisplayConstantString(STR_INVFULL_ITEMDROP, GUIColors::XPCHANGE);
		}
	} else {
		if (act->InParty && core->HasFeedback(FT_MISC)) displaymsg->DisplayConstantString(STR_GOTITEM, GUIColors::XPCHANGE);
	}
}

void GameScript::CreateItemNumGlobal(Scriptable *Sender, Action* parameters)
{
	Inventory *myinv;

	switch(Sender->Type) {
		case ST_ACTOR:
			myinv = &(static_cast<Actor*>(Sender)->inventory);
			break;
		case ST_CONTAINER:
			myinv = &(static_cast<Container*>(Sender)->inventory);
			break;
		default:
			return;
	}
	int value = CheckVariable( Sender, parameters->string0Parameter );
	CREItem *item = new CREItem();
	if (!CreateItemCore(item, parameters->resref1Parameter, value, 0, 0)) {
		delete item;
		return;
	}
	if (Sender->Type==ST_CONTAINER) {
		myinv->AddItem(item);
		return;
	}

	const Actor *act = static_cast<const Actor*>(Sender);
	if (ASI_SUCCESS != myinv->AddSlotItem(item, SLOT_ONLYINVENTORY)) {
		Map *map = Sender->GetCurrentArea();
		// drop it at my feet
		map->AddItemToLocation(Sender->Pos, item);
		if (act->InParty) {
			act->VerbalConstant(VB_INVENTORY_FULL);
			if (core->HasFeedback(FT_MISC)) displaymsg->DisplayConstantString(STR_INVFULL_ITEMDROP, GUIColors::XPCHANGE);
		}
	} else {
		if (act->InParty && core->HasFeedback(FT_MISC)) displaymsg->DisplayConstantString(STR_GOTITEM, GUIColors::XPCHANGE);
	}
}

void GameScript::SetItemFlags(Scriptable *Sender, Action* parameters)
{
	Scriptable* tar;
	if (parameters->objects[1]) {
		tar = GetScriptableFromObject(Sender, parameters->objects[1]);
	} else {
		tar = Sender;
	}
	if (!tar) return;

	const Inventory *myinv;
	switch(tar->Type) {
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

void GameScript::TakeItemReplace(Scriptable *Sender, Action* parameters)
{
	Scriptable* tar = GetScriptableFromObject(Sender, parameters->objects[1]);
	Actor* scr = Scriptable::As<Actor>(tar);
	if (!scr) {
		return;
	}

	CREItem *item;
	int slot = scr->inventory.RemoveItem(parameters->string1Parameter, IE_INV_ITEM_UNDROPPABLE, &item);
	if (!item) {
		item = new CREItem();
	}
	if (!CreateItemCore(item, parameters->resref0Parameter, -1, 0, 0)) {
		delete item;
		return;
	}
	if (ASI_SUCCESS != scr->inventory.AddSlotItem(item,slot)) {
		Map *map = scr->GetCurrentArea();
		map->AddItemToLocation(Sender->Pos, item);
	}
}

//same as equipitem, but with additional slots parameter, and object to perform action
// XEquipItem("00Troll1",Myself,SLOT_RING_LEFT,TRUE)
void GameScript::XEquipItem(Scriptable *Sender, Action* parameters)
{
	Scriptable* tar = GetScriptableFromObject(Sender, parameters->objects[1]);
	Actor* actor = Scriptable::As<Actor>(tar);
	if (!actor) {
		return;
	}
	int slot = actor->inventory.FindItem(parameters->resref0Parameter, IE_INV_ITEM_UNDROPPABLE);
	if (slot<0) {
		return;
	}

	int slot2 = parameters->int0Parameter;
	bool equip = parameters->int1Parameter;

	if (equip) {
		if (slot != slot2) {
			// swap them first, so we equip to the desired slot
			CREItem *si = actor->inventory.RemoveItem(slot);
			CREItem *si2 = actor->inventory.RemoveItem(slot2);
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
		CREItem *si = actor->inventory.RemoveItem(slot);
		if (si && actor->inventory.AddSlotItem(si, SLOT_ONLYINVENTORY) == ASI_FAILED) {
			Map *map = Sender->GetCurrentArea();
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
void GameScript::FillSlot(Scriptable *Sender, Action* parameters)
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
		if(actor->inventory.AddSlotItem(tmp, slot)!=ASI_SUCCESS) {
			delete tmp;
		}
	}
}

//iwd2 also has a flag for unequip (it might collide with original!)
void GameScript::EquipItem(Scriptable *Sender, Action* parameters)
{
	Actor* actor = Scriptable::As<Actor>(Sender);
	if (!actor) {
		return;
	}
	int slot = actor->inventory.FindItem(parameters->resref0Parameter, IE_INV_ITEM_UNDROPPABLE);
	if (slot<0) {
		return;
	}

	int slot2;

	if (parameters->int0Parameter) {
		//unequip item, and move it to the inventory
		slot2 = SLOT_ONLYINVENTORY;
	} else {
		//equip item if possible
		slot2 = SLOT_AUTOEQUIP;
	}
	CREItem *si = actor->inventory.RemoveItem(slot);
	if (si && actor->inventory.AddSlotItem(si, slot2) == ASI_FAILED) {
		Map *map = Sender->GetCurrentArea();
		if (map) {
			//drop item at the feet of the character instead of destroying it
			map->AddItemToLocation(Sender->Pos, si);
		} else {
			delete si;
		}
	}
	actor->ReinitQuickSlots();
}

void GameScript::DropItem(Scriptable *Sender, Action* parameters)
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
		MoveNearerTo(Sender, parameters->pointParameter, 10,0);
		return;
	}
	Map *map = Sender->GetCurrentArea();

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

void GameScript::DropInventory(Scriptable *Sender, Action* /*parameters*/)
{
	Actor* scr = Scriptable::As<Actor>(Sender);
	if (!scr) {
		return;
	}

	scr->DropItem("",0);
}

//this should work on containers!
//using the same code for DropInventoryEXExclude
void GameScript::DropInventoryEX(Scriptable *Sender, Action* parameters)
{
	Scriptable* tar = GetScriptableFromObject(Sender, parameters->objects[1]);
	if (!tar) {
		return;
	}
	Inventory *inv = NULL;
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
	Map *area = tar->GetCurrentArea();
	while(x--) {
		if (!parameters->resref0Parameter.IsEmpty()) {
			const ResRef& itemRef = inv->GetSlotItem(x)->ItemResRef;
			if (itemRef == parameters->resref0Parameter) {
				continue;
			}
		}
		inv->DropItemAtLocation(x, 0, area, tar->Pos);
	}
}

void GameScript::GivePartyAllEquipment(Scriptable *Sender, Action* /*parameters*/)
{
	const Actor* scr = Scriptable::As<Actor>(Sender);
	if (!scr) {
		return;
	}
	const Game *game = core->GetGame();
	// pick the first actor first
	for (int i = 0; i < game->GetPartySize(false); i++) {
		Actor *tar = game->GetPC(i,false);
		//don't try to give self, it would be an infinite loop
		if (tar == scr)
			continue;
		while(MoveItemCore(Sender, tar, "",0,0)!=MIC_NOITEM) { }
	}
}

//This is unsure, Plunder could be just handling ground piles and not dead actors
void GameScript::Plunder(Scriptable *Sender, Action* parameters)
{
	if (Sender->Type!=ST_ACTOR) {
		Sender->ReleaseCurrentAction();
		return;
	}
	Scriptable* tar = GetStoredActorFromObject( Sender, parameters->objects[1] );
	if (!tar) {
		Sender->ReleaseCurrentAction();
		return;
	}

	//you must be joking
	if (tar==Sender) {
		Sender->ReleaseCurrentAction();
		return;
	}

	// can plunder only dead actors
	const Actor* scr = Scriptable::As<Actor>(tar);
	if (scr && !(scr->BaseStats[IE_STATE_ID] & STATE_DEAD)) {
		Sender->ReleaseCurrentAction();
		return;
	}
	if (PersonalDistance(Sender, tar)>MAX_OPERATING_DISTANCE ) {
		MoveNearerTo(Sender, tar->Pos, MAX_OPERATING_DISTANCE,0);
		return;
	}
	//move all movable item from the target to the Sender
	//the rest will be dropped at the feet of Sender
	while(MoveItemCore(tar, Sender, "",0,0)!=MIC_NOITEM) { }
	Sender->ReleaseCurrentAction();
}

void GameScript::MoveInventory(Scriptable *Sender, Action* parameters)
{
	Scriptable* src = GetScriptableFromObject(Sender, parameters->objects[1]);
	if (!src || src->Type!=ST_ACTOR) {
		return;
	}
	Scriptable* tar = GetScriptableFromObject(Sender, parameters->objects[2]);
	if (!tar || tar->Type!=ST_ACTOR) {
		return;
	}
	//don't try to move to self, it would create infinite loop
	if (src==tar)
		return;
	//move all movable item from the target to the Sender
	//the rest will be dropped at the feet of Sender
	while(MoveItemCore(src, tar, "",0,0)!=MIC_NOITEM) { }
}

void GameScript::PickPockets(Scriptable *Sender, Action* parameters)
{
	Actor* snd = Scriptable::As<Actor>(Sender);
	if (!snd) {
		Sender->ReleaseCurrentAction();
		return;
	}
	Scriptable* tar = GetStoredActorFromObject( Sender, parameters->objects[1] );
	Actor* scr = Scriptable::As<Actor>(tar);
	if (!scr) {
		Sender->ReleaseCurrentAction();
		return;
	}

	//for PP one must go REALLY close
	Map *map=Sender->GetCurrentArea();
	if (!map) {
		Sender->ReleaseCurrentAction();
		return;
	}

	if (PersonalDistance(Sender, tar)>10 ) {
		MoveNearerTo(Sender, tar, 10);
		return;
	}

	static bool turnHostile = core->HasFeature(GF_STEAL_IS_ATTACK);
	static bool reportFailure = core->HasFeedback(FT_MISC);
	static bool breakInvisibility = true;
	AutoTable ppBehave = gamedata->LoadTable("ppbehave");
	if (ppBehave) {
		turnHostile = ppBehave->QueryFieldSigned<int>("TURN_HOSTILE", "VALUE") == 1;
		reportFailure &= ppBehave->QueryFieldSigned<int>("REPORT_FAILURE", "VALUE") == 1;
		breakInvisibility = ppBehave->QueryFieldSigned<int>("BREAK_INVISIBILITY", "VALUE") == 1;
	}

	if (scr->GetStat(IE_EA)>EA_EVILCUTOFF) {
		if (reportFailure) displaymsg->DisplayConstantStringName(STR_PICKPOCKET_EVIL, GUIColors::WHITE, Sender);
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
	if (core->HasFeature(GF_3ED_RULES)) {
		skill = snd->GetSkill(IE_PICKPOCKET);
		int roll = core->Roll(1, 20, 0);
		int level = scr->GetXPLevel(true);
		int wismod = scr->GetAbilityBonus(IE_WIS);
		// ~Pick pocket check. (10 + skill w/Dex bonus) %d vs. ((d20 + target's level) + Wisdom modifier) %d + %d.~
		displaymsg->DisplayRollStringName(ieStrRef::ROLL12, GUIColors::LIGHTGREY, snd, 10+skill, roll+level, wismod);
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
		if (reportFailure) displaymsg->DisplayConstantStringName(STR_PICKPOCKET_FAIL, GUIColors::WHITE, Sender);
		if (breakInvisibility) {
			snd->SetModal(MS_NONE);
			snd->CureInvisibility();
		}
		if (turnHostile) {
			scr->AttackedBy(snd);
		} else {
			//pickpocket failed trigger
			tar->AddTrigger(TriggerEntry(trigger_pickpocketfailed, snd->GetGlobalID()));
		}
		Sender->ReleaseCurrentAction();
		return;
	}

	int ret = MIC_NOITEM;
	if (slot != -1) {
		CREItem* item = scr->inventory.RemoveItem(slot);
		ret = snd->inventory.AddSlotItem(item, SLOT_ONLYINVENTORY);
		if (ret != ASI_SUCCESS) {
			map->AddItemToLocation(snd->Pos, item);
			ret = MIC_FULL;
		}
	}

	if (ret==MIC_NOITEM) {
		int money=0;
		//go for money too
		if (scr->GetStat(IE_GOLD)>0) {
			money = (RandomNumValue % scr->GetStat(IE_GOLD)) + 1;
		}
		if (!money) {
			//no stuff to steal
			if (reportFailure) displaymsg->DisplayConstantStringName(STR_PICKPOCKET_NONE, GUIColors::WHITE, Sender);
			Sender->ReleaseCurrentAction();
			return;
		}
		CREItem *item = new CREItem();
		if (!CreateItemCore(item, core->GoldResRef, money, 0, 0)) {
			error("GameScript", "Failed to create {} of pick-pocketed gold '{}'!", money, core->GoldResRef);
		}
		scr->SetBase(IE_GOLD, scr->GetBase(IE_GOLD) - money);
		if (ASI_SUCCESS != snd->inventory.AddSlotItem(item, SLOT_ONLYINVENTORY)) {
			// drop it at my feet
			map->AddItemToLocation(snd->Pos, item);
			ret = MIC_FULL;
		}
	}

	if (core->HasFeedback(FT_MISC)) displaymsg->DisplayConstantStringName(STR_PICKPOCKET_DONE, GUIColors::WHITE, Sender);
	DisplayStringCoreVC(snd, VB_PP_SUCC, DS_CONSOLE);

	int xp = gamedata->GetXPBonus(XP_PICKPOCKET, scr->GetXPLevel(1));
	core->GetGame()->ShareXP(xp, SX_DIVIDE);

	if (ret == MIC_FULL && snd->InParty) {
		if (!core->HasFeature(GF_PST_STATE_FLAGS)) snd->VerbalConstant(VB_INVENTORY_FULL);
		if (reportFailure) displaymsg->DisplayConstantStringName(STR_PICKPOCKET_INVFUL, GUIColors::WHITE, Sender);
	}
	Sender->ReleaseCurrentAction();
}

void GameScript::TakeItemList(Scriptable * Sender, Action* parameters)
{
	Scriptable* tar = GetScriptableFromObject(Sender, parameters->objects[1]);
	if (!tar || tar->Type!=ST_ACTOR) {
		return;
	}
	AutoTable tab = gamedata->LoadTable(parameters->resref0Parameter);
	if (!tab) {
		return;
	}

	TableMgr::index_t rows = tab->GetRowCount();
	for (TableMgr::index_t i = 0; i < rows; ++i) {
		MoveItemCore(tar, Sender, tab->QueryField(i,0), 0, IE_INV_ITEM_UNSTEALABLE);
	}
}

void GameScript::TakeItemListParty(Scriptable * Sender, Action* parameters)
{
	AutoTable tab = gamedata->LoadTable(parameters->resref0Parameter);
	if (!tab) {
		return;
	}
	const Game *game = core->GetGame();
	TableMgr::index_t rows = tab->GetRowCount();
	for (TableMgr::index_t i = 0; i < rows; ++i) {
		int j = game->GetPartySize(false);
		while (j--) {
			Actor *tar = game->GetPC(j, false);
			MoveItemCore(tar, Sender, tab->QueryField(i,0), 0, IE_INV_ITEM_UNSTEALABLE);
		}
	}
}

void GameScript::TakeItemListPartyNum(Scriptable * Sender, Action* parameters)
{
	AutoTable tab = gamedata->LoadTable(parameters->resref0Parameter);
	if (!tab) {
		return;
	}
	const Game *game = core->GetGame();
	TableMgr::index_t rows = tab->GetRowCount();
	int count = parameters->int0Parameter;
	for (TableMgr::index_t i = 0; i < rows; ++i) {
		int j = game->GetPartySize(false);
		while (j--) {
			Actor *tar = game->GetPC(j, false);
			int res = MoveItemCore(tar, Sender, tab->QueryField(i,0), 0, IE_INV_ITEM_UNSTEALABLE);
			if (res==MIC_GOTITEM) {
				j++;
				count--;
			}
			if (!count) break;
		}
	}
	if (count == 1) {
		// grant the default table item to the Sender in regular games
		Action *params = new Action(true);
		params->resref0Parameter = tab->QueryDefault();
		CreateItem(Sender, params);
		delete params;
	}
}

//bg2
void GameScript::SetRestEncounterProbabilityDay(Scriptable* Sender, Action* parameters)
{
	Map *map=Sender->GetCurrentArea();
	map->RestHeader.DayChance = (ieWord) parameters->int0Parameter;
}

void GameScript::SetRestEncounterProbabilityNight(Scriptable* Sender, Action* parameters)
{
	Map *map=Sender->GetCurrentArea();
	map->RestHeader.NightChance = (ieWord) parameters->int0Parameter;
}

//iwd
void GameScript::SetRestEncounterChance(Scriptable * Sender, Action* parameters)
{
	Map *map=Sender->GetCurrentArea();
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
void GameScript::ExpansionEndCredits(Scriptable *Sender, Action *parameters)
{
	core->PlayMovie("ecredit");

	// end the game for HoW-only runs, but teleport back to Kuldahar for full iwd runs
	bool howOnly = CheckVariable(Sender, "EXPANSION_DOOR", "GLOBAL") == 1; // 1 how only, 0/2 for full run
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
	ClearAllActions(Sender, parameters);
	core->GetDictionary()->SetAt("QuitGame1", (ieDword) parameters->int0Parameter);
	core->GetDictionary()->SetAt("QuitGame2", (ieDword) parameters->int1Parameter);
	core->GetDictionary()->SetAt("QuitGame3", (ieDword) parameters->int2Parameter);
	core->SetNextScript("QuitGame");
}

//BG2 demo end, shows some pictures then goes to main screen
void GameScript::DemoEnd(Scriptable* Sender, Action* parameters)
{
	ClearAllActions(Sender, parameters);
	core->GetDictionary()->SetAt("QuitGame1", (ieDword)0);
	core->GetDictionary()->SetAt("QuitGame2", (ieDword)0);
	core->GetDictionary()->SetAt("QuitGame3", (ieDword)-1);
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
	Scriptable* tar = GetScriptableFromObject(Sender, parameters->objects[1]);
	Actor* damagee = Scriptable::As<Actor>(tar);
	if (!damagee) {
		return;
	}
	Actor* damager = Scriptable::As<Actor>(Sender);
	if (!damager) {
		damager=damagee;
	}

	damagee->Damage(parameters->int0Parameter, parameters->int1Parameter >> 16, damager);
}

void GameScript::ApplyDamagePercent(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetScriptableFromObject(Sender, parameters->objects[1]);
	Actor* damagee = Scriptable::As<Actor>(tar);
	if (!damagee) {
		return;
	}
	Actor* damager = Scriptable::As<Actor>(Sender);
	if (!damager) {
		damager=damagee;
	}

	//this, if the percent is calculated from the current hp
	damagee->Damage((parameters->int0Parameter*damagee->Modified[IE_HITPOINTS])/100, parameters->int1Parameter >> 16, damager);
	//this, if the percent is calculated from the max hp
	//damagee->Damage(parameters->int0Parameter, parameters->int1Parameter >> 16, damager, MOD_PERCENT);
}

void GameScript::Damage(Scriptable* Sender, Action* parameters)
{
	Scriptable *damager = Sender;
	Scriptable* tar = GetScriptableFromObject(Sender, parameters->objects[1]);
	Actor* damagee = Scriptable::As<Actor>(tar);
	if (!damagee) {
		return;
	}

	// bones.ids handling
	int diceNum = (parameters->int1Parameter>>12)&15;
	int diceSize = (parameters->int1Parameter>>4)&255;
	int diceAdd = parameters->int1Parameter&15;
	int damage = 0;
	const Actor* damager2 = Scriptable::As<Actor>(Sender);

	if (damager2 && damager2 != damagee) {
		damage = damager2->LuckyRoll(diceNum, diceSize, diceAdd, LR_DAMAGELUCK, damagee);
	} else {
		damage = core->Roll(diceNum, diceSize, diceAdd);
	}
	int type=MOD_ADDITIVE;
	// delta.ids
	switch(parameters->int0Parameter) {
	case DM_LOWER: // lower
		break;
	case DM_RAISE: //raise
		damage=-damage;
		break;
	case DM_SET: //set
		type=MOD_ABSOLUTE;
		break;
	case 4: // GemRB extension
		type=MOD_PERCENT;
		break;
	// NOTE: forge.d has a bunch of calls missing a parameter, eg. Damage(Protagonist, 15)
	// it's unclear if it worked, but let's accommodate it
	default:
		damage = parameters->int0Parameter;
		break;
	}
	//damagetype seems to be always 0
	damagee->Damage( damage, 0, damager, type);
}

void GameScript::SetHomeLocation(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetScriptableFromObject(Sender, parameters->objects[1]);
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

	const Map *map = Sender->GetCurrentArea();
	if (!map) {
		return;
	}

	const Actor *target;
	if (!act->GetStat(IE_BERSERKSTAGE2) && (core->Roll(1,100,0)<50) ) {
		//anyone
		target = GetNearestEnemyOf(map, act, ORIGIN_SEES_ENEMY);
	} else {
		target = GetNearestOf(map, act, ORIGIN_SEES_ENEMY);
	}

	if (!target) {
		Sender->SetWait(6);
	} else {
		//generate attack action
		Action *newaction = GenerateActionDirect("NIDSpecial3()", target);
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
	act->Panic(NULL, PANIC_RANDOMWALK);
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
	WorldMap *worldmap = core->GetWorldMap();
	if (!worldmap) {
		error("GameScript", "Can't find worldmap!");
	}
	// WMP_ENTRY_ADJACENT because otherwise revealed bg2 areas are unreachable from city gates
	worldmap->SetAreaStatus(parameters->resref0Parameter, WMP_ENTRY_VISIBLE|WMP_ENTRY_ADJACENT, BitOp::OR);
	displaymsg->DisplayConstantString(STR_WORLDMAPCHANGE, GUIColors::XPCHANGE);
}

void GameScript::HideAreaOnMap( Scriptable* /*Sender*/, Action* parameters)
{
	WorldMap *worldmap = core->GetWorldMap();
	if (!worldmap) {
		error("GameScript", "Can't find worldmap!");
	}
	// WMP_ENTRY_ADJACENT because otherwise revealed bg2 areas are unreachable from city gates
	worldmap->SetAreaStatus(parameters->resref0Parameter, WMP_ENTRY_VISIBLE|WMP_ENTRY_ADJACENT, BitOp::NAND);
}

void GameScript::AddWorldmapAreaFlag(Scriptable* /*Sender*/, Action* parameters)
{
	WorldMap *worldmap = core->GetWorldMap();
	if (!worldmap) {
		error("GameScript", "Can't find worldmap!");
	}
	worldmap->SetAreaStatus(parameters->resref0Parameter, parameters->int0Parameter, BitOp::OR);
}

void GameScript::RemoveWorldmapAreaFlag(Scriptable* /*Sender*/, Action* parameters)
{
	WorldMap *worldmap = core->GetWorldMap();
	if (!worldmap) {
		error("GameScript", "Can't find worldmap!");
	}
	worldmap->SetAreaStatus(parameters->resref0Parameter, parameters->int0Parameter, BitOp::NAND);
}

void GameScript::SendTrigger(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetScriptableFromObject(Sender, parameters->objects[1], GA_NO_DEAD);
	if (!tar) {
		return;
	}
	tar->AddTrigger(TriggerEntry(trigger_trigger, parameters->int0Parameter));
}

void GameScript::Shout( Scriptable* Sender, Action* parameters)
{
	const Actor* actor = Scriptable::As<Actor>(Sender);
	if (!actor) {
		return;
	}
	//according to IESDP silenced creatures cannot use shout
	// neither do dead ones or the paladin ogres turn Garren hostile
	if (actor->GetStat(IE_STATE_ID) & (STATE_SILENCED|STATE_DEAD)) {
		return;
	}
	const Map *map = Sender->GetCurrentArea();
	map->Shout(actor, parameters->int0Parameter, false);
}

void GameScript::GlobalShout( Scriptable* Sender, Action* parameters)
{
	const Actor* actor = Scriptable::As<Actor>(Sender);
	if (!actor) {
		return;
	}
	//according to IESDP silenced creatures cannot use shout
	if (actor->GetStat(IE_STATE_ID) & (STATE_SILENCED|STATE_DEAD)) {
		return;
	}
	const Map *map = Sender->GetCurrentArea();
	// true means global, unlimited, shout distance
	map->Shout(actor, parameters->int0Parameter, true);
}

void GameScript::Help( Scriptable* Sender, Action* /*parameters*/)
{
	const Actor* actor = Scriptable::As<Actor>(Sender);
	if (!actor) {
		return;
	}
	//TODO: add state limiting like in Shout?
	const Map *map=Sender->GetCurrentArea();
	map->Shout(actor, 0, false);
}

void GameScript::GiveOrder(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetScriptableFromObject(Sender, parameters->objects[1]);
	if (tar) {
		tar->AddTrigger(TriggerEntry(trigger_receivedorder, Sender->GetGlobalID(), parameters->int0Parameter));
	}
}

void GameScript::AddMapnote( Scriptable* Sender, Action* parameters)
{
	Map *map=Sender->GetCurrentArea();
	map->AddMapNote(parameters->pointParameter, parameters->int1Parameter, ieStrRef(parameters->int0Parameter));
}

void GameScript::RemoveMapnote( Scriptable* Sender, Action* parameters)
{
	Map *map=Sender->GetCurrentArea();
	map->RemoveMapNote(parameters->pointParameter);
}

void GameScript::AttackOneRound( Scriptable* Sender, Action* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		Sender->ReleaseCurrentAction();
		return;
	}
	//using auto target!
	Scriptable* tar = GetStoredActorFromObject(Sender, parameters->objects[1], GA_NO_DEAD);
	if (!tar || (tar->Type != ST_ACTOR && tar->Type !=ST_DOOR && tar->Type !=ST_CONTAINER) ) {
		Sender->ReleaseCurrentAction();
		return;
	}

	//actor is already incapable of attack
	if (Sender->GetInternalFlag()&IF_STOPATTACK) {
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

void GameScript::RunningAttackNoSound( Scriptable* Sender, Action* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		Sender->ReleaseCurrentAction();
		return;
	}
	//using auto target!
	Scriptable* tar = GetStoredActorFromObject(Sender, parameters->objects[1], GA_NO_DEAD);
	if (!tar || (tar->Type != ST_ACTOR && tar->Type !=ST_DOOR && tar->Type !=ST_CONTAINER) ) {
		Sender->ReleaseCurrentAction();
		return;
	}

	//actor is already incapable of attack
	if (Sender->GetInternalFlag()&IF_STOPATTACK) {
		Sender->ReleaseCurrentAction();
		return;
	}

	AttackCore(Sender, tar, AC_NO_SOUND|AC_RUNNING);
}

void GameScript::AttackNoSound( Scriptable* Sender, Action* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		Sender->ReleaseCurrentAction();
		return;
	}
	//using auto target!
	Scriptable* tar = GetStoredActorFromObject(Sender, parameters->objects[1], GA_NO_DEAD);
	if (!tar || (tar->Type != ST_ACTOR && tar->Type !=ST_DOOR && tar->Type !=ST_CONTAINER) ) {
		Sender->ReleaseCurrentAction();
		return;
	}

	//actor is already incapable of attack
	if (Sender->GetInternalFlag()&IF_STOPATTACK) {
		Sender->ReleaseCurrentAction();
		return;
	}

	AttackCore(Sender, tar, AC_NO_SOUND);
}

void GameScript::RunningAttack( Scriptable* Sender, Action* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		Sender->ReleaseCurrentAction();
		return;
	}
	//using auto target!
	Scriptable* tar = GetStoredActorFromObject(Sender, parameters->objects[1], GA_NO_DEAD);
	if (!tar || (tar->Type != ST_ACTOR && tar->Type !=ST_DOOR && tar->Type !=ST_CONTAINER) ) {
		Sender->ReleaseCurrentAction();
		return;
	}

	//actor is already incapable of attack
	if (Sender->GetInternalFlag()&IF_STOPATTACK) {
		Sender->ReleaseCurrentAction();
		return;
	}

	AttackCore(Sender, tar, AC_RUNNING);
}

void GameScript::Attack( Scriptable* Sender, Action* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		Sender->ReleaseCurrentAction();
		return;
	}
	//using auto target!
	Scriptable* tar = GetStoredActorFromObject(Sender, parameters->objects[1], GA_NO_DEAD);

	if (!tar || (tar->Type != ST_ACTOR && tar->Type !=ST_DOOR && tar->Type !=ST_CONTAINER) || tar == Sender) {
		Sender->ReleaseCurrentAction();
		return;
	}

	//actor is already incapable of attack
	if (Sender->GetInternalFlag()&IF_STOPATTACK) {
		Sender->ReleaseCurrentAction();
		return;
	}

	AttackCore(Sender, tar, 0);
}

void GameScript::ForceAttack( Scriptable* Sender, Action* parameters)
{
	Scriptable* scr = GetScriptableFromObject(Sender, parameters->objects[1], GA_NO_DEAD);
	if (!scr || scr->Type != ST_ACTOR) {
		return;
	}
	Scriptable* tar = GetScriptableFromObject(Sender, parameters->objects[2], GA_NO_DEAD);
	if (!tar || (tar->Type != ST_ACTOR && tar->Type !=ST_DOOR && tar->Type !=ST_CONTAINER) ) {
		return;
	}
	//this is a hack, we use a gui variable for our own hideous reasons?
	if (tar->Type==ST_ACTOR) {
		const GameControl *gc = core->GetGameControl();
		if (gc) {
			//saving the target object ID from the gui variable
			scr->AddAction(GenerateActionDirect("NIDSpecial3()", static_cast<Actor*>(tar)));
		}
	} else {
		scr->AddAction(GenerateAction(fmt::format("BashDoor({})", tar->GetScriptName())));
	}
}

void GameScript::AttackReevaluate( Scriptable* Sender, Action* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		Sender->ReleaseCurrentAction();
		return;
	}

	if (!Sender->CurrentActionState) {
		Sender->CurrentActionState = parameters->int0Parameter;
		// TODO: reevaluate target (set CurrentActionTarget to 0) if we are not actively in combat
	}

	Scriptable* tar = GetStoredActorFromObject(Sender, parameters->objects[1], GA_NO_DEAD);
	if (!tar || (tar->Type != ST_ACTOR && tar->Type !=ST_DOOR && tar->Type !=ST_CONTAINER) ) {
		Sender->ReleaseCurrentAction();
		return;
	}

	//actor is already incapable of attack
	if (Sender->GetInternalFlag()&IF_STOPATTACK) {
		Sender->ReleaseCurrentAction();
		return;
	}

	// if same target as before, don't play the war cry again, as they'd pop up too often
	int flags = 0;
	if (Sender->LastTargetPersistent == tar->GetGlobalID()) {
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
		Sender->CurrentActionState = 0;
		Sender->ReleaseCurrentAction();
	}
}

void GameScript::Explore( Scriptable* Sender, Action* /*parameters*/)
{
	Sender->GetCurrentArea()->FillExplored(true);
}

void GameScript::UndoExplore( Scriptable* Sender, Action* /*parameters*/)
{
	Sender->GetCurrentArea()->FillExplored(false);
}

void GameScript::ExploreMapChunk( Scriptable* Sender, Action* parameters)
{
	Map *map = Sender->GetCurrentArea();
	/*
	There is a mode flag in int1Parameter, but i don't know what is it,
	our implementation uses it for LOS=1, or no LOS=0
	ExploreMapChunk will reveal both visibility/explored map, but the
	visibility will fade in the next update cycle (which is quite frequent)
	*/
	map->ExploreMapChunk(parameters->pointParameter, parameters->int0Parameter, parameters->int1Parameter);
}

void GameScript::StartStore( Scriptable* Sender, Action* parameters)
{
	if (core->GetCurrentStore() ) {
		return;
	}
	core->SetCurrentStore(parameters->resref0Parameter, Sender->GetGlobalID());
	core->SetEventFlag(EF_OPENSTORE);
	//sorry, i have absolutely no idea when i should do this :)
	Sender->ReleaseCurrentAction();
}

//The integer parameter is a GemRB extension, if set to 1, the player
//gains experience for learning the spell
void GameScript::AddSpecialAbility( Scriptable* Sender, Action* parameters)
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
void GameScript::RemoveSpell( Scriptable* Sender, Action* parameters)
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
	if (type==2) {
	//remove spell from both book and memorization
		actor->spellbook.RemoveSpell(spellRes);
		return;
	}
	//type == 1 remove spell only from memorization
	//type == 0 original behaviour: deplete only
	actor->spellbook.UnmemorizeSpell(spellRes, !type, 2);
}

void GameScript::SetScriptName( Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetScriptableFromObject(Sender, parameters->objects[1]);
	if (!tar || tar->Type!=ST_ACTOR) {
		return;
	}
	tar->SetScriptName(parameters->variable0Parameter);
}

//iwd2
//advance time with a constant
//This is in seconds according to IESDP
void GameScript::AdvanceTime(Scriptable* /*Sender*/, Action* parameters)
{
	core->GetGame()->AdvanceTime(parameters->int0Parameter * core->Time.ai_update_time);
	core->GetGame()->ResetPartyCommentTimes();
}

// advance at the beginning of the specified hour (minus one tick? unsure)
// the parameter is HOURS (time.ids, 0 to 23)
// never advance a full day or more (in fact, duplicating this action does nothing)
void GameScript::DayNight(Scriptable* /*Sender*/, Action* parameters)
{
	int delta = parameters->int0Parameter * core->Time.hour_size
	          - core->GetGame()->GameTime % core->Time.day_size;
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
	Game *game = core->GetGame();
	game->RestParty(REST_NOCHECKS, parameters->int0Parameter, parameters->int1Parameter);
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
	actor->SetBase(IE_FATIGUE,0);
}

//doesn't advance game time, just removes fatigue
void GameScript::RestNoSpells(Scriptable* Sender, Action* /*parameters*/)
{
	Actor* actor = Scriptable::As<Actor>(Sender);
	if (!actor) {
		return;
	}
	actor->fxqueue.RemoveAllEffects(fx_fatigue_ref);
	actor->SetBase(IE_FATIGUE,0);
}

//this most likely advances time and heals whole party
void GameScript::RestUntilHealed(Scriptable* Sender, Action* /*parameters*/)
{
	core->GetGame()->RestParty(REST_NOCHECKS, 0, 0);
	Sender->ReleaseCurrentAction();
}

//iwd2
//removes all delayed/duration/semi permanent effects (like a ctrl-r)
void GameScript::ClearPartyEffects(Scriptable* /*Sender*/, Action* /*parameters*/)
{
	const Game *game = core->GetGame();
	int i = game->GetPartySize(false);
	while (i--) {
		Actor *tar = game->GetPC(i, false);
		tar->fxqueue.RemoveExpiredEffects(0xffffffff);
	}
}

//iwd2 removes effects from a single sprite
void GameScript::ClearSpriteEffects(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetScriptableFromObject(Sender, parameters->objects[1]);
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
	const Scriptable* tar = GetScriptableFromObject(Sender, parameters->objects[1], GA_NO_DEAD);
	if (!tar || tar->Type!=ST_ACTOR) {
		return;
	}

	actor->LastMarked = tar->GetGlobalID();
}

void GameScript::MarkSpellAndObject(Scriptable* Sender, Action* parameters)
{
	Actor* me = Scriptable::As<Actor>(Sender);
	if (!me) {
		return;
	}
	if (me->LastMarkedSpell) {
		return;
	}

	const Scriptable* tar = GetScriptableFromObject(Sender, parameters->objects[1]);
	if (!tar) {
		// target died on us
		return;
	}
	const Actor* actor = Scriptable::As<Actor>(tar);

	int flags = parameters->int0Parameter;
	if (!(flags & MSO_IGNORE_NULL) && !actor) {
		return;
	}
	if (!(flags & MSO_IGNORE_INVALID) && actor && actor->InvalidSpellTarget() ) {
		return;
	}
	if (!(flags & MSO_IGNORE_SEE) && actor && !CanSee(Sender, actor, true, 0) ) {
		return;
	}
	uint8_t len = parameters->string0Parameter.length();
	// only allow multiples of 4
	if (!len || len & 3) {
		return;
	}
	len/=4;
	size_t max = len;
	size_t pos = 0;
	if (flags & MSO_RANDOM_SPELL) {
		pos = core->Roll(1, len, -1);
	}
	while(len--) {
		ResRef spl = SubStr(parameters->string0Parameter, static_cast<uint8_t>(pos * 4), 4);
		spl[4] = '\0';
		int splnum = atoi(spl.c_str());

		if (!(flags & MSO_IGNORE_HAVE) && !me->spellbook.HaveSpell(splnum, 0) ) {
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
		me->LastMarkedSpell = splnum;
		me->LastSpellTarget = tar->GetGlobalID();
		break;
end_mso_loop:
		pos++;
		if (pos==max) {
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
	actor->LastMarkedSpell = parameters->int0Parameter;
}

void GameScript::SetMarkedSpell(Scriptable* Sender, Action* parameters)
{
	Actor* actor = Scriptable::As<Actor>(Sender);
	if (!actor) {
		return;
	}
	if (parameters->int0Parameter) {
		if (actor->LastMarkedSpell) {
			return;
		}
		if (!actor->spellbook.HaveSpell(parameters->int0Parameter, 0) ) {
			return;
		}
	}

	actor->LastMarkedSpell = parameters->int0Parameter;
}

void GameScript::SetDialogueRange(Scriptable* Sender, Action* parameters)
{
	Actor* actor = Scriptable::As<Actor>(Sender);
	if (!actor) {
		return;
	}
	actor->SetBase( IE_DIALOGRANGE, parameters->int0Parameter );
}

void GameScript::SetGlobalTint(Scriptable* /*Sender*/, Action* parameters)
{
	Color c(parameters->int0Parameter, parameters->int1Parameter, parameters->int2Parameter, 0xff);
	core->GetWindowManager()->FadeColor = c;
}

void GameScript::SetArmourLevel(Scriptable* Sender, Action* parameters)
{
	const Scriptable* tar = GetScriptableFromObject(Sender, parameters->objects[1]);
	if (!tar || tar->Type!=ST_ACTOR) {
		return;
	}
	Actor* actor = static_cast<Actor*>(Sender);
	actor->SetBase( IE_ARMOR_TYPE, parameters->int0Parameter );
}

void GameScript::RandomWalk(Scriptable* Sender, Action* /*parameters*/)
{
	Actor* actor = Scriptable::As<Actor>(Sender);
	if (!actor) {
		Sender->ReleaseCurrentAction();
		return;
	}
	actor->RandomWalk( true, false );
}

void GameScript::RandomRun(Scriptable* Sender, Action* /*parameters*/)
{
	Actor* actor = Scriptable::As<Actor>(Sender);
	if (!actor) {
		Sender->ReleaseCurrentAction();
		return;
	}
	actor->RandomWalk( true, true );
}

void GameScript::RandomWalkContinuous(Scriptable* Sender, Action* /*parameters*/)
{
	Actor* actor = Scriptable::As<Actor>(Sender);
	if (!actor) {
		Sender->ReleaseCurrentAction();
		return;
	}
	actor->RandomWalk( false, false );
}

void GameScript::RandomFly(Scriptable* Sender, Action* /*parameters*/)
{
	Actor* actor = Scriptable::As<Actor>(Sender);
	if (!actor) {
		Sender->ReleaseCurrentAction();
		return;
	}
	int x = RAND(0,31);
	if (x<10) {
		actor->SetOrientation(PrevOrientation(actor->GetOrientation()), false);
	} else if (x>20) {
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

	Container *container = core->GetCurrentContainer();
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
		if (parameters->int1Parameter == (signed)distance) {
			parameters->int2Parameter++;
		} else {
			parameters->int1Parameter = distance;
		}
	}
	if (container->containerType == IE_CONTAINER_PILE && parameters->int2Parameter < 10) {
		needed = 0; // less than a search square (width)
	}
	if (distance<=needed)
	{
		//check if the container is unlocked
		if (!container->TryUnlock(actor)) {
			//playsound can't open container
			//display string, etc
			if (core->HasFeedback(FT_MISC)) displaymsg->DisplayConstantString(STR_CONTLOCKED, GUIColors::LIGHTGREY, container);
			Sender->ReleaseCurrentAction();
			return;
		}
		actor->SetModal(MS_NONE);
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
	Scriptable* tar = GetScriptableFromObject(Sender, parameters->objects[1]);
	if (!tar || tar->Type != ST_ACTOR) {
		Sender->ReleaseCurrentAction(); //why blocking???
		return;
	}
	Action *newaction = GenerateAction("UseContainer()");
	tar->AddActionInFront(newaction);
	Sender->ReleaseCurrentAction(); //why blocking???
}

//these actions directly manipulate a game variable (as the original engine)
void GameScript::SetMazeEasier(Scriptable* Sender, Action* /*parameters*/)
{
	int value = CheckVariable( Sender, "MAZEDIFFICULTY","GLOBAL");
	if (value>0) {
		SetVariable(Sender, "MAZEDIFFICULTY", value-1, "GLOBAL");
	}
}

void GameScript::SetMazeHarder(Scriptable* Sender, Action* /*parameters*/)
{
	int value = CheckVariable( Sender, "MAZEDIFFICULTY","GLOBAL");
	if (value<2) {
		SetVariable(Sender, "MAZEDIFFICULTY", value+1, "GLOBAL");
	}
}

void GameScript::GenerateMaze(Scriptable* /*Sender*/, Action* /*parameters*/)
{
	core->SetEventFlag(EF_CREATEMAZE);
}

void GameScript::FixEngineRoom(Scriptable* Sender, Action* /*parameters*/)
{
	int value = CheckVariable( Sender, "EnginInMaze","GLOBAL");
	if (value) {
		SetVariable(Sender, "EnginInMaze", 0, "GLOBAL");
		//this works only because the engine room exit depends only on the EnginInMaze variable
		ScriptEngine *sE = core->GetGUIScriptEngine();
		sE->RunFunction("Maze", "CustomizeArea");
	}
}

void GameScript::StartRainNow(Scriptable* /*Sender*/, Action* /*parameters*/)
{
	core->GetGame()->StartRainOrSnow( false, WB_RAIN|WB_RARELIGHTNING);
}

void GameScript::Weather(Scriptable* /*Sender*/, Action* parameters)
{
	Game *game = core->GetGame();
	switch(parameters->int0Parameter & WB_FOG) {
		case WB_NORMAL:
			game->StartRainOrSnow( false, 0);
			break;
		case WB_RAIN:
			game->StartRainOrSnow( true, WB_RAIN|WB_RARELIGHTNING);
			break;
		case WB_SNOW:
			game->StartRainOrSnow( true, WB_SNOW);
			break;
		case WB_FOG:
			game->StartRainOrSnow( true, WB_FOG);
			break;
	}
}

// Pos could be [-1,-1] in which case it copies the ground piles to their
// original position in the second area
void GameScript::CopyGroundPilesTo(Scriptable* Sender, Action* parameters)
{
	const Map *map = Sender->GetCurrentArea();
	Map *othermap = core->GetGame()->GetMap(parameters->resref0Parameter, false );
	if (!othermap) {
		return;
	}

	size_t containerCount = map->GetTileMap()->GetContainerCount();
	while (containerCount--) {
		Container* pile = map->GetTileMap()->GetContainer(containerCount);
		if (pile->containerType != IE_CONTAINER_PILE) continue;

		// creating (or grabbing) the container in the other map at the given position
		Container *otherPile;
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

	actor->SetModalSpell(MS_BATTLESONG, songs[songIdx]);
	actor->SetModal(MS_BATTLESONG);
}

void GameScript::BattleSong(Scriptable* Sender, Action* /*parameters*/)
{
	Actor* actor = Scriptable::As<Actor>(Sender);
	if (!actor) {
		return;
	}
	actor->SetModal( MS_BATTLESONG);
}

void GameScript::FindTraps(Scriptable* Sender, Action* /*parameters*/)
{
	Actor* actor = Scriptable::As<Actor>(Sender);
	if (!actor) {
		return;
	}
	actor->SetModal( MS_DETECTTRAPS);
}

void GameScript::Hide(Scriptable* Sender, Action* /*parameters*/)
{
	Actor* actor = Scriptable::As<Actor>(Sender);
	if (!actor) {
		return;
	}

	if (actor->TryToHide()) {
		actor->SetModal(MS_STEALTH);
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

	if (actor->Modal.State == MS_STEALTH) {
		actor->SetModal(MS_NONE);
	}
	actor->fxqueue.RemoveAllEffects(fx_set_invisible_state_ref);
}

void GameScript::Turn(Scriptable* Sender, Action* /*parameters*/)
{
	Actor* actor = Scriptable::As<Actor>(Sender);
	if (!actor) {
		return;
	}

	if (actor->Modified[IE_DISABLEDBUTTON] & (1<<ACT_TURN)) {
		return;
	}

	int skill = actor->GetStat(IE_TURNUNDEADLEVEL);
	if (skill < 1) return;

	actor->SetModal(MS_TURNUNDEAD);

}

void GameScript::TurnAMT(Scriptable* Sender, Action* parameters)
{
	Actor* actor = Scriptable::As<Actor>(Sender);
	if (!actor) {
		Sender->ReleaseCurrentAction();
		return;
	}
	actor->SetOrientation(NextOrientation(actor->GetOrientation(), parameters->int0Parameter), true);
	actor->SetWait( 1 );
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
	actor->SetWait(core->Time.ai_update_time * core->Roll(1, diceSides, 0));
}

void GameScript::AttachTransitionToDoor(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetScriptableFromObject(Sender, parameters->objects[1]);
	Door *door = Scriptable::As<Door>(tar);
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
	const Scriptable* tar = GetScriptableFromObject(Sender, parameters->objects[1]);
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
	const Scriptable* tar = GetScriptableFromObject(Sender, parameters->objects[1]);
	const Actor* actor = Scriptable::As<Actor>(tar);
	if (!actor) {
		return;
	}

	act->SetBase(IE_ANIMATION_ID, actor->GetBase(IE_ANIMATION_ID) );
}

void GameScript::ExportParty(Scriptable* /*Sender*/, Action* parameters)
{
	const Game *game = core->GetGame();
	int i = game->GetPartySize(false);
	while (i--) {
		const Actor *actor = game->GetPC(i, false);
		std::string fname = fmt::format("{}{}", parameters->string0Parameter, i + 1);
		core->WriteCharacter(fname, actor);
	}
	displaymsg->DisplayConstantString(STR_EXPORTED, GUIColors::XPCHANGE);
}

void GameScript::SaveGame(Scriptable* /*Sender*/, Action* parameters)
{
	if (core->HasFeature(GF_STRREF_SAVEGAME)) {
		std::string basename = "Auto-Save";
		AutoTable tab = gamedata->LoadTable("savegame");
		if (tab) {
			basename = tab->QueryDefault();
		}
		String str = core->GetString(ieStrRef(parameters->int0Parameter), STRING_FLAGS::STRREFOFF);
		std::string FolderName = fmt::format("{} - {}", basename, fmt::WideToChar{str});
		core->GetSaveGameIterator()->CreateSaveGame(core->GetSaveGameIterator()->GetSaveGame(FolderName), FolderName, true);
	} else {
		core->GetSaveGameIterator()->CreateSaveGame(parameters->int0Parameter);
	}
}

/*EscapeAreaMove(S:Area*,I:X*,I:Y*,I:Face*)*/
void GameScript::EscapeArea(Scriptable* Sender, Action* parameters)
{
	ScriptDebugLog(ID_ACTIONS, "EscapeArea/EscapeAreaMove");

	if (Sender->Type!=ST_ACTOR) {
		Sender->ReleaseCurrentAction();
		return;
	}
	Map *map = Sender->GetCurrentArea();
	if (!map) {
		Sender->ReleaseCurrentAction();
		return;
	}

	Point p = Sender->Pos;
	map->TMap->AdjustNearestTravel(p);

	if (!parameters->resref0Parameter.IsEmpty()) {
		Point q(parameters->int0Parameter, parameters->int1Parameter);
		EscapeAreaCore(Sender, p, parameters->resref0Parameter, q, 0, parameters->int2Parameter);
	} else {
		EscapeAreaCore(Sender, p, parameters->resref0Parameter, p, EA_DESTROY, parameters->int0Parameter);
	}
	//EscapeAreaCore will do its ReleaseCurrentAction
	//Sender->ReleaseCurrentAction();
}

void GameScript::EscapeAreaNoSee(Scriptable* Sender, Action* parameters)
{
	ScriptDebugLog(ID_ACTIONS, "EscapeAreaNoSee");

	if (Sender->Type!=ST_ACTOR) {
		Sender->ReleaseCurrentAction();
		return;
	}
	Map *map = Sender->GetCurrentArea();
	if (!map) {
		Sender->ReleaseCurrentAction();
		return;
	}

	Point p = Sender->Pos;
	map->TMap->AdjustNearestTravel(p);

	if (!parameters->resref0Parameter.IsEmpty()) {
		Point q(parameters->int0Parameter, parameters->int1Parameter);
		EscapeAreaCore(Sender, p, parameters->resref0Parameter, q, 0, parameters->int2Parameter);
	} else {
		EscapeAreaCore(Sender, p, parameters->resref0Parameter, p, EA_DESTROY|EA_NOSEE, parameters->int0Parameter);
	}
	//EscapeAreaCore will do its ReleaseCurrentAction
	//Sender->ReleaseCurrentAction();
}

void GameScript::EscapeAreaDestroy(Scriptable* Sender, Action* parameters)
{
	if (Sender->Type!=ST_ACTOR) {
		Sender->ReleaseCurrentAction();
		return;
	}
	Map *map = Sender->GetCurrentArea();
	if (!map) {
		Sender->ReleaseCurrentAction();
		return;
	}

	//find nearest exit
	Point p = Sender->Pos;
	map->TMap->AdjustNearestTravel(p);
	//EscapeAreaCore will do its ReleaseCurrentAction
	EscapeAreaCore(Sender, p, parameters->resref0Parameter, p, EA_DESTROY, parameters->int0Parameter);
}

/*EscapeAreaObjectMove(S:Area*,I:X*,I:Y*,I:Face*)*/
void GameScript::EscapeAreaObject(Scriptable* Sender, Action* parameters)
{
	if (Sender->Type!=ST_ACTOR) {
		Sender->ReleaseCurrentAction();
		return;
	}
	const Map *map = Sender->GetCurrentArea();
	if (!map) {
		Sender->ReleaseCurrentAction();
		return;
	}

	const Scriptable* tar = GetScriptableFromObject(Sender, parameters->objects[1]);
	if (!tar) {
		Sender->ReleaseCurrentAction();
		return;
	}
	Point p = tar->Pos;
	if (!parameters->resref0Parameter.IsEmpty()) {
		Point q(parameters->int0Parameter, parameters->int1Parameter);
		EscapeAreaCore(Sender, p, parameters->resref0Parameter, q, 0, parameters->int2Parameter);
	} else {
		EscapeAreaCore(Sender, p, {}, p, EA_DESTROY, parameters->int0Parameter);
	}
	//EscapeAreaCore will do its ReleaseCurrentAction
}

//This one doesn't require the object to be seen?
//We don't have that feature yet, so this is the same as EscapeAreaObject
void GameScript::EscapeAreaObjectNoSee(Scriptable* Sender, Action* parameters)
{
	if (Sender->Type!=ST_ACTOR) {
		Sender->ReleaseCurrentAction();
		return;
	}
	const Map *map = Sender->GetCurrentArea();
	if (!map) {
		Sender->ReleaseCurrentAction();
		return;
	}

	const Scriptable* tar = GetScriptableFromObject(Sender, parameters->objects[1]);
	if (!tar) {
		Sender->ReleaseCurrentAction();
		return;
	}
	Point p = tar->Pos;
	Sender->SetWait(parameters->int0Parameter);
	if (!parameters->resref0Parameter.IsEmpty()) {
		Point q(parameters->int0Parameter, parameters->int1Parameter);
		EscapeAreaCore(Sender, p, parameters->resref0Parameter, q, 0, parameters->int2Parameter);
	} else {
		EscapeAreaCore(Sender, p, {}, p, EA_DESTROY|EA_NOSEE, parameters->int0Parameter);
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
	Map *map = scr->GetCurrentArea();
	Container *c = map->GetPile(scr->Pos);
	if (!c) { //this shouldn't happen, but lets prepare for the worst
		return;
	}

	//the following part is coming from GUISCript.cpp with trivial changes
	int Slot = c->inventory.FindItem(parameters->resref0Parameter, 0);
	if (Slot<0) {
		return;
	}
	int res = core->CanMoveItem(c->inventory.GetSlotItem(Slot) );
	if (!res) { //cannot move
		return;
	}
	CREItem *item = c->RemoveItem(Slot,0);
	if (!item) {
		return;
	}
	if (res!=-1 && scr->InParty) { //it is gold and we got the party pool!
		if (scr->InParty) {
			core->GetGame()->PartyGold += res;
			// if you want message here then use core->GetGame()->AddGold(res);
		} else {
			scr->SetBase(IE_GOLD, scr->GetBase(IE_GOLD) + res);
		}
		delete item;
		return;
	}
	res = scr->inventory.AddSlotItem(item, SLOT_ONLYINVENTORY);
	if (res !=ASI_SUCCESS) { //putting it back
		c->AddItem(item);
	}
	return;
}

void GameScript::ChangeStoreMarkup(Scriptable* /*Sender*/, Action* parameters)
{
	bool has_current = false;
	ResRef current;
	ieDword owner;

	Store *store = core->GetCurrentStore();
	if (!store) {
		store = core->SetCurrentStore(parameters->resref0Parameter, 0);
	} else {
		if (store->Name != parameters->resref0Parameter) {
			//not the current store, we need some dirty hack
			has_current = true;
			current = store->Name;
			owner = store->GetOwnerID();
		}
	}
	store->BuyMarkup = parameters->int0Parameter;
	store->SellMarkup = parameters->int1Parameter;
	//additional markup, is this depreciation???
	store->DepreciationRate = parameters->int2Parameter;
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
	WMPAreaLink *link = wmap->GetLink(parameters->resref0Parameter, parameters->resref1Parameter);
	if (!link) {
		return;
	}
	link->EncounterChance = parameters->int0Parameter;
}

void GameScript::SpawnPtActivate(Scriptable* Sender, Action* parameters)
{
	if (parameters->objects[1]) {
		const Map *map = Sender->GetCurrentArea();
		Spawn *spawn = map->GetSpawn(parameters->objects[1]->objectName);
		if (spawn) {
			spawn->Enabled = 1;
		}
	}
}

void GameScript::SpawnPtDeactivate(Scriptable* Sender, Action* parameters)
{
	if (parameters->objects[1]) {
		const Map *map = Sender->GetCurrentArea();
		Spawn *spawn = map->GetSpawn(parameters->objects[1]->objectName);
		if (spawn) {
			spawn->Enabled = 0;
		}
	}
}

void GameScript::SpawnPtSpawn(Scriptable* Sender, Action* parameters)
{
	if (parameters->objects[1]) {
		Map *map = Sender->GetCurrentArea();
		Spawn *spawn = map->GetSpawn(parameters->objects[1]->objectName);
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

	Scriptable* tar = GetScriptableFromObject(Sender, parameters->objects[1]);
	if (!tar) {
		return;
	}
	if (tar->Type==ST_ACTOR) {
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

	const Scriptable* tar = GetScriptableFromObject(Sender, parameters->objects[1]);
	const Actor* actor = Scriptable::As<Actor>(tar);
	if (actor) {
		value = actor->GetStat( parameters->int0Parameter );
	}
	SetVariable( Sender, parameters->string0Parameter, value );
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
	GameControl *gc = core->GetGameControl();
	if (gc) {
		gc->SetDialogueFlags(DF_FREEZE_SCRIPTS, BitOp::OR);
		displaymsg->DisplayConstantString(STR_SCRIPTPAUSED, GUIColors::RED);
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
	if (!ip || (ip->Type!=ST_TRIGGER && ip->Type!=ST_TRAVEL && ip->Type!=ST_PROXIMITY)) {
		Log(WARNING, "Actions", "Script error: No trigger named \"{}\"", name);
		parameters->dump();
		return;
	}

	ip->ClearTriggers();
	// we also need to reset the IF_INTRAP bit for any actors that are inside or subsequent triggers will be skipped
	// there are only two users of this action, so we can be a bit sloppy and skip the geometry checks
	std::vector<Actor *> nearActors = Sender->GetCurrentArea()->GetAllActorsInRadius(ip->Pos, GA_NO_LOS|GA_NO_DEAD|GA_NO_UNSCHEDULED, MAX_OPERATING_DISTANCE);
	for (const auto& candidate : nearActors) {
		candidate->SetInTrap(false);
	}
}

void GameScript::UseDoor(Scriptable* Sender, Action* parameters)
{
	GameControl *gc = core->GetGameControl();
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
	GameControl *gc = core->GetGameControl();
	if (!gc) {
		Sender->ReleaseCurrentAction();
		return;
	}
	Actor* actor = Scriptable::As<Actor>(Sender);
	if (!actor) {
		Sender->ReleaseCurrentAction();
		return;
	}

	Scriptable* target = GetScriptableFromObject(Sender, parameters->objects[1]);
	if (!target) {
		Sender->ReleaseCurrentAction();
		return;
	}

	Door *door = nullptr;
	Container *container = nullptr;
	Point pos;
	if (target->Type == ST_DOOR) {
		door = static_cast<Door*>(target);
		pos = door->toOpen[0];
		const Point& otherp = door->toOpen[1];
		if (Distance(pos, Sender) > Distance(otherp, Sender)) {
			pos = otherp;
		}
	} else if(target->Type == ST_CONTAINER) {
		container = static_cast<Container*>(target);
		pos = target->Pos;
	} else {
		Sender->ReleaseCurrentAction();
		return;
	}

	if (SquaredPersonalDistance(pos, Sender) > MAX_OPERATING_DISTANCE * MAX_OPERATING_DISTANCE) {
		MoveNearerTo(Sender, pos, MAX_OPERATING_DISTANCE, 0);
		return;
	}

	//bashing makes the actor visible
	actor->CureInvisibility();
	gc->SetTargetMode(TARGET_MODE_ATTACK); //for bashing doors too

	// try to bash it
	if (door) {
		door->TryBashLock(actor);
	} else if (container) {
		container->TryBashLock(actor);
	}

	Sender->ReleaseCurrentAction();
}

//pst action
void GameScript::ActivatePortalCursor(Scriptable* Sender, Action* parameters)
{
	Scriptable* ip;
	if (!parameters->objects[1]) {
		ip=Sender;
	} else {
		ip = Sender->GetCurrentArea()->TMap->GetInfoPoint(parameters->objects[1]->objectNameVar);
	}
	if (!ip) {
		return;
	}
	if (ip->Type!=ST_PROXIMITY && ip->Type!=ST_TRAVEL) {
		return;
	}
	InfoPoint *tar = static_cast<InfoPoint*>(ip);
	if (parameters->int0Parameter) {
		tar->Trapped|=PORTAL_CURSOR;
	} else {
		tar->Trapped&=~PORTAL_CURSOR;
	}
}

//pst action
void GameScript::EnablePortalTravel(Scriptable* Sender, Action* parameters)
{
	Scriptable* ip;
	if (!parameters->objects[1]) {
		ip=Sender;
	} else {
		ip = Sender->GetCurrentArea()->TMap->GetInfoPoint(parameters->objects[1]->objectNameVar);
	}
	if (!ip) {
		return;
	}
	if (ip->Type!=ST_PROXIMITY && ip->Type!=ST_TRAVEL) {
		return;
	}
	InfoPoint *tar = static_cast<InfoPoint*>(ip);
	if (parameters->int0Parameter) {
		tar->Trapped|=PORTAL_TRAVEL;
	} else {
		tar->Trapped&=~PORTAL_TRAVEL;
	}
}

//unhardcoded iwd action (for the forge entrance change)
void GameScript::ChangeDestination(Scriptable* Sender, Action* parameters)
{
	InfoPoint *ip = Sender->GetCurrentArea()->TMap->GetInfoPoint(parameters->objects[1]->objectNameVar);
	if (ip && (ip->Type==ST_TRAVEL) ) {
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
	if ( parameters->int0Parameter != 0 ) {
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
	actor->inventory.EquipBestWeapon(EQUIP_MELEE|EQUIP_RANGED);
}

void GameScript::SetBestWeapon(Scriptable* Sender, Action* parameters)
{
	Actor* actor = Scriptable::As<Actor>(Sender);
	if (!actor) {
		return;
	}

	const Scriptable* tar = GetScriptableFromObject(Sender, parameters->objects[1]);
	const Actor* target = Scriptable::As<Actor>(tar);
	if (!target) {
		return;
	}

	if (PersonalDistance(actor,target)>(unsigned int) parameters->int0Parameter) {
		actor->inventory.EquipBestWeapon(EQUIP_RANGED);
	} else {
		actor->inventory.EquipBestWeapon(EQUIP_MELEE);
	}
}

void GameScript::FakeEffectExpiryCheck(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetScriptableFromObject(Sender, parameters->objects[1]);
	Actor* target = Scriptable::As<Actor>(tar);
	if (!target) {
		return;
	}

	target->fxqueue.RemoveExpiredEffects(parameters->int0Parameter * core->Time.ai_update_time);
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
	int slot = parameters->int0Parameter;
	int wslot = Inventory::GetWeaponSlot();
	//weapon
	if (core->QuerySlotType(slot)&SLOT_WEAPON) {
		slot-=wslot;
		if (slot<0 || slot>=MAX_QUICKWEAPONSLOT) {
			return;
		}
		scr->SetEquippedQuickSlot(slot, parameters->int1Parameter);
		return;
	}
	//quick item
	wslot = Inventory::GetQuickSlot();
	if (core->QuerySlotType(slot)&SLOT_ITEM) {
		slot-=wslot;
		if (slot<0 || slot>=MAX_QUICKITEMSLOT) {
			return;
		}
		if (scr->PCStats) {
			scr->PCStats->QuickItemHeaders[slot]=(ieWord) parameters->int1Parameter;
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
	const Scriptable* tar = GetStoredActorFromObject(Sender, parameters->objects[1]);
	if (!tar) {
		Sender->ReleaseCurrentAction();
		return;
	}
	int Slot;
	ieDword header;
	ieDword flags = 0; // aura pollution is on for everyone
	ResRef itemres;

	if (!parameters->resref0Parameter.IsEmpty()) {
		Slot = act->inventory.FindItem(parameters->resref0Parameter, IE_INV_ITEM_UNDROPPABLE);
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

	if (!ResolveItemName( itemres, act, Slot) ) {
		Sender->ReleaseCurrentAction();
		return;
	}

	double angle = AngleFromPoints(Sender->Pos, tar->Pos);
	unsigned int dist = GetItemDistance(itemres, header, angle);
	if (PersonalDistance(Sender, tar) > dist) {
		MoveNearerTo(Sender, tar, dist);
		return;
	}

	// only one use per round; skip for our internal attack projectile
	if (!(flags & UI_NOAURA) && act->AuraPolluted()) {
		return;
	}

	act->UseItem(Slot, header, tar, flags);
	Sender->ReleaseCurrentAction();
}

void GameScript::UseItemPoint(Scriptable* Sender, Action* parameters)
{
	Actor* act = Scriptable::As<Actor>(Sender);
	if (!act) {
		Sender->ReleaseCurrentAction();
		return;
	}
	int Slot;
	ieDword header;
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

	if (!ResolveItemName( itemres, act, Slot) ) {
		Sender->ReleaseCurrentAction();
		return;
	}

	double angle = AngleFromPoints(Sender->Pos, parameters->pointParameter);
	unsigned int dist = GetItemDistance(itemres, header, angle);
	if (PersonalDistance(parameters->pointParameter, Sender) > dist) {
		MoveNearerTo(Sender, parameters->pointParameter, dist, 0);
		return;
	}

	// only one use per round; skip for our internal attack projectile
	if (!(flags & UI_NOAURA) && act->AuraPolluted()) {
		return;
	}

	act->UseItemPoint(Slot, header, parameters->pointParameter, flags);
	Sender->ReleaseCurrentAction();
}

//addfeat will be able to remove feats too
//(the second int parameter is a value to add to the feat)
void GameScript::AddFeat(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetScriptableFromObject(Sender, parameters->objects[1]);
	Actor* actor = Scriptable::As<Actor>(tar);
	if (!actor) {
		return;
	}
	//value to add to the feat
	int value = parameters->int1Parameter;
	//default is increase by 1
	if (!value) value = 1;
	value += actor->GetFeat(parameters->int0Parameter);
	//SetFeatValue will handle edges
	actor->SetFeatValue(parameters->int0Parameter, value);
}

void GameScript::MatchHP(Scriptable* Sender, Action* parameters)
{
	if (Sender->Type!=ST_ACTOR) {
		return;
	}
	const Actor *scr = (const Actor *) Sender;
	Scriptable* tar = GetScriptableFromObject(Sender, parameters->objects[1]);
	Actor* actor = Scriptable::As<Actor>(tar);
	if (!actor) {
		return;
	}
	switch (parameters->int0Parameter) {
		case 1: //sadly the hpflags are not the same as stats
			actor->SetBase(IE_HITPOINTS,scr->GetBase(IE_HITPOINTS));
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
	if (stat<9 || stat>14) {
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
	GameControl *gc = core->GetGameControl();
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
	const Object *ob = parameters->objects[1];
	if (!ob) {
		return;
	}

	for (int i=0;i<MAX_OBJECT_FIELDS;i++) {
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
	Action *newact = GenerateAction(std::move(tmp));
	Sender->AddAction(newact);
}

//same as MoveToPointNoRecticle, but not blocking
void GameScript::Follow(Scriptable* Sender, Action* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		return;
	}

	std::string tmp = fmt::format("MoveToPointNoRecticle([{}.{}])", parameters->pointParameter.x, parameters->pointParameter.y);
	Action *newact = GenerateAction(std::move(tmp));
	Sender->AddAction(newact);
}

void GameScript::FollowCreature(Scriptable* Sender, Action* parameters)
{
	Actor* scr = Scriptable::As<Actor>(Sender);
	if (!scr) {
		Sender->ReleaseCurrentAction();
		return;
	}

	const Scriptable* tar = GetStoredActorFromObject(Sender, parameters->objects[1]);
	const Actor* actor = Scriptable::As<Actor>(tar);
	if (!actor) {
		Sender->ReleaseCurrentAction();
		return;
	}

	scr->LastFollowed = actor->GetGlobalID();
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

	const Scriptable* tar = GetStoredActorFromObject(Sender, parameters->objects[1]);
	const Actor* actor = Scriptable::As<Actor>(tar);
	if (!actor) {
		Sender->ReleaseCurrentAction();
		return;
	}

	scr->LastFollowed = actor->GetGlobalID();
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
		scr->WalkTo( parameters->pointParameter, 0, 1 );
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

	Scriptable* tar = GetStoredActorFromObject( Sender, parameters->objects[1] );
	Actor* actor = Scriptable::As<Actor>(tar);
	if (!actor) {
		Sender->ReleaseCurrentAction();
		return;
	}

	scr->LastFollowed = actor->GetGlobalID();
	scr->LastProtectee = actor->GetGlobalID();
	actor->LastProtector = scr->GetGlobalID();
	//not exactly range
	scr->FollowOffset.x = parameters->int0Parameter;
	scr->FollowOffset.y = parameters->int0Parameter;
	if (!scr->InMove() || scr->Destination != tar->Pos) {
		scr->WalkTo( tar->Pos, 0, MAX_OPERATING_DISTANCE );
	}
	// we should handle 'Protect' here rather than just unblocking
	Sender->ReleaseCurrentAction();
}

//keeps following the object in formation
void GameScript::FollowObjectFormation(Scriptable* Sender, Action* parameters)
{
	const GameControl *gc = core->GetGameControl();
	if (!gc) {
		Sender->ReleaseCurrentAction();
		return;
	}
	Actor* scr = Scriptable::As<Actor>(Sender);
	if (!scr) {
		Sender->ReleaseCurrentAction();
		return;
	}

	const Scriptable* tar = GetStoredActorFromObject(Sender, parameters->objects[1]);
	const Actor* actor = Scriptable::As<Actor>(tar);
	if (!actor) {
		Sender->ReleaseCurrentAction();
		return;
	}

	scr->LastFollowed = actor->GetGlobalID();
	ieDword formation = parameters->int0Parameter;
	ieDword pos = parameters->int1Parameter;
	scr->FollowOffset = gc->GetFormationOffset(formation, pos);
	if (!scr->InMove() || scr->Destination != tar->Pos) {
		scr->WalkTo( tar->Pos, 0, 1 );
	}
	Sender->ReleaseCurrentAction();
}

//walks to a specific offset of target (quite like movetoobject)
void GameScript::Formation(Scriptable* Sender, Action* parameters)
{
	const GameControl *gc = core->GetGameControl();
	if (!gc) {
		Sender->ReleaseCurrentAction();
		return;
	}
	Actor* scr = Scriptable::As<Actor>(Sender);
	if (!scr) {
		Sender->ReleaseCurrentAction();
		return;
	}
	const Scriptable* tar = GetStoredActorFromObject(Sender, parameters->objects[1]);
	if (!tar) {
		Sender->ReleaseCurrentAction();
		return;
	}

	ieDword formation = parameters->int0Parameter;
	ieDword pos = parameters->int1Parameter;
	Point FollowOffset = gc->GetFormationOffset(formation, pos) + tar->Pos;
	if (!scr->InMove() || scr->Destination != FollowOffset) {
		scr->WalkTo( FollowOffset, 0, 1 );
	}
}

void GameScript::TransformItem(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetScriptableFromObject(Sender, parameters->objects[1]);
	if (!tar || tar->Type!=ST_ACTOR) {
		return;
	}
	TransformItemCore((Actor *)tar, parameters, true);
}

void GameScript::TransformPartyItem(Scriptable* /*Sender*/, Action* parameters)
{
	const Game *game = core->GetGame();
	int i = game->GetPartySize(false);
	while (i--) {
		Actor *tar = game->GetPC(i, false);
		TransformItemCore(tar, parameters, true);
	}
}

void GameScript::TransformItemAll(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetScriptableFromObject(Sender, parameters->objects[1]);
	if (!tar || tar->Type!=ST_ACTOR) {
		return;
	}
	TransformItemCore((Actor *)tar, parameters, false);
}

void GameScript::TransformPartyItemAll(Scriptable* /*Sender*/, Action* parameters)
{
	const Game *game = core->GetGame();
	int i = game->GetPartySize(false);
	while (i--) {
		Actor *tar = game->GetPC(i, false);
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
	Actor *actor = core->GetGame()->FindNPC(string);
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
	core->GetGameControl()->DebugFlags |= DEBUG_SHOW_FOG_ALL;
}

void GameScript::DisableFogDither(Scriptable* /*Sender*/, Action* /*parameters*/)
{
	core->GetGameControl()->DebugFlags &= ~DEBUG_SHOW_FOG_ALL;
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
	Scriptable* tar = GetScriptableFromObject(Sender, parameters->objects[1]);
	Actor* actor = Scriptable::As<Actor>(tar);
	if (!actor) {
		return;
	}
	//the PST crew apparently loved hardcoding stuff
	static ResRef RebusResRef { "DABUS1" };
	RebusResRef[5]=(char) core->Roll(1,5,'0');
	ScriptedAnimation *vvc = gamedata->GetScriptedAnimation(RebusResRef, false);
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
	ieDword value = CheckVariable(Sender, variable, "GLOBAL" ) + 1;
	SetVariable(Sender, variable, value, "GLOBAL");
}

//this action uses the sceffect.ids (which should be covered by our cgtable.2da)
//The original engines solved cg by hardcoding either vvcs or sparkles
//so either we include sparkles as possible CG or just simulate all of these with projectiles
//in any case, this action just creates an opcode (0xeb) and plays sound
static EffectRef fx_iwd_casting_glow_ref = { "CastingGlow2", -1 };

void GameScript::SpellCastEffect(Scriptable* Sender, Action* parameters)
{
	Scriptable* src = GetScriptableFromObject(Sender, parameters->objects[1]);
	Actor* actor = Scriptable::As<Actor>(src);
	if (!actor) {
		return;
	}

	ieDword sparkle = parameters->int0Parameter;

	int opcode = EffectQueue::ResolveEffect(fx_iwd_casting_glow_ref);
	Effect *fx = core->GetEffect(opcode);
	if (!fx) {
		//invalid effect name didn't resolve to opcode
		return;
	}

	unsigned int channel = SFX_CHAN_DIALOG;
	if (actor->InParty > 0) {
		channel = SFX_CHAN_CHAR0 + actor->InParty - 1;
	} else if (actor->GetStat(IE_EA) >= EA_EVILCUTOFF) {
		channel = SFX_CHAN_MONSTER;
	}

	// voice
	core->GetAudioDrv()->Play(parameters->string0Parameter, channel, Sender->Pos, GEM_SND_SPEECH | GEM_SND_QUEUE);
	// starting sound, played at the same time, but on a different channel
	core->GetAudioDrv()->Play(parameters->string1Parameter, SFX_CHAN_CASTING, Sender->Pos, GEM_SND_QUEUE);
	// NOTE: only a few uses have also an ending sound that plays when the effect ends (also stopping Sound1)
	// but we don't even read all three string parameters, as Action stores just two
	// seems like a waste of memory to impose it on everyone, just for these few users
	// if it happens at some point, fx->Resource = parameters->string2Parameter and deal with it in the effect

	const Actor* caster = Scriptable::As<Actor>(Sender);
	int adjustedDuration = 0;
	if (caster) {
		adjustedDuration = parameters->int1Parameter - caster->GetStat(IE_MENTALSPEED);
		if (adjustedDuration < 0) adjustedDuration = 0;
	}
	adjustedDuration *= 10; // it's really not core->Time.ai_update_time

	// tell the effect to not use the main casting glow
	fx->Parameter4 = 1;
	fx->ProbabilityRangeMax = 100;
	fx->ProbabilityRangeMin = 0;
	fx->Parameter2 = sparkle; //animation type
	fx->TimingMode = FX_DURATION_INSTANT_LIMITED_TICKS;
	fx->Duration = adjustedDuration;
	fx->Target = FX_TARGET_PRESET;
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
	Scriptable* src = GetScriptableFromObject(Sender, parameters->objects[1]);
	if (!src) {
		return;
	}
	Scriptable* tar = GetScriptableFromObject(Sender, parameters->objects[2]);
	Actor* target = Scriptable::As<Actor>(tar);
	if (!target) {
		return;
	}
	int opcode = EffectQueue::ResolveEffect(fx_iwd_visual_spell_hit_ref);
	Effect *fx = core->GetEffect(opcode);
	if (!fx) {
		//invalid effect name didn't resolve to opcode
		return;
	}

	//vvc type
	fx->Parameter2 = parameters->int0Parameter + 0x1001;
	//height (not sure if this is in the opcode, but seems acceptable)
	fx->Parameter1 = parameters->int1Parameter;
	fx->Parameter4 = 1; // mark for special treatment
	fx->ProbabilityRangeMax = 100;
	fx->ProbabilityRangeMin = 0;
	fx->TimingMode=FX_DURATION_INSTANT_PERMANENT_AFTER_BONUSES;
	fx->Pos = tar->Pos;
	fx->Target = FX_TARGET_PRESET;
	core->ApplyEffect(fx, target, src);
}

void GameScript::SpellHitEffectPoint(Scriptable* Sender, Action* parameters)
{
	Scriptable* src = GetScriptableFromObject(Sender, parameters->objects[1]);
	if (!src) {
		return;
	}

	int opcode = EffectQueue::ResolveEffect(fx_iwd_visual_spell_hit_ref);
	Effect *fx = core->GetEffect(opcode);
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
	fx->TimingMode=FX_DURATION_INSTANT_PERMANENT_AFTER_BONUSES;
	// iwd2 with [-1.-1] again
	if (parameters->pointParameter.x == -1) {
		fx->Pos = src->Pos;
	} else {
		fx->Pos = parameters->pointParameter;
	}
	fx->Target = FX_TARGET_PRESET;
	core->ApplyEffect(fx, NULL, src);

	// it should probably wait until projectile payload, but a single tick works well for the use in 41cnatew
	Sender->SetWait(1);
}


void GameScript::ClickLButtonObject(Scriptable* Sender, Action* parameters)
{
	const Scriptable* tar = GetScriptableFromObject(Sender, parameters->objects[1]);
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
	const Scriptable* tar = GetScriptableFromObject(Sender, parameters->objects[1]);
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
	const Scriptable* tar = GetScriptableFromObject(Sender, parameters->objects[1]);
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
	const Scriptable* tar = GetScriptableFromObject(Sender, parameters->objects[1]);
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
	const Scriptable* tar = GetScriptableFromObject(Sender, parameters->objects[1]);
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
		core->GetTokenDictionary()->SetAt(tokenname, tm->QueryField(i, j));
	}
}

//this is a gemrb extension for scriptable tracks
void GameScript::SetTrackString(Scriptable* Sender, Action* parameters)
{
	Map *map = Sender->GetCurrentArea();
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
	IniSpawn *sp = Sender->GetCurrentArea()->INISpawn;
	if (!sp) {
		return;
	}
	sp->SetNamelessDeath(area, parameters->pointParameter, parameters->int1Parameter);
}

// like GameScript::Kill, but forces chunking damage (disabling resurrection)
void GameScript::ChunkCreature(Scriptable *Sender, Action* parameters)
{
	Scriptable* tar = GetScriptableFromObject(Sender, parameters->objects[1]);
	Actor* target = Scriptable::As<Actor>(tar);
	if (!target) {
		return;
	}

	Effect *fx = EffectQueue::CreateEffect(fx_death_ref, 0, 8, FX_DURATION_INSTANT_PERMANENT);
	target->fxqueue.AddEffect(fx, false);
}

void GameScript::MultiPlayerSync(Scriptable* Sender, Action* /*parameters*/)
{
	Sender->SetWait(1);
}

void GameScript::DestroyAllFragileEquipment(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetScriptableFromObject(Sender, parameters->objects[1]);
	Actor* actor = Scriptable::As<Actor>(tar);
	if (!actor) {
		return;
	}

	// TODO: ensure it's using the inventory/CREItem flags, not Item — IE_ITEM_ADAMANTINE won't work as an input otherwise
	actor->inventory.DestroyItem("", parameters->int0Parameter, ~0);
}

void GameScript::SetOriginalClass(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetScriptableFromObject(Sender, parameters->objects[1]);
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
	const Scriptable* tar = GetScriptableFromObject(Sender, parameters->objects[1]);
	const Actor* actor = Scriptable::As<const Actor>(tar);
	if (!actor || !actor->PCStats) {
		return;
	}

	// spell
	ResRef favourite = "/";
	ieWord maxUses = 0;
	for (int i = 0; i < MAX_FAVOURITES; i++) {
		const auto& spellRef = actor->PCStats->FavouriteSpells[i];
		if (spellRef.IsEmpty()) continue;

		ieWord uses = actor->PCStats->FavouriteSpellsCount[i];
		if (uses > maxUses) {
			maxUses = uses;
			favourite = spellRef;
		}
	}
	core->GetTokenDictionary()->SetAt("FAVOURITESPELL", favourite);
	core->GetTokenDictionary()->SetAt("FAVOURITESPELLNUM", maxUses);

	// weapon
	favourite = "/";
	maxUses = 0;
	for (int i = 0; i < MAX_FAVOURITES; i++) {
		const auto& spellRef = actor->PCStats->FavouriteWeapons[i];
		if (spellRef.IsEmpty()) continue;

		ieWord uses = actor->PCStats->FavouriteWeaponsCount[i];
		if (uses > maxUses) {
			maxUses = uses;
			favourite = spellRef;
		}
	}
	core->GetTokenDictionary()->SetAt("FAVOURITEWEAPON", favourite);
	core->GetTokenDictionary()->SetAt("FAVOURITEWEAPONNUM", maxUses);

	// kill stats
	core->GetTokenDictionary()->SetAt("KILLCOUNT", actor->PCStats->KillsTotalCount);
	core->GetTokenDictionary()->SetAt("KILLCOUNTCHAPTER", actor->PCStats->KillsChapterCount);
	core->GetTokenDictionary()->SetAt("BESTKILL", core->GetMBString(actor->PCStats->BestKilledName));
}

}
