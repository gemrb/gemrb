/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003-2005 The GemRB Project
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
#include "Calendar.h"
#include "DialogHandler.h"
#include "Game.h"
#include "GameData.h"
#include "Polygon.h"
#include "TableMgr.h"
#include "Video.h"
#include "GUI/GameControl.h"
#include "math.h" //needs for acos
#include "Scriptable/Container.h"
#include "Scriptable/Door.h"
#include "Scriptable/InfoPoint.h"

namespace GemRB {

//-------------------------------------------------------------
// Trigger Functions
//-------------------------------------------------------------
int GameScript::BreakingPoint(Scriptable* Sender, Trigger* /*parameters*/)
{
	int value=GetHappiness(Sender, core->GetGame()->Reputation );
	return value < -300;
}

int GameScript::Reaction(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if (!scr || scr->Type != ST_ACTOR) {
		parameters->dump();
		return 0;
	}
	int value = GetReaction(((Actor*) scr), Sender);
	return value == parameters->int0Parameter;
}

int GameScript::ReactionGT(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if (!scr || scr->Type != ST_ACTOR) {
		parameters->dump();
		return 0;
	}
	int value = GetReaction(((Actor*) scr), Sender);
	return value > parameters->int0Parameter;
}

int GameScript::ReactionLT(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if (!scr || scr->Type != ST_ACTOR) {
		parameters->dump();
		return 0;
	}
	int value = GetReaction(((Actor*) scr), Sender);
	return value < parameters->int0Parameter;
}

int GameScript::Happiness(Scriptable* Sender, Trigger* parameters)
{
	int value=GetHappiness(Sender, core->GetGame()->Reputation );
	return value == parameters->int0Parameter;
}

int GameScript::HappinessGT(Scriptable* Sender, Trigger* parameters)
{
	int value=GetHappiness(Sender, core->GetGame()->Reputation );
	return value > parameters->int0Parameter;
}

int GameScript::HappinessLT(Scriptable* Sender, Trigger* parameters)
{
	int value=GetHappiness(Sender, core->GetGame()->Reputation );
	return value < parameters->int0Parameter;
}

int GameScript::Reputation(Scriptable* /*Sender*/, Trigger* parameters)
{
	return core->GetGame()->Reputation/10 == (ieDword) parameters->int0Parameter;
}

int GameScript::ReputationGT(Scriptable* /*Sender*/, Trigger* parameters)
{
	return core->GetGame()->Reputation/10 > (ieDword) parameters->int0Parameter;
}

int GameScript::ReputationLT(Scriptable* /*Sender*/, Trigger* parameters)
{
	return core->GetGame()->Reputation/10 < (ieDword) parameters->int0Parameter;
}

int GameScript::Alignment(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if (!scr || scr->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) scr;
	return ID_Alignment( actor, parameters->int0Parameter);
}

int GameScript::Allegiance(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if (!scr || scr->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) scr;
	return ID_Allegiance( actor, parameters->int0Parameter);
}

//should return *_ALL stuff
int GameScript::Class(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if (!scr || scr->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = (Actor*)scr;
	return ID_Class( actor, parameters->int0Parameter);
}

int GameScript::ClassEx(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if (!scr || scr->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = (Actor*)scr;
	return ID_AVClass( actor, parameters->int0Parameter);
}

int GameScript::Faction(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if (!scr || scr->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = (Actor*)scr;
	return ID_Faction( actor, parameters->int0Parameter);
}

int GameScript::Team(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if (!scr || scr->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = (Actor*)scr;
	return ID_Team( actor, parameters->int0Parameter);
}

int GameScript::SubRace(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if (!scr || scr->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = (Actor*)scr;
	//subrace trigger uses a weird system, cannot use ID_*
	//return ID_Subrace( actor, parameters->int0Parameter);
	int value = actor->GetStat(IE_SUBRACE);
	if (value) {
		value |= actor->GetStat(IE_RACE)<<16;
	}
	if (value == parameters->int0Parameter) {
		return 1;
	}
	return 0;
}

//if object parameter is given (gemrb) it is used
//otherwise it works on the current object (iwd2)
int GameScript::IsTeamBitOn(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr = Sender;
	if (parameters->objectParameter) {
		scr = GetActorFromObject( Sender, parameters->objectParameter );
	}
	if (!scr || scr->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = (Actor*)scr;
	if (actor->GetStat(IE_TEAM) & parameters->int0Parameter) {
		return 1;
	}
	return 0;
}

int GameScript::NearbyDialog(Scriptable* Sender, Trigger* parameters)
{
	Actor *target = Sender->GetCurrentArea()->GetActorByDialog(parameters->string0Parameter);
	if ( !target ) {
		return 0;
	}
	return CanSee( Sender, target, true, GA_NO_DEAD | GA_NO_HIDDEN );
}

//atm this checks for InParty and See, it is unsure what is required
int GameScript::IsValidForPartyDialog(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if (!scr) {
		scr = Sender;
	}
	if (scr->Type != ST_ACTOR) {
		return 0;
	}
	Actor *target = (Actor *) scr;
	//inparty returns -1 if not in party
	if (core->GetGame()->InParty( target )<0) {
		return 0;
	}
	//don't accept parties currently in dialog!
	//this might disturb some modders, but this is the correct behaviour
	//for example the aaquatah dialog in irenicus dungeon depends on it
	GameControl *gc = core->GetGameControl();
	Actor *pc = (Actor *) scr;
	if (pc->GetGlobalID() == gc->dialoghandler->targetID || pc->GetGlobalID()==gc->dialoghandler->speakerID) {
		return 0;
	}

	//don't accept parties with the no interrupt flag
	//this fixes bug #2573808 on gamescript level
	//(still someone has to turn the no interrupt flag off)
	if(!pc->GetDialog(GD_CHECK)) {
		return 0;
	}
	return CanSee( Sender, target, false, GA_NO_DEAD );
}

int GameScript::InParty(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr;

	if (parameters->objectParameter) {
		scr = GetActorFromObject( Sender, parameters->objectParameter );
	} else {
		scr = Sender;
	}
	if (!scr || scr->Type != ST_ACTOR) {
		return 0;
	}
	Actor *tar = (Actor *) scr;
	if (core->GetGame()->InParty( tar ) <0) {
		return 0;
	}
	//don't allow dead, don't allow maze and similar effects
	if (tar->ValidTarget(GA_NO_DEAD|GA_NO_HIDDEN)) {
		return 1;
	}
	return 0;
}

int GameScript::InPartyAllowDead(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr;

	if (parameters->objectParameter) {
		scr = GetActorFromObject( Sender, parameters->objectParameter );
	} else {
		scr = Sender;
	}
	if (!scr || scr->Type != ST_ACTOR) {
		return 0;
	}
	return core->GetGame()->InParty( ( Actor * ) scr ) >= 0 ? 1 : 0;
}

int GameScript::InPartySlot(Scriptable* Sender, Trigger* parameters)
{
	Actor *actor = core->GetGame()->GetPC(parameters->int0Parameter, false);
	return MatchActor(Sender, actor->GetGlobalID(), parameters->objectParameter);
}

int GameScript::Exists(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if (!scr) {
		return 0;
	}
	return 1;
}

int GameScript::IsAClown(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if (!scr || scr->Type!=ST_ACTOR) {
		return 0;
	}
	return 1;
}

int GameScript::IsGabber(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if (!scr || scr->Type!=ST_ACTOR) {
		return 0;
	}
	if (scr->GetGlobalID() == core->GetGameControl()->dialoghandler->speakerID)
		return 1;
	return 0;
}

//returns true if the trap or infopoint is active
//returns true if the actor is active
//returns true if the sound source is active
//returns true if container is active
int GameScript::IsActive(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if (!scr) {
		AmbientMgr * ambientmgr = core->GetAudioDrv()->GetAmbientMgr();
		if (ambientmgr->isActive(parameters->objectParameter->objectName) ) {
			return 1;
		}
		return 0;
	}

	switch(scr->Type) {
		default:
			return 0;
		case ST_ACTOR:
			if( ((Actor *) scr)->Schedule(core->GetGame()->GameTime, true)) return 1;
			return 0;
		case ST_CONTAINER:
			if ( ((Container *) scr)->Flags&CONT_DISABLED) return 0;
			return 1;

		case ST_PROXIMITY: case ST_TRIGGER: case ST_TRAVEL:
			if ( ((InfoPoint *) scr)->Flags&(TRAP_DEACTIVATED|INFO_DOOR) ) {
				return 0;
			}
			return 1;
	}
	return 0;
}

int GameScript::InTrap(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if (!scr) {
		return 0;
	}
	if (scr->GetInternalFlag()&IF_INTRAP) {
		return 1;
	}
	return 0;
}

/* checks if targeted actor is in the specified region
 GemRB allows different regions, referenced by int0Parameter
 The polygons are stored in island<nn>.2da files */
int GameScript::OnIsland(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if (!scr) {
		return 0;
	}
	Gem_Polygon *p = GetPolygon2DA(parameters->int0Parameter);
	if (!p) {
		return 0;
	}
	return p->PointIn(scr->Pos);
}

int GameScript::School(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if (!scr || scr->Type!=ST_ACTOR) {
		return 0;
	}
	Actor* actor = (Actor *) scr;
	//only the low 2 bytes count
	//the School values start from 1 to 9 and the first school value is 0x40
	//so this mild hack will do
	if ( actor->GetStat(IE_KIT) == (ieDword) (0x20<<parameters->int0Parameter)) {
		return 1;
	}
	return 0;
}

int GameScript::Kit(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if (!scr || scr->Type!=ST_ACTOR) {
		return 0;
	}
	Actor* actor = (Actor *) scr;

	ieDword kit = actor->GetStat(IE_KIT);
	//TODO: fix baseclass / barbarian confusion

	//IWD2 style kit matching (also used for mage schools)
	if (kit == (ieDword) (parameters->int0Parameter)) {
		return 1;
	}
	//BG2 style kit matching (not needed anymore?), we do it on load
	//kit = (kit>>16)|(kit<<16);
	if ( kit == (ieDword) (parameters->int0Parameter)) {
		return 1;
	}
	return 0;
}

int GameScript::General(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if (!scr) {
		scr = Sender;
	}
	if (scr->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) scr;
	if (actor == NULL) {
		return 0;
	}
	return ID_General(actor, parameters->int0Parameter);
}

int GameScript::Specifics(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if (!scr) {
		scr = Sender;
	}
	if (scr->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) scr;
	if (actor == NULL) {
		return 0;
	}
	return ID_Specific(actor, parameters->int0Parameter);
}

int GameScript::BitCheck(Scriptable* Sender, Trigger* parameters)
{
	bool valid=true;

	ieDword value = CheckVariable(Sender, parameters->string0Parameter, &valid );
	if (valid) {
		if ( value & parameters->int0Parameter ) return 1;
	}
	return 0;
}

int GameScript::BitCheckExact(Scriptable* Sender, Trigger* parameters)
{
	bool valid=true;

	ieDword value = CheckVariable(Sender, parameters->string0Parameter, &valid );
	if (valid) {
		ieDword tmp = (ieDword) parameters->int0Parameter ;
		if ((value & tmp) == tmp) return 1;
	}
	return 0;
}

//BM_OR would make sense only if this trigger changes the value of the variable
//should I do that???
int GameScript::BitGlobal_Trigger(Scriptable* Sender, Trigger* parameters)
{
	bool valid=true;

	ieDword value = CheckVariable(Sender, parameters->string0Parameter, &valid );
	if (valid) {
		HandleBitMod(value, parameters->int0Parameter, parameters->int1Parameter);
		if (value!=0) return 1;
	}
	return 0;
}

int GameScript::GlobalOrGlobal_Trigger(Scriptable* Sender, Trigger* parameters)
{
	bool valid=true;

	ieDword value1 = CheckVariable(Sender, parameters->string0Parameter, &valid );
	if (valid) {
		if ( value1 ) return 1;
		ieDword value2 = CheckVariable(Sender, parameters->string1Parameter, &valid );
		if (valid) {
			if ( value2 ) return 1;
		}
	}
	return 0;
}

int GameScript::GlobalAndGlobal_Trigger(Scriptable* Sender, Trigger* parameters)
{
	bool valid=true;

	ieDword value1 = CheckVariable( Sender, parameters->string0Parameter, &valid );
	if (valid && value1) {
		ieDword value2 = CheckVariable( Sender, parameters->string1Parameter, &valid );
		if (valid && value2) return 1;
	}
	return 0;
}

int GameScript::GlobalBAndGlobal_Trigger(Scriptable* Sender, Trigger* parameters)
{
	bool valid=true;

	ieDword value1 = CheckVariable(Sender, parameters->string0Parameter, &valid );
	if (valid) {
		ieDword value2 = CheckVariable(Sender, parameters->string1Parameter, &valid );
		if (valid) {
			if ((value1& value2 ) != 0) return 1;
		}
	}
	return 0;
}

int GameScript::GlobalBAndGlobalExact(Scriptable* Sender, Trigger* parameters)
{
	bool valid=true;

	ieDword value1 = CheckVariable(Sender, parameters->string0Parameter, &valid );
	if (valid) {
		ieDword value2 = CheckVariable(Sender, parameters->string1Parameter, &valid );
		if (valid) {
			if (( value1& value2 ) == value2) return 1;
		}
	}
	return 0;
}

int GameScript::GlobalBitGlobal_Trigger(Scriptable* Sender, Trigger* parameters)
{
	bool valid=true;

	ieDword value1 = CheckVariable(Sender, parameters->string0Parameter, &valid );
	if (valid) {
		ieDword value2 = CheckVariable(Sender, parameters->string1Parameter, &valid );
		if (valid) {
			HandleBitMod( value1, value2, parameters->int1Parameter);
			if (value1!=0) return 1;
		}
	}
	return 0;
}

//no what exactly this trigger would do, defined in iwd2, but never used
//i just assume it sets a global in the trigger block
int GameScript::TriggerSetGlobal(Scriptable* Sender, Trigger* parameters)
{
	SetVariable( Sender, parameters->string0Parameter, parameters->int0Parameter );
	return 1;
}

//would this function also alter the variable?
int GameScript::Xor(Scriptable* Sender, Trigger* parameters)
{
	bool valid=true;

	ieDword value = CheckVariable(Sender, parameters->string0Parameter, &valid );
	if (valid) {
		if (( value ^ parameters->int0Parameter ) != 0) return 1;
	}
	return 0;
}

//TODO:
//no sprite is dead for iwd, they use KILL_<name>_CNT
int GameScript::NumDead(Scriptable* Sender, Trigger* parameters)
{
	ieDword value;

	if (core->HasFeature(GF_HAS_KAPUTZ) ) {
		value = CheckVariable(Sender, parameters->string0Parameter, "KAPUTZ");
	} else {
		ieVariable VariableName;
		snprintf(VariableName, 32, core->GetDeathVarFormat(), parameters->string0Parameter);
		value = CheckVariable(Sender, VariableName, "GLOBAL" );
	}
	return ( value == (ieDword) parameters->int0Parameter );
}

int GameScript::NumDeadGT(Scriptable* Sender, Trigger* parameters)
{
	ieDword value;

	if (core->HasFeature(GF_HAS_KAPUTZ) ) {
		value = CheckVariable(Sender, parameters->string0Parameter, "KAPUTZ");
	} else {
		ieVariable VariableName;
		snprintf(VariableName, 32, core->GetDeathVarFormat(), parameters->string0Parameter);
		value = CheckVariable(Sender, VariableName, "GLOBAL" );
	}
	return ( value > (ieDword) parameters->int0Parameter );
}

int GameScript::NumDeadLT(Scriptable* Sender, Trigger* parameters)
{
	ieDword value;

	if (core->HasFeature(GF_HAS_KAPUTZ) ) {
		value = CheckVariable(Sender, parameters->string0Parameter, "KAPUTZ");
	} else {
		ieVariable VariableName;

		snprintf(VariableName, 32, core->GetDeathVarFormat(), parameters->string0Parameter);
		value = CheckVariable(Sender, VariableName, "GLOBAL" );
	}
	return ( value < (ieDword) parameters->int0Parameter );
}

int GameScript::G_Trigger(Scriptable* Sender, Trigger* parameters)
{
	ieDwordSigned value = CheckVariable(Sender, parameters->string0Parameter, "GLOBAL" );
	return ( value == parameters->int0Parameter );
}

int GameScript::Global(Scriptable* Sender, Trigger* parameters)
{
	bool valid=true;

	ieDwordSigned value = CheckVariable(Sender, parameters->string0Parameter, &valid );
	if (valid) {
		if ( value == parameters->int0Parameter ) return 1;
	}
	return 0;
}

int GameScript::GLT_Trigger(Scriptable* Sender, Trigger* parameters)
{
	ieDwordSigned value = CheckVariable(Sender, parameters->string0Parameter,"GLOBAL" );
	return ( value < parameters->int0Parameter );
}

int GameScript::GlobalLT(Scriptable* Sender, Trigger* parameters)
{
	bool valid=true;

	ieDwordSigned value = CheckVariable(Sender, parameters->string0Parameter, &valid );
	if (valid) {
		if ( value < parameters->int0Parameter ) return 1;
	}
	return 0;
}

int GameScript::GGT_Trigger(Scriptable* Sender, Trigger* parameters)
{
	ieDwordSigned value = CheckVariable(Sender, parameters->string0Parameter, "GLOBAL" );
	return ( value > parameters->int0Parameter );
}

int GameScript::GlobalGT(Scriptable* Sender, Trigger* parameters)
{
	bool valid=true;

	ieDwordSigned value = CheckVariable(Sender, parameters->string0Parameter, &valid );
	if (valid) {
		if ( value > parameters->int0Parameter ) return 1;
	}
	return 0;
}

int GameScript::GlobalLTGlobal(Scriptable* Sender, Trigger* parameters)
{
	bool valid=true;

	ieDwordSigned value1 = CheckVariable(Sender, parameters->string0Parameter, &valid );
	if (valid) {
		ieDwordSigned value2 = CheckVariable(Sender, parameters->string1Parameter, &valid );
		if (valid) {
			if ( value1 < value2 ) return 1;
		}
	}
	return 0;
}

int GameScript::GlobalGTGlobal(Scriptable* Sender, Trigger* parameters)
{
	bool valid=true;

	ieDwordSigned value1 = CheckVariable(Sender, parameters->string0Parameter, &valid );
	if (valid) {
		ieDwordSigned value2 = CheckVariable(Sender, parameters->string1Parameter, &valid );
		if (valid) {
			if ( value1 > value2 ) return 1;
		}
	}
	return 0;
}

int GameScript::GlobalsEqual(Scriptable* Sender, Trigger* parameters)
{
	ieDword value1 = CheckVariable(Sender, parameters->string0Parameter, "GLOBAL" );
	ieDword value2 = CheckVariable(Sender, parameters->string1Parameter, "GLOBAL" );
	return ( value1 == value2 );
}

int GameScript::GlobalsGT(Scriptable* Sender, Trigger* parameters)
{
	ieDword value1 = CheckVariable(Sender, parameters->string0Parameter, "GLOBAL" );
	ieDword value2 = CheckVariable(Sender, parameters->string1Parameter, "GLOBAL" );
	return ( value1 > value2 );
}

int GameScript::GlobalsLT(Scriptable* Sender, Trigger* parameters)
{
	ieDword value1 = CheckVariable(Sender, parameters->string0Parameter, "GLOBAL" );
	ieDword value2 = CheckVariable(Sender, parameters->string1Parameter, "GLOBAL" );
	return ( value1 < value2 );
}

int GameScript::LocalsEqual(Scriptable* Sender, Trigger* parameters)
{
	ieDword value1 = CheckVariable(Sender, parameters->string0Parameter, "LOCALS" );
	ieDword value2 = CheckVariable(Sender, parameters->string1Parameter, "LOCALS" );
	return ( value1 == value2 );
}

int GameScript::LocalsGT(Scriptable* Sender, Trigger* parameters)
{
	ieDword value1 = CheckVariable(Sender, parameters->string0Parameter, "LOCALS" );
	ieDword value2 = CheckVariable(Sender, parameters->string1Parameter, "LOCALS" );
	return ( value1 > value2 );
}

int GameScript::LocalsLT(Scriptable* Sender, Trigger* parameters)
{
	ieDword value1 = CheckVariable(Sender, parameters->string0Parameter, "LOCALS" );
	ieDword value2 = CheckVariable(Sender, parameters->string1Parameter, "LOCALS" );
	return ( value1 < value2 );
}

int GameScript::RealGlobalTimerExact(Scriptable* Sender, Trigger* parameters)
{
	bool valid=true;

	ieDword value1 = CheckVariable(Sender, parameters->string0Parameter, parameters->string1Parameter, &valid );
	if (valid && value1) {
		ieDword value2 = core->GetGame()->RealTime;
		if ( value1 == value2 ) return 1;
	}
	return 0;
}

int GameScript::RealGlobalTimerExpired(Scriptable* Sender, Trigger* parameters)
{
	bool valid=true;

	ieDword value1 = CheckVariable(Sender, parameters->string0Parameter, parameters->string1Parameter, &valid );
	if (valid && value1) {
		if ( value1 < core->GetGame()->RealTime ) return 1;
	}
	return 0;
}

int GameScript::RealGlobalTimerNotExpired(Scriptable* Sender, Trigger* parameters)
{
	bool valid=true;

	ieDword value1 = CheckVariable(Sender, parameters->string0Parameter, parameters->string1Parameter, &valid );
	if (valid && value1) {
		if ( value1 > core->GetGame()->RealTime ) return 1;
	}
	return 0;
}

int GameScript::GlobalTimerExact(Scriptable* Sender, Trigger* parameters)
{
	bool valid=true;

	ieDword value1 = CheckVariable(Sender, parameters->string0Parameter, parameters->string1Parameter, &valid );
	if (valid) {
		if ( value1 == core->GetGame()->GameTime ) return 1;
	}
	return 0;
}

int GameScript::GlobalTimerExpired(Scriptable* Sender, Trigger* parameters)
{
	bool valid=true;

	ieDword value1 = CheckVariable(Sender, parameters->string0Parameter, parameters->string1Parameter, &valid );
	if (valid && value1) {
		if ( value1 < core->GetGame()->GameTime ) return 1;
	}
	return 0;
}

//globaltimernotexpired returns false if the timer doesn't exist
int GameScript::GlobalTimerNotExpired(Scriptable* Sender, Trigger* parameters)
{
	bool valid=true;

	ieDword value1 = CheckVariable(Sender, parameters->string0Parameter, parameters->string1Parameter, &valid );
	if (valid && value1) {
	 	if ( value1 > core->GetGame()->GameTime ) return 1;
	}
	return 0;
}

//globaltimerstarted returns false if the timer doesn't exist
//is it the same as globaltimernotexpired?
int GameScript::GlobalTimerStarted(Scriptable* Sender, Trigger* parameters)
{
	bool valid=true;

	ieDword value1 = CheckVariable(Sender, parameters->string0Parameter, parameters->string1Parameter, &valid );
	if (valid && value1) {
		if ( value1 > core->GetGame()->GameTime ) return 1;
	}
	return 0;
}

int GameScript::WasInDialog(Scriptable* Sender, Trigger* /*parameters*/)
{
	return Sender->MatchTrigger(trigger_wasindialog);
}

int GameScript::OnCreation(Scriptable* Sender, Trigger* /*parameters*/)
{
	return Sender->MatchTrigger(trigger_oncreation);
}

int GameScript::NumItemsParty(Scriptable* /*Sender*/, Trigger* parameters)
{
	int cnt = 0;
	Game *game=core->GetGame();

	int i = game->GetPartySize(true);
	while(i--) {
		Actor *actor = game->GetPC(i, true);
		cnt+=actor->inventory.CountItems(parameters->string0Parameter,1);
	}
	return cnt==parameters->int0Parameter;
}

int GameScript::NumItemsPartyGT(Scriptable* /*Sender*/, Trigger* parameters)
{
	int cnt = 0;
	Game *game=core->GetGame();

	int i = game->GetPartySize(true);
	while(i--) {
		Actor *actor = game->GetPC(i, true);
		cnt+=actor->inventory.CountItems(parameters->string0Parameter,1);
	}
	return cnt>parameters->int0Parameter;
}

int GameScript::NumItemsPartyLT(Scriptable* /*Sender*/, Trigger* parameters)
{
	int cnt = 0;
	Game *game=core->GetGame();

	int i = game->GetPartySize(true);
	while(i--) {
		Actor *actor = game->GetPC(i, true);
		cnt+=actor->inventory.CountItems(parameters->string0Parameter,1);
	}
	return cnt<parameters->int0Parameter;
}

int GameScript::NumItems(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar) {
		return 0;
	}

	Inventory *inv = NULL;
	switch (tar->Type) {
		case ST_ACTOR:
			inv = &(((Actor *) tar)->inventory);
			break;
		case ST_CONTAINER:
			inv = &(((Container *) tar)->inventory);
			break;
		default:;
	}
	if (!inv) {
		return 0;
	}

	int cnt = inv->CountItems(parameters->string0Parameter,1);
	return cnt==parameters->int0Parameter;
}

int GameScript::TotalItemCnt(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if ( !tar || tar->Type!=ST_ACTOR) {
		return 0;
	}
	Actor *actor = (Actor *) tar;
	int cnt = actor->inventory.CountItems("",1); //shall we count heaps or not?
	return cnt==parameters->int0Parameter;
}

int GameScript::TotalItemCntExclude(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if ( !tar || tar->Type!=ST_ACTOR) {
		return 0;
	}
	Actor *actor = (Actor *) tar;
	int cnt = actor->inventory.CountItems("",1)-actor->inventory.CountItems(parameters->string0Parameter,1); //shall we count heaps or not?
	return cnt==parameters->int0Parameter;
}

int GameScript::NumItemsGT(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar) {
		return 0;
	}

	Inventory *inv = NULL;
	switch (tar->Type) {
		case ST_ACTOR:
			inv = &(((Actor *) tar)->inventory);
			break;
		case ST_CONTAINER:
			inv = &(((Container *) tar)->inventory);
			break;
		default:;
	}
	if (!inv) {
		return 0;
	}

	int cnt = inv->CountItems(parameters->string0Parameter,1);
	return cnt>parameters->int0Parameter;
}

int GameScript::TotalItemCntGT(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if ( !tar || tar->Type!=ST_ACTOR) {
		return 0;
	}
	Actor *actor = (Actor *) tar;
	int cnt = actor->inventory.CountItems("",1); //shall we count heaps or not?
	return cnt>parameters->int0Parameter;
}

int GameScript::TotalItemCntExcludeGT(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if ( !tar || tar->Type!=ST_ACTOR) {
		return 0;
	}
	Actor *actor = (Actor *) tar;
	int cnt = actor->inventory.CountItems("",1)-actor->inventory.CountItems(parameters->string0Parameter,1); //shall we count heaps or not?
	return cnt>parameters->int0Parameter;
}

int GameScript::NumItemsLT(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar) {
		return 0;
	}

	Inventory *inv = NULL;
	switch (tar->Type) {
		case ST_ACTOR:
			inv = &(((Actor *) tar)->inventory);
			break;
		case ST_CONTAINER:
			inv = &(((Container *) tar)->inventory);
			break;
		default:;
	}
	if (!inv) {
		return 0;
	}

	int cnt = inv->CountItems(parameters->string0Parameter,1);
	return cnt<parameters->int0Parameter;
}

int GameScript::TotalItemCntLT(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if ( !tar || tar->Type!=ST_ACTOR) {
		return 0;
	}
	Actor *actor = (Actor *) tar;
	int cnt = actor->inventory.CountItems("",1); //shall we count heaps or not?
	return cnt<parameters->int0Parameter;
}

int GameScript::TotalItemCntExcludeLT(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if ( !tar || tar->Type!=ST_ACTOR) {
		return 0;
	}
	Actor *actor = (Actor *) tar;
	int cnt = actor->inventory.CountItems("",1)-actor->inventory.CountItems(parameters->string0Parameter,1); //shall we count heaps or not?
	return cnt<parameters->int0Parameter;
}

//the int0 parameter is an addition, normally it is 0
int GameScript::Contains(Scriptable* Sender, Trigger* parameters)
{
//actually this should be a container
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if ( !tar || tar->Type!=ST_CONTAINER) {
		return 0;
	}
	Container *cnt = (Container *) tar;
	if (HasItemCore(&cnt->inventory, parameters->string0Parameter, parameters->int0Parameter) ) {
		return 1;
	}
	return 0;
}

int GameScript::StoreHasItem(Scriptable* /*Sender*/, Trigger* parameters)
{
	return StoreHasItemCore(parameters->string0Parameter, parameters->string1Parameter);
}

//the int0 parameter is an addition, normally it is 0
int GameScript::HasItem(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if ( !scr ) {
		return 0;
	}
	Inventory *inventory;
	switch (scr->Type) {
		case ST_ACTOR:
			inventory = &( (Actor *) scr)->inventory;
			break;
		case ST_CONTAINER:
			inventory = &( (Container *) scr)->inventory;
			break;
		default:
			inventory = NULL;
			break;
	}
	if (inventory && HasItemCore(inventory, parameters->string0Parameter, parameters->int0Parameter) ) {
		return 1;
	}
	return 0;
}

int GameScript::ItemIsIdentified(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if ( !scr || scr->Type!=ST_ACTOR) {
		return 0;
	}
	Actor *actor = (Actor *) scr;
	if (HasItemCore(&actor->inventory, parameters->string0Parameter, IE_INV_ITEM_IDENTIFIED) ) {
		return 1;
	}
	return 0;
}

/** if the string is zero, then it will return true if there is any item in the slot (BG2)*/
/** if the string is non-zero, it will return true, if the given item was in the slot (IWD2)*/
int GameScript::HasItemSlot(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if ( !scr || scr->Type!=ST_ACTOR) {
		return 0;
	}
	Actor *actor = (Actor *) scr;
	//this might require a conversion of the slots
	if (actor->inventory.HasItemInSlot(parameters->string0Parameter, parameters->int0Parameter) ) {
		return 1;
	}
	return 0;
}

//this is a GemRB extension
//HasItemTypeSlot(Object, SLOT, ItemType)
//returns true if the item in SLOT is of ItemType
int GameScript::HasItemTypeSlot(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if ( !scr || scr->Type!=ST_ACTOR) {
		return 0;
	}
	Inventory *inv = &((Actor *) scr)->inventory;
	if (parameters->int0Parameter>=inv->GetSlotCount()) {
		return 0;
	}
	CREItem *slot = inv->GetSlotItem(parameters->int0Parameter);
	if (!slot) {
		return 0;
	}
	Item *itm = gamedata->GetItem(slot->ItemResRef);
	int itemtype = itm->ItemType;
	gamedata->FreeItem(itm, slot->ItemResRef, 0);
	if (itemtype==parameters->int1Parameter) {
		return 1;
	}
	return 0;
}

int GameScript::HasItemEquipped(Scriptable * Sender, Trigger* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if ( !scr || scr->Type!=ST_ACTOR) {
		return 0;
	}
	Actor *actor = (Actor *) scr;
	if (actor->inventory.HasItem(parameters->string0Parameter, IE_INV_ITEM_EQUIPPED) ) {
		return 1;
	}
	return 0;
}

int GameScript::Acquired(Scriptable * Sender, Trigger* parameters)
{
	if ( Sender->Type!=ST_ACTOR) {
		return 0;
	}
	Actor *actor = (Actor *) Sender;
	if (actor->inventory.HasItem(parameters->string0Parameter, IE_INV_ITEM_ACQUIRED) ) {
		return 1;
	}
	return 0;
}

/** this trigger accepts a numeric parameter, this number is the same as inventory flags
 like: 1 - identified, 2 - unstealable, 4 - stolen, 8 - undroppable, etc. */
/** this is a GemRB extension */
int GameScript::PartyHasItem(Scriptable * /*Sender*/, Trigger* parameters)
{
	Game *game=core->GetGame();

	int i = game->GetPartySize(true);
	while(i--) {
		Actor *actor = game->GetPC(i, true);
		if (HasItemCore(&actor->inventory, parameters->string0Parameter, parameters->int0Parameter) ) {
			return 1;
		}
	}
	return 0;
}

int GameScript::PartyHasItemIdentified(Scriptable * /*Sender*/, Trigger* parameters)
{
	Game *game=core->GetGame();

	int i = game->GetPartySize(true);
	while(i--) {
		Actor *actor = game->GetPC(i, true);
		if (HasItemCore(&actor->inventory, parameters->string0Parameter, IE_INV_ITEM_IDENTIFIED) ) {
			return 1;
		}
	}
	return 0;
}

int GameScript::InventoryFull( Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar || tar->Type!=ST_ACTOR) {
		return 0;
	}
	Actor *actor = (Actor *) tar;
	if (actor->inventory.FindCandidateSlot( SLOT_INVENTORY, 0 )==-1) {
		return 1;
	}
	return 0;
}

int GameScript::HasInnateAbility(Scriptable *Sender, Trigger *parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar || tar->Type!=ST_ACTOR) {
		return 0;
	}
	Actor *actor = (Actor *) tar;
	if (parameters->string0Parameter[0]) {
		return actor->spellbook.HaveSpell(parameters->string0Parameter, 0);
	}
	return actor->spellbook.HaveSpell(parameters->int0Parameter, 0);
}

int GameScript::HaveSpell(Scriptable *Sender, Trigger *parameters)
{
	if (Sender->Type!=ST_ACTOR) {
		return 0;
	}
	Actor *actor = (Actor *) Sender;
	if (parameters->string0Parameter[0]) {
		return actor->spellbook.HaveSpell(parameters->string0Parameter, 0);
	}
	return actor->spellbook.HaveSpell(parameters->int0Parameter, 0);
}

int GameScript::HaveAnySpells(Scriptable* Sender, Trigger* /*parameters*/)
{
	if (Sender->Type!=ST_ACTOR) {
		return 0;
	}
	Actor *actor = (Actor *) Sender;
	return actor->spellbook.HaveSpell("", 0);
}

int GameScript::HaveSpellParty(Scriptable* /*Sender*/, Trigger *parameters)
{
	Game *game=core->GetGame();

	int i = game->GetPartySize(true);

	if (parameters->string0Parameter[0]) {
		while(i--) {
			Actor *actor = game->GetPC(i, true);
			if (actor->spellbook.HaveSpell(parameters->string0Parameter, 0) ) {
				return 1;
			}
		}
	} else {
		while(i--) {
			Actor *actor = game->GetPC(i, true);
			if (actor->spellbook.HaveSpell(parameters->int0Parameter, 0) ) {
				return 1;
			}
		}
	}
	return 0;
}

int GameScript::KnowSpell(Scriptable *Sender, Trigger *parameters)
{
	if (Sender->Type!=ST_ACTOR) {
		return 0;
	}
	Actor *actor = (Actor *) Sender;
	if (parameters->string0Parameter[0]) {
		return actor->spellbook.KnowSpell(parameters->string0Parameter);
	}
	return actor->spellbook.KnowSpell(parameters->int0Parameter);
}

int GameScript::True(Scriptable * /* Sender*/, Trigger * /*parameters*/)
{
	return 1;
}

//in fact this could be used only on Sender, but we want to enhance these
//triggers and actions to accept an object argument whenever possible.
//0 defaults to Myself (Sender)
int GameScript::NumTimesTalkedTo(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if (!scr) {
		scr = Sender;
	}
	if (scr->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) scr;
	return actor->TalkCount == (ieDword) parameters->int0Parameter ? 1 : 0;
}

int GameScript::NumTimesTalkedToGT(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if (!scr) {
		scr = Sender;
	}
	if (scr->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) scr;
	return actor->TalkCount > (ieDword) parameters->int0Parameter ? 1 : 0;
}

int GameScript::NumTimesTalkedToLT(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if (!scr) {
		scr = Sender;
	}
	if (scr->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) scr;
	return actor->TalkCount < (ieDword) parameters->int0Parameter ? 1 : 0;
}

int GameScript::NumTimesInteracted(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if (!scr) {
		scr = Sender;
	}
	if (scr->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) scr;
	ieDword npcid = parameters->int0Parameter;
	if (npcid>=MAX_INTERACT) return 0;
	if (!actor->PCStats) return 0;
	return actor->PCStats->Interact[npcid] == (ieDword) parameters->int1Parameter ? 1 : 0;
}

int GameScript::NumTimesInteractedGT(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if (!scr) {
		scr = Sender;
	}
	if (scr->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) scr;
	ieDword npcid = parameters->int0Parameter;
	if (npcid>=MAX_INTERACT) return 0;
	if (!actor->PCStats) return 0;
	return actor->PCStats->Interact[npcid] > (ieDword) parameters->int1Parameter ? 1 : 0;
}

int GameScript::NumTimesInteractedLT(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if (!scr) {
		scr = Sender;
	}
	if (scr->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) scr;
	ieDword npcid = parameters->int0Parameter;
	if (npcid>=MAX_INTERACT) return 0;
	if (!actor->PCStats) return 0;
	return actor->PCStats->Interact[npcid] < (ieDword) parameters->int1Parameter ? 1 : 0;
}

//GemRB specific
//interacting npc counts were restricted to 24
//gemrb will increase a local variable in the interacting npc, with the scriptname of the
//target npc
int GameScript::NumTimesInteractedObject(Scriptable* Sender, Trigger* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		return 0;
	}

	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if (!scr) {
		return 0;
	}
	if (scr->Type != ST_ACTOR) {
		return 0;
	}
	Actor* tar = ( Actor* ) scr;
	return CheckVariable(Sender, tar->GetScriptName(), "LOCALS") == (ieDword) parameters->int0Parameter ? 1 : 0;
}

int GameScript::NumTimesInteractedObjectGT(Scriptable* Sender, Trigger* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		return 0;
	}

	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if (!scr) {
		return 0;
	}
	if (scr->Type != ST_ACTOR) {
		return 0;
	}
	Actor* tar = ( Actor* ) scr;
	return CheckVariable(Sender, tar->GetScriptName(), "LOCALS") > (ieDword) parameters->int0Parameter ? 1 : 0;
}

