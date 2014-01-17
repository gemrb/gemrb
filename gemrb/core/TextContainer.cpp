/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2014 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "TextContainer.h"

#include "Font.h"
#include "Palette.h"
#include "Sprite2D.h"
#include "System/String.h"

namespace GemRB {

TextSpan::TextSpan(const String& string, Font* fnt, Palette* pal)
	: text(string)
{
	font = fnt;
	spanSprite = NULL;
	// FIXME: this is only appropriate for inline text
	// for it to be of general use we would need to change the font calculations (a parameter?)
	frame = Region(0, 0, font->CalcStringWidth(text), font->CalcStringHeight(string));

	pal->acquire();
	palette = pal;
}

TextSpan::TextSpan(const String& string, Font* fnt, Palette* pal, const Region& rgn)
	: text(string), frame(rgn)
{
	font = fnt;
	spanSprite = NULL;

	pal->acquire();
	palette = pal;
}

TextSpan::~TextSpan()
{
	palette->release();
	spanSprite->release();
}

const Sprite2D* TextSpan::RenderedSpan()
{
	if (!spanSprite)
		RenderSpan();
	return spanSprite;
}

void TextSpan::RenderSpan()
{
	if (spanSprite) spanSprite->release();
	spanSprite = font->RenderText(text, frame);
	spanSprite->acquire();
}



TextContainer::TextContainer(Region& frame, Font* font, Palette* pal)
	: frame(frame), font(font)
{
	pal->acquire();
	pallete = pal;
}

TextContainer::~TextContainer()
{
	SpanList::iterator it = spans.begin();
	for (; it != spans.end(); ++it) {
		delete *it;
	}
	pallete->release();
}

void TextContainer::AppendText(const String& text)
{
	AppendSpan(new TextSpan(text, font, pallete));
}

void TextContainer::AppendSpan(TextSpan* span)
{
	spans.push_back(span);
}

const TextSpan* TextContainer::SpanAtPoint(const Point& p)
{
	// the point we are testing is relative to the container
	Region rgn = Region(0, 0, frame.w, frame.h);
	if (!rgn.PointInside(p))
		return NULL;

	rgn.h = 0; // start at 0 and work our way up by moving down the list
	SpanList::const_iterator it;
	for (it = spans.begin(); it != spans.end(); ++it) {
		// this list is "sorted" spans are layed out in this order from top to bottom
		const Region& spanRgn = (*it)->SpanFrame();
		rgn.h += spanRgn.h + spanRgn.y; // expand the search

		if (rgn.PointInside(p)) {
			return *it;
		}
	}
	return NULL;
}

}
