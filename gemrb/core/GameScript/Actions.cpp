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

#include "win32def.h"

#include "AmbientMgr.h"
#include "Audio.h"
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
#include "Video.h"
#include "WorldMap.h"
#include "GUI/GameControl.h"
#include "GUI/EventMgr.h"
#include "Scriptable/Container.h"
#include "Scriptable/Door.h"
#include "Scriptable/InfoPoint.h"

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
		map->AreaType|=AT_CAN_REST;
	} else {
		map->AreaType&=~AT_CAN_REST;
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
	HandleBitMod( value, parameters->int0Parameter, parameters->int1Parameter);
	map->AreaFlags=value;
}

void GameScript::AddAreaType(Scriptable* Sender, Action* parameters)
{
	Map *map=Sender->GetCurrentArea();
	map->AreaType|=parameters->int0Parameter;
}

void GameScript::RemoveAreaType(Scriptable* Sender, Action* parameters)
{
	Map *map=Sender->GetCurrentArea();
	map->AreaType&=~parameters->int0Parameter;
}

void GameScript::NoActionAtAll(Scriptable* /*Sender*/, Action* /*parameters*/)
{
	//thats all :)
}

// this action stops modal actions, so...
void GameScript::NoAction(Scriptable* Sender, Action* /*parameters*/)
{
	if (Sender->Type!=ST_ACTOR) {
		return;
	}
	Actor *actor = (Actor *) Sender;
	actor->SetModal( MS_NONE);
}

void GameScript::SG(Scriptable* Sender, Action* parameters)
{
	SetVariable( Sender, parameters->string0Parameter, "GLOBAL", parameters->int0Parameter );
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
		parameters->int0Parameter*AI_UPDATE_TIME + mytime);
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
	SetVariable( Sender, parameters->string0Parameter, random*AI_UPDATE_TIME + mytime);
}

void GameScript::SetGlobalTimerOnce(Scriptable* Sender, Action* parameters)
{
	ieDword mytime = CheckVariable( Sender, parameters->string0Parameter );
	if (mytime != 0) {
		return;
	}
	mytime=core->GetGame()->GameTime; //gametime (should increase it)
	SetVariable( Sender, parameters->string0Parameter,
		parameters->int0Parameter*AI_UPDATE_TIME + mytime);
}

void GameScript::RealSetGlobalTimer(Scriptable* Sender, Action* parameters)
{
	ieDword mytime=core->GetGame()->RealTime;

	SetVariable( Sender, parameters->string0Parameter,
		parameters->int0Parameter*AI_UPDATE_TIME + mytime);
}

void GameScript::ChangeAllegiance(Scriptable* Sender, Action* parameters)
{
	Scriptable *scr = Sender;
	if (parameters->objects[1]) {
		scr=GetActorFromObject( Sender, parameters->objects[1] );
	}
	if (!scr || scr->Type != ST_ACTOR) {
		return;
	}
	Actor* actor = ( Actor* ) scr;
	actor->SetBase( IE_EA, parameters->int0Parameter );
}

void GameScript::ChangeGeneral(Scriptable* Sender, Action* parameters)
{
	Scriptable *scr = Sender;
	if (parameters->objects[1]) {
		scr=GetActorFromObject( Sender, parameters->objects[1] );
	}
	if (!scr || scr->Type != ST_ACTOR) {
		return;
	}
	Actor* actor = ( Actor* ) scr;
	actor->SetBase( IE_GENERAL, parameters->int0Parameter );
}

void GameScript::ChangeRace(Scriptable* Sender, Action* parameters)
{
	Scriptable *scr = Sender;
	if (parameters->objects[1]) {
		scr=GetActorFromObject( Sender, parameters->objects[1] );
	}
	if (!scr || scr->Type != ST_ACTOR) {
		return;
	}
	Actor* actor = ( Actor* ) scr;
	actor->SetBase( IE_RACE, parameters->int0Parameter );
}

void GameScript::ChangeClass(Scriptable* Sender, Action* parameters)
{
	Scriptable *scr = Sender;
	if (parameters->objects[1]) {
		scr=GetActorFromObject( Sender, parameters->objects[1] );
	}
	if (!scr || scr->Type != ST_ACTOR) {
		return;
	}
	Actor* actor = ( Actor* ) scr;
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
	SetVariable(Sender, "APPEARANCE", "GLOBAL", parameters->int0Parameter);
	core->SetEventFlag(EF_UPDATEANIM);
}

void GameScript::ChangeSpecifics(Scriptable* Sender, Action* parameters)
{
	Scriptable *scr = Sender;
	if (parameters->objects[1]) {
		scr=GetActorFromObject( Sender, parameters->objects[1] );
	}
	if (!scr || scr->Type != ST_ACTOR) {
		return;
	}
	Actor* actor = ( Actor* ) scr;
	actor->SetBase( IE_SPECIFIC, parameters->int0Parameter );
}

void GameScript::PermanentStatChange(Scriptable* Sender, Action* parameters)
{
	Scriptable *scr = Sender;
	if (parameters->objects[1]) {
		scr=GetActorFromObject( Sender, parameters->objects[1] );
	}
	if (!scr || scr->Type != ST_ACTOR) {
		return;
	}
	Actor* actor = ( Actor* ) scr;
	ieDword value;
	switch (parameters->int1Parameter) {
		case 1:
			value = actor->GetBase(parameters->int0Parameter);
			value-= parameters->int2Parameter;
			break;
		case 2:
			value = actor->GetBase(parameters->int0Parameter);
			value+= parameters->int2Parameter;
			break;
		case 3:
		default: //no idea what happens
			value = parameters->int2Parameter;
			break;
	}
	actor->SetBase( parameters->int0Parameter, value);
}

void GameScript::ChangeStat(Scriptable* Sender, Action* parameters)
{
	Scriptable *scr = Sender;
	if (parameters->objects[1]) {
		scr=GetActorFromObject( Sender, parameters->objects[1] );
	}
	if (!scr || scr->Type != ST_ACTOR) {
		return;
	}
	Actor* actor = ( Actor* ) scr;
	ieDword value = parameters->int1Parameter;
	if (parameters->int2Parameter==1) {
		value+=actor->GetBase(parameters->int0Parameter);
	}
	actor->SetBase( parameters->int0Parameter, value);
}

void GameScript::ChangeStatGlobal(Scriptable* Sender, Action* parameters)
{
	Scriptable *scr = Sender;
	if (parameters->objects[1]) {
		scr=GetActorFromObject( Sender, parameters->objects[1] );
	}
	if (!scr || scr->Type != ST_ACTOR) {
		return;
	}
	ieDword value = (ieDword) CheckVariable( Sender, parameters->string0Parameter, parameters->string1Parameter );
	Actor* actor = ( Actor* ) scr;
	if (parameters->int1Parameter==1) {
		value+=actor->GetBase(parameters->int0Parameter);
	}
	actor->SetBase( parameters->int0Parameter, value);
}

void GameScript::ChangeGender(Scriptable* Sender, Action* parameters)
{
	Scriptable *scr = Sender;
	if (parameters->objects[1]) {
		scr=GetActorFromObject( Sender, parameters->objects[1] );
	}
	if (!scr || scr->Type != ST_ACTOR) {
		return;
	}
	Actor* actor = ( Actor* ) scr;
	actor->SetBase( IE_SEX, parameters->int0Parameter );
}

void GameScript::ChangeAlignment(Scriptable* Sender, Action* parameters)
{
	Scriptable *scr = Sender;
	if (parameters->objects[1]) {
		scr=GetActorFromObject( Sender, parameters->objects[1] );
	}
	if (!scr || scr->Type != ST_ACTOR) {
		return;
	}
	Actor* actor = ( Actor* ) scr;
	actor->SetBase( IE_ALIGNMENT, parameters->int0Parameter );
}

void GameScript::SetFaction(Scriptable* Sender, Action* parameters)
{
	Scriptable *scr = Sender;
	if (parameters->objects[1]) {
		scr=GetActorFromObject( Sender, parameters->objects[1] );
	}
	if (!scr || scr->Type != ST_ACTOR) {
		return;
	}
	Actor* actor = ( Actor* ) scr;
	actor->SetBase( IE_FACTION, parameters->int0Parameter );
}

void GameScript::SetHP(Scriptable* Sender, Action* parameters)
{
	Scriptable *scr = Sender;
	if (parameters->objects[1]) {
		scr=GetActorFromObject( Sender, parameters->objects[1] );
	}
	if (!scr || scr->Type != ST_ACTOR) {
		return;
	}
	Actor* actor = ( Actor* ) scr;
	actor->SetBase( IE_HITPOINTS, parameters->int0Parameter );
}

void GameScript::SetHPPercent(Scriptable* Sender, Action* parameters)
{
	Scriptable *scr = Sender;
	if (parameters->objects[1]) {
		scr=GetActorFromObject( Sender, parameters->objects[1] );
	}
	if (!scr || scr->Type != ST_ACTOR) {
		return;
	}
	Actor* actor = ( Actor* ) scr;
	actor->NewBase( IE_HITPOINTS, parameters->int0Parameter, MOD_PERCENT);
}

void GameScript::AddHP(Scriptable* Sender, Action* parameters)
{
	Scriptable *scr = Sender;
	if (parameters->objects[1]) {
		scr=GetActorFromObject( Sender, parameters->objects[1] );
	}
	if (!scr || scr->Type != ST_ACTOR) {
		return;
	}
	Actor* actor = ( Actor* ) scr;
	actor->NewBase(IE_HITPOINTS, parameters->int0Parameter, MOD_ADDITIVE);
}

//this works on an object (pst)
//but can also work on actor itself (gemrb)
void GameScript::SetTeam(Scriptable* Sender, Action* parameters)
{
	Scriptable *scr = Sender;
	if (parameters->objects[1]) {
		scr=GetActorFromObject( Sender, parameters->objects[1] );
	}
	if (!scr || scr->Type != ST_ACTOR) {
		return;
	}
	Actor* actor = ( Actor* ) scr;
	actor->SetBase( IE_TEAM, parameters->int0Parameter );
}

//this works on an object (gemrb)
//or on Myself if object isn't given (iwd2)
void GameScript::SetTeamBit(Scriptable* Sender, Action* parameters)
{
	Scriptable *scr = Sender;
	if (parameters->objects[1]) {
		scr=GetActorFromObject( Sender, parameters->objects[1] );
	}
	if (!scr || scr->Type != ST_ACTOR) {
		return;
	}
	Actor* actor = ( Actor* ) scr;
	if (parameters->int1Parameter) {
		actor->SetBase( IE_TEAM, actor->GetStat(IE_TEAM) | parameters->int0Parameter );
	} else {
		actor->SetBase( IE_TEAM, actor->GetStat(IE_TEAM) & ~parameters->int0Parameter );
	}
}

void GameScript::TriggerActivation(Scriptable* Sender, Action* parameters)
{
	Scriptable* ip;

	if (!parameters->objects[1]) {
		ip=Sender;
	} else {
		ip = Sender->GetCurrentArea()->TMap->GetInfoPoint(parameters->objects[1]->objectName);
	}
	if (!ip || (ip->Type!=ST_TRIGGER && ip->Type!=ST_TRAVEL && ip->Type!=ST_PROXIMITY)) {
		printMessage("Actions", "Script error: No Trigger Named \"%s\"\n", YELLOW, parameters->objects[1]->objectName);
		return;
	}
	InfoPoint *trigger = (InfoPoint *) ip;
	if ( parameters->int0Parameter != 0 ) {
		trigger->Flags &= ~TRAP_DEACTIVATED;
	} else {
		trigger->Flags |= TRAP_DEACTIVATED;
	}
}

void GameScript::FadeToColor(Scriptable* Sender, Action* parameters)
{
	core->timer->SetFadeToColor( parameters->pointParameter.x );
//	Sender->SetWait( parameters->pointParameter.x );
	Sender->ReleaseCurrentAction(); // todo, blocking?
}

void GameScript::FadeFromColor(Scriptable* Sender, Action* parameters)
{
	core->timer->SetFadeFromColor( parameters->pointParameter.x );
//	Sender->SetWait( parameters->pointParameter.x );
	Sender->ReleaseCurrentAction(); // todo, blocking?
}

void GameScript::FadeToAndFromColor(Scriptable* Sender, Action* parameters)
{
	core->timer->SetFadeToColor( parameters->pointParameter.x );
	core->timer->SetFadeFromColor( parameters->pointParameter.x );
//	Sender->SetWait( parameters->pointParameter.x<<1 ); //multiply by 2
	Sender->ReleaseCurrentAction(); // todo, blocking?
}

void GameScript::JumpToPoint(Scriptable* Sender, Action* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		return;
	}
	Actor* ab = ( Actor* ) Sender;
	ab->SetPosition( parameters->pointParameter, true );
}

void GameScript::JumpToPointInstant(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1] );
	if (!tar || tar->Type != ST_ACTOR) {
		return;
	}
	Actor* ab = ( Actor* ) tar;
	ab->SetPosition( parameters->pointParameter, true );
}

/** instant jump to location saved in stats */
/** default subject is the current actor */
void GameScript::JumpToSavedLocation(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1] );
	if (!tar) {
		tar = Sender;
	}
	if (tar->Type != ST_ACTOR) {
		return;
	}
	Actor *actor = (Actor *) tar;
	Point p((short) actor->GetStat(IE_SAVEDXPOS), (short) actor->GetStat(IE_SAVEDYPOS) );
	actor->SetPosition(p, true );
	actor->SetOrientation( actor->GetStat(IE_SAVEDFACE), false );
}

void GameScript::JumpToObject(Scriptable* Sender, Action* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		return;
	}
	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1] );

	if (!tar) {
		return;
	}
	const Map *map = tar->GetCurrentArea();

	if (map) {
		if (parameters->string0Parameter[0]) {
			CreateVisualEffectCore(Sender, Sender->Pos, parameters->string0Parameter, 0);
		}
		MoveBetweenAreasCore( (Actor *) Sender, map->GetScriptName(), tar->Pos, -1, true);
	}
}

void GameScript::TeleportParty(Scriptable* /*Sender*/, Action* parameters)
{
	Game *game = core->GetGame();
	int i = game->GetPartySize(false);
	while (i--) {
		Actor *tar = game->GetPC(i, false);
		MoveBetweenAreasCore( tar, parameters->string0Parameter,
			parameters->pointParameter, -1, true);
	}
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
	Game *game = core->GetGame();
	for (int i = 0; i < game->GetPartySize(false); i++) {
		Actor* act = game->GetPC( i, false );
		if (act) {
			GAMLocationEntry *gle;
			if (game->GetPlaneLocationCount() <= (unsigned int)i) {
				// no location, meaning the actor joined the party after the save
				// reuse the last valid location
				gle = game->GetPlaneLocationEntry(game->GetPlaneLocationCount()-1);
			} else {
				gle = game->GetPlaneLocationEntry(i);
			}
			MoveBetweenAreasCore(act, gle->AreaResRef, gle->Pos, -1, true);
		}
	}

	// don't clear locations!
}

//moves pcs and npcs from an area to another area
void GameScript::MoveGlobalsTo(Scriptable* /*Sender*/, Action* parameters)
{
	Game *game = core->GetGame();
	int i = game->GetPartySize(false);
	while (i--) {
		Actor *tar = game->GetPC(i, false);
		//if the actor isn't in the area, we don't care
		if (strnicmp(tar->Area, parameters->string0Parameter,8) ) {
			continue;
		}
		MoveBetweenAreasCore( tar, parameters->string1Parameter,
			parameters->pointParameter, -1, true);
	}
	i = game->GetNPCCount();
	while (i--) {
		Actor *tar = game->GetNPC(i);
		//if the actor isn't in the area, we don't care
		if (strnicmp(tar->Area, parameters->string0Parameter,8) ) {
			continue;
		}
		MoveBetweenAreasCore( tar, parameters->string1Parameter,
			parameters->pointParameter, -1, true);
	}
}

void GameScript::MoveGlobal(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1] );
	if (!tar || tar->Type != ST_ACTOR) {
		return;
	}

	MoveBetweenAreasCore( (Actor *) tar, parameters->string0Parameter,
		parameters->pointParameter, -1, true);
}

//we also allow moving to door, container
void GameScript::MoveGlobalObject(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1] );
	if (!tar || tar->Type != ST_ACTOR) {
		return;
	}
	Scriptable* to = GetActorFromObject( Sender, parameters->objects[2] );
	if (!to) {
		return;
	}
	const Map *map = to->GetCurrentArea();

	if (map) {
		MoveBetweenAreasCore( (Actor *) tar, map->GetScriptName(),
			to->Pos, -1, true);
	}
}

void GameScript::MoveGlobalObjectOffScreen(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1] );
	if (!tar || tar->Type != ST_ACTOR) {
		return;
	}
	Scriptable* to = GetActorFromObject( Sender, parameters->objects[2] );
	if (!to) {
		return;
	}
	MoveBetweenAreasCore( (Actor *) tar, parameters->string0Parameter,
		to->Pos, -1, false);
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
	strcpy(parameters->string1Parameter, "SPDIMNDR");
	CreateCreatureCore( Sender, parameters, CC_CHECK_IMPASSABLE|CC_CHECK_OVERLAP | CC_PLAY_ANIM );
}

//another highly redundant action
void GameScript::CreateCreatureObjectDoor(Scriptable* Sender, Action* parameters)
{
	//we hack this to death
	strcpy(parameters->string1Parameter, "SPDIMNDR");
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
	if (active) {
		game->ControlStatus |= CS_HIDEGUI;
	} else {
		game->ControlStatus &= ~CS_HIDEGUI;
	}
	core->SetEventFlag(EF_CONTROL);
	core->GetVideoDriver()->SetMouseEnabled(!active);
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
	GameScript* gs = new GameScript( parameters->string0Parameter, Sender );
	gs->EvaluateAllBlocks();
	delete( gs );
}

void GameScript::CutSceneID(Scriptable* /*Sender*/, Action* /*parameters*/)
{
	// shouldn't get called
	printMessage("GameScript","CutSceneID was called!\n",YELLOW);
}

static EffectRef fx_charm_ref = { "State:Charmed", -1 };

void GameScript::Enemy(Scriptable* Sender, Action* /*parameters*/)
{
	if (Sender->Type != ST_ACTOR) {
		return;
	}
	Actor* actor = ( Actor* ) Sender;

	actor->fxqueue.RemoveAllEffects(fx_charm_ref);
	actor->SetBase( IE_EA, EA_ENEMY );
}

void GameScript::Ally(Scriptable* Sender, Action* /*parameters*/)
{
	if (Sender->Type != ST_ACTOR) {
		return;
	}
	Actor* actor = ( Actor* ) Sender;

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
	Sender->SetScript( parameters->string0Parameter, parameters->int0Parameter, false );
}

void GameScript::ForceAIScript(Scriptable* Sender, Action* parameters)
{
	if (parameters->int0Parameter>=MAX_SCRIPTS) {
		return;
	}
	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1] );
	if (!tar || tar->Type != ST_ACTOR) {
		return;
	}
	Actor* actor = ( Actor* ) tar;
	//clearing the queue, and checking script level was intentionally removed
	actor->SetScript( parameters->string0Parameter, parameters->int0Parameter, false );
}

void GameScript::SetPlayerSound(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1] );
	if (!tar || tar->Type != ST_ACTOR) {
		return;
	}
	if (((ieDword) parameters->int0Parameter)>100) {
		printMessage("GameScript","Invalid index %d in SetPlayerSound.\n", YELLOW, parameters->int0Parameter);
		return;
	}
	Actor* actor = ( Actor* ) tar;
	actor->StrRefs[parameters->int0Parameter]=parameters->int1Parameter;
}

//this one works only on real actors, they got constants
void GameScript::VerbalConstantHead(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1] );
	if (!tar || tar->Type != ST_ACTOR) {
		return;
	}
	DisplayStringCore( tar, parameters->int0Parameter, DS_HEAD|DS_CONSOLE|DS_CONST);
}

void GameScript::VerbalConstant(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1] );
	if (!tar || tar->Type != ST_ACTOR) {
		return;
	}
	DisplayStringCore( tar, parameters->int0Parameter, DS_CONSOLE|DS_CONST);
}

//bg2 - variable
void GameScript::SaveLocation(Scriptable* Sender, Action* parameters)
{
	ieDword value = parameters->pointParameter.asDword();
	if (!parameters->string0Parameter[0]) {
		strcpy(parameters->string0Parameter,"LOCALSsavedlocation");
	}
	SetVariable(Sender, parameters->string0Parameter, value);
}