int GameScript::NumTimesInteractedObjectLT(Scriptable* Sender, Trigger* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		return 0;
	}

	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if (!scr) {
		return 0;
	}
	if (scr->Type != ST_ACTOR) {
		return 0;
	}
	Actor* tar = ( Actor* ) scr;
	return CheckVariable(Sender, tar->GetScriptName(), "LOCALS") < (ieDword) parameters->int0Parameter ? 1 : 0;
}

int GameScript::ObjectActionListEmpty(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if (!scr) {
		return 0;
	}

	// added CurrentAction as part of blocking action fixes
	if (scr->GetCurrentAction() || scr->GetNextAction()) {
		return 0;
	}
	return 1;
}

int GameScript::ActionListEmpty(Scriptable* Sender, Trigger* /*parameters*/)
{
	// added CurrentAction as part of blocking action fixes
	if (Sender->GetCurrentAction() || Sender->GetNextAction()) {
		return 0;
	}
	return 1;
}

int GameScript::False(Scriptable* /*Sender*/, Trigger* /*parameters*/)
{
	return 0;
}

/* i guess this is a range of circle edges (instead of centers) */
int GameScript::PersonalSpaceDistance(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if (!scr) {
		return 0;
	}
	int range = parameters->int0Parameter;

	int distance = PersonalDistance(Sender, scr);
	if (distance <= ( range * 10 )) {
		return 1;
	}
	return 0;
}

