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

#include "Interface.h"
#include "Palette.h"
#include "Sprite2D.h"
#include "System/String.h"
#include "Video.h"

namespace GemRB {

ContentSpan::ContentSpan()
	: frame()
{
	spanSprite = NULL;
}

ContentSpan::ContentSpan(const Size& size)
	: frame(size)
{
	spanSprite = NULL;
}

ContentSpan::~ContentSpan()
{
	if (spanSprite)
		spanSprite->release();
}

TextSpan::TextSpan(const String& string, Font* fnt, Palette* pal)
	: ContentSpan()
{
	Init(string, fnt, pal, 0);
}

TextSpan::TextSpan(const String& string, Font* fnt, Palette* pal, const Size& frame, ieByte align)
	: ContentSpan(frame)
{
	Init(string, fnt, pal, align);
}

void TextSpan::Init(const String& string, Font* fnt, Palette* pal, ieByte align)
{
	if (!pal) {
		palette = fnt->GetPalette();
		assert(palette);
	} else {
		palette = NULL;
		SetPalette(pal);
	}

	font = fnt;
	alignment = align;
	RenderSpan(string);
}

TextSpan::~TextSpan()
{
	palette->release();
}

void TextSpan::RenderSpan(const String& string)
{
	if (string.find_first_not_of(L"\n") == String::npos) {
		// entire string is newlines (no width, but still need to generate layout)
		frame.w = -1; // maximum value, nothing should ever go next to a newline
		// FIXME: this assumes a single newline and no more!
		frame.h = font->maxHeight; // newline is always a full line height
		stringLen = string.length();
		return;
	}
	if (spanSprite) spanSprite->release();
	// TODO: implement span alignments
	spanSprite = font->RenderTextAsSprite(string, frame, alignment, palette, &stringLen);
	// string is trimmed down to just the characters that fit.
	// some spans are created from huge strings but with a small size (the text next to a drop cap)
	// we may want a variation that keeps the entire string so the span can dynamically rerender
	text = string.substr(0, stringLen);

	if (text.find_last_of(L"\n") == text.length() - 1) {
		// if the span eneded in a newline we automatically know
		// following spans should only go below this one
		frame.w = (ieWord)-1;
	}

	// frame dimensions of 0 just mean size to fit
	if (frame.w == 0)
		frame.w = spanSprite->Width;
	if (frame.h == 0)
		frame.h = spanSprite->Height;
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



ImageSpan::ImageSpan(Sprite2D* image)
	: ContentSpan()
{
	assert(image);
	image->acquire();
	spanSprite = image;

	frame.w = spanSprite->Width;
	frame.h = spanSprite->Height;
}


ContentContainer::ContentContainer(const Size& frame, Font* font, Palette* pal)
	: maxFrame(frame), frame(), font(font)
{
	pal->acquire();
	pallete = pal;
}

ContentContainer::~ContentContainer()
{
	SpanList::iterator it = spans.begin();
	for (; it != spans.end(); ++it) {
		delete *it;
	}
	pallete->release();
}

void ContentContainer::AppendText(const String& text)
{
	if (text.length()) {
		Size stringSize = font->StringSize(text);
		if (stringSize.w < maxFrame.w)
			AppendSpan(new TextSpan(text, font, pallete));
		else
			AppendSpan(new TextSpan(text, font, pallete, Size(maxFrame.w, 0), 0));
	}
}

void ContentContainer::AppendSpan(ContentSpan* span)
{
	spans.push_back(span);
	LayoutSpansStartingAt(--spans.end());
}

void ContentContainer::InsertSpanAfter(ContentSpan* newSpan, const ContentSpan* existing)
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

ContentSpan* ContentContainer::RemoveSpan(const ContentSpan* span)
{
	SpanList::iterator it;
	it = std::find(spans.begin(), spans.end(), span);
	if (it != spans.end()) {
		LayoutSpansStartingAt(--spans.erase(it));
		return (ContentSpan*)span; // easiest to just cast away const, it would be the same pointer either way
	}
	return NULL;
}

void ContentContainer::ClearSpans()
{
	// FIXME: this isn't technically accurate
	Region ex = *--ExclusionRects.end();
	ex.w = maxFrame.w;
	ex.h = ex.y + ex.h;
	ex.y = 0;
	ex.x = 0;
	AddExclusionRect(ex);
}

ContentSpan* ContentContainer::SpanAtPoint(const Point& p) const
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

Point ContentContainer::PointForSpan(const ContentSpan* span)
{
	SpanList::iterator it;
	it = std::find(spans.begin(), spans.end(), span);
	if (it != spans.end()) {
		return layout[*it].Origin();
	}
	return Point(-1, -1);
}

void ContentContainer::SetSpanPadding(ContentSpan* span, Size pad)
{
	if (layout.find(span) == layout.end()) return;

	Region rgn = layout[span];
	rgn.x += pad.w;
	rgn.y += pad.h;
	layout[span] = rgn;
	rgn.y -= span->SpanDescent();
	// TODO: the span should be informed so that it can readjust the position of the sprite according to its alignment
	AddExclusionRect(rgn);
}

void ContentContainer::DrawContents(int x, int y) const
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
	ContentSpan* span = NULL;
	for (it = layout.begin(); it != layout.end(); ++it) {
		span = (*it).first;
		Region rgn = (*it).second;
		rgn.x += x;
		rgn.y += y;
		if (!rgn.IntersectsRegion(drawRgn)) {
			break; // layout is ordered so we know nothing else can draw
		}
		const Sprite2D* spr = span->RenderedSpan();
		video->BlitSprite(spr, rgn.x, rgn.y, true, &rgn);
#if DEBUG_TEXT
		// draw the layout rect
		video->DrawRect(rgn, ColorGreen, false);
		// draw the actual sprite boundaries
		rgn.x += span->RenderedSpan()->XPos;
		rgn.y += span->RenderedSpan()->YPos;
		rgn.w = span->RenderedSpan()->Width;
		rgn.h = span->RenderedSpan()->Height;
		video->DrawRect(rgn, ColorWhite, false);
#endif
	}
}

void ContentContainer::LayoutSpansStartingAt(SpanList::const_iterator it)
{
	assert(it != spans.end());
	Point drawPoint(0, 0);
	ContentSpan* span = *it;
	ContentSpan* prevSpan = NULL;

	if (it != spans.begin()) {
		// get the next draw position to try
		prevSpan = *--it;
		if (layout.find(prevSpan) != layout.end()) {
			const Region& rgn = layout[prevSpan];
			drawPoint.y = rgn.y;
			drawPoint.x = rgn.x + rgn.w;
		}
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
				drawPoint.x = excluded->x + excluded->w;
				if (drawPoint.x <= 0) // newline ?
					drawPoint.x = maxFrame.w;
			}
			if (drawPoint.x && drawPoint.x + spanFrame.w > maxFrame.w) {
				// move down and back
				drawPoint.x = 0;
				if (excluded) {
					drawPoint.y = excluded->y + excluded->h;
				} else {
					const Region& prevFrame = layout[prevSpan];
					drawPoint.y = prevFrame.h + prevFrame.y - prevSpan->SpanDescent();
				}
			}
			// we should not infinitely loop
			assert(!excluded || drawPoint != Point(layoutRgn.x, layoutRgn.y));
			layoutRgn = Region(drawPoint, spanFrame);
			excluded = ExcludedRegionForRect(layoutRgn);
		} while (excluded);

