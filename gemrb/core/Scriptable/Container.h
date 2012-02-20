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

#ifndef CONTAINER_H
#define CONTAINER_H

#include "Inventory.h"
#include "Scriptable/Scriptable.h"

namespace GemRB {

//container flags
#define CONT_LOCKED      1
#define CONT_RESET       8
#define CONT_DISABLED    (32|128)   //bg2 and pst uses different bits, luckily they are not overlapping

class GEM_EXPORT Container : public Highlightable {
public:
	Container(void);
	~Container(void);
	void SetContainerLocked(bool lock);
	//turns the container to a pile
	void DestroyContainer();
	//removes an item from the container's inventory
	CREItem *RemoveItem(unsigned int idx, unsigned int count);
	//adds an item to the container's inventory
	int AddItem(CREItem *item);
	//draws the ground icons
	void DrawPile(bool highlight, Region screen, Color tint);
	//returns dithering option
	int WantDither();
	int IsOpen() const;
	void TryPickLock(Actor *actor);
	void TryBashLock(Actor* actor) ;
	bool TryUnlock(Actor *actor);
	void dump() const;
	int TrapResets() const { return Flags & CONT_RESET; }
private:
	//updates the ground icons for a pile
	void RefreshGroundIcons();
	void FreeGroundIcons();
	void CreateGroundIconCover();
public:
	Point toOpen;
	ieWord Type;
	ieDword Flags;
	ieWord LockDifficulty;
	Inventory inventory;
	ieStrRef OpenFail;
	//these are not saved
	Sprite2D *groundicons[3];
	SpriteCover *groundiconcover;
	//keyresref is stored in Highlightable
};

}

#endif
