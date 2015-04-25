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

#include <algorithm>
#include <climits>

#define DEBUG_TEXT 1

namespace GemRB {

Content::Content(const Size& size)
	: frame(Point(0, 0), size)
{
	parent = NULL;
}

Content::~Content()
{}

Regions Content::LayoutForPointInRegion(Point p, const Region& rgn) const
{
	const Region& layoutRgn = Region(rgn.Origin() + p, frame.Dimensions());
	return Regions(1, layoutRgn);
}

TextSpan::TextSpan(const String& string, const Font* fnt, Palette* pal, const Size* frame)
	: Content((frame) ? *frame : Size()), text(string), font(fnt)
{
	palette = pal;
	if (palette)
		palette->acquire();
}

TextSpan::~TextSpan()
{
	if (palette)
		palette->release();
}

inline const Font* TextSpan::LayoutFont() const
{
	if (font) return font;

	TextContainer* container = dynamic_cast<TextContainer*>(parent);
	if (container) {
		return container->TextFont();
	}
	return NULL;
}

inline Region TextSpan::LayoutInFrameAtPoint(const Point& p, const Region& rgn) const
{
	const Font* layoutFont = LayoutFont();
	Size maxSize = frame.Dimensions();
	Region drawRegion = Region(p + rgn.Origin(), maxSize);
	if (maxSize.w <= 0) {
		if (maxSize.w == -1) {
			// take remainder of parent width
			drawRegion.w = rgn.w - p.x;
			maxSize.w = drawRegion.w;
		} else {
			Font::StringSizeMetrics metrics = {maxSize, 0, true};
			drawRegion.w = layoutFont->StringSize(text, &metrics).w;
		}
	}
	if (maxSize.h <= 0) {
		if (maxSize.h == -1) {
			// take remainder of parent height
			drawRegion.h = rgn.w - p.y;
		} else {
			Font::StringSizeMetrics metrics = {maxSize, 0, true};
			drawRegion.h = layoutFont->StringSize(text, &metrics).h;
		}
	}
	return drawRegion;
}

Regions TextSpan::LayoutForPointInRegion(Point layoutPoint, const Region& rgn) const
{
	Regions layoutRegions;
	const Point& drawOrigin = rgn.Origin();
	const Font* layoutFont = LayoutFont();
	assert(layoutFont);

	if (frame.Dimensions().IsZero()) {
		// this means we get to wrap :)
		// calculate each line and print line by line
		int lineheight = layoutFont->LineHeight;
		Regions lineExclusions;
		Region lineRgn(layoutPoint + drawOrigin, Size(rgn.w, lineheight));
		lineRgn.y -= lineheight;
		Region lineSegment;

#define LINE_REMAINDER lineRgn.w - lineSegment.x;
		const Region* excluded = NULL;
		size_t numPrinted = 0;
		bool newline = true;
		do {
			if (newline || lineSegment.x + lineSegment.w >= lineRgn.x + lineRgn.w) {
				// start next line
				newline = false;
				lineRgn.x = drawOrigin.x;
				lineRgn.y += lineheight;
				lineRgn.w = rgn.w;
				layoutPoint = lineRgn.Origin();

				lineSegment = lineRgn;
			}
			do {
				// process all overlaping exclusion zones until we trim down to the leftmost non conflicting region.
				// check for intersections with other content
				excluded = parent->ContentRegionForRect(lineSegment);
				if (!excluded) {
					// now check if we already used this region ourselves
					std::vector<Region>::const_iterator it;
					for (it = lineExclusions.begin(); it != lineExclusions.end(); ++it) {
						if (lineSegment.IntersectsRegion(*it)) {
							excluded = &*it;
							break;
						}
					}
				} // no else!
				if (excluded) {
					Region intersect = excluded->Intersect(lineSegment);
					if (intersect.x > lineSegment.x) { // to the right, simply shrink the width
						lineSegment.w = intersect.x - lineSegment.x;
					} else { // overlaps our start point, jump to the right of intersect
						int x = lineSegment.x;
						lineSegment.x = intersect.x + intersect.w;
						// must shrink to compensate for moving x
						lineSegment.w -= lineSegment.x - x;
					}

					// its possible that the resulting segment is 0 in width
					if (lineSegment.w <= 0) {
						lineSegment.x = intersect.x + intersect.w;
						lineSegment.w = LINE_REMAINDER;
						lineExclusions.push_back(lineSegment);
						newline = true;
						goto newline;
					}
				}
			} while (excluded);

			{ // protected scope for goto
				assert(lineSegment.h == lineheight);
				size_t numOnLine = 0;
				// must limit our operation to this single line.
				size_t nextLine = text.find_first_of(L'\n', numPrinted);
				if (nextLine == numPrinted) {
					// this is a new line, we dont have to actually size that
					// simply occupy the entire area and advance.
					lineSegment.w = LINE_REMAINDER;
					numOnLine = 1;
					newline = true;
				} else {
					size_t subLen = nextLine;
					if (nextLine != String::npos) {
						subLen = nextLine - numPrinted + 1; // +1 for the \n
					}
					const String& substr = text.substr(numPrinted, subLen);
					Font::StringSizeMetrics metrics = {lineSegment.Dimensions(), 0, lineSegment.w == lineRgn.w};
					Size printSize = layoutFont->StringSize(substr, &metrics);
					numOnLine = metrics.numChars;
					assert(numOnLine || !metrics.forceBreak);

					bool noFit = !metrics.forceBreak && numOnLine == 0;
					bool lineFilled = lineSegment.x + lineSegment.w == lineRgn.w;
					bool moreChars = numPrinted + numOnLine < text.length();
					if (subLen != String::npos || noFit || (lineFilled && moreChars)) {
						// optimization for when the segment is the entire line (and we have more text)
						// saves looping again for the known to be useless segment
						newline = true;
						lineSegment.w = LINE_REMAINDER;
					} else {
						assert(printSize.w);
						lineSegment.w = printSize.w;
					}
				}
				numPrinted += numOnLine;
			}
			assert(!lineSegment.Dimensions().IsEmpty());
			lineExclusions.push_back(lineSegment);

		newline:
			if (newline || numPrinted == text.length()) {
				// must claim the lineExclusions as part of the layout
				// just because we didnt fit doesnt mean somethng else wont...
				Region lineLayout = Region::RegionEnclosingRegions(lineExclusions);
				assert(lineLayout.h % lineheight == 0);
				layoutRegions.push_back(lineLayout);
				lineExclusions.clear();
			}
			// FIXME: infinite loop possibility.
		} while (numPrinted < text.length());
#undef LINE_REMAINDER
		assert(numPrinted == text.length());
	} else {
		// we are limited to drawing within our frame :(

		// FIXME: we ought to be able to set an alignment for "blocks" of text
		// probably the way to do this is have alignment on the container
		// then maybe another Draw method that takes an alignment argument?

		Region drawRegion = LayoutInFrameAtPoint(layoutPoint, rgn);
		assert(drawRegion.h && drawRegion.w);
		layoutRegions.push_back(drawRegion);
	}
	return layoutRegions;
}

void TextSpan::DrawContentsInRegions(const Regions& rgns, const Point& offset) const
{
	size_t charsPrinted = 0;
	Regions::const_iterator rit = rgns.begin();
	for (; rit != rgns.end(); ++rit) {
		Region drawRect = *rit;
		drawRect.x += offset.x;
		drawRect.y += offset.y;
		const Font* printFont = font;
		Palette* printPalette = palette;
		TextContainer* container = dynamic_cast<TextContainer*>(parent);
		if (printFont == NULL && container) {
			printFont = container->TextFont();
		}
		if (printPalette == NULL && container) {
			printPalette = container->TextPalette();
		}
		assert(printFont && printPalette);
		assert(charsPrinted < text.length());
#if (DEBUG_TEXT)
		core->GetVideoDriver()->DrawRect(drawRect, ColorRed, true);
#endif
		charsPrinted += printFont->Print(drawRect, text.substr(charsPrinted), printPalette, IE_FONT_ALIGN_LEFT);
#if (DEBUG_TEXT)
		core->GetVideoDriver()->DrawRect(drawRect, ColorWhite, false);
#endif
	}
}

ImageSpan::ImageSpan(Sprite2D* im)
	: Content(Size(im->Width, im->Height))
{
	assert(im);
	im->acquire();
	image = im;
}

void ImageSpan::DrawContentsInRegions(const Regions& rgns, const Point& offset) const
{
	// we only care about the first region... (should only be 1 anyway)
	Region r = rgns.front();
	r.x += offset.x;
	r.y += offset.y;
	core->GetVideoDriver()->BlitSprite(image, r.x, r.y, true, &r);
}

ContentContainer::~ContentContainer()
{
	ContentList::iterator it = contents.begin();
	for (; it != contents.end(); ++it) {
		delete *it;
	}
}

void ContentContainer::DrawSelf(Region drawFrame, const Region& clip)
{
#if DEBUG_TEXT
	core->GetVideoDriver()->DrawRect(clip, ColorGreen, true);
#endif

	// layout shouldn't be empty unless there is no content anyway...
	if (layout.empty()) return;

	ContentLayout::const_iterator it = layout.begin();
	for (; it != layout.end(); ++it) {
		const Layout& l = *it;
		// TODO: pass the clip rect so we can skip non-intersecting regions
		l.content->DrawContentsInRegions(l.regions, drawFrame.Origin());
	}
}

void ContentContainer::AppendContent(Content* content)
{
	if (contents.empty())
		InsertContentAfter(content, 0);
	else
		InsertContentAfter(content, *(--contents.end()));
}

void ContentContainer::InsertContentAfter(Content* newContent, const Content* existing)
{
	newContent->parent = this;
	if (!existing) { // insert at beginning;
		contents.push_front(newContent);
		LayoutContentsFrom(contents.begin());
	} else {
		ContentList::iterator it;
		it = std::find(contents.begin(), contents.end(), existing);
		contents.insert(++it, newContent);
		LayoutContentsFrom(--it);
	}
}

void ContentContainer::SizeChanged(const Size& /*oldSize*/)
{
	if (frame.w <= 0) {
		resizeFlags |= RESIZE_WIDTH;
	}
	if (frame.h <= 0) {
		resizeFlags |= RESIZE_HEIGHT;
	}
	LayoutContentsFrom(contents.begin());
}

void ContentContainer::SubviewAdded(View* view, View* parent)
{
	if (parent == this) {
		// ContentContainer should grow to the size of its (immidiate) subviews automatically
		const Region& subViewFrame = view->Frame();
		Size s;
		s.h = subViewFrame.y + subViewFrame.h;
		s.h = (s.h > frame.h) ? s.h : frame.h;
		s.w = subViewFrame.x + subViewFrame.w;
		s.w = (s.w > frame.w) ? s.w : frame.w;
		SetFrameSize(s);
	}
}

Content* ContentContainer::RemoveContent(const Content* span)
{
	return RemoveContent(span, true);
}

Content* ContentContainer::RemoveContent(const Content* span, bool doLayout)
{
	ContentList::iterator it;
	it = std::find(contents.begin(), contents.end(), span);
	if (it != contents.end()) {
		Content* content = *it;
		it = contents.erase(it);
		content->parent = NULL;
		layout.erase(std::find(layout.begin(), layout.end(), content));

		layoutPoint = Point(); // reset cached layoutPoint
		if (doLayout) {
			LayoutContentsFrom(it);
		}
		return content;
	}
	return NULL;
}

Content* ContentContainer::ContentAtPoint(const Point& p) const
{
	// attempting to optimize the search by assuming content is evenly distributed vertically
	// we are also assuming that layout regions are always contiguous and ordered ltr-ttb
	// we do know by definition that the content itself is ordered ltr-ttb
	// based on the above a simple binary search should suffice

	ContentLayout::const_iterator it = layout.begin();
	size_t count = layout.size();
	while (count > 0) {
		size_t step = count / 2;
		std::advance(it, step);
		if ((*it).PointInside(p)) {
			// i know we are casting away const.
			// we could return std::find(contents.begin(), contents.end(), *it) instead, but whats the point?
			return (Content*)(*it).content;
		}
		if (*it < p) {
			it++;
			count -= step + 1;
		} else {
			std::advance(it, -step);
			count = step;
		}
	}
	return NULL;
}

Region ContentContainer::BoundingBoxForContent(const Content* c) const
{
	return Region::RegionEnclosingRegions(LayoutForContent(c).regions);
}

const ContentContainer::Layout& ContentContainer::LayoutForContent(const Content* c) const
{
	ContentLayout::const_iterator it = std::find(layout.begin(), layout.end(), c);
	if (it != layout.end()) {
		return *it;
	}
	static Layout NullLayout(NULL, Regions());
	return NullLayout;
}

const Region* ContentContainer::ContentRegionForRect(const Region& r) const
{
	ContentLayout::const_iterator it = layout.begin();
	for (; it != layout.end(); ++it) {
		const Regions& rgns = (*it).regions;
		Regions::const_iterator rit = rgns.begin();
		for (; rit != rgns.end(); ++rit) {
			if ((*rit).IntersectsRegion(r)) {
				return &(*rit);
			}
		}
	}
	return NULL;
}

void ContentContainer::LayoutContentsFrom(const Content* c)
{
	LayoutContentsFrom(std::find(contents.begin(), contents.end(), c));
}

void ContentContainer::LayoutContentsFrom(ContentList::const_iterator it)
{
	if (it == contents.end()) {
		return; // must bail or things will get screwed up!
	}

	const Content* exContent = NULL;
	const Region* excluded = NULL;

	if (it != contents.begin()) {
		// relaying content some place in the middle of the container
		exContent = *--it;
		it++;
	}
	// clear the existing layout, but only for "it" and onward
	ContentList::const_iterator clearit = it;
	for (; clearit != contents.end(); ++clearit) {
		ContentLayout::iterator i = std::find(layout.begin(), layout.end(), *clearit);
		if (i != layout.end()) {
			layoutPoint = Point(); // reset cached layoutPoint
			layout.erase(i);
		}
	}

	Size contentBounds = Dimensions();
	Region layoutFrame = Region(Point(), contentBounds);
	if (resizeFlags&RESIZE_WIDTH) {
		layoutFrame.w = SHRT_MAX;
	}
	if (resizeFlags&RESIZE_HEIGHT) {
		layoutFrame.h = SHRT_MAX;
	}

	assert(!layoutFrame.Dimensions().IsEmpty());
	while (it != contents.end()) {
		const Content* content = *it++;
		while (exContent) {
			const Regions& rgns = LayoutForContent(exContent).regions;
			Regions::const_iterator rit = rgns.begin();
			for (; rit != rgns.end(); ++rit) {
				if ((*rit).PointInside(layoutPoint)) {
					excluded = &(*rit);
					break;
				}
			}
			if (excluded) {
				// we know that we have to move at least to the right
				layoutPoint.x = excluded->x + excluded->w;
				if (frame.w > 0 && layoutPoint.x >= layoutFrame.w) {
					layoutPoint.x = 0;
					assert(excluded->y + excluded->h >= layoutPoint.y);
					layoutPoint.y = excluded->y + excluded->h;
				}
			}
			exContent = ContentAtPoint(layoutPoint);
			assert(exContent != content);
		}
		const Regions& rgns = content->LayoutForPointInRegion(layoutPoint, layoutFrame);
		layout.push_back(Layout(content, rgns));
		exContent = content;

		if (resizeFlags) {
		const Region& bounds = Region::RegionEnclosingRegions(rgns);
			if (resizeFlags&RESIZE_HEIGHT)
		contentBounds.h = (bounds.y + bounds.h > contentBounds.h) ? bounds.y + bounds.h : contentBounds.h;
			if (resizeFlags&RESIZE_WIDTH)
		contentBounds.w = (bounds.x + bounds.w > contentBounds.w) ? bounds.x + bounds.w : contentBounds.w;
	}
	}

	// avoid infinite layout loop by setting frame directly...
	//SetFrameSize(contentBounds);
	frame.w = contentBounds.w;
	frame.h = contentBounds.h;
}

void ContentContainer::DeleteContentsInRect(Region exclusion)
{
	int top = exclusion.y;
	int bottom = top;
	const Content* content;
	while (const Region* rgn = ContentRegionForRect(exclusion)) {
		content = ContentAtPoint(rgn->Origin());
		assert(content);

		top = (rgn->y < top) ? rgn->y : top;
		bottom = (rgn->y + rgn->h > bottom) ? rgn->y + rgn->h : bottom;
		// must delete content last!
		delete RemoveContent(content, false);
	}

	// TODO: we could optimize this to only layout content after exclusion.y
	LayoutContentsFrom(contents.begin());
}


TextContainer::TextContainer(const Region& frame, Font* fnt, Palette* pal)
	: ContentContainer(frame), font(fnt)
{
	if (!pal) {
		palette = font->GetPalette();
	} else {
		pal->acquire();
		palette = pal;
	}
}

TextContainer::~TextContainer()
{
	palette->release();
}

void TextContainer::AppendText(const String& text)
{
	AppendText(text, NULL, NULL);
}

void TextContainer::AppendText(const String& text, Font* fnt, Palette* pal)
{
	if (text.length()) {
		AppendContent(new TextSpan(text, fnt, pal));
	}
}

void TextContainer::SetPalette(Palette* pal)
{
	if (!pal) {
		pal = font->GetPalette();
	} else {
		pal->acquire();
	}
	if (palette)
		palette->release();
	palette = pal;
}

String TextContainer::Text() const
{
	// iterate all the content and pick out the TextSpans and concatonate them into a single string
	String text;
	ContentList::const_iterator it = contents.begin();
	for (; it != contents.end(); ++it) {
		if (const TextSpan* textSpan = dynamic_cast<TextSpan*>(*it)) {
			text.append(textSpan->Text());
		}
	}
	return text;
}

}
