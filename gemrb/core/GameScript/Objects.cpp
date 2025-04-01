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

#include "ie_stats.h"

#include "DialogHandler.h"
#include "Game.h"
#include "Interface.h"
#include "Map.h"

#include "GUI/GameControl.h"
#include "GameScript/GameScript.h"
#include "GameScript/Matching.h"
#include "GameScript/Targets.h"
#include "Scriptable/InfoPoint.h"

namespace GemRB {

//-------------------------------------------------------------
// Object Functions
//-------------------------------------------------------------

//in this implementation, Myself will drop the parameter array
//i think all object filters could be expected to do so
//they should remove unnecessary elements from the parameters
Targets* GameScript::Myself(const Scriptable* Sender, Targets* parameters, int ga_flags)
{
	parameters->Clear();
	// hack to allow all Object methods to take a const Scriptable
	Scriptable* snd;
	Map* ca = Sender->GetCurrentArea();
	if (ca) {
		snd = ca->GetScriptableByGlobalID(Sender->GetGlobalID());
	} else {
		// just in case we're in the middle of a move
		snd = core->GetGame()->GetActorByGlobalID(Sender->GetGlobalID());
	}
	// you are always visible to yourself
	parameters->AddTarget(snd, 0, ga_flags & ~GA_NO_UNSCHEDULED);
	return parameters;
}

Targets* GameScript::NearestDoor(const Scriptable* /*Sender*/, Targets* parameters, int /*ga_flags*/)
{
	return XthNearestDoor(parameters, 0);
}

Targets* GameScript::SecondNearestDoor(const Scriptable* /*Sender*/, Targets* parameters, int /*ga_flags*/)
{
	return XthNearestDoor(parameters, 1);
}

Targets* GameScript::ThirdNearestDoor(const Scriptable* /*Sender*/, Targets* parameters, int /*ga_flags*/)
{
	return XthNearestDoor(parameters, 2);
}

Targets* GameScript::FourthNearestDoor(const Scriptable* /*Sender*/, Targets* parameters, int /*ga_flags*/)
{
	return XthNearestDoor(parameters, 3);
}

Targets* GameScript::FifthNearestDoor(const Scriptable* /*Sender*/, Targets* parameters, int /*ga_flags*/)
{
	return XthNearestDoor(parameters, 4);
}

Targets* GameScript::SixthNearestDoor(const Scriptable* /*Sender*/, Targets* parameters, int /*ga_flags*/)
{
	return XthNearestDoor(parameters, 5);
}

Targets* GameScript::SeventhNearestDoor(const Scriptable* /*Sender*/, Targets* parameters, int /*ga_flags*/)
{
	return XthNearestDoor(parameters, 6);
}

Targets* GameScript::EighthNearestDoor(const Scriptable* /*Sender*/, Targets* parameters, int /*ga_flags*/)
{
	return XthNearestDoor(parameters, 7);
}

Targets* GameScript::NinthNearestDoor(const Scriptable* /*Sender*/, Targets* parameters, int /*ga_flags*/)
{
	return XthNearestDoor(parameters, 8);
}

Targets* GameScript::TenthNearestDoor(const Scriptable* /*Sender*/, Targets* parameters, int /*ga_flags*/)
{
	return XthNearestDoor(parameters, 9);
}

//in bg2 it is same as player1 so far
//in iwd2 this is the Gabber!!!
//but also, if there is no gabber, it is the first PC
//probably it is simply the nearest exportable character...
// alt info for iwd2: the protagonist is the selected party member with the left-most portrait instead of gabber (the fallback matches)
Targets* GameScript::Protagonist(const Scriptable* Sender, Targets* parameters, int ga_flags)
{
	parameters->Clear();
	//this sucks but IWD2 is like that...
	static bool charnameisgabber = core->HasFeature(GFFlags::CHARNAMEISGABBER);
	if (!charnameisgabber) {
		parameters->AddTarget(core->GetGame()->GetPC(0, false), 0, ga_flags);
		return parameters;
	}

	const GameControl* gc = core->GetGameControl();
	if (gc) {
		parameters->AddTarget(gc->dialoghandler->GetSpeaker(), 0, ga_flags);
	}
	if (parameters->Count()) {
		return parameters;
	}

	if (core->HasFeature(GFFlags::RULES_3ED)) { // limit to iwd2 for now, unclear if iwd is the same
		// grab the selected pc with the lowest party ID; if needed fully emulate PartySlotN
		const Game* game = core->GetGame();
		int minPID = 100;
		Actor* target = nullptr;
		for (auto& act : game->selected) {
			if (act->InParty < minPID) {
				minPID = act->InParty;
				target = act;
			}
		}
		if (target) {
			parameters->AddTarget(target, Distance(Sender, target), ga_flags);
		} else { // the original didn't handle this
			parameters->AddTarget(core->GetGame()->GetPC(0, false), 0, ga_flags);
		}
	} else {
		// ok, this will return the nearest PC in the first slot
		const Game* game = core->GetGame();
		int i = game->GetPartySize(false);
		while (i--) {
			Actor* target = game->GetPC(i, false);
			parameters->AddTarget(target, Distance(Sender, target), ga_flags);
		}
	}
	return parameters;
}

//last talker
Targets* GameScript::Gabber(const Scriptable* /*Sender*/, Targets* parameters, int ga_flags)
{
	parameters->Clear();
	const GameControl* gc = core->GetGameControl();
	if (gc) {
		parameters->AddTarget(gc->dialoghandler->GetSpeaker(), 0, ga_flags);
	}
	return parameters;
}

Targets* GameScript::LastTrigger(const Scriptable* Sender, Targets* parameters, int ga_flags)
{
	Scriptable* target = parameters->GetTarget(0, ST_ANY);
	parameters->Clear();
	if (target) {
		target = Sender->GetCurrentArea()->GetActorByGlobalID(target->objects.LastTrigger);
		parameters->AddTarget(target, 0, ga_flags);
	} else if (Sender->objects.LastTrigger) {
		target = Sender->GetCurrentArea()->GetActorByGlobalID(Sender->objects.LastTrigger);
		parameters->AddTarget(target, 0, ga_flags);
	}
	return parameters;
}

Targets* GameScript::LastMarkedObject(const Scriptable* Sender, Targets* parameters, int ga_flags)
{
	// not sure if this is even needed, can LastMarkedObject get nested at all?
	const Scriptable* sender = parameters->GetTarget(0, ST_ACTOR);
	if (!sender) {
		sender = Sender;
	}
	parameters->Clear();
	if (sender) {
		Actor* target = sender->GetCurrentArea()->GetActorByGlobalID(sender->objects.LastMarked);
		if (target) {
			parameters->AddTarget(target, 0, ga_flags);
		}
	}
	return parameters;
}

Targets* GameScript::SpellTarget(const Scriptable* Sender, Targets* parameters, int ga_flags)
{
	const Actor* actor = static_cast<Actor*>(parameters->GetTarget(0, ST_ACTOR));
	if (!actor && Sender->Type == ST_ACTOR) {
		actor = static_cast<const Actor*>(Sender);
	}
	parameters->Clear();
	if (actor) {
		Actor* target = actor->GetCurrentArea()->GetActorByGlobalID(actor->objects.LastSpellTarget);
		if (target) {
			parameters->AddTarget(target, 0, ga_flags);
		}
	}
	return parameters;
}

//actions should always use LastMarkedObject, because LastSeen could be deleted
Targets* GameScript::LastSeenBy(const Scriptable* Sender, Targets* parameters, int ga_flags)
{
	const Actor* actor = static_cast<Actor*>(parameters->GetTarget(0, ST_ACTOR));
	if (!actor && Sender->Type == ST_ACTOR) {
		actor = static_cast<const Actor*>(Sender);
	}

	if (actor) {
		Actor* target = actor->GetCurrentArea()->GetActorByGlobalID(actor->objects.LastSeen);
		parameters->Clear();
		if (target) {
			parameters->AddTarget(target, 0, ga_flags);
		}
	} else {
		// try with regions for iwd2
		const InfoPoint* ip = static_cast<InfoPoint*>(parameters->GetTarget(0, ST_PROXIMITY));
		parameters->Clear();
		if (ip) {
			Actor* target = ip->GetCurrentArea()->GetActorByGlobalID(ip->objects.LastSeen);
			if (target) {
				parameters->AddTarget(target, 0, ga_flags);
			}
		}
	}
	return parameters;
}

Targets* GameScript::LastHelp(const Scriptable* Sender, Targets* parameters, int ga_flags)
{
	const Actor* actor = static_cast<Actor*>(parameters->GetTarget(0, ST_ACTOR));
	if (!actor && Sender->Type == ST_ACTOR) {
		actor = static_cast<const Actor*>(Sender);
	}
	parameters->Clear();
	if (actor) {
		Actor* target = actor->GetCurrentArea()->GetActorByGlobalID(actor->objects.LastHelp);
		if (target) {
			parameters->AddTarget(target, 0, ga_flags);
		}
	}
	return parameters;
}

Targets* GameScript::LastHeardBy(const Scriptable* Sender, Targets* parameters, int ga_flags)
{
	const Actor* actor = static_cast<Actor*>(parameters->GetTarget(0, ST_ACTOR));
	if (!actor && Sender->Type == ST_ACTOR) {
		actor = static_cast<const Actor*>(Sender);
	}
	parameters->Clear();
	if (actor) {
		Actor* target = actor->GetCurrentArea()->GetActorByGlobalID(actor->objects.LastHeard);
		if (target) {
			parameters->AddTarget(target, 0, ga_flags);
		}
	}
	return parameters;
}

//i was told that Group means the same specifics, so this is just an
//object selector for everyone with the same specifics as the current object
Targets* GameScript::GroupOf(const Scriptable* Sender, Targets* parameters, int ga_flags)
{
	const Actor* actor = static_cast<Actor*>(parameters->GetTarget(0, ST_ACTOR));
	if (!actor && Sender->Type == ST_ACTOR) {
		actor = static_cast<const Actor*>(Sender);
	}
	parameters->Clear();
	if (actor) {
		ieDword tmp = actor->GetStat(IE_SPECIFIC);
		const Map* cm = Sender->GetCurrentArea();
		int i = cm->GetActorCount(true);
		while (i--) {
			Actor* target = cm->GetActor(i, true);
			if (target && (target->GetStat(IE_SPECIFIC) == tmp)) {
				parameters->AddTarget(target, 0, ga_flags);
			}
		}
	}
	return parameters;
}

/*this one is tough, but done */
Targets* GameScript::ProtectorOf(const Scriptable* Sender, Targets* parameters, int ga_flags)
{
	const Actor* actor = static_cast<Actor*>(parameters->GetTarget(0, ST_ACTOR));
	if (!actor && Sender->Type == ST_ACTOR) {
		actor = static_cast<const Actor*>(Sender);
	}
	parameters->Clear();

	if (actor) {
		Actor* target = actor->GetCurrentArea()->GetActorByGlobalID(actor->objects.LastProtector);
		if (target) {
			parameters->AddTarget(target, 0, ga_flags);
		}
	}
	return parameters;
}

Targets* GameScript::ProtectedBy(const Scriptable* Sender, Targets* parameters, int ga_flags)
{
	const Actor* actor = static_cast<Actor*>(parameters->GetTarget(0, ST_ACTOR));
	if (!actor && Sender->Type == ST_ACTOR) {
		actor = static_cast<const Actor*>(Sender);
	}
	parameters->Clear();
	if (actor) {
		Actor* target = actor->GetCurrentArea()->GetActorByGlobalID(actor->objects.LastProtectee);
		if (target) {
			parameters->AddTarget(target, 0, ga_flags);
		}
	}
	return parameters;
}

Targets* GameScript::LastCommandedBy(const Scriptable* Sender, Targets* parameters, int ga_flags)
{
	const Actor* actor = static_cast<Actor*>(parameters->GetTarget(0, ST_ACTOR));
	if (!actor && Sender->Type == ST_ACTOR) {
		actor = static_cast<const Actor*>(Sender);
	}
	parameters->Clear();
	if (actor) {
		Actor* target = actor->GetCurrentArea()->GetActorByGlobalID(actor->objects.LastCommander);
		if (target) {
			parameters->AddTarget(target, 0, ga_flags);
		}
	}
	return parameters;
}

Targets* GameScript::LastKilled(const Scriptable* Sender, Targets* parameters, int gaFlags)
{
	const Scriptable* actor = parameters->GetTarget(0, ST_ANY);
	if (!actor) {
		actor = Sender;
	}
	parameters->Clear();
	if (actor) {
		Actor* target = actor->GetCurrentArea()->GetActorByGlobalID(actor->objects.LastKilled);
		if (target) {
			parameters->AddTarget(target, 0, gaFlags);
		}
	}
	return parameters;
}

// this is essentially a LastTargetedBy(0) - or MySelf
// but IWD2 defines it
Targets* GameScript::MyTarget(const Scriptable* Sender, Targets* parameters, int ga_flags)
{
	const Scriptable* actor = parameters->GetTarget(0, ST_ANY);
	if (!actor) {
		actor = Sender;
	}
	parameters->Clear();
	if (actor) {
		Actor* target = actor->GetCurrentArea()->GetActorByGlobalID(actor->objects.MyTarget);
		if (target) {
			parameters->AddTarget(target, 0, ga_flags);
		}
	}
	return parameters;
}

Targets* GameScript::LastTargetedBy(const Scriptable* Sender, Targets* parameters, int ga_flags)
{
	const Actor* actor = static_cast<Actor*>(parameters->GetTarget(0, ST_ACTOR));
	return GetMyTarget(Sender, actor, parameters, ga_flags);
}

Targets* GameScript::LastAttackerOf(const Scriptable* Sender, Targets* parameters, int ga_flags)
{
	const Actor* actor = static_cast<Actor*>(parameters->GetTarget(0, ST_ACTOR));
	if (!actor && Sender->Type == ST_ACTOR) {
		actor = static_cast<const Actor*>(Sender);
	}
	parameters->Clear();
	if (actor) {
		Actor* target = actor->GetCurrentArea()->GetActorByGlobalID(actor->objects.LastAttacker);
		if (target) {
			parameters->AddTarget(target, 0, ga_flags);
		}
	}
	return parameters;
}

Targets* GameScript::LastHitter(const Scriptable* Sender, Targets* parameters, int ga_flags)
{
	const Actor* actor = static_cast<Actor*>(parameters->GetTarget(0, ST_ACTOR));
	if (!actor && Sender->Type == ST_ACTOR) {
		actor = static_cast<const Actor*>(Sender);
	}
	parameters->Clear();
	if (actor) {
		Actor* target = actor->GetCurrentArea()->GetActorByGlobalID(actor->objects.LastHitter);
		if (target) {
			parameters->AddTarget(target, 0, ga_flags);
		}
	}
	return parameters;
}

Targets* GameScript::LeaderOf(const Scriptable* Sender, Targets* parameters, int ga_flags)
{
	const Actor* actor = static_cast<Actor*>(parameters->GetTarget(0, ST_ACTOR));
	if (!actor && Sender->Type == ST_ACTOR) {
		actor = static_cast<const Actor*>(Sender);
	}
	parameters->Clear();
	if (actor) {
		Actor* target = actor->GetCurrentArea()->GetActorByGlobalID(actor->objects.LastFollowed);
		if (target) {
			parameters->AddTarget(target, 0, ga_flags);
		}
	}
	return parameters;
}

Targets* GameScript::LastTalkedToBy(const Scriptable* Sender, Targets* parameters, int ga_flags)
{
	const Actor* actor = static_cast<Actor*>(parameters->GetTarget(0, ST_ACTOR));
	if (!actor && Sender->Type == ST_ACTOR) {
		actor = static_cast<const Actor*>(Sender);
	}
	parameters->Clear();
	if (actor) {
		Actor* target = actor->GetCurrentArea()->GetActorByGlobalID(actor->objects.LastTalker);
		if (target) {
			parameters->AddTarget(target, 0, ga_flags);
		}
	}
	return parameters;
}

Targets* GameScript::LastSummonerOf(const Scriptable* Sender, Targets* parameters, int ga_flags)
{
	const Actor* actor = static_cast<Actor*>(parameters->GetTarget(0, ST_ACTOR));
	if (!actor && Sender->Type == ST_ACTOR) {
		actor = static_cast<const Actor*>(Sender);
	}
	parameters->Clear();
	if (actor) {
		Actor* target = actor->GetCurrentArea()->GetActorByGlobalID(actor->objects.LastSummoner);
		if (target) {
			parameters->AddTarget(target, 0, ga_flags);
		}
	}
	return parameters;
}

// PlayerN is usually filled based on join-order
//   if a pc left the party, PlayerN will be invalid for them
//   actors joining the party fill the first available PlayerN, not necessarily max + 1
// PlayerNFill (our InParty) is PlayerN, but just with valid entries
//   On a game load PlayerNFill entries will replace PlayerN ones
// The two are usually the same, so we don't bother with maintaining a sparse list
// PartySlotN is the actual portrait order, which was also set/reset in JoinParty/LeaveParty
inline Targets* PlayerX(Targets* parameters, int ga_flags, unsigned int slot, int mode = 0)
{
	parameters->Clear();
	Actor* pc;
	if (mode == 1) {
		// PlayerNFill
		pc = core->GetGame()->FindPC(slot + 1);
	} else if (mode == 2) {
		// PartySlotN, currently the same as PlayerNFill
		++slot;
		pc = core->GetGame()->FindPC(slot);
	} else {
		// PlayerN
		pc = core->GetGame()->GetPC(slot, false);
	}
	parameters->AddTarget(pc, 0, ga_flags);
	return parameters;
}

Targets* GameScript::Player1(const Scriptable* /*Sender*/, Targets* parameters, int ga_flags)
{
	return PlayerX(parameters, ga_flags, 0);
}

Targets* GameScript::Player1Fill(const Scriptable* /*Sender*/, Targets* parameters, int ga_flags)
{
	return PlayerX(parameters, ga_flags, 0, 1);
}

Targets* GameScript::Player2(const Scriptable* /*Sender*/, Targets* parameters, int ga_flags)
{
	return PlayerX(parameters, ga_flags, 1);
}

Targets* GameScript::Player2Fill(const Scriptable* /*Sender*/, Targets* parameters, int ga_flags)
{
	return PlayerX(parameters, ga_flags, 1, 1);
}

Targets* GameScript::Player3(const Scriptable* /*Sender*/, Targets* parameters, int ga_flags)
{
	return PlayerX(parameters, ga_flags, 2);
}

Targets* GameScript::Player3Fill(const Scriptable* /*Sender*/, Targets* parameters, int ga_flags)
{
	return PlayerX(parameters, ga_flags, 2, 1);
}

Targets* GameScript::Player4(const Scriptable* /*Sender*/, Targets* parameters, int ga_flags)
{
	return PlayerX(parameters, ga_flags, 3);
}

Targets* GameScript::Player4Fill(const Scriptable* /*Sender*/, Targets* parameters, int ga_flags)
{
	return PlayerX(parameters, ga_flags, 3, 1);
}

Targets* GameScript::Player5(const Scriptable* /*Sender*/, Targets* parameters, int ga_flags)
{
	return PlayerX(parameters, ga_flags, 4);
}

Targets* GameScript::Player5Fill(const Scriptable* /*Sender*/, Targets* parameters, int ga_flags)
{
	return PlayerX(parameters, ga_flags, 4, 1);
}

Targets* GameScript::Player6(const Scriptable* /*Sender*/, Targets* parameters, int ga_flags)
{
	return PlayerX(parameters, ga_flags, 5);
}

Targets* GameScript::Player6Fill(const Scriptable* /*Sender*/, Targets* parameters, int ga_flags)
{
	return PlayerX(parameters, ga_flags, 5, 1);
}

Targets* GameScript::Player7(const Scriptable* /*Sender*/, Targets* parameters, int ga_flags)
{
	return PlayerX(parameters, ga_flags, 6);
}

Targets* GameScript::Player7Fill(const Scriptable* /*Sender*/, Targets* parameters, int ga_flags)
{
	return PlayerX(parameters, ga_flags, 6, 1);
}

Targets* GameScript::Player8(const Scriptable* /*Sender*/, Targets* parameters, int ga_flags)
{
	return PlayerX(parameters, ga_flags, 7);
}

Targets* GameScript::Player8Fill(const Scriptable* /*Sender*/, Targets* parameters, int ga_flags)
{
	return PlayerX(parameters, ga_flags, 7, 1);
}

Targets* GameScript::Player9(const Scriptable* /*Sender*/, Targets* parameters, int ga_flags)
{
	return PlayerX(parameters, ga_flags, 8);
}

Targets* GameScript::Player9Fill(const Scriptable* /*Sender*/, Targets* parameters, int ga_flags)
{
	return PlayerX(parameters, ga_flags, 8, 1);
}

Targets* GameScript::Player10(const Scriptable* /*Sender*/, Targets* parameters, int ga_flags)
{
	return PlayerX(parameters, ga_flags, 9);
}

Targets* GameScript::Player10Fill(const Scriptable* /*Sender*/, Targets* parameters, int ga_flags)
{
	return PlayerX(parameters, ga_flags, 9, 1);
}

Targets* GameScript::PartySlot1(const Scriptable* /*Sender*/, Targets* parameters, int gaFlags)
{
	return PlayerX(parameters, gaFlags, 0, 2);
}

Targets* GameScript::PartySlot2(const Scriptable* /*Sender*/, Targets* parameters, int gaFlags)
{
	return PlayerX(parameters, gaFlags, 1, 2);
}

Targets* GameScript::PartySlot3(const Scriptable* /*Sender*/, Targets* parameters, int gaFlags)
{
	return PlayerX(parameters, gaFlags, 2, 2);
}

Targets* GameScript::PartySlot4(const Scriptable* /*Sender*/, Targets* parameters, int gaFlags)
{
	return PlayerX(parameters, gaFlags, 3, 2);
}

Targets* GameScript::PartySlot5(const Scriptable* /*Sender*/, Targets* parameters, int gaFlags)
{
	return PlayerX(parameters, gaFlags, 4, 2);
}

Targets* GameScript::PartySlot6(const Scriptable* /*Sender*/, Targets* parameters, int gaFlags)
{
	return PlayerX(parameters, gaFlags, 5, 2);
}

Targets* GameScript::PartySlot7(const Scriptable* /*Sender*/, Targets* parameters, int gaFlags)
{
	return PlayerX(parameters, gaFlags, 6, 2);
}

Targets* GameScript::PartySlot8(const Scriptable* /*Sender*/, Targets* parameters, int gaFlags)
{
	return PlayerX(parameters, gaFlags, 7, 2);
}

Targets* GameScript::PartySlot9(const Scriptable* /*Sender*/, Targets* parameters, int gaFlags)
{
	return PlayerX(parameters, gaFlags, 8, 2);
}

Targets* GameScript::PartySlot10(const Scriptable* /*Sender*/, Targets* parameters, int gaFlags)
{
	return PlayerX(parameters, gaFlags, 9, 2);
}

//This filter works only on the Party - silly restriction, but the dataset expects this
Targets* GameScript::StrongestOfMale(const Scriptable* Sender, Targets* parameters, int ga_flags)
{
	const Map* area = Sender->GetCurrentArea();
	const Game* game = core->GetGame();
	Scriptable* scr = NULL;
	int besthp = 0;
	int i = game->GetPartySize(false);
	while (i--) {
		Actor* actor = game->GetPC(i, false);
		if (actor->GetStat(IE_SEX) != SEX_MALE) continue;
		if (actor->GetCurrentArea() == area) {
			int hp = actor->GetStat(IE_HITPOINTS);
			if (!scr || besthp < hp) {
				besthp = hp;
				scr = actor;
			}
		}
	}
	parameters->Clear();
	parameters->AddTarget(scr, 0, ga_flags);
	return parameters;
}

//This filter works only on the Party - silly restriction, but the dataset expects this
Targets* GameScript::StrongestOf(const Scriptable* Sender, Targets* parameters, int ga_flags)
{
	const Map* area = Sender->GetCurrentArea();
	const Game* game = core->GetGame();
	Scriptable* scr = NULL;
	int besthp = 0;
	int i = game->GetPartySize(false);
	while (i--) {
		Actor* actor = game->GetPC(i, false);
		if (actor->GetCurrentArea() == area) {
			int hp = actor->GetStat(IE_HITPOINTS);
			if (!scr || besthp < hp) {
				besthp = hp;
				scr = actor;
			}
		}
	}
	parameters->Clear();
	parameters->AddTarget(scr, 0, ga_flags);
	return parameters;
}

//This filter works only on the Party - silly restriction, but the dataset expects this
Targets* GameScript::WeakestOf(const Scriptable* Sender, Targets* parameters, int ga_flags)
{
	const Map* area = Sender->GetCurrentArea();
	const Game* game = core->GetGame();
	Scriptable* scr = NULL;
	int worsthp = 0;
	int i = game->GetPartySize(false);
	while (i--) {
		Actor* actor = game->GetPC(i, false);
		if (actor->GetCurrentArea() == area) {
			int hp = actor->GetStat(IE_HITPOINTS);
			if (!scr || worsthp > hp) {
				worsthp = hp;
				scr = actor;
			}
		}
	}
	parameters->Clear();
	parameters->AddTarget(scr, 0, ga_flags);
	return parameters;
}

//This filter works only on the Party - silly restriction, but the dataset expects this
Targets* GameScript::BestAC(const Scriptable* Sender, Targets* parameters, int ga_flags)
{
	const Map* area = Sender->GetCurrentArea();
	const Game* game = core->GetGame();
	Scriptable* scr = NULL;
	int bestac = 0;
	int i = game->GetPartySize(false);
	while (i--) {
		Actor* actor = game->GetPC(i, false);
		if (actor->GetCurrentArea() == area) {
			int ac = actor->AC.GetTotal();
			if (!scr || bestac > ac) {
				bestac = ac;
				scr = actor;
			}
		}
	}
	parameters->Clear();
	parameters->AddTarget(scr, 0, ga_flags);
	return parameters;
}

//This filter works only on the Party - silly restriction, but the dataset expects this
Targets* GameScript::WorstAC(const Scriptable* Sender, Targets* parameters, int ga_flags)
{
	const Map* area = Sender->GetCurrentArea();
	const Game* game = core->GetGame();
	Scriptable* scr = NULL;
	int worstac = 0;
	int i = game->GetPartySize(false);
	while (i--) {
		Actor* actor = game->GetPC(i, false);
		if (actor->GetCurrentArea() == area) {
			int ac = actor->AC.GetTotal();
			if (!scr || worstac < ac) {
				worstac = ac;
				scr = actor;
			}
		}
	}
	parameters->Clear();
	parameters->AddTarget(scr, 0, ga_flags);
	return parameters;
}

//This filter works only on the Party - silly restriction, but the dataset expects this
Targets* GameScript::MostDamagedOf(const Scriptable* Sender, Targets* parameters, int ga_flags)
{
	const Map* area = Sender->GetCurrentArea();
	const Game* game = core->GetGame();
	Scriptable* scr = NULL;
	int worsthp = 0;
	int i = game->GetPartySize(false);
	while (i--) {
		Actor* actor = game->GetPC(i, false);
		if (actor->GetCurrentArea() == area) {
			int hp = actor->GetStat(IE_MAXHITPOINTS) - actor->GetBase(IE_HITPOINTS);
			if (!scr || hp > worsthp) {
				worsthp = hp;
				scr = actor;
			}
		}
	}
	parameters->Clear();
	parameters->AddTarget(scr, 0, ga_flags);
	return parameters;
}

//This filter works only on the Party - silly restriction, but the dataset expects this
//For example the beholder01 script
Targets* GameScript::LeastDamagedOf(const Scriptable* Sender, Targets* parameters, int ga_flags)
{
	const Map* area = Sender->GetCurrentArea();
	const Game* game = core->GetGame();
	Scriptable* scr = NULL;
	int besthp = 0;
	int i = game->GetPartySize(false);
	while (i--) {
		Actor* actor = game->GetPC(i, false);
		if (actor->GetCurrentArea() == area) {
			int hp = actor->GetStat(IE_MAXHITPOINTS) - actor->GetBase(IE_HITPOINTS);
			if (!scr || besthp < hp) {
				besthp = hp;
				scr = actor;
			}
		}
	}
	parameters->Clear();
	parameters->AddTarget(scr, 0, ga_flags);
	return parameters;
}

Targets* GameScript::Farthest(const Scriptable* /*Sender*/, Targets* parameters, int ga_flags)
{
	const targettype* t = parameters->GetLastTarget(ST_ACTOR);
	parameters->Clear();
	if (t) {
		parameters->AddTarget(t->actor, 0, ga_flags);
	}
	return parameters;
}

Targets* GameScript::FarthestEnemyOf(const Scriptable* /*Sender*/, Targets* parameters, int ga_flags)
{
	return XthNearestEnemyOf(parameters, -1, ga_flags);
}

Targets* GameScript::SecondFarthestEnemyOf(const Scriptable* /*Sender*/, Targets* parameters, int gaFlags)
{
	return XthNearestEnemyOf(parameters, 1, gaFlags, true);
}

Targets* GameScript::ThirdFarthestEnemyOf(const Scriptable* /*Sender*/, Targets* parameters, int gaFlags)
{
	return XthNearestEnemyOf(parameters, 2, gaFlags, true);
}

Targets* GameScript::FourthFarthestEnemyOf(const Scriptable* /*Sender*/, Targets* parameters, int gaFlags)
{
	return XthNearestEnemyOf(parameters, 3, gaFlags, true);
}

Targets* GameScript::FifthFarthestEnemyOf(const Scriptable* /*Sender*/, Targets* parameters, int gaFlags)
{
	return XthNearestEnemyOf(parameters, 4, gaFlags, true);
}

Targets* GameScript::SixthFarthestEnemyOf(const Scriptable* /*Sender*/, Targets* parameters, int gaFlags)
{
	return XthNearestEnemyOf(parameters, 5, gaFlags, true);
}

Targets* GameScript::SeventhFarthestEnemyOf(const Scriptable* /*Sender*/, Targets* parameters, int gaFlags)
{
	return XthNearestEnemyOf(parameters, 6, gaFlags, true);
}

Targets* GameScript::EighthFarthestEnemyOf(const Scriptable* /*Sender*/, Targets* parameters, int gaFlags)
{
	return XthNearestEnemyOf(parameters, 7, gaFlags, true);
}

Targets* GameScript::NinthFarthestEnemyOf(const Scriptable* /*Sender*/, Targets* parameters, int gaFlags)
{
	return XthNearestEnemyOf(parameters, 8, gaFlags, true);
}

Targets* GameScript::TenthFarthestEnemyOf(const Scriptable* /*Sender*/, Targets* parameters, int gaFlags)
{
	return XthNearestEnemyOf(parameters, 9, gaFlags, true);
}

Targets* GameScript::NearestEnemyOf(const Scriptable* /*Sender*/, Targets* parameters, int ga_flags)
{
	return XthNearestEnemyOf(parameters, 0, ga_flags);
}

Targets* GameScript::SecondNearestEnemyOf(const Scriptable* /*Sender*/, Targets* parameters, int ga_flags)
{
	return XthNearestEnemyOf(parameters, 1, ga_flags);
}

Targets* GameScript::ThirdNearestEnemyOf(const Scriptable* /*Sender*/, Targets* parameters, int ga_flags)
{
	return XthNearestEnemyOf(parameters, 2, ga_flags);
}

Targets* GameScript::FourthNearestEnemyOf(const Scriptable* /*Sender*/, Targets* parameters, int ga_flags)
{
	return XthNearestEnemyOf(parameters, 3, ga_flags);
}

Targets* GameScript::FifthNearestEnemyOf(const Scriptable* /*Sender*/, Targets* parameters, int ga_flags)
{
	return XthNearestEnemyOf(parameters, 4, ga_flags);
}

Targets* GameScript::SixthNearestEnemyOf(const Scriptable* /*Sender*/, Targets* parameters, int ga_flags)
{
	return XthNearestEnemyOf(parameters, 5, ga_flags);
}

Targets* GameScript::SeventhNearestEnemyOf(const Scriptable* /*Sender*/, Targets* parameters, int ga_flags)
{
	return XthNearestEnemyOf(parameters, 6, ga_flags);
}

Targets* GameScript::EighthNearestEnemyOf(const Scriptable* /*Sender*/, Targets* parameters, int ga_flags)
{
	return XthNearestEnemyOf(parameters, 7, ga_flags);
}

Targets* GameScript::NinthNearestEnemyOf(const Scriptable* /*Sender*/, Targets* parameters, int ga_flags)
{
	return XthNearestEnemyOf(parameters, 8, ga_flags);
}

Targets* GameScript::TenthNearestEnemyOf(const Scriptable* /*Sender*/, Targets* parameters, int ga_flags)
{
	return XthNearestEnemyOf(parameters, 9, ga_flags);
}

Targets* GameScript::NearestEnemySummoned(const Scriptable* Sender, Targets* parameters, int ga_flags)
{
	return ClosestEnemySummoned(Sender, parameters, ga_flags);
}

Targets* GameScript::NearestEnemyOfType(const Scriptable* Sender, Targets* parameters, int ga_flags)
{
	return XthNearestEnemyOfType(Sender, parameters, 0, ga_flags);
}

Targets* GameScript::SecondNearestEnemyOfType(const Scriptable* Sender, Targets* parameters, int ga_flags)
{
	return XthNearestEnemyOfType(Sender, parameters, 1, ga_flags);
}

Targets* GameScript::ThirdNearestEnemyOfType(const Scriptable* Sender, Targets* parameters, int ga_flags)
{
	return XthNearestEnemyOfType(Sender, parameters, 2, ga_flags);
}

Targets* GameScript::FourthNearestEnemyOfType(const Scriptable* Sender, Targets* parameters, int ga_flags)
{
	return XthNearestEnemyOfType(Sender, parameters, 3, ga_flags);
}

Targets* GameScript::FifthNearestEnemyOfType(const Scriptable* Sender, Targets* parameters, int ga_flags)
{
	return XthNearestEnemyOfType(Sender, parameters, 4, ga_flags);
}

Targets* GameScript::SixthNearestEnemyOfType(const Scriptable* Sender, Targets* parameters, int ga_flags)
{
	return XthNearestEnemyOfType(Sender, parameters, 5, ga_flags);
}

Targets* GameScript::SeventhNearestEnemyOfType(const Scriptable* Sender, Targets* parameters, int ga_flags)
{
	return XthNearestEnemyOfType(Sender, parameters, 6, ga_flags);
}

Targets* GameScript::EighthNearestEnemyOfType(const Scriptable* Sender, Targets* parameters, int ga_flags)
{
	return XthNearestEnemyOfType(Sender, parameters, 7, ga_flags);
}

Targets* GameScript::NinthNearestEnemyOfType(const Scriptable* Sender, Targets* parameters, int ga_flags)
{
	return XthNearestEnemyOfType(Sender, parameters, 8, ga_flags);
}

Targets* GameScript::TenthNearestEnemyOfType(const Scriptable* Sender, Targets* parameters, int ga_flags)
{
	return XthNearestEnemyOfType(Sender, parameters, 9, ga_flags);
}

Targets* GameScript::NearestMyGroupOfType(const Scriptable* Sender, Targets* parameters, int ga_flags)
{
	return XthNearestMyGroupOfType(Sender, parameters, 0, ga_flags);
}

Targets* GameScript::SecondNearestMyGroupOfType(const Scriptable* Sender, Targets* parameters, int ga_flags)
{
	return XthNearestMyGroupOfType(Sender, parameters, 1, ga_flags);
}

Targets* GameScript::ThirdNearestMyGroupOfType(const Scriptable* Sender, Targets* parameters, int ga_flags)
{
	return XthNearestMyGroupOfType(Sender, parameters, 2, ga_flags);
}

Targets* GameScript::FourthNearestMyGroupOfType(const Scriptable* Sender, Targets* parameters, int ga_flags)
{
	return XthNearestMyGroupOfType(Sender, parameters, 3, ga_flags);
}

Targets* GameScript::FifthNearestMyGroupOfType(const Scriptable* Sender, Targets* parameters, int ga_flags)
{
	return XthNearestMyGroupOfType(Sender, parameters, 4, ga_flags);
}

Targets* GameScript::SixthNearestMyGroupOfType(const Scriptable* Sender, Targets* parameters, int ga_flags)
{
	return XthNearestMyGroupOfType(Sender, parameters, 5, ga_flags);
}

Targets* GameScript::SeventhNearestMyGroupOfType(const Scriptable* Sender, Targets* parameters, int ga_flags)
{
	return XthNearestMyGroupOfType(Sender, parameters, 6, ga_flags);
}

Targets* GameScript::EighthNearestMyGroupOfType(const Scriptable* Sender, Targets* parameters, int ga_flags)
{
	return XthNearestMyGroupOfType(Sender, parameters, 7, ga_flags);
}

Targets* GameScript::NinthNearestMyGroupOfType(const Scriptable* Sender, Targets* parameters, int ga_flags)
{
	return XthNearestMyGroupOfType(Sender, parameters, 8, ga_flags);
}

Targets* GameScript::TenthNearestMyGroupOfType(const Scriptable* Sender, Targets* parameters, int ga_flags)
{
	return XthNearestMyGroupOfType(Sender, parameters, 9, ga_flags);
}

/* returns only living PC's? if not, alter getpartysize/getpc flag*/
Targets* GameScript::NearestPC(const Scriptable* Sender, Targets* parameters, int ga_flags)
{
	parameters->Clear();
	const Map* map = Sender->GetCurrentArea();
	const Game* game = core->GetGame();
	int i = game->GetPartySize(true);
	int mindist = -1;
	Actor* ac = NULL;
	while (i--) {
		Actor* newactor = game->GetPC(i, true);
		//NearestPC for PC's will not give themselves as a result
		//this might be different from the original engine
		if (Sender->Type == ST_ACTOR && (newactor == (const Actor*) Sender)) {
			continue;
		}
		if (newactor->GetCurrentArea() != map) {
			continue;
		}
		int dist = Distance(Sender, newactor);
		if ((mindist == -1) || (dist < mindist)) {
			ac = newactor;
			mindist = dist;
		}
	}
	if (ac) {
		parameters->AddTarget(ac, 0, ga_flags);
	}
	return parameters;
}

Targets* GameScript::Nearest(const Scriptable* /*Sender*/, Targets* parameters, int ga_flags)
{
	return XthNearestOf(parameters, 0, ga_flags);
}

Targets* GameScript::SecondNearest(const Scriptable* /*Sender*/, Targets* parameters, int ga_flags)
{
	return XthNearestOf(parameters, 1, ga_flags);
}

Targets* GameScript::ThirdNearest(const Scriptable* /*Sender*/, Targets* parameters, int ga_flags)
{
	return XthNearestOf(parameters, 2, ga_flags);
}

Targets* GameScript::FourthNearest(const Scriptable* /*Sender*/, Targets* parameters, int ga_flags)
{
	return XthNearestOf(parameters, 3, ga_flags);
}

Targets* GameScript::FifthNearest(const Scriptable* /*Sender*/, Targets* parameters, int ga_flags)
{
	return XthNearestOf(parameters, 4, ga_flags);
}

Targets* GameScript::SixthNearest(const Scriptable* /*Sender*/, Targets* parameters, int ga_flags)
{
	return XthNearestOf(parameters, 5, ga_flags);
}

Targets* GameScript::SeventhNearest(const Scriptable* /*Sender*/, Targets* parameters, int ga_flags)
{
	return XthNearestOf(parameters, 6, ga_flags);
}

Targets* GameScript::EighthNearest(const Scriptable* /*Sender*/, Targets* parameters, int ga_flags)
{
	return XthNearestOf(parameters, 7, ga_flags);
}

Targets* GameScript::NinthNearest(const Scriptable* /*Sender*/, Targets* parameters, int ga_flags)
{
	return XthNearestOf(parameters, 8, ga_flags);
}

Targets* GameScript::TenthNearest(const Scriptable* /*Sender*/, Targets* parameters, int ga_flags)
{
	return XthNearestOf(parameters, 9, ga_flags);
}

// IESDP says they have to have identical EA, which is quite restrictive, so we allow all below EA_GOODCUTOFF
// for a start, before testing, we only support party-friendly runners
// generalize XthNearestMyGroupOfType and use it here with IE_EA if it turns out this is wrong
Targets* GameScript::NearestAllyOf(const Scriptable* /*Sender*/, Targets* parameters, int gaFlags)
{
	return XthNearestOf(parameters, 0, gaFlags | GA_NO_ENEMY | GA_NO_NEUTRAL);
}

Targets* GameScript::SecondNearestAllyOf(const Scriptable* /*Sender*/, Targets* parameters, int gaFlags)
{
	return XthNearestOf(parameters, 1, gaFlags | GA_NO_ENEMY | GA_NO_NEUTRAL);
}

Targets* GameScript::ThirdNearestAllyOf(const Scriptable* /*Sender*/, Targets* parameters, int gaFlags)
{
	return XthNearestOf(parameters, 2, gaFlags | GA_NO_ENEMY | GA_NO_NEUTRAL);
}

Targets* GameScript::FourthNearestAllyOf(const Scriptable* /*Sender*/, Targets* parameters, int gaFlags)
{
	return XthNearestOf(parameters, 3, gaFlags | GA_NO_ENEMY | GA_NO_NEUTRAL);
}

Targets* GameScript::FifthNearestAllyOf(const Scriptable* /*Sender*/, Targets* parameters, int gaFlags)
{
	return XthNearestOf(parameters, 4, gaFlags | GA_NO_ENEMY | GA_NO_NEUTRAL);
}

Targets* GameScript::SixthNearestAllyOf(const Scriptable* /*Sender*/, Targets* parameters, int gaFlags)
{
	return XthNearestOf(parameters, 5, gaFlags | GA_NO_ENEMY | GA_NO_NEUTRAL);
}

Targets* GameScript::SeventhNearestAllyOf(const Scriptable* /*Sender*/, Targets* parameters, int gaFlags)
{
	return XthNearestOf(parameters, 6, gaFlags | GA_NO_ENEMY | GA_NO_NEUTRAL);
}

Targets* GameScript::EighthNearestAllyOf(const Scriptable* /*Sender*/, Targets* parameters, int gaFlags)
{
	return XthNearestOf(parameters, 7, gaFlags | GA_NO_ENEMY | GA_NO_NEUTRAL);
}

Targets* GameScript::NinthNearestAllyOf(const Scriptable* /*Sender*/, Targets* parameters, int gaFlags)
{
	return XthNearestOf(parameters, 8, gaFlags | GA_NO_ENEMY | GA_NO_NEUTRAL);
}

Targets* GameScript::TenthNearestAllyOf(const Scriptable* /*Sender*/, Targets* parameters, int gaFlags)
{
	return XthNearestOf(parameters, 9, gaFlags | GA_NO_ENEMY | GA_NO_NEUTRAL);
}

Targets* GameScript::SelectedCharacter(const Scriptable* Sender, Targets* parameters, int ga_flags)
{
	const Map* cm = Sender->GetCurrentArea();
	parameters->Clear();
	int i = cm->GetActorCount(true);
	while (i--) {
		Actor* ac = cm->GetActor(i, true);
		if (ac->GetCurrentArea() != cm) {
			continue;
		}
		if (ac->IsSelected()) {
			parameters->AddTarget(ac, Distance(Sender, ac), ga_flags);
		}
	}
	return parameters;
}

Targets* GameScript::Familiar(const Scriptable* Sender, Targets* parameters, int ga_flags)
{
	const Map* cm = Sender->GetCurrentArea();
	parameters->Clear();
	int i = cm->GetActorCount(true);
	while (i--) {
		Actor* ac = cm->GetActor(i, true);
		if (ac->GetCurrentArea() != cm) continue;
		if (ac->GetStat(IE_EA) == EA_FAMILIAR) { // if this turns out to not be enough, check for the FamiliarMarker effect
			parameters->AddTarget(ac, Distance(Sender, ac), ga_flags);
		}
	}
	return parameters;
}

// the original assumed there can only be one here as well
// in EEs returns the pc matching game->FamiliarOwner (set in fx_familiar_marker) if the familiar list is non-empty
Targets* GameScript::FamiliarSummoner(const Scriptable* Sender, Targets* parameters, int ga_flags)
{
	static EffectRef fx_familiar_marker_ref = { "FamiliarMarker", -1 };
	const Map* cm = Sender->GetCurrentArea();
	parameters->Clear();
	int i = cm->GetActorCount(true);
	while (i--) {
		Actor* ac = cm->GetActor(i, true);
		if (ac->GetCurrentArea() != cm) continue;
		if (ac->fxqueue.HasEffect(fx_familiar_marker_ref)) {
			parameters->AddTarget(ac, Distance(Sender, ac), ga_flags);
		}
	}
	return parameters;
}

Targets* GameScript::Nothing(const Scriptable* /*Sender*/, Targets* parameters, int /*ga_flags*/)
{
	parameters->Clear();
	return parameters;
}

//-------------------------------------------------------------
// IDS Functions
//-------------------------------------------------------------

int GameScript::ID_Alignment(const Actor* actor, int parameter)
{
	int value = actor->GetStat(IE_ALIGNMENT);
	int a = parameter & 15;
	if (a && a != (value & 15)) {
		return 0;
	}
	a = parameter & 240;
	if (a && a != (value & 240)) {
		return 0;
	}
	return 1;
}

int GameScript::ID_Allegiance(const Actor* actor, int parameter)
{
	int value = actor->GetStat(IE_EA);
	switch (parameter) {
		case EA_GOODCUTOFF:
			return value <= EA_GOODCUTOFF;

		case EA_NOTGOOD:
			return value >= EA_NOTGOOD;

		case EA_NOTNEUTRAL:
			return value >= EA_EVILCUTOFF || value <= EA_GOODCUTOFF;

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
inline bool idclass(const Actor* actor, int parameter, bool iwd2)
{
	int value = 0;
	if (parameter < 202 || parameter > 209) {
		value = actor->GetActiveClass();
		return parameter == value;
	}

	const int* classes;
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
		// DRUID_ALL, shaman is a guess
		value = actor->GetDruidLevel() + actor->GetShamanLevel();
	} else if (parameter == classes[6]) {
		// RANGER_ALL
		value = actor->GetRangerLevel();
	}
	return value > 0;
}

int GameScript::ID_Class(const Actor* actor, int parameter)
{
	if (core->HasFeature(GFFlags::RULES_3ED)) {
		//iwd2 has different values, see also the note for AVClass
		return idclass(actor, parameter, true);
	}
	return idclass(actor, parameter, false);
}

// IE_CLASS holds only one class, not a bitmask like with iwd2 kits. The ids values
// are friendly to binary comparison, so we just need to build such a class value
int GameScript::ID_ClassMask(const Actor* actor, int parameter)
{
	// maybe we're lucky...
	int value = actor->GetActiveClass();
	if (parameter & (1 << (value - 1))) return 1;

	// otherwise iterate over all the classes
	value = actor->GetClassMask();

	if (parameter & value) return 1;
	return 0;
}

// Defunct object selector in IWD2. AVCLASS.IDS doesn't exist, and the engine
// doesn't do anything with the value other than read / write it to sprites.
int GameScript::ID_AVClass(const Actor*, int)
{
	return true;
}

int GameScript::ID_Race(const Actor* actor, int parameter)
{
	int value = actor->GetStat(IE_RACE);
	return parameter == value;
}

int GameScript::ID_Subrace(const Actor* actor, int parameter)
{
	int value = actor->GetStat(IE_SUBRACE);
	return parameter == value;
}

int GameScript::ID_Faction(const Actor* actor, int parameter)
{
	int value = actor->GetStat(IE_FACTION);
	return parameter == value;
}

int GameScript::ID_Team(const Actor* actor, int parameter)
{
	int value = actor->GetStat(IE_TEAM);
	return parameter == value;
}

int GameScript::ID_Gender(const Actor* actor, int parameter)
{
	int value = actor->GetStat(IE_SEX);
	return parameter == value;
}

int GameScript::ID_General(const Actor* actor, int parameter)
{
	int value = actor->GetStat(IE_GENERAL);
	return parameter == value;
}

int GameScript::ID_Specific(const Actor* actor, int parameter)
{
	int value = actor->GetStat(IE_SPECIFIC);
	return parameter == value;
}

}
