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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#ifndef SELECTABLE_H
#define SELECTABLE_H

#include "RGBAColor.h"

#include "Scriptable.h"
#include "Sprite2D.h"

namespace GemRB {

class GEM_EXPORT Selectable : public Scriptable {
public:
	using Scriptable::Scriptable;

public:
	ieWord Selected = 0; // could be 0x80 for unselectable
	bool Over = false;
	Color selectedColor = ColorBlack;
	Color overColor = ColorBlack;
	Holder<Sprite2D> circleBitmap[2] = {};
	int circleSize = 0;
	float_t sizeFactor = 1.0f;

public:
	void SetBBox(const Region& newBBox);
	void DrawCircle(const Point& p) const;
	bool IsOver(const Point& Pos) const;
	void SetOver(bool over);
	bool IsSelected() const;
	void Select(int Value);
	void SetCircle(int size, float_t, const Color& color, Holder<Sprite2D> normal_circle, Holder<Sprite2D> selected_circle);
	int CircleSize2Radius() const;
};

}

#endif
