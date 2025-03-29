/* GemRB - Infinity Engine Emulator
* Copyright (C) 2024 The GemRB Project
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 2
* of the License, or (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*
*/

#include "Highlightable.h"

#include "defsounds.h"
#include "ie_stats.h"

#include "Interface.h"

#include "GUI/GameControl.h"
#include "GameScript/GSUtils.h"

namespace GemRB {

bool Highlightable::IsOver(const Point& place) const
{
	if (!outline) {
		return false;
	}
	return outline->PointIn(place);
}

void Highlightable::DrawOutline(Point origin) const
{
	if (!outline) {
		return;
	}
	origin = outline->BBox.origin - origin;

	bool highlightOutlineOnly = core->HasFeature(GFFlags::HIGHLIGHT_OUTLINE_ONLY);
	bool pstStateFlags = core->HasFeature(GFFlags::PST_STATE_FLAGS);

	if (!highlightOutlineOnly) {
		BlitFlags flag = BlitFlags::HALFTRANS | (pstStateFlags ? BlitFlags::MOD : BlitFlags::BLENDED);

		VideoDriver->DrawPolygon(outline.get(), origin, outlineColor, true, flag);
	}

	if (highlightOutlineOnly || !pstStateFlags) {
		VideoDriver->DrawPolygon(outline.get(), origin, outlineColor, false);
	}
}

void Highlightable::SetCursor(unsigned char cursorIndex)
{
	Cursor = cursorIndex;
}

// set some triggers so trap scripts can deliver their payload
bool Highlightable::TriggerTrap(int /*skill*/, ieDword ID)
{
	if (!Trapped) {
		return false;
	}
	// actually this could be script name[0]
	if (!Scripts[0] && EnterWav.IsEmpty()) {
		return false;
	}

	// the second part is a hack to deal with bg2's ar1401 lava floor trap ("muck"), which doesn't have the repeating bit set
	// just testing TrapDetectionDiff/TrapRemovalDiff is not good enough as DeadThiefProx in the initial bg2 chamber exit is
	// almost identical and would retrigger instead of being a one-off
	// at the same time iwd2 Saablic_Greeting in ar6200 shouldn't repeat, while being practically identical
	// should we always send Entered instead, also when !Trapped? Does not appear so, see history of InfoPoint::TriggerTrap
	if (TrapResets()) {
		AddTrigger(TriggerEntry(trigger_reset, GetGlobalID()));
	} else if (core->HasFeature(GFFlags::RULES_3ED) || scriptName != "muck") {
		Trapped = false;
	}

	// add these two last, so LastTrigger points to the trap triggerer (test case: iwd2 ar6100 walkway traps)
	AddTrigger(TriggerEntry(trigger_entered, ID));
	AddTrigger(TriggerEntry(trigger_traptriggered, ID)); // for that one user in bg2

	return true;
}

bool Highlightable::TryUnlock(Actor* actor, bool removeKey) const
{
	if (KeyResRef.IsEmpty()) {
		return false;
	}

	Actor* haskey = nullptr;
	if (actor->InParty) {
		const Game* game = core->GetGame();
		// allow unlock when the key is on any partymember
		for (int idx = 0; idx < game->GetPartySize(false); idx++) {
			Actor* pc = game->FindPC(idx + 1);
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

	if (!removeKey) return true;

	CREItem* item = nullptr;
	int result = haskey->inventory.RemoveItem(KeyResRef, 0, &item);
	// check also in bags if nothing was found
	if (result == -1) {
		int i = haskey->inventory.GetSlotCount();
		while (i--) {
			// maybe we could speed this up if we mark bag items with a flags bit
			const CREItem* itemSlot = haskey->inventory.GetSlotItem(i);
			if (!itemSlot) continue;
			const Item* itemStore = gamedata->GetItem(itemSlot->ItemResRef);
			if (!itemStore) continue;
			if (core->CheckItemType(itemStore, SLOT_BAG)) {
				// the store is the same as the item's name
				RemoveStoreItem(itemSlot->ItemResRef, KeyResRef);
			}
			gamedata->FreeItem(itemStore, itemSlot->ItemResRef);
		}
	}
	// the item should always be existing!!!
	delete item;

	return true;
}

bool Highlightable::TryBashLock(Actor* actor, ieWord lockDifficulty, HCStrings failStr)
{
	// Get the strength bonus against lock difficulty
	int bonus;
	unsigned int roll;
	ieWord adjLockDiff = lockDifficulty;

	if (core->HasFeature(GFFlags::RULES_3ED)) {
		bonus = actor->GetAbilityBonus(IE_STR);
		roll = RAND(1, 20) + bonus;
		adjLockDiff = lockDifficulty / 6 + 9;
	} else {
		int str = actor->GetStat(IE_STR);
		int strEx = actor->GetStat(IE_STREXTRA);
		bonus = core->GetStrengthBonus(2, str, strEx); // BEND_BARS_LIFT_GATES
		roll = actor->LuckyRoll(1, 10, bonus, 0);
	}

	actor->FaceTarget(this);
	if (core->HasFeature(GFFlags::RULES_3ED)) {
		// ~Bash door check. Roll %d + %d Str mod > %d door DC.~
		// there is no separate string for non-doors
		displaymsg->DisplayRollStringName(ieStrRef::ROLL1, GUIColors::LIGHTGREY, actor, roll, bonus, adjLockDiff);
	}

	if (roll < adjLockDiff || lockDifficulty == 100) {
		displaymsg->DisplayMsgAtLocation(failStr, FT_ANY, actor, actor, GUIColors::XPCHANGE);
		return false;
	}

	// This is ok, bashdoor also sends the unlocked trigger
	AddTrigger(TriggerEntry(trigger_unlocked, actor->GetGlobalID()));
	ImmediateEvent();
	core->GetGameControl()->ResetTargetMode();
	return true;
}

bool Highlightable::TryPickLock(Actor* actor, ieWord lockDifficulty, ieStrRef customFailStr, HCStrings failStr)
{
	if (lockDifficulty == 100) {
		if (customFailStr != ieStrRef::INVALID) {
			displaymsg->DisplayStringName(customFailStr, GUIColors::XPCHANGE, actor, STRING_FLAGS::SOUND | STRING_FLAGS::SPEECH);
		} else {
			displaymsg->DisplayMsgAtLocation(failStr, FT_ANY, actor, actor, GUIColors::XPCHANGE);
		}
		return false;
	}

	int stat = actor->GetStat(IE_LOCKPICKING);
	if (core->HasFeature(GFFlags::RULES_3ED)) {
		int skill = actor->GetSkill(IE_LOCKPICKING);
		if (skill == 0) { // a trained skill, make sure we fail
			stat = 0;
		} else {
			stat *= 7; // convert to percent (magic 7 is from RE)
			int dexmod = actor->GetAbilityBonus(IE_DEX);
			displaymsg->DisplayRollStringName(ieStrRef::ROLL11, GUIColors::LIGHTGREY, actor, stat - dexmod, lockDifficulty, dexmod);
		}
	}
	if (stat < (int) lockDifficulty) {
		displaymsg->DisplayMsgAtLocation(HCStrings::LockpickFailed, FT_ANY, actor, actor, GUIColors::XPCHANGE);
		AddTrigger(TriggerEntry(trigger_picklockfailed, actor->GetGlobalID()));
		core->GetAudioPlayback().PlayDefaultSound(DS_PICKFAIL, SFXChannel::Hits);
		return false;
	}

	core->GetGameControl()->ResetTargetMode();
	displaymsg->DisplayMsgAtLocation(HCStrings::LockpickDone, FT_ANY, actor, actor);
	core->GetAudioPlayback().PlayDefaultSound(DS_PICKLOCK, SFXChannel::Hits);
	AddTrigger(TriggerEntry(trigger_unlocked, actor->GetGlobalID()));
	ImmediateEvent();

	int xp = gamedata->GetXPBonus(XP_LOCKPICK, actor->GetXPLevel(1));
	const Game* game = core->GetGame();
	game->ShareXP(xp, SX_DIVIDE);
	return true;
}

// detect this trap, using a skill, skill could be set to 256 for 'sure'
// skill is the all around modified trap detection skill
// a trapdetectiondifficulty of 100 means impossible detection short of a spell
void Highlightable::DetectTrap(int skill, ieDword actorID)
{
	if (!CanDetectTrap()) return;
	if (TrapDetected) return;
	if (!Scripts[0]) return;
	if (skill >= 100 && skill != 256) skill = 100;

	int check = 0;
	Actor* detective = core->GetGame()->GetActorByGlobalID(actorID);
	assert(detective);
	if (core->HasFeature(GFFlags::RULES_3ED)) {
		// ~Search (detect traps) check. Search skill %d vs. trap's difficulty %d (searcher's %d INT bonus).~
		int bonus = detective->GetAbilityBonus(IE_INT);
		displaymsg->DisplayRollStringName(ieStrRef::ROLL13, GUIColors::LIGHTGREY, detective, skill - bonus, TrapDetectionDiff / 5, bonus);
		check = skill * 7;
	} else {
		check = skill / 2 + core->Roll(1, skill / 2, 0);
	}
	if (check > TrapDetectionDiff) {
		SetTrapDetected(1); // probably could be set to the player #?
		AddTrigger(TriggerEntry(trigger_detected, actorID));
		displaymsg->DisplayMsgAtLocation(HCStrings::TrapFound, FT_ANY, detective, detective, GUIColors::WHITE);
	}
}

bool Highlightable::PossibleToSeeTrap() const
{
	return CanDetectTrap();
}

// trap that is visible on screen (marked by red)
// if TrapDetected is a bitflag, we could show traps selectively for
// players, really nice for multiplayer
bool Highlightable::VisibleTrap(int seeAll) const
{
	if (!Trapped) return false;
	if (!PossibleToSeeTrap()) return false;
	if (!Scripts[0]) return false;
	if (seeAll) return true;
	if (TrapDetected) return true;
	return false;
}


void Highlightable::SetTrapDetected(int detected)
{
	if (detected == TrapDetected)
		return;
	TrapDetected = detected;
	if (TrapDetected) {
		core->GetAudioPlayback().PlayDefaultSound(DS_FOUNDSECRET, SFXChannel::Hits);
		core->Autopause(AUTOPAUSE::TRAP, this);
	}
}

void Highlightable::TryDisarm(Actor* actor)
{
	if (!Trapped || !TrapDetected) return;

	int skill = actor->GetStat(IE_TRAPS);
	int roll = 0;
	int bonus = 0;
	int trapDC = TrapRemovalDiff;

	if (core->HasFeature(GFFlags::RULES_3ED)) {
		skill = actor->GetSkill(IE_TRAPS);
		roll = core->Roll(1, 20, 0);
		bonus = actor->GetAbilityBonus(IE_INT);
		trapDC = TrapRemovalDiff / 7 + 10; // oddity from the original
		if (skill == 0) { // a trained skill
			trapDC = 100;
		}
	} else {
		roll = core->Roll(1, skill / 2, 0);
		skill /= 2;
	}

	int check = skill + roll + bonus;
	if (check > trapDC) {
		AddTrigger(TriggerEntry(trigger_disarmed, actor->GetGlobalID()));
		// trap removed
		Trapped = 0;
		if (core->HasFeature(GFFlags::RULES_3ED)) {
			// ~Successful Disarm Device - d20 roll %d + Disarm Device skill %d + INT mod %d >= Trap DC %d~
			displaymsg->DisplayRollStringName(ieStrRef::ROLL6, GUIColors::LIGHTGREY, actor, roll, skill - bonus, bonus, trapDC);
		}
		displaymsg->DisplayMsgAtLocation(HCStrings::DisarmDone, FT_ANY, actor, actor);
		int xp = gamedata->GetXPBonus(XP_DISARM, actor->GetXPLevel(1));
		const Game* game = core->GetGame();
		game->ShareXP(xp, SX_DIVIDE);
		core->GetGameControl()->ResetTargetMode();
		core->GetAudioPlayback().PlayDefaultSound(DS_DISARMED, SFXChannel::Hits);
	} else {
		AddTrigger(TriggerEntry(trigger_disarmfailed, actor->GetGlobalID()));
		if (core->HasFeature(GFFlags::RULES_3ED)) {
			// ~Failed Disarm Device - d20 roll %d + Disarm Device skill %d + INT mod %d >= Trap DC %d~
			displaymsg->DisplayRollStringName(ieStrRef::ROLL6, GUIColors::LIGHTGREY, actor, roll, skill - bonus, bonus, trapDC);
		}
		displaymsg->DisplayMsgAtLocation(HCStrings::DisarmFail, FT_ANY, actor, actor);
		TriggerTrap(skill, actor->GetGlobalID());
	}
	ImmediateEvent();
}

}
