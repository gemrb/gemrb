/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
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

#include "DialogHandler.h"
#include "Game.h"
#include "GUI/GameControl.h"

namespace GemRB {

//-------------------------------------------------------------
// Object Functions
//-------------------------------------------------------------

//in this implementation, Myself will drop the parameter array
//i think all object filters could be expected to do so
//they should remove unnecessary elements from the parameters
Targets *GameScript::Myself(Scriptable* Sender, Targets* parameters, int ga_flags)
{
	parameters->Clear();
	parameters->AddTarget(Sender, 0, ga_flags);
	return parameters;
}

Targets *GameScript::NearestDoor(Scriptable* /*Sender*/, Targets *parameters, int /*ga_flags*/)
{
	return XthNearestDoor(parameters, 0);
}

Targets *GameScript::SecondNearestDoor(Scriptable* /*Sender*/, Targets *parameters, int /*ga_flags*/)
{
	return XthNearestDoor(parameters, 1);
}

Targets *GameScript::ThirdNearestDoor(Scriptable* /*Sender*/, Targets *parameters, int /*ga_flags*/)
{
	return XthNearestDoor(parameters, 2);
}

Targets *GameScript::FourthNearestDoor(Scriptable* /*Sender*/, Targets *parameters, int /*ga_flags*/)
{
	return XthNearestDoor(parameters, 3);
}

Targets *GameScript::FifthNearestDoor(Scriptable* /*Sender*/, Targets *parameters, int /*ga_flags*/)
{
	return XthNearestDoor(parameters, 4);
}

Targets *GameScript::SixthNearestDoor(Scriptable* /*Sender*/, Targets *parameters, int /*ga_flags*/)
{
	return XthNearestDoor(parameters, 5);
}

Targets *GameScript::SeventhNearestDoor(Scriptable* /*Sender*/, Targets *parameters, int /*ga_flags*/)
{
	return XthNearestDoor(parameters, 6);
}

Targets *GameScript::EighthNearestDoor(Scriptable* /*Sender*/, Targets *parameters, int /*ga_flags*/)
{
	return XthNearestDoor(parameters, 7);
}

Targets *GameScript::NinthNearestDoor(Scriptable* /*Sender*/, Targets *parameters, int /*ga_flags*/)
{
	return XthNearestDoor(parameters, 8);
}

Targets *GameScript::TenthNearestDoor(Scriptable* /*Sender*/, Targets *parameters, int /*ga_flags*/)
{
	return XthNearestDoor(parameters, 9);
}

//in bg2 it is same as player1 so far
//in iwd2 this is the Gabber!!!
//but also, if there is no gabber, it is the first PC
//probably it is simply the nearest exportable character...
Targets *GameScript::Protagonist(Scriptable* Sender, Targets *parameters, int ga_flags)
{
	parameters->Clear();
	//this sucks but IWD2 is like that...
	static bool charnameisgabber = core->HasFeature(GF_CHARNAMEISGABBER);
	if (charnameisgabber) {
		GameControl* gc = core->GetGameControl();
		if (gc) {
			parameters->AddTarget(gc->dialoghandler->GetSpeaker(), 0, ga_flags);
		}
		if (parameters->Count()) {
			return parameters;
		}
		//ok, this will return the nearest PC in the first slot
		Game *game = core->GetGame();
		int i = game->GetPartySize(false);
		while(i--) {
			Actor *target = game->GetPC(i,false);
			parameters->AddTarget(target, Distance(Sender, target), ga_flags);
		}
		return parameters;
	}
	parameters->AddTarget(core->GetGame()->GetPC(0, false), 0, ga_flags);
	return parameters;
}

//last talker
Targets *GameScript::Gabber(Scriptable* /*Sender*/, Targets *parameters, int ga_flags)
{
	parameters->Clear();
	GameControl* gc = core->GetGameControl();
	if (gc) {
		parameters->AddTarget(gc->dialoghandler->GetSpeaker(), 0, ga_flags);
	}
	return parameters;
}

Targets *GameScript::LastTrigger(Scriptable *Sender, Targets *parameters, int ga_flags)
{
	Scriptable *target = parameters->GetTarget(0, -1);
	parameters->Clear();
	if (target) {
		target = Sender->GetCurrentArea()->GetActorByGlobalID(target->LastTrigger);
		parameters->AddTarget(target, 0, ga_flags);
	} else if (Sender->LastTrigger) {
		target = Sender->GetCurrentArea()->GetActorByGlobalID(Sender->LastTrigger);
		parameters->AddTarget(target, 0, ga_flags);
	}
	return parameters;
}

Targets *GameScript::LastMarkedObject(Scriptable *Sender, Targets *parameters, int ga_flags)
{
	Actor *actor = (Actor *) parameters->GetTarget(0, ST_ACTOR);
	if (!actor) {
		if (Sender->Type==ST_ACTOR) {
			actor = (Actor *) Sender;
		}
	}
	parameters->Clear();
	if (actor) {
		Actor *target = actor->GetCurrentArea()->GetActorByGlobalID(actor->LastMarked);
		if (target) {
			parameters->AddTarget(target, 0, ga_flags);
		}
	}
	return parameters;
}

Targets *GameScript::SpellTarget(Scriptable *Sender, Targets *parameters, int ga_flags)
{
	Actor *actor = (Actor *) parameters->GetTarget(0, ST_ACTOR);
	if (!actor) {
		if (Sender->Type==ST_ACTOR) {
			actor = (Actor *) Sender;
		}
	}
	parameters->Clear();
	if (actor) {
		Actor *target = actor->GetCurrentArea()->GetActorByGlobalID(actor->LastTarget);
		if (target) {
			parameters->AddTarget(target, 0, ga_flags);
		}
	}
	return parameters;
}

//actions should always use LastMarkedObject, because LastSeen could be deleted
Targets *GameScript::LastSeenBy(Scriptable *Sender, Targets *parameters, int ga_flags)
{
	Actor *actor = (Actor *) parameters->GetTarget(0, ST_ACTOR);
	if (!actor) {
		if (Sender->Type==ST_ACTOR) {
			actor = (Actor *) Sender;
		}
	}
	parameters->Clear();
	if (actor) {
		Actor *target = actor->GetCurrentArea()->GetActorByGlobalID(actor->LastSeen);
		if (target) {
			parameters->AddTarget(target, 0, ga_flags);
		}
	}
	return parameters;
}

Targets *GameScript::LastHelp(Scriptable *Sender, Targets *parameters, int ga_flags)
{
	Actor *actor = (Actor *) parameters->GetTarget(0, ST_ACTOR);
	if (!actor) {
		if (Sender->Type==ST_ACTOR) {
			actor = (Actor *) Sender;
		}
	}
	parameters->Clear();
	if (actor) {
		Actor *target = actor->GetCurrentArea()->GetActorByGlobalID(actor->LastHelp);
		if (target) {
			parameters->AddTarget(target, 0, ga_flags);
		}
	}
	return parameters;
}

Targets *GameScript::LastHeardBy(Scriptable *Sender, Targets *parameters, int ga_flags)
{
	Actor *actor = (Actor *) parameters->GetTarget(0, ST_ACTOR);
	if (!actor) {
		if (Sender->Type==ST_ACTOR) {
			actor = (Actor *) Sender;
		}
	}
	parameters->Clear();
	if (actor) {
		Actor *target = actor->GetCurrentArea()->GetActorByGlobalID(actor->LastHeard);
		if (target) {
			parameters->AddTarget(target, 0, ga_flags);
		}
	}
	return parameters;
}

//i was told that Group means the same specifics, so this is just an
//object selector for everyone with the same specifics as the current object
Targets *GameScript::GroupOf(Scriptable *Sender, Targets *parameters, int ga_flags)
{
	Actor *actor = (Actor *) parameters->GetTarget(0, ST_ACTOR);
	if (!actor) {
		if (Sender->Type==ST_ACTOR) {
			actor = (Actor *) Sender;
		}
	}
	parameters->Clear();
	if (actor) {
		ieDword tmp = actor->GetStat(IE_SPECIFIC);
		Map *cm = Sender->GetCurrentArea();
		int i = cm->GetActorCount(true);
		while (i--) {
			Actor *target=cm->GetActor(i,true);
			if (target && (target->GetStat(IE_SPECIFIC)==tmp) ) {
				parameters->AddTarget(target, 0, ga_flags);
			}
		}
	}
	return parameters;
}

/*this one is tough, but done */
Targets *GameScript::ProtectorOf(Scriptable *Sender, Targets *parameters, int ga_flags)
{
	Actor *actor = (Actor *) parameters->GetTarget(0, ST_ACTOR);
	if (!actor) {
		if (Sender->Type==ST_ACTOR) {
			actor = (Actor *) Sender;
		}
	}
	parameters->Clear();
	/*if (actor) {
		ieWord tmp = actor->LastProtected;
		Map *cm = Sender->GetCurrentArea();
		int i = cm->GetActorCount(true);
		while (i--) {
			Actor *target=cm->GetActor(i,true);
			if (target && (target->LastProtected ==tmp) ) {
				parameters->AddTarget(target, 0, ga_flags);
			}
		}
	}*/
	if (actor) {
		Actor *target = actor->GetCurrentArea()->GetActorByGlobalID(actor->LastProtector);
		if (target) {
			parameters->AddTarget(target, 0, ga_flags);
		}
	}
	return parameters;
}

Targets *GameScript::ProtectedBy(Scriptable *Sender, Targets *parameters, int ga_flags)
{
	Actor *actor = (Actor *) parameters->GetTarget(0, ST_ACTOR);
	if (!actor) {
		if (Sender->Type==ST_ACTOR) {
			actor = (Actor *) Sender;
		}
	}
	parameters->Clear();
	if (actor) {
		Actor *target = actor->GetCurrentArea()->GetActorByGlobalID(actor->LastProtectee);
		if (target) {
			parameters->AddTarget(target, 0, ga_flags);
		}
	}
	return parameters;
}

Targets *GameScript::LastCommandedBy(Scriptable *Sender, Targets *parameters, int ga_flags)
{
	Actor *actor = (Actor *) parameters->GetTarget(0, ST_ACTOR);
	if (!actor) {
		if (Sender->Type==ST_ACTOR) {
			actor = (Actor *) Sender;
		}
	}
	parameters->Clear();
	if (actor) {
		Actor *target = actor->GetCurrentArea()->GetActorByGlobalID(actor->LastCommander);
		if (target) {
			parameters->AddTarget(target, 0, ga_flags);
		}
	}
	return parameters;
}

// this is essentially a LastTargetedBy(0) - or MySelf
// but IWD2 defines it
Targets *GameScript::MyTarget(Scriptable *Sender, Targets *parameters, int ga_flags)
{
	return GetMyTarget(Sender, NULL, parameters, ga_flags);
}

Targets *GameScript::LastTargetedBy(Scriptable *Sender, Targets *parameters, int ga_flags)
{
	Actor *actor = (Actor *) parameters->GetTarget(0, ST_ACTOR);
	return GetMyTarget(Sender, actor, parameters, ga_flags);
}

Targets *GameScript::LastAttackerOf(Scriptable *Sender, Targets *parameters, int ga_flags)
{
	Actor *actor = (Actor *) parameters->GetTarget(0, ST_ACTOR);
	if (!actor) {
		if (Sender->Type==ST_ACTOR) {
			actor = (Actor *) Sender;
		}
	}
	parameters->Clear();
	if (actor) {
		Actor *target = actor->GetCurrentArea()->GetActorByGlobalID(actor->LastAttacker);
		if (target) {
			parameters->AddTarget(target, 0, ga_flags);
		}
	}
	return parameters;
}

Targets *GameScript::LastHitter(Scriptable *Sender, Targets *parameters, int ga_flags)
{
	Actor *actor = (Actor *) parameters->GetTarget(0, ST_ACTOR);
	if (!actor) {
		if (Sender->Type==ST_ACTOR) {
			actor = (Actor *) Sender;
		}
	}
	parameters->Clear();
	if (actor) {
		Actor *target = actor->GetCurrentArea()->GetActorByGlobalID(actor->LastHitter);
		if (target) {
			parameters->AddTarget(target, 0, ga_flags);
		}
	}
	return parameters;
}

Targets *GameScript::LeaderOf(Scriptable *Sender, Targets *parameters, int ga_flags)
{
	Actor *actor = (Actor *) parameters->GetTarget(0, ST_ACTOR);
	if (!actor) {
		if (Sender->Type==ST_ACTOR) {
			actor = (Actor *) Sender;
		}
	}
	parameters->Clear();
	if (actor) {
		Actor *target = actor->GetCurrentArea()->GetActorByGlobalID(actor->LastFollowed);
		if (target) {
			parameters->AddTarget(target, 0, ga_flags);
		}
	}
	return parameters;
}

Targets *GameScript::LastTalkedToBy(Scriptable *Sender, Targets *parameters, int ga_flags)
{
	Actor *actor = (Actor *) parameters->GetTarget(0, ST_ACTOR);
	if (!actor) {
		if (Sender->Type==ST_ACTOR) {
			actor = (Actor *) Sender;
		}
	}
	parameters->Clear();
	if (actor) {
		Actor *target = actor->GetCurrentArea()->GetActorByGlobalID(actor->LastTalker);
		if (target) {
			parameters->AddTarget(target, 0, ga_flags);
		}
	}
	return parameters;
}

Targets *GameScript::LastSummonerOf(Scriptable* Sender, Targets *parameters, int ga_flags)
{
	Actor *actor = (Actor *) parameters->GetTarget(0, ST_ACTOR);
	if (!actor) {
		if (Sender->Type==ST_ACTOR) {
			actor = (Actor *) Sender;
		}
	}
	parameters->Clear();
	if (actor) {
		Actor *target = actor->GetCurrentArea()->GetActorByGlobalID(actor->LastSummoner);
		if (target) {
			parameters->AddTarget(target, 0, ga_flags);
		}
	}
	return parameters;
}

Targets *GameScript::Player1(Scriptable* /*Sender*/, Targets *parameters, int ga_flags)
{
	parameters->Clear();
	parameters->AddTarget(core->GetGame()->GetPC(0,false), 0, ga_flags);
	return parameters;
}

Targets *GameScript::Player1Fill(Scriptable* /*Sender*/, Targets *parameters, int ga_flags)
{
	parameters->Clear();
	parameters->AddTarget(core->GetGame()->FindPC(1), 0, ga_flags);
	return parameters;
}

Targets *GameScript::Player2(Scriptable* /*Sender*/, Targets *parameters, int ga_flags)
{
	parameters->Clear();
	parameters->AddTarget(core->GetGame()->GetPC(1,false), 0, ga_flags);
	return parameters;
}

Targets *GameScript::Player2Fill(Scriptable* /*Sender*/, Targets *parameters, int ga_flags)
{
	parameters->Clear();
	parameters->AddTarget(core->GetGame()->FindPC(2), 0, ga_flags);
	return parameters;
}

Targets *GameScript::Player3(Scriptable* /*Sender*/, Targets *parameters, int ga_flags)
{
	parameters->Clear();
	parameters->AddTarget(core->GetGame()->GetPC(2,false), 0, ga_flags);
	return parameters;
}

Targets *GameScript::Player3Fill(Scriptable* /*Sender*/, Targets *parameters, int ga_flags)
{
	parameters->Clear();
	parameters->AddTarget(core->GetGame()->FindPC(3), 0, ga_flags);
	return parameters;
}

Targets *GameScript::Player4(Scriptable* /*Sender*/, Targets *parameters, int ga_flags)
{
	parameters->Clear();
	parameters->AddTarget(core->GetGame()->GetPC(3,false), 0, ga_flags);
	return parameters;
}

Targets *GameScript::Player4Fill(Scriptable* /*Sender*/, Targets *parameters, int ga_flags)
{
	parameters->Clear();
	parameters->AddTarget(core->GetGame()->FindPC(4), 0, ga_flags);
	return parameters;
}

Targets *GameScript::Player5(Scriptable* /*Sender*/, Targets *parameters, int ga_flags)
{
	parameters->Clear();
	parameters->AddTarget(core->GetGame()->GetPC(4,false), 0, ga_flags);
	return parameters;
}

Targets *GameScript::Player5Fill(Scriptable* /*Sender*/, Targets *parameters, int ga_flags)
{
	parameters->Clear();
	parameters->AddTarget(core->GetGame()->FindPC(5), 0, ga_flags);
	return parameters;
}

Targets *GameScript::Player6(Scriptable* /*Sender*/, Targets *parameters, int ga_flags)
{
	parameters->Clear();
	parameters->AddTarget(core->GetGame()->GetPC(5,false), 0, ga_flags);
	return parameters;
}

Targets *GameScript::Player6Fill(Scriptable* /*Sender*/, Targets *parameters, int ga_flags)
{
	parameters->Clear();
	parameters->AddTarget(core->GetGame()->FindPC(6), 0, ga_flags);
	return parameters;
}

Targets *GameScript::Player7(Scriptable* /*Sender*/, Targets *parameters, int ga_flags)
{
	parameters->Clear();
	parameters->AddTarget(core->GetGame()->GetPC(6,false), 0, ga_flags);
	return parameters;
}

Targets *GameScript::Player7Fill(Scriptable* /*Sender*/, Targets *parameters, int ga_flags)
{
	parameters->Clear();
	parameters->AddTarget(core->GetGame()->FindPC(7), 0, ga_flags);
	return parameters;
}

Targets *GameScript::Player8(Scriptable* /*Sender*/, Targets *parameters, int ga_flags)
{
	parameters->Clear();
	parameters->AddTarget(core->GetGame()->GetPC(7,false), 0, ga_flags);
	return parameters;
}

Targets *GameScript::Player8Fill(Scriptable* /*Sender*/, Targets *parameters, int ga_flags)
{
	parameters->Clear();
	parameters->AddTarget(core->GetGame()->FindPC(8), 0, ga_flags);
	return parameters;
}

//This filter works only on the Party - silly restriction, but the dataset expects this
Targets *GameScript::StrongestOfMale(Scriptable* Sender, Targets *parameters, int ga_flags)
{
	Map* area = Sender->GetCurrentArea();
	Game *game = core->GetGame();
	Scriptable* scr = NULL;
	int besthp = 0;
	int i = game->GetPartySize(false);
	while (i--) {
		Actor *actor = game->GetPC(i, false);
		if (actor->GetStat(IE_SEX)!=SEX_MALE) continue;
		if(actor->GetCurrentArea() == area) {
			int hp = actor->GetStat(IE_HITPOINTS);
			if (!scr || besthp<hp) {
				besthp=hp;
				scr=actor;
			}
		}
	}
	parameters->Clear();
	parameters->AddTarget(scr, 0, ga_flags);
	return parameters;
}

//This filter works only on the Party - silly restriction, but the dataset expects this
Targets *GameScript::StrongestOf(Scriptable* Sender, Targets *parameters, int ga_flags)
{
	Map* area = Sender->GetCurrentArea();
	Game *game = core->GetGame();
	Scriptable* scr = NULL;
	int besthp = 0;
	int i = game->GetPartySize(false);
	while (i--) {
		Actor *actor = game->GetPC(i, false);
		if(actor->GetCurrentArea() == area) {
			int hp = actor->GetStat(IE_HITPOINTS);
			if (!scr || besthp<hp) {
				besthp=hp;
				scr=actor;
			}
		}
	}
	parameters->Clear();
	parameters->AddTarget(scr, 0, ga_flags);
	return parameters;
}

//This filter works only on the Party - silly restriction, but the dataset expects this
Targets *GameScript::WeakestOf(Scriptable* Sender, Targets *parameters, int ga_flags)
{
	Map* area = Sender->GetCurrentArea();
	Game *game = core->GetGame();
	Scriptable* scr = NULL;
	int worsthp = 0;
	int i = game->GetPartySize(false);
	while (i--) {
		Actor *actor = game->GetPC(i, false);
		if(actor->GetCurrentArea() == area) {
			int hp = actor->GetStat(IE_HITPOINTS);
			if (!scr || worsthp>hp) {
				worsthp=hp;
				scr=actor;
			}
		}
	}
	parameters->Clear();
	parameters->AddTarget(scr, 0, ga_flags);
	return parameters;
}

//This filter works only on the Party - silly restriction, but the dataset expects this
Targets *GameScript::BestAC(Scriptable* Sender, Targets *parameters, int ga_flags)
{
	Map* area = Sender->GetCurrentArea();
	Game *game = core->GetGame();
	Scriptable* scr = NULL;
	int bestac = 0;
	int i = game->GetPartySize(false);
	while (i--) {
		Actor *actor = game->GetPC(i, false);
		if(actor->GetCurrentArea() == area) {
			int ac = actor->AC.GetTotal();
			if (!scr || bestac>ac) {
				bestac=ac;
				scr=actor;
			}
		}
	}
	parameters->Clear();
	parameters->AddTarget(scr, 0, ga_flags);
	return parameters;
}

//This filter works only on the Party - silly restriction, but the dataset expects this
Targets *GameScript::WorstAC(Scriptable* Sender, Targets *parameters, int ga_flags)
{
	Map* area = Sender->GetCurrentArea();
	Game *game = core->GetGame();
	Scriptable* scr = NULL;
	int worstac = 0;
	int i = game->GetPartySize(false);
	while (i--) {
		Actor *actor = game->GetPC(i, false);
		if(actor->GetCurrentArea() == area) {
			int ac = actor->AC.GetTotal();
			if (!scr || worstac<ac) {
				worstac=ac;
				scr=actor;
			}
		}
	}
	parameters->Clear();
	parameters->AddTarget(scr, 0, ga_flags);
	return parameters;
}

//This filter works only on the Party - silly restriction, but the dataset expects this
Targets *GameScript::MostDamagedOf(Scriptable* Sender, Targets *parameters, int ga_flags)
{
	Map* area = Sender->GetCurrentArea();
	Game *game = core->GetGame();
	Scriptable* scr = NULL;
	int worsthp = 0;
	int i = game->GetPartySize(false);
	while (i--) {
		Actor *actor = game->GetPC(i, false);
		if(actor->GetCurrentArea() == area) {
			int hp=actor->GetStat(IE_MAXHITPOINTS)-actor->GetBase(IE_HITPOINTS);
			if (!scr || hp>worsthp) {
				worsthp=hp;
				scr=actor;
			}
		}
	}
	parameters->Clear();
	parameters->AddTarget(scr, 0, ga_flags);
	return parameters;
}

//This filter works only on the Party - silly restriction, but the dataset expects this
//For example the beholder01 script
Targets *GameScript::LeastDamagedOf(Scriptable* Sender, Targets *parameters, int ga_flags)
{
	Map* area = Sender->GetCurrentArea();
	Game *game = core->GetGame();
	Scriptable* scr = NULL;
	int besthp = 0;
	int i = game->GetPartySize(false);
	while (i--) {
		Actor *actor = game->GetPC(i, false);
		if(actor->GetCurrentArea() == area) {
			int hp=actor->GetStat(IE_MAXHITPOINTS)-actor->GetBase(IE_HITPOINTS);
			if (!scr || besthp<hp) {
				besthp=hp;
				scr=actor;
			}
		}
	}
	parameters->Clear();
	parameters->AddTarget(scr, 0, ga_flags);
	return parameters;
}

Targets *GameScript::Farthest(Scriptable* /*Sender*/, Targets *parameters, int ga_flags)
{
	const targettype *t = parameters->GetLastTarget(ST_ACTOR);
	parameters->Clear();
	if (t) {
		parameters->AddTarget(t->actor, 0, ga_flags);
	}
	return parameters;
}

Targets *GameScript::FarthestEnemyOf(Scriptable* /*Sender*/, Targets *parameters, int ga_flags)
{
	return XthNearestEnemyOf(parameters, -1, ga_flags);
}

Targets *GameScript::NearestEnemyOf(Scriptable* /*Sender*/, Targets *parameters, int ga_flags)
{
	return XthNearestEnemyOf(parameters, 0, ga_flags);
}

Targets *GameScript::SecondNearestEnemyOf(Scriptable* /*Sender*/, Targets *parameters, int ga_flags)
{
	return XthNearestEnemyOf(parameters, 1, ga_flags);
}

Targets *GameScript::ThirdNearestEnemyOf(Scriptable* /*Sender*/, Targets *parameters, int ga_flags)
{
	return XthNearestEnemyOf(parameters, 2, ga_flags);
}

Targets *GameScript::FourthNearestEnemyOf(Scriptable* /*Sender*/, Targets *parameters, int ga_flags)
{
	return XthNearestEnemyOf(parameters, 3, ga_flags);
}

Targets *GameScript::FifthNearestEnemyOf(Scriptable* /*Sender*/, Targets *parameters, int ga_flags)
{
	return XthNearestEnemyOf(parameters, 4, ga_flags);
}

Targets *GameScript::SixthNearestEnemyOf(Scriptable* /*Sender*/, Targets *parameters, int ga_flags)
{
	return XthNearestEnemyOf(parameters, 5, ga_flags);
}

Targets *GameScript::SeventhNearestEnemyOf(Scriptable* /*Sender*/, Targets *parameters, int ga_flags)
{
	return XthNearestEnemyOf(parameters, 6, ga_flags);
}

Targets *GameScript::EighthNearestEnemyOf(Scriptable* /*Sender*/, Targets *parameters, int ga_flags)
{
	return XthNearestEnemyOf(parameters, 7, ga_flags);
}

Targets *GameScript::NinthNearestEnemyOf(Scriptable* /*Sender*/, Targets *parameters, int ga_flags)
{
	return XthNearestEnemyOf(parameters, 8, ga_flags);
}

Targets *GameScript::TenthNearestEnemyOf(Scriptable* /*Sender*/, Targets *parameters, int ga_flags)
{
	return XthNearestEnemyOf(parameters, 9, ga_flags);
}

Targets *GameScript::NearestEnemySummoned(Scriptable* Sender, Targets *parameters, int ga_flags)
{
	return ClosestEnemySummoned(Sender, parameters, ga_flags);
}

Targets *GameScript::NearestEnemyOfType(Scriptable* Sender, Targets *parameters, int ga_flags)
{
	return XthNearestEnemyOfType(Sender, parameters, 0, ga_flags);
}

Targets *GameScript::SecondNearestEnemyOfType(Scriptable* Sender, Targets *parameters, int ga_flags)
{
	return XthNearestEnemyOfType(Sender, parameters, 1, ga_flags);
}

Targets *GameScript::ThirdNearestEnemyOfType(Scriptable* Sender, Targets *parameters, int ga_flags)
{
	return XthNearestEnemyOfType(Sender, parameters, 2, ga_flags);
}

Targets *GameScript::FourthNearestEnemyOfType(Scriptable* Sender, Targets *parameters, int ga_flags)
{
	return XthNearestEnemyOfType(Sender, parameters, 3, ga_flags);
}

Targets *GameScript::FifthNearestEnemyOfType(Scriptable* Sender, Targets *parameters, int ga_flags)
{
	return XthNearestEnemyOfType(Sender, parameters, 4, ga_flags);
}

Targets *GameScript::SixthNearestEnemyOfType(Scriptable* Sender, Targets *parameters, int ga_flags)
{
	return XthNearestEnemyOfType(Sender, parameters, 5, ga_flags);
}

Targets *GameScript::SeventhNearestEnemyOfType(Scriptable* Sender, Targets *parameters, int ga_flags)
{
	return XthNearestEnemyOfType(Sender, parameters, 6, ga_flags);
}

Targets *GameScript::EighthNearestEnemyOfType(Scriptable* Sender, Targets *parameters, int ga_flags)
{
	return XthNearestEnemyOfType(Sender, parameters, 7, ga_flags);
}

Targets *GameScript::NinthNearestEnemyOfType(Scriptable* Sender, Targets *parameters, int ga_flags)
{
	return XthNearestEnemyOfType(Sender, parameters, 8, ga_flags);
}

Targets *GameScript::TenthNearestEnemyOfType(Scriptable* Sender, Targets *parameters, int ga_flags)
{
	return XthNearestEnemyOfType(Sender, parameters, 9, ga_flags);
}

Targets *GameScript::NearestMyGroupOfType(Scriptable* Sender, Targets *parameters, int ga_flags)
{
	return XthNearestMyGroupOfType(Sender, parameters, 0, ga_flags);
}

Targets *GameScript::SecondNearestMyGroupOfType(Scriptable* Sender, Targets *parameters, int ga_flags)
{
	return XthNearestMyGroupOfType(Sender, parameters, 1, ga_flags);
}

Targets *GameScript::ThirdNearestMyGroupOfType(Scriptable* Sender, Targets *parameters, int ga_flags)
{
	return XthNearestMyGroupOfType(Sender, parameters, 2, ga_flags);
}

Targets *GameScript::FourthNearestMyGroupOfType(Scriptable* Sender, Targets *parameters, int ga_flags)
{
	return XthNearestMyGroupOfType(Sender, parameters, 3, ga_flags);
}

Targets *GameScript::FifthNearestMyGroupOfType(Scriptable* Sender, Targets *parameters, int ga_flags)
{
	return XthNearestMyGroupOfType(Sender, parameters, 4, ga_flags);
}

Targets *GameScript::SixthNearestMyGroupOfType(Scriptable* Sender, Targets *parameters, int ga_flags)
{
	return XthNearestMyGroupOfType(Sender, parameters, 5, ga_flags);
}

Targets *GameScript::SeventhNearestMyGroupOfType(Scriptable* Sender, Targets *parameters, int ga_flags)
{
	return XthNearestMyGroupOfType(Sender, parameters, 6, ga_flags);
}

Targets *GameScript::EighthNearestMyGroupOfType(Scriptable* Sender, Targets *parameters, int ga_flags)
{
	return XthNearestMyGroupOfType(Sender, parameters, 7, ga_flags);
}

Targets *GameScript::NinthNearestMyGroupOfType(Scriptable* Sender, Targets *parameters, int ga_flags)
{
	return XthNearestMyGroupOfType(Sender, parameters, 8, ga_flags);
}

Targets *GameScript::TenthNearestMyGroupOfType(Scriptable* Sender, Targets *parameters, int ga_flags)
{
	return XthNearestMyGroupOfType(Sender, parameters, 9, ga_flags);
}

/* returns only living PC's? if not, alter getpartysize/getpc flag*/
Targets *GameScript::NearestPC(Scriptable* Sender, Targets *parameters, int ga_flags)
{
	parameters->Clear();
	Map *map = Sender->GetCurrentArea();
	Game *game = core->GetGame();
	int i = game->GetPartySize(true);
	int mindist = -1;
	Actor *ac = NULL;
	while (i--) {
		Actor *newactor=game->GetPC(i,true);
		//NearestPC for PC's will not give themselves as a result
		//this might be different from the original engine
		if ((Sender->Type==ST_ACTOR) && (newactor == (Actor *) Sender)) {
			continue;
		}
		if (newactor->GetCurrentArea()!=map) {
			continue;
		}
		int dist = Distance(Sender, newactor);
		if ( (mindist == -1) || (dist<mindist) ) {
			ac = newactor;
			mindist = dist;
		}
	}
	if (ac) {
		parameters->AddTarget(ac, 0, ga_flags);
	}
	return parameters;
}

Targets *GameScript::Nearest(Scriptable* /*Sender*/, Targets *parameters, int ga_flags)
{
	return XthNearestOf(parameters, 0, ga_flags);
}

Targets *GameScript::SecondNearest(Scriptable* /*Sender*/, Targets *parameters, int ga_flags)
{
	return XthNearestOf(parameters, 1, ga_flags);
}

Targets *GameScript::ThirdNearest(Scriptable* /*Sender*/, Targets *parameters, int ga_flags)
{
	return XthNearestOf(parameters, 2, ga_flags);
}

Targets *GameScript::FourthNearest(Scriptable* /*Sender*/, Targets *parameters, int ga_flags)
{
	return XthNearestOf(parameters, 3, ga_flags);
}

Targets *GameScript::FifthNearest(Scriptable* /*Sender*/, Targets *parameters, int ga_flags)
{
	return XthNearestOf(parameters, 4, ga_flags);
}

Targets *GameScript::SixthNearest(Scriptable* /*Sender*/, Targets *parameters, int ga_flags)
{
	return XthNearestOf(parameters, 5, ga_flags);
}

Targets *GameScript::SeventhNearest(Scriptable* /*Sender*/, Targets *parameters, int ga_flags)
{
	return XthNearestOf(parameters, 6, ga_flags);
}

Targets *GameScript::EighthNearest(Scriptable* /*Sender*/, Targets *parameters, int ga_flags)
{
	return XthNearestOf(parameters, 7, ga_flags);
}

Targets *GameScript::NinthNearest(Scriptable* /*Sender*/, Targets *parameters, int ga_flags)
{
	return XthNearestOf(parameters, 8, ga_flags);
}

Targets *GameScript::TenthNearest(Scriptable* /*Sender*/, Targets *parameters, int ga_flags)
{
	return XthNearestOf(parameters, 9, ga_flags);
}

Targets *GameScript::SelectedCharacter(Scriptable* Sender, Targets* parameters, int ga_flags)
{
	Map *cm = Sender->GetCurrentArea();
	parameters->Clear();
	int i = cm->GetActorCount(true);
	while (i--) {
		Actor *ac=cm->GetActor(i,true);
		if (ac->GetCurrentArea()!=cm) {
			continue;
		}
		if (ac->IsSelected()) {
			parameters->AddTarget(ac, Distance(Sender, ac), ga_flags );
		}
	}
	return parameters;
}

Targets *GameScript::Nothing(Scriptable* /*Sender*/, Targets* parameters, int /*ga_flags*/)
{
	parameters->Clear();
	return parameters;
}

//-------------------------------------------------------------
// IDS Functions
//-------------------------------------------------------------

int GameScript::ID_Alignment(Actor *actor, int parameter)
{
	int value = actor->GetStat( IE_ALIGNMENT );
	int a = parameter&15;
	if (a) {
		if (a != ( value & 15 )) {
			return 0;
		}
	}
	a = parameter & 240;
	if (a) {
		if (a != ( value & 240 )) {
			return 0;
		}
	}
	return 1;
}

int GameScript::ID_Allegiance(Actor *actor, int parameter)
{
	int value = actor->GetStat( IE_EA );
	switch (parameter) {
		case EA_GOODCUTOFF:
			return value <= EA_GOODCUTOFF;

		case EA_NOTGOOD:
			return value >= EA_NOTGOOD;

		case EA_NOTNEUTRAL:
			return value >=EA_EVILCUTOFF || value <= EA_GOODCUTOFF;

		case EA_NOTEVIL:
			return value <= EA_NOTEVIL;

		case EA_EVILCUTOFF:
			return value >= EA_EVILCUTOFF;

		case 0:
		case EA_ANYTHING:
			return true;

	}
	//default
	return parameter == value;
}

// *_ALL constants are different in iwd2 due to different classes (see note below)
// bard, cleric, druid, fighter, mage, paladin, ranger, thief
static const int all_bg_classes[] = { 206, 204, 208, 203, 202, 207, 209, 205 };
static const int all_iwd2_classes[] = { 202, 203, 204, 205, 209, 206, 207, 208 };

// Dual-classed characters will detect only as their new class until their
// original class is re-activated, when they will detect as a multi-class
// GetClassLevel takes care of this automatically!
inline bool idclass(Actor *actor, int parameter, bool iwd2) {
	int value = 0;
	if (parameter < 202 || parameter > 209) {
		value = actor->GetStat(IE_CLASS);
		return parameter==value;
	}

	const int *classes;
	if (iwd2) {
		classes = all_iwd2_classes;
	} else {
		classes = all_bg_classes;
	}

	// FIXME: unhardcode this ugly mess
	// in IWD2, mage_all = sorcerer or wizard
	// fighter_all = fighter, paladin or ranger (but not monk)
	// cleric_all = druid or cleric
	// thief_all = thief
	// bard_all = bard
	// paladin_all = paladin
	// druid_all = druid
	// ranger_all = ranger

	// we got one of the *_ALL values
	if (parameter == classes[4]) {
		// MAGE_ALL (also sorcerers)
		value = actor->GetMageLevel() + actor->GetSorcererLevel();
	} else if (parameter == classes[3]) {
		// FIGHTER_ALL (also monks)
		if (iwd2) {
			value = actor->GetFighterLevel() + actor->GetPaladinLevel() + actor->GetRangerLevel();
		} else {
			value = actor->GetFighterLevel() + actor->GetMonkLevel();
		}
	} else if (parameter == classes[1]) {
		// CLERIC_ALL
		if (iwd2) {
			value = actor->GetClericLevel() + actor->GetDruidLevel();
		} else {
			value = actor->GetClericLevel();
		}
	} else if (parameter == classes[7]) {
		// THIEF_ALL
		value = actor->GetThiefLevel();
	} else if (parameter == classes[0]) {
		// BARD_ALL
		value = actor->GetBardLevel();
	} else if (parameter == classes[5]) {
		// PALADIN_ALL
		value = actor->GetPaladinLevel();
	} else if (parameter == classes[2]) {
		// DRUID_ALL
		value = actor->GetDruidLevel();
	} else if (parameter == classes[6]) {
		// RANGER_ALL
		value = actor->GetRangerLevel();
	}
	return value > 0;
}

int GameScript::ID_Class(Actor *actor, int parameter)
{
	if (core->HasFeature(GF_3ED_RULES)) {
		//iwd2 has different values, see also the note for AVClass
		return idclass(actor, parameter, 1);
	}
	return idclass(actor, parameter, 0);
}

// IE_CLASS holds only one class, not a bitmask like with iwd2 kits. The ids values
// are friendly to binary comparison, so we just need to build such a class value
int GameScript::ID_ClassMask(Actor *actor, int parameter)
{
	// maybe we're lucky...
	int value = actor->GetStat(IE_CLASS);
	if (parameter&(1<<(value-1))) return 1;

	// otherwise iterate over all the classes
	value = actor->GetClassMask();

	if (parameter&value) return 1;
	return 0;
}

// this is only present in iwd2
// the function is identical to ID_Class, but uses the class20 IDS,
// iwd2's class.ids is different than the rest, while class20 is identical (remnant)
int GameScript::ID_AVClass(Actor *actor, int parameter)
{
	return idclass(actor, parameter, 0);
}

int GameScript::ID_Race(Actor *actor, int parameter)
{
	int value = actor->GetStat(IE_RACE);
	return parameter==value;
}

int GameScript::ID_Subrace(Actor *actor, int parameter)
{
	int value = actor->GetStat(IE_SUBRACE);
	return parameter==value;
}

int GameScript::ID_Faction(Actor *actor, int parameter)
{
	int value = actor->GetStat(IE_FACTION);
	return parameter==value;
}

int GameScript::ID_Team(Actor *actor, int parameter)
{
	int value = actor->GetStat(IE_TEAM);
	return parameter==value;
}

int GameScript::ID_Gender(Actor *actor, int parameter)
{
	int value = actor->GetStat(IE_SEX);
	return parameter==value;
}

int GameScript::ID_General(Actor *actor, int parameter)
{
	int value = actor->GetStat(IE_GENERAL);
	return parameter==value;
}

int GameScript::ID_Specific(Actor *actor, int parameter)
{
	int value = actor->GetStat(IE_SPECIFIC);
	return parameter==value;
}

}