int GameScript::Range(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if (!scr) {
		return 0;
	}
	if (Sender->GetCurrentArea() != scr->GetCurrentArea()) {
		return 0;
	}
	int distance = SquaredMapDistance(Sender, scr);
	return DiffCore(distance, (parameters->int0Parameter+1)*(parameters->int0Parameter+1), parameters->int1Parameter);
}

int GameScript::InLine(Scriptable* Sender, Trigger* parameters)
{
	Map *map = Sender->GetCurrentArea();
	if (!map) {
		return 0;
	}

	Scriptable* scr1 = GetActorFromObject( Sender, parameters->objectParameter );
	if (!scr1) {
		return 0;
	}

	//looking for a scriptable by scriptname only
	Scriptable* scr2 = map->GetActor( parameters->string0Parameter, 0 );
	if (!scr2) {
		scr2 = GetActorObject(map->GetTileMap(), parameters->string0Parameter);
	}
	if (!scr2) {
		return 0;
	}

	double fdm1 = SquaredDistance(Sender, scr1);
	double fdm2 = SquaredDistance(Sender, scr2);
	double fd12 = SquaredDistance(scr1, scr2);
	double dm1 = sqrt(fdm1);
	double dm2 = sqrt(fdm2);

	if (fdm1>fdm2 || fd12>fdm2) {
		return 0;
	}
	double angle = acos(( fdm2 + fdm1 - fd12 ) / (2*dm1*dm2));
	if (angle*180.0*M_PI<30.0) return 1;
	return 0;
}