//PST:has parameters, IWD2: no params
void GameScript::SetSavedLocation(Scriptable* Sender, Action* parameters)
{
	if (Sender->Type!=ST_ACTOR) {
		return;
	}
	Actor *actor = (Actor *) Sender;
	//iwd2
	if (parameters->pointParameter.isnull()) {
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
	if (Sender->Type!=ST_ACTOR) {
		return;
	}
	Actor *actor = (Actor *) Sender;
	actor->SetBase(IE_SAVEDXPOS, parameters->int0Parameter);
	actor->SetBase(IE_SAVEDYPOS, parameters->int1Parameter);
	actor->SetBase(IE_SAVEDFACE, parameters->int2Parameter);
}
//IWD2, sets the homepoint P
void GameScript::SetStartPos(Scriptable* Sender, Action* parameters)
{
	if (Sender->Type!=ST_ACTOR) {
		return;
	}
	Actor *actor = (Actor *) Sender;
	actor->SetBase(IE_SAVEDXPOS, parameters->pointParameter.x);
	actor->SetBase(IE_SAVEDYPOS, parameters->pointParameter.y);
	actor->SetBase(IE_SAVEDFACE, parameters->int0Parameter);
}

void GameScript::SaveObjectLocation(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1] );
	if (!tar) {
		return;
	}
	ieDword value = tar->Pos.asDword();
	if (!parameters->string0Parameter[0]) {
		strcpy(parameters->string0Parameter,"LOCALSsavedlocation");
	}
	SetVariable(Sender, parameters->string0Parameter, value);
}

/** you may omit the string0Parameter, in this case this will be a */
/** CreateCreatureAtSavedLocation */
void GameScript::CreateCreatureAtLocation(Scriptable* Sender, Action* parameters)
{
	if (!parameters->string0Parameter[0]) {
		strcpy(parameters->string0Parameter,"LOCALSsavedlocation");
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
			width = rand() % width + parameters->int0Parameter;
		}
		Sender->CurrentActionState = width * AI_UPDATE_TIME;
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
		Sender->CurrentActionState = parameters->int0Parameter * AI_UPDATE_TIME;
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
		Sender->CurrentActionState = rand() % random + parameters->int0Parameter;
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
	core->timer->SetMoveViewPort( parameters->pointParameter.x, parameters->pointParameter.y, parameters->int0Parameter<<1, true );
	Sender->SetWait(1); // todo, blocking?
	Sender->ReleaseCurrentAction(); // todo, blocking?
}

void GameScript::MoveViewObject(Scriptable* Sender, Action* parameters)
{
	Scriptable * scr = GetActorFromObject( Sender, parameters->objects[1]);
	if (!scr) {
		Sender->ReleaseCurrentAction();
		return;
	}
	core->timer->SetMoveViewPort( scr->Pos.x, scr->Pos.y, parameters->int0Parameter<<1, true );
	Sender->SetWait(1); // todo, blocking?
	Sender->ReleaseCurrentAction(); // todo, blocking?
}

void GameScript::AddWayPoint(Scriptable* Sender, Action* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		Sender->ReleaseCurrentAction();
		return;
	}
	Actor* actor = ( Actor* ) Sender;
	actor->AddWayPoint( parameters->pointParameter );
	// this is marked as AF_BLOCKING (and indeed AddWayPoint causes moves),
	// but this probably needs more thought
	Sender->ReleaseCurrentAction();
}

void GameScript::MoveToPointNoRecticle(Scriptable* Sender, Action* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		Sender->ReleaseCurrentAction();
		return;
	}
	Actor *actor = ( Actor* ) Sender;
	if (!actor->InMove() || actor->Destination != parameters->pointParameter) {
		actor->WalkTo( parameters->pointParameter, IF_NORETICLE, 0 );
	}
	if (!actor->InMove()) {
		// we should probably instead keep retrying until we reach dest
		Sender->ReleaseCurrentAction();
	}
}

void GameScript::MoveToPointNoInterrupt(Scriptable* Sender, Action* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		Sender->ReleaseCurrentAction();
		return;
	}
	Actor* actor = ( Actor* ) Sender;
	if (!actor->InMove() || actor->Destination != parameters->pointParameter) {
		actor->WalkTo( parameters->pointParameter, IF_NOINT, 0 );
	}
	// should we always force IF_NOINT here?
	if (!actor->InMove()) {
		// we should probably instead keep retrying until we reach dest
		actor->Interrupt();
		Sender->ReleaseCurrentAction();
	}
}

void GameScript::RunToPointNoRecticle(Scriptable* Sender, Action* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		Sender->ReleaseCurrentAction();
		return;
	}
	Actor* actor = ( Actor* ) Sender;
	if (!actor->InMove() || actor->Destination != parameters->pointParameter) {
		actor->WalkTo( parameters->pointParameter, IF_NORETICLE|IF_RUNNING, 0 );
	}
	if (!actor->InMove()) {
		// we should probably instead keep retrying until we reach dest
		Sender->ReleaseCurrentAction();
	}
}

void GameScript::RunToPoint(Scriptable* Sender, Action* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		Sender->ReleaseCurrentAction();
		return;
	}
	Actor* actor = ( Actor* ) Sender;
	if (!actor->InMove() || actor->Destination != parameters->pointParameter) {
		actor->WalkTo( parameters->pointParameter, IF_RUNNING, 0 );
	}
	if (!actor->InMove()) {
		// we should probably instead keep retrying until we reach dest
		Sender->ReleaseCurrentAction();
	}
}

//movetopoint until timer is down or target reached
void GameScript::TimedMoveToPoint(Scriptable* Sender, Action* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		Sender->ReleaseCurrentAction();
		return;
	}
	if (parameters->int0Parameter<=0) {
		Sender->ReleaseCurrentAction();
		return;
	}
	Actor *actor = (Actor *) Sender;

	if (!actor->InMove() || actor->Destination != parameters->pointParameter) {
		actor->WalkTo( parameters->pointParameter, parameters->int1Parameter,0 );
	}

	//hopefully this hack will prevent lockups
	if (!actor->InMove()) {
		// we should probably instead keep retrying until we reach dest
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
	if (Sender->Type != ST_ACTOR) {
		Sender->ReleaseCurrentAction();
		return;
	}
	Actor* actor = ( Actor* ) Sender;

	// try the actual move, if we are not already moving there
	if (!actor->InMove() || actor->Destination != parameters->pointParameter) {
		actor->WalkTo( parameters->pointParameter, 0 );
	}

	// give up if we can't move there (no path was found)
	if (!actor->InMove()) {
		// we should probably instead keep retrying until we reach dest
		Sender->ReleaseCurrentAction();
	}
}

//bg2, jumps to saved location in variable
void GameScript::MoveToSavedLocation(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1] );
	if (!tar) {
		tar = Sender;
	}
	if (tar->Type != ST_ACTOR) {
		Sender->ReleaseCurrentAction();
		return;
	}

	Point p;
	Actor* actor = ( Actor* ) tar;
	ieDword value = (ieDword) CheckVariable( Sender, parameters->string0Parameter );
	p.fromDword(value);
	actor->SetPosition(p, true );
	Sender->ReleaseCurrentAction();
}
/** iwd2 returntosavedlocation (with stats) */
/** pst returntosavedplace */
/** use Sender as default subject */
void GameScript::ReturnToSavedLocation(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1], GA_NO_DEAD );
	if (!tar) {
		tar = Sender;
	}
	if (tar->Type != ST_ACTOR) {
		Sender->ReleaseCurrentAction();
		return;
	}

	Actor* actor = ( Actor* ) tar;
	Point p((short) actor->GetBase(IE_SAVEDXPOS),(short) actor->GetBase(IE_SAVEDYPOS) );
	if (p.isnull()) {
		Sender->ReleaseCurrentAction();
		return;
	}
	if (!actor->InMove() || actor->Destination != p) {
		actor->WalkTo( p, 0, 0 );
	}
	if (!actor->InMove()) {
		// we should probably instead keep retrying until we reach dest
		Sender->ReleaseCurrentAction();
	}
}

//PST
void GameScript::RunToSavedLocation(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1], GA_NO_DEAD );
	if (!tar) {
		tar = Sender;
	}
	if (tar->Type != ST_ACTOR) {
		Sender->ReleaseCurrentAction();
		return;
	}

	Actor* actor = ( Actor* ) tar;
	Point p((short) actor->GetBase(IE_SAVEDXPOS),(short) actor->GetBase(IE_SAVEDYPOS) );
	if (p.isnull()) {
		Sender->ReleaseCurrentAction();
		return;
	}
	if (!actor->InMove() || actor->Destination != p) {
		actor->WalkTo( p, IF_RUNNING, 0 );
	}
	if (!actor->InMove()) {
		// we should probably instead keep retrying until we reach dest
		Sender->ReleaseCurrentAction();
	}
}

//iwd2
void GameScript::ReturnToSavedLocationDelete(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1], GA_NO_DEAD );
	if (!tar) {
		tar = Sender;
	}
	if (tar->Type != ST_ACTOR) {
		Sender->ReleaseCurrentAction();
		return;
	}

	Actor* actor = ( Actor* ) tar;
	Point p((short) actor->GetBase(IE_SAVEDXPOS),(short) actor->GetBase(IE_SAVEDYPOS) );
	actor->SetBase(IE_SAVEDXPOS,0);
	actor->SetBase(IE_SAVEDYPOS,0);
	if (p.isnull()) {
		Sender->ReleaseCurrentAction();
		return;
	}
	if (!actor->InMove() || actor->Destination != p) {
		actor->WalkTo( p, 0, 0 );
	}
	//what else?
	if (!actor->InMove()) {
		// we should probably instead keep retrying until we reach dest
		Sender->ReleaseCurrentAction();
	}
}

void GameScript::TriggerWalkTo(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1], 0 );
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
	if (Sender->Type != ST_ACTOR) {
		Sender->ReleaseCurrentAction();
		return;
	}
	Scriptable* target = GetStoredActorFromObject( Sender, parameters->objects[1] );
	if (!target) {
		Sender->ReleaseCurrentAction();
		return;
	}
	Actor* actor = ( Actor* ) Sender;
	//follow leader from a distance of 5
	//could also follow the leader with a point offset
	if (target->Type==ST_ACTOR) {
		actor->SetLeader( (Actor *) target, 5);
	}
	MoveNearerTo(Sender, target, MAX_OPERATING_DISTANCE);
}

void GameScript::StorePartyLocation(Scriptable* /*Sender*/, Action* /*parameters*/)
{
	Game *game = core->GetGame();
	for (int i = 0; i < game->GetPartySize(false); i++) {
		Actor* act = game->GetPC( i, false );
		GAMLocationEntry *gle = game->GetSavedLocationEntry(i);
		if (act && gle) {
			gle->Pos = act->Pos;
			memcpy(gle->AreaResRef, act->Area, 9);
		}
	}
}

void GameScript::RestorePartyLocation(Scriptable* /*Sender*/, Action* /*parameters*/)
{
	Game *game = core->GetGame();
	for (int i = 0; i < game->GetPartySize(false); i++) {
		Actor* act = game->GetPC( i, false );
		if (act) {
			GAMLocationEntry *gle;
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
	if (Sender->Type != ST_ACTOR) {
		Sender->ReleaseCurrentAction();
		return;
	}
	Region vp = core->GetVideoDriver()->GetViewport();
	Actor* actor = ( Actor* ) Sender;
	Point p((short) (vp.x+vp.w/2), (short) (vp.y+vp.h/2) );
	if (!actor->InMove() || actor->Destination != p) {
		actor->WalkTo( p, IF_NOINT, 0 );
	}
	if (!actor->InMove()) {
		// we should probably instead keep retrying until we reach dest
		Sender->ReleaseCurrentAction();
	}
}

void GameScript::MoveToOffset(Scriptable* Sender, Action* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		Sender->ReleaseCurrentAction();
		return;
	}
	Actor* actor = ( Actor* ) Sender;
	Point p(Sender->Pos.x+parameters->pointParameter.x, Sender->Pos.y+parameters->pointParameter.y);
	if (!actor->InMove() || actor->Destination != p) {
		actor->WalkTo( p, 0, 0 );
	}
	if (!actor->InMove()) {
		// we should probably instead keep retrying until we reach dest
		Sender->ReleaseCurrentAction();
	}
}

void GameScript::RunAwayFrom(Scriptable* Sender, Action* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		Sender->ReleaseCurrentAction();
		return;
	}
	if (Sender->GetInternalFlag()&IF_STOPATTACK) {
		Sender->ReleaseCurrentAction();
		return;
	}
	Actor* actor = ( Actor* ) Sender;
	Scriptable* tar = GetStoredActorFromObject( Sender, parameters->objects[1] );
	if (!tar) {
		Sender->ReleaseCurrentAction();
		return;
	}
	//TODO: actor could use travel areas
	// we should be using int0Parameter for the timing here, not distance
	if (!actor->InMove()) {
		// we should make sure our existing walk is a 'run away', or fix moving/path code
		actor->RunAwayFrom( tar->Pos, parameters->int0Parameter, false);
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

void GameScript::RunAwayFromNoLeaveArea(Scriptable* Sender, Action* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		Sender->ReleaseCurrentAction();
		return;
	}
	if (Sender->GetInternalFlag()&IF_STOPATTACK) {
		Sender->ReleaseCurrentAction();
		return;
	}
	Actor* actor = ( Actor* ) Sender;
	Scriptable* tar = GetStoredActorFromObject( Sender, parameters->objects[1] );
	if (!tar) {
		Sender->ReleaseCurrentAction();
		return;
	}
	// we should be using int0Parameter for the timing here, not distance
	if (!actor->InMove()) {
		// we should make sure our existing walk is a 'run away', or fix moving/path code
		actor->RunAwayFrom( tar->Pos, parameters->int0Parameter, false);
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

void GameScript::RunAwayFromNoInterrupt(Scriptable* Sender, Action* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		Sender->ReleaseCurrentAction();
		return;
	}
	//i believe being dead still interrupts this action
	if (Sender->GetInternalFlag()&IF_STOPATTACK) {
		Sender->ReleaseCurrentAction();
		return;
	}
	Actor* actor = ( Actor* ) Sender;
	Scriptable* tar = GetStoredActorFromObject( Sender, parameters->objects[1] );
	if (!tar) {
		Sender->ReleaseCurrentAction();
		return;
	}
	//actor->InternalFlags|=IF_NOINT;
	actor->NoInterrupt();
	// we should be using int0Parameter for the timing here, not distance
	if (!actor->InMove()) {
		// we should make sure our existing walk is a 'run away', or fix moving/path code
		actor->RunAwayFrom( tar->Pos, parameters->int0Parameter, false);
	}

	//repeat movement...
	if (parameters->int0Parameter>0) {
		Action *newaction = ParamCopyNoOverride(parameters);
		newaction->int0Parameter--;
		actor->AddActionInFront(newaction);
		Sender->SetWait(1);
	} else {
		actor->Interrupt();
	}

	Sender->ReleaseCurrentAction();
}

void GameScript::RunAwayFromPoint(Scriptable* Sender, Action* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		Sender->ReleaseCurrentAction();
		return;
	}
	if (Sender->GetInternalFlag()&IF_STOPATTACK) {
		Sender->ReleaseCurrentAction();
		return;
	}
	Actor* actor = ( Actor* ) Sender;
	// we should be using int0Parameter for the timing here, not distance?
	if (!actor->InMove()) {
		// we should make sure our existing walk is a 'run away', or fix moving/path code
		actor->RunAwayFrom( parameters->pointParameter, parameters->int0Parameter, false);
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

void GameScript::DisplayStringNoName(Scriptable* Sender, Action* parameters)
{
	Scriptable* target = GetActorFromObject( Sender, parameters->objects[1]);
	if (!target) {
		target=Sender;
	}
	if (Sender->Type==ST_ACTOR) {
		DisplayStringCore( target, parameters->int0Parameter, DS_CONSOLE|DS_NONAME);
	} else {
		DisplayStringCore( target, parameters->int0Parameter, DS_AREA|DS_NONAME);
	}
}

void GameScript::DisplayStringNoNameHead(Scriptable* Sender, Action* parameters)
{
	Scriptable* target = GetActorFromObject( Sender, parameters->objects[1] );
	if (!target) {
		target=Sender;
	}

	DisplayStringCore( target, parameters->int0Parameter, DS_HEAD|DS_CONSOLE|DS_NONAME);
}

//display message over current script owner
void GameScript::DisplayMessage(Scriptable* Sender, Action* parameters)
{
	DisplayStringCore(Sender, parameters->int0Parameter, DS_CONSOLE );
}

//float message over target
void GameScript::DisplayStringHead(Scriptable* Sender, Action* parameters)
{
	Scriptable* target = GetActorFromObject( Sender, parameters->objects[1] );
	if (!target) {
		target=Sender;
		printMessage("Actions", "DisplayStringHead/FloatMessage got no target, assuming Sender!\n", YELLOW);
	}

	DisplayStringCore(target, parameters->int0Parameter, DS_CONSOLE|DS_HEAD|DS_SPEECH );
}

void GameScript::KillFloatMessage(Scriptable* Sender, Action* parameters)
{
	Scriptable* target = GetActorFromObject( Sender, parameters->objects[1] );
	if (!target) {
		target=Sender;
	}
	target->DisplayHeadText(NULL);
}

void GameScript::DisplayStringHeadOwner(Scriptable* /*Sender*/, Action* parameters)
{
	Game *game=core->GetGame();

	int i = game->GetPartySize(true);
	while(i--) {
		Actor *actor = game->GetPC(i, true);
		if (actor->inventory.HasItem(parameters->string0Parameter,parameters->int0Parameter) ) {
			DisplayStringCore(actor, parameters->int0Parameter, DS_CONSOLE|DS_HEAD );
		}
	}
}

void GameScript::FloatMessageFixed(Scriptable* Sender, Action* parameters)
{
	Scriptable* target = GetActorFromObject( Sender, parameters->objects[1] );
	if (!target) {
		target=Sender;
		printMessage("GameScript", "DisplayStringHead/FloatMessage got no target, assuming Sender!\n", LIGHT_RED);
	}

	DisplayStringCore(target, parameters->int0Parameter, DS_CONSOLE|DS_HEAD);
}

void GameScript::FloatMessageFixedRnd(Scriptable* Sender, Action* parameters)
{
	Scriptable* target = GetActorFromObject( Sender, parameters->objects[1] );
	if (!target) {
		target=Sender;
		printMessage("GameScript", "DisplayStringHead/FloatMessage got no target, assuming Sender!\n", LIGHT_RED);
	}

	SrcVector *rndstr=LoadSrc(parameters->string0Parameter);
	if (!rndstr) {
		printMessage("GameScript", "Cannot display resource!\n", LIGHT_RED);
		return;
	}
	DisplayStringCore(target, rndstr->at(rand()%rndstr->size()), DS_CONSOLE|DS_HEAD);
	FreeSrc(rndstr, parameters->string0Parameter);
}

void GameScript::FloatMessageRnd(Scriptable* Sender, Action* parameters)
{
	Scriptable* target = GetActorFromObject( Sender, parameters->objects[1] );
	if (!target) {
		target=Sender;
		printMessage("GameScript", "DisplayStringHead/FloatMessage got no target, assuming Sender!\n", LIGHT_RED);
	}

	SrcVector *rndstr=LoadSrc(parameters->string0Parameter);
	if (!rndstr) {
		printMessage("GameScript", "Cannot display resource!\n", LIGHT_RED);
		return;
	}
	DisplayStringCore(target, rndstr->at(rand()%rndstr->size()), DS_CONSOLE|DS_HEAD);
	FreeSrc(rndstr, parameters->string0Parameter);
}

//apparently this should not display over head (for actors)
void GameScript::DisplayString(Scriptable* Sender, Action* parameters)
{
	Scriptable* target = GetActorFromObject( Sender, parameters->objects[1]);
	if (!target) {
		target=Sender;
	}
	if (Sender->Type==ST_ACTOR) {
		DisplayStringCore( target, parameters->int0Parameter, DS_CONSOLE);
	} else {
		DisplayStringCore( target, parameters->int0Parameter, DS_AREA);
	}
}

//DisplayStringHead, but wait until done
void GameScript::DisplayStringWait(Scriptable* Sender, Action* parameters)
{
	if (Sender->CurrentActionState) {
		// TODO: should probably store the actual time and wait for that,
		// rather than this hack
		if (!core->GetAudioDrv()->IsSpeaking()) {
			Sender->ReleaseCurrentAction();
		}
		return;
	}
	Scriptable* target = GetActorFromObject( Sender, parameters->objects[1]);
	if (!target) {
		target=Sender;
	}
	DisplayStringCore( target, parameters->int0Parameter, DS_CONSOLE|DS_WAIT|DS_SPEECH|DS_HEAD);
	Sender->CurrentActionState = 1;
}

void GameScript::ForceFacing(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1] );
	if (!tar || tar->Type!=ST_ACTOR) {
		Sender->ReleaseCurrentAction();
		return;
	}
	Actor *actor = (Actor *) tar;
	actor->SetOrientation(parameters->int0Parameter, false);
}

/* A -1 means random facing? */
void GameScript::Face(Scriptable* Sender, Action* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		Sender->ReleaseCurrentAction();
		return;
	}
	Actor* actor = ( Actor* ) Sender;
	if (parameters->int0Parameter==-1) {
		actor->SetOrientation(core->Roll(1,MAX_ORIENT,-1), false);
	} else {
		actor->SetOrientation(parameters->int0Parameter, false);
	}
	actor->SetWait( 1 );
	Sender->ReleaseCurrentAction(); // todo, blocking?
}

void GameScript::FaceObject(Scriptable* Sender, Action* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		Sender->ReleaseCurrentAction();
		return;
	}
	Scriptable* target = GetActorFromObject( Sender, parameters->objects[1] );
	if (!target) {
		Sender->ReleaseCurrentAction();
		return;
	}
	Actor* actor = ( Actor* ) Sender;
	actor->SetOrientation( GetOrient( target->Pos, actor->Pos ), false);
	actor->SetWait( 1 );
	Sender->ReleaseCurrentAction(); // todo, blocking?
}

