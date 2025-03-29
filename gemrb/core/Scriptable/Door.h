/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
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
 *
 */

#ifndef DOOR_H
#define DOOR_H

#include "Polygon.h"
#include "TileOverlay.h"

#include "GUI/GameControlDefs.h"
#include "Scriptable/Highlightable.h"

namespace GemRB {

// door flags; iwd2 differences are remapped in the importer
#define DOOR_OPEN                 1
#define DOOR_LOCKED               2
#define DOOR_RESET                4 //reset trap
#define DOOR_DETECTABLE           8 //trap detectable
#define DOOR_BROKEN               16 //door is broken (force opened)
#define DOOR_CANTCLOSE            32 // used in BG1 (at least TotSC). Prevents random closing of doors in CGameDoor::CompressTime. (The random closing is only done when more than a hour of ingame time passed.)
#define DOOR_LINKED               64 // info trigger linked to this door (see LinkedInfo, INFO_DOOR), iwd2: DETECTED (used, but no code)
#define DOOR_SECRET               128 //door is secret
#define DOOR_FOUND                256 //secret door found
#define DOOR_TRANSPARENT          512 //obscures vision (LOCKEDINFOTEXT in iwd2)
#define DOOR_KEY                  1024 //key removed when used (SEETHROUGH in iwd2)
#define DOOR_SLIDE                2048 //impeded blocks ignored (WARNINGINFOTEXT in iwd2)
#define DOOR_WARNINGTEXTDISPLAYED 0x1000 // iwd2 after DOOR_WARNINGINFOTEXT's warning was shown; cleared when closing the door
#define DOOR_HIDDEN               8192 // iwd2, ignore the door
#define DOOR_USEUPKEY             0x4000 // iwd2, treating as identical to DOOR_KEY
#define DOOR_LOCKEDINFOTEXT       0x8000 // iwd2, use custom text instead of default "Locked"
#define DOOR_WARNINGINFOTEXT      0x10000 // iwd2, display warning when trying to open a door, unused in game

class GEM_EXPORT DoorTrigger {
	WallPolygonGroup openWalls;
	WallPolygonGroup closedWalls;

	std::shared_ptr<Gem_Polygon> openTrigger;
	std::shared_ptr<Gem_Polygon> closedTrigger;

	bool isOpen = false;

public:
	DoorTrigger(std::shared_ptr<Gem_Polygon> openTrigger, WallPolygonGroup&& openWall,
		    std::shared_ptr<Gem_Polygon> closedTrigger, WallPolygonGroup&& closedWall);

	void SetState(bool open, Map* map);

	std::shared_ptr<Gem_Polygon> StatePolygon() const;
	std::shared_ptr<Gem_Polygon> StatePolygon(bool open) const;
};

class GEM_EXPORT Door : public Highlightable {
public:
	Door(Holder<TileOverlay> Overlay, DoorTrigger&& trigger);

public:
	ieVariable LinkedInfo;
	ResRef ID; //WED ID
	Holder<TileOverlay> overlay;
	std::vector<ieWord> tiles;
	ieDword Flags = 0;
	int closedIndex = 0;
	//trigger areas
	DoorTrigger doorTrigger;
	Region& OpenBBox = BBox; // an alias for the base class BBox
	Region ClosedBBox;
	//impeded blocks
	std::vector<SearchmapPoint> open_ib; // impeded blocks stored in a Point array
	std::vector<SearchmapPoint> closed_ib;

	Point toOpen[2];
	ResRef OpenSound;
	ResRef CloseSound;
	ResRef LockSound; // these two are gemrb extensions
	ResRef UnLockSound;
	ieDword DiscoveryDiff = 0;
	ieDword LockDifficulty = 0; //this is a dword?
	ieStrRef LockedStrRef = ieStrRef::INVALID;
	ieStrRef NameStrRef = ieStrRef::INVALID;
	// unused, but learned from IE DEV info
	ieWord hp = 0;
	ieWord ac = 0;

private:
	void ImpedeBlocks(const std::vector<SearchmapPoint>& points, PathMapFlags value) const;
	bool BlockedOpen(int Open, int ForceOpen) const;

public:
	void UpdateDoor();
	void ToggleTiles(int State, int playsound = false);
	void SetName(const ResRef& Name); // sets door ID
	void SetTiles(std::vector<ieWord>);
	bool CanDetectTrap() const override;
	void SetDoorLocked(int Locked, int playsound);
	void SetDoorOpen(int Open, int playsound, ieDword openerID, bool addTrigger = true);
	int IsOpen() const;
	bool HitTest(const Point& p) const;
	void TryPickLock(Actor* actor) override;
	void TryBashLock(Actor* actor) override;
	bool TryUnlock(Actor* actor) const;
	void TryDetectSecret(int skill, ieDword actorID);
	bool Visible() const;
	int GetCursor(TargetMode targetMode, int lastCursor) const;
	std::string dump() const override;
	bool IsLocked() const override { return Flags & DOOR_LOCKED; }
	int TrapResets() const override { return Flags & DOOR_RESET; }
	bool CantAutoClose() const { return Flags & (DOOR_CANTCLOSE | DOOR_LOCKED); }
	void SetNewOverlay(Holder<TileOverlay> Overlay);
	const Point* GetClosestApproach(const Scriptable* src, unsigned int& distance) const;

	std::shared_ptr<Gem_Polygon> OpenTriggerArea() const;
	std::shared_ptr<Gem_Polygon> ClosedTriggerArea() const;
};

}

#endif