//PST
int GameScript::AtLocation( Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar) {
		return 0;
	}
	if ( (tar->Pos.x==parameters->pointParameter.x) &&
		(tar->Pos.y==parameters->pointParameter.y) ) {
		return 1;
	}
	return 0;
}

//in pst this is a point
//in iwd2 this is not a point
int GameScript::NearLocation(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if (!scr) {
		return 0;
	}
	if (parameters->pointParameter.isnull()) {
		Point p((short) parameters->int0Parameter, (short) parameters->int1Parameter);
		int distance = PersonalDistance(p, scr);
		if (distance <= ( parameters->int2Parameter * 10 )) {
			return 1;
		}
		return 0;
	}
	//personaldistance is needed for modron constructs in PST maze
	int distance = PersonalDistance(parameters->pointParameter, scr);
	if (distance <= ( parameters->int0Parameter * 10 )) {
		return 1;
	}
	return 0;
}

int GameScript::NearSavedLocation(Scriptable* Sender, Trigger* parameters)
{
	if (Sender->Type!=ST_ACTOR) {
		return 0;
	}
	if (core->HasFeature(GF_HAS_KAPUTZ)) {
		// we don't understand how this works in pst yet
		return 1;
	}
	Actor *actor = (Actor *) Sender;
	Point p( (short) actor->GetStat(IE_SAVEDXPOS), (short) actor->GetStat(IE_SAVEDYPOS) );
	// should this be PersonalDistance?
	int distance = Distance(p, Sender);
	if (distance <= ( parameters->int0Parameter * 10 )) {
		return 1;
	}
	return 0;
}

int GameScript::Or(Scriptable* /*Sender*/, Trigger* parameters)
{
	return parameters->int0Parameter;
}

int GameScript::TriggerTrigger(Scriptable* Sender, Trigger* parameters)
{
	return Sender->MatchTrigger(trigger_trigger, parameters->int0Parameter);
}

int GameScript::WalkedToTrigger(Scriptable* Sender, Trigger* parameters)
{
	return Sender->MatchTriggerWithObject(trigger_walkedtotrigger, parameters->objectParameter);
}

int GameScript::Clicked(Scriptable* Sender, Trigger* parameters)
{
	return Sender->MatchTriggerWithObject(trigger_clicked, parameters->objectParameter);
}

int GameScript::Disarmed(Scriptable* Sender, Trigger* parameters)
{
	return Sender->MatchTriggerWithObject(trigger_disarmed, parameters->objectParameter);
}

//stealing from a store failed, owner triggered
int GameScript::StealFailed(Scriptable* Sender, Trigger* parameters)
{
	return Sender->MatchTriggerWithObject(trigger_disarmfailed, parameters->objectParameter);
}

int GameScript::PickpocketFailed(Scriptable* Sender, Trigger* parameters)
{
	return Sender->MatchTriggerWithObject(trigger_pickpocketfailed, parameters->objectParameter);
}

int GameScript::PickLockFailed(Scriptable* Sender, Trigger* parameters)
{
	return Sender->MatchTriggerWithObject(trigger_picklockfailed, parameters->objectParameter);
}

int GameScript::OpenFailed(Scriptable* Sender, Trigger* parameters)
{
	return Sender->MatchTriggerWithObject(trigger_failedtoopen, parameters->objectParameter);
}

int GameScript::DisarmFailed(Scriptable* Sender, Trigger* parameters)
{
	return Sender->MatchTriggerWithObject(trigger_disarmfailed, parameters->objectParameter);
}

//opened for doors/containers
int GameScript::Opened(Scriptable* Sender, Trigger* parameters)
{
	return Sender->MatchTriggerWithObject(trigger_opened, parameters->objectParameter);
}

int GameScript::HarmlessOpened(Scriptable* Sender, Trigger* parameters)
{
	return Sender->MatchTriggerWithObject(trigger_harmlessopened, parameters->objectParameter);
}

//closed for doors
int GameScript::Closed(Scriptable* Sender, Trigger* parameters)
{
	return Sender->MatchTriggerWithObject(trigger_closed, parameters->objectParameter);
}

int GameScript::HarmlessClosed(Scriptable* Sender, Trigger* parameters)
{
	return Sender->MatchTriggerWithObject(trigger_harmlessclosed, parameters->objectParameter);
}

//unlocked for doors/containers (using lastUnlocked)
int GameScript::Unlocked(Scriptable* Sender, Trigger* parameters)
{
	return Sender->MatchTriggerWithObject(trigger_unlocked, parameters->objectParameter);
}

int GameScript::Entered(Scriptable* Sender, Trigger* parameters)
{
	return Sender->MatchTriggerWithObject(trigger_entered, parameters->objectParameter);
}

int GameScript::HarmlessEntered(Scriptable* Sender, Trigger* parameters)
{
	return Sender->MatchTriggerWithObject(trigger_harmlessentered, parameters->objectParameter);
}

int GameScript::IsOverMe(Scriptable* Sender, Trigger* parameters)
{
	if (Sender->Type != ST_PROXIMITY) {
		return 0;
	}
	Highlightable *trap = (Highlightable *)Sender;

	Targets *tgts = GetAllObjects(Sender->GetCurrentArea(), Sender, parameters->objectParameter, GA_NO_DEAD);
	int ret = 0;
	if (tgts) {
		targetlist::iterator m;
		const targettype *tt = tgts->GetFirstTarget(m, ST_ACTOR);
		while (tt) {
			Actor *actor = (Actor *) tt->actor;
			if (trap->IsOver(actor->Pos)) {
				ret = 1;
				break;
			}
			tt = tgts->GetNextTarget(m, ST_ACTOR);
		}
	}
	delete tgts;
	return ret;
}

//this function is different in every engines, if you use a string0parameter
//then it will be considered as a variable check
//you can also use an object parameter (like in iwd)
int GameScript::Dead(Scriptable* Sender, Trigger* parameters)
{
	if (parameters->string0Parameter[0]) {
		ieDword value;
		ieVariable Variable;

		if (core->HasFeature( GF_HAS_KAPUTZ )) {
			value = CheckVariable( Sender, parameters->string0Parameter, "KAPUTZ");
		} else {
			snprintf( Variable, 32, core->GetDeathVarFormat(), parameters->string0Parameter );
		}
		value = CheckVariable( Sender, Variable, "GLOBAL" );
		if (value>0) {
			return 1;
		}
		return 0;
	}
	Scriptable* target = GetActorFromObject( Sender, parameters->objectParameter );
	if (!target) {
		return 1;
	}
	if (target->Type != ST_ACTOR) {
		return 1;
	}
	Actor* actor = ( Actor* ) target;
	if (actor->GetStat( IE_STATE_ID ) & STATE_DEAD) {
		return 1;
	}
	return 0;
}

int GameScript::CreatureHidden(Scriptable* Sender, Trigger* /*parameters*/)
{
	if (Sender->Type!=ST_ACTOR) {
		return 0;
	}
	Actor *act=(Actor *) Sender;

	//this stuff is not completely clear, but HoW has a flag for this
	//and GemRB uses the avatarremoval stat for it.
	//HideCreature also sets this stat, so...
	if (act->GetStat(IE_AVATARREMOVAL)) {
		return 1;
	}

	if (act->GetInternalFlag()&IF_VISIBLE) {
		return 0;
	}
	return 1;
}
int GameScript::BecameVisible(Scriptable* Sender, Trigger* /*parameters*/)
{
	return Sender->MatchTrigger(trigger_becamevisible);
}

int GameScript::Die(Scriptable* Sender, Trigger* /*parameters*/)
{
	return Sender->MatchTrigger(trigger_die);
}

int GameScript::Died(Scriptable* Sender, Trigger* parameters)
{
	return Sender->MatchTriggerWithObject(trigger_died, parameters->objectParameter);
}

int GameScript::PartyMemberDied(Scriptable* Sender, Trigger* parameters)
{
	return Sender->MatchTriggerWithObject(trigger_partymemberdied, parameters->objectParameter);
}

int GameScript::NamelessBitTheDust(Scriptable* Sender, Trigger* /*parameters*/)
{
	return Sender->MatchTrigger(trigger_namelessbitthedust);
}

int GameScript::Killed(Scriptable* Sender, Trigger* parameters)
{
	return Sender->MatchTriggerWithObject(trigger_killed, parameters->objectParameter);
}

int GameScript::Race(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if (!scr) {
		return 0;
	}
	if (scr->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) scr;
	return ID_Race(actor, parameters->int0Parameter);
}

int GameScript::Gender(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if (!scr) {
		return 0;
	}
	if (scr->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) scr;
	return ID_Gender(actor, parameters->int0Parameter);
}

int GameScript::HP(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if (!scr) {
		return 0;
	}
	if (scr->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) scr;
	if ((signed) actor->GetBase( IE_HITPOINTS ) == parameters->int0Parameter) {
		return 1;
	}
	return 0;
}

int GameScript::HPGT(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if (!scr) {
		return 0;
	}
	if (scr->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) scr;
	if ( (signed) actor->GetBase( IE_HITPOINTS ) > parameters->int0Parameter) {
		return 1;
	}
	return 0;
}

int GameScript::HPLT(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if (!scr) {
		return 0;
	}
	if (scr->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) scr;
	if ( (signed) actor->GetBase( IE_HITPOINTS ) < parameters->int0Parameter) {
		return 1;
	}
	return 0;
}

//these triggers work on the current damage (not the last damage)
//actually, they use lastdamage
int GameScript::DamageTaken(Scriptable* Sender, Trigger* parameters)
{
	if (Sender->Type!=ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) Sender;
	int damage = actor->LastDamage;
	if (damage==(int) parameters->int0Parameter) {
		return 1;
	}
	return 0;
}

int GameScript::DamageTakenGT(Scriptable* Sender, Trigger* parameters)
{
	if (Sender->Type!=ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) Sender;
	int damage = actor->LastDamage;
	if (damage>(int) parameters->int0Parameter) {
		return 1;
	}
	return 0;
}

int GameScript::DamageTakenLT(Scriptable* Sender, Trigger* parameters)
{
	if (Sender->Type!=ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) Sender;
	int damage = actor->LastDamage;
	if (damage<(int) parameters->int0Parameter) {
		return 1;
	}
	return 0;
}

int GameScript::HPLost(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if (!scr) {
		return 0;
	}
	if (scr->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) scr;
	//max-current
	if ( (signed) actor->GetStat(IE_MAXHITPOINTS)-(signed) actor->GetBase( IE_HITPOINTS ) == (signed) parameters->int0Parameter) {
		return 1;
	}
	return 0;
}

int GameScript::HPLostGT(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if (!scr) {
		return 0;
	}
	if (scr->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) scr;
	//max-current
	if ( (signed) actor->GetStat(IE_MAXHITPOINTS)-(signed) actor->GetBase( IE_HITPOINTS ) > (signed) parameters->int0Parameter) {
		return 1;
	}
	return 0;
}

int GameScript::HPLostLT(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if (!scr) {
		return 0;
	}
	if (scr->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) scr;
	//max-current
	if ( (signed) actor->GetStat(IE_MAXHITPOINTS)-(signed) actor->GetBase( IE_HITPOINTS ) < (signed) parameters->int0Parameter) {
		return 1;
	}
	return 0;
}

int GameScript::HPPercent(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if (!scr) {
		return 0;
	}
	if (GetHPPercent( scr ) == parameters->int0Parameter) {
		return 1;
	}
	return 0;
}

int GameScript::HPPercentGT(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if (!scr) {
		return 0;
	}
	if (GetHPPercent( scr ) > parameters->int0Parameter) {
		return 1;
	}
	return 0;
}

int GameScript::HPPercentLT(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if (!scr) {
		return 0;
	}
	if (GetHPPercent( scr ) < parameters->int0Parameter) {
		return 1;
	}
	return 0;
}

int GameScript::XP(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if (!scr) {
		return 0;
	}
	if (scr->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) scr;
	if (actor->GetStat( IE_XP ) == (unsigned) parameters->int0Parameter) {
		return 1;
	}
	return 0;
}

