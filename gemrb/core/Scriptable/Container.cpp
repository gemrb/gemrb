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
#include "Interface.h"
#include "Item.h"
#include "Sprite2D.h"
#include "TileMap.h"
#include "GameScript/GSUtils.h"
#include "GUI/GameControl.h"

namespace GemRB {

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
	for (const auto& icon : groundicons) {
		if (!icon) continue;

		if (highlight) {
			VideoDriver->BlitGameSprite(icon, Pos - vp.origin, flags, tint);
		} else {
			const Color trans;
			Holder<Palette> p = icon->GetPalette();
			Color tmpc = p->GetColorAt(1);
			p->SetColor(1, trans);
			VideoDriver->BlitGameSprite(icon, Pos - vp.origin, flags, tint);
			p->SetColor(1, tmpc);
		}
	}
}

// returns the appropriate cursor over a container (or pile)
int Container::GetCursor(TargetMode targetMode, int lastCursor) const
{
	if (Flags & CONT_DISABLED) {
		return lastCursor;
	}

	if (targetMode == TargetMode::Pick) {
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
	return ASI_SUCCESS;
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
	if (!Highlightable::TryPickLock(actor, LockDifficulty, OpenFail, HCStrings::ContNotpickable)) return;

	SetContainerLocked(false);
}

void Container::TryBashLock(Actor *actor)
{
	if (!Highlightable::TryBashLock(actor, LockDifficulty, HCStrings::ContBashFail)) return;

	displaymsg->DisplayMsgAtLocation(HCStrings::ContBashDone, FT_ANY, actor, actor);
	SetContainerLocked(false);
}

std::string Container::dump() const
{
	std::string buffer;
	AppendFormat(buffer, "Debugdump of Container {}\n", GetScriptName() );
	AppendFormat(buffer, "Container Global ID: {}\n", GetGlobalID());
	AppendFormat(buffer, "Position: {}\n", Pos);
	AppendFormat(buffer, "Type: {}, Locked: {}, LockDifficulty: {}\n", containerType, YesNo(Flags & CONT_LOCKED), LockDifficulty);
	AppendFormat(buffer, "Flags: {}, Trapped: {}, Detected: {}\n", Flags, YesNo(Trapped), TrapDetected);
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
	return Trapped && (core->HasFeature(GFFlags::RULES_3ED) || TrapDetectionDiff != 0);
}

}
