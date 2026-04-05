// SPDX-FileCopyrightText: 2016 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef __GemRB__Tooltip__
#define __GemRB__Tooltip__

#include "Region.h"

#include "GUI/TextSystem/Font.h"

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
	Tooltip(const String& s, Holder<Font> fnt, const Font::PrintColors& cols, std::unique_ptr<TooltipBackground> bg);

	const String& GetText() const { return text; };
	void SetText(const String& s);
	Size TextSize() const;

	void Draw(const Point& p) const;
};

}

#endif
