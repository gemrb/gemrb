/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2016 The GemRB Project
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

#ifndef __GemRB__Tooltip__
#define __GemRB__Tooltip__

#include "Region.h"

#include "GUI/TextSystem/Font.h"
#include "Video/Video.h"

namespace GemRB {

class TooltipBackground {
	int animationSpeed = 0; // no animation by default
	mutable int animationPos = 9999;
	int margin = 5;

	Holder<Sprite2D> background;
	Holder<Sprite2D> leftbg;
	Holder<Sprite2D> rightbg;

public:
	explicit TooltipBackground(Holder<Sprite2D> bg, Holder<Sprite2D> = nullptr, Holder<Sprite2D> right = nullptr);
	TooltipBackground(const TooltipBackground&);

	void Draw(Region rgn) const;

	void SetMargin(int);
	void SetAnimationSpeed(int);

	void Reset();
	Size MaxTextSize() const;
};

class Tooltip {
	String text;
	Holder<Font> font;
	std::unique_ptr<TooltipBackground> background;
	Size textSize;
	Font::PrintColors colors;

public:
	Tooltip(const String& s, Holder<Font> fnt, const Font::PrintColors& cols, TooltipBackground* bg);

	void SetText(const String& s);
	Size TextSize() const;

	void Draw(const Point& p) const;
};

}

#endif