int GameScript::XPGT(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if (!scr) {
		return 0;
	}
	if (scr->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) scr;
	if (actor->GetStat( IE_XP ) > (unsigned) parameters->int0Parameter) {
		return 1;
	}
	return 0;
}

int GameScript::XPLT(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if (!scr) {
		return 0;
	}
	if (scr->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) scr;
	if (actor->GetStat( IE_XP ) < (unsigned) parameters->int0Parameter) {
		return 1;
	}
	return 0;
}

int GameScript::CheckSkill(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* target = GetActorFromObject( Sender, parameters->objectParameter );
	if (!target) {
		return 0;
	}
	if (target->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) target;
	int sk = actor->GetSkill( parameters->int1Parameter );
	if (sk<0) return 0;
	if ( sk == parameters->int0Parameter) {
		return 1;
	}
	return 0;
}
int GameScript::CheckStat(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* target = GetActorFromObject( Sender, parameters->objectParameter );
	if (!target) {
		return 0;
	}
	if (target->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) target;
	if ( (signed) actor->GetStat( parameters->int1Parameter ) == parameters->int0Parameter) {
		return 1;
	}
	return 0;
}

int GameScript::CheckSkillGT(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar || tar->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) tar;
	int sk = actor->GetSkill( parameters->int1Parameter );
	if (sk<0) return 0;
	if ( sk > parameters->int0Parameter) {
		return 1;
	}
	return 0;
}

int GameScript::CheckStatGT(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar || tar->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) tar;
	if ( (signed) actor->GetStat( parameters->int1Parameter ) > parameters->int0Parameter) {
		return 1;
	}
	return 0;
}

int GameScript::CheckSkillLT(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar || tar->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) tar;
	int sk = actor->GetSkill( parameters->int1Parameter );
	if (sk<0) return 0;
	if ( sk < parameters->int0Parameter) {
		return 1;
	}
	return 0;
}

int GameScript::CheckStatLT(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar || tar->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) tar;
	if ( (signed) actor->GetStat( parameters->int1Parameter ) < parameters->int0Parameter) {
		return 1;
	}
	return 0;
}

/* i believe this trigger is the same as 'MarkObject' action
 except that if it cannot set the marked object, it returns false */
int GameScript::SetLastMarkedObject(Scriptable* Sender, Trigger* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		return 0;
	}
	Actor *scr = (Actor *) Sender;

	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar || tar->Type != ST_ACTOR) {
		return 0;
	}
	scr->LastMarked = tar->GetGlobalID();
	return 1;
}

int GameScript::IsSpellTargetValid(Scriptable* Sender, Trigger* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		return 0;
	}
	Actor *scr = (Actor *) Sender;

	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	Actor *actor = NULL;
	if (tar->Type == ST_ACTOR) {
		actor = (Actor *) tar;
	}

	int flags = parameters->int1Parameter;
	if (!(flags & MSO_IGNORE_NULL) && !actor) {
		return 0;
	}
	if (!(flags & MSO_IGNORE_INVALID) && actor && actor->InvalidSpellTarget() ) {
		return 0;
	}
	int splnum = parameters->int0Parameter;
	if (!(flags & MSO_IGNORE_HAVE) && !scr->spellbook.HaveSpell(splnum, 0) ) {
		return 0;
	}
	int range;
	if ((flags & MSO_IGNORE_RANGE) || !actor) {
		range = 0;
	} else {
		range = Distance(scr, actor);
	}
	if (!(flags & MSO_IGNORE_INVALID) && actor->InvalidSpellTarget(splnum, scr, range)) {
		return 0;
	}
	return 1;
}

//This trigger seems to always return true for actors...
//Always manages to set spell to 0, otherwise it sets if there was nothing set earlier
int GameScript::SetMarkedSpell_Trigger(Scriptable* Sender, Trigger* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		return 0;
	}
	Actor *scr = (Actor *) Sender;
	if (parameters->int0Parameter) {
		if (scr->LastMarkedSpell) {
			return 1;
		}
		if (!scr->spellbook.HaveSpell(parameters->int0Parameter, 0) ) {
			return 1;
		}
	}

	//TODO: check if spell exists (not really important)
	scr->LastMarkedSpell = parameters->int0Parameter;
	return 1;
}

int GameScript::ForceMarkedSpell_Trigger(Scriptable* Sender, Trigger* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		return 0;
	}
	Actor *scr = (Actor *) Sender;
	scr->LastMarkedSpell = parameters->int0Parameter;
	return 1;
}

int GameScript::IsMarkedSpell(Scriptable* Sender, Trigger* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		return 0;
	}
	Actor *scr = (Actor *) Sender;
	return scr->LastMarkedSpell == parameters->int0Parameter;
}


int GameScript::See(Scriptable* Sender, Trigger* parameters)
{
	int see = SeeCore(Sender, parameters, 0);
	//don't mark LastSeen for clear!!!
	if (Sender->Type==ST_ACTOR && see) {
		Actor *act = (Actor *) Sender;
		//save lastseen as lastmarked
		//FIXME: what is this doing?
		act->LastMarked = act->LastSeen;
		//Sender->AddTrigger (&act->LastSeen);
	}
	return see;
}

int GameScript::Detect(Scriptable* Sender, Trigger* parameters)
{
	parameters->int0Parameter=1; //seedead/invis
	int see = SeeCore(Sender, parameters, 0);
	if (!see) {
		return 0;
	}
	return 1;
}

int GameScript::LOS(Scriptable* Sender, Trigger* parameters)
{
	int see=SeeCore(Sender, parameters, 1);
	if (!see) {
		return 0;
	}
	return Range(Sender, parameters); //same as range
}

int GameScript::NumCreatures(Scriptable* Sender, Trigger* parameters)
{
	int value = GetObjectCount(Sender, parameters->objectParameter);
	return value == parameters->int0Parameter;
}

int GameScript::NumCreaturesAtMyLevel(Scriptable* Sender, Trigger* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		return 0;
	}
	int level = ((Actor *) Sender)->GetXPLevel(true);
	int value;

	if (parameters->int0Parameter) {
		value = GetObjectLevelCount(Sender, parameters->objectParameter);
	} else {
		value = GetObjectCount(Sender, parameters->objectParameter);
	}
	return value == level;
}

int GameScript::NumCreaturesLT(Scriptable* Sender, Trigger* parameters)
{
	int value = GetObjectCount(Sender, parameters->objectParameter);
	return value < parameters->int0Parameter;
}

int GameScript::NumCreaturesLTMyLevel(Scriptable* Sender, Trigger* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		return 0;
	}
	int level = ((Actor *) Sender)->GetXPLevel(true);
	int value;

	if (parameters->int0Parameter) {
		value = GetObjectLevelCount(Sender, parameters->objectParameter);
	} else {
		value = GetObjectCount(Sender, parameters->objectParameter);
	}
	return value < level;
}

int GameScript::NumCreaturesGT(Scriptable* Sender, Trigger* parameters)
{
	int value = GetObjectCount(Sender, parameters->objectParameter);
	return value > parameters->int0Parameter;
}

int GameScript::NumCreaturesGTMyLevel(Scriptable* Sender, Trigger* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		return 0;
	}
	int level = ((Actor *) Sender)->GetXPLevel(true);
	int value;

	if (parameters->int0Parameter) {
		value = GetObjectLevelCount(Sender, parameters->objectParameter);
	} else {
		value = GetObjectCount(Sender, parameters->objectParameter);
	}
	return value > level;
}

int GameScript::NumCreatureVsParty(Scriptable* Sender, Trigger* parameters)
{
	//creating object on the spot
	if (!parameters->objectParameter) {
		parameters->objectParameter = new Object();
	}
	int value = GetObjectCount(Sender, parameters->objectParameter);
	value -= core->GetGame()->GetPartySize(true);
	return value == parameters->int0Parameter;
}

int GameScript::NumCreatureVsPartyGT(Scriptable* Sender, Trigger* parameters)
{
	if (!parameters->objectParameter) {
		parameters->objectParameter = new Object();
	}
	int value = GetObjectCount(Sender, parameters->objectParameter);
	value -= core->GetGame()->GetPartySize(true);
	return value > parameters->int0Parameter;
}

int GameScript::NumCreatureVsPartyLT(Scriptable* Sender, Trigger* parameters)
{
	if (!parameters->objectParameter) {
		parameters->objectParameter = new Object();
	}
	int value = GetObjectCount(Sender, parameters->objectParameter);
	value -= core->GetGame()->GetPartySize(true);
	return value < parameters->int0Parameter;
}

int GameScript::Morale(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar || tar->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) tar;
	return (signed) actor->GetStat(IE_MORALEBREAK) == parameters->int0Parameter;
}

int GameScript::MoraleGT(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar || tar->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) tar;
	return (signed) actor->GetStat(IE_MORALEBREAK) > parameters->int0Parameter;
}

int GameScript::MoraleLT(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar || tar->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) tar;
	return (signed) actor->GetStat(IE_MORALEBREAK) < parameters->int0Parameter;
}

int GameScript::CheckSpellState(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar || tar->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) tar;
	if (parameters->int0Parameter>255) {
		return 0;
	}
	unsigned int position = parameters->int0Parameter>>5;
	unsigned int bit = 1<<(parameters->int0Parameter&31);
	if (actor->GetStat(IE_SPLSTATE_ID1+position) & bit) {
		return 1;
	}
	return 0;
}

int GameScript::StateCheck(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar || tar->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) tar;
	if (actor->GetStat(IE_STATE_ID) & parameters->int0Parameter) {
		return 1;
	}
	return 0;
}

int GameScript::ExtendedStateCheck(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar || tar->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) tar;
	if (actor->GetStat(IE_EXTSTATE_ID) & parameters->int0Parameter) {
		return 1;
	}
	return 0;
}

int GameScript::NotStateCheck(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar || tar->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) tar;
	if (actor->GetStat(IE_STATE_ID) & ~parameters->int0Parameter) {
		return 1;
	}
	return 0;
}

int GameScript::RandomNum(Scriptable* /*Sender*/, Trigger* parameters)
{
	if (parameters->int0Parameter<0) {
		return 0;
	}
	if (parameters->int1Parameter<0) {
		return 0;
	}
	return parameters->int1Parameter-1 == RandomNumValue%parameters->int0Parameter;
}

int GameScript::RandomNumGT(Scriptable* /*Sender*/, Trigger* parameters)
{
	if (parameters->int0Parameter<0) {
		return 0;
	}
	if (parameters->int1Parameter<0) {
		return 0;
	}
	return parameters->int1Parameter-1 < RandomNumValue%parameters->int0Parameter;
}

int GameScript::RandomNumLT(Scriptable* /*Sender*/, Trigger* parameters)
{
	if (parameters->int0Parameter<0) {
		return 0;
	}
	if (parameters->int1Parameter<0) {
		return 0;
	}
	return parameters->int1Parameter-1 > RandomNumValue%parameters->int0Parameter;
}

int GameScript::OpenState(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar) {
		if (InDebug&ID_TRIGGERS) {
			Log(ERROR, "GameScript", "Couldn't find door/container:%s",
				parameters->objectParameter? parameters->objectParameter->objectName:"<NULL>");
			print("Sender: %s", Sender->GetScriptName());
		}
		return 0;
	}
	switch(tar->Type) {
		case ST_DOOR:
		{
			Door *door =(Door *) tar;
			return !door->IsOpen() == !parameters->int0Parameter;
		}
		case ST_CONTAINER:
		{
			Container *cont = (Container *) tar;
			return !(cont->Flags&CONT_LOCKED) == !parameters->int0Parameter;
		}
		default:; //to remove a warning
	}
	Log(ERROR, "GameScript", "Not a door/container:%s",
		tar->GetScriptName());
	return 0;
}

int GameScript::IsLocked(Scriptable * Sender, Trigger *parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar) {
		Log(ERROR, "GameScript", "Couldn't find door/container:%s",
			parameters->objectParameter? parameters->objectParameter->objectName:"<NULL>");
		print("Sender: %s", Sender->GetScriptName());
		return 0;
	}
	switch(tar->Type) {
		case ST_DOOR:
		{
			Door *door =(Door *) tar;
			return !!(door->Flags&DOOR_LOCKED);
		}
		case ST_CONTAINER:
		{
			Container *cont = (Container *) tar;
			return !!(cont->Flags&CONT_LOCKED);
		}
		default:; //to remove a warning
	}
	Log(ERROR, "GameScript", "Not a door/container:%s",
		tar->GetScriptName());
	return 0;
}

int GameScript::Level(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar) {
		return 0;
	}
	if (tar->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) tar;
	// FIXME: what about multiclasses or dualclasses?
	return actor->GetStat(IE_LEVEL) == (unsigned) parameters->int0Parameter;
}

//this is just a hack, actually multiclass should be available
int GameScript::ClassLevel(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar) {
		return 0;
	}
	if (tar->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) tar;
	return actor->GetLevelInClass(parameters->int0Parameter) == (unsigned) parameters->int1Parameter;
}

// iwd2 and pst have different order of parameters:
// ClassLevelGT(Protagonist,MAGE,10)
// LevelInClass(Myself,10,CLERIC)
int GameScript::LevelInClass(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar) {
		return 0;
	}
	if (tar->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) tar;
	return actor->GetLevelInClass(parameters->int1Parameter) == (unsigned) parameters->int0Parameter;
}

