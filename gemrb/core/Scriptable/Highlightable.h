/* GemRB - Infinity Engine Emulator
* Copyright (C) 2024 The GemRB Project
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 2
* of the License, or (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*
*/

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
