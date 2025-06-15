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

#include "Tooltip.h"

#include "Video/Video.h"

#include <utility>

namespace GemRB {

TooltipBackground::TooltipBackground(Holder<Sprite2D> bg, Holder<Sprite2D> left, Holder<Sprite2D> right)
	: background(std::move(bg)), leftbg(std::move(left)), rightbg(std::move(right))
{
	assert(background);
	assert((leftbg && rightbg) || (!leftbg && !rightbg));

	Reset();
}

TooltipBackground::TooltipBackground(const TooltipBackground& bg)
{
	animationSpeed = bg.animationSpeed;
	margin = bg.margin;

	background = bg.background;
	leftbg = bg.leftbg;
	rightbg = bg.rightbg;

	Reset();
}

void TooltipBackground::Reset()
{
	if (animationSpeed) {
		// the animation starts with the curls side by side
		animationPos = leftbg->Frame.w + rightbg->Frame.w;
	} else {
		animationPos = 9999; // will get clamped at draw times
	}
}

void TooltipBackground::SetMargin(int m)
{
	margin = m;
}

void TooltipBackground::SetAnimationSpeed(int s)
{
	animationSpeed = s;
}

Size TooltipBackground::MaxTextSize() const
{
	return Size(background->Frame.w - (margin * 2), background->Frame.h);
}

void TooltipBackground::Draw(Region rgn) const
{
	rgn.w += margin * 2;
	rgn.x -= margin;

	Point dp = rgn.origin;
	dp.x += (rgn.w / 2);
	dp.x -= (animationPos / 2); // start @ left curl pos

	// calculate the unrolled region
	Region bgclip(dp + Point(leftbg->Frame.w, -background->Frame.y), Size(animationPos + 1, background->Frame.h));
	bgclip.w -= leftbg->Frame.w + rightbg->Frame.w;

	// draw unrolled paper
	// note that there is transparency at the edges... this will get covered up by the right curl's Xpos offset
	VideoDriver->BlitSprite(background, Point(dp.x + background->Frame.x + 3, dp.y), &bgclip);

	// draw left paper curl
	VideoDriver->BlitSprite(leftbg, dp);

	// draw right paper curl (note it's sprite has a non 0 xpos)
	VideoDriver->BlitSprite(rightbg, Point(dp.x + animationPos - 1, dp.y));

	// clip the tooltip text to the background
	VideoDriver->SetScreenClip(&bgclip);

	// advance the animation
	int maxw = std::min(MaxTextSize().w, rgn.w) + leftbg->Frame.w + rightbg->Frame.w;
	if (animationPos < maxw) {
		animationPos += animationSpeed;
	} else {
		animationPos = maxw;
	}
}


Tooltip::Tooltip(const String& s, Holder<Font> fnt, const Font::PrintColors& cols, TooltipBackground* bg)
	: font(std::move(fnt)), background(bg), colors(cols)
{
	SetText(s);
}

void Tooltip::SetText(const String& t)
{
	text = t;

	Font::StringSizeMetrics metrics = { Size(), 0, 0, true };
	// NOTE: arbitrary fallback size which doesn't come into play with original data
	metrics.size = background ? background->MaxTextSize() : Size(128, 128);
	textSize = font->StringSize(text, &metrics);

	if (background)
		background->Reset();
}

Size Tooltip::TextSize() const
{
	return textSize;
}

void Tooltip::Draw(const Point& pos) const
{
	if (text.length() == 0) {
		return;
	}

	Region textr(pos, textSize);
	// the tooltip is centered at pos
	textr.x -= textr.w / 2;

	if (background) {
		const Size& maxs = background->MaxTextSize();

		background->Draw(textr);
		textr.h = maxs.h;
		textr.y = pos.y - textr.h / 2;
	}

	font->Print(textr, text, IE_FONT_ALIGN_CENTER | IE_FONT_ALIGN_MIDDLE, colors);
}

}