int GameScript::LevelGT(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar) {
		return 0;
	}
	if (tar->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) tar;
	return actor->GetStat(IE_LEVEL) > (unsigned) parameters->int0Parameter;
}

//this is just a hack, actually multiclass should be available
int GameScript::ClassLevelGT(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar) {
		return 0;
	}
	if (tar->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) tar;
	return actor->GetLevelInClass(parameters->int0Parameter) > (unsigned) parameters->int1Parameter;
}

int GameScript::LevelInClassGT(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar) {
		return 0;
	}
	if (tar->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) tar;
	return actor->GetLevelInClass(parameters->int1Parameter) > (unsigned) parameters->int0Parameter;
}

int GameScript::LevelLT(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar) {
		return 0;
	}
	if (tar->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) tar;
	return actor->GetStat(IE_LEVEL) < (unsigned) parameters->int0Parameter;
}

int GameScript::ClassLevelLT(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar) {
		return 0;
	}
	if (tar->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) tar;
	return actor->GetLevelInClass(parameters->int0Parameter) < (unsigned) parameters->int1Parameter;
}

int GameScript::LevelInClassLT(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar) {
		return 0;
	}
	if (tar->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) tar;
	return actor->GetLevelInClass(parameters->int1Parameter) < (unsigned) parameters->int0Parameter;
}

int GameScript::UnselectableVariable(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar) {
		return 0;
	}
	return tar->UnselectableTimer == (unsigned) parameters->int0Parameter;
}

int GameScript::UnselectableVariableGT(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar) {
		return 0;
	}
	return tar->UnselectableTimer > (unsigned) parameters->int0Parameter;
}

int GameScript::UnselectableVariableLT(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar) {
		return 0;
	}
	return tar->UnselectableTimer < (unsigned) parameters->int0Parameter;
}

int GameScript::AreaCheck(Scriptable* Sender, Trigger* parameters)
{
	if (!strnicmp(Sender->GetCurrentArea()->GetScriptName(), parameters->string0Parameter, 8)) {
		return 1;
	}
	return 0;
}

int GameScript::AreaCheckObject(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );

	if (!tar) {
		return 0;
	}
	if (!strnicmp(tar->GetCurrentArea()->GetScriptName(), parameters->string0Parameter, 8)) {
		return 1;
	}
	return 0;
}

//lame iwd2 uses a numeric area identifier, this reduces its usability
int GameScript::CurrentAreaIs(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );

	if (!tar) {
		return 0;
	}
	ieResRef arearesref;
	snprintf(arearesref, 8, "AR%04d", parameters->int0Parameter);
	if (!strnicmp(tar->GetCurrentArea()->GetScriptName(), arearesref, 8)) {
		return 1;
	}
	return 0;
}

//lame bg2 uses a constant areaname prefix, this reduces its usability
//but in the spirit of flexibility, gemrb extension allows arbitrary prefixes
int GameScript::AreaStartsWith(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );

	if (!tar) {
		return 0;
	}
	ieResRef arearesref;
	if (parameters->string0Parameter[0]) {
		strnlwrcpy(arearesref, parameters->string0Parameter, 8);
	} else {
		strnlwrcpy(arearesref, "AR30", 8); //InWatchersKeep
	}
	int i = strlen(arearesref);
	if (!strnicmp(tar->GetCurrentArea()->GetScriptName(), arearesref, i)) {
		return 1;
	}
	return 0;
}

int GameScript::EntirePartyOnMap(Scriptable* Sender, Trigger* /*parameters*/)
{
	Map *map = Sender->GetCurrentArea();
	Game *game=core->GetGame();
	int i=game->GetPartySize(true);
	while (i--) {
		Actor *actor=game->GetPC(i,true);
		if (actor->GetCurrentArea()!=map) {
			return 0;
		}
	}
	return 1;
}

int GameScript::AnyPCOnMap(Scriptable* Sender, Trigger* /*parameters*/)
{
	Map *map = Sender->GetCurrentArea();
	Game *game=core->GetGame();
	int i=game->GetPartySize(true);
	while (i--) {
		Actor *actor=game->GetPC(i,true);
		if (actor->GetCurrentArea()==map) {
			return 1;
		}
	}
	return 0;
}

int GameScript::InActiveArea(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar) {
		return 0;
	}
	if (core->GetGame()->GetCurrentArea() == tar->GetCurrentArea()) {
		return 1;
	}
	return 0;
}

int GameScript::InMyArea(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar) {
		return 0;
	}
	if (Sender->GetCurrentArea() == tar->GetCurrentArea()) {
		return 1;
	}
	return 0;
}

int GameScript::AreaType(Scriptable* Sender, Trigger* parameters)
{
	Map *map=Sender->GetCurrentArea();
	return (map->AreaType&parameters->int0Parameter)>0;
}

int GameScript::IsExtendedNight( Scriptable* Sender, Trigger* /*parameters*/)
{
	Map *map=Sender->GetCurrentArea();
	if (map->AreaType&AT_EXTENDED_NIGHT) {
		return 1;
	}
	return 0;
}

int GameScript::AreaFlag(Scriptable* Sender, Trigger* parameters)
{
	Map *map=Sender->GetCurrentArea();
	return (map->AreaFlags&parameters->int0Parameter)>0;
}

int GameScript::AreaRestDisabled(Scriptable* Sender, Trigger* /*parameters*/)
{
	Map *map=Sender->GetCurrentArea();
	if (map->AreaFlags&2) {
		return 1;
	}
	return 0;
}

//new optional parameter: size of actor (to reach target)
int GameScript::TargetUnreachable(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar || tar->Type != ST_ACTOR) {
		return 1; //well, if it doesn't exist it is unreachable
	}
	Map *map=Sender->GetCurrentArea();
	if (!map) {
		return 1;
	}
	unsigned int size = parameters->int0Parameter;

	if (!size) {
		if (Sender->Type==ST_ACTOR) {
			size = ((Movable *) Sender)->size;
		}
		else {
			size = 1;
		}
	}
	return map->TargetUnreachable( Sender->Pos, tar->Pos, size);
}

int GameScript::PartyCountEQ(Scriptable* /*Sender*/, Trigger* parameters)
{
	return core->GetGame()->GetPartySize(0)==parameters->int0Parameter;
}

int GameScript::PartyCountLT(Scriptable* /*Sender*/, Trigger* parameters)
{
	return core->GetGame()->GetPartySize(0)<parameters->int0Parameter;
}

int GameScript::PartyCountGT(Scriptable* /*Sender*/, Trigger* parameters)
{
	return core->GetGame()->GetPartySize(0)>parameters->int0Parameter;
}

int GameScript::PartyCountAliveEQ(Scriptable* /*Sender*/, Trigger* parameters)
{
	return core->GetGame()->GetPartySize(1)==parameters->int0Parameter;
}

int GameScript::PartyCountAliveLT(Scriptable* /*Sender*/, Trigger* parameters)
{
	return core->GetGame()->GetPartySize(1)<parameters->int0Parameter;
}

int GameScript::PartyCountAliveGT(Scriptable* /*Sender*/, Trigger* parameters)
{
	return core->GetGame()->GetPartySize(1)>parameters->int0Parameter;
}

int GameScript::LevelParty(Scriptable* /*Sender*/, Trigger* parameters)
{
	return core->GetGame()->GetPartyLevel(1)==parameters->int0Parameter;
}

int GameScript::LevelPartyLT(Scriptable* /*Sender*/, Trigger* parameters)
{
	return core->GetGame()->GetPartyLevel(1)<parameters->int0Parameter;
}

int GameScript::LevelPartyGT(Scriptable* /*Sender*/, Trigger* parameters)
{
	return core->GetGame()->GetPartyLevel(1)>parameters->int0Parameter;
}

int GameScript::PartyGold(Scriptable* /*Sender*/, Trigger* parameters)
{
	return core->GetGame()->PartyGold == (ieDword) parameters->int0Parameter;
}

int GameScript::PartyGoldGT(Scriptable* /*Sender*/, Trigger* parameters)
{
	return core->GetGame()->PartyGold > (ieDword) parameters->int0Parameter;
}

int GameScript::PartyGoldLT(Scriptable* /*Sender*/, Trigger* parameters)
{
	return core->GetGame()->PartyGold < (ieDword) parameters->int0Parameter;
}

int GameScript::OwnsFloaterMessage(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar) {
		return 0;
	}
	return tar->textDisplaying;
}

int GameScript::InCutSceneMode(Scriptable* /*Sender*/, Trigger* /*parameters*/)
{
	return core->InCutSceneMode();
}

int GameScript::Proficiency(Scriptable* Sender, Trigger* parameters)
{
	unsigned int idx = parameters->int0Parameter;
	if (idx>31) {
		return 0;
	}
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar) {
		return 0;
	}
	if (tar->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) tar;
	return (signed) actor->GetStat(IE_PROFICIENCYBASTARDSWORD+idx) == parameters->int1Parameter;
}

int GameScript::ProficiencyGT(Scriptable* Sender, Trigger* parameters)
{
	unsigned int idx = parameters->int0Parameter;
	if (idx>31) {
		return 0;
	}
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar) {
		return 0;
	}
	if (tar->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) tar;
	return (signed) actor->GetStat(IE_PROFICIENCYBASTARDSWORD+idx) > parameters->int1Parameter;
}

int GameScript::ProficiencyLT(Scriptable* Sender, Trigger* parameters)
{
	unsigned int idx = parameters->int0Parameter;
	if (idx>31) {
		return 0;
	}
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar) {
		return 0;
	}
	if (tar->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) tar;
	return (signed) actor->GetStat(IE_PROFICIENCYBASTARDSWORD+idx) < parameters->int1Parameter;
}

//this is a PST specific stat, shows how many free proficiency slots we got
//we use an unused stat for it
int GameScript::ExtraProficiency(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar) {
		return 0;
	}
	if (tar->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) tar;
	return (signed) actor->GetStat(IE_FREESLOTS) == parameters->int0Parameter;
}

int GameScript::ExtraProficiencyGT(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar) {
		return 0;
	}
	if (tar->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) tar;
	return (signed) actor->GetStat(IE_FREESLOTS) > parameters->int0Parameter;
}

int GameScript::ExtraProficiencyLT(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar) {
		return 0;
	}
	if (tar->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) tar;
	return (signed) actor->GetStat(IE_FREESLOTS) < parameters->int0Parameter;
}

int GameScript::Internal(Scriptable* Sender, Trigger* parameters)
{
	unsigned int idx = parameters->int0Parameter;
	if (idx>15) {
		return 0;
	}
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar) {
		return 0;
	}
	if (tar->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) tar;
	return (signed) actor->GetStat(IE_INTERNAL_0+idx) == parameters->int1Parameter;
}

int GameScript::InternalGT(Scriptable* Sender, Trigger* parameters)
{
	unsigned int idx = parameters->int0Parameter;
	if (idx>15) {
		return 0;
	}
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar) {
		return 0;
	}
	if (tar->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) tar;
	return (signed) actor->GetStat(IE_INTERNAL_0+idx) > parameters->int1Parameter;
}

int GameScript::InternalLT(Scriptable* Sender, Trigger* parameters)
{
	unsigned int idx = parameters->int0Parameter;
	if (idx>15) {
		return 0;
	}
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar) {
		return 0;
	}
	if (tar->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) tar;
	return (signed) actor->GetStat(IE_INTERNAL_0+idx) < parameters->int1Parameter;
}

//we check if target is currently in dialog or not
int GameScript::NullDialog(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar) {
		return 0;
	}
	if (tar->Type != ST_ACTOR) {
		return 0;
	}
	GameControl *gc = core->GetGameControl();
	if ( (tar->GetGlobalID() != gc->dialoghandler->targetID) && (tar->GetGlobalID() != gc->dialoghandler->speakerID) ) {
		return 1;
	}
	return 0;
}

//this one checks scriptname (deathvar), i hope it is right
//IsScriptName depends on this too
//Name is another (similar function)
int GameScript::CalledByName(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar) {
		return 0;
	}
	if (tar->Type != ST_ACTOR) {
		return 0;
	}
	Actor *actor = (Actor *) tar;
	if (stricmp(actor->GetScriptName(), parameters->string0Parameter) ) {
		return 0;
	}
	return 1;
}

//This is checking on the character's name as it was typed in
int GameScript::CharName(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if (!scr || scr->Type!=ST_ACTOR) {
		return 0;
	}
	Actor* actor = (Actor *) scr;
	if (!strnicmp(actor->ShortName, parameters->string0Parameter, 32) ) {
		return 1;
	}
	return 0;
}

int GameScript::AnimationID(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar) {
		return 0;
	}
	if (tar->Type != ST_ACTOR) {
		return 0;
	}
	Actor *actor = (Actor *) tar;
	if ((ieWord) actor->GetStat(IE_ANIMATION_ID) == (ieWord) parameters->int0Parameter) {
		return 1;
	}
	return 0;
}

int GameScript::AnimState(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar) {
		return 0;
	}
	if (tar->Type != ST_ACTOR) {
		return 0;
	}
	Actor *actor = (Actor *) tar;
	return actor->GetStance() == parameters->int0Parameter;
}