void GameScript::FaceSavedLocation(Scriptable* Sender, Action* parameters)
{
	Scriptable* target = GetActorFromObject( Sender, parameters->objects[1] );
	if (!target || target->Type!=ST_ACTOR) {
		Sender->ReleaseCurrentAction();
		return;
	}
	Actor* actor = ( Actor* ) target;
	ieDword value;
	if (!parameters->string0Parameter[0]) {
		strcpy(parameters->string0Parameter,"LOCALSsavedlocation");
	}
	value = (ieDword) CheckVariable( target, parameters->string0Parameter );
	Point p;
	p.fromDword(value);

	actor->SetOrientation ( GetOrient( p, actor->Pos ), false);
	actor->SetWait( 1 );
	Sender->ReleaseCurrentAction(); // todo, blocking?
}

//pst and bg2 can play a song designated by index
//actually pst has some extra params not currently implemented
//switchplaylist implements fade by simply scheduling the next
//music after the currently running one
//FIXME: This code is similar to PlayAreaSong, consider refactoring
void GameScript::StartSong(Scriptable* /*Sender*/, Action* parameters)
{
	//the force play logic should be handled by SwitchPlayList
	bool force;
	char* poi = core->GetMusicPlaylist( parameters->int0Parameter );
	if (!poi) return;

	//if parameter is force, force the music, otherwise just schedule it for next
	if (parameters->int1Parameter==1) {
		force = true;
	} else {
		force = false;
	}
	int ret = core->GetMusicMgr()->SwitchPlayList( poi, force );
	if (ret) {
		*poi = '*';
	}
	if (parameters->int0Parameter == SONG_BATTLE) {
		core->GetGame()->CombatCounter = 150;
	}
}

//starts the current area music (songtype is in int0Parameter)
//PlayAreaSong will set the CombatCounter to 150 if
//it is battlemusic (the Counter will tick back to 0)
void GameScript::StartMusic(Scriptable* Sender, Action* parameters)
{
	//don't break on bad values
	if (parameters->int0Parameter>10) return;
	Map *map = Sender->GetCurrentArea();
	if (!map) return;
	bool force, restart;

	switch (parameters->int1Parameter) {
	case 1: //force switch
		force = true;
		restart = true;
		break;
	case 3: //force switch, but wait for previous music to end gracefully
		force = false;
		restart = true;
		break;
	default:
		force = false;
		restart = false;
		break;
	}
	map->PlayAreaSong(parameters->int0Parameter, restart, force);
}

void GameScript::StartCombatCounter(Scriptable* Sender, Action* /*parameters*/)
{
	Map *map = Sender->GetCurrentArea();
	if (!map) return;
	map->PlayAreaSong(3, 1, 1);
}

/*iwd2 can set an areasong slot*/
void GameScript::SetMusic(Scriptable* Sender, Action* parameters)
{
	//iwd2 allows setting all 10 slots, though, there is no evidence they are used
	if (parameters->int0Parameter>10) return;
	Map *map = Sender->GetCurrentArea();
	if (!map) return;
	map->SongHeader.SongList[parameters->int0Parameter]=parameters->int1Parameter;
}

//optional integer parameter (isSpeech)
void GameScript::PlaySound(Scriptable* Sender, Action* parameters)
{
	printMessage("Actions", "PlaySound(%s)\n", LIGHT_WHITE, parameters->string0Parameter);
	core->GetAudioDrv()->Play( parameters->string0Parameter, Sender->Pos.x,
				Sender->Pos.y, parameters->int0Parameter ? GEM_SND_SPEECH : 0 );
}

void GameScript::PlaySoundPoint(Scriptable* /*Sender*/, Action* parameters)
{
	printMessage("Actions", "PlaySound(%s)\n", LIGHT_WHITE, parameters->string0Parameter );
	core->GetAudioDrv()->Play( parameters->string0Parameter, parameters->pointParameter.x, parameters->pointParameter.y );
}

void GameScript::PlaySoundNotRanged(Scriptable* /*Sender*/, Action* parameters)
{
	printMessage("Actions", "PlaySound(%s)\n", LIGHT_WHITE, parameters->string0Parameter );
	core->GetAudioDrv()->Play( parameters->string0Parameter, 0, 0);
}

void GameScript::Continue(Scriptable* /*Sender*/, Action* /*parameters*/)
{
}

// creates area vvc at position of object
void GameScript::CreateVisualEffectObject(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1] );
	if (!tar) {
		return;
	}
	CreateVisualEffectCore(tar, tar->Pos, parameters->string0Parameter, parameters->int0Parameter);
}

// creates sticky vvc on actor or normal animation on object
void GameScript::CreateVisualEffectObjectSticky(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1] );
	if (!tar) {
		return;
	}
	if (tar->Type==ST_ACTOR) {
		CreateVisualEffectCore((Actor *) tar, parameters->string0Parameter, parameters->int0Parameter);
	} else {
		CreateVisualEffectCore(tar, tar->Pos, parameters->string0Parameter, parameters->int0Parameter);
	}
}

// creates area effect at point
void GameScript::CreateVisualEffect(Scriptable* Sender, Action* parameters)
{
	CreateVisualEffectCore(Sender, parameters->pointParameter, parameters->string0Parameter, parameters->int0Parameter);
}

void GameScript::DestroySelf(Scriptable* Sender, Action* /*parameters*/)
{
	if (Sender->Type != ST_ACTOR) {
		return;
	}
	Sender->ClearActions();
	Actor* actor = ( Actor* ) Sender;
	actor->DestroySelf();
	//actor->InternalFlags |= IF_CLEANUP;
}

void GameScript::ScreenShake(Scriptable* Sender, Action* parameters)
{
	if (parameters->int1Parameter) { //IWD2 has a different profile
		core->timer->SetScreenShake( parameters->int1Parameter,
			parameters->int2Parameter, parameters->int0Parameter );
	} else {
		core->timer->SetScreenShake( parameters->pointParameter.x,
			parameters->pointParameter.y, parameters->int0Parameter );
	}
	Sender->SetWait( parameters->int0Parameter );
	Sender->ReleaseCurrentAction(); // todo, blocking?
}

void GameScript::UnhideGUI(Scriptable* /*Sender*/, Action* /*parameters*/)
{
	Game* game = core->GetGame();
	game->SetControlStatus(CS_HIDEGUI, BM_NAND);
}

void GameScript::HideGUI(Scriptable* /*Sender*/, Action* /*parameters*/)
{
	Game* game = core->GetGame();
	game->SetControlStatus(CS_HIDEGUI, BM_OR);
}

void GameScript::LockScroll(Scriptable* /*Sender*/, Action* /*parameters*/)
{
	GameControl* gc = core->GetGameControl();
	if (gc) {
		gc->SetScreenFlags(SF_LOCKSCROLL, BM_OR);
	}
}

void GameScript::UnlockScroll(Scriptable* /*Sender*/, Action* /*parameters*/)
{
	GameControl* gc = core->GetGameControl();
	if (gc) {
		gc->SetScreenFlags(SF_LOCKSCROLL, BM_NAND);
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
		ambientmgr->activate(parameters->objects[1]->objectName);
	} else {
		ambientmgr->deactivate(parameters->objects[1]->objectName);
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
	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1], GA_NO_DEAD );
	if (!tar) {
		return;
	}
	if (tar->Type != ST_DOOR) {
		return;
	}
	Door* door = ( Door* ) tar;
	int state = parameters->int0Parameter;
	if(door) {
		door->ToggleTiles(state); /* default is false for playsound */
	}
}

void GameScript::StaticStart(Scriptable* Sender, Action* parameters)
{
	AreaAnimation *anim = Sender->GetCurrentArea()->GetAnimation(parameters->objects[1]->objectName);
	if (!anim) {
		printMessage("Actions", "Script error: No Animation Named \"%s\"\n", YELLOW,
			parameters->objects[1]->objectName );
		return;
	}
	anim->Flags &=~A_ANI_PLAYONCE;
}

void GameScript::StaticStop(Scriptable* Sender, Action* parameters)
{
	AreaAnimation *anim = Sender->GetCurrentArea()->GetAnimation(parameters->objects[1]->objectName);
	if (!anim) {
		printMessage("Actions","Script error: No Animation Named \"%s\"\n", YELLOW,
			parameters->objects[1]->objectName );
		return;
	}
	anim->Flags |= A_ANI_PLAYONCE;
}

void GameScript::StaticPalette(Scriptable* Sender, Action* parameters)
{
	AreaAnimation *anim = Sender->GetCurrentArea()->GetAnimation(parameters->objects[1]->objectName);
	if (!anim) {
		printMessage("Actions", "Script error: No Animation Named \"%s\"\n", YELLOW,
			parameters->objects[1]->objectName );
		return;
	}
	anim->SetPalette( parameters->string0Parameter );
}

//this is a special case of PlaySequence (with wait time, not for area anims)
void GameScript::PlaySequenceTimed(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar;
	if (parameters->objects[1]) {
		tar = GetActorFromObject( Sender, parameters->objects[1] );
	} else {
		tar=Sender;
	}
	if (!tar || tar->Type != ST_ACTOR) {
		return;
	}
	Actor* actor = ( Actor* ) tar;
	actor->SetStance( parameters->int0Parameter );
	int delay = parameters->int1Parameter;
	actor->SetWait( delay );
}

//waitanimation: waiting while animation of target is of a certain type
void GameScript::WaitAnimation(Scriptable* Sender, Action* parameters)
{
	Scriptable *tar = GetActorFromObject( Sender, parameters->objects[1] );
	if (!tar) {
		tar=Sender;
	}
	if (tar->Type != ST_ACTOR) {
		return;
	}
	Actor* actor = ( Actor* ) tar;
	if (actor->GetStance()!=parameters->int0Parameter) {
		Sender->ReleaseCurrentAction();
		return;
	}
}

// PlaySequence without object parameter defaults to Sender
void GameScript::PlaySequence(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar;
	if (parameters->objects[1]) {
		tar = GetActorFromObject( Sender, parameters->objects[1] );
		if (!tar) {
			//could be an animation
			AreaAnimation* anim = Sender->GetCurrentArea( )->GetAnimation( parameters->objects[1]->objectName);
			if (anim) {
				//set animation's cycle to parameters->int0Parameter;
				anim->sequence=parameters->int0Parameter;
				anim->frame=0;
				//what else to be done???
				anim->InitAnimation();
			}
			return;
		}

	} else {
		tar = Sender;
	}
	if (tar->Type != ST_ACTOR) {
		return;
	}
	Actor* actor = ( Actor* ) tar;
	actor->SetStance( parameters->int0Parameter );
}

//same as PlaySequence, but the value comes from a variable
//ToDo: create a PlaySequenceCore in GSUtils
void GameScript::PlaySequenceGlobal(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar;
	ieDword value;

	value = (ieDword) CheckVariable( Sender, parameters->string0Parameter );

	if (parameters->objects[1]) {
		tar = GetActorFromObject( Sender, parameters->objects[1] );
		if (!tar) {
			//could be an animation
			AreaAnimation* anim = Sender->GetCurrentArea( )->GetAnimation( parameters->objects[1]->objectName);
			if (anim) {
				//set animation's cycle to value;
				anim->sequence=value;
				anim->frame=0;
				//what else to be done???
				anim->InitAnimation();
			}
			return;
		}

	} else {
		tar = Sender;
	}
	if (tar->Type != ST_ACTOR) {
		return;
	}
	Actor* actor = ( Actor* ) tar;
	actor->SetStance( value );
}

void GameScript::SetDialogue(Scriptable* Sender, Action* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		return;
	}
	Actor* target = ( Actor* ) Sender;
	target->SetDialog( parameters->string0Parameter );
}

void GameScript::ChangeDialogue(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1] );
	if (!tar) {
		return;
	}
	if (tar->Type != ST_ACTOR) {
		return;
	}
	Actor* target = ( Actor* ) tar;
	target->SetDialog( parameters->string0Parameter );
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
	if (Sender->Type != ST_ACTOR) {
		Sender->ReleaseCurrentAction();
		return;
	}
	Game *game=core->GetGame();
	if (!game->EveryoneStopped() ) {
		//wait for a while
		Sender->SetWait( 1 * AI_UPDATE_TIME );
		return;
	}
	Actor *actor = (Actor *) Sender;
	if (!game->EveryoneNearPoint(actor->GetCurrentArea(), actor->Pos, true) ) {
		//we abort the command, everyone should be here
		Sender->ReleaseCurrentAction();
		return;
	}
	//travel direction passed to guiscript
	int direction = Sender->GetCurrentArea()->WhichEdge(actor->Pos);
	printMessage("Actions", "Travel direction returned: %d\n", LIGHT_WHITE, direction);
	if (direction==-1) {
		Sender->ReleaseCurrentAction();
		return;
	}
	core->GetDictionary()->SetAt("Travel", (ieDword) direction);
	core->GetGUIScriptEngine()->RunFunction( "GUIMA", "OpenWorldMapWindow" );
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
	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1], GA_NO_DEAD );
	if (!tar) {
		return;
	}
	if (tar->Type != ST_DOOR) {
		return;
	}
	Door* door = ( Door* ) tar;
	if (door->Flags & DOOR_SECRET) {
		door->Flags |= DOOR_FOUND;
	}
}

//this is an immediate action without checking Sender
void GameScript::Lock(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1] );
	if (!tar) {
		return;
	}
	switch (tar->Type) {
		case ST_DOOR:
			((Door *)tar)->SetDoorLocked(true, true);
			break;
		case ST_CONTAINER:
			((Container *)tar)->SetContainerLocked(true);
			break;
		default:
			return;
	}
}

void GameScript::Unlock(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1] );
	if (!tar) {
		return;
	}
	switch (tar->Type) {
		case ST_DOOR:
			((Door *)tar)->SetDoorLocked(false, true);
			break;
		case ST_CONTAINER:
			((Container *)tar)->SetContainerLocked(false);
			break;
		default:
			return;
	}
}

void GameScript::SetDoorLocked(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1] );
	if (!tar) {
		return;
	}
	if (tar->Type != ST_DOOR) {
		return;
	}
	Door* door = ( Door* ) tar;
	door->SetDoorLocked( parameters->int0Parameter!=0, false);
}

void GameScript::SetDoorFlag(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1] );
	if (!tar) {
		return;
	}
	if (tar->Type != ST_DOOR) {
		return;
	}
	Door* door = ( Door* ) tar;
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

	if (parameters->int1Parameter) {
		door->Flags|=flag;
	} else {
		door->Flags&=~flag;
	}
}

void GameScript::RemoveTraps(Scriptable* Sender, Action* parameters)
{
	//only actors may try to pick a lock
	if (Sender->Type != ST_ACTOR) {
		Sender->ReleaseCurrentAction();
		return;
	}
	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1] );
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
		door = ( Door* ) tar;
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
		container = (Container *) tar;
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
		break;
	default:
		Sender->ReleaseCurrentAction();
		return;
	}
	Actor * actor = (Actor *) Sender;
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
	if (Sender->Type != ST_ACTOR) {
		Sender->ReleaseCurrentAction();
		return;
	}
	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1] );
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
		door = ( Door* ) tar;
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
		container = (Container *) tar;
		p = &container->Pos;
		otherp = p;
		distance = Distance(*p, Sender);
		flags = container->Flags&CONT_LOCKED;
		break;
	default:
		Sender->ReleaseCurrentAction();
		return;
	}
	Actor * actor = (Actor *) Sender;
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
	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1] );
	if (!tar) {
		return;
	}
	if (tar->Type != ST_DOOR) {
		return;
	}
	Door* door = ( Door* ) tar;
	// no idea if this is right, or whether OpenDoor/CloseDoor should allow opening
	// of all doors, or some doors, or whether it should still check for non-actors
	if (Sender->Type == ST_ACTOR) {
		Actor *actor = (Actor *)Sender;
		actor->SetModal(MS_NONE);
		if (!door->TryUnlock(actor)) {
			return;
		}
	}
	//if not an actor opens, it don't play sound
	door->SetDoorOpen( true, (Sender->Type == ST_ACTOR), 0 );
	Sender->ReleaseCurrentAction();
}

void GameScript::CloseDoor(Scriptable* Sender, Action* parameters) {
	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1] );
	if (!tar) {
		return;
	}
	if (tar->Type != ST_DOOR) {
		return;
	}
	Door* door = ( Door* ) tar;
	// see comments in OpenDoor above
	if (Sender->Type == ST_ACTOR) {
		Actor *actor = (Actor *)Sender;
		if (!door->TryUnlock(actor)) {
			return;
		}
	}
	//if not an actor closes, it don't play sound
	door->SetDoorOpen( false, (Sender->Type == ST_ACTOR), 0 );
	Sender->ReleaseCurrentAction();
}

void GameScript::ToggleDoor(Scriptable* Sender, Action* /*parameters*/)
{
	if (Sender->Type != ST_ACTOR) {
		Sender->ReleaseCurrentAction();
		return;
	}
	Actor *actor = (Actor *) Sender;
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
			displaymsg->DisplayConstantString(STR_DOORLOCKED, DMC_LIGHTGREY, door);
			//playsound unsuccessful opening of door
			if(door->IsOpen())
				core->PlaySound(DS_CLOSE_FAIL);
			else
				core->PlaySound(DS_OPEN_FAIL);
			Sender->ReleaseCurrentAction();
			actor->TargetDoor = 0;
			return; //don't open door
		}

		// should we be triggering the trap on close?
		door->TriggerTrap(0, actor->GetGlobalID());
		door->SetDoorOpen( !door->IsOpen(), true, actor->GetGlobalID() );
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
	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1] );
	if (!tar || tar->Type!=ST_CONTAINER) {
		return;
	}
	Container *cnt = (Container *) tar;
	if (parameters->int0Parameter) {
		cnt->Flags&=~CONT_DISABLED;
	} else {
		cnt->Flags|=CONT_DISABLED;
	}
}

void GameScript::MoveBetweenAreas(Scriptable* Sender, Action* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		return;
	}
	if (parameters->string1Parameter[0]) {
		CreateVisualEffectCore(Sender, Sender->Pos, parameters->string1Parameter, 0);
	}
	MoveBetweenAreasCore((Actor *) Sender, parameters->string0Parameter,
		parameters->pointParameter, parameters->int0Parameter, true);
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
	SpellCore(Sender, parameters, SC_NOINTERRUPT);
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
	SpellPointCore(Sender, parameters, SC_NOINTERRUPT);
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
	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1] );
	if (!tar) {
		//it could still be an area animation, PST allows deactivating them via Activate
		AmbientActivateCore(Sender, parameters, 1);
		return;
	}
	if (tar->Type == ST_ACTOR) {
		tar->Unhide();
		return;
	}

	//PST allows activating of containers
	if (tar->Type == ST_CONTAINER) {
		((Container *) tar)->Flags&=~CONT_DISABLED;
		return;
	}

	//and regions
	if (tar->Type == ST_PROXIMITY || tar->Type == ST_TRAVEL || tar->Type==ST_TRIGGER) {
		((InfoPoint *) tar)->Flags&=~TRAP_DEACTIVATED;
		return;
	}
}

void GameScript::Deactivate(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1] );
	if (!tar) {
		//it could still be an area animation, PST allows deactivating them via Deactivate
		AmbientActivateCore(Sender, parameters, 0);
		return;
	}
	if (tar->Type == ST_ACTOR) {
		tar->Hide();
		return;
	}
	//PST allows deactivating of containers
	if (tar->Type == ST_CONTAINER) {
		((Container *) tar)->Flags|=CONT_DISABLED;
		return;
	}

	//and regions
	if (tar->Type == ST_PROXIMITY || tar->Type == ST_TRAVEL || tar->Type==ST_TRIGGER) {
		((InfoPoint *) tar)->Flags|=TRAP_DEACTIVATED;
		return;
	}
}

void GameScript::MakeGlobal(Scriptable* Sender, Action* /*parameters*/)
{
	if (Sender->Type != ST_ACTOR) {
		return;
	}
	Actor* act = ( Actor* ) Sender;
	core->GetGame()->AddNPC( act );
}

void GameScript::UnMakeGlobal(Scriptable* Sender, Action* /*parameters*/)
{
	if (Sender->Type != ST_ACTOR) {
		return;
	}
	Actor* act = ( Actor* ) Sender;
	int slot;
	slot = core->GetGame()->InStore( act );
	if (slot >= 0) {
		core->GetGame()->DelNPC( slot );
	}
}