		frame.w = (layoutRgn.w > frame.w) ? layoutRgn.w : frame.w;
		if (span->RenderedSpan()) {
			// only layout redered spans
			assert(!layoutRgn.Dimensions().IsEmpty());
			layout[span] = layoutRgn;
		}
		// TODO: need to extend the exclusion rect for some alignments
		// eg right align should invalidate the entire area infront also
		layoutRgn.h -= span->SpanDescent();
		assert(layoutRgn.h > 0);
		AddExclusionRect(layoutRgn);
		prevSpan = span;
	}
	// TODO: we could optimize by testing *before* we try to add/layout a new span
	// we either shouldnt accept new spans that dont fit or at leat not lay them out
	// currently we cant resize a container dynamically so...
	ieWord newh = drawPoint.y + span->SpanFrame().h;
	if (newh > frame.h) {
		frame.h = newh;
		if (frame.h > maxFrame.h)
			frame.h = maxFrame.h;
	}
	if (frame.w > maxFrame.w)
		frame.w = maxFrame.w;
}

void ContentContainer::SetMaxFrame(const Size& newSize) {
	// TODO: need to relayout if width changed, or if hew height is greater
	maxFrame = newSize;
}

void ContentContainer::AddExclusionRect(const Region& rect)
{
	assert(!rect.Dimensions().IsEmpty());
	std::vector<Region>::iterator it;
	for (it = ExclusionRects.begin(); it != ExclusionRects.end(); ++it) {
		if (rect.InsideRegion(*it)) {
			// already have an encompassing region
			break;
		} else if ((*it).InsideRegion(rect)) {
			// cant just replace and break. may have eaten more than one...
			it = ExclusionRects.erase(it);
			if (it == ExclusionRects.end()) break;
		}
	}
	if (it == ExclusionRects.end()) {
		// no match found
		ExclusionRects.push_back(rect);
	}
}

const Region* ContentContainer::ExcludedRegionForRect(const Region& rect)
{
	std::vector<Region>::const_iterator it;
	for (it = ExclusionRects.begin(); it != ExclusionRects.end(); ++it) {
		if (rect.IntersectsRegion(*it))
			return &*it;
	}
	return NULL;
}



void RestrainedContentContainer::AppendSpan(ContentSpan* span)
{
	while (spans.size() >= spanLimit) {
		// we need to remove the first span and any spans on the same "line"
		SpanList::iterator it = spans.begin();
		ContentSpan* span = *it;
		Region rgn = layout[span];

		// extend the region all the way across the container and up to the top
		rgn.x = 0;
		rgn.w = maxFrame.w;
		rgn.h += rgn.y;
		rgn.h -= span->SpanDescent();
		rgn.y = 0;
		// now iterate the list and erase until we find a span that doesnt intersect
		do {
			span = *it;
			if (!rgn.IntersectsRegion(layout[span])) {
				break;
			}
			layout.erase(span);
			delete span;
		} while (++it != spans.end());
		spans.erase(spans.begin(), it);

		// adjust the remainder
		int offset = layout[spans.front()].y;
		int h = 0;
		for (it = spans.begin(); it != spans.end(); ++it) {
			Region& r = layout[*it];
			r.y -= offset;
			h += r.h;
		}

		// destroy all exclusions and recreate a single exclusion
		ExclusionRects.clear();
		if (h) {
			rgn.h = h;
			AddExclusionRect(rgn);
		}
	}
	ContentContainer::AppendSpan(span);
}

}
