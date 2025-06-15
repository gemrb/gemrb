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

#include "Scriptable/Door.h"

#include "strrefs.h"

#include "DisplayMessage.h"
#include "Game.h"
#include "GameData.h"
#include "Interface.h"
#include "Projectile.h"
#include "TileMap.h"

#include "GUI/GameControl.h"
#include "GameScript/GSUtils.h"
#include "Scriptable/InfoPoint.h"

namespace GemRB {

DoorTrigger::DoorTrigger(std::shared_ptr<Gem_Polygon> openTrigger, WallPolygonGroup&& openWalls,
			 std::shared_ptr<Gem_Polygon> closedTrigger, WallPolygonGroup&& closedWalls)
	: openWalls(std::move(openWalls)), closedWalls(std::move(closedWalls)), openTrigger(std::move(openTrigger)), closedTrigger(std::move(closedTrigger))
{}

void DoorTrigger::SetState(bool open, Map* map)
{
	isOpen = open;
	for (const auto& wp : openWalls) {
		wp->SetDisabled(!isOpen);
	}
	for (const auto& wp : closedWalls) {
		wp->SetDisabled(isOpen);
	}

	// also force update the Map stencils
	// without the viewport reset we would not notice there was a change
	map->ResetStencilViewport();
}

std::shared_ptr<Gem_Polygon> DoorTrigger::StatePolygon() const
{
	return StatePolygon(isOpen);
}

std::shared_ptr<Gem_Polygon> DoorTrigger::StatePolygon(bool open) const
{
	return open ? openTrigger : closedTrigger;
}

Door::Door(Holder<TileOverlay> Overlay, DoorTrigger&& trigger)
	: Highlightable(ST_DOOR), overlay(std::move(Overlay)), doorTrigger(std::move(trigger))
{
}

void Door::ImpedeBlocks(const std::vector<SearchmapPoint>& points, PathMapFlags value) const
{
	for (const SearchmapPoint& point : points) {
		PathMapFlags tmp = area->tileProps.QuerySearchMap(point) & PathMapFlags::NOTDOOR;
		area->tileProps.PaintSearchMap(point, tmp | value);
	}
}

void Door::UpdateDoor()
{
	doorTrigger.SetState(Flags & DOOR_OPEN, area);
	outline = doorTrigger.StatePolygon();

	if (outline) {
		// update the Scriptable position
		Pos.x = outline->BBox.x + outline->BBox.w / 2;
		Pos.y = outline->BBox.y + outline->BBox.h / 2;
		SetPos(Pos);
	}

	PathMapFlags pmdflags;

	if (Flags & DOOR_TRANSPARENT) {
		pmdflags = PathMapFlags::DOOR_IMPASSABLE;
	} else {
		//both door flags are needed here, one for transparency the other
		//is for passability
		pmdflags = PathMapFlags::DOOR_OPAQUE | PathMapFlags::DOOR_IMPASSABLE;
	}
	if (Flags & DOOR_OPEN) {
		ImpedeBlocks(closed_ib, PathMapFlags::IMPASSABLE);
		ImpedeBlocks(open_ib, pmdflags);
	} else {
		ImpedeBlocks(open_ib, PathMapFlags::IMPASSABLE);
		ImpedeBlocks(closed_ib, pmdflags);
	}

	InfoPoint* ip = area->TMap->GetInfoPoint(LinkedInfo);
	if (ip) {
		if (Flags & DOOR_OPEN)
			ip->Flags &= ~INFO_DOOR;
		else
			ip->Flags |= INFO_DOOR;
	}
}

void Door::ToggleTiles(int State, int playsound)
{
	int state;

	if (State) {
		state = !closedIndex;
		if (playsound && !OpenSound.IsEmpty()) {
			core->GetAudioDrv()->Play(OpenSound, SFXChannel::Actions, toOpen[0], GEM_SND_SPATIAL);
		}
	} else {
		state = closedIndex;
		if (playsound && !CloseSound.IsEmpty()) {
			core->GetAudioDrv()->Play(CloseSound, SFXChannel::Actions, toOpen[0], GEM_SND_SPATIAL);
		}
	}
	for (const auto& tile : tiles) {
		overlay->tiles[tile].tileIndex = (ieByte) state;
	}

	//set door_open as state
	Flags = (Flags & ~DOOR_OPEN) | (State == !core->HasFeature(GFFlags::REVERSE_DOOR));
}

//this is the short name (not the scripting name)
void Door::SetName(const ResRef& name)
{
	ID = name;
}

void Door::SetTiles(std::vector<ieWord> Tiles)
{
	tiles = std::move(Tiles);
}

bool Door::CanDetectTrap() const
{
	// Traps can be detected on all types of infopoint, as long
	// as the trap is detectable and isn't deactivated.
	return (Flags & DOOR_DETECTABLE) && Trapped;
}

void Door::SetDoorLocked(int Locked, int playsound)
{
	if (Locked) {
		if (Flags & DOOR_LOCKED) return;
		Flags |= DOOR_LOCKED;
		// only close it in pst, needed for Dead nations (see 4a3e1cb4ef)
		if (core->HasFeature(GFFlags::REVERSE_DOOR)) SetDoorOpen(false, playsound, 0);
		if (playsound && !LockSound.IsEmpty())
			core->GetAudioDrv()->Play(LockSound, SFXChannel::Actions, toOpen[0], GEM_SND_SPATIAL);
	} else {
		if (!(Flags & DOOR_LOCKED)) return;
		Flags &= ~DOOR_LOCKED;
		if (playsound && !UnLockSound.IsEmpty())
			core->GetAudioDrv()->Play(UnLockSound, SFXChannel::Actions, toOpen[0], GEM_SND_SPATIAL);
	}
}

int Door::IsOpen() const
{
	int ret = core->HasFeature(GFFlags::REVERSE_DOOR);
	if (Flags & DOOR_OPEN) {
		ret = !ret;
	}
	return ret;
}

bool Door::HitTest(const Point& p) const
{
	if (Flags & DOOR_HIDDEN) {
		return false;
	}

	auto doorpoly = doorTrigger.StatePolygon();
	if (doorpoly) {
		if (!doorpoly->PointIn(p)) return false;
	} else if (Flags & DOOR_OPEN) {
		if (!OpenBBox.PointInside(p)) return false;
	} else {
		if (!ClosedBBox.PointInside(p)) return false;
	}

	return true;
}

std::shared_ptr<Gem_Polygon> Door::OpenTriggerArea() const
{
	return doorTrigger.StatePolygon(true);
}

std::shared_ptr<Gem_Polygon> Door::ClosedTriggerArea() const
{
	return doorTrigger.StatePolygon(false);
}

//also mark actors to fix position
bool Door::BlockedOpen(int Open, int ForceOpen) const
{
	const std::vector<SearchmapPoint>* points = Open ? &open_ib : &closed_ib;
	bool blocked = false;

	//getting all impeded actors flagged for jump
	Region rgn;
	rgn.w = 16;
	rgn.h = 12;
	for (const SearchmapPoint& p : *points) {
		rgn.origin = p.ToNavmapPoint();
		PathMapFlags tmp = area->tileProps.QuerySearchMap(p) & PathMapFlags::ACTOR;
		if (tmp != PathMapFlags::IMPASSABLE) {
			auto actors = area->GetActorsInRect(rgn, GA_NO_DEAD | GA_NO_UNSCHEDULED);
			for (Actor* actor : actors) {
				if (actor->GetBase(IE_DONOTJUMP)) {
					continue;
				}
				actor->SetBase(IE_DONOTJUMP, DNJ_JUMP);
				blocked = true;
			}
		}
	}

	if ((Flags & DOOR_SLIDE) || ForceOpen) {
		return false;
	}
	return blocked;
}

void Door::SetDoorOpen(int Open, int playsound, ieDword openerID, bool addTrigger)
{
	if (playsound) {
		//the door cannot be blocked when opening,
		//but the actors will be pushed
		//BlockedOpen will mark actors to be pushed
		if (BlockedOpen(Open, 0) && !Open) {
			//clear up the blocking actors
			area->JumpActors(false);
			return;
		}
		area->JumpActors(true);
	}
	if (Open) {
		if (addTrigger) {
			if (Trapped) {
				AddTrigger(TriggerEntry(trigger_opened, openerID));
			} else {
				AddTrigger(TriggerEntry(trigger_harmlessopened, openerID));
			}
		}

		// in PS:T, opening a door does not unlock it
		// iwd2 ar6051 pit traps (eg. the lava switch door) also show it's not true there
		// except perhaps sometimes on closing?
		if (!core->HasFeature(GFFlags::REVERSE_DOOR) && !core->HasFeature(GFFlags::RULES_3ED)) {
			SetDoorLocked(false, playsound);
		}
	} else if (addTrigger) {
		if (Trapped) {
			AddTrigger(TriggerEntry(trigger_closed, openerID));
		} else {
			AddTrigger(TriggerEntry(trigger_harmlessclosed, openerID));
		}
	}
	ToggleTiles(Open, playsound);
	//synchronising other data with the door state
	UpdateDoor();

	core->SetEventFlag(EF_TARGETMODE);
}

bool Door::TryUnlock(Actor* actor) const
{
	if (!(Flags & DOOR_LOCKED)) return true;

	// don't remove key in PS:T!
	bool removekey = !core->HasFeature(GFFlags::REVERSE_DOOR) && Flags & DOOR_KEY;
	return Highlightable::TryUnlock(actor, removekey);
}

void Door::TryDetectSecret(int skill, ieDword actorID)
{
	if (Type != ST_DOOR || !(Flags & DOOR_SECRET)) return;
	if (Visible()) return;
	if (skill > (signed) DiscoveryDiff) {
		Flags |= DOOR_FOUND;
		core->PlaySound(DS_FOUNDSECRET, SFXChannel::Hits);
		if (core->HasFeature(GFFlags::HAS_EE_EFFECTS)) {
			AddTrigger(TriggerEntry(trigger_secreddoordetected, GetGlobalID()));
		} else {
			AddTrigger(TriggerEntry(trigger_detected, actorID));
		}
	}
}

// return true if the door isn't secret or if it is, but was already discovered
bool Door::Visible() const
{
	return (!(Flags & DOOR_SECRET) || (Flags & DOOR_FOUND)) && !(Flags & DOOR_HIDDEN);
}

void Door::SetNewOverlay(Holder<TileOverlay> Overlay)
{
	overlay = std::move(Overlay);
	ToggleTiles(IsOpen(), false);
}

void Door::TryPickLock(Actor* actor)
{
	if (!Highlightable::TryPickLock(actor, LockDifficulty, LockedStrRef, HCStrings::DoorNotPickable)) return;

	SetDoorLocked(false, true);
}

void Door::TryBashLock(Actor* actor)
{
	if (!Highlightable::TryBashLock(actor, LockDifficulty, HCStrings::DoorBashFail)) return;

	displaymsg->DisplayMsgAtLocation(HCStrings::DoorBashDone, FT_ANY, actor, actor, GUIColors::XPCHANGE);
	SetDoorLocked(false, true);
	Flags |= DOOR_BROKEN;
}

// returns the appropriate cursor over a door
int Door::GetCursor(TargetMode targetMode, int lastCursor) const
{
	if (!Visible()) {
		if (targetMode == TargetMode::None) {
			// most secret doors are in walls, so default to the blocked cursor to not give them away
			// iwd ar6010 table/door/puzzle is walkable, secret and undetectable
			return area->GetCursor(Pos);
		} else {
			return lastCursor | IE_CURSOR_GRAY;
		}
	}

	if (targetMode == TargetMode::Pick) {
		if (VisibleTrap(0)) {
			return IE_CURSOR_TRAP;
		}
		if (Flags & DOOR_LOCKED) {
			return IE_CURSOR_LOCK;
		}
		return IE_CURSOR_STEALTH | IE_CURSOR_GRAY;
	}

	return Cursor;
}

const Point* Door::GetClosestApproach(Scriptable* src, unsigned int& distance) const
{
	const Point* p = &toOpen[0];
	unsigned int dist1 = Distance(toOpen[0], src);
	unsigned int dist2 = Distance(toOpen[1], src);
	distance = dist1;
	if (dist1 > dist2) {
		p = &toOpen[1];
		distance = dist2;
	}
	return p;
}

std::string Door::dump() const
{
	std::string buffer;
	AppendFormat(buffer, "Debugdump of Door {}:\n", GetScriptName());
	AppendFormat(buffer, "Door Global ID: {}\n", GetGlobalID());
	AppendFormat(buffer, "Position: {}\n", Pos);
	AppendFormat(buffer, "Door Open: {}\n", YesNo(IsOpen()));
	AppendFormat(buffer, "Door Locked: {}\tDifficulty: {}\n", YesNo(Flags & DOOR_LOCKED), LockDifficulty);
	AppendFormat(buffer, "Door Trapped: {}\tDifficulty: {}\n", YesNo(Trapped), TrapRemovalDiff);
	if (Trapped) {
		AppendFormat(buffer, "Trap Permanent: {} Detectable: {}\n", YesNo(Flags & DOOR_RESET), YesNo(Flags & DOOR_DETECTABLE));
	}
	AppendFormat(buffer, "Secret door: {} (Found: {})\n", YesNo(Flags & DOOR_SECRET), YesNo(Flags & DOOR_FOUND));
	ResRef name = "NONE";
	if (Scripts[0]) {
		name = Scripts[0]->GetName();
	}
	AppendFormat(buffer, "Script: {}, Key ({}) removed: {}, Dialog: {}\n", name, KeyResRef, YesNo(Flags & DOOR_KEY), Dialog);
	Log(DEBUG, "Door", "{}", buffer);
	return buffer;
}


}
