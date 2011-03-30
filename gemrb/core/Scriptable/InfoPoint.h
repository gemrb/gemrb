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

#include "Scriptable.h"

//trigger flags
#define TRAP_INVISIBLE  1
#define TRAP_RESET      2
#define TRAVEL_PARTY    4
#define TRAP_DETECTABLE 8
//#define TRAP_16	 16
#define TRAP_LOWMEM	 32 //special treatment when low on memory ?
#define TRAP_NPC	64
//#define TRAP_128	128
#define TRAP_DEACTIVATED  256
#define TRAVEL_NONPC      512
#define TRAP_USEPOINT       1024 //override usage point of travel regions (used for sound in PST traps)
#define INFO_DOOR	 2048 //info trigger blocked by door

class GEM_EXPORT InfoPoint : public Highlightable {
public:
	InfoPoint(void);
	~InfoPoint(void);
	//returns true if trap has been triggered, tumble skill???
	void SetEnter(const char *resref);
	bool TriggerTrap(int skill, ieDword ID);
	//call this to check if an actor entered the trigger zone
	bool Entered(Actor *actor);
	//checks if the actor may use this travel trigger
	int CheckTravel(Actor *actor);
	void DebugDump() const;
	int TrapResets() const { return Flags & TRAP_RESET; }
	bool CanDetectTrap() const;
	bool PossibleToSeeTrap() const;
	bool IsPortal() const;

public:
	ieResRef Destination;
	ieVariable EntranceName;
	ieDword Flags;
	//overheadtext contains the string, but we have to save this
	ieStrRef StrRef;
	Point UsePoint;
	Point TalkPos;
};

#endif
