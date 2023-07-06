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

#include "Scriptable/Container.h"

#include "strrefs.h"

#include "DisplayMessage.h"
#include "Game.h"
#include "GameData.h"
#include "Item.h"
#include "Sprite2D.h"
#include "TileMap.h"
#include "GameScript/GSUtils.h"
#include "GUI/GameControl.h"

namespace GemRB {

#define YESNO(x) ( (x)?"Yes":"No")

Container::Container(void)
	: Highlightable( ST_CONTAINER )
{
	inventory.SetInventoryType(ieInventoryType::HEAP);
}

Region Container::DrawingRegion() const
{
	Region r(Pos.x, Pos.y, 0, 0);
	
	for (const auto& icon : groundicons) {
		if (icon) {
			Region frame = icon->Frame;
			frame.x = Pos.x - frame.x;
			frame.y = Pos.y - frame.y;
			r.ExpandToRegion(frame);
		}
	}
	
	return r;
}

void Container::Draw(bool highlight, const Region& vp, Color tint, BlitFlags flags) const
{
	Video* video = core->GetVideoDriver();

	for (const auto& icon : groundicons) {
		if (!icon) continue;

		if (highlight) {
			video->BlitGameSprite(icon, Pos - vp.origin, flags, tint);
		} else {
			const Color trans;
			PaletteHolder p = icon->GetPalette();
			Color tmpc = p->col[1];
			p->CopyColorRange(&trans, &trans + 1, 1);
			video->BlitGameSprite(icon, Pos - vp.origin, flags, tint);
			p->CopyColorRange(&tmpc, &tmpc + 1, 1);
		}
	}
}

// returns the appropriate cursor over a container (or pile)
int Container::GetCursor(int targetMode, int lastCursor) const
{
	if (Flags & CONT_DISABLED) {
		return lastCursor;
	}

	if (targetMode == TARGET_MODE_PICK) {
		if (VisibleTrap(0)) {
			return IE_CURSOR_TRAP;
		}
		if (Flags & CONT_LOCKED) {
			return IE_CURSOR_LOCK2;
		}
		return IE_CURSOR_STEALTH | IE_CURSOR_GRAY;
	}

	return IE_CURSOR_TAKE;
}

void Container::SetContainerLocked(bool lock)
{
	if (lock) {
		Flags|=CONT_LOCKED;
	} else {
		Flags&=~CONT_LOCKED;
	}
}

//This function doesn't exist in the original IE, destroys a container
//turning it to a ground pile
void Container::DestroyContainer()
{
	//it is already a groundpile?
	if (containerType == IE_CONTAINER_PILE)
		return;
	containerType = IE_CONTAINER_PILE;
	RefreshGroundIcons();
	//probably we should stop the script or trigger it, whatever
}

//Takes an item from the container's inventory and returns its pointer
CREItem *Container::RemoveItem(unsigned int idx, unsigned int count)
{
	CREItem *ret = inventory.RemoveItem(idx, count);
	//if we just took one of the first few items, groundpile changed
	if (containerType == IE_CONTAINER_PILE && idx < MAX_GROUND_ICON_DRAWN) {
		RefreshGroundIcons();
	}
	return ret;
}

//Adds an item to the container's inventory
//containers always have enough capacity (so far), thus we always return 2
int Container::AddItem(CREItem *item)
{
	inventory.AddItem(item);
	//we just added a 3. or less item, groundpile changed
	if (containerType == IE_CONTAINER_PILE && inventory.GetSlotCount() <= MAX_GROUND_ICON_DRAWN) {
		RefreshGroundIcons();
	}
	return 2;
}

void Container::RefreshGroundIcons()
{
	int i = std::min(inventory.GetSlotCount(), MAX_GROUND_ICON_DRAWN);
	int count = MAX_GROUND_ICON_DRAWN;
	while (count > i) {
		groundicons[--count] = nullptr;
	}
	while (i--) {
		const CREItem* slot = inventory.GetSlotItem(i); //borrowed reference
		const Item* itm = gamedata->GetItem(slot->ItemResRef); //cached reference
		if (!itm) continue;
		//well, this is required in PST, needs more work if some other
		//game is broken by not using -1,0
		groundicons[i] = gamedata->GetBAMSprite( itm->GroundIcon, 0, 0 );
		gamedata->FreeItem( itm, slot->ItemResRef ); //decref
	}
}

void Container::TryPickLock(Actor* actor)
{
	if (LockDifficulty == 100) {
		if (OpenFail != ieStrRef::INVALID) {
			displaymsg->DisplayStringName(OpenFail, GUIColors::XPCHANGE, actor, STRING_FLAGS::SOUND | STRING_FLAGS::SPEECH);
		} else {
			displaymsg->DisplayMsgAtLocation(HCStrings::ContNotpickable, FT_ANY, actor, actor, GUIColors::XPCHANGE);
		}
		return;
	}
	int stat = actor->GetStat(IE_LOCKPICKING);
	if (core->HasFeature(GFFlags::RULES_3ED)) {
		int skill = actor->GetSkill(IE_LOCKPICKING);
		if (skill == 0) { // a trained skill, make sure we fail
			stat = 0;
		} else {
			stat *= 7; // convert to percent (magic 7 is from RE)
			int dexmod = actor->GetAbilityBonus(IE_DEX);
			stat += dexmod; // the original didn't use it, so let's not multiply it
			displaymsg->DisplayRollStringName(ieStrRef::ROLL11, GUIColors::LIGHTGREY, actor, stat-dexmod, LockDifficulty, dexmod);
		}
	}
	if (stat < LockDifficulty) {
		displaymsg->DisplayMsgAtLocation(HCStrings::LockpickFailed, FT_ANY, actor, actor, GUIColors::XPCHANGE);
		AddTrigger(TriggerEntry(trigger_picklockfailed, actor->GetGlobalID()));
		core->PlaySound(DS_PICKFAIL, SFX_CHAN_HITS); //AMB_D21
		return;
	}
	SetContainerLocked(false);
	core->GetGameControl()->ResetTargetMode();
	displaymsg->DisplayMsgAtLocation(HCStrings::LockpickDone, FT_ANY, actor, actor);
	AddTrigger(TriggerEntry(trigger_unlocked, actor->GetGlobalID()));
	core->PlaySound(DS_PICKLOCK, SFX_CHAN_HITS); //AMB_D21D
	ImmediateEvent();
	int xp = gamedata->GetXPBonus(XP_LOCKPICK, actor->GetXPLevel(1));
	const Game *game = core->GetGame();
	game->ShareXP(xp, SX_DIVIDE);
}

void Container::TryBashLock(Actor *actor)
{
	// Get the strength bonus against lock difficulty
	int bonus;
	unsigned int roll;

	if (core->HasFeature(GFFlags::RULES_3ED)) {
		bonus = actor->GetAbilityBonus(IE_STR);
		roll = actor->LuckyRoll(1, 100, bonus, 0);
	} else {
		int str = actor->GetStat(IE_STR);
		int strEx = actor->GetStat(IE_STREXTRA);
		bonus = core->GetStrengthBonus(2, str, strEx); //BEND_BARS_LIFT_GATES
		roll = actor->LuckyRoll(1, 10, bonus, 0);
	}

	if (core->HasFeature(GFFlags::RULES_3ED)) {
		// ~Bash door check. Roll %d + %d Str mod > %d door DC.~
		// there is no separate string for non-doors
		displaymsg->DisplayRollStringName(ieStrRef::ROLL1, GUIColors::LIGHTGREY, actor, roll, bonus, LockDifficulty);
	}

	actor->FaceTarget(this);
	if(roll < LockDifficulty || LockDifficulty == 100) {
		displaymsg->DisplayMsgAtLocation(HCStrings::ContBashFail, FT_ANY, actor, actor, GUIColors::XPCHANGE);
		return;
	}

	displaymsg->DisplayMsgAtLocation(HCStrings::ContBashDone, FT_ANY, actor, actor);
	SetContainerLocked(false);
	core->GetGameControl()->ResetTargetMode();
	//Is this really useful ?
	AddTrigger(TriggerEntry(trigger_unlocked, actor->GetGlobalID()));
	ImmediateEvent();
}

std::string Container::dump() const
{
	std::string buffer;
	AppendFormat(buffer, "Debugdump of Container {}\n", GetScriptName() );
	AppendFormat(buffer, "Container Global ID: {}\n", GetGlobalID());
	AppendFormat(buffer, "Position: {}\n", Pos);
	AppendFormat(buffer, "Type: {}, Locked: {}, LockDifficulty: {}\n", containerType, YESNO(Flags&CONT_LOCKED), LockDifficulty);
	AppendFormat(buffer, "Flags: {}, Trapped: {}, Detected: {}\n", Flags, YESNO(Trapped), TrapDetected );
	AppendFormat(buffer, "Trap detection: {}%, Trap removal: {}%\n", TrapDetectionDiff,
		TrapRemovalDiff );
	ResRef name = "NONE";
	if (Scripts[0]) {
		name = Scripts[0]->GetName();
	}
	AppendFormat(buffer, "Script: {}, Key: {}\n", name, KeyResRef);
	buffer.append(inventory.dump(false));
	Log(DEBUG, "Container", "{}", buffer);
	return buffer;
}

bool Container::TryUnlock(Actor *actor) const
{
	if (!(Flags&CONT_LOCKED)) return true;

	return Highlightable::TryUnlock(actor, false);
}

bool Container::CanDetectTrap() const
{
	return Trapped && TrapDetectionDiff != 0;
}

}