//this trigger uses hours
int GameScript::Time(Scriptable* /*Sender*/, Trigger* parameters)
{
	return (core->GetGame()->GameTime/AI_UPDATE_TIME)%7200/300 == (ieDword) parameters->int0Parameter;
}

//this trigger uses hours
int GameScript::TimeGT(Scriptable* /*Sender*/, Trigger* parameters)
{
	return (core->GetGame()->GameTime/AI_UPDATE_TIME)%7200/300 > (ieDword) parameters->int0Parameter;
}

//this trigger uses hours
int GameScript::TimeLT(Scriptable* /*Sender*/, Trigger* parameters)
{
	return (core->GetGame()->GameTime/AI_UPDATE_TIME)%7200/300 < (ieDword) parameters->int0Parameter;
}

int GameScript::HotKey(Scriptable* Sender, Trigger* parameters)
{
	return Sender->MatchTrigger(trigger_hotkey, parameters->int0Parameter);
}

int GameScript::CombatCounter(Scriptable* /*Sender*/, Trigger* parameters)
{
	return core->GetGame()->CombatCounter == (ieDword) parameters->int0Parameter;
}

int GameScript::CombatCounterGT(Scriptable* /*Sender*/, Trigger* parameters)
{
	return core->GetGame()->CombatCounter > (ieDword) parameters->int0Parameter;
}

int GameScript::CombatCounterLT(Scriptable* /*Sender*/, Trigger* parameters)
{
	return core->GetGame()->CombatCounter < (ieDword) parameters->int0Parameter;
}

int GameScript::TrapTriggered(Scriptable* Sender, Trigger* parameters)
{
	return Sender->MatchTriggerWithObject(trigger_traptriggered, parameters->objectParameter);
}

int GameScript::InteractingWith(Scriptable* Sender, Trigger* parameters)
{
	if (Sender->Type!=ST_ACTOR) {
		return 0;
	}
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar || tar->Type != ST_ACTOR) {
		return 0;
	}
	GameControl *gc = core->GetGameControl();
	if (Sender->GetGlobalID() != gc->dialoghandler->targetID && Sender->GetGlobalID() != gc->dialoghandler->speakerID) {
		return 0;
	}
	if (tar->GetGlobalID() != gc->dialoghandler->targetID && tar->GetGlobalID() != gc->dialoghandler->speakerID) {
		return 0;
	}
	return 1;
}

int GameScript::LastPersonTalkedTo(Scriptable* Sender, Trigger* parameters)
{
	if (Sender->Type!=ST_ACTOR) {
		return 0;
	}
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar || tar->Type != ST_ACTOR) {
		return 0;
	}
	Actor *scr = (Actor *) Sender;
	if (MatchActor(Sender, scr->LastTalker, parameters->objectParameter)) {
		return 1;
	}
	return 0;
}

int GameScript::IsRotation(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar || tar->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) tar;
	if ( actor->GetOrientation() == parameters->int0Parameter ) {
		return 1;
	}
	return 0;
}

//GemRB currently stores the saved location in a local variable, but it is
//actually stored in the .gam structure (only for PCs)
int GameScript::IsFacingSavedRotation(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar || tar->Type!=ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) tar;
	if (actor->GetOrientation() == actor->GetStat(IE_SAVEDFACE) ) {
		return 1;
	}
	return 0;
}

int GameScript::IsFacingObject(Scriptable* Sender, Trigger* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		return 0;
	}
	Scriptable* target = GetActorFromObject( Sender, parameters->objectParameter );
	if (!target) {
		return 0;
	}
	Actor* actor = ( Actor* ) Sender;
	if (actor->GetOrientation()==GetOrient( target->Pos, actor->Pos ) ) {
		return 1;
	}
	return 0;
}

int GameScript::AttackedBy(Scriptable* Sender, Trigger* parameters)
{
	return Sender->MatchTriggerWithObject(trigger_attackedby, parameters->objectParameter, parameters->int0Parameter);
}

int GameScript::TookDamage(Scriptable* Sender, Trigger* /*parameters*/)
{
	return Sender->MatchTrigger(trigger_tookdamage);
}

int GameScript::HitBy(Scriptable* Sender, Trigger* parameters)
{
	return Sender->MatchTriggerWithObject(trigger_hitby, parameters->objectParameter, parameters->int0Parameter);
}

int GameScript::Heard(Scriptable* Sender, Trigger* parameters)
{
	return Sender->MatchTriggerWithObject(trigger_heard, parameters->objectParameter, parameters->int0Parameter);
}

int GameScript::Detected(Scriptable* Sender, Trigger* parameters)
{
	return Sender->MatchTriggerWithObject(trigger_detected, parameters->objectParameter, parameters->int0Parameter);
}

int GameScript::LastMarkedObject_Trigger(Scriptable* Sender, Trigger* parameters)
{
	if (Sender->Type!=ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) Sender;
	if (MatchActor(Sender, actor->LastMarked, parameters->objectParameter)) {
		//don't mark this object for clear
		//Sender->AddTrigger(&actor->LastSeen);
		return 1;
	}
	return 0;
}

int GameScript::HelpEX(Scriptable* Sender, Trigger* parameters)
{
	if (Sender->Type!=ST_ACTOR) {
		return 0;
	}
	int stat;
	switch (parameters->int0Parameter) {
		case 1: stat = IE_EA; break;
		case 2: stat = IE_GENERAL; break;
		case 3: stat = IE_RACE; break;
		case 4: stat = IE_CLASS; break;
		case 5: stat = IE_SPECIFIC; break;
		case 6: stat = IE_SEX; break;
		case 7: stat = IE_ALIGNMENT; break;
		default: return 0;
	}
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar || tar->Type!=ST_ACTOR) {
		//a non actor checking for help?
		return 0;
	}
	Actor* actor = ( Actor* ) tar;
	Actor* help = Sender->GetCurrentArea()->GetActorByGlobalID(actor->LastHelp);
	if (!help) {
		//no help required
		return 0;
	}
	if (actor->GetStat(stat)==help->GetStat(stat) ) {
		// FIXME
		//Sender->AddTrigger(&actor->LastHelp);
		return 1;
	}
	return 0;
}

int GameScript::Help_Trigger(Scriptable* Sender, Trigger* parameters)
{
	return Sender->MatchTriggerWithObject(trigger_help, parameters->objectParameter);
}

int GameScript::ReceivedOrder(Scriptable* Sender, Trigger* parameters)
{
	return Sender->MatchTriggerWithObject(trigger_receivedorder, parameters->objectParameter, parameters->int0Parameter);
}

int GameScript::Joins(Scriptable* Sender, Trigger* parameters)
{
	return Sender->MatchTriggerWithObject(trigger_joins, parameters->objectParameter);
}

int GameScript::Leaves(Scriptable* Sender, Trigger* parameters)
{
	return Sender->MatchTriggerWithObject(trigger_leaves, parameters->objectParameter);
}

int GameScript::FallenPaladin(Scriptable* Sender, Trigger* /*parameters*/)
{
	if (Sender->Type!=ST_ACTOR) {
		return 0;
	}
	Actor* act = ( Actor* ) Sender;
	return (act->GetStat(IE_MC_FLAGS) & MC_FALLEN_PALADIN)!=0;
}

int GameScript::FallenRanger(Scriptable* Sender, Trigger* /*parameters*/)
{
	if (Sender->Type!=ST_ACTOR) {
		return 0;
	}
	Actor* act = ( Actor* ) Sender;
	return (act->GetStat(IE_MC_FLAGS) & MC_FALLEN_RANGER)!=0;
}

int GameScript::NightmareModeOn(Scriptable* /*Sender*/, Trigger* /*parameters*/)
{
	ieDword diff;

	core->GetDictionary()->Lookup("Nightmare Mode", diff);
	if (diff) {
		return 1;
	}
	return 0;
}

int GameScript::Difficulty(Scriptable* /*Sender*/, Trigger* parameters)
{
	ieDword diff;

	core->GetDictionary()->Lookup("Difficulty Level", diff);
	int mode = parameters->int1Parameter;
	//hack for compatibility
	if (!mode) {
		mode = EQUALS;
	}
	return DiffCore(diff, (ieDword) parameters->int0Parameter, mode);
}

int GameScript::DifficultyGT(Scriptable* /*Sender*/, Trigger* parameters)
{
	ieDword diff;

	core->GetDictionary()->Lookup("Difficulty Level", diff);
	return diff>(ieDword) parameters->int0Parameter;
}

int GameScript::DifficultyLT(Scriptable* /*Sender*/, Trigger* parameters)
{
	ieDword diff;

	core->GetDictionary()->Lookup("Difficulty Level", diff);
	return diff<(ieDword) parameters->int0Parameter;
}

int GameScript::Vacant(Scriptable* Sender, Trigger* /*parameters*/)
{
	if (Sender->Type!=ST_AREA) {
		return 0;
	}
	Map *map = (Map *) Sender;
	if ( map->CanFree() ) {
		return 1;
	}
	return 0;
}

//this trigger always checks the right hand weapon?
int GameScript::InWeaponRange(Scriptable* Sender, Trigger* parameters)
{
	if (Sender->Type!=ST_ACTOR) {
		return 0;
	}
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar) {
		return 0;
	}
	Actor *actor = (Actor *) Sender;
	WeaponInfo wi;
	unsigned int wrange = 0;
	ITMExtHeader *header = actor->GetWeapon(wi, false);
	if (header) {
		wrange = wi.range;
	}
	header = actor->GetWeapon(wi, true);
	if (header && (wi.range>wrange)) {
		wrange = wi.range;
	}
	if ( PersonalDistance( Sender, tar ) <= wrange * 10 ) {
		return 1;
	}
	return 0;
}

//this implementation returns only true if there is a bow wielded
//but there is no ammo for it
//if the implementation should sign 'no ranged attack possible'
//then change some return values
//in bg2/iwd2 it doesn't accept an object (the object parameter is gemrb ext.)
int GameScript::OutOfAmmo(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr = Sender;
	if (parameters->objectParameter) {
		scr = GetActorFromObject( Sender, parameters->objectParameter );
	}
	if ( !scr || scr->Type!=ST_ACTOR) {
		return 0;
	}
	Actor *actor = (Actor *) scr;
	WeaponInfo wi;
	ITMExtHeader *header = actor->GetWeapon(wi, false);
	//no bow wielded?
	if (!header || header->AttackType!=ITEM_AT_BOW) {
		return 0;
	}
	//we either have a projectile (negative) or an empty bow (positive)
	//so we should find a negative slot, positive slot means: OutOfAmmo
	if (actor->inventory.GetEquipped()<0) {
		return 0;
	}
	//out of ammo
	return 1;
}

//returns true if a weapon is equipped (with more than 0 range)
//if a bow is equipped without projectile, it is useless!
//please notice how similar is this to OutOfAmmo
int GameScript::HaveUsableWeaponEquipped(Scriptable* Sender, Trigger* /*parameters*/)
{
	if (Sender->Type!=ST_ACTOR) {
		return 0;
	}
	Actor *actor = (Actor *) Sender;
	WeaponInfo wi;
	ITMExtHeader *header = actor->GetWeapon(wi, false);

	//bows are not usable (because if they are loaded, the equipped
	//weapon is the projectile)
	if (!header || header->AttackType==ITEM_AT_BOW) {
		return 0;
	}
	//only fist we have, it is not qualified as weapon?
	if (actor->inventory.GetEquippedSlot() == actor->inventory.GetFistSlot()) {
		return 0;
	}
	return 1;
}

int GameScript::HasWeaponEquipped(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar || tar->Type!=ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) tar;
	if (actor->inventory.GetEquippedSlot() == IW_NO_EQUIPPED) {
		return 0;
	}
	return 1;
}

int GameScript::PCInStore( Scriptable* /*Sender*/, Trigger* /*parameters*/)
{
	if (core->GetCurrentStore()) {
		return 1;
	}
	return 0;
}

//this checks if the launch point is onscreen, a more elaborate check
//would see if any piece of the Scriptable is onscreen, what is the original
//behaviour?
int GameScript::OnScreen( Scriptable* Sender, Trigger* /*parameters*/)
{
	Region vp = core->GetVideoDriver()->GetViewport();
	if (vp.PointInside(Sender->Pos) ) {
		return 1;
	}
	return 0;
}

int GameScript::IsPlayerNumber( Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar || tar->Type!=ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) tar;
	if (actor->InParty == parameters->int0Parameter) {
		return 1;
	}
	return 0;
}

int GameScript::PCCanSeePoint( Scriptable* /*Sender*/, Trigger* parameters)
{
	Map* map = core->GetGame()->GetCurrentArea();
	if (map->IsVisible(parameters->pointParameter, false) ) {
		return 1;
	}
	return 0;
}

//i'm clueless about this trigger
int GameScript::StuffGlobalRandom( Scriptable* Sender, Trigger* parameters)
{
	unsigned int max=parameters->int0Parameter+1;
	ieDword Value;
	if (max) {
		Value = RandomNumValue%max;
	} else {
		Value = RandomNumValue;
	}
	SetVariable( Sender, parameters->string0Parameter, Value );
	if (Value) {
		return 1;
	}
	return 0;
}

int GameScript::IsCreatureAreaFlag( Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar || tar->Type!=ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) tar;
	if (actor->GetStat(IE_MC_FLAGS) & parameters->int0Parameter) {
		return 1;
	}
	return 0;
}

