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

namespace GemRB {

struct GEM_EXPORT OverHeadText {
	Point pos{-1, -1};
	bool isDisplaying = false;
	tick_t timeStartDisplaying = 0;
	String text;
	Color color = ColorBlack;
	const Scriptable* owner = nullptr;

	OverHeadText(const Scriptable* head) : owner(head) {};
	const String& GetText() const;
	void SetText(String newText, bool display = true, const Color& newColor = ColorBlack);
	bool Empty() const { return text.empty(); }
	bool Display(bool);
	void FixPos();
	int GetOffset() const;
	void Draw();
};

}

#endif