//this apparently doesn't check the gold, thus could be used from non actors
void GameScript::GivePartyGoldGlobal(Scriptable* Sender, Action* parameters)
{
	ieDword gold = (ieDword) CheckVariable( Sender, parameters->string0Parameter, parameters->string1Parameter );
	if (Sender->Type == ST_ACTOR) {
		Actor* act = ( Actor* ) Sender;
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
	if (Sender->Type == ST_ACTOR) {
		Actor* act = ( Actor* ) Sender;
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
	if (Sender->Type == ST_ACTOR) {
		Actor* act = ( Actor* ) Sender;
		//fixes PST limlim shop, partymembers don't receive the taken gold
		if (!act->InParty) {
			act->SetBase(IE_GOLD, act->GetBase(IE_GOLD)+gold);
		}
	}
}

void GameScript::AddXPObject(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1] );
	if (!tar) {
		return;
	}
	if (tar->Type != ST_ACTOR) {
		return;
	}
	Actor* actor = ( Actor* ) tar;
	int xp = parameters->int0Parameter;
	if (displaymsg->HasStringReference(STR_GOTQUESTXP)) {
		core->GetTokenDictionary()->SetAtCopy("EXPERIENCEAMOUNT", xp);
		displaymsg->DisplayConstantStringName(STR_GOTQUESTXP, DMC_BG2XPGREEN, actor);
	} else {
		displaymsg->DisplayConstantStringValue(STR_GOTXP, DMC_BG2XPGREEN, (ieDword)xp);
	}
	actor->AddExperience(xp);
	core->PlaySound(DS_GOTXP);
}

void GameScript::AddXP2DA(Scriptable* /*Sender*/, Action* parameters)
{
	AutoTable xptable;

	if (core->HasFeature(GF_HAS_EXPTABLE) ) {
		xptable.load("exptable");
	} else {
		xptable.load("xplist");
	}

	if (parameters->int0Parameter>0) {
		displaymsg->DisplayString(parameters->int0Parameter, DMC_BG2XPGREEN, IE_STR_SOUND);
	}
	if (!xptable) {
		printMessage("GameScript", "Can't perform ADDXP2DA\n", LIGHT_RED);
		return;
	}
	const char * xpvalue = xptable->QueryField( parameters->string0Parameter, "0" ); //level is unused

	if ( xpvalue[0]=='P' && xpvalue[1]=='_') {
		//divide party xp
		core->GetGame()->ShareXP(atoi(xpvalue+2), SX_DIVIDE );
	} else {
		//give xp everyone
		core->GetGame()->ShareXP(atoi(xpvalue), 0 );
	}
	core->PlaySound(DS_GOTXP);
}

void GameScript::AddExperienceParty(Scriptable* /*Sender*/, Action* parameters)
{
	core->GetGame()->ShareXP(parameters->int0Parameter, SX_DIVIDE);
	core->PlaySound(DS_GOTXP);
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
	core->PlaySound(DS_GOTXP);
}

void GameScript::SetMoraleAI(Scriptable* Sender, Action* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		return;
	}
	Actor* act = ( Actor* ) Sender;
	act->SetBase(IE_MORALE, parameters->int0Parameter);
}

void GameScript::IncMoraleAI(Scriptable* Sender, Action* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		return;
	}
	Actor* act = ( Actor* ) Sender;
	act->SetBase(IE_MORALE, parameters->int0Parameter+act->GetBase(IE_MORALE) );
}

void GameScript::MoraleSet(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1] );
	if (!tar) {
		return;
	}
	if (tar->Type != ST_ACTOR) {
		return;
	}
	Actor* act = ( Actor* ) tar;
	act->SetBase(IE_MORALEBREAK, parameters->int0Parameter);
}

void GameScript::MoraleInc(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1] );
	if (!tar) {
		return;
	}
	if (tar->Type != ST_ACTOR) {
		return;
	}
	Actor* act = ( Actor* ) tar;
	act->SetBase(IE_MORALEBREAK, act->GetBase(IE_MORALEBREAK)+parameters->int0Parameter);
}

void GameScript::MoraleDec(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1] );
	if (!tar) {
		return;
	}
	if (tar->Type != ST_ACTOR) {
		return;
	}
	Actor* act = ( Actor* ) tar;
	act->SetBase(IE_MORALEBREAK, act->GetBase(IE_MORALEBREAK)-parameters->int0Parameter);
}

void GameScript::JoinParty(Scriptable* Sender, Action* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		return;
	}
	// make sure we're in the same area, otherwise Dynaheir joins when Minsc does
	// but she's in another area and needs to be rescued first!
	Actor* act = ( Actor* ) Sender;
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
		act->SetScript( "", SCR_RACE, true );
		act->SetScript( "", SCR_GENERAL, true );
		act->SetScript( "DPLAYER2", SCR_DEFAULT, false );
	}
	AutoTable pdtable("pdialog");
	if (pdtable) {
		const char* scriptname = act->GetScriptName();
		ieResRef resref;
		//set dialog only if we got a row
		if (pdtable->GetRowIndex( scriptname ) != -1) {
			if (game->Expansion==5) {
				strnlwrcpy(resref, pdtable->QueryField( scriptname, "25JOIN_DIALOG_FILE"), sizeof(ieResRef)-1);
			} else {
				strnlwrcpy(resref, pdtable->QueryField( scriptname, "JOIN_DIALOG_FILE"), sizeof(ieResRef)-1);
			}
			act->SetDialog( resref );
		}
	}
	game->JoinParty( act, JP_JOIN );
}

void GameScript::LeaveParty(Scriptable* Sender, Action* /*parameters*/)
{
	if (Sender->Type != ST_ACTOR) {
		return;
	}
	Actor* act = ( Actor* ) Sender;
	core->GetGame()->LeaveParty( act );
}

//HideCreature hides only the visuals of a creature
//(feet circle and avatar)
//the scripts of the creature are still running
//iwd2 stores this flag in the MC field
void GameScript::HideCreature(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1] );
	if (!tar || tar->Type != ST_ACTOR) {
		return;
	}
	Actor* actor = ( Actor* ) tar;
	actor->BaseStats[IE_AVATARREMOVAL]=parameters->int0Parameter;
}

//i have absolutely no idea why this is needed when we have HideCreature
void GameScript::ForceHide(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1] );
	if (!tar) {
		tar=Sender;
	}
	if (tar->Type != ST_ACTOR) {
		return;
	}
	Actor* actor = ( Actor* ) tar;
	actor->BaseStats[IE_AVATARREMOVAL]=1;
}

void GameScript::ForceLeaveAreaLUA(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1] );
	if (!tar || tar->Type != ST_ACTOR) {
		return;
	}
	Actor* actor = ( Actor* ) tar;
	//the LoadMos ResRef may be empty, don't overwrite it if there is one set (textscreens may do this)
	if (parameters->string1Parameter[0]) {
		strnlwrcpy(core->GetGame()->LoadMos, parameters->string1Parameter, sizeof(ieResRef)-1);
	}
	//strncpy(core->GetGame()->LoadMos, parameters->string1Parameter,8);
	MoveBetweenAreasCore( actor, parameters->string0Parameter, parameters->pointParameter, parameters->int0Parameter, true);
}

void GameScript::LeaveAreaLUA(Scriptable* Sender, Action* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		return;
	}
	Actor* actor = ( Actor* ) Sender;
	//the LoadMos ResRef may be empty
	if (parameters->string1Parameter[0]) {
		strnlwrcpy(core->GetGame()->LoadMos, parameters->string1Parameter, sizeof(ieResRef)-1);
	}
	//strncpy(core->GetGame()->LoadMos, parameters->string1Parameter,8);
	MoveBetweenAreasCore( actor, parameters->string0Parameter, parameters->pointParameter, parameters->int0Parameter, true);
}

//this is a blocking action, because we have to move to the Entry
void GameScript::LeaveAreaLUAEntry(Scriptable* Sender, Action* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		Sender->ReleaseCurrentAction();
		return;
	}
	Game *game = core->GetGame();
	if (parameters->string1Parameter[0]) {
		strnlwrcpy(game->LoadMos, parameters->string1Parameter, sizeof(ieResRef)-1);
	}
	//strncpy(game->LoadMos, parameters->string1Parameter,8);
	Point p = GetEntryPoint(parameters->string0Parameter, parameters->string1Parameter);
	if (p.isempty()) {
		Sender->ReleaseCurrentAction();
		return;
	}
	parameters->pointParameter=p;
	strcpy(parameters->string1Parameter, "");
	LeaveAreaLUA(Sender, parameters);
	Sender->ReleaseCurrentAction();
}

void GameScript::LeaveAreaLUAPanic(Scriptable* Sender, Action* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		return;
	}
	Actor* actor = ( Actor* ) Sender;
	if (parameters->string1Parameter[0]) {
		strnlwrcpy(core->GetGame()->LoadMos, parameters->string1Parameter, sizeof(ieResRef)-1);
	}
	//strncpy(core->GetGame()->LoadMos, parameters->string1Parameter,8);
	MoveBetweenAreasCore( actor, parameters->string0Parameter, parameters->pointParameter, parameters->int0Parameter, true);
}

//this is a blocking action, because we have to move to the Entry
void GameScript::LeaveAreaLUAPanicEntry(Scriptable* Sender, Action* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		Sender->ReleaseCurrentAction();
		return;
	}
	Game *game = core->GetGame();
	if (parameters->string1Parameter[0]) {
		strnlwrcpy(game->LoadMos, parameters->string1Parameter, sizeof(ieResRef)-1);
	}
	//strncpy(game->LoadMos, parameters->string1Parameter,8);
	Point p = GetEntryPoint(parameters->string0Parameter, parameters->string1Parameter);
	if (p.isempty()) {
		Sender->ReleaseCurrentAction();
		return;
	}
	parameters->pointParameter=p;
	strcpy(parameters->string1Parameter, "");
	LeaveAreaLUAPanic(Sender, parameters);
	Sender->ReleaseCurrentAction();
}

void GameScript::SetToken(Scriptable* /*Sender*/, Action* parameters)
{
	//SetAt takes a newly created reference (no need of free/copy)
	char * str = core->GetString( parameters->int0Parameter);
	core->GetTokenDictionary()->SetAt( parameters->string1Parameter, str);
}

//Assigns a numeric variable to the token
void GameScript::SetTokenGlobal(Scriptable* Sender, Action* parameters)
{
	ieDword value = CheckVariable( Sender, parameters->string0Parameter );
	//using SetAtCopy because we need a copy of the value
	core->GetTokenDictionary()->SetAtCopy( parameters->string1Parameter, value );
}

//Assigns the target object's name (not scriptname) to the token
void GameScript::SetTokenObject(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1] );
	if (!tar || tar->Type != ST_ACTOR) {
		return;
	}
	Actor* actor = ( Actor* ) tar;
	core->GetTokenDictionary()->SetAtCopy( parameters->string0Parameter, actor->GetName(0) );
}

void GameScript::PlayDead(Scriptable* Sender, Action* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		Sender->ReleaseCurrentAction();
		return;
	}
	Actor* actor = ( Actor* ) Sender;

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
	if (Sender->Type != ST_ACTOR) {
		Sender->ReleaseCurrentAction();
		return;
	}
	Actor* actor = ( Actor* ) Sender;

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

/* this may not be correct, just a placeholder you can fix */
void GameScript::Swing(Scriptable* Sender, Action* /*parameters*/)
{
	if (Sender->Type != ST_ACTOR) {
		return;
	}
	Actor* actor = ( Actor* ) Sender;
	actor->SetStance( IE_ANI_ATTACK );
	actor->SetWait( 1 );
}

/* this may not be correct, just a placeholder you can fix */
void GameScript::SwingOnce(Scriptable* Sender, Action* /*parameters*/)
{
	if (Sender->Type != ST_ACTOR) {
		return;
	}
	Actor* actor = ( Actor* ) Sender;
	actor->SetStance( IE_ANI_ATTACK );
	actor->SetWait( 1 );
}

void GameScript::Recoil(Scriptable* Sender, Action* /*parameters*/)
{
	if (Sender->Type != ST_ACTOR) {
		return;
	}
	Actor* actor = ( Actor* ) Sender;
	actor->SetStance( IE_ANI_DAMAGE );
	actor->SetWait( 1 );
}

void GameScript::AnkhegEmerge(Scriptable* Sender, Action* /*parameters*/)
{
	if (Sender->Type != ST_ACTOR) {
		return;
	}
	Actor* actor = ( Actor* ) Sender;
	if (actor->GetStance()!=IE_ANI_EMERGE) {
		actor->SetStance( IE_ANI_EMERGE );
		actor->SetWait( 1 );
	}
}

void GameScript::AnkhegHide(Scriptable* Sender, Action* /*parameters*/)
{
	if (Sender->Type != ST_ACTOR) {
		return;
	}
	Actor* actor = ( Actor* ) Sender;
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
	SetVariable( Sender, parameters->string0Parameter, "GLOBAL", value1 + value2 );
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
	long value1 = CheckVariable( Sender, parameters->string0Parameter );
	if (value1 > parameters->int0Parameter) {
		SetVariable( Sender, parameters->string0Parameter, value1 );
	}
}

void GameScript::GlobalMin(Scriptable* Sender, Action* parameters)
{
	long value1 = CheckVariable( Sender, parameters->string0Parameter );
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
	Actor *except = NULL;
	if (Sender->Type==ST_ACTOR) {
		except = (Actor *) Sender;
	}
	Map *map = Sender->GetCurrentArea();
	ieDword gametime = core->GetGame()->GameTime;
	int i = map->GetActorCount(true);
	while(i--) {
		Actor* act = map->GetActor(i,true);
		if (act && act!=except) {
			if (!act->ValidTarget(GA_NO_DEAD) ) {
				continue;
			}
			//Do we need this???
			if (!act->Schedule(gametime, false) ) {
				continue;
			}
			act->ClearActions();
			act->ClearPath();
			act->SetModal(MS_NONE);
		}
	}
}

void GameScript::ClearActions(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = Sender;
	if (parameters->objects[1]) {
		tar = GetActorFromObject( Sender, parameters->objects[1] );
		if (!tar) {
			printMessage("GameScript","Couldn't find target for ClearActions!",YELLOW);
			parameters->objects[1]->dump();
			return;
		}
	}
	tar->ClearActions();
	if (tar->Type==ST_ACTOR) {
		Actor* act = (Actor *) tar;
		act->ClearPath();
		//not sure about this
		//act->SetModal(MS_NONE);
	}
}

void GameScript::SetNumTimesTalkedTo(Scriptable* Sender, Action* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		return;
	}
	Actor* actor = ( Actor* ) Sender;
	actor->TalkCount = parameters->int0Parameter;
}

void GameScript::StartMovie(Scriptable* Sender, Action* parameters)
{
	core->PlayMovie( parameters->string0Parameter );
	Sender->ReleaseCurrentAction(); // should this be blocking?
}

void GameScript::SetLeavePartyDialogFile(Scriptable* Sender, Action* /*parameters*/)
{
	if (Sender->Type != ST_ACTOR) {
		return;
	}
	AutoTable pdtable("pdialog");
	Actor* act = ( Actor* ) Sender;
	const char* scriptname = act->GetScriptName();
	if (pdtable->GetRowIndex( scriptname ) != -1) {
		ieResRef resref;

		if (core->GetGame()->Expansion==5) {
			strnlwrcpy(resref, pdtable->QueryField( scriptname, "25POST_DIALOG_FILE" ), sizeof(ieResRef)-1 );
		} else {
			strnlwrcpy(resref, pdtable->QueryField( scriptname, "POST_DIALOG_FILE" ), sizeof(ieResRef)-1 );
		}
		act->SetDialog(resref);
	}
}

void GameScript::TextScreen(Scriptable* /*Sender*/, Action* parameters)
{
	strnlwrcpy(core->GetGame()->LoadMos, parameters->string0Parameter, sizeof(ieResRef)-1);
	core->SetEventFlag(EF_TEXTSCREEN);
}

void GameScript::IncrementChapter(Scriptable* Sender, Action* parameters)
{
	core->GetGame()->IncrementChapter();
	TextScreen(Sender, parameters);
}

void GameScript::SetCriticalPathObject(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1] );
	if (!tar || tar->Type != ST_ACTOR) {
		return;
	}
	Actor* actor = ( Actor* ) tar;
	if (parameters->int0Parameter) {
		actor->SetMCFlag(MC_PLOT_CRITICAL, BM_OR);
	} else {
		actor->SetMCFlag(MC_PLOT_CRITICAL, BM_NAND);
	}
}

void GameScript::SetBeenInPartyFlags(Scriptable* Sender, Action* /*parameters*/)
{
	if (Sender->Type != ST_ACTOR) {
		return;
	}
	Actor* actor = ( Actor* ) Sender;
	//it is bit 15 of the multi-class flags (confirmed)
	actor->SetMCFlag(MC_BEENINPARTY, BM_OR);
}

/*iwd2 sets the high MC bits this way*/
void GameScript::SetCreatureAreaFlag(Scriptable* Sender, Action* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		return;
	}
	Actor* actor = ( Actor* ) Sender;
	actor->SetMCFlag(parameters->int0Parameter, parameters->int1Parameter);
}

//this will be a global change, fixme if it should be local
void GameScript::SetTextColor(Scriptable* /*Sender*/, Action* parameters)
{
	Color c;
	memcpy(&c,&parameters->int0Parameter,4);
	core->SetInfoTextColor(c);
}

void GameScript::BitGlobal(Scriptable* Sender, Action* parameters)
{
	ieDword value = CheckVariable(Sender, parameters->string0Parameter );
	HandleBitMod( value, parameters->int0Parameter, parameters->int1Parameter);
	SetVariable(Sender, parameters->string0Parameter, value);
}

void GameScript::GlobalBitGlobal(Scriptable* Sender, Action* parameters)
{
	ieDword value1 = CheckVariable(Sender, parameters->string0Parameter );
	ieDword value2 = CheckVariable(Sender, parameters->string1Parameter );
	HandleBitMod( value1, value2, parameters->int1Parameter);
	SetVariable(Sender, parameters->string0Parameter, value1);
}

void GameScript::SetVisualRange(Scriptable* Sender, Action* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		return;
	}
	Actor* actor = ( Actor* ) Sender;
	actor->SetBase(IE_VISUALRANGE,parameters->int0Parameter);
}

void GameScript::MakeUnselectable(Scriptable* Sender, Action* parameters)
{
	Sender->UnselectableTimer=parameters->int0Parameter * AI_UPDATE_TIME;

	//update color
	if (Sender->Type != ST_ACTOR) {
		return;
	}
	Actor* actor = ( Actor* ) Sender;
	if (parameters->int0Parameter) {
		// flags may be wrong
		core->GetGame()->SelectActor(actor, false, SELECT_QUIET);
	}

	actor->SetCircleSize();
}

void GameScript::Debug(Scriptable* /*Sender*/, Action* parameters)
{
	InDebug=parameters->int0Parameter;
	printMessage("GameScript", "DEBUG: %s\n", YELLOW, parameters->string0Parameter);
}

void GameScript::IncrementProficiency(Scriptable* Sender, Action* parameters)
{
	unsigned int idx = parameters->int0Parameter;
	if (idx>31) {
		return;
	}
	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1] );
	if (!tar) {
		return;
	}
	if (tar->Type != ST_ACTOR) {
		return;
	}
	Actor* target = ( Actor* ) tar;
	//start of the proficiency stats
	target->SetBase(IE_PROFICIENCYBASTARDSWORD+idx,
		target->GetBase(IE_PROFICIENCYBASTARDSWORD+idx)+parameters->int1Parameter);
}

void GameScript::IncrementExtraProficiency(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1] );
	if (!tar) {
		return;
	}
	if (tar->Type != ST_ACTOR) {
		return;
	}
	Actor* target = ( Actor* ) tar;
	target->SetBase(IE_FREESLOTS, target->GetBase(IE_FREESLOTS)+parameters->int0Parameter);
}

//the third parameter is a GemRB extension
void GameScript::AddJournalEntry(Scriptable* /*Sender*/, Action* parameters)
{
	core->GetGame()->AddJournalEntry(parameters->int0Parameter, parameters->int1Parameter, parameters->int2Parameter);
}

void GameScript::SetQuestDone(Scriptable* /*Sender*/, Action* parameters)
{
	Game *game = core->GetGame();
	game->DeleteJournalEntry(parameters->int0Parameter);
	game->AddJournalEntry(parameters->int0Parameter, IE_GAM_QUEST_DONE, parameters->int2Parameter);

}

void GameScript::RemoveJournalEntry(Scriptable* /*Sender*/, Action* parameters)
{
	core->GetGame()->DeleteJournalEntry(parameters->int0Parameter);
}

void GameScript::SetInternal(Scriptable* Sender, Action* parameters)
{
	unsigned int idx = parameters->int0Parameter;
	if (idx>15) {
		return;
	}
	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1] );
	if (!tar) {
		return;
	}
	if (tar->Type != ST_ACTOR) {
		return;
	}
	Actor* target = ( Actor* ) tar;
	//start of the internal stats
	target->SetBase(IE_INTERNAL_0+idx, parameters->int1Parameter);
}

void GameScript::IncInternal(Scriptable* Sender, Action* parameters)
{
	unsigned int idx = parameters->int0Parameter;
	if (idx>15) {
		return;
	}
	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1] );
	if (!tar || tar->Type != ST_ACTOR) {
		return;
	}
	Actor* target = ( Actor* ) tar;
	//start of the internal stats
	target->SetBase(IE_INTERNAL_0+idx,
		target->GetBase(IE_INTERNAL_0+idx)+parameters->int1Parameter);
}