int GameScript::IsPathCriticalObject( Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar || tar->Type!=ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) tar;
	if (actor->GetStat(IE_MC_FLAGS) & MC_PLOT_CRITICAL) {
		return 1;
	}
	return 0;
}

// 0 - ability, 1 - number, 2 - mode
int GameScript::ChargeCount( Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar || tar->Type!=ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) tar;
	int Slot = actor->inventory.FindItem(parameters->string0Parameter,0);
	if (Slot<0) {
		return 0;
	}
	CREItem *item = actor->inventory.GetSlotItem (Slot);
	if (!item) {//bah
		return 0;
	}
	if (parameters->int0Parameter>2) {
		return 0;
	}
	int charge = item->Usages[parameters->int0Parameter];
	switch (parameters->int2Parameter) {
		case DM_EQUAL:
			if (charge == parameters->int1Parameter)
				return 1;
			break;
		case DM_LESS:
			if (charge < parameters->int1Parameter)
				return 1;
			break;
		case DM_GREATER:
			if (charge > parameters->int1Parameter)
				return 1;
			break;
		default:
			return 0;
	}
	return 0;
}

// no idea if it checks only alive partymembers
int GameScript::CheckPartyLevel( Scriptable* /*Sender*/, Trigger* parameters)
{
	if (core->GetGame()->GetPartyLevel(false)<parameters->int0Parameter) {
		return 0;
	}
	return 1;
}

// no idea if it checks only alive partymembers
int GameScript::CheckPartyAverageLevel( Scriptable* /*Sender*/, Trigger* parameters)
{
	int level = core->GetGame()->GetPartyLevel(false);
	switch (parameters->int1Parameter) {
		case DM_EQUAL:
			if (level ==parameters->int0Parameter) {
				return 1;
			}
			break;
		case DM_LESS:
			if (level < parameters->int0Parameter) {
				return 1;
			}
			break;
		case DM_GREATER:
			if (level > parameters->int0Parameter) {
				return 1;
			}
			break;
		default:
			return 0;
	}
	return 1;
}

int GameScript::CheckDoorFlags( Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar || tar->Type!=ST_DOOR) {
		return 0;
	}
	Door* door = ( Door* ) tar;
	if (door->Flags&parameters->int0Parameter) {
		return 1;
	}
	return 0;
}

// works only on animations?
// Be careful when converting to GetActorFromObject, it won't return animations (those are not scriptable)
int GameScript::Frame( Scriptable* Sender, Trigger* parameters)
{
	//to avoid a crash
	if (!parameters->objectParameter) {
		return 0;
	}
	AreaAnimation* anim = Sender->GetCurrentArea()->GetAnimation(parameters->objectParameter->objectName);
	if (!anim) {
		return 0;
	}
	int frame = anim->frame;
	if ((frame>=parameters->int0Parameter) &&
	(frame<=parameters->int1Parameter) ) {
		return 1;
	}
	return 0;
}

//Modalstate in IWD2 allows specifying an object
int GameScript::ModalState( Scriptable* Sender, Trigger* parameters)
{
	Scriptable *scr;

	if (parameters->objectParameter) {
		scr = GetActorFromObject( Sender, parameters->objectParameter );
	} else {
		scr = Sender;
	}
	if (scr->Type!=ST_ACTOR) {
		return 0;
	}
	Actor *actor = (Actor *) scr;

	if (actor->ModalState==(ieDword) parameters->int0Parameter) {
		return 1;
	}
	return 0;
}

/* a special redundant trigger for iwd2 - could do something extra */
int GameScript::IsCreatureHiddenInShadows( Scriptable* Sender, Trigger* /*parameters*/)
{
	if (Sender->Type!=ST_ACTOR) {
		return 0;
	}
	Actor *actor = (Actor *) Sender;

	if (actor->ModalState==MS_STEALTH) {
		return 1;
	}
	return 0;
}

int GameScript::IsWeather( Scriptable* /*Sender*/, Trigger* parameters)
{
	Game *game = core->GetGame();
	ieDword weather = game->WeatherBits & parameters->int0Parameter;
	if (weather == (ieDword) parameters->int1Parameter) {
		return 1;
	}
	return 0;
}

int GameScript::Delay( Scriptable* Sender, Trigger* parameters)
{
	ieDword delay = (ieDword) parameters->int0Parameter;
	if (delay<=1) {
		return 1;
	}

	return (Sender->ScriptTicks % delay) <= Sender->IdleTicks;
}

int GameScript::TimeOfDay(Scriptable* /*Sender*/, Trigger* parameters)
{
	ieDword timeofday = (core->GetGame()->GameTime/AI_UPDATE_TIME)%7200/1800;

	if (timeofday==(ieDword) parameters->int0Parameter) {
		return 1;
	}
	return 0;
}

//this is a PST action, it's using delta, not diffmode
int GameScript::RandomStatCheck(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar || tar->Type!=ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) tar;

	ieDword stat = actor->GetStat(parameters->int0Parameter);
	ieDword value = Bones(parameters->int2Parameter);
	switch(parameters->int1Parameter) {
		case DM_SET:
			if (stat==value)
				return 1;
			break;
		case DM_LOWER:
			if (stat<value)
				return 1;
			break;
		case DM_RAISE:
			if (stat>value)
				return 1;
			break;
	}
	return 0;
}

int GameScript::PartyRested(Scriptable* Sender, Trigger* /*parameters*/)
{
	return Sender->MatchTrigger(trigger_partyrested);
}

int GameScript::IsWeaponRanged(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar || tar->Type!=ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) tar;
	if (actor->inventory.GetEquipped()<0) {
		return 1;
	}
	return 0;
}

//HoW applies sequence on area animations
int GameScript::Sequence(Scriptable* Sender, Trigger* parameters)
{
	//to avoid a crash, check if object is NULL
	if (parameters->objectParameter) {
		AreaAnimation *anim = Sender->GetCurrentArea()->GetAnimation(parameters->objectParameter->objectName);
		if (anim) {
			//this is the cycle count for the area animation
			//very much like stance for avatar anims
			if (anim->sequence==parameters->int0Parameter) {
				return 1;
			}
			return 0;
		}
	}

	Scriptable *tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar || tar->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) tar;
	if (actor->GetStance()==parameters->int0Parameter) {
		return 1;
	}
	return 0;
}

int GameScript::TimerExpired(Scriptable* Sender, Trigger* parameters)
{
	if (Sender->TimerExpired(parameters->int0Parameter) ) {
		return 1;
	}
	return 0;
}

int GameScript::TimerActive(Scriptable* Sender, Trigger* parameters)
{
	if (Sender->TimerActive(parameters->int0Parameter) ) {
		return 1;
	}
	return 0;
}

int GameScript::ActuallyInCombat(Scriptable* /*Sender*/, Trigger* /*parameters*/)
{
	Game *game=core->GetGame();
	if (game->AnyPCInCombat()) return 1;
	return 0;
}

int GameScript::InMyGroup(Scriptable* Sender, Trigger* parameters)
{
	if (Sender->Type!=ST_ACTOR) {
		return 0;
	}

	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar || tar->Type!=ST_ACTOR) {
		return 0;
	}
/* IESDP SUCKS
	if (GetGroup( (Actor *) tar)==GetGroup( (Actor *) Sender) ) {
		return 1;
	}
*/
	if ( ((Actor *) tar)->GetStat(IE_SPECIFIC)==((Actor *) tar)->GetStat(IE_SPECIFIC) ) {
		return 1;
	}
	return 0;
}

int GameScript::AnyPCSeesEnemy(Scriptable* /*Sender*/, Trigger* /*parameters*/)
{
	Game *game = core->GetGame();
	unsigned int i = (unsigned int) game->GetLoadedMapCount();
	while(i--) {
		Map *map = game->GetMap(i);
		if (map->AnyPCSeesEnemy()) {
			return 1;
		}
	}
	return 0;
}

int GameScript::Unusable(Scriptable* Sender, Trigger* parameters)
{
	if (Sender->Type!=ST_ACTOR) {
		return 0;
	}
	Actor *actor = (Actor *) Sender;

	Item *item = gamedata->GetItem(parameters->string0Parameter);
	int ret;
	if (actor->Unusable(item)) {
		ret = 0;
	} else {
		ret = 1;
	}
	gamedata->FreeItem(item, parameters->string0Parameter, true);
	return ret;
}

//returns true if the immunity flag is set
//(attacker has to make a successful spell save to hit the target)
int GameScript::IsInGuardianMantle(Scriptable* Sender, Trigger* parameters)
{
	Scriptable *tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar || tar->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) tar;
	if (actor->GetStat(IE_IMMUNITY)&IMM_GUARDIAN) {
		return 1;
	}
	return 0;
}

int GameScript::HasBounceEffects(Scriptable* Sender, Trigger* parameters)
{
	Scriptable *tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar || tar->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) tar;
	if (actor->GetStat(IE_BOUNCE)) return 1;
	return 0;
}

int GameScript::HasImmunityEffects(Scriptable* Sender, Trigger* parameters)
{
	Scriptable *tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar || tar->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) tar;
	if (actor->GetStat(IE_IMMUNITY)) return 1;
	return 0;
}

// this is a GemRB specific trigger, to transfer some system variables
// to a global (game variable), it will always return true, and the
// variable could be checked in a subsequent trigger (like triggersetglobal)

#define SYSV_SCREENFLAGS    0
#define SYSV_CONTROLSTATUS  1
#define SYSV_REPUTATION     2
#define SYSV_PARTYGOLD      3

int GameScript::SystemVariable_Trigger(Scriptable* Sender, Trigger* parameters)
{
	ieDword value;

	switch (parameters->int0Parameter) {
	case SYSV_SCREENFLAGS:
		value = core->GetGameControl()->GetScreenFlags();
		break;
	case SYSV_CONTROLSTATUS:
		value = core->GetGame()->ControlStatus;
		break;
	case SYSV_REPUTATION:
		value = core->GetGame()->Reputation;
		break;
	case SYSV_PARTYGOLD:
		value = core->GetGame()->PartyGold;
		break;
	default:
		return 0;
	}

	SetVariable(Sender, parameters->string0Parameter, value);
	return 1;
}

int GameScript::SpellCast(Scriptable* Sender, Trigger* parameters)
{
	return Sender->MatchTriggerWithObject(trigger_spellcast, parameters->objectParameter, parameters->int0Parameter);
}

int GameScript::SpellCastPriest(Scriptable* Sender, Trigger* parameters)
{
	return Sender->MatchTriggerWithObject(trigger_spellcastpriest, parameters->objectParameter, parameters->int0Parameter);
}

int GameScript::SpellCastInnate(Scriptable* Sender, Trigger* parameters)
{
	return Sender->MatchTriggerWithObject(trigger_spellcastinnate, parameters->objectParameter, parameters->int0Parameter);
}

int GameScript::SpellCastOnMe(Scriptable* Sender, Trigger* parameters)
{
	return Sender->MatchTriggerWithObject(trigger_spellcastonme, parameters->objectParameter, parameters->int0Parameter);
}

int GameScript::CalendarDay(Scriptable* /*Sender*/, Trigger* parameters)
{
	int day = core->GetCalendar()->GetCalendarDay(core->GetGame()->GameTime/AI_UPDATE_TIME/7200);
	if(day == parameters->int0Parameter) {
		return 1;
	}
	return 0;
}

int GameScript::CalendarDayGT(Scriptable* /*Sender*/, Trigger* parameters)
{
	int day = core->GetCalendar()->GetCalendarDay(core->GetGame()->GameTime/AI_UPDATE_TIME/7200);
	if(day > parameters->int0Parameter) {
		return 1;
	}
	return 0;
}

int GameScript::CalendarDayLT(Scriptable* /*Sender*/, Trigger* parameters)
{
	int day = core->GetCalendar()->GetCalendarDay(core->GetGame()->GameTime/AI_UPDATE_TIME/7200);
	if(day < parameters->int0Parameter) {
		return 1;
	}
	return 0;
}

//NT Returns true only if the active CRE was turned by the specified priest or paladin.
int GameScript::TurnedBy(Scriptable* Sender, Trigger* parameters)
{
	return Sender->MatchTriggerWithObject(trigger_turnedby, parameters->objectParameter);
}

//This is used for pst portals
//usage: UsedExit(Protagonist, "sigil")
//where sigil.2da contains all the exits that should trigger the teleport
int GameScript::UsedExit(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if (!scr || scr->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) scr;
	if (!actor) {
		return 0;
	}

	if (actor->GetInternalFlag()&IF_USEEXIT) {
		return 0;
	}

	Map *ca = core->GetGame()->GetMap(actor->LastArea, false);

	if (!ca) {
		return 0;
	}

	InfoPoint *ip = ca->GetInfoPointByGlobalID(actor->UsedExit);
	if (!ip || ip->Type!=ST_TRAVEL) {
		return 0;
	}

	AutoTable tm(parameters->string0Parameter);
	if (!tm) {
		return 0;
	}

	int count = tm->GetRowCount();
	for (int i=0;i<count;i++) {
		const char *area = tm->QueryField( i, 0 );
		if (strnicmp(actor->LastArea, area, 8) ) {
			continue;
		}
		const char *exit = tm->QueryField( i, 1 );
		if (strnicmp(ip->GetScriptName(), exit, 32) ) {
			continue;
		}
		return 1;
	}
	return 0;
}

}
