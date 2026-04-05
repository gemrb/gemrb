// SPDX-FileCopyrightText: 2024 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef H_HIGHLIGHTABLE
#define H_HIGHLIGHTABLE

#include "ie_cursors.h"
#include "strrefs.h"

#include "Scriptable.h"

namespace GemRB {

class Gem_Polygon;

class GEM_EXPORT Highlightable : public Scriptable {
public:
	using Scriptable::Scriptable;
	virtual int TrapResets() const = 0;
	virtual bool CanDetectTrap() const { return true; }
	virtual bool PossibleToSeeTrap() const;
	virtual void TryBashLock(Actor* actor) = 0;
	virtual void TryPickLock(Actor* actor) = 0;
	virtual bool IsLocked() const { return false; }

public:
	std::shared_ptr<Gem_Polygon> outline = nullptr;
	Color outlineColor = ColorBlack;
	ieDword Cursor = IE_CURSOR_NORMAL;
	bool Highlight = false;
	Point TrapLaunch = Point(-1, -1);
	ieWord TrapDetectionDiff = 0;
	ieWord TrapRemovalDiff = 0;
	ieWord Trapped = 0;
	ieWord TrapDetected = 0;
	ResRef KeyResRef;
	// play this wav file when stepping on the trap (on PST)
	ResRef EnterWav;

public:
	bool IsOver(const Point& place) const;
	void DrawOutline(Point origin) const;
	void SetCursor(unsigned char cursorIndex);

	void SetTrapDetected(int detected);
	void TryDisarm(Actor* actor);
	// detect trap, set skill to 256 if you want sure fire
	void DetectTrap(int skill, ieDword actorID);
	// returns true if trap is visible
	bool VisibleTrap(int seeAll) const;
	// returns true if trap has been triggered
	virtual bool TriggerTrap(int skill, ieDword ID);
	bool TryUnlock(Actor* actor, bool removekey) const;
	bool TryBashLock(Actor* actor, ieWord lockDifficulty, HCStrings failStr);
	bool TryPickLock(Actor* actor, ieWord lockDifficulty, ieStrRef customFailStr, HCStrings failStr);
};

}

#endif