void GameScript::DestroyAllEquipment(Scriptable* Sender, Action* /*parameters*/)
{
	Inventory *inv=NULL;

	switch (Sender->Type) {
		case ST_ACTOR:
			inv = &(((Actor *) Sender)->inventory);
			break;
		case ST_CONTAINER:
			inv = &(((Container *) Sender)->inventory);
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
			inv = &(((Actor *) Sender)->inventory);
			break;
		case ST_CONTAINER:
			inv = &(((Container *) Sender)->inventory);
			break;
		default:;
	}
	if (inv) {
		inv->DestroyItem(parameters->string0Parameter,0,1); //destroy one (even indestructible?)
	}
}

//negative destroygold creates gold
void GameScript::DestroyGold(Scriptable* Sender, Action* parameters)
{
	if (Sender->Type!=ST_ACTOR)
		return;
	Actor *act=(Actor *) Sender;
	int max=(int) act->GetStat(IE_GOLD);
	if (parameters->int0Parameter != 0) {
		if (max>parameters->int0Parameter) {
			max=parameters->int0Parameter;
		}
	}
	act->SetBase(IE_GOLD, act->GetBase(IE_GOLD)-max);
}

void GameScript::DestroyPartyItem(Scriptable* /*Sender*/, Action* parameters)
{
	Game *game = core->GetGame();
	int i = game->GetPartySize(false);
	ieDword count;
	if (parameters->int0Parameter) {
		count=0;
	} else {
		count=1;
	}
	while (i--) {
		Inventory *inv = &(game->GetPC( i,false )->inventory);
		int res=inv->DestroyItem(parameters->string0Parameter,0,count);
		if ( (count == 1) && res) {
			break;
		}
	}
}

/* this is a gemrb extension */
void GameScript::DestroyPartyItemNum(Scriptable* /*Sender*/, Action* parameters)
{
	Game *game = core->GetGame();
	int i = game->GetPartySize(false);
	ieDword count;
	count = parameters->int0Parameter;
	while (i--) {
		Inventory *inv = &(game->GetPC( i,false )->inventory);
		count -= inv->DestroyItem(parameters->string0Parameter,0,count);
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
			inv = &(((Actor *) Sender)->inventory);
			break;
		case ST_CONTAINER:
			inv = &(((Container *) Sender)->inventory);
			break;
		default:;
	}
	if (inv) {
		inv->DestroyItem("", IE_INV_ITEM_DESTRUCTIBLE, (ieDword) ~0);
	}
}

void GameScript::SetApparentName(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1] );
	if (!tar || tar->Type != ST_ACTOR) {
		return;
	}
	Actor* target = ( Actor* ) tar;
	target->SetName(parameters->int0Parameter,1);
}

void GameScript::SetRegularName(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1] );
	if (!tar || tar->Type != ST_ACTOR) {
		return;
	}
	Actor* target = ( Actor* ) tar;
	target->SetName(parameters->int0Parameter,2);
}

/** this is a gemrb extension */
void GameScript::UnloadArea(Scriptable* /*Sender*/, Action* parameters)
{
	int map=core->GetGame()->FindMap(parameters->string0Parameter);
	if (map>=0) {
		core->GetGame()->DelMap(map, parameters->int0Parameter);
	}
}

static EffectRef fx_death_ref = { "Death", -1 };
void GameScript::Kill(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1] );
	if (!tar || tar->Type != ST_ACTOR) {
		return;
	}
	Actor* target = ( Actor* ) tar;
	Effect *fx = EffectQueue::CreateEffect(fx_death_ref, 0, 0, FX_DURATION_INSTANT_PERMANENT);
	target->fxqueue.AddEffect(fx, false);
	delete fx;
}

void GameScript::SetGabber(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1] );
	if (!tar || tar->Type != ST_ACTOR) {
		return;
	}
	GameControl* gc = core->GetGameControl();
	if (gc->GetDialogueFlags()&DF_IN_DIALOG) {
		gc->dialoghandler->speakerID = tar->GetGlobalID();
	} else {
		printMessage("GameScript", "Can't set gabber!\n", YELLOW);
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
	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1] );
	if (!tar || tar->Type != ST_ACTOR) {
		return;
	}
	Actor *scr = (Actor *) tar;
	//0 means full healing
	//Heal() might contain curing of some conditions
	//if FullHeal doesn't do that, replace this with a SetBase
	//fullhealex might still be the curing action
	scr->Heal(0);
}

void GameScript::RemovePaladinHood(Scriptable* Sender, Action* /*parameters*/)
{
	if (Sender->Type!=ST_ACTOR) {
		return;
	}
	Actor *act = (Actor *) Sender;
	act->ApplyKit(true);
	act->SetMCFlag(MC_FALLEN_PALADIN, BM_OR);
	if (act->InParty) displaymsg->DisplayConstantStringName(STR_PALADIN_FALL, DMC_BG2XPGREEN, act);
}

void GameScript::RemoveRangerHood(Scriptable* Sender, Action* /*parameters*/)
{
	if (Sender->Type!=ST_ACTOR) {
		return;
	}
	Actor *act = (Actor *) Sender;
	act->ApplyKit(true);
	act->SetMCFlag(MC_FALLEN_RANGER, BM_OR);
	if (act->InParty) displaymsg->DisplayConstantStringName(STR_RANGER_FALL, DMC_BG2XPGREEN, act);
}

void GameScript::RegainPaladinHood(Scriptable* Sender, Action* /*parameters*/)
{
	if (Sender->Type!=ST_ACTOR) {
		return;
	}
	Actor *act = (Actor *) Sender;
	act->SetMCFlag(MC_FALLEN_PALADIN, BM_NAND);
	act->ApplyKit(false);
}

void GameScript::RegainRangerHood(Scriptable* Sender, Action* /*parameters*/)
{
	if (Sender->Type!=ST_ACTOR) {
		return;
	}
	Actor *act = (Actor *) Sender;
	act->SetMCFlag(MC_FALLEN_RANGER, BM_NAND);
	act->ApplyKit(false);
}

//transfering item from Sender to target, target must be an actor
//if target can't get it, it will be dropped at its feet
//a container or an actor can take an item from someone
void GameScript::GetItem(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1] );
	MoveItemCore(tar, Sender, parameters->string0Parameter,0,0);
}

//getting one single item
void GameScript::TakePartyItem(Scriptable* Sender, Action* parameters)
{
	Game *game=core->GetGame();
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
	Game *game=core->GetGame();
	int i=game->GetPartySize(false);
	while (i--) {
		Actor *pc = game->GetPC(i, false);
		int personal_count = pc->inventory.CountItems(parameters->string0Parameter, true);
		if (!personal_count) continue;
		int res=MoveItemCore(pc, Sender, parameters->string0Parameter,IE_INV_ITEM_UNDROPPABLE, IE_INV_ITEM_UNSTEALABLE, count);
		if (res == MIC_GOTITEM) {
			i++;
			// decrease only by the removed amount, since multiple slots may have been involved
			count -= (personal_count - pc->inventory.CountItems(parameters->string0Parameter, true));
		}
		if (count < 1) return;
	}
}

void GameScript::TakePartyItemRange(Scriptable* Sender, Action* parameters)
{
	Game *game=core->GetGame();
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
	Game *game=core->GetGame();
	int i=game->GetPartySize(false);
	while (i--) {
		while (MoveItemCore(game->GetPC(i,false), Sender, parameters->string0Parameter,IE_INV_ITEM_UNDROPPABLE, IE_INV_ITEM_UNSTEALABLE)==MIC_GOTITEM) { }
	}
}

//an actor can 'give' an item to a container or another actor
void GameScript::GiveItem(Scriptable *Sender, Action* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1] );
	MoveItemCore(Sender, tar, parameters->string0Parameter,0,0);
}

//this action creates an item in a container or a creature
//if there is an object it works as GiveItemCreate
//otherwise it creates the item on the Sender
void GameScript::CreateItem(Scriptable *Sender, Action* parameters)
{
	Scriptable* tar;
	if (parameters->objects[1]) {
		tar = GetActorFromObject( Sender, parameters->objects[1] );
	} else {
		tar = Sender;
	}
	if (!tar)
		return;
	Inventory *myinv;

	switch(tar->Type) {
		case ST_ACTOR:
			myinv = &((Actor *) tar)->inventory;
			break;
		case ST_CONTAINER:
			myinv = &((Container *) tar)->inventory;
			break;
		default:
			return;
	}

	CREItem *item = new CREItem();
	if (!CreateItemCore(item, parameters->string0Parameter, parameters->int0Parameter, parameters->int1Parameter, parameters->int2Parameter)) {
		delete item;
		return;
	}
	if (tar->Type==ST_CONTAINER) {
		myinv->AddItem(item);
	} else {
		if ( ASI_SUCCESS != myinv->AddSlotItem(item, SLOT_ONLYINVENTORY)) {
			Map *map=tar->GetCurrentArea();
			// drop it at my feet
			map->AddItemToLocation(tar->Pos, item);
			if (((Actor *)tar)->InParty) displaymsg->DisplayConstantString(STR_INVFULL_ITEMDROP, DMC_BG2XPGREEN);
		} else {
			if (((Actor *)tar)->InParty) displaymsg->DisplayConstantString(STR_GOTITEM, DMC_BG2XPGREEN);
		}
	}
}

void GameScript::CreateItemNumGlobal(Scriptable *Sender, Action* parameters)
{
	Inventory *myinv;

	switch(Sender->Type) {
		case ST_ACTOR:
			myinv = &((Actor *) Sender)->inventory;
			break;
		case ST_CONTAINER:
			myinv = &((Container *) Sender)->inventory;
			break;
		default:
			return;
	}
	int value = CheckVariable( Sender, parameters->string0Parameter );
	CREItem *item = new CREItem();
	if (!CreateItemCore(item, parameters->string1Parameter, value, 0, 0)) {
		delete item;
		return;
	}
	if (Sender->Type==ST_CONTAINER) {
		myinv->AddItem(item);
	} else {
		if ( ASI_SUCCESS != myinv->AddSlotItem(item, SLOT_ONLYINVENTORY)) {
			Map *map=Sender->GetCurrentArea();
			// drop it at my feet
			map->AddItemToLocation(Sender->Pos, item);
			if (((Actor *)Sender)->InParty) displaymsg->DisplayConstantString(STR_INVFULL_ITEMDROP, DMC_BG2XPGREEN);
		} else {
			if (((Actor *)Sender)->InParty) displaymsg->DisplayConstantString(STR_GOTITEM, DMC_BG2XPGREEN);
		}
	}
}

void GameScript::TakeItemReplace(Scriptable *Sender, Action* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1] );
	if (!tar || tar->Type != ST_ACTOR) {
		return;
	}

	Actor *scr = (Actor *) tar;
	CREItem *item;
	int slot = scr->inventory.RemoveItem(parameters->string1Parameter, IE_INV_ITEM_UNDROPPABLE, &item);
	if (!item) {
		item = new CREItem();
	}
	if (!CreateItemCore(item, parameters->string0Parameter, -1, 0, 0)) {
		delete item;
		return;
	}
	if (ASI_SUCCESS != scr->inventory.AddSlotItem(item,slot)) {
		Map *map = scr->GetCurrentArea();
		map->AddItemToLocation(Sender->Pos, item);
	}
}

//same as equipitem, but with additional slots parameter, and object to perform action
void GameScript::XEquipItem(Scriptable *Sender, Action* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1] );

	if (!tar || tar->Type!=ST_ACTOR) {
		return;
	}
	Actor *actor = (Actor *) tar;
	int slot = actor->inventory.FindItem(parameters->string0Parameter, 0);
	if (slot<0) {
		return;
	}
	actor->inventory.EquipItem(slot);
	actor->ReinitQuickSlots();
}

//GemRB extension: if int1Parameter is nonzero, don't destroy existing items
void GameScript::FillSlot(Scriptable *Sender, Action* parameters)
{
	if (Sender->Type!=ST_ACTOR) {
		return;
	}

	CREItem *tmp = NULL;
	Actor *actor = (Actor *) Sender;
	int slot = parameters->int0Parameter;

	//free up target slot
	tmp = actor->inventory.RemoveItem(slot);

	actor->inventory.TryEquipAll(slot);

	if (tmp) {
		if (actor->inventory.HasItemInSlot("",slot) ) {
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
	if (Sender->Type!=ST_ACTOR) {
		return;
	}
	Actor *actor = (Actor *) Sender;
	int slot = actor->inventory.FindItem(parameters->string0Parameter, IE_INV_ITEM_UNDROPPABLE);
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
	if (si) {
		if (actor->inventory.AddSlotItem(si, slot2)==ASI_FAILED) {
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

void GameScript::DropItem(Scriptable *Sender, Action* parameters)
{
	if (Sender->Type!=ST_ACTOR) {
		Sender->ReleaseCurrentAction();
		return;
	}
	if (Distance(parameters->pointParameter, Sender) > 10) {
		MoveNearerTo(Sender, parameters->pointParameter, 10,0);
		return;
	}
	Actor *scr = (Actor *) Sender;
	Map *map = Sender->GetCurrentArea();

	if (parameters->string0Parameter[0]) {
		//dropping location isn't exactly our place, this is why i didn't use a simple DropItem
		scr->inventory.DropItemAtLocation(parameters->string0Parameter,
0, map, parameters->pointParameter);
	} else {
		//this should be converted from scripting slot to physical slot
		scr->inventory.DropItemAtLocation(parameters->int0Parameter, 0, map, parameters->pointParameter);
	}

	Sender->ReleaseCurrentAction();
}

void GameScript::DropInventory(Scriptable *Sender, Action* /*parameters*/)
{
	if (Sender->Type!=ST_ACTOR) {
		return;
	}
	Actor *scr = (Actor *) Sender;
	scr->DropItem("",0);
}

//this should work on containers!
//using the same code for DropInventoryEXExclude
void GameScript::DropInventoryEX(Scriptable *Sender, Action* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1] );
	if (!tar) {
		return;
	}
	Inventory *inv = NULL;
	switch (Sender->Type) {
		case ST_ACTOR:
			inv = &(((Actor *) tar)->inventory);
			break;
		case ST_CONTAINER:
			inv = &(((Container *) tar)->inventory);
			break;
		default:;
	}
	if (inv) {
		int x = inv->GetSlotCount();
		Map *area = tar->GetCurrentArea();
		while(x--) {
			if (parameters->string0Parameter[0]) {
				const char *resref = inv->GetSlotItem(x)->ItemResRef;
				if (!strnicmp(parameters->string0Parameter, resref, 8)) {
					continue;
				}
			}
			inv->DropItemAtLocation(x, 0, area, tar->Pos);
		}
	}
}

void GameScript::GivePartyAllEquipment(Scriptable *Sender, Action* /*parameters*/)
{
	if (Sender->Type!=ST_ACTOR) {
		return;
	}
	Game *game = core->GetGame();
	// pick the first actor first
	for (int i = 0; i < game->GetPartySize(false); i++) {
		Actor *tar = game->GetPC(i,false);
		//don't try to give self, it would be an infinite loop
		if (tar==(Actor *) Sender)
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

	if (tar->Type == ST_ACTOR) {
		Actor *scr = (Actor *) tar;
		//can plunder only dead actors
		if (! (scr->BaseStats[IE_STATE_ID]&STATE_DEAD) ) {
			Sender->ReleaseCurrentAction();
			return;
		}
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
	Scriptable* src = GetActorFromObject( Sender, parameters->objects[1] );
	if (!src || src->Type!=ST_ACTOR) {
		return;
	}
	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[2] );
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
	if (Sender->Type!=ST_ACTOR) {
		Sender->ReleaseCurrentAction();
		return;
	}
	Scriptable* tar = GetStoredActorFromObject( Sender, parameters->objects[1] );
	if (!tar || tar->Type!=ST_ACTOR) {
		Sender->ReleaseCurrentAction();
		return;
	}
	Actor *snd = (Actor *) Sender;
	Actor *scr = (Actor *) tar;
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

	if (scr->GetStat(IE_EA)>EA_EVILCUTOFF) {
		displaymsg->DisplayConstantString(STR_PICKPOCKET_EVIL, DMC_WHITE);
		Sender->ReleaseCurrentAction();
		return;
	}

	int skill = snd->GetStat(IE_PICKPOCKET);
	int tgt = scr->GetStat(IE_PICKPOCKET);
	//the original engine has no random here
	if (tgt != 255) {
		skill -= tgt;
		//if you want original behaviour: remove this
		skill += core->Roll(1,100, snd->GetStat(IE_LUCK) );
	} else {
		skill = 0;
	}
	//and change this 50 to 0.
	if (skill<50) {
		//noticed attempt
		displaymsg->DisplayConstantString(STR_PICKPOCKET_FAIL, DMC_WHITE);
		if (core->HasFeature(GF_STEAL_IS_ATTACK) ) {
			scr->AttackedBy(snd);
		} else {
			//pickpocket failed trigger
			tar->AddTrigger(TriggerEntry(trigger_pickpocketfailed, snd->GetGlobalID()));
		}
		Sender->ReleaseCurrentAction();
		return;
	}

	int ret = MIC_NOITEM;
	if ((RandomNumValue&3) || (scr->GetStat(IE_GOLD)<=0) ) {
		int slot = scr->inventory.FindStealableItem();
		if (slot) {
			CREItem *item = scr->inventory.RemoveItem(slot);
			ret = snd->inventory.AddSlotItem(item, SLOT_ONLYINVENTORY);
			if (ret!=ASI_SUCCESS) {
				map->AddItemToLocation(snd->Pos, item);
				ret = MIC_FULL;
			}
		}
	}

	if (ret==MIC_NOITEM) {
		int money=0;
		//go for money too
		if (scr->GetStat(IE_GOLD)>0) {
			money=RandomNumValue%(scr->GetStat(IE_GOLD)+1);
		}
		if (!money) {
			//no stuff to steal
			displaymsg->DisplayConstantString(STR_PICKPOCKET_NONE, DMC_WHITE);
			Sender->ReleaseCurrentAction();
			return;
		}
		CREItem *item = new CREItem();
		if (!CreateItemCore(item, core->GoldResRef, money, 0, 0)) {
			error("GameScript", "Failed to create pick-pocketed gold '%s' %dg.\n", core->GoldResRef, money);
		}
		if ( ASI_SUCCESS == snd->inventory.AddSlotItem(item, SLOT_ONLYINVENTORY)) {
			scr->SetBase(IE_GOLD,scr->GetBase(IE_GOLD)-money);
		} else {
			// drop it at my feet
			map->AddItemToLocation(Sender->Pos, item);
			if (((Actor *)Sender)->InParty) displaymsg->DisplayConstantString(STR_INVFULL_ITEMDROP, DMC_BG2XPGREEN);
			Sender->ReleaseCurrentAction();
			return;
		}
	}

	displaymsg->DisplayConstantString(STR_PICKPOCKET_DONE, DMC_WHITE);
	DisplayStringCore(snd, VB_PP_SUCC, DS_CONSOLE|DS_CONST );
	Sender->ReleaseCurrentAction();
}

void GameScript::TakeItemList(Scriptable * Sender, Action* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1] );
	if (!tar || tar->Type!=ST_ACTOR) {
		return;
	}
	AutoTable tab(parameters->string0Parameter);
	if (!tab) {
		return;
	}

	int rows = tab->GetRowCount();
	for (int i=0;i<rows;i++) {
		MoveItemCore(tar, Sender, tab->QueryField(i,0), 0, IE_INV_ITEM_UNSTEALABLE);
	}
}

void GameScript::TakeItemListParty(Scriptable * Sender, Action* parameters)
{
	AutoTable tab(parameters->string0Parameter);
	if (!tab) {
		return;
	}
	Game *game = core->GetGame();
	int rows = tab->GetRowCount();
	for (int i=0;i<rows;i++) {
		int j = game->GetPartySize(false);
		while (j--) {
			Actor *tar = game->GetPC(j, false);
			MoveItemCore(tar, Sender, tab->QueryField(i,0), 0, IE_INV_ITEM_UNSTEALABLE);
		}
	}
}

void GameScript::TakeItemListPartyNum(Scriptable * Sender, Action* parameters)
{
	AutoTable tab(parameters->string0Parameter);
	if (!tab) {
		return;
	}
	Game *game = core->GetGame();
	int rows = tab->GetRowCount();
	int count = parameters->int0Parameter;
	for (int i=0;i<rows;i++) {
		int j = game->GetPartySize(false);
		while (j--) {
			Actor *tar = game->GetPC(j, false);
			int res=MoveItemCore(tar, Sender, tab->QueryField(i,0), 0, IE_INV_ITEM_UNSTEALABLE);
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
		sprintf(params->string0Parameter, "%s", tab->QueryField(9999,9999));
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
void GameScript::EndCredits(Scriptable* /*Sender*/, Action* /*parameters*/)
{
	core->PlayMovie("credits");
}

//easily hardcoded end sequence
void GameScript::ExpansionEndCredits(Scriptable* /*Sender*/, Action* /*parameters*/)
{
	core->PlayMovie("ecredit");
}

//always quits game, but based on game it can play end animation, or display
//death text, etc
//this covers:
//QuitGame (play two of 3 movies in PST, display death screen with strref)
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
	if (Sender->Type!=ST_ACTOR) {
		return;
	}
	Actor *actor = (Actor *) Sender;
	actor->ClearPath();
}

void GameScript::ApplyDamage(Scriptable* Sender, Action* parameters)
{
	Actor *damagee;
	Actor *damager;
	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1] );
	if (!tar || tar->Type!=ST_ACTOR) {
		return;
	}
	damagee = (Actor *) tar;
	if (Sender->Type==ST_ACTOR) {
		damager=(Actor *) Sender;
	} else {
		damager=damagee;
	}
	damagee->Damage(parameters->int0Parameter, parameters->int1Parameter, damager);
}

void GameScript::ApplyDamagePercent(Scriptable* Sender, Action* parameters)
{
	Actor *damagee;
	Actor *damager;
	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1] );
	if (!tar || tar->Type!=ST_ACTOR) {
		return;
	}
	damagee = (Actor *) tar;
	if (Sender->Type==ST_ACTOR) {
		damager=(Actor *) Sender;
	} else {
		damager=damagee;
	}
	//this, if the percent is calculated from the current hp
	damagee->Damage((parameters->int0Parameter*damagee->Modified[IE_HITPOINTS])/100, parameters->int1Parameter, damager);
	//this, if the percent is calculated from the max hp
	//damagee->Damage(parameters->int0Parameter, parameters->int1Parameter, damager, MOD_PERCENT);
}

