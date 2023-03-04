/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2023 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

#ifndef OVERHEADTEXT_H
#define OVERHEADTEXT_H

#include "exports.h"
#include "globals.h"

#include <vector>

namespace GemRB {

struct OverHeadMsg {
	Point pos{-1, -1};
	Point scrollOffset{-1, -1};
	Color color = ColorBlack;
	tick_t timeStartDisplaying = 0;
	String text;

	bool Draw(int heightOffset, const Point& fallbackPos);
};

class GEM_EXPORT OverHeadText {
	bool isDisplaying = false;
	const Scriptable* owner = nullptr;
	// the first message is reserved for regular overhead text
	// all the rest are used only in pst by either:
	// - actors (full queue at same position) or
	// - areas (disparate messages at different positions)
	std::vector<OverHeadMsg> messages{1};

public:
	OverHeadText(const Scriptable* head) : owner(head) {};
	const String& GetText(size_t idx = 0) const;
	void SetText(String newText, bool display = true, bool append = true, const Color& newColor = ColorBlack);
	bool Empty(size_t idx = 0) const { return messages[idx].text.empty(); }
	bool IsDisplaying() const { return isDisplaying; }
	bool Display(bool show, size_t idx = 0);
	void FixPos(const Point& pos, size_t idx = 0);
	int GetHeightOffset() const;
	void Draw();
};

}

#endif
