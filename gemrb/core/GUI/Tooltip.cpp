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

#include "Interface.h"
#include "GUI/TextSystem/Font.h"

namespace GemRB {

TooltipBackground::TooltipBackground(Holder<Sprite2D> bg, Holder<Sprite2D> left, Holder<Sprite2D> right)
: background(bg), leftbg(left), rightbg(right)
{
	assert(background);
	assert((leftbg && rightbg) || (!leftbg && !rightbg));

	margin = 5;
	// no animation by default
	animationSpeed = 0;

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
	animationPos = 0;
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
	return Size(background->Width, background->Height);
}

void TooltipBackground::Draw(const Region& rgn) const
{
	Point dp = rgn.Origin();
	dp.x += (rgn.w / 2);
	dp.x -= (animationPos / 2);
	dp.y += margin;

	Video* video = core->GetVideoDriver();

	// draw left paper curl
	video->BlitSprite( leftbg.get(), dp.x - leftbg->Width, dp.y );

	// calculate the unrolled region
	Region bgclip(dp, Size(animationPos + 1, background->Height));
	bgclip.y -= background->YPos;

	// draw unrolled paper
	int mid = (rgn.w / 2) - (background->Width / 2) + dp.x - background->XPos;
	video->BlitSprite( background.get(), -mid, dp.y, &bgclip );

	// draw right paper curl
	dp.x += animationPos;
	video->BlitSprite( rightbg.get(), dp.x + leftbg->Width, dp.y );

	// clip the tooltip text to the background
	video->SetScreenClip(&bgclip);

	// advance the animation
	int maxw = rgn.w + (margin * 2);
	if (animationPos < maxw ) {
		animationPos += animationSpeed;
	} else {
		animationPos = maxw;
	}
}


Tooltip::Tooltip(const String& s, Font* fnt, TooltipBackground* bg)
: font(fnt), background(bg)
{
	SetText(s);
}

Tooltip::~Tooltip()
{
	delete background;
}

void Tooltip::SetText(const String& t)
{
	text = t;

	Font::StringSizeMetrics metrics = {Size(), 0, true};
	// FIXME: arbitrary fallback size
	metrics.size = (background) ? background->MaxTextSize() : Size(128,128);
	textSize = font->StringSize( text, &metrics );

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
	textr.x -= textr.w/2;
	textr.y -= textr.h/2;

	if (background) {
		background->Draw(textr);
	}
	
	textr.y -= (font->LineHeight / 2) + 2;
	textr.h = font->LineHeight * 2;

	font->Print( textr, text, NULL, IE_FONT_ALIGN_CENTER | IE_FONT_ALIGN_MIDDLE );
}

}