void GameScript::Damage(Scriptable* Sender, Action* parameters)
{
	Actor *damagee;
	Actor *damager;
	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1] );
	if (!tar || tar->Type!=ST_ACTOR) {
		return;
	}
	damagee = (Actor *) tar;
	if (Sender->Type==ST_ACTOR) {
		damager=(Actor *) Sender;
	} else {
		damager=damagee;
	}
	int damage = damager->LuckyRoll( (parameters->int1Parameter>>12)&15, (parameters->int1Parameter>>4)&255, parameters->int1Parameter&15, LR_DAMAGELUCK, damagee);
	int type=MOD_ADDITIVE;
	switch(parameters->int0Parameter) {
	case 2: //raise
		damage=-damage;
		break;
	case 3: //set
		type=MOD_ABSOLUTE;
		break;
	case 4: //
		type=MOD_PERCENT;
		break;
	}
	//damagetype seems to be always 0
	damagee->Damage( damage, 0, damager, type);
}
/*
void GameScript::SetHomeLocation(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1] );
	if (!tar || tar->Type!=ST_ACTOR) {
		return;
	}
	Movable *movable = (Movable *) tar; //not actor, though it is the only moveable
	movable->Destination = parameters->pointParameter;
	//no movement should be started here, i think
}
*/

void GameScript::SetMasterArea(Scriptable* /*Sender*/, Action* parameters)
{
	core->GetGame()->SetMasterArea(parameters->string0Parameter);
}

void GameScript::Berserk(Scriptable* Sender, Action* /*parameters*/)
{
	if (Sender->Type!=ST_ACTOR) {
		return;
	}
	Actor *act = (Actor *) Sender;
	act->SetBaseBit(IE_STATE_ID, STATE_BERSERK, true);
	act->Panic(NULL, PANIC_BERSERK);
}

void GameScript::Panic(Scriptable* Sender, Action* /*parameters*/)
{
	if (Sender->Type!=ST_ACTOR) {
		return;
	}
	Actor *act = (Actor *) Sender;
	act->Panic(NULL, PANIC_RANDOMWALK);
}

/* as of now: removes panic and berserk */
void GameScript::Calm(Scriptable* Sender, Action* /*parameters*/)
{
	if (Sender->Type!=ST_ACTOR) {
		return;
	}
	Actor *act = (Actor *) Sender;
	act->SetBaseBit(IE_STATE_ID, STATE_BERSERK|STATE_PANIC, false);
}

void GameScript::RevealAreaOnMap(Scriptable* /*Sender*/, Action* parameters)
{
	WorldMap *worldmap = core->GetWorldMap();
	if (!worldmap) {
		error("GameScript", "Can't find worldmap!\n");
	}
	// WMP_ENTRY_ADJACENT because otherwise revealed bg2 areas are unreachable from city gates
	worldmap->SetAreaStatus(parameters->string0Parameter, WMP_ENTRY_VISIBLE|WMP_ENTRY_ADJACENT, BM_OR);
	displaymsg->DisplayConstantString(STR_WORLDMAPCHANGE, DMC_BG2XPGREEN);
}

void GameScript::HideAreaOnMap( Scriptable* /*Sender*/, Action* parameters)
{
	WorldMap *worldmap = core->GetWorldMap();
	if (!worldmap) {
		error("GameScript", "Can't find worldmap!\n");
	}
	// WMP_ENTRY_ADJACENT because otherwise revealed bg2 areas are unreachable from city gates
	worldmap->SetAreaStatus(parameters->string0Parameter, WMP_ENTRY_VISIBLE|WMP_ENTRY_ADJACENT, BM_NAND);
}

void GameScript::SendTrigger(Scriptable* Sender, Action* parameters)
{
	Scriptable *tar = GetActorFromObject( Sender, parameters->objects[1], GA_NO_DEAD );
	if (!tar) {
		return;
	}
	tar->AddTrigger(TriggerEntry(trigger_trigger, parameters->int0Parameter));
}

void GameScript::Shout( Scriptable* Sender, Action* parameters)
{
	if (Sender->Type!=ST_ACTOR) {
		return;
	}
	//according to IESDP silenced creatures cannot use shout
	Actor *actor = (Actor *) Sender;
	if (actor->GetStat( IE_STATE_ID) & STATE_SILENCED) {
		return;
	}
	Map *map=Sender->GetCurrentArea();
	//max. shouting distance, please adjust it if you know better
	map->Shout(actor, parameters->int0Parameter, MAX_TRAVELING_DISTANCE);
}

void GameScript::GlobalShout( Scriptable* Sender, Action* parameters)
{
	if (Sender->Type!=ST_ACTOR) {
		return;
	}
	//according to IESDP silenced creatures cannot use shout
	Actor *actor = (Actor *) Sender;
	if (actor->GetStat( IE_STATE_ID) & STATE_SILENCED) {
		return;
	}
	Map *map=Sender->GetCurrentArea();
	// 0 means unlimited shout distance
	map->Shout(actor, parameters->int0Parameter, 0);
}

void GameScript::Help( Scriptable* Sender, Action* /*parameters*/)
{
	if (Sender->Type!=ST_ACTOR) {
		return;
	}
	Map *map=Sender->GetCurrentArea();
	map->Shout((Actor *) Sender, 0, 40);
}

void GameScript::GiveOrder(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1] );
	if (tar) {
		tar->AddTrigger(TriggerEntry(trigger_receivedorder, Sender->GetGlobalID(), parameters->int0Parameter));
	}
}

void GameScript::AddMapnote( Scriptable* Sender, Action* parameters)
{
	Map *map=Sender->GetCurrentArea();
	char *str = core->GetString( parameters->int0Parameter, 0);
	map->AddMapNote(parameters->pointParameter, parameters->int1Parameter, str, parameters->int0Parameter);
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
	Scriptable* tar;
	tar = GetStoredActorFromObject( Sender, parameters->objects[1], GA_NO_DEAD );
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
	Scriptable* tar;
	tar = GetStoredActorFromObject( Sender, parameters->objects[1], GA_NO_DEAD );
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
	Scriptable* tar;
	tar = GetStoredActorFromObject( Sender, parameters->objects[1], GA_NO_DEAD );
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
	Scriptable* tar;
	tar = GetStoredActorFromObject( Sender, parameters->objects[1], GA_NO_DEAD );
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
	Scriptable* tar;
	tar = GetStoredActorFromObject( Sender, parameters->objects[1], GA_NO_DEAD );

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
	Scriptable* scr = GetActorFromObject( Sender, parameters->objects[1], GA_NO_DEAD );
	if (!scr || scr->Type != ST_ACTOR) {
		return;
	}
	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[2], GA_NO_DEAD );
	if (!tar || (tar->Type != ST_ACTOR && tar->Type !=ST_DOOR && tar->Type !=ST_CONTAINER) ) {
		return;
	}
	//this is a hack, we use a gui variable for our own hideous reasons?
	if (tar->Type==ST_ACTOR) {
		GameControl *gc = core->GetGameControl();
		if (gc) {
			//saving the target object ID from the gui variable
			char Tmp[40];
			strncpy(Tmp,"NIDSpecial3()",sizeof(Tmp) );
			scr->AddAction( GenerateActionDirect(Tmp, (Actor *) tar) );
		}
	} else {
		char Tmp[80];
		snprintf(Tmp, sizeof(Tmp), "BashDoor(%s)", tar->GetScriptName());
		scr->AddAction ( GenerateAction(Tmp) );
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

	Scriptable* tar;
	tar = GetStoredActorFromObject( Sender, parameters->objects[1], GA_NO_DEAD );
	if (!tar || (tar->Type != ST_ACTOR && tar->Type !=ST_DOOR && tar->Type !=ST_CONTAINER) ) {
		Sender->ReleaseCurrentAction();
		return;
	}

	//actor is already incapable of attack
	if (Sender->GetInternalFlag()&IF_STOPATTACK) {
		Sender->ReleaseCurrentAction();
		return;
	}

	AttackCore(Sender, tar, 0);

	if (Sender->CurrentActionState) {
		Sender->CurrentActionState--;
	} else {
		Sender->ReleaseCurrentAction();
	}
}

void GameScript::Explore( Scriptable* Sender, Action* /*parameters*/)
{
	Sender->GetCurrentArea( )->Explore(-1);
}

void GameScript::UndoExplore( Scriptable* Sender, Action* /*parameters*/)
{
	Sender->GetCurrentArea( )->Explore(0);
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
	core->SetCurrentStore( parameters->string0Parameter, Sender->GetGlobalID());
	core->SetEventFlag(EF_OPENSTORE);
	//sorry, i have absolutely no idea when i should do this :)
	Sender->ReleaseCurrentAction();
}

//The integer parameter is a GemRB extension, if set to 1, the player
//gains experience for learning the spell
void GameScript::AddSpecialAbility( Scriptable* Sender, Action* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		return;
	}
	Actor *actor = (Actor *) Sender;
	actor->LearnSpell (parameters->string0Parameter, parameters->int0Parameter|LS_MEMO|LS_LEARN);
	core->SetEventFlag(EF_ACTION);
}

//actually this just depletes a spell, doesn't remove it from the book
//GemRB extension: the first/second int parameter can also make it removed
//from the spell memorization schedule (also from the spellbook)
void GameScript::RemoveSpell( Scriptable* Sender, Action* parameters)
{
	ieResRef spellres;
	int type;

	if (Sender->Type!=ST_ACTOR) {
		return;
	}
	if (!ResolveSpellName( spellres, parameters) ) {
		return;
	}
	Actor *actor = (Actor *) Sender;
	if (parameters->string0Parameter[0]) {
		//the spell resref is in the string parameter
		type = parameters->int0Parameter;
	} else {
		//the spell number is in the int0 parameter
		type = parameters->int1Parameter;
	}
	if (type==2) {
	//remove spell from both book and memorization
		actor->spellbook.RemoveSpell(spellres);
		return;
	}
	//type == 1 remove spell only from memorization
	//type == 0 original behaviour: deplete only
	actor->spellbook.UnmemorizeSpell(spellres, type);
}

void GameScript::SetScriptName( Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1] );
	if (!tar || tar->Type!=ST_ACTOR) {
		return;
	}
	tar->SetScriptName(parameters->string0Parameter);
}

//iwd2
//advance time with a constant
//This is in seconds according to IESDP
void GameScript::AdvanceTime(Scriptable* /*Sender*/, Action* parameters)
{
	core->GetGame()->AdvanceTime(parameters->int0Parameter*1000/AI_UPDATE_TIME);
}

//advance at least one day, then stop at next day/dusk/night/morning
//oops, not TimeODay is used but Time (this means we got hours)
//i'm not sure if we should add a whole day either, needs more research
void GameScript::DayNight(Scriptable* /*Sender*/, Action* parameters)
{
	// first, calculate the current number of hours.
	int padding = ((core->GetGame()->GameTime / AI_UPDATE_TIME) % 7200) / 300;
	// then, calculate the offset (in hours) required to take us to the desired hour.
	padding = (24 + parameters->int0Parameter - padding) % 24;
	// then, advance one day (7200), plus the desired number of hours.
	core->GetGame()->AdvanceTime(AI_UPDATE_TIME*(7200 + padding*300));
}

//implement pst style parameters:
//suggested dream - unused
//if suggested dream is 0, then area flags determine the 'movie'
//hp - number of hps healed
//renting - crashes pst, we simply ignore it
void GameScript::RestParty(Scriptable* Sender, Action* parameters)
{
	Game *game = core->GetGame();
	game->RestParty(REST_NOAREA|REST_NOMOVE|REST_NOCRITTER|REST_NOSCATTER, parameters->int0Parameter, parameters->int1Parameter);
	Sender->ReleaseCurrentAction();
}

//doesn't advance game time, just removes fatigue & refreshes spells of target
//this is a non-blocking action
static EffectRef fx_fatigue_ref = { "FatigueModifier", -1 };

void GameScript::Rest(Scriptable* Sender, Action* /*parameters*/)
{
	if (Sender->Type!=ST_ACTOR) {
		return;
	}
	Actor *actor = (Actor *) Sender;
	actor->spellbook.ChargeAllSpells();
	actor->fxqueue.RemoveAllEffects(fx_fatigue_ref);
	actor->SetBase(IE_FATIGUE,0);
}

//doesn't advance game time, just removes fatigue
void GameScript::RestNoSpells(Scriptable* Sender, Action* /*parameters*/)
{
	if (Sender->Type!=ST_ACTOR) {
		return;
	}
	Actor *actor = (Actor *) Sender;
	actor->fxqueue.RemoveAllEffects(fx_fatigue_ref);
	actor->SetBase(IE_FATIGUE,0);
}

//this most likely advances time and heals whole party
void GameScript::RestUntilHealed(Scriptable* Sender, Action* /*parameters*/)
{
	core->GetGame()->RestParty(REST_NOSCATTER, 0, 0);
	Sender->ReleaseCurrentAction();
}

//iwd2
//removes all delayed/duration/semi permanent effects (like a ctrl-r)
void GameScript::ClearPartyEffects(Scriptable* /*Sender*/, Action* /*parameters*/)
{
	Game *game = core->GetGame();
	int i = game->GetPartySize(false);
	while (i--) {
		Actor *tar = game->GetPC(i, false);
		tar->fxqueue.RemoveExpiredEffects(0xffffffff);
	}
}

//iwd2 removes effects from a single sprite
void GameScript::ClearSpriteEffects(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1] );
	if (!tar || tar->Type!=ST_ACTOR) {
		return;
	}
	Actor *actor = (Actor *) tar;
	actor->fxqueue.RemoveExpiredEffects(0xffffffff);
}

//IWD2 special, can mark only actors, hope it is enough
void GameScript::MarkObject(Scriptable* Sender, Action* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		return;
	}
	//unsure, could mark dead objects?
	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1], GA_NO_DEAD );
	if (!tar || tar->Type!=ST_ACTOR) {
		return;
	}
	Actor *actor = (Actor *) Sender;
	actor->LastMarked = tar->GetGlobalID();
	//if this doesn't modify LastSeen, then remove this line
	actor->LastSeen = actor->LastMarked;
}

void GameScript::MarkSpellAndObject(Scriptable* Sender, Action* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		return;
	}
	Actor *me = (Actor *) Sender;
	if (me->LastMarkedSpell) {
		return;
	}

	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1]);
	Actor *actor = NULL;
	if (tar->Type == ST_ACTOR) {
		actor = (Actor *) tar;
	}

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
	int len = strlen(parameters->string0Parameter);
	//
	if (len&3) {
		return;
	}
	len/=4;
	int max = len;
	int pos;
	if (flags & MSO_RANDOM_SPELL) {
		pos = core->Roll(1,len,0);
	} else {
		pos = 0;
	}
	while(len--) {
		char spl[5];

		memcpy(spl, parameters->string0Parameter+pos*4, 4);
		spl[4]=0;
		int splnum = atoi(spl);

		if (!(flags & MSO_IGNORE_HAVE) && !me->spellbook.HaveSpell(splnum, 0) ) {
			goto end_mso_loop;
		}
		int range;
		if ((flags & MSO_IGNORE_RANGE) || !actor) {
			range = 0;
		} else {
			range = Distance(me, actor);
		}
		if (!(flags & MSO_IGNORE_INVALID) && actor->InvalidSpellTarget(splnum, me, range)) {
			goto end_mso_loop;
		}
		//mark spell and target
		me->LastMarkedSpell = splnum;
		me->LastMarked = actor->GetGlobalID();
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
	if (Sender->Type != ST_ACTOR) {
		return;
	}
	Actor *actor = (Actor *) Sender;
	actor->LastMarkedSpell = parameters->int0Parameter;
}

void GameScript::SetMarkedSpell(Scriptable* Sender, Action* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		return;
	}
	Actor *actor = (Actor *) Sender;
	if (parameters->int0Parameter) {
		if (actor->LastMarkedSpell) {
			return;
		}
		if (!actor->spellbook.HaveSpell(parameters->int0Parameter, 0) ) {
			return;
		}
	}

	//TODO: check if spell exists (not really important)
	actor->LastMarkedSpell = parameters->int0Parameter;
	return;
}

void GameScript::SetDialogueRange(Scriptable* Sender, Action* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		return;
	}
	Actor *actor = (Actor *) Sender;
	actor->SetBase( IE_DIALOGRANGE, parameters->int0Parameter );
}

void GameScript::SetGlobalTint(Scriptable* /*Sender*/, Action* parameters)
{
	core->GetVideoDriver()->SetFadeColor(parameters->int0Parameter, parameters->int1Parameter, parameters->int2Parameter);
}

void GameScript::SetArmourLevel(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1] );
	if (!tar || tar->Type!=ST_ACTOR) {
		return;
	}
	Actor *actor = (Actor *) Sender;
	actor->SetBase( IE_ARMOR_TYPE, parameters->int0Parameter );
}

void GameScript::RandomWalk(Scriptable* Sender, Action* /*parameters*/)
{
	if (Sender->Type != ST_ACTOR) {
		Sender->ReleaseCurrentAction();
		return;
	}
	Actor* actor = ( Actor* ) Sender;
	actor->RandomWalk( true, false );
	Sender->ReleaseCurrentAction();
}

void GameScript::RandomRun(Scriptable* Sender, Action* /*parameters*/)
{
	if (Sender->Type != ST_ACTOR) {
		Sender->ReleaseCurrentAction();
		return;
	}
	Actor* actor = ( Actor* ) Sender;
	actor->RandomWalk( true, true );
	Sender->ReleaseCurrentAction();
}

void GameScript::RandomWalkContinuous(Scriptable* Sender, Action* /*parameters*/)
{
	if (Sender->Type != ST_ACTOR) {
		Sender->ReleaseCurrentAction();
		return;
	}
	Actor* actor = ( Actor* ) Sender;
	actor->RandomWalk( false, false );
}

void GameScript::RandomFly(Scriptable* Sender, Action* /*parameters*/)
{
	if (Sender->Type != ST_ACTOR) {
		Sender->ReleaseCurrentAction();
		return;
	}
	Actor* actor = ( Actor* ) Sender;
	int x = rand()&31;
	if (x<10) {
		actor->SetOrientation(actor->GetOrientation()-1, false);
	} else if (x>20) {
		actor->SetOrientation(actor->GetOrientation()+1, false);
	}
	//fly in this direction for 5 steps
	actor->MoveLine(5, GL_PASS, actor->GetOrientation() );
	//readding the action to the end of the queue
	//Sender->AddAction( parameters );
	//Sender->ReleaseCurrentAction();
}

//UseContainer uses the predefined target (like Nidspecial1 dialog hack)
void GameScript::UseContainer(Scriptable* Sender, Action* /*parameters*/)
{
	if (Sender->Type != ST_ACTOR) {
		Sender->ReleaseCurrentAction();
		return;
	}

	if (core->InCutSceneMode()) {
		//cannot use container in dialog or cutscene
		Sender->ReleaseCurrentAction();
		return;
	}

	Actor *actor = (Actor *)Sender;
	Container *container = core->GetCurrentContainer();
	if (!container) {
		printMessage("GameScript", "No container selected!\n", YELLOW);
		Sender->ReleaseCurrentAction();
		return;
	}

	ieDword distance = PersonalDistance(Sender, container);
	ieDword needed = MAX_OPERATING_DISTANCE;
	if (container->Type==IE_CONTAINER_PILE) {
		needed = 0; // less than a search square (width)
	}
	if (distance<=needed)
	{
		//check if the container is unlocked
		if (!container->TryUnlock(actor)) {
			//playsound can't open container
			//display string, etc
			displaymsg->DisplayConstantString(STR_CONTLOCKED, DMC_LIGHTGREY, container);
			Sender->ReleaseCurrentAction();
			return;
		}
		Actor *actor = (Actor *)Sender;
		actor->SetModal(MS_NONE);
		container->TriggerTrap(0, actor->GetGlobalID());
		core->SetCurrentContainer(actor, container, true);
		Sender->ReleaseCurrentAction();
		return;
	}
	MoveNearerTo(Sender, container, needed);
}

