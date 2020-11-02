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

#include "voodooconst.h"
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
// bg1 and bg2 have some dead bcs code - perhaps the first implementation
// of morale, since the uses suggest being able to detect panic
int GameScript::BreakingPoint(Scriptable *Sender, const Trigger */*parameters*/)
{
	int value=GetHappiness(Sender, core->GetGame()->Reputation );
	return value < -300;
}

int GameScript::Reaction(Scriptable *Sender, const Trigger *parameters)
{
	const Scriptable *scr = GetActorFromObject(Sender, parameters->objectParameter);
	if (!scr || scr->Type != ST_ACTOR) {
		parameters->dump();
		return 0;
	}
	int value = GetReaction((const Actor *) scr, Sender);
	bool matched = value == parameters->int0Parameter;
	if (matched) {
		Sender->SetLastTrigger(trigger_reaction, scr->GetGlobalID());
	}
	return matched;
}

int GameScript::ReactionGT(Scriptable *Sender, const Trigger *parameters)
{
	const Scriptable *scr = GetActorFromObject(Sender, parameters->objectParameter);
	if (!scr || scr->Type != ST_ACTOR) {
		parameters->dump();
		return 0;
	}
	int value = GetReaction((const Actor *) scr, Sender);
	bool matched = value > parameters->int0Parameter;
	if (matched) {
		Sender->SetLastTrigger(trigger_reaction, scr->GetGlobalID());
	}
	return matched;
}

int GameScript::ReactionLT(Scriptable *Sender, const Trigger *parameters)
{
	const Scriptable *scr = GetActorFromObject(Sender, parameters->objectParameter);
	if (!scr || scr->Type != ST_ACTOR) {
		parameters->dump();
		return 0;
	}
	int value = GetReaction((const Actor *) scr, Sender);
	bool matched = value < parameters->int0Parameter;
	if (matched) {
		Sender->SetLastTrigger(trigger_reaction, scr->GetGlobalID());
	}
	return matched;
}

int GameScript::Happiness(Scriptable *Sender, const Trigger *parameters)
{
	int value=GetHappiness(Sender, core->GetGame()->Reputation );
	return value == parameters->int0Parameter;
}

int GameScript::HappinessGT(Scriptable *Sender, const Trigger *parameters)
{
	int value=GetHappiness(Sender, core->GetGame()->Reputation );
	return value > parameters->int0Parameter;
}

int GameScript::HappinessLT(Scriptable *Sender, const Trigger *parameters)
{
	int value=GetHappiness(Sender, core->GetGame()->Reputation );
	return value < parameters->int0Parameter;
}

// these also take an object parameter, but reputation is global
// but we do need to use it to be precise for LastTrigger purposes
int GameScript::Reputation(Scriptable *Sender, const Trigger *parameters)
{
	const Scriptable *scr = GetActorFromObject(Sender, parameters->objectParameter);
	bool matched = core->GetGame()->Reputation / 10 == (ieDword) parameters->int0Parameter;
	if (matched && scr) {
		Sender->SetLastTrigger(trigger_reputation, scr->GetGlobalID());
	}
	return matched;
}

int GameScript::ReputationGT(Scriptable *Sender, const Trigger *parameters)
{
	const Scriptable *scr = GetActorFromObject(Sender, parameters->objectParameter);
	bool matched = core->GetGame()->Reputation / 10 > (ieDword) parameters->int0Parameter;
	if (matched && scr) {
		Sender->SetLastTrigger(trigger_reputation, scr->GetGlobalID());
	}
	return matched;
}

int GameScript::ReputationLT(Scriptable *Sender, const Trigger *parameters)
{
	const Scriptable *scr = GetActorFromObject(Sender, parameters->objectParameter);
	bool matched = core->GetGame()->Reputation / 10 < (ieDword) parameters->int0Parameter;
	if (matched && scr) {
		Sender->SetLastTrigger(trigger_reputation, scr->GetGlobalID());
	}
	return matched;
}

int GameScript::Alignment(Scriptable *Sender, const Trigger *parameters)
{
	const Scriptable *scr = GetActorFromObject(Sender, parameters->objectParameter);
	if (!scr || scr->Type != ST_ACTOR) {
		return 0;
	}
	const Actor *actor = (const Actor *) scr;
	bool matched = ID_Alignment(actor, parameters->int0Parameter);
	if (matched) {
		Sender->SetLastTrigger(trigger_alignment, actor->GetGlobalID());
	}
	return matched;
}

int GameScript::Allegiance(Scriptable *Sender, const Trigger *parameters)
{
	const Scriptable *scr = GetActorFromObject(Sender, parameters->objectParameter);
	if (!scr || scr->Type != ST_ACTOR) {
		return 0;
	}
	const Actor *actor = (const Actor *) scr;
	bool matched = ID_Allegiance(actor, parameters->int0Parameter);
	if (matched) {
		Sender->SetLastTrigger(trigger_allegiance, actor->GetGlobalID());
	}
	return matched;
}

//should return *_ALL stuff
int GameScript::Class(Scriptable *Sender, const Trigger *parameters)
{
	const Scriptable *scr = GetActorFromObject(Sender, parameters->objectParameter);
	if (!scr || scr->Type != ST_ACTOR) {
		return 0;
	}
	const Actor *actor = (const Actor *) scr;
	bool matched = ID_Class(actor, parameters->int0Parameter);
	if (matched) {
		Sender->SetLastTrigger(trigger_class, actor->GetGlobalID());
	}
	return matched;
}

int GameScript::ClassEx(Scriptable *Sender, const Trigger *parameters)
{
	const Scriptable *scr = GetActorFromObject(Sender, parameters->objectParameter);
	if (!scr || scr->Type != ST_ACTOR) {
		return 0;
	}
	const Actor *actor = (const Actor *) scr;
	return ID_AVClass(actor, parameters->int0Parameter);
}

int GameScript::Faction(Scriptable *Sender, const Trigger *parameters)
{
	const Scriptable *scr = GetActorFromObject(Sender, parameters->objectParameter);
	if (!scr || scr->Type != ST_ACTOR) {
		return 0;
	}
	const Actor *actor = (const Actor *) scr;
	return ID_Faction(actor, parameters->int0Parameter);
}

int GameScript::Team(Scriptable *Sender, const Trigger *parameters)
{
	const Scriptable *scr = GetActorFromObject(Sender, parameters->objectParameter);
	if (!scr || scr->Type != ST_ACTOR) {
		return 0;
	}
	const Actor *actor = (const Actor *) scr;
	return ID_Team(actor, parameters->int0Parameter);
}

