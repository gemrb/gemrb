// SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef CONTAINER_H
#define CONTAINER_H

#include "Inventory.h"
#include "Sprite2D.h"

#include "GUI/GameControlDefs.h"
#include "Scriptable/Highlightable.h"

#include <array>

namespace GemRB {

//container flags
#define CONT_LOCKED 1
//#define CONT_          2 // "disable if no owner" comment from original bg2 source
//#define CONT_          4 // "magically locked", probably was meant to prevent lockpicking
#define CONT_RESET 8
//#define CONT_          16 // "Remove only"
#define CONT_DISABLED (32 | 128) //bg2 and pst uses different bits, luckily they are not overlapping

class GEM_EXPORT Container : public Highlightable {
public:
	Container(void);
	void SetContainerLocked(bool lock);
	//removes an item from the container's inventory
	CREItem* RemoveItem(unsigned int idx, unsigned int count);
	//adds an item to the container's inventory
	int AddItem(CREItem* item);
	//draws the ground icons
	Region DrawingRegion() const override;
	void Draw(bool highlight, const Region& screen, Color tint, BlitFlags flags) const;
	int GetCursor(TargetMode targetMode, int lastCursor) const;

	void TryPickLock(Actor* actor) override;
	void TryBashLock(Actor* actor) override;
	bool TryUnlock(Actor* actor) const;
	std::string dump() const override;
	bool IsLocked() const override { return Flags & CONT_LOCKED; }
	int TrapResets() const override { return Flags & CONT_RESET; }
	bool CanDetectTrap() const override;

private:
	//updates the ground icons for a pile
	void RefreshGroundIcons();

public:
	Point toOpen;
	ieWord containerType = 0;
	ieDword Flags = 0;
	ieWord LockDifficulty = 0;
	Inventory inventory;
	ieStrRef OpenFail = ieStrRef::INVALID;
	//these are not saved
	std::array<Holder<Sprite2D>, MAX_GROUND_ICON_DRAWN> groundicons;
	//keyresref is stored in Highlightable
};

}

#endif