//call the usecontainer action in target (not used)
void GameScript::ForceUseContainer(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1] );
	if (!tar || tar->Type != ST_ACTOR) {
		Sender->ReleaseCurrentAction(); //why blocking???
		return;
	}
	char Tmp[256];
	sprintf( Tmp, "UseContainer()");
	Action *newaction = GenerateAction(Tmp);
	tar->AddActionInFront(newaction);
	Sender->ReleaseCurrentAction(); //why blocking???
}

//these actions directly manipulate a game variable (as the original engine)
void GameScript::SetMazeEasier(Scriptable* Sender, Action* /*parameters*/)
{
	int value = CheckVariable( Sender, "MAZEDIFFICULTY","GLOBAL");
	if (value>0) {
		SetVariable(Sender, "MAZEDIFFICULTY", "GLOBAL", value-1);
	}
}

void GameScript::SetMazeHarder(Scriptable* Sender, Action* /*parameters*/)
{
	int value = CheckVariable( Sender, "MAZEDIFFICULTY","GLOBAL");
	if (value<2) {
		SetVariable(Sender, "MAZEDIFFICULTY", "GLOBAL", value+1);
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
		SetVariable(Sender, "EnginInMaze", "GLOBAL", 0);
		//this works only because the engine room exit depends only on the EnginInMaze variable
		ScriptEngine *sE = core->GetGUIScriptEngine();
		sE->RunFunction("Maze", "CustomizeArea");
	}
}

void GameScript::StartRainNow(Scriptable* /*Sender*/, Action* /*parameters*/)
{
	core->GetGame()->StartRainOrSnow( false, WB_RAIN|WB_LIGHTNING);
}

void GameScript::Weather(Scriptable* /*Sender*/, Action* parameters)
{
	Game *game = core->GetGame();
	switch(parameters->int0Parameter & WB_FOG) {
		case WB_NORMAL:
			game->StartRainOrSnow( false, 0);
			break;
		case WB_RAIN:
			game->StartRainOrSnow( true, WB_RAIN|WB_LIGHTNING);
			break;
		case WB_SNOW:
			game->StartRainOrSnow( true, WB_SNOW);
			break;
		case WB_FOG:
			game->StartRainOrSnow( true, WB_FOG);
			break;
	}
}

void GameScript::CopyGroundPilesTo(Scriptable* Sender, Action* parameters)
{
	Map *map = Sender->GetCurrentArea();
	Map *othermap = core->GetGame()->GetMap( parameters->string0Parameter, false );
	if (!othermap) {
		return;
	}
	map->CopyGroundPiles( othermap, parameters->pointParameter );
}

//iwd specific
void GameScript::PlayBardSong(Scriptable* Sender, Action* /*parameters*/)
{
	if (Sender->Type!=ST_ACTOR) {
		return;
	}
	//actually this one must use int0Parameter to set a bardsong
	Actor *actor = (Actor *) Sender;
	actor->SetModal( MS_BATTLESONG);
}

void GameScript::BattleSong(Scriptable* Sender, Action* /*parameters*/)
{
	if (Sender->Type!=ST_ACTOR) {
		return;
	}
	Actor *actor = (Actor *) Sender;
	actor->SetModal( MS_BATTLESONG);
}

void GameScript::FindTraps(Scriptable* Sender, Action* /*parameters*/)
{
	if (Sender->Type!=ST_ACTOR) {
		return;
	}
	Actor *actor = (Actor *) Sender;
	actor->SetModal( MS_DETECTTRAPS);
}

void GameScript::Hide(Scriptable* Sender, Action* /*parameters*/)
{
	if (Sender->Type!=ST_ACTOR) {
		return;
	}
	Actor *actor = (Actor *) Sender;

	if (actor->TryToHide()) {
		actor->SetModal(MS_STEALTH);
	}
	//TODO: expiry isn't instant (skill based transition?)

}

void GameScript::Turn(Scriptable* Sender, Action* /*parameters*/)
{
	if (Sender->Type!=ST_ACTOR) {
		return;
	}
	Actor *actor = (Actor *) Sender;

	if (actor->Modified[IE_DISABLEDBUTTON] & (1<<ACT_TURN)) {
		return;
	}

	int skill = actor->GetStat(IE_TURNUNDEADLEVEL);
	if (skill < 1) return;

	actor->SetModal(MS_TURNUNDEAD);

}

void GameScript::TurnAMT(Scriptable* Sender, Action* parameters)
{
	if (Sender->Type!=ST_ACTOR) {
		Sender->ReleaseCurrentAction();
		return;
	}
	Actor *actor = (Actor *) Sender;
	actor->SetOrientation(actor->GetOrientation()+parameters->int0Parameter, true);
	actor->SetWait( 1 );
	Sender->ReleaseCurrentAction(); // todo, blocking?
}

void GameScript::RandomTurn(Scriptable* Sender, Action* /*parameters*/)
{
	if (Sender->Type!=ST_ACTOR) {
		Sender->ReleaseCurrentAction();
		return;
	}
	Actor *actor = (Actor *) Sender;
	actor->SetOrientation(rand() % MAX_ORIENT, true);
	actor->SetWait( 1 );
	Sender->ReleaseCurrentAction(); // todo, blocking?
}

void GameScript::AttachTransitionToDoor(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1] );
	if (!tar || tar->Type != ST_DOOR) {
		return;
	}
	Door* door = ( Door* ) tar;
	strnspccpy(door->LinkedInfo, parameters->string0Parameter, 32);
}

/*getting a handle of a temporary actor resource to copy its selected attributes*/
void GameScript::ChangeAnimation(Scriptable* Sender, Action* parameters)
{
	if (Sender->Type!=ST_ACTOR) {
		return;
	}
	ChangeAnimationCore((Actor *) Sender, parameters->string0Parameter,1);
}

void GameScript::ChangeAnimationNoEffect(Scriptable* Sender, Action* parameters)
{
	if (Sender->Type!=ST_ACTOR) {
		return;
	}
	ChangeAnimationCore((Actor *) Sender, parameters->string0Parameter,0);
}

void GameScript::Polymorph(Scriptable* Sender, Action* parameters)
{
	if (Sender->Type!=ST_ACTOR) {
		return;
	}
	Actor *act = (Actor *) Sender;
	act->SetBase(IE_ANIMATION_ID, parameters->int0Parameter);
}

void GameScript::PolymorphCopy(Scriptable* Sender, Action* parameters)
{
	if (Sender->Type!=ST_ACTOR) {
		return;
	}
	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1] );
	if (!tar || tar->Type!=ST_ACTOR) {
		return;
	}
	PolymorphCopyCore((Actor *) tar, (Actor *) Sender, false);
}

/* according to IESDP this only copies the animation ID */
void GameScript::PolymorphCopyBase(Scriptable* Sender, Action* parameters)
{
	if (Sender->Type!=ST_ACTOR) {
		return;
	}
	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1] );
	if (!tar || tar->Type!=ST_ACTOR) {
		return;
	}
	Actor *act = (Actor *) Sender;
	Actor *actor = (Actor *) tar;
	act->SetBase(IE_ANIMATION_ID, actor->GetBase(IE_ANIMATION_ID) );
}

void GameScript::ExportParty(Scriptable* /*Sender*/, Action* parameters)
{
	char FileName[_MAX_PATH];

	Game *game = core->GetGame();
	int i = game->GetPartySize(false);
	while (i--) {
		Actor *actor = game->GetPC(i, false);
		snprintf(FileName,_MAX_PATH,"%s%d",parameters->string0Parameter,i+1);
		core->WriteCharacter(FileName, actor);
	}
	displaymsg->DisplayConstantString(STR_EXPORTED, DMC_BG2XPGREEN);
}

void GameScript::SaveGame(Scriptable* /*Sender*/, Action* parameters)
{
	if (core->HasFeature(GF_STRREF_SAVEGAME)) {
		const char *basename = "Auto-Save";
		AutoTable tab("savegame");
		if (tab) {
			basename = tab->QueryDefault();
		}
		char * str = core->GetString( parameters->int0Parameter, IE_STR_STRREFOFF);
		char FolderName[_MAX_PATH];
		snprintf (FolderName, sizeof(FolderName), "%s - %s", basename, str);
		core->FreeString( str );

		core->GetSaveGameIterator()->CreateSaveGame(core->GetSaveGameIterator()->GetSaveGame(FolderName), FolderName);
	} else {
		core->GetSaveGameIterator()->CreateSaveGame(parameters->int0Parameter);
	}
}

/*EscapeAreaMove(S:Area*,I:X*,I:Y*,I:Face*)*/
void GameScript::EscapeArea(Scriptable* Sender, Action* parameters)
{
	if (InDebug&ID_ACTIONS) {
		printMessage("Actions", "EscapeArea/EscapeAreaMove\n", LIGHT_WHITE);
	}
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

	if (parameters->string0Parameter[0]) {
		Point q((short) parameters->int0Parameter, (short) parameters->int1Parameter);
		EscapeAreaCore( Sender, p, parameters->string0Parameter, q, 0, parameters->int2Parameter );
	} else {
		EscapeAreaCore( Sender, p, parameters->string0Parameter, p, EA_DESTROY, parameters->int0Parameter );
	}
	//EscapeAreaCore will do its ReleaseCurrentAction
	//Sender->ReleaseCurrentAction();
}

void GameScript::EscapeAreaNoSee(Scriptable* Sender, Action* parameters)
{
	if (InDebug&ID_ACTIONS) {
		printMessage("Actions", "EscapeAreaNoSee\n", LIGHT_WHITE);
	}
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

	if (parameters->string0Parameter[0]) {
		Point q((short) parameters->int0Parameter, (short) parameters->int1Parameter);
		EscapeAreaCore( Sender, p, parameters->string0Parameter, q, 0, parameters->int2Parameter );
	} else {
		EscapeAreaCore( Sender, p, parameters->string0Parameter, p, EA_DESTROY|EA_NOSEE, parameters->int0Parameter );
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
	EscapeAreaCore( Sender, p, parameters->string0Parameter, p, EA_DESTROY, parameters->int0Parameter );
}

/*EscapeAreaObjectMove(S:Area*,I:X*,I:Y*,I:Face*)*/
void GameScript::EscapeAreaObject(Scriptable* Sender, Action* parameters)
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

	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1] );
	if (!tar) {
		Sender->ReleaseCurrentAction();
		return;
	}
	Point p = tar->Pos;
	if (parameters->string0Parameter[0]) {
		Point q((short) parameters->int0Parameter, (short) parameters->int1Parameter);
		EscapeAreaCore( Sender, p, parameters->string0Parameter, q, 0, parameters->int2Parameter );
	} else {
		EscapeAreaCore( Sender, p, 0, p, EA_DESTROY, parameters->int0Parameter );
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
	Map *map = Sender->GetCurrentArea();
	if (!map) {
		Sender->ReleaseCurrentAction();
		return;
	}

	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1] );
	if (!tar) {
		Sender->ReleaseCurrentAction();
		return;
	}
	Point p = tar->Pos;
	Sender->SetWait(parameters->int0Parameter);
	if (parameters->string0Parameter[0]) {
		Point q((short) parameters->int0Parameter, (short) parameters->int1Parameter);
		EscapeAreaCore( Sender, p, parameters->string0Parameter, q, 0, parameters->int2Parameter );
	} else {
		EscapeAreaCore( Sender, p, 0, p, EA_DESTROY|EA_NOSEE, parameters->int0Parameter );
	}
	//EscapeAreaCore will do its ReleaseCurrentAction
}

//takes first fitting item from container at feet, doesn't seem to be working in the original engines
void GameScript::PickUpItem(Scriptable* Sender, Action* parameters)
{
	if (Sender->Type!=ST_ACTOR) {
		return;
	}
	Actor *scr = (Actor *) Sender;
	Map *map = scr->GetCurrentArea();
	Container *c = map->GetPile(scr->Pos);
	if (!c) { //this shouldn't happen, but lets prepare for the worst
		return;
	}

	//the following part is coming from GUISCript.cpp with trivial changes
	int Slot = c->inventory.FindItem(parameters->string0Parameter, 0);
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
		goto item_is_gold;
	}
	res = scr->inventory.AddSlotItem(item, SLOT_ONLYINVENTORY);
	if (res !=ASI_SUCCESS) { //putting it back
		c->AddItem(item);
	}
	return;
item_is_gold: //we take gold!
	if (scr->InParty) {
		core->GetGame()->PartyGold += res;
		//if you want message here then use
		//core->GetGame()->AddGold(res);
	} else {
		scr->SetBase( IE_GOLD, scr->GetBase(IE_GOLD) + res );
	}
	delete item;
}

void GameScript::ChangeStoreMarkup(Scriptable* /*Sender*/, Action* parameters)
{
	bool has_current = false;
	ieResRef current;
	ieDword owner;

	Store *store = core->GetCurrentStore();
	if (!store) {
		store = core->SetCurrentStore(parameters->string0Parameter, 0);
	} else {
		if (strnicmp(store->Name, parameters->string0Parameter, sizeof(ieResRef)-1) ) {
			//not the current store, we need some dirty hack
			has_current = true;
			strnlwrcpy(current, store->Name, sizeof(ieResRef)-1);
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
	WorldMap *wmap = core->GetWorldMap(parameters->string0Parameter);
	if (!wmap) {
		//no such starting area
		return;
	}
	WMPAreaLink *link = wmap->GetLink(parameters->string0Parameter, parameters->string1Parameter);
	if (!link) {
		return;
	}
	link->EncounterChance = parameters->int0Parameter;
}

void GameScript::SpawnPtActivate(Scriptable* Sender, Action* parameters)
{
	if (parameters->objects[1]) {
		Map *map = Sender->GetCurrentArea();
		Spawn *spawn = map->GetSpawn(parameters->objects[1]->objectName);
		if (spawn) {
			spawn->Enabled = 1;
		}
	}
}

void GameScript::SpawnPtDeactivate(Scriptable* Sender, Action* parameters)
{
	if (parameters->objects[1]) {
		Map *map = Sender->GetCurrentArea();
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
	ieResRef spellres;

	if (!ResolveSpellName( spellres, parameters) ) {
		return;
	}

	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1] );
	if (!tar) {
		return;
	}
	if (tar->Type==ST_ACTOR) {
		//apply spell on target
/*
		Actor *owner;

		if (Sender->Type==ST_ACTOR) {
			owner = (Actor *) Sender;
		} else {
			owner = (Actor *) tar;
		}
*/
		//core->ApplySpell(spellres, (Actor *) tar, owner, parameters->int1Parameter);
		core->ApplySpell(spellres, (Actor *) tar, Sender, parameters->int1Parameter);
	} else {
		//no idea about this one
/*
		Actor *owner;

		if (Sender->Type==ST_ACTOR) {
			owner = (Actor *) Sender;
		} else {
			owner = NULL;
		}
*/
		//apply spell on point
		Point d;
		GetPositionFromScriptable(tar, d, false);
		//core->ApplySpellPoint(spellres, tar->GetCurrentArea(), d, owner, parameters->int1Parameter);
		core->ApplySpellPoint(spellres, tar->GetCurrentArea(), d, Sender, parameters->int1Parameter);
	}
}

void GameScript::ApplySpellPoint(Scriptable* Sender, Action* parameters)
{
	ieResRef spellres;
	Actor *owner;

	if (!ResolveSpellName( spellres, parameters) ) {
		return;
	}

	if (Sender->Type==ST_ACTOR) {
		owner = (Actor *) Sender;
	} else {
		owner = NULL;
	}
	core->ApplySpellPoint(spellres, Sender->GetCurrentArea(), parameters->pointParameter, owner, parameters->int1Parameter);
}

//this is a gemrb extension
//sets a variable to the stat value
void GameScript::GetStat(Scriptable* Sender, Action* parameters)
{
	ieDword value;

	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1] );
	if (!tar || tar->Type!=ST_ACTOR) {
		value = 0;
	} else {
		Actor* actor = ( Actor* ) tar;
		value = actor->GetStat( parameters->int0Parameter );
	}
	SetVariable( Sender, parameters->string0Parameter, value );
}

void GameScript::BreakInstants(Scriptable* Sender, Action* /*parameters*/)
{
	//don't do anything, apparently the point of this action is to
	//delay the execution of further actions to the next AI cycle
	Sender->SetWait(1);
	Sender->ReleaseCurrentAction(); // this doesn't really need to block
}

//an interesting improvement would be to pause game for a given duration
void GameScript::PauseGame(Scriptable* Sender, Action* /*parameters*/)
{
	GameControl *gc = core->GetGameControl();
	if (gc) {
		gc->SetDialogueFlags(DF_FREEZE_SCRIPTS, BM_OR);
		displaymsg->DisplayConstantString(STR_SCRIPTPAUSED, DMC_RED);
	}
	// releasing this action allows actions to continue executing,
	// so we force a wait
	Sender->SetWait(1);
	Sender->ReleaseCurrentAction(); // does this need to block?
}

void GameScript::SetNoOneOnTrigger(Scriptable* Sender, Action* parameters)
{
	Scriptable* ip;

	if (!parameters->objects[1]) {
		ip=Sender;
	} else {
		ip = Sender->GetCurrentArea()->TMap->GetInfoPoint(parameters->objects[1]->objectName);
	}
	if (!ip || (ip->Type!=ST_TRIGGER && ip->Type!=ST_TRAVEL && ip->Type!=ST_PROXIMITY)) {
		printMessage("Actions", "Script error: No Trigger Named \"%s\"\n", YELLOW, parameters->objects[1]->objectName);
		return;
	}
	// FIXME: what does this do? clear triggers?
	/*ip->LastEntered = 0;
	ip->LastTrigger = 0;
	ip->LastTriggerObject = 0;*/
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

//this will force bashing the door
void GameScript::BashDoor(Scriptable* Sender, Action* parameters)
{
	GameControl *gc = core->GetGameControl();
	if (!gc) {
		Sender->ReleaseCurrentAction();
		return;
	}
	if (Sender->Type != ST_ACTOR) {
		Sender->ReleaseCurrentAction();
		return;
	}

	Scriptable *target = GetActorFromObject(Sender, parameters->objects[1]);
	TileMap *tmap = Sender->GetCurrentArea()->TMap;
	Door *door = NULL;
	Container *container = NULL;
	Point pos;
	if (target->Type == ST_DOOR) {
		// FIXME: actually it chooses from two possible points
		pos = target->Pos;
		door = tmap->GetDoorByPosition(pos);
	} else if(target->Type == ST_CONTAINER) {
		pos = target->Pos;
		container = tmap->GetContainerByPosition(pos);
	} else {
		Sender->ReleaseCurrentAction();
		return;
	}

	// TODO: "sets a field in the door/container to 1"

	if (SquaredPersonalDistance(pos, Sender) > MAX_OPERATING_DISTANCE*MAX_OPERATING_DISTANCE) {
		MoveNearerTo(Sender, pos, MAX_OPERATING_DISTANCE, 0);
		return;
	}

	gc->SetTargetMode(TARGET_MODE_ATTACK); //for bashing doors too

	// try to bash it
	if (door) {
		door->TryBashLock((Actor *) Sender);
	} else if (container) {
		container->TryBashLock((Actor *) Sender);
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
		ip = Sender->GetCurrentArea()->TMap->GetInfoPoint(parameters->objects[1]->objectName);
	}
	if (!ip) {
		return;
	}
	if (ip->Type!=ST_PROXIMITY && ip->Type!=ST_TRAVEL) {
		return;
	}
	InfoPoint *tar = (InfoPoint *) ip;
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
		ip = Sender->GetCurrentArea()->TMap->GetInfoPoint(parameters->objects[1]->objectName);
	}
	if (!ip) {
		return;
	}
	if (ip->Type!=ST_PROXIMITY && ip->Type!=ST_TRAVEL) {
		return;
	}
	InfoPoint *tar = (InfoPoint *) ip;
	if (parameters->int0Parameter) {
		tar->Trapped|=PORTAL_TRAVEL;
	} else {
		tar->Trapped&=~PORTAL_TRAVEL;
	}
}

//unhardcoded iwd action (for the forge entrance change)
void GameScript::ChangeDestination(Scriptable* Sender, Action* parameters)
{
	InfoPoint *ip = Sender->GetCurrentArea()->TMap->GetInfoPoint(parameters->objects[1]->objectName);
	if (ip && (ip->Type==ST_TRAVEL) ) {
		//alter the destination area, don't touch the entrance variable link
		strnlwrcpy(ip->Destination, parameters->string0Parameter, sizeof(ieResRef)-1 );
	}
}

void GameScript::MoveCursorPoint(Scriptable* /*Sender*/, Action* parameters)
{
	core->GetVideoDriver()->MoveMouse(parameters->pointParameter.x, parameters->pointParameter.y);
}

//false means, no talk
void GameScript::DialogueInterrupt(Scriptable* Sender, Action* parameters)
{
	if (Sender->Type!=ST_ACTOR) {
		return;
	}
	Actor* actor = ( Actor* ) Sender;
	if ( parameters->int0Parameter != 0 ) {
		actor->SetMCFlag(MC_NO_TALK, BM_NAND);
	} else {
		actor->SetMCFlag(MC_NO_TALK, BM_OR);
	}
}

