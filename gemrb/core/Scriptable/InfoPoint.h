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

#ifndef INFOPOINT_H
#define INFOPOINT_H

#include "Scriptable/Scriptable.h"

namespace GemRB {

class GEM_EXPORT InfoPoint : public Highlightable {
public:
	InfoPoint(void);
	//returns true if trap has been triggered, tumble skill???
	void SetEnter(const char *resref);
	bool TriggerTrap(int skill, ieDword ID) override;
	//call this to check if an actor entered the trigger zone
	bool Entered(Actor *actor);
	ieDword GetUsePoint() const;
	//checks if the actor may use this travel trigger
	int CheckTravel(const Actor *actor) const;
	std::string dump() const;
	int TrapResets() const override { return Flags & TRAP_RESET; }
	bool CanDetectTrap() const override;
	bool PossibleToSeeTrap() const override;
	bool IsPortal() const;
	int GetCursor(int targetMode) const;

public:
	ResRef Destination;
	ieVariable EntranceName;
	ieDword Flags = 0;
	//overheadtext contains the string, but we have to save this
	ieStrRef StrRef = 0;
	Point UsePoint = Point(-1, -1);
	Point TalkPos = Point(-1, -1);
};

}

#endif
