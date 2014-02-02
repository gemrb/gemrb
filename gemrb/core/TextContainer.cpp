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
#include "Interface.h"
#include "Palette.h"
#include "Sprite2D.h"
#include "System/String.h"
#include "Video.h"

namespace GemRB {

TextSpan::TextSpan(const String& string, Font* fnt, Palette* pal)
	: text(string)
{
	font = fnt;
	spanSprite = NULL;
	palette = NULL;
	frame = font->StringSize(text);

	SetPalette(pal);
	alignment = IE_FONT_SINGLE_LINE;
}

TextSpan::TextSpan(const String& string, Font* fnt, Palette* pal, const Size& frame, ieByte align)
	: text(string), frame(frame)
{
	font = fnt;
	spanSprite = NULL;
	palette = NULL;

	SetPalette(pal);
	alignment = align;
}

TextSpan::~TextSpan()
{
	palette->release();
	if (spanSprite)
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
	// TODO: implement span alignments
	spanSprite = font->RenderTextAsSprite(text, frame, alignment, palette);
	spanSprite->acquire();
	// frame dimensions of 0 just mean size to fit
	if (frame.w == 0)
		frame.w = spanSprite->Width;
	if (frame.h == 0)
		frame.h = spanSprite->Height;
}

const Size& TextSpan::SpanFrame()
{
	if (frame.IsEmpty())
		RenderSpan(); // our true frame is determined by the rendering
	return frame;
}

void TextSpan::SetPalette(Palette* pal)
{
	assert(pal);
	pal->acquire();
	if (palette)
		palette->release();
	palette = pal;
	if (spanSprite)
		spanSprite->SetPalette(palette);
}


TextContainer::TextContainer(const Size& frame, Font* font, Palette* pal)
	: maxFrame(frame), frame(), font(font)
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
	AppendSpan(new TextSpan(text, font, pallete, Size(maxFrame.w, 0), 0));
}

void TextContainer::AppendSpan(TextSpan* span)
{
	spans.push_back(span);
	LayoutSpansStartingAt(--spans.end());
}

void TextContainer::InsertSpanAfter(TextSpan* newSpan, const TextSpan* existing)
{
	if (!existing) { // insert at beginning;
		spans.push_front(newSpan);
		return;
	}
	SpanList::iterator it;
	it = std::find(spans.begin(), spans.end(), existing);
	spans.insert(++it, newSpan);
	LayoutSpansStartingAt(it);
}

TextSpan* TextContainer::RemoveSpan(const TextSpan* span)
{
	SpanList::iterator it;
	it = std::find(spans.begin(), spans.end(), span);
	if (it != spans.end()) {
		LayoutSpansStartingAt(--spans.erase(it));
		return (TextSpan*)span;
	}
	return NULL;
}

TextSpan* TextContainer::SpanAtPoint(const Point& p) const
{
	// the point we are testing is relative to the container
	Region rgn = Region(0, 0, frame.w, frame.h);
	if (!rgn.PointInside(p))
		return NULL;

	SpanLayout::const_iterator it;
	for (it = layout.begin(); it != layout.end(); ++it) {
		if ((*it).second.PointInside(p)) {
			return (*it).first;
		}
	}
	return NULL;
}

void TextContainer::DrawContents(int x, int y) const
{
	Video* video = core->GetVideoDriver();
	Region drawRgn = Region(Point(x, y), maxFrame);
#if DEBUG_TEXT
	// Draw the exclusion regions
	std::vector<Region>::const_iterator ex;
	for (ex = ExclusionRects.begin(); ex != ExclusionRects.end(); ++ex) {
		Region rgn = *ex;
		rgn.x += x;
		rgn.y += y;
		video->DrawRect(rgn, ColorRed);
	}
#endif
	SpanLayout::const_iterator it;
	for (it = layout.begin(); it != layout.end(); ++it) {
		Region rgn = (*it).second;
		rgn.x += x;
		rgn.y += y;
		if (!rgn.IntersectsRegion(drawRgn)) {
			break; // layout is ordered so we know nothing else can draw
		}
		video->BlitSprite((*it).first->RenderedSpan(),
						  rgn.x, rgn.y, true, &rgn);
#if DEBUG_TEXT
		// draw the layout rect
		video->DrawRect(rgn, ColorGreen, false);
		// draw the actual sprite boundaries
		rgn.x += (*it).first->RenderedSpan()->XPos;
		rgn.y += (*it).first->RenderedSpan()->YPos;
		rgn.w = (*it).first->RenderedSpan()->Width;
		rgn.h = (*it).first->RenderedSpan()->Height;
		video->DrawRect(rgn, ColorWhite, false);
#endif
	}
}

void TextContainer::LayoutSpansStartingAt(SpanList::const_iterator it)
{
	assert(it != spans.end());
	Point drawPoint(0, 0);
	TextSpan* span = *it;
	TextSpan* prevSpan = NULL;

	if (it != spans.begin()) {
		// get the next draw position to try
		prevSpan = *--it;
		const Region& rgn = layout[prevSpan];
		drawPoint.x = rgn.x + rgn.w + 1;
		drawPoint.y = rgn.y;
		it++;
	}

	for (; it != spans.end(); it++) {
		span = *it;
		const Size& spanFrame = span->SpanFrame();

		// FIXME: this only calculates left alignment
		// it also doesnt support block layout
		Region layoutRgn;
		const Region* excluded = NULL;
		do {
			if (excluded) {
				// we know that we have to move at least to the right
				// TODO: implement handling for block alignment
				drawPoint.x = excluded->x + excluded->w + 1;
			}
			frame.w = drawPoint.x;
			if (drawPoint.x && drawPoint.x + spanFrame.w > frame.w) {
				// move down and back
				drawPoint.x = 0;
				if (excluded) {
					drawPoint.y = excluded->y + spanFrame.h + 1;
				} else {
					const Region& prevFrame = layout[prevSpan];
					drawPoint.y = prevFrame.h + prevFrame.y + 1;
				}
			}
			// we should not infinitely loop
			assert(!excluded || drawPoint != Point(layoutRgn.x, layoutRgn.y));
			layoutRgn = Region(drawPoint, spanFrame);
			excluded = ExcludedRegionForRect(layoutRgn);
		} while (excluded);

		layout[span] = layoutRgn;
		// TODO: need to extend the exclusion rect for some alignments
		// eg right align should invalidate the entire area infront also
		AddExclusionRect(layoutRgn);
		prevSpan = span;
	}
	frame.h = drawPoint.y + span->SpanFrame().h;
	if (frame.h > maxFrame.h)
		frame.h = maxFrame.h;
	if (frame.w > maxFrame.w)
		frame.w = maxFrame.w;
}

void TextContainer::AddExclusionRect(const Region& rect)
{
	assert(!rect.Dimensions().IsEmpty());
	std::vector<Region>::iterator it;
	for (it = ExclusionRects.begin(); it != ExclusionRects.end(); ++it) {
		if (rect.InsideRegion(*it)) {
			// already have an encompassing region
			break;
		} else if ((*it).InsideRegion(rect)) {
			// new region swallows the old, replace it;
			*it = rect;
			break;
		}
	}
	if (it == ExclusionRects.end()) {
		// no match found
		ExclusionRects.push_back(rect);
	}
}

const Region* TextContainer::ExcludedRegionForRect(const Region& rect)
{
	std::vector<Region>::const_iterator it;
	for (it = ExclusionRects.begin(); it != ExclusionRects.end(); ++it) {
		if (rect.IntersectsRegion(*it))
			return &*it;
	}
	return NULL;
}

}