void GameScript::EquipMostDamagingMelee(Scriptable* Sender, Action* /*parameters*/)
{
	if (Sender->Type!=ST_ACTOR) {
		return;
	}
	Actor* actor = ( Actor* ) Sender;
	actor->inventory.EquipBestWeapon(EQUIP_MELEE);
}

void GameScript::EquipRanged(Scriptable* Sender, Action* /*parameters*/)
{
	if (Sender->Type!=ST_ACTOR) {
		return;
	}
	Actor* actor = ( Actor* ) Sender;
	actor->inventory.EquipBestWeapon(EQUIP_RANGED);
}

//will equip best weapon regardless of range considerations
void GameScript::EquipWeapon(Scriptable* Sender, Action* /*parameters*/)
{
	if (Sender->Type!=ST_ACTOR) {
		return;
	}
	Actor* actor = ( Actor* ) Sender;
	actor->inventory.EquipBestWeapon(EQUIP_MELEE|EQUIP_RANGED);
}

void GameScript::SetBestWeapon(Scriptable* Sender, Action* parameters)
{
	if (Sender->Type!=ST_ACTOR) {
		return;
	}

	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1] );
	if (!tar || tar->Type!=ST_ACTOR) {
		return;
	}
	Actor* actor = ( Actor* ) Sender;

	Actor *target = (Actor *) tar;
	if (PersonalDistance(actor,target)>(unsigned int) parameters->int0Parameter) {
		actor->inventory.EquipBestWeapon(EQUIP_RANGED);
	} else {
		actor->inventory.EquipBestWeapon(EQUIP_MELEE);
	}
}

void GameScript::FakeEffectExpiryCheck(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1] );
	if (!tar || tar->Type!=ST_ACTOR) {
		return;
	}
	Actor *target = (Actor *) tar;
		target->fxqueue.RemoveExpiredEffects(parameters->int0Parameter);
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
	if (Sender->Type!=ST_ACTOR) {
		return;
	}
	Actor *scr = (Actor *) Sender;
	int slot = parameters->int0Parameter;
	int wslot = scr->inventory.GetWeaponSlot();
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
	wslot = scr->inventory.GetQuickSlot();
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
	if (Sender->Type!=ST_ACTOR) {
		Sender->ReleaseCurrentAction();
		return;
	}
	Scriptable* tar = GetStoredActorFromObject( Sender, parameters->objects[1] );
	if (!tar) {
		Sender->ReleaseCurrentAction();
		return;
	}
	Actor *act = (Actor *) Sender;
	int Slot;
	ieDword header, flags;
	ieResRef itemres;

	if (parameters->string0Parameter[0]) {
		Slot = act->inventory.FindItem(parameters->string0Parameter, 0);
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

	unsigned int dist = GetItemDistance(itemres, header);

	if (PersonalDistance(tar->Pos, Sender) > dist) {
		MoveNearerTo(Sender, tar, dist);
		return;
	}

	act->UseItem(Slot, header, tar, flags);
	Sender->ReleaseCurrentAction();
}

void GameScript::UseItemPoint(Scriptable* Sender, Action* parameters)
{
	if (Sender->Type!=ST_ACTOR) {
		Sender->ReleaseCurrentAction();
		return;
	}

	Actor *act = (Actor *) Sender;
	int Slot;
	ieDword header;
	ieResRef itemres;
	ieDword flags;

	if (parameters->string0Parameter[0]) {
		Slot = act->inventory.FindItem(parameters->string0Parameter, 0);
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

	unsigned int dist = GetItemDistance(itemres, header);

	if (PersonalDistance(parameters->pointParameter, Sender) > dist) {
		MoveNearerTo(Sender, parameters->pointParameter, dist, 0);
		return;
	}

	act->UseItemPoint(Slot, header, parameters->pointParameter, flags);
	Sender->ReleaseCurrentAction();
}

//addfeat will be able to remove feats too
//(the second int parameter is a value to add to the feat)
void GameScript::AddFeat(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1] );
	if (!tar || tar->Type!=ST_ACTOR) {
		return;
	}
	Actor *actor = (Actor *)tar;
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
	Actor *scr = (Actor *) Sender;
	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1] );
	if (!tar || tar->Type!=ST_ACTOR) {
		return;
	}
	Actor *actor = (Actor *)tar;
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
	if (Sender->Type!=ST_ACTOR) {
		return;
	}
	Actor *scr = (Actor *) Sender;
	ieDword stat = parameters->int0Parameter;
	if (stat<9 || stat>14) {
		return;
	}
	stat += IE_COLORS - 9;
	scr->SetBase(stat, (scr->GetBase(stat)&~255)|(parameters->int1Parameter&255));
}

//removes previous kit, adds new
void GameScript::AddKit(Scriptable* Sender, Action* parameters)
{
	if (Sender->Type!=ST_ACTOR) {
		return;
	}
	Actor *scr = (Actor *) Sender;
	//remove previous kit stuff
	scr->ApplyKit(true);
	//this adds the current level abilities
	scr->SetBase(IE_KIT, parameters->int0Parameter);
	scr->ApplyKit(false);
}

//doesn't remove old kit
void GameScript::AddSuperKit(Scriptable* Sender, Action* parameters)
{
	if (Sender->Type!=ST_ACTOR) {
		return;
	}
	Actor *scr = (Actor *) Sender;
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
void GameScript::ChangeAIType(Scriptable* Sender, Action* parameters)
{
	if (Sender->Type!=ST_ACTOR) {
		return;
	}
	Object *ob = parameters->objects[1];
	if (!ob) {
		return;
	}
	Actor *scr = (Actor *) Sender;
	for (int i=0;i<MAX_OBJECT_FIELDS;i++) {
		int val = ob->objectFields[i];
		if (!val) continue;
		if (!strnicmp(ObjectIDSTableNames[i],"ea",8)) {
			scr->SetBase(IE_EA, val);
			continue;
		}
		if (!strnicmp(ObjectIDSTableNames[i],"general",8)) {
			scr->SetBase(IE_GENERAL, val);
			continue;
		}
		if (!strnicmp(ObjectIDSTableNames[i],"race",8)) {
			scr->SetBase(IE_RACE, val);
			continue;
		}
		if (!strnicmp(ObjectIDSTableNames[i],"class",8)) {
			scr->SetBase(IE_CLASS, val);
			continue;
		}
		if (!strnicmp(ObjectIDSTableNames[i],"gender",8)) {
			scr->SetBase(IE_SEX, val);
			continue;
		}
		if (!strnicmp(ObjectIDSTableNames[i],"specific",8)) {
			scr->SetBase(IE_SPECIFIC, val);
			continue;
		}
		if (!strnicmp(ObjectIDSTableNames[i],"align",8)) {
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

	char Tmp[256];

	snprintf(Tmp, 256, "MoveToPoint([%d.%d])", parameters->pointParameter.x, parameters->pointParameter.y);
	Action *newact = GenerateAction(Tmp);
	Sender->AddAction(newact);
}

//same as MoveToPointNoRecticle, but not blocking
void GameScript::Follow(Scriptable* Sender, Action* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		return;
	}

	char Tmp[256];

	snprintf(Tmp, 256, "MoveToPointNoRecticle([%d.%d])", parameters->pointParameter.x, parameters->pointParameter.y);
	Action *newact = GenerateAction(Tmp);
	Sender->AddAction(newact);
}

void GameScript::FollowCreature(Scriptable* Sender, Action* parameters)
{
	if (Sender->Type!=ST_ACTOR) {
		Sender->ReleaseCurrentAction();
		return;
	}

	Scriptable* tar = GetStoredActorFromObject( Sender, parameters->objects[1] );
	if (!tar || tar->Type!=ST_ACTOR) {
		Sender->ReleaseCurrentAction();
		return;
	}
	Actor *scr = (Actor *)Sender;
	Actor *actor = (Actor *)tar;
	scr->LastFollowed = actor->GetGlobalID();
	scr->FollowOffset.empty();
	if (!scr->InMove() || scr->Destination != actor->Pos) {
		scr->WalkTo(actor->Pos, 0, 1);
	}
}

void GameScript::RunFollow(Scriptable* Sender, Action* parameters)
{
	if (Sender->Type!=ST_ACTOR) {
		Sender->ReleaseCurrentAction();
		return;
	}

	Scriptable* tar = GetStoredActorFromObject( Sender, parameters->objects[1] );
	if (!tar || tar->Type!=ST_ACTOR) {
		Sender->ReleaseCurrentAction();
		return;
	}
	Actor *scr = (Actor *)Sender;
	Actor *actor = (Actor *)tar;
	scr->LastFollowed = actor->GetGlobalID();
	scr->FollowOffset.empty();
	if (!scr->InMove() || scr->Destination != actor->Pos) {
		scr->WalkTo(actor->Pos, IF_RUNNING, 1);
	}
}

void GameScript::ProtectPoint(Scriptable* Sender, Action* parameters)
{
	if (Sender->Type!=ST_ACTOR) {
		Sender->ReleaseCurrentAction();
		return;
	}
	Actor *scr = (Actor *)Sender;
	if (!scr->InMove() || scr->Destination != parameters->pointParameter) {
		scr->WalkTo( parameters->pointParameter, 0, 1 );
	}
	// we should handle 'Protect' here rather than just unblocking
	Sender->ReleaseCurrentAction();
}

void GameScript::ProtectObject(Scriptable* Sender, Action* parameters)
{
	if (Sender->Type!=ST_ACTOR) {
		Sender->ReleaseCurrentAction();
		return;
	}

	Scriptable* tar = GetStoredActorFromObject( Sender, parameters->objects[1] );
	if (!tar || tar->Type!=ST_ACTOR) {
		Sender->ReleaseCurrentAction();
		return;
	}
	Actor *scr = (Actor *)Sender;
	Actor *actor = (Actor *)tar;
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
	GameControl *gc = core->GetGameControl();
	if (!gc) {
		Sender->ReleaseCurrentAction();
		return;
	}
	if (Sender->Type!=ST_ACTOR) {
		Sender->ReleaseCurrentAction();
		return;
	}

	Scriptable* tar = GetStoredActorFromObject( Sender, parameters->objects[1] );
	if (!tar || tar->Type!=ST_ACTOR) {
		Sender->ReleaseCurrentAction();
		return;
	}
	Actor *scr = (Actor *)Sender;
	Actor *actor = (Actor *)tar;
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
	GameControl *gc = core->GetGameControl();
	if (!gc) {
		Sender->ReleaseCurrentAction();
		return;
	}
	if (Sender->Type!=ST_ACTOR) {
		Sender->ReleaseCurrentAction();
		return;
	}
	Scriptable* tar = GetStoredActorFromObject( Sender, parameters->objects[1] );
	if (!tar) {
		Sender->ReleaseCurrentAction();
		return;
	}
	Actor *scr = (Actor *)Sender;
	ieDword formation = parameters->int0Parameter;
	ieDword pos = parameters->int1Parameter;
	Point FollowOffset = gc->GetFormationOffset(formation, pos);
	FollowOffset.x+=tar->Pos.x;
	FollowOffset.y+=tar->Pos.y;
	if (!scr->InMove() || scr->Destination != FollowOffset) {
		scr->WalkTo( FollowOffset, 0, 1 );
	}
}

void GameScript::TransformItem(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1] );
	if (!tar || tar->Type!=ST_ACTOR) {
		return;
	}
	TransformItemCore((Actor *)tar, parameters, true);
}

void GameScript::TransformPartyItem(Scriptable* /*Sender*/, Action* parameters)
{
	Game *game = core->GetGame();
	int i = game->GetPartySize(false);
	while (i--) {
		Actor *tar = game->GetPC(i, false);
		TransformItemCore(tar, parameters, true);
	}
}

void GameScript::TransformItemAll(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1] );
	if (!tar || tar->Type!=ST_ACTOR) {
		return;
	}
	TransformItemCore((Actor *)tar, parameters, false);
}

void GameScript::TransformPartyItemAll(Scriptable* /*Sender*/, Action* parameters)
{
	Game *game = core->GetGame();
	int i = game->GetPartySize(false);
	while (i--) {
		Actor *tar = game->GetPC(i, false);
		TransformItemCore(tar, parameters, false);
	}
}

void GameScript::GeneratePartyMember(Scriptable* /*Sender*/, Action* parameters)
{
	AutoTable pcs("bios");
	if (!pcs) {
		return;
	}
	const char* string = pcs->QueryField( parameters->int0Parameter, 0 );
	int pos = gamedata->LoadCreature(string,0,false);
	if (pos<0) {
		return;
	}
	Actor *actor = core->GetGame()->GetNPC(pos);
	if (!actor) {
		return;
	}
	actor->SetOrientation(parameters->int1Parameter, false);
	actor->MoveTo(parameters->pointParameter);
}

void GameScript::EnableFogDither(Scriptable* /*Sender*/, Action* /*parameters*/)
{
	core->FogOfWar|=FOG_DRAWFOG;
}

void GameScript::DisableFogDither(Scriptable* /*Sender*/, Action* /*parameters*/)
{
	core->FogOfWar&=~FOG_DRAWFOG;
}

void DeleteAllSpriteCovers()
{
	Game *game = core->GetGame();
	int i = game->GetPartySize(false);
	while (i--) {
		Selectable *tar = (Selectable *) game->GetPC(i, false);
		tar->SetSpriteCover(NULL);
	}
}

void GameScript::EnableSpriteDither(Scriptable* /*Sender*/, Action* /*parameters*/)
{
	core->FogOfWar&=~FOG_DITHERSPRITES;
	DeleteAllSpriteCovers();
}

void GameScript::DisableSpriteDither(Scriptable* /*Sender*/, Action* /*parameters*/)
{
	core->FogOfWar|=~FOG_DITHERSPRITES;
	DeleteAllSpriteCovers();
}

//the PST crew apparently loved hardcoding stuff
ieResRef RebusResRef={"DABUS1"};

void GameScript::FloatRebus(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1] );
	if (!tar || tar->Type!=ST_ACTOR) {
		return;
	}
	Actor *actor = (Actor *)tar;
	RebusResRef[5]=(char) core->Roll(1,5,'0');
	ScriptedAnimation *vvc = gamedata->GetScriptedAnimation(RebusResRef, 0);
	if (vvc) {
		//setting the height
		vvc->ZPos=actor->size*20;
		vvc->PlayOnce();
		//maybe this needs setting up some time
		vvc->SetDefaultDuration(20);
		actor->AddVVCell(vvc);
	}
}

void GameScript::IncrementKillStat(Scriptable* Sender, Action* parameters)
{
	DataFileMgr * ini = core->GetBeastsINI();
	if (!ini) {
		return;
	}
	char key[5];
	sprintf(key,"%d", parameters->int0Parameter);
	const char *variable = ini->GetKeyAsString( key, "killvar", NULL );
	if (!variable) {
		return;
	}
	ieDword value = CheckVariable( Sender, variable, "GLOBAL" ) + 1;
	SetVariable( Sender, variable, "GLOBAL", value );
}

void GameScript::SpellCastEffect(Scriptable* Sender, Action* parameters)
{
	Scriptable* src = GetActorFromObject( Sender, parameters->objects[1] );
	if (!src) {
		return;
	}
	//TODO: finish this
}

//this action plays a vvc animation over target
//we simply apply the appropriate opcode on the target (see iwdopcodes)
//the list of vvcs is in iwdshtab.2da
static EffectRef fx_iwd_visual_spell_hit_ref = { "IWDVisualSpellHit", -1 };

void GameScript::SpellHitEffectSprite(Scriptable* Sender, Action* parameters)
{
	Scriptable* src = GetActorFromObject( Sender, parameters->objects[1] );
	if (!src) {
		return;
	}
	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[2] );
	if (!tar || tar->Type!=ST_ACTOR) {
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
	fx->Probability1=100;
	fx->TimingMode=FX_DURATION_INSTANT_PERMANENT_AFTER_BONUSES;
	core->ApplyEffect(fx, (Actor *) tar, src);
}

void GameScript::SpellHitEffectPoint(Scriptable* Sender, Action* parameters)
{
	Scriptable* src = GetActorFromObject( Sender, parameters->objects[1] );
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
	fx->Probability1=100;
	fx->TimingMode=FX_DURATION_INSTANT_PERMANENT_AFTER_BONUSES;
	fx->PosX=parameters->pointParameter.x;
	fx->PosY=parameters->pointParameter.y;
	core->ApplyEffect(fx, NULL, src);
}


void GameScript::ClickLButtonObject(Scriptable* Sender, Action* parameters)
{
	Scriptable *tar = GetActorFromObject(Sender, parameters->objects[1] );
	if (!tar) {
		Sender->ReleaseCurrentAction(); // this is blocking for some reason?
		return;
	}
	ClickCore(Sender, tar->Pos, GEM_MB_ACTION, parameters->int0Parameter);
}

void GameScript::ClickLButtonPoint(Scriptable* Sender, Action* parameters)
{
	ClickCore(Sender, parameters->pointParameter, GEM_MB_ACTION, parameters->int0Parameter);
}

void GameScript::ClickRButtonObject(Scriptable* Sender, Action* parameters)
{
	Scriptable *tar = GetActorFromObject(Sender, parameters->objects[1] );
	if (!tar) {
		Sender->ReleaseCurrentAction(); // this is blocking for some reason?
		return;
	}
	ClickCore(Sender, tar->Pos, GEM_MB_MENU, parameters->int0Parameter);
}

void GameScript::ClickRButtonPoint(Scriptable* Sender, Action* parameters)
{
	ClickCore(Sender, parameters->pointParameter, GEM_MB_MENU, parameters->int0Parameter);
}

void GameScript::DoubleClickLButtonObject(Scriptable* Sender, Action* parameters)
{
	Scriptable *tar = GetActorFromObject(Sender, parameters->objects[1] );
	if (!tar) {
		Sender->ReleaseCurrentAction(); // this is blocking for some reason?
		return;
	}
	ClickCore(Sender, tar->Pos, GEM_MB_ACTION|GEM_MB_DOUBLECLICK, parameters->int0Parameter);
}

void GameScript::DoubleClickLButtonPoint(Scriptable* Sender, Action* parameters)
{
	ClickCore(Sender, parameters->pointParameter, GEM_MB_ACTION|GEM_MB_DOUBLECLICK, parameters->int0Parameter);
}

void GameScript::DoubleClickRButtonObject(Scriptable* Sender, Action* parameters)
{
	Scriptable *tar = GetActorFromObject(Sender, parameters->objects[1] );
	if (!tar) {
		Sender->ReleaseCurrentAction(); // this is blocking for some reason?
		return;
	}
	ClickCore(Sender, tar->Pos, GEM_MB_MENU|GEM_MB_DOUBLECLICK, parameters->int0Parameter);
}

void GameScript::DoubleClickRButtonPoint(Scriptable* Sender, Action* parameters)
{
	ClickCore(Sender, parameters->pointParameter, GEM_MB_MENU|GEM_MB_DOUBLECLICK, parameters->int0Parameter);
}

//Picks 5 lines from wish.2da
//Gets the 5 values (column is int0parameter) from the table.
//Sets the five wishpowerNN to 1, while resets the rest to 0.
//TODO: investigate what happens with * values
void GameScript::SetupWish(Scriptable* Sender, Action* parameters)
{
	SetupWishCore(Sender, parameters->int0Parameter, parameters->int1Parameter);
}

//The same as the previous action, except that the column parameter comes from
//the target object's wisdom directly (this action is not used in the original)
void GameScript::SetupWishObject(Scriptable* Sender, Action* parameters)
{
	Scriptable *tar = GetActorFromObject(Sender, parameters->objects[1] );
	if (!tar || tar->Type!=ST_ACTOR) {
		return;
	}
	SetupWishCore(Sender, ((Actor *)tar)->GetStat(IE_WIS), parameters->int0Parameter);
}

//GemRB specific action
//Sets up multiple tokens randomly (one per 2da row)
//the row label column sets the token names
void GameScript::SetToken2DA(Scriptable* /*Sender*/, Action* parameters)
{
	int count;
	int i,j;
	ieVariable tokenname;

	AutoTable tm(parameters->string0Parameter);
	if (!tm) {
		printMessage("Actions", "Cannot find %s.2da.", LIGHT_RED, parameters->string0Parameter);
		printStatus("ERROR", LIGHT_RED);
		return;
	}

	count = tm->GetRowCount();
	for(i=0;i<count;i++) {
		//roll a random number between 0 and column #
		j = core->Roll(1,tm->GetColumnCount(i),-1);
		strnuprcpy(tokenname, tm->GetRowName(i), 32);
		core->GetTokenDictionary()->SetAtCopy( tokenname, tm->QueryField(i, j) );
	}
}

//this is a gemrb extension for scriptable tracks
void GameScript::SetTrackString(Scriptable* Sender, Action* parameters)
{
	Map *map = Sender->GetCurrentArea();
	if (!map) return;
	map->SetTrackString(parameters->int0Parameter, parameters->int1Parameter, parameters->int2Parameter);
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
	ieResRef area;

	snprintf(area,8,"AR%04d", parameters->int0Parameter);
	IniSpawn *sp = Sender->GetCurrentArea()->INISpawn;
	if (!sp) {
		return;
	}
	sp->SetNamelessDeath(area, parameters->pointParameter, parameters->int1Parameter);
}
