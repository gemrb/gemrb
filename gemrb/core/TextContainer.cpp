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

#define CONTENT_MAX_SIZE (SHRT_MAX / 2) // just something larger than any screen height and small enough to not overflow
#define DEBUG_TEXT 0

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
	Region layoutRgn = Region(rgn.Origin() + p, frame.Dimensions());

	Regions rgns;
	rgns.push_back(layoutRgn);
	return rgns;
}

void Content::Draw(Point p) const
{
	Size s(frame.Dimensions());
	s.h = (s.h <= 0) ? CONTENT_MAX_SIZE: s.h;
	s.w = (s.w <= 0) ? CONTENT_MAX_SIZE: s.w;

#if DEBUG_TEXT
	Region drawRgn(p, s);
	core->GetVideoDriver()->DrawRect(drawRgn, ColorGreen, true);
#endif
	// FIXME: passing around a screen offset is clumsy.
	// there should be a way to have the video dirver draw relative to a given rect
	// we *almost* have this functionality, but it is tied to the gamecontrol viewport at the moment

	// this is the root of the drawing so region and point are both at 0,0
	Point origin;
	DrawContentsInRegions(LayoutForPointInRegion(origin, Region(origin, s)), p);
}


TextSpan::TextSpan(const String& string, Font* fnt, Palette* pal, const Size* frame)
	: Content((frame) ? *frame : Size()), text(string), font(fnt)
{
	palette = NULL;
	SetPalette(pal);
}

TextSpan::~TextSpan()
{
	palette->release();
}

Regions TextSpan::LayoutForPointInRegion(Point layoutPoint, const Region& rgn) const
{
	Regions layoutRegions;
	const Point& drawOrigin = rgn.Origin();
	if (frame.Dimensions().IsZero()) {
		// this means we get to wrap :)
		// calculate each line and print line by line
		Regions lineExclusions;
		Region lineRgn(layoutPoint + drawOrigin, Size(rgn.w, font->maxHeight));
		lineRgn.y -= font->maxHeight;
		Region lineSegment;

		const Region* excluded = NULL;
		size_t numPrinted = 0;
		bool newline = true;
		do {
			if (newline || lineSegment.x + lineSegment.w >= lineRgn.x + lineRgn.w) {
				// start next line
				newline = false;
				lineRgn.x = drawOrigin.x;
				lineRgn.y += font->maxHeight;
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
						newline = true;
						goto newline;
					}
				}
			} while (excluded);

			{ // protected scope for goto
				assert(lineSegment.h == font->maxHeight);
				size_t numOnLine = 0;
				// must limit our operation to this single line.
				size_t nextLine = text.find_first_of(L"\n", numPrinted);
				if (nextLine == numPrinted) {
					// this is a new line, we dont have to actually size that
					// simply occupy the entire area and advance.
					lineSegment.w = lineRgn.w;
					numOnLine = 1;
					newline = true;
				} else {
					Size printMax = lineSegment.Dimensions();
					Size printSize = font->StringSize(text.substr(numPrinted, nextLine), &printMax, &numOnLine);
					if (printMax.w == lineRgn.w && numPrinted + numOnLine < text.length()) {
						// optimization for when the segment is the entire line (and we have more text)
						// saves looping again for the known to be useless segment
						newline = true;
						lineSegment.w = lineRgn.w;
					} else {
						lineSegment.w = printSize.w;
					}
				}
				numPrinted += numOnLine;
			}
			lineExclusions.push_back(lineSegment);

		newline:
			if (newline || numPrinted == text.length()) {
				// must claim the lineExclusions as part of the layout
				// just because we didnt fit doesnt mean somethng else wont...
				Region lineLayout = Region::RegionEnclosingRegions(lineExclusions);
				//lineLayout.h--; // we want the following content to "collapse" with this rect
				assert(lineLayout.h % font->maxHeight == 0);
				layoutRegions.push_back(lineLayout);
				lineExclusions.clear();
			}
			// FIXME: infinite loop possibility.
		} while (numPrinted < text.length());
		assert(numPrinted == text.length());
	} else {
		// we are limited to drawing within our frame :(

		// TODO: implement absolute positioning
		//dp.x += frame.x;
		//dp.y += frame.y;

		// FIXME: we ought to be able to set an alignment for "blocks" of text
		// probably the way to do this is have alignment on the container
		// then maybe another Draw method that takes an alignment argument?

		Size maxSize = frame.Dimensions();
		Region drawRegion = Region(layoutPoint + drawOrigin, maxSize);
		Size stringSize = font->StringSize(text, &maxSize);
		if (maxSize.w <= 0) {
			drawRegion.w = stringSize.w;
		}
		if (maxSize.h <= 0) {
			drawRegion.h = stringSize.h;
		}

		//drawRegion.h--; // we want the following content to "collapse" with this rect
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
#if (DEBUG_TEXT)
		core->GetVideoDriver()->DrawRect(drawRect, ColorRed, true);
#endif
		charsPrinted += font->Print(drawRect, text.substr(charsPrinted), palette, IE_FONT_ALIGN_LEFT);
#if (DEBUG_TEXT)
		//core->GetVideoDriver()->DrawRect(drawRect, ColorWhite, false);
#endif
	}
}

void TextSpan::SetPalette(Palette* pal)
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

