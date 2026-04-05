// SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef INFOPOINT_H
#define INFOPOINT_H

#include "strrefs.h"

#include "GUI/GameControlDefs.h"
#include "Scriptable/Highlightable.h"

namespace GemRB {

class GEM_EXPORT InfoPoint : public Highlightable {
public:
	InfoPoint(void);
	//returns true if trap has been triggered, tumble skill???
	void SetEnter(const ResRef& resref);
	bool TriggerTrap(int skill, ieDword ID) override;
	//call this to check if an actor entered the trigger zone
	bool Entered(Actor* actor);
	ieDword GetUsePoint() const;
	//checks if the actor may use this travel trigger
	int CheckTravel(const Actor* actor) const;
	std::string dump() const override;
	int TrapResets() const override { return Flags & TRAP_RESET; }
	bool CanDetectTrap() const override;
	bool PossibleToSeeTrap() const override;
	bool IsPortal() const;
	int GetCursor(TargetMode targetMode) const;
	void TryBashLock(Actor*) override { return; };
	void TryPickLock(Actor*) override { return; };

public:
	ResRef Destination;
	ieVariable EntranceName;
	ieDword Flags = 0;
	//overheadtext contains the string, but we have to save this
	ieStrRef StrRef = ieStrRef::INVALID;
	Point UsePoint = Point(-1, -1);
	Point TalkPos = Point(-1, -1);
};

}

#endif
