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

#include "Selectable.h"

#include "GUI/GUIAnimation.h"
#include "Video/Video.h"

namespace GemRB {

void Selectable::SetBBox(const Region& newBBox)
{
	BBox = newBBox;
}

// NOTE: still need to multiply by 4 or 3 to get full pixel radii
int Selectable::CircleSize2Radius() const
{
	// for size >= 2, radii are (size-1)*16, (size-1)*12
	// for size == 1, radii are 12, 9
	int adjustedSize = (circleSize - 1) * 4;
	if (adjustedSize < 4) adjustedSize = 3;
	return adjustedSize;
}

void Selectable::DrawCircle(const Point& p) const
{
	if (circleSize <= 0) {
		return;
	}

	Color mix;
	const Color* col = &selectedColor;
	Holder<Sprite2D> sprite = circleBitmap[0];

	if (Selected && !Over) {
		sprite = circleBitmap[1];
	} else if (Over) {
		mix = GlobalColorCycle.Blend(overColor, selectedColor);
		col = &mix;
	} else if (IsPC()) {
		// only dim base EA colors
		if (*col == ColorGreen || *col == ColorBlue || *col == ColorRed) col = &overColor;
	}

	if (sprite) {
		VideoDriver->BlitSprite(sprite, Pos - p);
	} else {
		auto baseSize = CircleSize2Radius() * sizeFactor;
		const Size s(baseSize * 8, baseSize * 6);
		const Region r(Pos - p - s.Center(), s);
		VideoDriver->DrawEllipse(r, *col);
	}
}

// Check if P is over our ground circle
bool Selectable::IsOver(const Point& P) const
{
	return IsOver(P, Pos);
}

bool Selectable::IsOver(const Point &P, const Point &CenterPos) const {
	int csize = circleSize;
	if (csize < 2) {
		Point d = P - CenterPos;
		if (d.x < -16 || d.x > 16) return false;
		if (d.y < -12 || d.y > 12) return false;
		return true;
	}
	// TODO: make sure to match the actual blocking shape; use GetEllipseSize/GetEllipseOffset instead?
	return P.IsWithinEllipse(csize - 1, CenterPos);
}

bool Selectable::IsSelected() const
{
	return Selected == 1;
}

void Selectable::SetOver(bool over)
{
	Over = over;
}

//don't call this function after rendering the cover and before the
//blitting of the sprite or bad things will happen :)
void Selectable::Select(int Value)
{
	if (Selected != 0x80 || Value != 1) {
		Selected = (ieWord) Value;
	}
}

void Selectable::SetCircle(int circlesize, float_t factor, const Color& color, Holder<Sprite2D> normal_circle, Holder<Sprite2D> selected_circle)
{
	circleSize = circlesize;
	sizeFactor = factor;
	selectedColor = color;
	overColor.r = color.r >> 1;
	overColor.g = color.g >> 1;
	overColor.b = color.b >> 1;
	overColor.a = color.a;
	circleBitmap[0] = std::move(normal_circle);
	circleBitmap[1] = std::move(selected_circle);
}

}