int GameScript::SubRace(Scriptable *Sender, const Trigger *parameters)
{
	const Scriptable *scr = GetActorFromObject(Sender, parameters->objectParameter);
	if (!scr || scr->Type != ST_ACTOR) {
		return 0;
	}
	const Actor *actor = (const Actor *) scr;
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
int GameScript::IsTeamBitOn(Scriptable *Sender, const Trigger *parameters)
{
	const Scriptable *scr = Sender;
	if (parameters->objectParameter) {
		scr = GetActorFromObject(Sender, parameters->objectParameter);
	}
	if (!scr || scr->Type != ST_ACTOR) {
		return 0;
	}
	const Actor *actor = (const Actor *) scr;
	if (actor->GetStat(IE_TEAM) & parameters->int0Parameter) {
		return 1;
	}
	return 0;
}

int GameScript::NearbyDialog(Scriptable *Sender, const Trigger *parameters)
{
	const Scriptable *target = Sender->GetCurrentArea()->GetActorByDialog(parameters->string0Parameter);
	if ( !target ) {
		return 0;
	}
	return CanSee( Sender, target, true, GA_NO_DEAD|GA_NO_HIDDEN|GA_NO_UNSCHEDULED );
}

//atm this checks for InParty and See, it is unsure what is required
int GameScript::IsValidForPartyDialog(Scriptable *Sender, const Trigger *parameters)
{
	const Scriptable *scr = GetActorFromObject(Sender, parameters->objectParameter);
	if (!scr) {
		scr = Sender;
	}
	if (scr->Type != ST_ACTOR) {
		return 0;
	}
	const Actor *target = (const Actor *) scr;
	if (core->GetGame()->InParty(target) == -1) {
		return 0;
	}
	//don't accept parties currently in dialog!
	//this might disturb some modders, but this is the correct behaviour
	//for example the aaquatah dialog in irenicus dungeon depends on it
	const GameControl *gc = core->GetGameControl();
	if (gc->dialoghandler->InDialog(scr)) {
		return 0;
	}

	//don't accept parties with the no interrupt flag
	//this fixes bug #2573808 on gamescript level
	//(still someone has to turn the no interrupt flag off)
	if(!target->GetDialog(GD_CHECK)) {
		return 0;
	}
	return CanSee( Sender, target, false, GA_NO_DEAD|GA_NO_UNSCHEDULED );
}

int GameScript::InParty(Scriptable *Sender, const Trigger *parameters, bool allowdead)
{
	const Scriptable *scr;

	if (parameters->objectParameter) {
		scr = GetActorFromObject( Sender, parameters->objectParameter );
	} else {
		scr = Sender;
	}

	if (!scr || scr->Type != ST_ACTOR) {
		return 0;
	}
	
	const Actor *act = (const Actor *) scr;
	//don't allow dead, don't allow maze and similar effects
	if (!allowdead && (!act->ValidTarget(GA_NO_DEAD) || act->GetStat(IE_AVATARREMOVAL) != 0)) {
		return 0;
	}

	bool matched = core->GetGame()->InParty(act) >= 0;
	if (matched) {
		Sender->SetLastTrigger(trigger_inparty, scr->GetGlobalID());
	}
	return matched;
}

int GameScript::InParty(Scriptable *Sender, const Trigger *parameters)
{
	return InParty(Sender, parameters, core->HasFeature(GF_IN_PARTY_ALLOWS_DEAD));
}

int GameScript::InPartyAllowDead(Scriptable *Sender, const Trigger *parameters)
{
	return InParty(Sender, parameters, true);
}

int GameScript::InPartySlot(Scriptable *Sender, const Trigger *parameters)
{
	const Actor *actor = core->GetGame()->GetPC(parameters->int0Parameter, false);
	return MatchActor(Sender, actor->GetGlobalID(), parameters->objectParameter);
}

int GameScript::Exists(Scriptable *Sender, const Trigger *parameters)
{
	const Scriptable *scr = GetActorFromObject(Sender, parameters->objectParameter);
	if (!scr) {
		return 0;
	}
	Sender->SetLastTrigger(trigger_exists, scr->GetGlobalID());
	return 1;
}

int GameScript::IsAClown(Scriptable *Sender, const Trigger *parameters)
{
	const Scriptable *scr = GetActorFromObject(Sender, parameters->objectParameter);
	if (!scr || scr->Type!=ST_ACTOR) {
		return 0;
	}
	return 1;
}

int GameScript::IsGabber(Scriptable *Sender, const Trigger *parameters)
{
	const Scriptable *scr = GetActorFromObject(Sender, parameters->objectParameter);
	if (!scr || scr->Type!=ST_ACTOR) {
		return 0;
	}
	if (core->GetGameControl()->dialoghandler->IsSpeaker(scr))
		return 1;
	return 0;
}

//returns true if the trap or infopoint is active
//returns true if the actor is active
//returns true if the sound source is active
//returns true if container is active
int GameScript::IsActive(Scriptable *Sender, const Trigger *parameters)
{
	const Scriptable *scr = GetActorFromObject(Sender, parameters->objectParameter);
	if (!scr) {
		const AmbientMgr *ambientmgr = core->GetAudioDrv()->GetAmbientMgr();
		if (ambientmgr->isActive(parameters->objectParameter->objectName)) {
			return 1;
		}
		return 0;
	}

	switch(scr->Type) {
		case ST_ACTOR:
			if (((const Actor *) scr)->Schedule(core->GetGame()->GameTime, true)) return 1;
			return 0;
		case ST_CONTAINER:
			if (((const Container *) scr)->Flags & CONT_DISABLED) return 0;
			return 1;

		case ST_PROXIMITY: case ST_TRIGGER: case ST_TRAVEL:
			if (((const InfoPoint *) scr)->Flags & (TRAP_DEACTIVATED | INFO_DOOR)) {
				return 0;
			}
			return 1;
		default:
			return 0;
	}
}

int GameScript::InTrap(Scriptable *Sender, const Trigger *parameters)
{
	const Scriptable *scr = GetActorFromObject(Sender, parameters->objectParameter);
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
int GameScript::OnIsland(Scriptable *Sender, const Trigger *parameters)
{
	const Scriptable *scr = GetActorFromObject(Sender, parameters->objectParameter);
	if (!scr) {
		return 0;
	}
	const Gem_Polygon *p = GetPolygon2DA(parameters->int0Parameter);
	if (!p) {
		return 0;
	}
	return p->PointIn(scr->Pos);
}

int GameScript::School(Scriptable *Sender, const Trigger *parameters)
{
	const Scriptable *scr = GetActorFromObject(Sender, parameters->objectParameter);
	if (!scr || scr->Type!=ST_ACTOR) {
		return 0;
	}
	const Actor *actor = (const Actor *) scr;
	//only the low 2 bytes count
	//the School values start from 1 to 9 and the first school value is 0x40
	//so this mild hack will do
	if (actor->GetStat(IE_KIT) == (ieDword) (0x20<<parameters->int0Parameter)) {
		return 1;
	}
	return 0;
}

int GameScript::Kit(Scriptable *Sender, const Trigger *parameters)
{
	const Scriptable *scr = GetActorFromObject(Sender, parameters->objectParameter);
	if (!scr || scr->Type!=ST_ACTOR) {
		return 0;
	}
	const Actor *actor = (const Actor *) scr;

	ieDword kit = actor->GetStat(IE_KIT);
	//TODO: fix baseclass / barbarian confusion

	//IWD2 style kit matching (also used for mage schools)
	if (kit == (ieDword) (parameters->int0Parameter)) {
		return 1;
	}
	if (kit == (ieDword) (parameters->int0Parameter)) {
		return 1;
	}
	return 0;
}

int GameScript::General(Scriptable *Sender, const Trigger *parameters)
{
	const Scriptable *scr = GetActorFromObject(Sender, parameters->objectParameter);
	if (!scr) {
		scr = Sender;
	}
	if (scr->Type != ST_ACTOR) {
		return 0;
	}
	const Actor *actor = (const Actor *) scr;
	bool matched = ID_General(actor, parameters->int0Parameter);
	if (matched) {
		Sender->SetLastTrigger(trigger_general, actor->GetGlobalID());
	}
	return matched;
}

int GameScript::Specifics(Scriptable *Sender, const Trigger *parameters)
{
	const Scriptable *scr = GetActorFromObject(Sender, parameters->objectParameter);
	if (!scr) {
		scr = Sender;
	}
	if (scr->Type != ST_ACTOR) {
		return 0;
	}
	const Actor *actor = (const Actor *) scr;
	bool matched = ID_Specific(actor, parameters->int0Parameter);
	if (matched) {
		Sender->SetLastTrigger(trigger_specifics, actor->GetGlobalID());
	}
	return matched;
}

int GameScript::BitCheck(Scriptable *Sender, const Trigger *parameters)
{
	bool valid=true;

	ieDword value = CheckVariable(Sender, parameters->string0Parameter, &valid );
	if (valid && value & parameters->int0Parameter) return 1;
	return 0;
}

int GameScript::BitCheckExact(Scriptable *Sender, const Trigger *parameters)
{
	bool valid=true;

	ieDword value = CheckVariable(Sender, parameters->string0Parameter, &valid );
	if (valid) {
		ieDword tmp = (ieDword) parameters->int0Parameter ;
		if ((value & tmp) == tmp) return 1;
	}
	return 0;
}

//OP_OR would make sense only if this trigger changes the value of the variable
//should I do that???
int GameScript::BitGlobal_Trigger(Scriptable *Sender, const Trigger *parameters)
{
	bool valid=true;

	ieDword value = CheckVariable(Sender, parameters->string0Parameter, &valid );
	if (valid) {
		HandleBitMod(value, parameters->int0Parameter, parameters->int1Parameter);
		if (value!=0) return 1;
	}
	return 0;
}

int GameScript::GlobalOrGlobal_Trigger(Scriptable *Sender, const Trigger *parameters)
{
	bool valid=true;

	ieDword value1 = CheckVariable(Sender, parameters->string0Parameter, &valid);
	if (valid) {
		if (value1) return 1;
		ieDword value2 = CheckVariable(Sender, parameters->string1Parameter, &valid);
		if (valid && value2) return 1;
	}
	return 0;
}

int GameScript::GlobalAndGlobal_Trigger(Scriptable *Sender, const Trigger *parameters)
{
	bool valid=true;

	ieDword value1 = CheckVariable( Sender, parameters->string0Parameter, &valid );
	if (valid && value1) {
		ieDword value2 = CheckVariable( Sender, parameters->string1Parameter, &valid );
		if (valid && value2) return 1;
	}
	return 0;
}

int GameScript::GlobalBAndGlobal_Trigger(Scriptable *Sender, const Trigger *parameters)
{
	bool valid=true;

	ieDword value1 = CheckVariable(Sender, parameters->string0Parameter, &valid );
	if (valid) {
		ieDword value2 = CheckVariable(Sender, parameters->string1Parameter, &valid );
		if (valid && (value1 & value2) != 0) return 1;
	}
	return 0;
}

int GameScript::GlobalBAndGlobalExact(Scriptable *Sender, const Trigger *parameters)
{
	bool valid=true;

	ieDword value1 = CheckVariable(Sender, parameters->string0Parameter, &valid );
	if (valid) {
		ieDword value2 = CheckVariable(Sender, parameters->string1Parameter, &valid );
		if (valid && (value1 & value2) == value2) return 1;
	}
	return 0;
}

int GameScript::GlobalBitGlobal_Trigger(Scriptable *Sender, const Trigger *parameters)
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
int GameScript::TriggerSetGlobal(Scriptable *Sender, const Trigger *parameters)
{
	SetVariable( Sender, parameters->string0Parameter, parameters->int0Parameter );
	return 1;
}

//would this function also alter the variable?
int GameScript::Xor(Scriptable *Sender, const Trigger *parameters)
{
	bool valid=true;

	ieDword value = CheckVariable(Sender, parameters->string0Parameter, &valid );
	if (valid && (value ^ parameters->int0Parameter) != 0) return 1;
	return 0;
}

//TODO:
//no sprite is dead for iwd, they use KILL_<name>_CNT
int GameScript::NumDead(Scriptable *Sender, const Trigger *parameters)
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

int GameScript::NumDeadGT(Scriptable *Sender, const Trigger *parameters)
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

int GameScript::NumDeadLT(Scriptable *Sender, const Trigger *parameters)
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

int GameScript::G_Trigger(Scriptable *Sender, const Trigger *parameters)
{
	ieDwordSigned value = CheckVariable(Sender, parameters->string0Parameter, "GLOBAL" );
	return ( value == parameters->int0Parameter );
}

int GameScript::Global(Scriptable *Sender, const Trigger *parameters)
{
	bool valid=true;

	ieDwordSigned value = CheckVariable(Sender, parameters->string0Parameter, &valid );
	if (valid) {
		if ( value == parameters->int0Parameter ) return 1;
	}
	return 0;
}

int GameScript::GLT_Trigger(Scriptable *Sender, const Trigger *parameters)
{
	ieDwordSigned value = CheckVariable(Sender, parameters->string0Parameter,"GLOBAL" );
	return ( value < parameters->int0Parameter );
}

int GameScript::GlobalLT(Scriptable *Sender, const Trigger *parameters)
{
	bool valid=true;

	ieDwordSigned value = CheckVariable(Sender, parameters->string0Parameter, &valid );
	if (valid && value < parameters->int0Parameter) return 1;
	return 0;
}

int GameScript::GGT_Trigger(Scriptable *Sender, const Trigger *parameters)
{
	ieDwordSigned value = CheckVariable(Sender, parameters->string0Parameter, "GLOBAL" );
	return ( value > parameters->int0Parameter );
}

int GameScript::GlobalGT(Scriptable *Sender, const Trigger *parameters)
{
	bool valid=true;

	ieDwordSigned value = CheckVariable(Sender, parameters->string0Parameter, &valid );
	if (valid && value > parameters->int0Parameter) return 1;
	return 0;
}

int GameScript::GlobalLTGlobal(Scriptable *Sender, const Trigger *parameters)
{
	bool valid=true;

	ieDwordSigned value1 = CheckVariable(Sender, parameters->string0Parameter, &valid );
	if (valid) {
		ieDwordSigned value2 = CheckVariable(Sender, parameters->string1Parameter, &valid );
		if (valid && value1 < value2) return 1;
	}
	return 0;
}

int GameScript::GlobalGTGlobal(Scriptable *Sender, const Trigger *parameters)
{
	bool valid=true;

	ieDwordSigned value1 = CheckVariable(Sender, parameters->string0Parameter, &valid );
	if (valid) {
		ieDwordSigned value2 = CheckVariable(Sender, parameters->string1Parameter, &valid );
		if (valid && value1 > value2) return 1;
	}
	return 0;
}

int GameScript::GlobalsEqual(Scriptable *Sender, const Trigger *parameters)
{
	ieDword value1 = CheckVariable(Sender, parameters->string0Parameter, "GLOBAL" );
	ieDword value2 = CheckVariable(Sender, parameters->string1Parameter, "GLOBAL" );
	return ( value1 == value2 );
}

int GameScript::GlobalsGT(Scriptable *Sender, const Trigger *parameters)
{
	ieDword value1 = CheckVariable(Sender, parameters->string0Parameter, "GLOBAL" );
	ieDword value2 = CheckVariable(Sender, parameters->string1Parameter, "GLOBAL" );
	return ( value1 > value2 );
}

int GameScript::GlobalsLT(Scriptable *Sender, const Trigger *parameters)
{
	ieDword value1 = CheckVariable(Sender, parameters->string0Parameter, "GLOBAL" );
	ieDword value2 = CheckVariable(Sender, parameters->string1Parameter, "GLOBAL" );
	return ( value1 < value2 );
}

int GameScript::LocalsEqual(Scriptable *Sender, const Trigger *parameters)
{
	ieDword value1 = CheckVariable(Sender, parameters->string0Parameter, "LOCALS" );
	ieDword value2 = CheckVariable(Sender, parameters->string1Parameter, "LOCALS" );
	return ( value1 == value2 );
}

int GameScript::LocalsGT(Scriptable *Sender, const Trigger *parameters)
{
	ieDword value1 = CheckVariable(Sender, parameters->string0Parameter, "LOCALS" );
	ieDword value2 = CheckVariable(Sender, parameters->string1Parameter, "LOCALS" );
	return ( value1 > value2 );
}

int GameScript::LocalsLT(Scriptable *Sender, const Trigger *parameters)
{
	ieDword value1 = CheckVariable(Sender, parameters->string0Parameter, "LOCALS" );
	ieDword value2 = CheckVariable(Sender, parameters->string1Parameter, "LOCALS" );
	return ( value1 < value2 );
}

int GameScript::RealGlobalTimerExact(Scriptable *Sender, const Trigger *parameters)
{
	bool valid=true;

	ieDword value1 = CheckVariable(Sender, parameters->string0Parameter, parameters->string1Parameter, &valid );
	if (valid && value1) {
		ieDword value2 = core->GetGame()->RealTime;
		if ( value1 == value2 ) return 1;
	}
	return 0;
}

int GameScript::RealGlobalTimerExpired(Scriptable *Sender, const Trigger *parameters)
{
	bool valid=true;

	ieDword value1 = CheckVariable(Sender, parameters->string0Parameter, parameters->string1Parameter, &valid );
	if (valid && value1 && value1 < core->GetGame()->RealTime) return 1;
	return 0;
}

int GameScript::RealGlobalTimerNotExpired(Scriptable *Sender, const Trigger *parameters)
{
	bool valid=true;

	ieDword value1 = CheckVariable(Sender, parameters->string0Parameter, parameters->string1Parameter, &valid );
	if (valid && value1 && value1 > core->GetGame()->RealTime) return 1;
	return 0;
}

int GameScript::GlobalTimerExact(Scriptable *Sender, const Trigger *parameters)
{
	bool valid=true;

	ieDword value1 = CheckVariable(Sender, parameters->string0Parameter, parameters->string1Parameter, &valid );
	if (valid && value1 == core->GetGame()->GameTime) return 1;
	return 0;
}

int GameScript::GlobalTimerExpired(Scriptable *Sender, const Trigger *parameters)
{
	bool valid=true;

	ieDword value1 = CheckVariable(Sender, parameters->string0Parameter, parameters->string1Parameter, &valid );
	if (valid && (core->HasFeature(GF_ZERO_TIMER_IS_VALID) || value1)) {
		if ( value1 < core->GetGame()->GameTime ) return 1;
	}
	return 0;
}

//globaltimernotexpired returns false if the timer doesn't exist or is zero
int GameScript::GlobalTimerNotExpired(Scriptable *Sender, const Trigger *parameters)
{
	bool valid=true;

	ieDword value1 = CheckVariable(Sender, parameters->string0Parameter, parameters->string1Parameter, &valid );
	if (valid && value1 && value1 > core->GetGame()->GameTime) return 1;
	return 0;
}

//globaltimerstarted returns false if the timer doesn't exist
int GameScript::GlobalTimerStarted(Scriptable *Sender, const Trigger *parameters)
{
	bool valid = VariableExists(Sender, parameters->string0Parameter, parameters->string1Parameter);
	if (valid) {
		return 1;
	}
	return 0;
}

int GameScript::WasInDialog(Scriptable *Sender, const Trigger */*parameters*/)
{
	return Sender->MatchTrigger(trigger_wasindialog);
}

int GameScript::OnCreation(Scriptable *Sender, const Trigger */*parameters*/)
{
	return Sender->MatchTrigger(trigger_oncreation);
}

int GameScript::SummoningLimit(Scriptable *Sender, const Trigger *parameters)
{
	const Map *map = Sender->GetCurrentArea();
	if (!map) return 0;

	int sl = map->CountSummons(GA_NO_DEAD, SEX_SUMMON);
	if (sl == parameters->int0Parameter) return 1;
	return 0;
}

int GameScript::SummoningLimitGT(Scriptable *Sender, const Trigger *parameters)
{
	const Map *map = Sender->GetCurrentArea();
	if (!map) return 0;

	int sl = map->CountSummons(GA_NO_DEAD, SEX_SUMMON);
	if (sl > parameters->int0Parameter) return 1;
	return 0;
}

int GameScript::SummoningLimitLT(Scriptable *Sender, const Trigger *parameters)
{
	const Map *map = Sender->GetCurrentArea();
	if (!map) return 0;

	int sl = map->CountSummons(GA_NO_DEAD, SEX_SUMMON);
	if (sl < parameters->int0Parameter) return 1;
	return 0;
}


int GameScript::NumItemsParty(Scriptable */*Sender*/, const Trigger *parameters)
{
	int cnt = 0;
	const Game *game = core->GetGame();

	int i = game->GetPartySize(true);
	while(i--) {
		const Actor *actor = game->GetPC(i, true);
		cnt += actor->inventory.CountItems(parameters->string0Parameter, true);
	}
	return cnt==parameters->int0Parameter;
}

int GameScript::NumItemsPartyGT(Scriptable */*Sender*/, const Trigger *parameters)
{
	int cnt = 0;
	const Game *game = core->GetGame();

	int i = game->GetPartySize(true);
	while(i--) {
		const Actor *actor = game->GetPC(i, true);
		cnt += actor->inventory.CountItems(parameters->string0Parameter, true);
	}
	return cnt>parameters->int0Parameter;
}

int GameScript::NumItemsPartyLT(Scriptable */*Sender*/, const Trigger *parameters)
{
	int cnt = 0;
	const Game *game = core->GetGame();

	int i = game->GetPartySize(true);
	while(i--) {
		const Actor *actor = game->GetPC(i, true);
		cnt += actor->inventory.CountItems(parameters->string0Parameter, true);
	}
	return cnt<parameters->int0Parameter;
}

int GameScript::NumItems(Scriptable *Sender, const Trigger *parameters)
{
	const Scriptable *tar = GetActorFromObject(Sender, parameters->objectParameter);
	if (!tar) {
		return 0;
	}

	const Inventory *inv = nullptr;
	switch (tar->Type) {
		case ST_ACTOR:
			inv = &(((const Actor *) tar)->inventory);
			break;
		case ST_CONTAINER:
			inv = &(((const Container *) tar)->inventory);
			break;
		default:;
	}
	if (!inv) {
		return 0;
	}

	int cnt = inv->CountItems(parameters->string0Parameter, true);
	return cnt==parameters->int0Parameter;
}

int GameScript::TotalItemCnt(Scriptable *Sender, const Trigger *parameters)
{
	const Scriptable *tar = GetActorFromObject(Sender, parameters->objectParameter);
	if ( !tar || tar->Type!=ST_ACTOR) {
		return 0;
	}
	const Actor *actor = (const Actor *) tar;
	int cnt = actor->inventory.CountItems("", true); //shall we count heaps or not?
	return cnt==parameters->int0Parameter;
}

int GameScript::TotalItemCntExclude(Scriptable *Sender, const Trigger *parameters)
{
	const Scriptable *tar = GetActorFromObject(Sender, parameters->objectParameter);
	if ( !tar || tar->Type!=ST_ACTOR) {
		return 0;
	}
	const Actor *actor = (const Actor *) tar;
	int cnt = actor->inventory.CountItems("", true) - actor->inventory.CountItems(parameters->string0Parameter, true); //shall we count heaps or not?
	return cnt==parameters->int0Parameter;
}

int GameScript::NumItemsGT(Scriptable *Sender, const Trigger *parameters)
{
	const Scriptable *tar = GetActorFromObject(Sender, parameters->objectParameter);
	if (!tar) {
		return 0;
	}

	const Inventory *inv = NULL;
	switch (tar->Type) {
		case ST_ACTOR:
			inv = &(((const Actor *) tar)->inventory);
			break;
		case ST_CONTAINER:
			inv = &(((const Container *) tar)->inventory);
			break;
		default:;
	}
	if (!inv) {
		return 0;
	}

	int cnt = inv->CountItems(parameters->string0Parameter, true);
	return cnt>parameters->int0Parameter;
}

int GameScript::TotalItemCntGT(Scriptable *Sender, const Trigger *parameters)
{
	const Scriptable *tar = GetActorFromObject(Sender, parameters->objectParameter);
	if (!tar || tar->Type!=ST_ACTOR) {
		return 0;
	}
	const Actor *actor = (const Actor *) tar;
	int cnt = actor->inventory.CountItems("", true); //shall we count heaps or not?
	return cnt>parameters->int0Parameter;
}

int GameScript::TotalItemCntExcludeGT(Scriptable *Sender, const Trigger *parameters)
{
	const Scriptable *tar = GetActorFromObject(Sender, parameters->objectParameter);
	if (!tar || tar->Type!=ST_ACTOR) {
		return 0;
	}
	const Actor *actor = (const Actor *) tar;
	int cnt = actor->inventory.CountItems("", true) - actor->inventory.CountItems(parameters->string0Parameter, true); //shall we count heaps or not?
	return cnt>parameters->int0Parameter;
}

int GameScript::NumItemsLT(Scriptable *Sender, const Trigger *parameters)
{
	const Scriptable *tar = GetActorFromObject(Sender, parameters->objectParameter);
	if (!tar) {
		return 0;
	}

	const Inventory *inv = nullptr;
	switch (tar->Type) {
		case ST_ACTOR:
			inv = &(((const Actor *) tar)->inventory);
			break;
		case ST_CONTAINER:
			inv = &(((const Container *) tar)->inventory);
			break;
		default:;
	}
	if (!inv) {
		return 0;
	}

	int cnt = inv->CountItems(parameters->string0Parameter, true);
	return cnt<parameters->int0Parameter;
}

int GameScript::TotalItemCntLT(Scriptable *Sender, const Trigger *parameters)
{
	const Scriptable *tar = GetActorFromObject(Sender, parameters->objectParameter);
	if ( !tar || tar->Type!=ST_ACTOR) {
		return 0;
	}
	const Actor *actor = (const Actor *) tar;
	int cnt = actor->inventory.CountItems("", true); //shall we count heaps or not?
	return cnt<parameters->int0Parameter;
}

int GameScript::TotalItemCntExcludeLT(Scriptable *Sender, const Trigger *parameters)
{
	const Scriptable *tar = GetActorFromObject(Sender, parameters->objectParameter);
	if ( !tar || tar->Type!=ST_ACTOR) {
		return 0;
	}
	const Actor *actor = (const Actor *) tar;
	int cnt = actor->inventory.CountItems("", true) - actor->inventory.CountItems(parameters->string0Parameter, true); //shall we count heaps or not?
	return cnt<parameters->int0Parameter;
}

//the int0 parameter is an addition, normally it is 0
int GameScript::Contains(Scriptable *Sender, const Trigger *parameters)
{
//actually this should be a container
	const Scriptable *tar = GetActorFromObject(Sender, parameters->objectParameter);
	if ( !tar || tar->Type!=ST_CONTAINER) {
		return 0;
	}
	const Container *cnt = (const Container *) tar;
	if (HasItemCore(&cnt->inventory, parameters->string0Parameter, parameters->int0Parameter)) {
		return 1;
	}
	return 0;
}

int GameScript::StoreHasItem(Scriptable */*Sender*/, const Trigger *parameters)
{
	return StoreHasItemCore(parameters->string0Parameter, parameters->string1Parameter);
}

//the int0 parameter is an addition, normally it is 0
int GameScript::HasItem(Scriptable *Sender, const Trigger *parameters)
{
	const Scriptable *scr = GetActorFromObject(Sender, parameters->objectParameter);
	if ( !scr ) {
		return 0;
	}
	const Inventory *inventory = nullptr;
	switch (scr->Type) {
		case ST_ACTOR:
			inventory = &((const Actor *) scr)->inventory;
			break;
		case ST_CONTAINER:
			inventory = &((const Container *) scr)->inventory;
			break;
		default:
			break;
	}
	if (inventory && HasItemCore(inventory, parameters->string0Parameter, parameters->int0Parameter) ) {
		return 1;
	}
	return 0;
}

int GameScript::ItemIsIdentified(Scriptable *Sender, const Trigger *parameters)
{
	// hardcode the Nothing filtering exception (needed for most iwd2 dialog uses)
	bool nothing = parameters->objectParameter[0].objectFilters[0] == 255;
	if (nothing) {
		// reset the filter to 19 LastTalkedToBy
		parameters->objectParameter[0].objectFilters[0] = 19;
	}
	const Scriptable *scr = GetActorFromObject(Sender, parameters->objectParameter);
	if ( !scr || scr->Type!=ST_ACTOR) {
		return 0;
	}
	const Actor *actor = (const Actor *) scr;
	if (HasItemCore(&actor->inventory, parameters->string0Parameter, IE_INV_ITEM_IDENTIFIED) ) {
		return 1;
	}
	return 0;
}

/** if the string is zero, then it will return true if there is any item in the slot (BG2)*/
/** if the string is non-zero, it will return true, if the given item was in the slot (IWD2)*/
int GameScript::HasItemSlot(Scriptable *Sender, const Trigger *parameters)
{
	const Scriptable *scr = GetActorFromObject(Sender, parameters->objectParameter);
	if ( !scr || scr->Type!=ST_ACTOR) {
		return 0;
	}
	const Actor *actor = (const Actor *) scr;
	//this might require a conversion of the slots
	if (actor->inventory.HasItemInSlot(parameters->string0Parameter, parameters->int0Parameter) ) {
		return 1;
	}
	return 0;
}

//this is a GemRB extension
//HasItemTypeSlot(Object, SLOT, ItemType)
//returns true if the item in SLOT is of ItemType
int GameScript::HasItemTypeSlot(Scriptable *Sender, const Trigger *parameters)
{
	const Scriptable *scr = GetActorFromObject(Sender, parameters->objectParameter);
	if ( !scr || scr->Type!=ST_ACTOR) {
		return 0;
	}
	const Inventory *inv = &((const Actor *) scr)->inventory;
	if (parameters->int0Parameter>=inv->GetSlotCount()) {
		return 0;
	}
	const CREItem *slot = inv->GetSlotItem(parameters->int0Parameter);
	if (!slot) {
		return 0;
	}
	const Item *itm = gamedata->GetItem(slot->ItemResRef);
	if (!itm) {
		return 0;
	}
	int itemtype = itm->ItemType;
	gamedata->FreeItem(itm, slot->ItemResRef);
	if (itemtype==parameters->int1Parameter) {
		return 1;
	}
	return 0;
}

int GameScript::HasItemEquipped(Scriptable * Sender, const Trigger *parameters)
{
	const Scriptable *scr = GetActorFromObject(Sender, parameters->objectParameter);
	if ( !scr || scr->Type!=ST_ACTOR) {
		return 0;
	}
	const Actor *actor = (const Actor *) scr;
	// HACK: temporarily look at all items, since we now set the bit only for weapons
	// bg2/ddguard7.baf is the only user with something else - the strohm mask helmet
	if (actor->inventory.HasItem(parameters->string0Parameter, IE_INV_ITEM_EQUIPPED*0) ) {
		return 1;
	}
	return 0;
}

int GameScript::Acquired(Scriptable * Sender, const Trigger *parameters)
{
	if ( Sender->Type!=ST_ACTOR) {
		return 0;
	}
	const Actor *actor = (const Actor *) Sender;
	if (actor->inventory.HasItem(parameters->string0Parameter, IE_INV_ITEM_ACQUIRED) ) {
		return 1;
	}
	return 0;
}

/** this trigger accepts a numeric parameter, this number is the same as inventory flags
 like: 1 - identified, 2 - unstealable, 4 - stolen, 8 - undroppable, etc. */
/** this is a GemRB extension */
int GameScript::PartyHasItem(Scriptable * /*Sender*/, const Trigger *parameters)
{
	const Game *game = core->GetGame();

	int i = game->GetPartySize(true);
	while(i--) {
		const Actor *actor = game->GetPC(i, true);
		if (HasItemCore(&actor->inventory, parameters->string0Parameter, parameters->int0Parameter) ) {
			return 1;
		}
	}
	return 0;
}

int GameScript::PartyHasItemIdentified(Scriptable * /*Sender*/, const Trigger *parameters)
{
	const Game *game = core->GetGame();

	int i = game->GetPartySize(true);
	while(i--) {
		const Actor *actor = game->GetPC(i, true);
		if (HasItemCore(&actor->inventory, parameters->string0Parameter, IE_INV_ITEM_IDENTIFIED) ) {
			return 1;
		}
	}
	return 0;
}

int GameScript::InventoryFull( Scriptable *Sender, const Trigger *parameters)
{
	const Scriptable *tar = GetActorFromObject(Sender, parameters->objectParameter);
	if (!tar || tar->Type!=ST_ACTOR) {
		return 0;
	}
	const Actor *actor = (const Actor *) tar;
	if (actor->inventory.FindCandidateSlot(SLOT_INVENTORY, 0) == -1) {
		return 1;
	}
	return 0;
}

int GameScript::HasInnateAbility(Scriptable *Sender, const Trigger *parameters)
{
	Scriptable *tar = GetActorFromObject(Sender, parameters->objectParameter);
	if (!tar || tar->Type!=ST_ACTOR) {
		return 0;
	}
	Actor *actor = (Actor *) tar;
	if (parameters->string0Parameter[0]) {
		return actor->spellbook.HaveSpell(parameters->string0Parameter, 0);
	}
	return actor->spellbook.HaveSpell(parameters->int0Parameter, 0);
}

int GameScript::HaveSpell(Scriptable *Sender, const Trigger *parameters)
{
	if (Sender->Type!=ST_ACTOR) {
		return 0;
	}

	if (parameters->int0Parameter == 0 && Sender->LastMarkedSpell == 0) {
		return false;
	}

	Actor *actor = (Actor *) Sender;
	if (parameters->string0Parameter[0]) {
		return actor->spellbook.HaveSpell(parameters->string0Parameter, 0);
	}
	int spellNum = parameters->int0Parameter;
	if (!spellNum) spellNum = Sender->LastMarkedSpell;
	return actor->spellbook.HaveSpell(spellNum, 0);
}

int GameScript::HaveAnySpells(Scriptable *Sender, const Trigger */*parameters*/)
{
	if (Sender->Type!=ST_ACTOR) {
		return 0;
	}
	Actor *actor = (Actor *) Sender;
	return actor->spellbook.HaveSpell("", 0);
}

int GameScript::HaveSpellParty(Scriptable */*Sender*/, const Trigger *parameters)
{
	const Game *game = core->GetGame();

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

int GameScript::KnowSpell(Scriptable *Sender, const Trigger *parameters)
{
	if (Sender->Type!=ST_ACTOR) {
		return 0;
	}
	const Actor *actor = (const Actor *) Sender;
	if (parameters->string0Parameter[0]) {
		return actor->spellbook.KnowSpell(parameters->string0Parameter);
	}
	return actor->spellbook.KnowSpell(parameters->int0Parameter);
}

int GameScript::True(Scriptable */* Sender*/, const Trigger */*parameters*/)
{
	return 1;
}

//in fact this could be used only on Sender, but we want to enhance these
//triggers and actions to accept an object argument whenever possible.
//0 defaults to Myself (Sender)
int GameScript::NumTimesTalkedTo(Scriptable *Sender, const Trigger *parameters)
{
	const Scriptable *scr = GetActorFromObject(Sender, parameters->objectParameter);
	if (!scr) {
		scr = Sender;
	}
	if (scr->Type != ST_ACTOR) {
		return 0;
	}
	const Actor *actor = (const Actor *) scr;
	return actor->TalkCount == (ieDword) parameters->int0Parameter ? 1 : 0;
}

int GameScript::NumTimesTalkedToGT(Scriptable *Sender, const Trigger *parameters)
{
	const Scriptable *scr = GetActorFromObject(Sender, parameters->objectParameter);
	if (!scr) {
		scr = Sender;
	}
	if (scr->Type != ST_ACTOR) {
		return 0;
	}
	const Actor *actor = (const Actor *) scr;
	return actor->TalkCount > (ieDword) parameters->int0Parameter ? 1 : 0;
}

int GameScript::NumTimesTalkedToLT(Scriptable *Sender, const Trigger *parameters)
{
	const Scriptable *scr = GetActorFromObject(Sender, parameters->objectParameter);
	if (!scr) {
		scr = Sender;
	}
	if (scr->Type != ST_ACTOR) {
		return 0;
	}
	const Actor *actor = (const Actor *) scr;
	return actor->TalkCount < (ieDword) parameters->int0Parameter ? 1 : 0;
}

int GameScript::NumTimesInteracted(Scriptable *Sender, const Trigger *parameters)
{
	const Scriptable *scr = GetActorFromObject(Sender, parameters->objectParameter);
	if (!scr) {
		scr = Sender;
	}
	if (scr->Type != ST_ACTOR) {
		return 0;
	}
	const Actor *actor = (const Actor *) scr;
	ieDword npcid = parameters->int0Parameter;
	if (npcid>=MAX_INTERACT) return 0;
	if (!actor->PCStats) return 0;
	return actor->PCStats->Interact[npcid] == (ieDword) parameters->int1Parameter ? 1 : 0;
}

int GameScript::NumTimesInteractedGT(Scriptable *Sender, const Trigger *parameters)
{
	const Scriptable *scr = GetActorFromObject(Sender, parameters->objectParameter);
	if (!scr) {
		scr = Sender;
	}
	if (scr->Type != ST_ACTOR) {
		return 0;
	}
	const Actor *actor = (const Actor *) scr;
	ieDword npcid = parameters->int0Parameter;
	if (npcid>=MAX_INTERACT) return 0;
	if (!actor->PCStats) return 0;
	return actor->PCStats->Interact[npcid] > (ieDword) parameters->int1Parameter ? 1 : 0;
}

int GameScript::NumTimesInteractedLT(Scriptable *Sender, const Trigger *parameters)
{
	const Scriptable *scr = GetActorFromObject(Sender, parameters->objectParameter);
	if (!scr) {
		scr = Sender;
	}
	if (scr->Type != ST_ACTOR) {
		return 0;
	}
	const Actor *actor = (const Actor *) scr;
	ieDword npcid = parameters->int0Parameter;
	if (npcid>=MAX_INTERACT) return 0;
	if (!actor->PCStats) return 0;
	return actor->PCStats->Interact[npcid] < (ieDword) parameters->int1Parameter ? 1 : 0;
}

//GemRB specific
//interacting npc counts were restricted to 24
//gemrb will increase a local variable in the interacting npc, with the scriptname of the
//target npc
int GameScript::NumTimesInteractedObject(Scriptable *Sender, const Trigger *parameters)
{
	if (Sender->Type != ST_ACTOR) {
		return 0;
	}

	const Scriptable *scr = GetActorFromObject(Sender, parameters->objectParameter);
	if (!scr) {
		return 0;
	}
	if (scr->Type != ST_ACTOR) {
		return 0;
	}
	const Actor *tar = (const Actor *) scr;
	return CheckVariable(Sender, tar->GetScriptName(), "LOCALS") == (ieDword) parameters->int0Parameter ? 1 : 0;
}

int GameScript::NumTimesInteractedObjectGT(Scriptable *Sender, const Trigger *parameters)
{
	if (Sender->Type != ST_ACTOR) {
		return 0;
	}

	const Scriptable *scr = GetActorFromObject(Sender, parameters->objectParameter);
	if (!scr) {
		return 0;
	}
	if (scr->Type != ST_ACTOR) {
		return 0;
	}
	const Actor *tar = (const Actor *) scr;
	return CheckVariable(Sender, tar->GetScriptName(), "LOCALS") > (ieDword) parameters->int0Parameter ? 1 : 0;
}

int GameScript::NumTimesInteractedObjectLT(Scriptable *Sender, const Trigger *parameters)
{
	if (Sender->Type != ST_ACTOR) {
		return 0;
	}

	const Scriptable *scr = GetActorFromObject(Sender, parameters->objectParameter);
	if (!scr) {
		return 0;
	}
	if (scr->Type != ST_ACTOR) {
		return 0;
	}
	const Actor *tar = (const Actor *) scr;
	return CheckVariable(Sender, tar->GetScriptName(), "LOCALS") < (ieDword) parameters->int0Parameter ? 1 : 0;
}

int GameScript::ObjectActionListEmpty(Scriptable *Sender, const Trigger *parameters)
{
	const Scriptable *scr = GetActorFromObject(Sender, parameters->objectParameter);
	if (!scr) {
		return 0;
	}

	// added CurrentAction as part of blocking action fixes
	if (scr->GetCurrentAction() || scr->GetNextAction()) {
		return 0;
	}
	return 1;
}

int GameScript::ActionListEmpty(Scriptable *Sender, const Trigger */*parameters*/)
{
	// added CurrentAction as part of blocking action fixes
	if (Sender->GetCurrentAction() || Sender->GetNextAction()) {
		return 0;
	}
	return 1;
}

int GameScript::False(Scriptable */*Sender*/, const Trigger */*parameters*/)
{
	return 0;
}

/* i guess this is a range of circle edges (instead of centers) */
int GameScript::PersonalSpaceDistance(Scriptable *Sender, const Trigger *parameters)
{
	const Scriptable *scr = GetActorFromObject(Sender, parameters->objectParameter);
	if (!scr) {
		return 0;
	}

	if (WithinPersonalRange(scr, Sender, parameters->int0Parameter)) {
		return 1;
	}
	return 0;
}

int GameScript::Range(Scriptable *Sender, const Trigger *parameters)
{
	const Scriptable *scr = GetActorFromObject(Sender, parameters->objectParameter);
	if (!scr) {
		return 0;
	}
	if (Sender->GetCurrentArea() != scr->GetCurrentArea()) {
		return 0;
	}
	if (Sender->Type == ST_ACTOR) {
		Sender->LastMarked = scr->GetGlobalID();
	}
	int distance = SquaredMapDistance(Sender, scr);
	bool matched = DiffCore(distance, (parameters->int0Parameter + 1) * (parameters->int0Parameter + 1), parameters->int1Parameter);
	if (matched) {
		Sender->SetLastTrigger(trigger_range, scr->GetGlobalID());
	}
	return matched;
}

int GameScript::InLine(Scriptable *Sender, const Trigger *parameters)
{
	const Map *map = Sender->GetCurrentArea();
	if (!map) {
		return 0;
	}

	const Scriptable *scr1 = GetActorFromObject(Sender, parameters->objectParameter);
	if (!scr1) {
		return 0;
	}

	//looking for a scriptable by scriptname only
	const Scriptable *scr2 = map->GetActor(parameters->string0Parameter, 0);
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
int GameScript::AtLocation( Scriptable *Sender, const Trigger *parameters)
{
	const Scriptable *tar = GetActorFromObject(Sender, parameters->objectParameter);
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
//  and -2,-2 is treated specially in iwd2 (Jorun in Targos)
int GameScript::NearLocation(Scriptable *Sender, const Trigger *parameters)
{
	const Scriptable *scr = GetActorFromObject(Sender, parameters->objectParameter);
	if (!scr) {
		return 0;
	}
	if (parameters->pointParameter.isnull()) {
		int distance;
		if (parameters->int0Parameter < 0) { // use Sender's position
			distance = PersonalDistance(Sender, scr);
		} else {
			Point p((short) parameters->int0Parameter, (short) parameters->int1Parameter);
			distance = PersonalDistance(p, scr);
		}
		if (distance <= (parameters->int2Parameter * VOODOO_NEARLOC_F)) {
			return 1;
		}
		return 0;
	}
	//personaldistance is needed for modron constructs in PST maze
	int distance = PersonalDistance(parameters->pointParameter, scr);
	if (distance <= (parameters->int0Parameter * VOODOO_NEARLOC_F)) {
		return 1;
	}
	return 0;
}

// EEs extend this to NearSavedLocation(O:Object*,S:Global*,I:Range*)
int GameScript::NearSavedLocation(Scriptable *Sender, const Trigger *parameters)
{
	if (Sender->Type!=ST_ACTOR) {
		return 0;
	}
	if (core->HasFeature(GF_HAS_KAPUTZ)) {
		// TODO: we don't understand how this works in pst yet
		Log(ERROR, "GameScript", "Aborting NearSavedLocation! Don't know what to do!");
		return 1;
	}
	const Actor *actor = (const Actor *) Sender;
	Point p;
	if ((signed) actor->GetStat(IE_SAVEDXPOS) <= 0 && (signed) actor->GetStat(IE_SAVEDYPOS) <= 0) {
		p = actor->HomeLocation;
	} else {
		p.x = (short) actor->GetStat(IE_SAVEDXPOS);
		p.y = (short) actor->GetStat(IE_SAVEDYPOS);
	}
	// should this be PersonalDistance?
	int distance = Distance(p, Sender);
	if (distance <= (parameters->int0Parameter * VOODOO_NEARLOC_F)) {
		return 1;
	}
	return 0;
}

int GameScript::Or(Scriptable */*Sender*/, const Trigger *parameters)
{
	return parameters->int0Parameter;
}

int GameScript::TriggerTrigger(Scriptable *Sender, const Trigger *parameters)
{
	return Sender->MatchTrigger(trigger_trigger, parameters->int0Parameter);
}

int GameScript::WalkedToTrigger(Scriptable *Sender, const Trigger *parameters)
{
	return Sender->MatchTriggerWithObject(trigger_walkedtotrigger, parameters->objectParameter);
}

int GameScript::Clicked(Scriptable *Sender, const Trigger *parameters)
{
	return Sender->MatchTriggerWithObject(trigger_clicked, parameters->objectParameter);
}

int GameScript::Disarmed(Scriptable *Sender, const Trigger *parameters)
{
	return Sender->MatchTriggerWithObject(trigger_disarmed, parameters->objectParameter);
}

//stealing from a store failed, owner triggered
int GameScript::StealFailed(Scriptable *Sender, const Trigger *parameters)
{
	return Sender->MatchTriggerWithObject(trigger_stealfailed, parameters->objectParameter);
}

int GameScript::PickpocketFailed(Scriptable *Sender, const Trigger *parameters)
{
	return Sender->MatchTriggerWithObject(trigger_pickpocketfailed, parameters->objectParameter);
}

int GameScript::PickLockFailed(Scriptable *Sender, const Trigger *parameters)
{
	return Sender->MatchTriggerWithObject(trigger_picklockfailed, parameters->objectParameter);
}

int GameScript::OpenFailed(Scriptable *Sender, const Trigger *parameters)
{
	return Sender->MatchTriggerWithObject(trigger_failedtoopen, parameters->objectParameter);
}

int GameScript::DisarmFailed(Scriptable *Sender, const Trigger *parameters)
{
	return Sender->MatchTriggerWithObject(trigger_disarmfailed, parameters->objectParameter);
}

//opened for doors/containers
int GameScript::Opened(Scriptable *Sender, const Trigger *parameters)
{
	return Sender->MatchTriggerWithObject(trigger_opened, parameters->objectParameter);
}

int GameScript::HarmlessOpened(Scriptable *Sender, const Trigger *parameters)
{
	return Sender->MatchTriggerWithObject(trigger_harmlessopened, parameters->objectParameter);
}

//closed for doors
int GameScript::Closed(Scriptable *Sender, const Trigger *parameters)
{
	return Sender->MatchTriggerWithObject(trigger_closed, parameters->objectParameter);
}

int GameScript::HarmlessClosed(Scriptable *Sender, const Trigger *parameters)
{
	return Sender->MatchTriggerWithObject(trigger_harmlessclosed, parameters->objectParameter);
}

//unlocked for doors/containers (using lastUnlocked)
int GameScript::Unlocked(Scriptable *Sender, const Trigger *parameters)
{
	return Sender->MatchTriggerWithObject(trigger_unlocked, parameters->objectParameter);
}

int GameScript::Entered(Scriptable *Sender, const Trigger *parameters)
{
	return Sender->MatchTriggerWithObject(trigger_entered, parameters->objectParameter);
}

int GameScript::HarmlessEntered(Scriptable *Sender, const Trigger *parameters)
{
	return Sender->MatchTriggerWithObject(trigger_harmlessentered, parameters->objectParameter);
}

int GameScript::IsOverMe(Scriptable *Sender, const Trigger *parameters)
{
	if (Sender->Type != ST_PROXIMITY) {
		return 0;
	}
	const Highlightable *trap = (Highlightable *) Sender;

	Targets *tgts = GetAllObjects(Sender->GetCurrentArea(), Sender, parameters->objectParameter, GA_NO_DEAD|GA_NO_UNSCHEDULED);
	int ret = 0;
	if (tgts) {
		targetlist::iterator m;
		const targettype *tt = tgts->GetFirstTarget(m, ST_ACTOR);
		while (tt) {
			const Actor *actor = (Actor *) tt->actor;
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
int GameScript::Dead(Scriptable *Sender, const Trigger *parameters)
{
	if (parameters->string0Parameter[0]) {
		ieDword value;
		ieVariable Variable;
		size_t len;

		if (core->HasFeature( GF_HAS_KAPUTZ )) {
			len = snprintf(Variable,sizeof(ieVariable),"%s_DEAD",parameters->string0Parameter);
			value = CheckVariable( Sender, Variable, "KAPUTZ");
		} else {
			len = snprintf(Variable, sizeof(ieVariable), core->GetDeathVarFormat(), parameters->string0Parameter);
			value = CheckVariable( Sender, Variable, "GLOBAL" );
		}
		if (len > sizeof(ieVariable)) {
			Log(ERROR, "GameScript", "Scriptname %s (sender: %s) is too long for generating death globals!", parameters->string0Parameter, Sender->GetScriptName());
		}
		if (value>0) {
			return 1;
		}
		return 0;
	}
	const Scriptable *target = GetActorFromObject(Sender, parameters->objectParameter);
	if (!target) {
		return 1;
	}
	if (target->Type != ST_ACTOR) {
		return 1;
	}
	const Actor *actor = (const Actor *) target;
	// actors not meeting AreaDifficulty get deleted before we have to worry about them
	if (actor->GetStat( IE_STATE_ID ) & STATE_DEAD) {
		return 1;
	}
	return 0;
}

int GameScript::CreatureHidden(Scriptable *Sender, const Trigger *parameters)
{
	Scriptable *target = GetActorFromObject(Sender, parameters->objectParameter);
	if (!target) {
		return 0;
	}
	if (target->Type != ST_ACTOR) {
		return 0;
	}
	const Actor *act = (const Actor *) target;

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
int GameScript::BecameVisible(Scriptable *Sender, const Trigger */*parameters*/)
{
	return Sender->MatchTrigger(trigger_becamevisible);
}

int GameScript::Die(Scriptable *Sender, const Trigger */*parameters*/)
{
	return Sender->MatchTrigger(trigger_die);
}

int GameScript::Died(Scriptable *Sender, const Trigger *parameters)
{
	return Sender->MatchTriggerWithObject(trigger_died, parameters->objectParameter);
}

int GameScript::PartyMemberDied(Scriptable *Sender, const Trigger *parameters)
{
	return Sender->MatchTriggerWithObject(trigger_partymemberdied, parameters->objectParameter);
}

int GameScript::NamelessBitTheDust(Scriptable *Sender, const Trigger */*parameters*/)
{
	return Sender->MatchTrigger(trigger_namelessbitthedust);
}

int GameScript::Killed(Scriptable *Sender, const Trigger *parameters)
{
	return Sender->MatchTriggerWithObject(trigger_killed, parameters->objectParameter);
}

int GameScript::Race(Scriptable *Sender, const Trigger *parameters)
{
	const Scriptable *scr = GetActorFromObject(Sender, parameters->objectParameter);
	if (!scr) {
		return 0;
	}
	if (scr->Type != ST_ACTOR) {
		return 0;
	}
	const Actor *actor = (const Actor *) scr;
	bool matched = ID_Race(actor, parameters->int0Parameter);
	if (matched) {
		Sender->SetLastTrigger(trigger_race, actor->GetGlobalID());
	}
	return matched;
}

int GameScript::Gender(Scriptable *Sender, const Trigger *parameters)
{
	const Scriptable *scr = GetActorFromObject(Sender, parameters->objectParameter);
	if (!scr) {
		return 0;
	}
	if (scr->Type != ST_ACTOR) {
		return 0;
	}
	const Actor *actor = (const Actor *) scr;
	bool matched = ID_Gender(actor, parameters->int0Parameter);
	if (matched) {
		Sender->SetLastTrigger(trigger_gender, actor->GetGlobalID());
	}
	return matched;
}

int GameScript::HP(Scriptable *Sender, const Trigger *parameters)
{
	const Scriptable *scr = GetActorFromObject(Sender, parameters->objectParameter);
	if (!scr) {
		return 0;
	}
	if (scr->Type != ST_ACTOR) {
		return 0;
	}
	const Actor *actor = (const Actor *) scr;
	if ((signed) actor->GetBase(IE_HITPOINTS) == parameters->int0Parameter) {
		Sender->SetLastTrigger(trigger_hpgt, actor->GetGlobalID());
		return 1;
	}
	return 0;
}

int GameScript::HPGT(Scriptable *Sender, const Trigger *parameters)
{
	const Scriptable *scr = GetActorFromObject(Sender, parameters->objectParameter);
	if (!scr) {
		return 0;
	}
	if (scr->Type != ST_ACTOR) {
		return 0;
	}
	const Actor *actor = (const Actor *) scr;
	if ((signed) actor->GetBase(IE_HITPOINTS) > parameters->int0Parameter) {
		Sender->SetLastTrigger(trigger_hpgt, actor->GetGlobalID());
		return 1;
	}
	return 0;
}

int GameScript::HPLT(Scriptable *Sender, const Trigger *parameters)
{
	const Scriptable *scr = GetActorFromObject(Sender, parameters->objectParameter);
	if (!scr) {
		return 0;
	}
	if (scr->Type != ST_ACTOR) {
		return 0;
	}
	const Actor *actor = (const Actor *) scr;
	if ((signed) actor->GetBase(IE_HITPOINTS) < parameters->int0Parameter) {
		Sender->SetLastTrigger(trigger_hpgt, actor->GetGlobalID());
		return 1;
	}
	return 0;
}

//these triggers work on the current damage (not the last damage)
//actually, they use lastdamage
int GameScript::DamageTaken(Scriptable *Sender, const Trigger *parameters)
{
	if (Sender->Type!=ST_ACTOR) {
		return 0;
	}
	const Actor *actor = (const Actor *) Sender;
	int damage = actor->LastDamage;
	if (damage == parameters->int0Parameter) {
		return 1;
	}
	return 0;
}

int GameScript::DamageTakenGT(Scriptable *Sender, const Trigger *parameters)
{
	if (Sender->Type!=ST_ACTOR) {
		return 0;
	}
	const Actor *actor = (const Actor *) Sender;
	int damage = actor->LastDamage;
	if (damage > parameters->int0Parameter) {
		return 1;
	}
	return 0;
}

int GameScript::DamageTakenLT(Scriptable *Sender, const Trigger *parameters)
{
	if (Sender->Type!=ST_ACTOR) {
		return 0;
	}
	const Actor *actor = (const Actor *) Sender;
	int damage = actor->LastDamage;
	if (damage < parameters->int0Parameter) {
		return 1;
	}
	return 0;
}

int GameScript::HPLost(Scriptable *Sender, const Trigger *parameters)
{
	const Scriptable *scr = GetActorFromObject(Sender, parameters->objectParameter);
	if (!scr) {
		return 0;
	}
	if (scr->Type != ST_ACTOR) {
		return 0;
	}
	const Actor *actor = (const Actor *) scr;
	//max-current
	if ((signed) actor->GetStat(IE_MAXHITPOINTS) - (signed) actor->GetBase(IE_HITPOINTS) == parameters->int0Parameter) {
		return 1;
	}
	return 0;
}

int GameScript::HPLostGT(Scriptable *Sender, const Trigger *parameters)
{
	const Scriptable *scr = GetActorFromObject(Sender, parameters->objectParameter);
	if (!scr) {
		return 0;
	}
	if (scr->Type != ST_ACTOR) {
		return 0;
	}
	const Actor *actor = (const Actor *) scr;
	//max-current
	if ((signed) actor->GetStat(IE_MAXHITPOINTS) - (signed) actor->GetBase(IE_HITPOINTS) > parameters->int0Parameter) {
		return 1;
	}
	return 0;
}

int GameScript::HPLostLT(Scriptable *Sender, const Trigger *parameters)
{
	const Scriptable *scr = GetActorFromObject(Sender, parameters->objectParameter);
	if (!scr) {
		return 0;
	}
	if (scr->Type != ST_ACTOR) {
		return 0;
	}
	const Actor *actor = (const Actor *) scr;
	//max-current
	if ((signed) actor->GetStat(IE_MAXHITPOINTS) - (signed) actor->GetBase(IE_HITPOINTS) < parameters->int0Parameter) {
		return 1;
	}
	return 0;
}

int GameScript::HPPercent(Scriptable *Sender, const Trigger *parameters)
{
	const Scriptable *scr = GetActorFromObject(Sender, parameters->objectParameter);
	if (!scr) {
		return 0;
	}
	if (GetHPPercent( scr ) == parameters->int0Parameter) {
		Sender->SetLastTrigger(trigger_hpgt, scr->GetGlobalID());
		return 1;
	}
	return 0;
}

int GameScript::HPPercentGT(Scriptable *Sender, const Trigger *parameters)
{
	const Scriptable *scr = GetActorFromObject(Sender, parameters->objectParameter);
	if (!scr) {
		return 0;
	}
	if (GetHPPercent( scr ) > parameters->int0Parameter) {
		Sender->SetLastTrigger(trigger_hpgt, scr->GetGlobalID());
		return 1;
	}
	return 0;
}

int GameScript::HPPercentLT(Scriptable *Sender, const Trigger *parameters)
{
	const Scriptable *scr = GetActorFromObject(Sender, parameters->objectParameter);
	if (!scr) {
		return 0;
	}
	if (GetHPPercent( scr ) < parameters->int0Parameter) {
		Sender->SetLastTrigger(trigger_hpgt, scr->GetGlobalID());
		return 1;
	}
	return 0;
}

int GameScript::XP(Scriptable *Sender, const Trigger *parameters)
{
	const Scriptable *scr = GetActorFromObject(Sender, parameters->objectParameter);
	if (!scr) {
		return 0;
	}
	if (scr->Type != ST_ACTOR) {
		return 0;
	}
	const Actor *actor = (const Actor *) scr;
	if (actor->GetStat( IE_XP ) == (unsigned) parameters->int0Parameter) {
		return 1;
	}
	return 0;
}

int GameScript::XPGT(Scriptable *Sender, const Trigger *parameters)
{
	const Scriptable *scr = GetActorFromObject(Sender, parameters->objectParameter);
	if (!scr) {
		return 0;
	}
	if (scr->Type != ST_ACTOR) {
		return 0;
	}
	const Actor *actor = (const Actor *) scr;
	if (actor->GetStat( IE_XP ) > (unsigned) parameters->int0Parameter) {
		return 1;
	}
	return 0;
}

int GameScript::XPLT(Scriptable *Sender, const Trigger *parameters)
{
	const Scriptable *scr = GetActorFromObject(Sender, parameters->objectParameter);
	if (!scr) {
		return 0;
	}
	if (scr->Type != ST_ACTOR) {
		return 0;
	}
	const Actor *actor = (const Actor *) scr;
	if (actor->GetStat( IE_XP ) < (unsigned) parameters->int0Parameter) {
		return 1;
	}
	return 0;
}

int GameScript::CheckSkill(Scriptable *Sender, const Trigger *parameters)
{
	const Scriptable *target = GetActorFromObject(Sender, parameters->objectParameter);
	if (!target) {
		return 0;
	}
	if (target->Type != ST_ACTOR) {
		return 0;
	}
	const Actor *actor = (const Actor *) target;
	int sk = actor->GetSkill(parameters->int1Parameter, true);
	if (sk<0) return 0;
	if (sk == parameters->int0Parameter) {
		return 1;
	}
	return 0;
}

int GameScript::CheckStat(Scriptable *Sender, const Trigger *parameters)
{
	const Scriptable *target = GetActorFromObject(Sender, parameters->objectParameter);
	if (!target) {
		return 0;
	}
	if (target->Type != ST_ACTOR) {
		return 0;
	}
	const Actor *actor = (const Actor *) target;
	if ((signed) actor->GetStat(parameters->int1Parameter) == parameters->int0Parameter) {
		Sender->SetLastTrigger(trigger_checkstat, actor->GetGlobalID());
		return 1;
	}
	return 0;
}

int GameScript::CheckSkillGT(Scriptable *Sender, const Trigger *parameters)
{
	const Scriptable *tar = GetActorFromObject(Sender, parameters->objectParameter);
	if (!tar || tar->Type != ST_ACTOR) {
		return 0;
	}
	const Actor *actor = (const Actor *) tar;
	int sk = actor->GetSkill(parameters->int1Parameter, true);
	if (sk<0) return 0;
	if (sk > parameters->int0Parameter) {
		Sender->SetLastTrigger(trigger_checkstat, actor->GetGlobalID());
		return 1;
	}
	return 0;
}

int GameScript::CheckStatGT(Scriptable *Sender, const Trigger *parameters)
{
	const Scriptable *tar = GetActorFromObject(Sender, parameters->objectParameter);
	if (!tar || tar->Type != ST_ACTOR) {
		return 0;
	}
	const Actor *actor = (const Actor *) tar;
	if ((signed) actor->GetStat(parameters->int1Parameter) > parameters->int0Parameter) {
		Sender->SetLastTrigger(trigger_checkstat, actor->GetGlobalID());
		return 1;
	}
	return 0;
}

int GameScript::CheckSkillLT(Scriptable *Sender, const Trigger *parameters)
{
	const Scriptable *tar = GetActorFromObject(Sender, parameters->objectParameter);
	if (!tar || tar->Type != ST_ACTOR) {
		return 0;
	}
	const Actor *actor = (const Actor *) tar;
	int sk = actor->GetSkill(parameters->int1Parameter, true);
	if (sk<0) return 0;
	if (sk < parameters->int0Parameter) {
		return 1;
	}
	return 0;
}

int GameScript::CheckStatLT(Scriptable *Sender, const Trigger *parameters)
{
	const Scriptable *tar = GetActorFromObject(Sender, parameters->objectParameter);
	if (!tar || tar->Type != ST_ACTOR) {
		return 0;
	}
	const Actor *actor = (const Actor *) tar;
	if ((signed) actor->GetStat(parameters->int1Parameter) < parameters->int0Parameter) {
		return 1;
	}
	return 0;
}

/* i believe this trigger is the same as 'MarkObject' action
 except that if it cannot set the marked object, it returns false */
int GameScript::SetLastMarkedObject(Scriptable *Sender, const Trigger *parameters)
{
	if (Sender->Type != ST_ACTOR) {
		return 0;
	}
	Actor *scr = (Actor *) Sender;

	const Scriptable *tar = GetActorFromObject(Sender, parameters->objectParameter);
	if (!tar || tar->Type != ST_ACTOR) {
		return 0;
	}
	scr->LastMarked = tar->GetGlobalID();
	return 1;
}

// TODO: should there be any more failure modes?
// iwd2 only
int GameScript::SetSpellTarget(Scriptable *Sender, const Trigger *parameters)
{
	if (Sender->Type != ST_ACTOR) {
		return 0;
	}
	Actor *scr = (Actor *) Sender;
	
	const Scriptable *tar = GetActorFromObject(Sender, parameters->objectParameter);
	if (!tar) {
		// we got called with Nothing to invalidate the target
		scr->LastSpellTarget = 0;
		scr->LastTargetPos.empty();
		return 1;
	}
	scr->LastTargetPos.empty();
	scr->LastSpellTarget = tar->GetGlobalID();
	return 1;
}

int GameScript::IsSpellTargetValid(Scriptable *Sender, const Trigger *parameters)
{
	if (Sender->Type != ST_ACTOR) {
		return 0;
	}
	Actor *scr = (Actor *) Sender;

	const Scriptable *tar = GetActorFromObject(Sender, parameters->objectParameter);
	if (!tar) {
		return 0;
	}
	const Actor *actor = nullptr;
	if (tar->Type == ST_ACTOR) {
		actor = (const Actor *) tar;
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
	if (!(flags & MSO_IGNORE_INVALID) && actor && actor->InvalidSpellTarget(splnum, scr, range)) {
		return 0;
	}
	return 1;
}

//This trigger seems to always return true for actors...
//Always manages to set spell to 0, otherwise it sets if there was nothing set earlier
int GameScript::SetMarkedSpell_Trigger(Scriptable *Sender, const Trigger *parameters)
{
	Action *params = new Action(true);
	params->int0Parameter = parameters->int0Parameter;
	GameScript::SetMarkedSpell(Sender, params);
	delete params;
	return 1;
}

int GameScript::ForceMarkedSpell_Trigger(Scriptable *Sender, const Trigger *parameters)
{
	if (Sender->Type != ST_ACTOR) {
		return 0;
	}
	Actor *scr = (Actor *) Sender;
	scr->LastMarkedSpell = parameters->int0Parameter;
	return 1;
}

int GameScript::IsMarkedSpell(Scriptable *Sender, const Trigger *parameters)
{
	if (Sender->Type != ST_ACTOR) {
		return 0;
	}
	const Actor *scr = (const Actor *) Sender;
	return scr->LastMarkedSpell == parameters->int0Parameter;
}


int GameScript::See(Scriptable *Sender, const Trigger *parameters)
{
	int see = SeeCore(Sender, parameters, 0);
	return see;
}

int GameScript::Detect(Scriptable *Sender, const Trigger *parameters)
{
	Trigger *params = new Trigger;
	params->int0Parameter = 1; //seedead/invis
	params->objectParameter = parameters->objectParameter;
	int see = SeeCore(Sender, params, 0);
	params->objectParameter = nullptr;
	params->Release();
	if (!see) {
		return 0;
	}
	return 1;
}

int GameScript::LOS(Scriptable *Sender, const Trigger *parameters)
{
	int see=SeeCore(Sender, parameters, 1);
	if (!see) {
		return 0;
	}
	return Range(Sender, parameters); //same as range
}

int GameScript::NumCreatures(Scriptable *Sender, const Trigger *parameters)
{
	int value = GetObjectCount(Sender, parameters->objectParameter);
	bool matched = value == parameters->int0Parameter;
	// NOTE: in the original this is also supposed to set LastTarget, but it's not
	// clear to which of the possibly several actors, however
	// there are only two users and neither relies on LastTrigger, so we ignore it
	return matched;
}

int GameScript::NumCreaturesAtMyLevel(Scriptable *Sender, const Trigger *parameters)
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

int GameScript::NumCreaturesLT(Scriptable *Sender, const Trigger *parameters)
{
	int value = GetObjectCount(Sender, parameters->objectParameter);
	// see NOTE in NumCreatures; there are more users of LT and GT, but still none rely on LastTrigger
	return value < parameters->int0Parameter;
}

int GameScript::NumCreaturesLTMyLevel(Scriptable *Sender, const Trigger *parameters)
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

int GameScript::NumCreaturesGT(Scriptable *Sender, const Trigger *parameters)
{
	int value = GetObjectCount(Sender, parameters->objectParameter);
	// see NOTE in NumCreatures
	return value > parameters->int0Parameter;
}

int GameScript::NumCreaturesGTMyLevel(Scriptable *Sender, const Trigger *parameters)
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

int GameScript::NumCreatureVsParty(Scriptable *Sender, const Trigger *parameters)
{
	//creating object on the spot
	Object *obj = parameters->objectParameter;
	if (!obj) {
		obj = new Object();
	}
	int value = GetObjectCount(Sender, obj);
	if (!obj->isNull()) obj->Release();
	value -= core->GetGame()->GetPartySize(true);
	return value == parameters->int0Parameter;
}

int GameScript::NumCreatureVsPartyGT(Scriptable *Sender, const Trigger *parameters)
{
	Object *obj = parameters->objectParameter;
	if (!obj) {
		obj = new Object();
	}
	int value = GetObjectCount(Sender, obj);
	if (!obj->isNull()) obj->Release();
	value -= core->GetGame()->GetPartySize(true);
	return value > parameters->int0Parameter;
}

int GameScript::NumCreatureVsPartyLT(Scriptable *Sender, const Trigger *parameters)
{
	Object *obj = parameters->objectParameter;
	if (!obj) {
		obj = new Object();
	}
	int value = GetObjectCount(Sender, obj);
	if (!obj->isNull()) obj->Release();
	value -= core->GetGame()->GetPartySize(true);
	return value < parameters->int0Parameter;
}

int GameScript::Morale(Scriptable *Sender, const Trigger *parameters)
{
	const Scriptable *tar = GetActorFromObject(Sender, parameters->objectParameter);
	if (!tar || tar->Type != ST_ACTOR) {
		return 0;
	}
	const Actor *actor = (const Actor *) tar;
	bool matched = (signed) actor->GetStat(IE_MORALEBREAK) == parameters->int0Parameter;
	if (matched) {
		Sender->SetLastTrigger(trigger_morale, actor->GetGlobalID());
	}
	return matched;
}

int GameScript::MoraleGT(Scriptable *Sender, const Trigger *parameters)
{
	const Scriptable *tar = GetActorFromObject(Sender, parameters->objectParameter);
	if (!tar || tar->Type != ST_ACTOR) {
		return 0;
	}
	const Actor *actor = (const Actor *) tar;
	bool matched = (signed) actor->GetStat(IE_MORALEBREAK) > parameters->int0Parameter;
	if (matched) {
		Sender->SetLastTrigger(trigger_morale, actor->GetGlobalID());
	}
	return matched;
}

int GameScript::MoraleLT(Scriptable *Sender, const Trigger *parameters)
{
	const Scriptable *tar = GetActorFromObject(Sender, parameters->objectParameter);
	if (!tar || tar->Type != ST_ACTOR) {
		return 0;
	}
	const Actor *actor = (const Actor *) tar;
	bool matched = (signed) actor->GetStat(IE_MORALEBREAK) < parameters->int0Parameter;
	if (matched) {
		Sender->SetLastTrigger(trigger_morale, actor->GetGlobalID());
	}
	return matched;
}

int GameScript::CheckSpellState(Scriptable *Sender, const Trigger *parameters)
{
	const Scriptable *tar = GetActorFromObject(Sender, parameters->objectParameter);
	if (!tar || tar->Type != ST_ACTOR) {
		return 0;
	}
	const Actor *actor = (const Actor *) tar;
	if (parameters->int0Parameter>255) {
		return 0;
	}
	unsigned int position = parameters->int0Parameter>>5;
	unsigned int bit = 1<<(parameters->int0Parameter&31);
	if (actor->spellStates[position] & bit) {
		return 1;
	}
	return 0;
}

int GameScript::StateCheck(Scriptable *Sender, const Trigger *parameters)
{
	const Scriptable *tar = GetActorFromObject(Sender, parameters->objectParameter);
	if (!tar || tar->Type != ST_ACTOR) {
		return 0;
	}
	const Actor *actor = (const Actor *) tar;
	if (actor->GetStat(IE_STATE_ID) & parameters->int0Parameter) {
		Sender->SetLastTrigger(trigger_statecheck, tar->GetGlobalID());
		return 1;
	}
	return 0;
}

int GameScript::ExtendedStateCheck(Scriptable *Sender, const Trigger *parameters)
{
	const Scriptable *tar = GetActorFromObject(Sender, parameters->objectParameter);
	if (!tar || tar->Type != ST_ACTOR) {
		return 0;
	}
	const Actor *actor = (const Actor *) tar;
	if (actor->GetStat(IE_EXTSTATE_ID) & parameters->int0Parameter) {
		return 1;
	}
	return 0;
}

int GameScript::NotStateCheck(Scriptable *Sender, const Trigger *parameters)
{
	const Scriptable *tar = GetActorFromObject(Sender, parameters->objectParameter);
	if (!tar || tar->Type != ST_ACTOR) {
		return 0;
	}
	const Actor *actor = (const Actor *) tar;
	if (actor->GetStat(IE_STATE_ID) & ~parameters->int0Parameter) {
		Sender->SetLastTrigger(trigger_statecheck, tar->GetGlobalID());
		return 1;
	}
	return 0;
}

int GameScript::RandomNum(Scriptable */*Sender*/, const Trigger *parameters)
{
	if (parameters->int0Parameter <= 0 || parameters->int1Parameter <= 0) {
		return 0;
	}
	return parameters->int1Parameter-1 == RandomNumValue%parameters->int0Parameter;
}

int GameScript::RandomNumGT(Scriptable */*Sender*/, const Trigger *parameters)
{
	if (parameters->int0Parameter <= 0 || parameters->int1Parameter <= 0) {
		return 0;
	}
	return parameters->int1Parameter-1 < RandomNumValue%parameters->int0Parameter;
}

int GameScript::RandomNumLT(Scriptable */*Sender*/, const Trigger *parameters)
{
	if (parameters->int0Parameter <= 0 || parameters->int1Parameter <= 0) {
		return 0;
	}
	return parameters->int1Parameter-1 > RandomNumValue%parameters->int0Parameter;
}

int GameScript::OpenState(Scriptable *Sender, const Trigger *parameters)
{
	const Scriptable *tar = GetActorFromObject(Sender, parameters->objectParameter);
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
			const Door *door = (const Door *) tar;
			return !door->IsOpen() == !parameters->int0Parameter;
		}
		case ST_CONTAINER:
		{
			const Container *cont = (const Container *) tar;
			return !(cont->Flags&CONT_LOCKED) == !parameters->int0Parameter;
		}
		default:; //to remove a warning
	}
	Log(ERROR, "GameScript", "OpenState: Not a door/container: %s", tar->GetScriptName());
	return 0;
}

int GameScript::IsLocked(Scriptable *Sender, const Trigger *parameters)
{
	const Scriptable *tar = GetActorFromObject(Sender, parameters->objectParameter);
	if (!tar) {
		Log(ERROR, "GameScript", "Couldn't find door/container:%s",
			parameters->objectParameter? parameters->objectParameter->objectName:"<NULL>");
		print("Sender: %s", Sender->GetScriptName());
		return 0;
	}
	switch(tar->Type) {
		case ST_DOOR:
		{
			const Door *door = (const Door *) tar;
			return !!(door->Flags&DOOR_LOCKED);
		}
		case ST_CONTAINER:
		{
			const Container *cont = (const Container *) tar;
			return !!(cont->Flags&CONT_LOCKED);
		}
		default:; //to remove a warning
	}
	Log(ERROR, "GameScript", "IsLocked: Not a door/container: %s", tar->GetScriptName());
	return 0;
}

int GameScript::Level(Scriptable *Sender, const Trigger *parameters)
{
	const Scriptable *tar = GetActorFromObject(Sender, parameters->objectParameter);
	if (!tar) {
		return 0;
	}
	if (tar->Type != ST_ACTOR) {
		return 0;
	}
	const Actor *actor = (const Actor *) tar;
	// NOTE: which level to check is handled in GetXPLevel. The only user outside of iwd2
	// is the bg2 druid grove challenge. Fighter/druid is the only applicable multiclass,
	// but the dialog checks for level 14, which both classes reach at the same xp.
	// However, checking the gt/lt variants in gorodr1.d (wk start) it becomes clear
	// that multiclasses check against their rounded average, like elsewhere:
	// f/d levels (8/10: 9 < 10, 8/11: 9.5 >= 10 )
	return actor->GetXPLevel(true) == (unsigned) parameters->int0Parameter;
}

// works intuitively only with single-classed characters — the way the originals use it
int GameScript::ClassLevel(Scriptable *Sender, const Trigger *parameters)
{
	const Scriptable *tar = GetActorFromObject(Sender, parameters->objectParameter);
	if (!tar) {
		return 0;
	}
	if (tar->Type != ST_ACTOR) {
		return 0;
	}
	const Actor *actor = (const Actor *) tar;
	return actor->GetLevelInClass(parameters->int0Parameter) == (unsigned) parameters->int1Parameter;
}

// iwd2 and pst have different order of parameters:
// ClassLevelGT(Protagonist,MAGE,10)
// LevelInClass(Myself,10,CLERIC)
int GameScript::LevelInClass(Scriptable *Sender, const Trigger *parameters)
{
	const Scriptable *tar = GetActorFromObject(Sender, parameters->objectParameter);
	if (!tar) {
		return 0;
	}
	if (tar->Type != ST_ACTOR) {
		return 0;
	}
	const Actor *actor = (const Actor *) tar;
	return actor->GetLevelInClass(parameters->int1Parameter) == (unsigned) parameters->int0Parameter;
}

int GameScript::LevelGT(Scriptable *Sender, const Trigger *parameters)
{
	const Scriptable *tar = GetActorFromObject(Sender, parameters->objectParameter);
	if (!tar) {
		return 0;
	}
	if (tar->Type != ST_ACTOR) {
		return 0;
	}
	const Actor *actor = (const Actor *) tar;
	return actor->GetXPLevel(true) > (unsigned) parameters->int0Parameter;
}

int GameScript::ClassLevelGT(Scriptable *Sender, const Trigger *parameters)
{
	const Scriptable *tar = GetActorFromObject(Sender, parameters->objectParameter);
	if (!tar) {
		return 0;
	}
	if (tar->Type != ST_ACTOR) {
		return 0;
	}
	const Actor *actor = (const Actor *) tar;
	return actor->GetLevelInClass(parameters->int0Parameter) > (unsigned) parameters->int1Parameter;
}

int GameScript::LevelInClassGT(Scriptable *Sender, const Trigger *parameters)
{
	const Scriptable *tar = GetActorFromObject(Sender, parameters->objectParameter);
	if (!tar) {
		return 0;
	}
	if (tar->Type != ST_ACTOR) {
		return 0;
	}
	const Actor *actor = (const Actor *) tar;
	return actor->GetLevelInClass(parameters->int1Parameter) > (unsigned) parameters->int0Parameter;
}

int GameScript::LevelLT(Scriptable *Sender, const Trigger *parameters)
{
	const Scriptable *tar = GetActorFromObject(Sender, parameters->objectParameter);
	if (!tar) {
		return 0;
	}
	if (tar->Type != ST_ACTOR) {
		return 0;
	}
	const Actor *actor = (const Actor *) tar;
	return actor->GetXPLevel(true) < (unsigned) parameters->int0Parameter;
}

int GameScript::ClassLevelLT(Scriptable *Sender, const Trigger *parameters)
{
	const Scriptable *tar = GetActorFromObject(Sender, parameters->objectParameter);
	if (!tar) {
		return 0;
	}
	if (tar->Type != ST_ACTOR) {
		return 0;
	}
	const Actor *actor = (const Actor *) tar;
	return actor->GetLevelInClass(parameters->int0Parameter) < (unsigned) parameters->int1Parameter;
}

int GameScript::LevelInClassLT(Scriptable *Sender, const Trigger *parameters)
{
	const Scriptable *tar = GetActorFromObject(Sender, parameters->objectParameter);
	if (!tar) {
		return 0;
	}
	if (tar->Type != ST_ACTOR) {
		return 0;
	}
	const Actor *actor = (const Actor *) tar;
	return actor->GetLevelInClass(parameters->int1Parameter) < (unsigned) parameters->int0Parameter;
}

int GameScript::UnselectableVariable(Scriptable *Sender, const Trigger *parameters)
{
	const Scriptable *tar = GetActorFromObject(Sender, parameters->objectParameter);
	if (!tar) {
		return 0;
	}
	return tar->UnselectableTimer == (unsigned) parameters->int0Parameter;
}

int GameScript::UnselectableVariableGT(Scriptable *Sender, const Trigger *parameters)
{
	const Scriptable *tar = GetActorFromObject(Sender, parameters->objectParameter);
	if (!tar) {
		return 0;
	}
	return tar->UnselectableTimer > (unsigned) parameters->int0Parameter;
}

int GameScript::UnselectableVariableLT(Scriptable *Sender, const Trigger *parameters)
{
	const Scriptable *tar = GetActorFromObject(Sender, parameters->objectParameter);
	if (!tar) {
		return 0;
	}
	return tar->UnselectableTimer < (unsigned) parameters->int0Parameter;
}

int GameScript::AreaCheck(Scriptable *Sender, const Trigger *parameters)
{
	const Map *area = Sender->GetCurrentArea();
	if (!area) return 0;

	if (!strnicmp(area->GetScriptName(), parameters->string0Parameter, 8)) {
		return 1;
	}
	return 0;
}

int GameScript::AreaCheckObject(Scriptable *Sender, const Trigger *parameters)
{
	const Scriptable *tar = GetActorFromObject(Sender, parameters->objectParameter);

	if (!tar) {
		return 0;
	}
	const Map *map = tar->GetCurrentArea();
	if (!map) {
		return 0;
	}
	if (!strnicmp(map->GetScriptName(), parameters->string0Parameter, 8)) {
		return 1;
	}
	return 0;
}

//lame iwd2 uses a numeric area identifier, this reduces its usability
int GameScript::CurrentAreaIs(Scriptable *Sender, const Trigger *parameters)
{
	const Scriptable *tar = GetActorFromObject(Sender, parameters->objectParameter);

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
int GameScript::AreaStartsWith(Scriptable *Sender, const Trigger *parameters)
{
	const Scriptable *tar = GetActorFromObject(Sender, parameters->objectParameter);

	if (!tar) {
		return 0;
	}
	ieResRef arearesref;
	if (parameters->string0Parameter[0]) {
		strnlwrcpy(arearesref, parameters->string0Parameter, 8);
	} else {
		strnlwrcpy(arearesref, "AR30", 8); //InWatchersKeep
	}
	size_t i = strlen(arearesref);
	if (!strnicmp(tar->GetCurrentArea()->GetScriptName(), arearesref, i)) {
		return 1;
	}
	return 0;
}

int GameScript::EntirePartyOnMap(Scriptable *Sender, const Trigger */*parameters*/)
{
	Map *map = Sender->GetCurrentArea();
	const Game *game = core->GetGame();
	int i=game->GetPartySize(true);
	while (i--) {
		const Actor *actor = game->GetPC(i, true);
		if (actor->GetCurrentArea() != map) {
			return 0;
		}
	}
	return 1;
}

int GameScript::AnyPCOnMap(Scriptable *Sender, const Trigger */*parameters*/)
{
	Map *map = Sender->GetCurrentArea();
	const Game *game = core->GetGame();
	int i=game->GetPartySize(true);
	while (i--) {
		const Actor *actor = game->GetPC(i,true);
		if (actor->GetCurrentArea() == map) {
			return 1;
		}
	}
	return 0;
}

int GameScript::InActiveArea(Scriptable *Sender, const Trigger *parameters)
{
	const Scriptable *tar = GetActorFromObject(Sender, parameters->objectParameter);
	if (!tar) {
		return 0;
	}
	if (core->GetGame()->GetCurrentArea() == tar->GetCurrentArea()) {
		return 1;
	}
	return 0;
}

int GameScript::InMyArea(Scriptable *Sender, const Trigger *parameters)
{
	const Scriptable *tar = GetActorFromObject(Sender, parameters->objectParameter);
	if (!tar) {
		return 0;
	}
	if (Sender->GetCurrentArea() == tar->GetCurrentArea()) {
		return 1;
	}
	return 0;
}

int GameScript::AreaType(Scriptable *Sender, const Trigger *parameters)
{
	const Map *map = Sender->GetCurrentArea();
	if (!map) {
		return 1;
	}
	return (map->AreaType&parameters->int0Parameter)>0;
}

int GameScript::IsExtendedNight( Scriptable *Sender, const Trigger */*parameters*/)
{
	const Map *map = Sender->GetCurrentArea();
	if (!map) {
		return 1;
	}
	if (map->AreaType&AT_EXTENDED_NIGHT) {
		return 1;
	}
	return 0;
}

int GameScript::AreaFlag(Scriptable *Sender, const Trigger *parameters)
{
	const Map *map = Sender->GetCurrentArea();
	if (!map) {
		return 1;
	}
	return (map->AreaFlags&parameters->int0Parameter)>0;
}

int GameScript::AreaRestDisabled(Scriptable *Sender, const Trigger */*parameters*/)
{
	const Map *map = Sender->GetCurrentArea();
	if (!map) {
		return 1;
	}
	if (map->AreaFlags&2) {
		return 1;
	}
	return 0;
}

int GameScript::TargetUnreachable(Scriptable *Sender, const Trigger */*parameters*/)
{
	return Sender->MatchTrigger(trigger_targetunreachable);
}

int GameScript::PartyCountEQ(Scriptable */*Sender*/, const Trigger *parameters)
{
	return core->GetGame()->GetPartySize(false) == parameters->int0Parameter;
}

int GameScript::PartyCountLT(Scriptable */*Sender*/, const Trigger *parameters)
{
	return core->GetGame()->GetPartySize(false) < parameters->int0Parameter;
}

int GameScript::PartyCountGT(Scriptable */*Sender*/, const Trigger *parameters)
{
	return core->GetGame()->GetPartySize(false) > parameters->int0Parameter;
}

int GameScript::PartyCountAliveEQ(Scriptable */*Sender*/, const Trigger *parameters)
{
	return core->GetGame()->GetPartySize(true) == parameters->int0Parameter;
}

int GameScript::PartyCountAliveLT(Scriptable */*Sender*/, const Trigger *parameters)
{
	return core->GetGame()->GetPartySize(true) < parameters->int0Parameter;
}

int GameScript::PartyCountAliveGT(Scriptable */*Sender*/, const Trigger *parameters)
{
	return core->GetGame()->GetPartySize(true) > parameters->int0Parameter;
}

int GameScript::LevelParty(Scriptable */*Sender*/, const Trigger *parameters)
{
	int count = core->GetGame()->GetPartySize(true);

	if (count) {
		return core->GetGame()->GetTotalPartyLevel(true) / count == parameters->int0Parameter;
	}
	return 0;
}

int GameScript::LevelPartyLT(Scriptable */*Sender*/, const Trigger *parameters)
{
	int count = core->GetGame()->GetPartySize(true);

	if (count) {
		return core->GetGame()->GetTotalPartyLevel(true) / count < parameters->int0Parameter;
	}
	return 0;
}

int GameScript::LevelPartyGT(Scriptable */*Sender*/, const Trigger *parameters)
{
	int count = core->GetGame()->GetPartySize(true);

	if (count) {
		return core->GetGame()->GetTotalPartyLevel(true) / count > parameters->int0Parameter;
	}
	return 0;
}

int GameScript::PartyGold(Scriptable */*Sender*/, const Trigger *parameters)
{
	return core->GetGame()->PartyGold == (ieDword) parameters->int0Parameter;
}

int GameScript::PartyGoldGT(Scriptable */*Sender*/, const Trigger *parameters)
{
	return core->GetGame()->PartyGold > (ieDword) parameters->int0Parameter;
}

int GameScript::PartyGoldLT(Scriptable */*Sender*/, const Trigger *parameters)
{
	return core->GetGame()->PartyGold < (ieDword) parameters->int0Parameter;
}

int GameScript::OwnsFloaterMessage(Scriptable *Sender, const Trigger *parameters)
{
	const Scriptable *tar = GetActorFromObject(Sender, parameters->objectParameter);
	if (!tar) {
		return 0;
	}
	return tar->OverheadTextIsDisplaying();
}

int GameScript::InCutSceneMode(Scriptable */*Sender*/, const Trigger */*parameters*/)
{
	return core->InCutSceneMode();
}

int GameScript::Proficiency(Scriptable *Sender, const Trigger *parameters)
{
	unsigned int idx = parameters->int0Parameter;
	if (idx>31) {
		return 0;
	}
	const Scriptable *tar = GetActorFromObject(Sender, parameters->objectParameter);
	if (!tar) {
		return 0;
	}
	if (tar->Type != ST_ACTOR) {
		return 0;
	}
	const Actor *actor = (const Actor *) tar;
	return (signed) actor->GetStat(IE_PROFICIENCYBASTARDSWORD+idx) == parameters->int1Parameter;
}

int GameScript::ProficiencyGT(Scriptable *Sender, const Trigger *parameters)
{
	unsigned int idx = parameters->int0Parameter;
	if (idx>31) {
		return 0;
	}
	const Scriptable *tar = GetActorFromObject(Sender, parameters->objectParameter);
	if (!tar) {
		return 0;
	}
	if (tar->Type != ST_ACTOR) {
		return 0;
	}
	const Actor *actor = (const Actor *) tar;
	return (signed) actor->GetStat(IE_PROFICIENCYBASTARDSWORD+idx) > parameters->int1Parameter;
}

int GameScript::ProficiencyLT(Scriptable *Sender, const Trigger *parameters)
{
	unsigned int idx = parameters->int0Parameter;
	if (idx>31) {
		return 0;
	}
	const Scriptable *tar = GetActorFromObject(Sender, parameters->objectParameter);
	if (!tar) {
		return 0;
	}
	if (tar->Type != ST_ACTOR) {
		return 0;
	}
	const Actor *actor = (const Actor *) tar;
	return (signed) actor->GetStat(IE_PROFICIENCYBASTARDSWORD+idx) < parameters->int1Parameter;
}

//this is a PST specific stat, shows how many free proficiency slots we got
//we use an unused stat for it
int GameScript::ExtraProficiency(Scriptable *Sender, const Trigger *parameters)
{
	const Scriptable *tar = GetActorFromObject(Sender, parameters->objectParameter);
	if (!tar) {
		return 0;
	}
	if (tar->Type != ST_ACTOR) {
		return 0;
	}
	const Actor *actor = (const Actor *) tar;
	return (signed) actor->GetStat(IE_FREESLOTS) == parameters->int0Parameter;
}

int GameScript::ExtraProficiencyGT(Scriptable *Sender, const Trigger *parameters)
{
	const Scriptable *tar = GetActorFromObject(Sender, parameters->objectParameter);
	if (!tar) {
		return 0;
	}
	if (tar->Type != ST_ACTOR) {
		return 0;
	}
	const Actor *actor = (const Actor *) tar;
	return (signed) actor->GetStat(IE_FREESLOTS) > parameters->int0Parameter;
}

int GameScript::ExtraProficiencyLT(Scriptable *Sender, const Trigger *parameters)
{
	const Scriptable *tar = GetActorFromObject(Sender, parameters->objectParameter);
	if (!tar) {
		return 0;
	}
	if (tar->Type != ST_ACTOR) {
		return 0;
	}
	const Actor *actor = (const Actor *) tar;
	return (signed) actor->GetStat(IE_FREESLOTS) < parameters->int0Parameter;
}

int GameScript::Internal(Scriptable *Sender, const Trigger *parameters)
{
	unsigned int idx = parameters->int0Parameter;
	if (idx>15) {
		return 0;
	}
	const Scriptable *tar = GetActorFromObject(Sender, parameters->objectParameter);
	if (!tar) {
		return 0;
	}
	if (tar->Type != ST_ACTOR) {
		return 0;
	}
	const Actor *actor = (const Actor *) tar;
	return (signed) actor->GetStat(IE_INTERNAL_0+idx) == parameters->int1Parameter;
}

int GameScript::InternalGT(Scriptable *Sender, const Trigger *parameters)
{
	unsigned int idx = parameters->int0Parameter;
	if (idx>15) {
		return 0;
	}
	const Scriptable *tar = GetActorFromObject(Sender, parameters->objectParameter);
	if (!tar) {
		return 0;
	}
	if (tar->Type != ST_ACTOR) {
		return 0;
	}
	const Actor *actor = (const Actor *) tar;
	return (signed) actor->GetStat(IE_INTERNAL_0+idx) > parameters->int1Parameter;
}

int GameScript::InternalLT(Scriptable *Sender, const Trigger *parameters)
{
	unsigned int idx = parameters->int0Parameter;
	if (idx>15) {
		return 0;
	}
	const Scriptable *tar = GetActorFromObject(Sender, parameters->objectParameter);
	if (!tar) {
		return 0;
	}
	if (tar->Type != ST_ACTOR) {
		return 0;
	}
	const Actor *actor = (const Actor *) tar;
	return (signed) actor->GetStat(IE_INTERNAL_0+idx) < parameters->int1Parameter;
}

//we check if target is currently in dialog or not
int GameScript::NullDialog(Scriptable *Sender, const Trigger *parameters)
{
	const Scriptable *tar = GetActorFromObject(Sender, parameters->objectParameter);
	if (!tar) {
		return 0;
	}
	if (tar->Type != ST_ACTOR) {
		return 0;
	}
	const GameControl *gc = core->GetGameControl();
	if (!gc->dialoghandler->InDialog(tar)) {
		return 1;
	}
	return 0;
}

//this one checks scriptname (deathvar), i hope it is right
//IsScriptName depends on this too
//Name is another (similar function)
int GameScript::CalledByName(Scriptable *Sender, const Trigger *parameters)
{
	const Scriptable *tar = GetActorFromObject(Sender, parameters->objectParameter);
	if (!tar) {
		return 0;
	}
	if (tar->Type != ST_ACTOR) {
		return 0;
	}
	const Actor *actor = (const Actor *) tar;
	if (stricmp(actor->GetScriptName(), parameters->string0Parameter) ) {
		return 0;
	}
	return 1;
}

//This is checking on the character's name as it was typed in
int GameScript::CharName(Scriptable *Sender, const Trigger *parameters)
{
	const Scriptable *scr = GetActorFromObject(Sender, parameters->objectParameter);
	if (!scr || scr->Type!=ST_ACTOR) {
		return 0;
	}
	const Actor *actor = (const Actor *) scr;
	if (!strnicmp(actor->ShortName, parameters->string0Parameter, 32) ) {
		return 1;
	}
	return 0;
}

int GameScript::AnimationID(Scriptable *Sender, const Trigger *parameters)
{
	const Scriptable *tar = GetActorFromObject(Sender, parameters->objectParameter);
	if (!tar) {
		return 0;
	}
	if (tar->Type != ST_ACTOR) {
		return 0;
	}
	const Actor *actor = (const Actor *) tar;
	if ((ieWord) actor->GetStat(IE_ANIMATION_ID) == (ieWord) parameters->int0Parameter) {
		return 1;
	}
	return 0;
}

int GameScript::AnimState(Scriptable *Sender, const Trigger *parameters)
{
	const Scriptable *tar = GetActorFromObject(Sender, parameters->objectParameter);
	if (!tar) {
		return 0;
	}
	if (tar->Type != ST_ACTOR) {
		return 0;
	}
	const Actor *actor = (const Actor *) tar;
	return actor->GetStance() == parameters->int0Parameter;
}

//this trigger uses hours
int GameScript::Time(Scriptable */*Sender*/, const Trigger *parameters)
{
	int hour = parameters->int0Parameter;
	if (hour < 0 || hour > 23) return 0;

	if (!hour) hour = 24;
	return Schedule(1 << (hour - 1), core->GetGame()->GameTime);
}

//this trigger uses hours
int GameScript::TimeGT(Scriptable */*Sender*/, const Trigger *parameters)
{
	if (parameters->int0Parameter < 0 || parameters->int0Parameter > 22) return 0;

	return Schedule((0xFFFFFFu << parameters->int0Parameter) & 0x7FFFFFu, core->GetGame()->GameTime);
}

//this trigger uses hours
int GameScript::TimeLT(Scriptable */*Sender*/, const Trigger *parameters)
{
	if (parameters->int0Parameter < 1 || parameters->int0Parameter > 23) return 0;

	return Schedule((0xFFFFFFu >> (25 - parameters->int0Parameter)) | 1 << 23, core->GetGame()->GameTime);
}

int GameScript::HotKey(Scriptable *Sender, const Trigger *parameters)
{
	return Sender->MatchTrigger(trigger_hotkey, parameters->int0Parameter);
}

int GameScript::CombatCounter(Scriptable */*Sender*/, const Trigger *parameters)
{
	return core->GetGame()->CombatCounter == (ieDword) parameters->int0Parameter;
}

int GameScript::CombatCounterGT(Scriptable */*Sender*/, const Trigger *parameters)
{
	return core->GetGame()->CombatCounter > (ieDword) parameters->int0Parameter;
}

int GameScript::CombatCounterLT(Scriptable */*Sender*/, const Trigger *parameters)
{
	return core->GetGame()->CombatCounter < (ieDword) parameters->int0Parameter;
}

int GameScript::TrapTriggered(Scriptable *Sender, const Trigger *parameters)
{
	return Sender->MatchTriggerWithObject(trigger_traptriggered, parameters->objectParameter);
}

int GameScript::InteractingWith(Scriptable *Sender, const Trigger *parameters)
{
	if (Sender->Type!=ST_ACTOR) {
		return 0;
	}
	const Scriptable *tar = GetActorFromObject(Sender, parameters->objectParameter);
	if (!tar || tar->Type != ST_ACTOR) {
		return 0;
	}
	const GameControl *gc = core->GetGameControl();
	if (!gc->dialoghandler->InDialog(Sender)) {
		return 0;
	}
	if (!gc->dialoghandler->InDialog(tar)) {
		return 0;
	}
	return 1;
}

int GameScript::LastPersonTalkedTo(Scriptable *Sender, const Trigger *parameters)
{
	if (Sender->Type!=ST_ACTOR) {
		return 0;
	}
	const Scriptable *tar = GetActorFromObject(Sender, parameters->objectParameter);
	if (!tar || tar->Type != ST_ACTOR) {
		return 0;
	}
	const Actor *scr = (const Actor *) Sender;
	if (MatchActor(Sender, scr->LastTalker, parameters->objectParameter)) {
		return 1;
	}
	return 0;
}

int GameScript::IsRotation(Scriptable *Sender, const Trigger *parameters)
{
	const Scriptable *tar = GetActorFromObject(Sender, parameters->objectParameter);
	if (!tar || tar->Type != ST_ACTOR) {
		return 0;
	}
	const Actor *actor = (const Actor *) tar;
	if ( actor->GetOrientation() == parameters->int0Parameter ) {
		return 1;
	}
	return 0;
}

//GemRB currently stores the saved location in a local variable, but it is
//actually stored in the .gam structure (only for PCs)
int GameScript::IsFacingSavedRotation(Scriptable *Sender, const Trigger *parameters)
{
	const Scriptable *tar = GetActorFromObject(Sender, parameters->objectParameter);
	if (!tar || tar->Type!=ST_ACTOR) {
		return 0;
	}
	const Actor *actor = (const Actor *) tar;
	if (actor->GetOrientation() == actor->GetStat(IE_SAVEDFACE) ) {
		return 1;
	}
	return 0;
}

int GameScript::IsFacingObject(Scriptable *Sender, const Trigger *parameters)
{
	if (Sender->Type != ST_ACTOR) {
		return 0;
	}
	const Scriptable *target = GetActorFromObject(Sender, parameters->objectParameter);
	if (!target) {
		return 0;
	}
	const Actor *actor = (const Actor *) Sender;
	if (actor->GetOrientation() == GetOrient(target->Pos, actor->Pos)) {
		return 1;
	}
	return 0;
}

int GameScript::AttackedBy(Scriptable *Sender, const Trigger *parameters)
{
	bool match = Sender->MatchTriggerWithObject(trigger_attackedby, parameters->objectParameter, parameters->int0Parameter);
	const Scriptable *target = GetActorFromObject(Sender, parameters->objectParameter);
	if (match && target && Sender->Type == ST_ACTOR) {
		Sender->LastMarked = target->GetGlobalID();
	}
	return match;
}

int GameScript::TookDamage(Scriptable *Sender, const Trigger */*parameters*/)
{
	return Sender->MatchTrigger(trigger_tookdamage);
}

int GameScript::HitBy(Scriptable *Sender, const Trigger *parameters)
{
	return Sender->MatchTriggerWithObject(trigger_hitby, parameters->objectParameter, parameters->int0Parameter);
}

int GameScript::Heard(Scriptable *Sender, const Trigger *parameters)
{
	return Sender->MatchTriggerWithObject(trigger_heard, parameters->objectParameter, parameters->int0Parameter);
}

int GameScript::Detected(Scriptable *Sender, const Trigger *parameters)
{
	return Sender->MatchTriggerWithObject(trigger_detected, parameters->objectParameter, parameters->int0Parameter);
}

int GameScript::LastMarkedObject_Trigger(Scriptable *Sender, const Trigger *parameters)
{
	if (Sender->Type!=ST_ACTOR) {
		return 0;
	}
	const Actor *actor = (const Actor *) Sender;
	if (MatchActor(Sender, actor->LastMarked, parameters->objectParameter)) {
		//don't mark this object for clear
		//Sender->AddTrigger(&actor->LastSeen);
		return 1;
	}
	return 0;
}

int GameScript::HelpEX(Scriptable *Sender, const Trigger *parameters)
{
	if (Sender->Type!=ST_ACTOR) {
		return 0;
	}
	const Scriptable *tar = GetActorFromObject(Sender, parameters->objectParameter);
	if (!tar || tar->Type!=ST_ACTOR) {
		//a non actor checking for help?
		return 0;
	}
	const Actor *actor = (const Actor *) tar;
	const Actor *help = Sender->GetCurrentArea()->GetActorByGlobalID(actor->LastHelp);
	if (!help) {
		//no help required
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
	bool match = false;
	if (stat == IE_CLASS) {
		match = actor->GetActiveClass() == help->GetActiveClass();
	} else if (actor->GetStat(stat) == help->GetStat(stat)) {
		// FIXME
		//Sender->AddTrigger(&actor->LastHelp);
		match = true;
	}
	if (match && Sender->Type == ST_ACTOR) {
		Sender->LastMarked = actor->GetGlobalID();
	}
	return match;
}

int GameScript::Help_Trigger(Scriptable *Sender, const Trigger *parameters)
{
	 bool match = Sender->MatchTriggerWithObject(trigger_help, parameters->objectParameter);
	 const Scriptable *target = GetActorFromObject(Sender, parameters->objectParameter);
	 if (match && target && Sender->Type == ST_ACTOR) {
		 Sender->LastMarked = target->GetGlobalID();
	 }
	 return match;
}

// a few values are named in order.ids
int GameScript::ReceivedOrder(Scriptable *Sender, const Trigger *parameters)
{
	return Sender->MatchTriggerWithObject(trigger_receivedorder, parameters->objectParameter, parameters->int0Parameter);
}

int GameScript::Joins(Scriptable *Sender, const Trigger *parameters)
{
	return Sender->MatchTriggerWithObject(trigger_joins, parameters->objectParameter);
}

int GameScript::Leaves(Scriptable *Sender, const Trigger *parameters)
{
	return Sender->MatchTriggerWithObject(trigger_leaves, parameters->objectParameter);
}

int GameScript::FallenPaladin(Scriptable *Sender, const Trigger */*parameters*/)
{
	if (Sender->Type!=ST_ACTOR) {
		return 0;
	}
	const Actor *act = (const Actor *) Sender;
	return (act->GetStat(IE_MC_FLAGS) & MC_FALLEN_PALADIN) != 0;
}

int GameScript::FallenRanger(Scriptable *Sender, const Trigger */*parameters*/)
{
	if (Sender->Type!=ST_ACTOR) {
		return 0;
	}
	const Actor *act = (const Actor *) Sender;
	return (act->GetStat(IE_MC_FLAGS) & MC_FALLEN_RANGER) != 0;
}

int GameScript::NightmareModeOn(Scriptable */*Sender*/, const Trigger */*parameters*/)
{
	ieDword diff;

	core->GetDictionary()->Lookup("Nightmare Mode", diff);
	if (diff) {
		return 1;
	}
	return 0;
}

int GameScript::StoryModeOn(Scriptable */*Sender*/, const Trigger */*parameters*/)
{
	ieDword mode;

	core->GetDictionary()->Lookup("Story Mode", mode);
	if (mode) {
		return 1;
	}
	return 0;
}

// the original was more complicated, but we simplify by doing more work in AREImporter
int GameScript::CheckAreaDiffLevel(Scriptable */*Sender*/, const Trigger *parameters)
{
	const Map *map = core->GetGame()->GetCurrentArea();
	if (!map) return 0;
	return map->AreaDifficulty == 1 << (parameters->int0Parameter - 1);
}

int GameScript::Difficulty(Scriptable */*Sender*/, const Trigger *parameters)
{
	ieDword diff;

	core->GetDictionary()->Lookup("Difficulty Level", diff);
	int mode = parameters->int1Parameter;
	//hack for compatibility
	if (!mode) {
		mode = EQUALS;
	}
	return DiffCore(diff+1, (ieDword) parameters->int0Parameter, mode);
}

int GameScript::DifficultyGT(Scriptable */*Sender*/, const Trigger *parameters)
{
	ieDword diff;

	core->GetDictionary()->Lookup("Difficulty Level", diff);
	return diff+1>(ieDword) parameters->int0Parameter;
}

int GameScript::DifficultyLT(Scriptable */*Sender*/, const Trigger *parameters)
{
	ieDword diff;

	core->GetDictionary()->Lookup("Difficulty Level", diff);
	return diff+1<(ieDword) parameters->int0Parameter;
}

int GameScript::Vacant(Scriptable *Sender, const Trigger */*parameters*/)
{
	if (Sender->Type!=ST_AREA) {
		return 0;
	}
	const Map *map = (Map *) Sender;
	// map->CanFree() has side effects, don't use it here! Would make some loot and corpses disappear immediately
	int i = map->GetActorCount(true);
	while (i--) {
		const Actor *actor = map->GetActor(i, true);
		bool usedExit = actor->GetInternalFlag() & IF_USEEXIT;
		if (actor->IsPartyMember()) {
			if (!usedExit) {
				return 0;
			}
		} else if (usedExit) {
			return 0;
		}
	}
	return 1;
}

//this trigger always checks the right hand weapon?
int GameScript::InWeaponRange(Scriptable *Sender, const Trigger *parameters)
{
	if (Sender->Type!=ST_ACTOR) {
		return 0;
	}
	const Scriptable *tar = GetActorFromObject(Sender, parameters->objectParameter);
	if (!tar) {
		return 0;
	}
	const Actor *actor = (const Actor *) Sender;
	WeaponInfo wi;
	unsigned int wrange = 0;
	const ITMExtHeader *header = actor->GetWeapon(wi, false);
	if (header) {
		wrange = actor->GetWeaponRange(wi);
	}
	// checking also the left hand, in case they're dualwielding
	header = actor->GetWeapon(wi, true);
	if (header && (wi.range>wrange)) {
		wrange = actor->GetWeaponRange(wi);
	}
	if (WithinPersonalRange(actor, tar, wrange)) {
		return 1;
	}
	return 0;
}

//it is impossible to equip a bow without projectile (it will be fist)
//So outofammo equals fist is equipped
int GameScript::OutOfAmmo(Scriptable *Sender, const Trigger *parameters)
{
	const Scriptable *scr = Sender;
	if (parameters->objectParameter) {
		scr = GetActorFromObject( Sender, parameters->objectParameter );
	}
	if (!scr || scr->Type != ST_ACTOR) {
		return 0;
	}
	const Actor *actor = (const Actor *) scr;

	//if a bow is equipped, but out of ammo, the core system will swap to fist anyway
	if (actor->inventory.GetEquippedSlot() == actor->inventory.GetFistSlot()) {
		return 1;
	}

	return 0;
}

//returns true if a weapon is equipped and target is in range
//if a bow is equipped without projectile, it is useless (but it will be a fist)!
int GameScript::HaveUsableWeaponEquipped(Scriptable *Sender, const Trigger */*parameters*/)
{
	if (Sender->Type!=ST_ACTOR) {
		return 0;
	}
	const Actor *actor = (const Actor *) Sender;
	//if a bow is equipped, but out of ammo, the core system will swap to fist anyway
	if (actor->inventory.GetEquippedSlot() == actor->inventory.GetFistSlot()) {
		return 0;
	}

	return 1;
}

//if the equipped slot is not a fist, this is true
int GameScript::HasWeaponEquipped(Scriptable *Sender, const Trigger *parameters)
{
	const Scriptable *tar = GetActorFromObject(Sender, parameters->objectParameter);
	if (!tar || tar->Type!=ST_ACTOR) {
		return 0;
	}
	const Actor *actor = (const Actor *) tar;
	if (actor->inventory.GetEquippedSlot() == actor->inventory.GetFistSlot()) {
		return 0;
	}
	return 1;
}

int GameScript::PCInStore( Scriptable */*Sender*/, const Trigger */*parameters*/)
{
	if (core->GetCurrentStore()) {
		return 1;
	}
	return 0;
}

//this checks if the launch point is onscreen, a more elaborate check
//would see if any piece of the Scriptable is onscreen, what is the original
//behaviour?
int GameScript::OnScreen( Scriptable *Sender, const Trigger */*parameters*/)
{
	Region vp = core->GetVideoDriver()->GetViewport();
	if (vp.PointInside(Sender->Pos) ) {
		return 1;
	}
	return 0;
}

int GameScript::IsPlayerNumber( Scriptable *Sender, const Trigger *parameters)
{
	const Scriptable *tar = GetActorFromObject(Sender, parameters->objectParameter);
	if (!tar || tar->Type!=ST_ACTOR) {
		return 0;
	}
	const Actor *actor = (const Actor *) tar;
	if (actor->InParty == parameters->int0Parameter) {
		return 1;
	}
	return 0;
}

int GameScript::PCCanSeePoint( Scriptable */*Sender*/, const Trigger *parameters)
{
	const Map *map = core->GetGame()->GetCurrentArea();
	if (map->IsVisible(parameters->pointParameter, false) ) {
		return 1;
	}
	return 0;
}

// I'm clueless about this trigger ... but it looks fine, pst dgaoha.d is the only user
int GameScript::StuffGlobalRandom( Scriptable *Sender, const Trigger *parameters)
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

int GameScript::IsCreatureAreaFlag( Scriptable *Sender, const Trigger *parameters)
{
	const Scriptable *tar = GetActorFromObject(Sender, parameters->objectParameter);
	if (!tar || tar->Type!=ST_ACTOR) {
		return 0;
	}
	const Actor *actor = (const Actor *) tar;
	if (actor->GetStat(IE_MC_FLAGS) & parameters->int0Parameter) {
		return 1;
	}
	return 0;
}

int GameScript::IsPathCriticalObject( Scriptable *Sender, const Trigger *parameters)
{
	const Scriptable *tar = GetActorFromObject(Sender, parameters->objectParameter);
	if (!tar || tar->Type!=ST_ACTOR) {
		return 0;
	}
	const Actor *actor = (const Actor *) tar;
	if (actor->GetStat(IE_MC_FLAGS) & MC_PLOT_CRITICAL) {
		return 1;
	}
	return 0;
}

// 0 - ability, 1 - number, 2 - mode
int GameScript::ChargeCount( Scriptable *Sender, const Trigger *parameters)
{
	const Scriptable *tar = GetActorFromObject(Sender, parameters->objectParameter);
	if (!tar || tar->Type!=ST_ACTOR) {
		return 0;
	}
	const Actor *actor = (const Actor *) tar;
	int Slot = actor->inventory.FindItem(parameters->string0Parameter,0);
	if (Slot<0) {
		return 0;
	}
	const CREItem *item = actor->inventory.GetSlotItem (Slot);
	if (!item) {//bah
		return 0;
	}
	if (parameters->int0Parameter>2) {
		return 0;
	}
	int charge = item->Usages[parameters->int0Parameter];
	switch (parameters->int2Parameter) {
		case EQUALS:
			if (charge == parameters->int1Parameter)
				return 1;
			break;
		case LESS_THAN:
			if (charge < parameters->int1Parameter)
				return 1;
			break;
		case GREATER_THAN:
			if (charge > parameters->int1Parameter)
				return 1;
			break;
		default:
			return 0;
	}
	return 0;
}

// no idea if it checks only alive partymembers or if it is average or not
int GameScript::CheckPartyLevel( Scriptable */*Sender*/, const Trigger *parameters)
{
	if (core->GetGame()->GetTotalPartyLevel(false) < parameters->int0Parameter) {
		return 0;
	}
	return 1;
}

// no idea if it checks only alive partymembers
int GameScript::CheckPartyAverageLevel( Scriptable */*Sender*/, const Trigger *parameters)
{
	const Game *game = core->GetGame();

	int count = game->GetPartySize(false);
	int level = game->GetTotalPartyLevel(false);

	if (count) level/=count;

	switch (parameters->int1Parameter) {
		case EQUALS:
			if (level ==parameters->int0Parameter) {
				return 1;
			}
			break;
		case LESS_THAN:
			if (level < parameters->int0Parameter) {
				return 1;
			}
			break;
		case GREATER_THAN:
			if (level > parameters->int0Parameter) {
				return 1;
			}
			break;
		default:
			return 0;
	}
	return 1;
}

int GameScript::CheckDoorFlags( Scriptable *Sender, const Trigger *parameters)
{
	const Scriptable *tar = GetActorFromObject(Sender, parameters->objectParameter);
	if (!tar || tar->Type!=ST_DOOR) {
		return 0;
	}
	const Door *door = (const Door *) tar;
	if (door->Flags & parameters->int0Parameter) {
		return 1;
	}
	return 0;
}

// works only on animations?
// Be careful when converting to GetActorFromObject, it won't return animations (those are not scriptable)
int GameScript::Frame( Scriptable *Sender, const Trigger *parameters)
{
	//to avoid a crash
	if (!parameters->objectParameter) {
		return 0;
	}
	const AreaAnimation *anim = Sender->GetCurrentArea()->GetAnimation(parameters->objectParameter->objectName);
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
int GameScript::ModalState( Scriptable *Sender, const Trigger *parameters)
{
	const Scriptable *scr;

	if (parameters->objectParameter) {
		scr = GetActorFromObject( Sender, parameters->objectParameter );
	} else {
		scr = Sender;
	}
	if (!scr || scr->Type!=ST_ACTOR) {
		return 0;
	}
	const Actor *actor = (const Actor *) scr;

	if (actor->Modal.State == (ieDword) parameters->int0Parameter) {
		return 1;
	}
	return 0;
}

/* a special redundant trigger for iwd2 - could do something extra */
int GameScript::IsCreatureHiddenInShadows( Scriptable *Sender, const Trigger */*parameters*/)
{
	if (Sender->Type!=ST_ACTOR) {
		return 0;
	}
	const Actor *actor = (const Actor *) Sender;

	if (actor->Modal.State == MS_STEALTH) {
		return 1;
	}
	return 0;
}

int GameScript::IsWeather( Scriptable */*Sender*/, const Trigger *parameters)
{
	const Game *game = core->GetGame();
	ieDword weather = game->WeatherBits & parameters->int0Parameter;
	if (weather == (ieDword) parameters->int1Parameter) {
		return 1;
	}
	return 0;
}

int GameScript::Delay( Scriptable *Sender, const Trigger *parameters)
{
	ieDword delay = (ieDword) parameters->int0Parameter;
	if (delay<=1) {
		return 1;
	}

	return (Sender->ScriptTicks % delay) <= Sender->IdleTicks;
}

#define TIMEOFDAY_DAY		0	/* 7-21 */
#define TIMEOFDAY_DUSK		1	/* 21-22 */
#define TIMEOFDAY_NIGHT		2	/* 22-6 */
#define TIMEOFDAY_MORNING	3	/* 6-7 */

int GameScript::TimeOfDay(Scriptable */*Sender*/, const Trigger *parameters)
{
	int hour = core->Time.GetHour(core->GetGame()->GameTime);

	if ((parameters->int0Parameter == TIMEOFDAY_DAY && hour >= 7 && hour < 21)
		|| (parameters->int0Parameter == TIMEOFDAY_DUSK && hour == 21)
		|| (parameters->int0Parameter == TIMEOFDAY_NIGHT && (hour >= 22 || hour < 6))
		|| (parameters->int0Parameter == TIMEOFDAY_MORNING && hour == 6)) {
		return 1;
	}
	return 0;
}

//this is a PST action, it's using delta.ids, not diffmode.ids
int GameScript::RandomStatCheck(Scriptable *Sender, const Trigger *parameters)
{
	const Scriptable *tar = GetActorFromObject(Sender, parameters->objectParameter);
	if (!tar || tar->Type!=ST_ACTOR) {
		return 0;
	}
	const Actor *actor = (const Actor *) tar;

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
		default:
			Log(ERROR, "GameScript", "RandomStatCheck: unknown int parameter 1 passed: %d, ignoring!", parameters->int1Parameter);
			break;
	}
	return 0;
}

int GameScript::PartyRested(Scriptable *Sender, const Trigger */*parameters*/)
{
	return Sender->MatchTrigger(trigger_partyrested);
}

int GameScript::IsWeaponRanged(Scriptable *Sender, const Trigger *parameters)
{
	const Scriptable *tar = GetActorFromObject(Sender, parameters->objectParameter);
	if (!tar || tar->Type!=ST_ACTOR) {
		return 0;
	}
	const Actor *actor = (const Actor *) tar;
	if (actor->inventory.GetEquipped()<0) {
		return 1;
	}
	return 0;
}

//HoW applies sequence on area animations
int GameScript::Sequence(Scriptable *Sender, const Trigger *parameters)
{
	//to avoid a crash, check if object is NULL
	if (parameters->objectParameter) {
		const AreaAnimation *anim = Sender->GetCurrentArea()->GetAnimation(parameters->objectParameter->objectName);
		if (anim) {
			//this is the cycle count for the area animation
			//very much like stance for avatar anims
			if (anim->sequence==parameters->int0Parameter) {
				return 1;
			}
			return 0;
		}
	}

	const Scriptable *tar = GetActorFromObject(Sender, parameters->objectParameter);
	if (!tar || tar->Type != ST_ACTOR) {
		return 0;
	}
	const Actor *actor = (const Actor *) tar;
	if (actor->GetStance()==parameters->int0Parameter) {
		return 1;
	}
	return 0;
}

int GameScript::TimerExpired(Scriptable *Sender, const Trigger *parameters)
{
	if (Sender->TimerExpired(parameters->int0Parameter) ) {
		return 1;
	}
	return 0;
}

int GameScript::TimerActive(Scriptable *Sender, const Trigger *parameters)
{
	if (Sender->TimerActive(parameters->int0Parameter) ) {
		return 1;
	}
	return 0;
}

int GameScript::ActuallyInCombat(Scriptable */*Sender*/, const Trigger */*parameters*/)
{
	const Game *game = core->GetGame();
	if (game->AnyPCInCombat()) return 1;
	return 0;
}

int GameScript::InMyGroup(Scriptable *Sender, const Trigger *parameters)
{
	if (Sender->Type!=ST_ACTOR) {
		return 0;
	}

	const Scriptable *tar = GetActorFromObject(Sender, parameters->objectParameter);
	if (!tar || tar->Type!=ST_ACTOR) {
		return 0;
	}

	if (((const Actor *) tar)->GetStat(IE_SPECIFIC) == ((const Actor *) Sender)->GetStat(IE_SPECIFIC)) {
		return 1;
	}
	return 0;
}

int GameScript::AnyPCSeesEnemy(Scriptable */*Sender*/, const Trigger */*parameters*/)
{
	const Game *game = core->GetGame();
	unsigned int i = (unsigned int) game->GetLoadedMapCount();
	while(i--) {
		const Map *map = game->GetMap(i);
		if (map->AnyPCSeesEnemy()) {
			return 1;
		}
	}
	return 0;
}

int GameScript::Unusable(Scriptable *Sender, const Trigger *parameters)
{
	if (Sender->Type!=ST_ACTOR) {
		return 0;
	}
	const Actor *actor = (const Actor *) Sender;

	const Item *item = gamedata->GetItem(parameters->string0Parameter);
	if (!item) {
		return 0;
	}
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
int GameScript::IsInGuardianMantle(Scriptable *Sender, const Trigger *parameters)
{
	const Scriptable *tar = GetActorFromObject(Sender, parameters->objectParameter);
	if (!tar || tar->Type != ST_ACTOR) {
		return 0;
	}
	const Actor *actor = (const Actor *) tar;
	if (actor->GetStat(IE_IMMUNITY)&IMM_GUARDIAN) {
		return 1;
	}
	return 0;
}

int GameScript::HasBounceEffects(Scriptable *Sender, const Trigger *parameters)
{
	const Scriptable *tar = GetActorFromObject(Sender, parameters->objectParameter);
	if (!tar || tar->Type != ST_ACTOR) {
		return 0;
	}
	const Actor *actor = (const Actor *) tar;
	if (actor->GetStat(IE_BOUNCE)) return 1;
	return 0;
}

int GameScript::HasImmunityEffects(Scriptable *Sender, const Trigger *parameters)
{
	const Scriptable *tar = GetActorFromObject(Sender, parameters->objectParameter);
	if (!tar || tar->Type != ST_ACTOR) {
		return 0;
	}
	const Actor *actor = (const Actor *) tar;
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

int GameScript::SystemVariable_Trigger(Scriptable *Sender, const Trigger *parameters)
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

int GameScript::SpellCast(Scriptable *Sender, const Trigger *parameters)
{
	return Sender->MatchTriggerWithObject(trigger_spellcast, parameters->objectParameter, parameters->int0Parameter);
}

int GameScript::SpellCastPriest(Scriptable *Sender, const Trigger *parameters)
{
	return Sender->MatchTriggerWithObject(trigger_spellcastpriest, parameters->objectParameter, parameters->int0Parameter);
}

int GameScript::SpellCastInnate(Scriptable *Sender, const Trigger *parameters)
{
	return Sender->MatchTriggerWithObject(trigger_spellcastinnate, parameters->objectParameter, parameters->int0Parameter);
}

int GameScript::SpellCastOnMe(Scriptable *Sender, const Trigger *parameters)
{
	return Sender->MatchTriggerWithObject(trigger_spellcastonme, parameters->objectParameter, parameters->int0Parameter);
}

int GameScript::CalendarDay(Scriptable */*Sender*/, const Trigger *parameters)
{
	int day = core->GetCalendar()->GetCalendarDay(core->GetGame()->GameTime/core->Time.day_size);
	if(day == parameters->int0Parameter) {
		return 1;
	}
	return 0;
}

int GameScript::CalendarDayGT(Scriptable */*Sender*/, const Trigger *parameters)
{
	int day = core->GetCalendar()->GetCalendarDay(core->GetGame()->GameTime/core->Time.day_size);
	if(day > parameters->int0Parameter) {
		return 1;
	}
	return 0;
}

int GameScript::CalendarDayLT(Scriptable */*Sender*/, const Trigger *parameters)
{
	int day = core->GetCalendar()->GetCalendarDay(core->GetGame()->GameTime/core->Time.day_size);
	if(day < parameters->int0Parameter) {
		return 1;
	}
	return 0;
}

//NT Returns true only if the active CRE was turned by the specified priest or paladin.
int GameScript::TurnedBy(Scriptable *Sender, const Trigger *parameters)
{
	return Sender->MatchTriggerWithObject(trigger_turnedby, parameters->objectParameter);
}

//This is used for pst portals
//usage: UsedExit(Protagonist, "sigil")
//where sigil.2da contains all the exits that should trigger the teleport
int GameScript::UsedExit(Scriptable *Sender, const Trigger *parameters)
{
	const Scriptable *scr = GetActorFromObject(Sender, parameters->objectParameter);
	if (!scr || scr->Type != ST_ACTOR) {
		return 0;
	}
	const Actor *actor = (const Actor *) scr;

	if (actor->GetInternalFlag()&IF_USEEXIT) {
		return 0;
	}

	if (!actor->LastArea[0]) {
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
		if (strnicmp(actor->UsedExit, exit, 32) ) {
			continue;
		}
		return 1;
	}
	return 0;
}

int GameScript::IsTouchGUI(Scriptable */*Sender*/, const Trigger */*parameters*/)
{
	return core->GetVideoDriver()->TouchInputEnabled();
}

// always evaluates to true on Windows/OS X/Linux (there's no DLC); on other platforms it depends
//TODO: add the real check
int GameScript::HasDLC(Scriptable */*Sender*/, const Trigger */*parameters*/)
{
	return 1;
}

int GameScript::BeenInParty(Scriptable *Sender, const Trigger */*parameters*/)
{
	if (Sender->Type != ST_ACTOR) {
		return 0;
	}
	const Actor *actor = (const Actor *) Sender;
	return actor->GetStat(IE_MC_FLAGS) & MC_BEENINPARTY;
}

/*
 * TobEx triggers
 */

/* Compares the animation movement rate of the target creature specified by Object to Value.
 * This is not affected by slow or haste, but is affected if the Object is entangled, webbed, etc.
 */
int GameScript::MovementRate(Scriptable *Sender, const Trigger *parameters)
{
	const Scriptable *tar = GetActorFromObject(Sender, parameters->objectParameter);
	if (!tar || tar->Type != ST_ACTOR) {
		return 0;
	}
	const Actor *actor = (const Actor *) tar;

	int rate = actor->GetBase(IE_MOVEMENTRATE);
	if (actor->Immobile()) {
		rate = 0;
	}
	return rate == parameters->int0Parameter;
}

int GameScript::MovementRateGT(Scriptable *Sender, const Trigger *parameters)
{
	const Scriptable *tar = GetActorFromObject(Sender, parameters->objectParameter);
	if (!tar || tar->Type != ST_ACTOR) {
		return 0;
	}
	const Actor *actor = (const Actor *) tar;

	int rate = actor->GetBase(IE_MOVEMENTRATE);
	if (actor->Immobile()) {
		rate = 0;
	}
	return rate > parameters->int0Parameter;
}

int GameScript::MovementRateLT(Scriptable *Sender, const Trigger *parameters)
{
	const Scriptable *tar = GetActorFromObject(Sender, parameters->objectParameter);
	if (!tar || tar->Type != ST_ACTOR) {
		return 0;
	}
	const Actor *actor = (const Actor *) tar;

	int rate = actor->GetBase(IE_MOVEMENTRATE);
	if (actor->Immobile()) {
		rate = 0;
	}
	return rate < parameters->int0Parameter;
}

// Compares the number of mirror images present on the target creature specified by Object to Value.
int GameScript::NumMirrorImages(Scriptable *Sender, const Trigger *parameters)
{
	const Scriptable *tar = GetActorFromObject(Sender, parameters->objectParameter);
	if (!tar || tar->Type != ST_ACTOR) {
		return 0;
	}
	const Actor *actor = (const Actor *) tar;

	return (signed)actor->GetStat(IE_MIRRORIMAGES) == parameters->int0Parameter;
}

int GameScript::NumMirrorImagesGT(Scriptable *Sender, const Trigger *parameters)
{
	const Scriptable *tar = GetActorFromObject(Sender, parameters->objectParameter);
	if (!tar || tar->Type != ST_ACTOR) {
		return 0;
	}
	const Actor *actor = (const Actor *) tar;

	return (signed)actor->GetStat(IE_MIRRORIMAGES) > parameters->int0Parameter;
}

int GameScript::NumMirrorImagesLT(Scriptable *Sender, const Trigger *parameters)
{
	const Scriptable *tar = GetActorFromObject(Sender, parameters->objectParameter);
	if (!tar || tar->Type != ST_ACTOR) {
		return 0;
	}
	const Actor *actor = (const Actor *) tar;

	return (signed)actor->GetStat(IE_MIRRORIMAGES) < parameters->int0Parameter;
}

/* Returns true if the target creature specified by Object is bouncing spells of power Level.
 * This returns true for both Bounce Spell Level (199) and Decrementing Bounce Spells (200) effects.
 * (fx_bounce_spelllevel and fx_bounce_spelllevel_dec respectively)
 */
static EffectRef fx_level_bounce_ref = { "Bounce:SpellLevel", -1 };
static EffectRef fx_level_bounce_dec_ref = { "Bounce:SpellLevelDec", -1 };
int GameScript::BouncingSpellLevel(Scriptable *Sender, const Trigger *parameters)
{
	const Scriptable *tar = GetActorFromObject(Sender, parameters->objectParameter);
	if (!tar || tar->Type != ST_ACTOR) {
		return 0;
	}
	const Actor *actor = (const Actor *) tar;

	return actor->fxqueue.HasEffectWithPower(fx_level_bounce_ref, parameters->int0Parameter) ||
		actor->fxqueue.HasEffectWithPower(fx_level_bounce_dec_ref, parameters->int0Parameter);
}

/* Compares the number of spell bounces remaining on the target creature specified by Object
 * at the power Level to Amount. If Object has the Bounce Spell Level (199) opcode, then the
 * number of spell bounces is unsigned 0xFFFFFFFF.
 * NOTE: does not check for multiple bounce effects (if that's even possible)
 */
int GameScript::NumBouncingSpellLevel(Scriptable *Sender, const Trigger *parameters)
{
	const Scriptable *tar = GetActorFromObject(Sender, parameters->objectParameter);
	if (!tar || tar->Type != ST_ACTOR) {
		return 0;
	}
	const Actor *actor = (const Actor *) tar;

	unsigned int bounceCount = 0;
	if (actor->fxqueue.HasEffectWithPower(fx_level_bounce_ref, parameters->int0Parameter)) {
		bounceCount = 0xFFFFFFFF;
	} else {
		const Effect *fx = actor->fxqueue.HasEffectWithPower(fx_level_bounce_dec_ref, parameters->int0Parameter);
		if (fx) {
			bounceCount = fx->Parameter1;
		}
	}

	return bounceCount == (unsigned) parameters->int1Parameter;
}

int GameScript::NumBouncingSpellLevelGT(Scriptable *Sender, const Trigger *parameters)
{
	const Scriptable *tar = GetActorFromObject(Sender, parameters->objectParameter);
	if (!tar || tar->Type != ST_ACTOR) {
		return 0;
	}
	const Actor *actor = (const Actor *) tar;

	unsigned int bounceCount = 0;
	if (actor->fxqueue.HasEffectWithPower(fx_level_bounce_ref, parameters->int0Parameter)) {
		bounceCount = 0xFFFFFFFF;
	} else {
		const Effect *fx = actor->fxqueue.HasEffectWithPower(fx_level_bounce_dec_ref, parameters->int0Parameter);
		if (fx) {
			bounceCount = fx->Parameter1;
		}
	}

	return bounceCount > (unsigned) parameters->int1Parameter;
}

int GameScript::NumBouncingSpellLevelLT(Scriptable *Sender, const Trigger *parameters)
{
	const Scriptable *tar = GetActorFromObject(Sender, parameters->objectParameter);
	if (!tar || tar->Type != ST_ACTOR) {
		return 0;
	}
	const Actor *actor = (const Actor *) tar;

	unsigned int bounceCount = 0;
	if (actor->fxqueue.HasEffectWithPower(fx_level_bounce_ref, parameters->int0Parameter)) {
		bounceCount = 0xFFFFFFFF;
	} else {
		const Effect *fx = actor->fxqueue.HasEffectWithPower(fx_level_bounce_dec_ref, parameters->int0Parameter);
		if (fx) {
			bounceCount = fx->Parameter1;
		}
	}

	return bounceCount < (unsigned) parameters->int1Parameter;
}

/* Returns true if the target creature specified by Object is protected from spells of power Level.
 * This returns true for both Protection from Spell Levels (102) and Decrementing Spell Immunity (201) effects.
 */
static EffectRef fx_level_immunity_ref = { "Protection:Spelllevel", -1 };
static EffectRef fx_level_immunity_dec_ref = { "Protection:SpellLevelDec", -1 };
int GameScript::ImmuneToSpellLevel(Scriptable *Sender, const Trigger *parameters)
{
	const Scriptable *tar = GetActorFromObject(Sender, parameters->objectParameter);
	if (!tar || tar->Type != ST_ACTOR) {
		return 0;
	}
	const Actor *actor = (const Actor *) tar;

	return actor->fxqueue.HasEffectWithPower(fx_level_immunity_ref, parameters->int0Parameter) ||
	actor->fxqueue.HasEffectWithPower(fx_level_immunity_dec_ref, parameters->int0Parameter);
}

/* Compares the number of spell protections remaining on the target creature specified by Object
 * at the power Level to Amount. If Object has the Protection from Spell Levels (102) opcode,
 * then the number of spell protections is unsigned 0xFFFFFFFF.
 */
int GameScript::NumImmuneToSpellLevel(Scriptable *Sender, const Trigger *parameters)
{
	const Scriptable *tar = GetActorFromObject(Sender, parameters->objectParameter);
	if (!tar || tar->Type != ST_ACTOR) {
		return 0;
	}
	const Actor *actor = (const Actor *) tar;

	unsigned int bounceCount = 0;
	if (actor->fxqueue.HasEffectWithPower(fx_level_immunity_ref, parameters->int0Parameter)) {
		bounceCount = 0xFFFFFFFF;
	} else {
		const Effect *fx = actor->fxqueue.HasEffectWithPower(fx_level_immunity_dec_ref, parameters->int0Parameter);
		if (fx) {
			bounceCount = fx->Parameter1;
		}
	}

	return bounceCount == (unsigned) parameters->int1Parameter;
}

int GameScript::NumImmuneToSpellLevelGT(Scriptable *Sender, const Trigger *parameters)
{
	const Scriptable *tar = GetActorFromObject(Sender, parameters->objectParameter);
	if (!tar || tar->Type != ST_ACTOR) {
		return 0;
	}
	const Actor *actor = (const Actor *) tar;

	unsigned int bounceCount = 0;
	if (actor->fxqueue.HasEffectWithPower(fx_level_immunity_ref, parameters->int0Parameter)) {
		bounceCount = 0xFFFFFFFF;
	} else {
		const Effect *fx = actor->fxqueue.HasEffectWithPower(fx_level_immunity_dec_ref, parameters->int0Parameter);
		if (fx) {
			bounceCount = fx->Parameter1;
		}
	}

	return bounceCount > (unsigned) parameters->int1Parameter;
}

int GameScript::NumImmuneToSpellLevelLT(Scriptable *Sender, const Trigger *parameters)
{
	const Scriptable *tar = GetActorFromObject(Sender, parameters->objectParameter);
	if (!tar || tar->Type != ST_ACTOR) {
		return 0;
	}
	const Actor *actor = (const Actor *) tar;

	unsigned int bounceCount = 0;
	if (actor->fxqueue.HasEffectWithPower(fx_level_immunity_ref, parameters->int0Parameter)) {
		bounceCount = 0xFFFFFFFF;
	} else {
		const Effect *fx = actor->fxqueue.HasEffectWithPower(fx_level_immunity_dec_ref, parameters->int0Parameter);
		if (fx) {
			bounceCount = fx->Parameter1;
		}
	}

	return bounceCount < (unsigned) parameters->int1Parameter;
}

// Compares the number of ticks left of time stop to Number.
int GameScript::TimeStopCounter(Scriptable */*Sender*/, const Trigger *parameters)
{
	return core->GetGame()->RemainingTimestop() == parameters->int0Parameter;
}

int GameScript::TimeStopCounterGT(Scriptable */*Sender*/, const Trigger *parameters)
{
	return core->GetGame()->RemainingTimestop() > parameters->int0Parameter;
}

int GameScript::TimeStopCounterLT(Scriptable */*Sender*/, const Trigger *parameters)
{
	return core->GetGame()->RemainingTimestop() < parameters->int0Parameter;
}

// Returns true if the the target sprite specified by Object is the caster of time stop
int GameScript::TimeStopObject(Scriptable *Sender, const Trigger *parameters)
{
	const Scriptable *tar = GetActorFromObject(Sender, parameters->objectParameter);
	if (!tar || tar->Type != ST_ACTOR) {
		return 0;
	}

	return tar == core->GetGame()->GetTimestopOwner();
}

// Compares the number of spell traps remaining on the target creature specified
// by Object at the power Level to Amount.
static EffectRef fx_spelltrap = { "SpellTrap", -1 };
int GameScript::NumTrappingSpellLevel(Scriptable *Sender, const Trigger *parameters)
{
	const Scriptable *tar = GetActorFromObject(Sender, parameters->objectParameter);
	if (!tar || tar->Type != ST_ACTOR) {
		return 0;
	}
	const Actor *actor = (const Actor *) tar;

	int bounceCount = 0;
	const Effect *fx = actor->fxqueue.HasEffectWithPower(fx_spelltrap, parameters->int0Parameter);
	if (fx) {
		bounceCount = fx->Parameter1;
	}

	return bounceCount == parameters->int1Parameter;
}

int GameScript::NumTrappingSpellLevelGT(Scriptable *Sender, const Trigger *parameters)
{
	const Scriptable *tar = GetActorFromObject(Sender, parameters->objectParameter);
	if (!tar || tar->Type != ST_ACTOR) {
		return 0;
	}
	const Actor *actor = (const Actor *) tar;

	int bounceCount = 0;
	const Effect *fx = actor->fxqueue.HasEffectWithPower(fx_spelltrap, parameters->int0Parameter);
	if (fx) {
		bounceCount = fx->Parameter1;
	}

	return bounceCount > parameters->int1Parameter;
}

int GameScript::NumTrappingSpellLevelLT(Scriptable *Sender, const Trigger *parameters)
{
	const Scriptable *tar = GetActorFromObject(Sender, parameters->objectParameter);
	if (!tar || tar->Type != ST_ACTOR) {
		return 0;
	}
	const Actor *actor = (const Actor *) tar;

	int bounceCount = 0;
	const Effect *fx = actor->fxqueue.HasEffectWithPower(fx_spelltrap, parameters->int0Parameter);
	if (fx) {
		bounceCount = fx->Parameter1;
	}

	return bounceCount < parameters->int1Parameter;
}

// Returns true if the target creature specified by Object is dual-classed and
// the original class matches Class.
int GameScript::OriginalClass(Scriptable *Sender, const Trigger *parameters)
{
	const Scriptable *tar = GetActorFromObject(Sender, parameters->objectParameter);
	if (!tar || tar->Type != ST_ACTOR) {
		return 0;
	}
	const Actor *actor = (const Actor *) tar;

	// we need to look up the bit mapping again
	return actor->WasClass(parameters->int0Parameter);
}

// NOTE: HPLost, HPLostGT, HPLostLT are implemented above

/* ASSIGN
 * Assigns a value determined by Statement of the type Type from ARGTYPE.IDS (INT integer, or STR string)
 * to a local trigger block variable. The general form of Statement is "prefix[params]". This trigger
 * does not evaluate and does not count as a trigger in an OR() block.
 *
 * "prefix" can be:                                                                                                                                                                                                                                                                                                          *
 * prefix	| description |	params | type	| examples
 * c	| assigns a constant value	| integer or string	| c[1], c[FOO]
 * e	| assigns the value of an expression	| expression (see trigger 0x411B Eval() for format)	| e[6 + 7]
 * id	| assigns the index of a IDS file value	| file.value	| id[EA.CHARMED]
 * s	| assigns the value of the stat specified of the current object	| STATS.IDS name	| s[LEVEL]
 * sp	| assigns the value of the special value specified (of the current object, if applicable)	| ASGNSPEC.IDS name	| sp[SPRITE_PT_X]
 * tn	| assigns the value of a 2DA file value by coordinates	| file.x.y	| tn[IMPORT01.0.0]
 * ts	| assigns the value of a 2DA file value by column and row name	| file.column.row	| tn[IMPORT01.ITEMS.1]
 * v	| assigns the value of a variable	| name.scope	| v[foo.GLOBAL]
 *
 * "params" values containing #<num> and @<num> are replaced by the integer and string values,
 * respectively, stored in local trigger block variables of index "num". Avoid using integer
 * variables in expressions of string type. Avoid using string variables in expressions of integer
 * type. The range of "num" is 0 to 24.
 */
int GameScript::Assign(Scriptable */*Sender*/, const Trigger */*parameters*/)
{
	// TODO: implement
	return 0;
}

/* EVAL
 * Overwrites the (Loc)th argument of type Type from ARGTYPE.IDS (INT integer, or STR string)
 * in the next trigger with the value returned by Expression. This trigger does not evaluate
 * and does not count as a trigger in an OR() block. This trigger does not overwrite values of
 * the Assign(), NextTriggerObject() and OR() triggers. The NextTriggerObject() trigger ignores
 * this trigger.
 *
 * Expression is a math expression that can use the following symbols:                                                                                                                                                                                                                                                          *   = + - * / % ^ ( )
 *   min(x, y), max(x, y), avg(x, y)
 *   ceil(x), floor(x), round(x)
 *   abs(x)
 *   reciprocal(x)
 *   sqrt(x), pow(x, y)
 *   log(x), log10(x)
 *   sin(x), cos(x), tan(x), sinh(x), cosh(x), tanh(x), asin(x), acos(x), atan(x), atan2(x)
 *   Custom: and(x, y), or(x, y), band(x, y), bor(x, y)
 *
 * Any text in Expression of form #<num> and @<num> is replaced by the integer and string values,
 * respectively, stored in local trigger block variables of index "num". Avoid using integer
 * variables in expressions of string type. Avoid using string variables in expressions of integer
 * type. The range of "num" is 0 to 24.
 */
int GameScript::Eval(Scriptable */*Sender*/, const Trigger */*parameters*/)
{
	// TODO: implement
	return 0;
}

/* Compares "Num1" to "Num2", where E is equals, GT is greater than, and LT is less than.
 * To make use of these triggers, the 0x411B Eval() trigger should be used prior to this trigger.
 */
int GameScript::E(Scriptable */*Sender*/, const Trigger *parameters)
{
	return parameters->int0Parameter == parameters->int1Parameter;
}

int GameScript::GT(Scriptable */*Sender*/, const Trigger *parameters)
{
	return parameters->int0Parameter > parameters->int1Parameter;
}

int GameScript::LT(Scriptable */*Sender*/, const Trigger *parameters)
{
	return parameters->int0Parameter < parameters->int1Parameter;
}

/*
 * End TobEx triggers
 */

int GameScript::CurrentAmmo(Scriptable *Sender, const Trigger *parameters)
{
	const Scriptable *tar = GetActorFromObject(Sender, parameters->objectParameter);
	if (!tar || tar->Type != ST_ACTOR) {
		return 0;
	}
	const Actor *actor = (const Actor *) tar;

	int eqslot = actor->inventory.GetEquippedSlot();
	int effect = core->QuerySlotEffects(eqslot);
	if (effect != SLOT_EFFECT_MISSILE) {
		return 0;
	}

	int ammoslot = core->FindSlot(eqslot);
	if (ammoslot == -1) {
		return 0;
	}

	return actor->inventory.HasItemInSlot(parameters->string0Parameter, ammoslot);
}

}
