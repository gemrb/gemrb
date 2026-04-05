// SPDX-FileCopyrightText: 2024 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

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
	bool IsOver(const Point& Pos, const Point& CenterPos) const;
	void SetOver(bool over);
	bool IsSelected() const;
	void Select(int Value);
	void SetCircle(int size, float_t, const Color& color, Holder<Sprite2D> normal_circle, Holder<Sprite2D> selected_circle);
	int CircleSize2Radius() const;
	static int CircleSize2Radius(int circleSize);
};

}

#endif