Size ContentContainer::ContentFrame() const
{
	Size cf = Content::ContentFrame();
	if (frame.w <= 0) {
		cf.w = contentBounds.w;
	}
	if (frame.h <= 0) {
		cf.h = contentBounds.h;
	}
	return cf;
}

void ContentContainer::AppendContent(Content* content)
{
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
		layout.erase(content);
		if (doLayout) {
			LayoutContentsFrom(it);
		}
		return content;
	}
	return NULL;
}

Content* ContentContainer::ContentAtPoint(const Point& p) const
{
	ContentLayout::const_reverse_iterator it = layout.rbegin();
	for (; it != layout.rend(); ++it) {
		const Regions& rgns = (*it).second;
		Regions::const_iterator rit = rgns.begin();
		for (; rit != rgns.end(); ++rit) {
			if ((*rit).PointInside(p)) {
				// i know we are casting away const.
				// we could return sdt::find(contents.begin(), contents.end(), *it) instead, but whats the point?
				return (Content*)(*it).first;
			}
		}
	}
	return NULL;
}

Region ContentContainer::BoundingBoxForContent(const Content* c) const
{
	return Region::RegionEnclosingRegions(layout.at(c));
}

const Region* ContentContainer::ContentRegionForRect(const Region& r) const
{
	ContentLayout::const_iterator it = layout.begin();
	for (; it != layout.end(); ++it) {
		const Regions& rgns = (*it).second;
		Regions::const_iterator rit = rgns.begin();
		for (; rit != rgns.end(); ++rit) {
			if ((*rit).IntersectsRegion(r)) {
				return &(*rit);
			}
		}
	}
	return NULL;
}

void ContentContainer::SetFrame(const Region& newFrame)
{
	if (newFrame.Dimensions() != frame.Dimensions()) {
		frame = newFrame; // must assign new frame before calling LayoutContents
		LayoutContentsFrom(contents.begin());
	} else {
		frame = newFrame;
	}
}

Regions ContentContainer::LayoutForPointInRegion(Point p, const Region&) const
{
	Region layoutRgn(p, ContentFrame());

	Regions rgns;
	rgns.push_back(layoutRgn);
	return rgns;
}

void ContentContainer::LayoutContentsFrom(const Content* c)
{
	LayoutContentsFrom(std::find(contents.begin(), contents.end(), c));
}

void ContentContainer::LayoutContentsFrom(ContentList::const_iterator it)
{
	const Content* exContent = NULL;
	const Region* excluded = NULL;
	Point layoutPoint;

	if (it != contents.begin()) {
		// relaying content some place in the middle of the container
		exContent = *--it;
		it++;
		assert(layout.find(exContent) != layout.end());
	}

	while (it != contents.end()) {
		const Content* content = *it++;
		while (exContent) {
			Regions::const_iterator rit = layout[exContent].begin();
			for (; rit != layout[exContent].end(); ++rit) {
				if ((*rit).PointInside(layoutPoint)) {
					excluded = &(*rit);
					break;
				}
			}
			if (excluded) {
				// we know that we have to move at least to the right
				layoutPoint.x = excluded->x + excluded->w + 1;
				if (frame.w > 0 && layoutPoint.x >= frame.w) {
					layoutPoint.x = 0;
					assert(excluded->y + excluded->h >= layoutPoint.y);
					layoutPoint.y = excluded->y + excluded->h + 1;
				}
			}
			exContent = ContentAtPoint(layoutPoint);
			assert(exContent != content);
		}
		const Regions& rgns = content->LayoutForPointInRegion(layoutPoint, frame);
		layout[content] = rgns;
		Region bounds = Region::RegionEnclosingRegions(rgns);
		contentBounds.h = (bounds.y + bounds.h > contentBounds.h) ? bounds.y + bounds.h : contentBounds.h;
		contentBounds.w = (bounds.x + bounds.w > contentBounds.w) ? bounds.x + bounds.w : contentBounds.w;
		exContent = content;
	}
	if (parent) {
		// the parent needs to update to compensate for changes in this container
		parent->LayoutContentsFrom(this);
	}
}

void ContentContainer::DrawContentsInRegions(const Regions& rgns, const Point& offset) const
{
	// layout shouldn't be empty unless there is no content anyway...
	if (layout.empty()) return;

	// should only have 1 region
	const Region& rgn = rgns.front();

	// TODO: intersect with the screen clip so we can bail out even earlier

	const Point& drawOrigin = rgn.Origin();
	Point drawPoint = drawOrigin;
	ContentList::const_iterator it = contents.begin();
	for (; it != contents.end(); ++it) {
		const Content* content = *it;

		assert(drawPoint.x <= drawOrigin.x + frame.w);
		content->DrawContentsInRegions(layout.at(content), offset);
	}
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


TextContainer::TextContainer(const Size& frame, Font* fnt, Palette* pal)
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
	AppendText(text, font, palette);
}

void TextContainer::AppendText(const String& text, Font* fnt, Palette* pal)
{
	if (text.length()) {
		AppendContent(new TextSpan(text, fnt, pal));
	}
}

const String& TextContainer::Text() const
{
	// iterate all the content and pick out the TextSpans and concatonate them into a single string
	static String text;
	ContentList::const_iterator it = contents.begin();
	for (; it != contents.end(); ++it) {
		if (const TextSpan* textSpan = dynamic_cast<TextSpan*>(*it)) {
			// FIXME: this will produce odd results since adjacent spans wont be separated by any whitespace
			text.append(textSpan->Text());
		}
	}
	return text;
}

}
