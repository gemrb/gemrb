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
	: frame(Point(-1, -1), size)
{
	parent = NULL;
}

Content::~Content()
{}

Size Content::ContentFrame() const
{
	if (frame.Dimensions().IsEmpty() && !layoutRegions.empty()) {
		return Region::RegionEnclosingRegions(layoutRegions).Dimensions();
	}
	return frame.Dimensions();
}

void Content::Draw(Point p) const
{
	Size s(frame.Dimensions());
	s.h = (s.h <= 0) ? CONTENT_MAX_SIZE: s.h;
	s.w = (s.w <= 0) ? CONTENT_MAX_SIZE: s.w;
	DrawContents(Point(), Region(p, s));
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

void TextSpan::DrawContents(Point dp, const Region& rgn) const
{
	if (!parent && frame.Dimensions().IsEmpty()) {
		return;
	}

	Point drawOrigin = rgn.Origin();
	if (frame.Dimensions().IsZero()) {
		// this means we get to wrap :)
		// calculate each line and print line by line
		Regions lineExclusions;
		Region lineRgn(dp + drawOrigin, Size(rgn.w, font->maxHeight));
		Region lineSegment = lineRgn;

		const Region* excluded = NULL;
		size_t numPrinted = 0;
		do {
			newline:
			if (lineSegment.w == 0 || lineSegment.x + lineSegment.w > lineRgn.x + lineRgn.w) {
				// start next line
				lineRgn.x = drawOrigin.x;
				lineRgn.y += font->maxHeight;
				lineRgn.w = rgn.w;
				dp = lineRgn.Origin();

				// must claim the lineExclusions as part of the layout
				// just because we didnt fit doesnt mean somethng else wont...
				layoutRegions.push_back(Region::RegionEnclosingRegions(lineExclusions));

				lineExclusions.clear();
				lineSegment = lineRgn;
			}
			do {
				// process all overlaping exclusion zones until we trim down to the leftmost non conflicting region.
				// check for intersections with other content
				Size s = lineSegment.Intersect(rgn).Dimensions();
				Size stringSize = font->StringSize(text.substr(numPrinted, text.find_first_of(L' ')), &s);

				if (s.w > 0 && stringSize.w > s.w) {
					// we dont even have enough area to print a single word. skip this segment.
					// if the segment doesnt "exist" (s.w <= 0) it will be handled later
					excluded = &lineSegment;
					lineExclusions.push_back(lineSegment);
				} else {
					excluded = parent->ContentRegionForRect(lineSegment);
				}
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
						goto newline;
					}
				}
			} while (excluded);

#if (DEBUG_TEXT)
			core->GetVideoDriver()->DrawRect(lineSegment, ColorRed, true);
#endif
			// collapse with previous content (shared borders)
			lineSegment.y--;
			assert(lineSegment.h == font->maxHeight);
			Point printPoint;
			numPrinted += font->Print(lineSegment.Intersect(rgn), text.substr(numPrinted), palette, IE_FONT_ALIGN_LEFT, &printPoint);

			lineExclusions.push_back(lineSegment);
			if (printPoint.y) {
				// a newline occured; occupy the entire line area
				lineSegment.w = lineRgn.w;
				// in case the line has multiple line breaks ('\n') use the return point to determine the next line position
				lineRgn.y += printPoint.y - lineSegment.h;
			} else if (printPoint.x) {
				dp.x += printPoint.x;
				lineSegment.w = printPoint.x;
			}

#if (DEBUG_TEXT)
			core->GetVideoDriver()->DrawRect(lineSegment, ColorWhite, false);
#endif
			assert(lineSegment.h % font->maxHeight == 0);
			layoutRegions.push_back(lineSegment);

			// FIXME: infinite loop possibility.
		} while (numPrinted < text.length());
	} else {
		// we are limited to drawing within our frame :(

		// TODO: implement absolute positioning
		//dp.x += frame.x;
		//dp.y += frame.y;

		Region drawRegion = Region(dp + drawOrigin, frame.Dimensions());

		// FIXME: we ought to be able to set an alignment for "blocks" of text
		// probably the way to do this is have alignment on the container
		// then maybe another Draw method that takes an alignment argument?
		if (drawRegion.w <= 0) {
			Size max = Size(0, frame.h);
			Size ts = font->StringSize(text, &max);
			assert(ts.w && ts.h);
			drawRegion.w = (drawRegion.w > 0) ? drawRegion.w : ts.w;
		}

		// collapse with previous content (shared borders)
		drawRegion.y--;
		Point printPoint;
		if (drawRegion.h <= 0) {
			drawRegion.h = CONTENT_MAX_SIZE;
			font->Print(drawRegion.Intersect(rgn), text, palette, IE_FONT_ALIGN_LEFT, &printPoint);
			drawRegion.h = printPoint.y + font->maxHeight;
			assert(drawRegion.h % font->maxHeight == 0);
		} else {
			font->Print(drawRegion.Intersect(rgn), text, palette, IE_FONT_ALIGN_LEFT);
		}
#if (DEBUG_TEXT)
		core->GetVideoDriver()->DrawRect(drawRegion, ColorWhite, false);
#endif
		assert(drawRegion.h && drawRegion.w);
		layoutRegions.push_back(drawRegion);
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

void ImageSpan::DrawContents(Point dp, const Region& rgn) const
{
	layoutRegions.clear();
	Point p( dp.x + rgn.x, dp.y + rgn.y);
	core->GetVideoDriver()->BlitSprite(image, p.x, p.y, true, &rgn);
	layoutRegions.push_back(Region(p, Size(image->Width, image->Height)));
}


ContentContainer::~ContentContainer()
{
	ContentList::iterator it = contents.begin();
	for (; it != contents.end(); ++it) {
		delete *it;
	}
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
	} else {
		ContentList::iterator it;
		it = std::find(contents.begin(), contents.end(), existing);
		contents.insert(++it, newContent);
	}
}

Content* ContentContainer::RemoveContent(const Content* span)
{
	ContentList::iterator it;
	it = std::find(contents.begin(), contents.end(), span);
	if (it != contents.end()) {
		contents.erase(it);
		Content* content = *it;
		layout.erase(content);
		content->parent = NULL;
		return content;
	}
	return NULL;
}

Content* ContentContainer::ContentAtPoint(const Point& p) const
{
	return ContentAtScreenPoint(p + screenOffset);
}

Content* ContentContainer::ContentAtScreenPoint(const Point& p) const
{
	ContentLayout::iterator it = layout.begin();
	for (; it != layout.end(); ++it) {
		const Regions& rgns = (*it).second;
		Regions::const_iterator rit = rgns.begin();
		for (; rit != rgns.end(); ++rit) {
			if ((*rit).PointInside(p)) {
				return (*it).first;
			}
		}
	}

	return NULL;
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
	frame = newFrame;
}

void ContentContainer::DrawContents(Point dp, const Region& rgn) const
{
	screenOffset = rgn.Origin();
	// TODO: intersect with the screen clip so we can bail out even earlier

	// using a dynamic layout, may not be most efficient,
	// but its at least as fast as our previous (horrible) string printing implementation
	const Point& drawOrigin = screenOffset;
	Point drawPoint = dp + drawOrigin;
	int maxH = 0;
	Content* content = NULL;
	layoutRegions.clear();
	layout.clear();
	ContentList::const_iterator it = contents.begin();
	for (; it != contents.end(); ++it) {
		content = *it;

		content->layoutRegions.clear();
		content->DrawContents(drawPoint - drawOrigin, rgn);

		layout.insert(std::make_pair(content, content->layoutRegions));
		Regions::const_iterator rit = layout[content].begin();
		for (; rit != layout[content].end(); ++rit) {
			int h = ((*rit).y + (*rit).h) - drawOrigin.y - dp.y;
			maxH = (h > maxH) ? h : maxH;
		}
		if (it == --contents.end()) break; // dont care about calculating next layout

		const Region* excluded = NULL;
		do {
			excluded = NULL;
			Content* exContent = ContentAtScreenPoint(drawPoint);
			if (exContent) {
				Regions::const_iterator it = layout[exContent].begin();
				for (; it != layout[exContent].end(); ++it) {
					if ((*it).PointInside(drawPoint)) {
						excluded = &(*it);
						break;
					}
				}
			}
			if (excluded) {
				// we know that we have to move at least to the right
				drawPoint.x = excluded->x + excluded->w + 1;
				if (drawPoint.x > drawOrigin.x + frame.w) {
					drawPoint.x = drawOrigin.x;
					drawPoint.y = excluded->y + excluded->h + 1;
				}
			}
		} while (excluded);
	}

	Region layoutRegion = Region(drawOrigin + dp, Size(frame.w, maxH));
	layoutRegions.push_back(layoutRegion);
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


void RestrainedTextContainer::AppendContent(Content* newContent)
{
	while (contents.size() >= spanLimit) {
		// we need to remove the first span and any spans on the same "line"
		ContentList::iterator it = contents.begin();
		Content* content = *it;
		if (layout.find(content) == layout.end()) {
			delete RemoveContent(content);
			return; // no layout yet, cant possibly delete any other spans
		}

		Region rgn = layout[content].front();
		delete RemoveContent(content);
		it = contents.begin();
		while (it != contents.end()) {
			content = *it;
			if (layout.find(content) != layout.end()) {
				Regions::iterator rit = layout[content].begin();
				for (; rit != layout[content].end(); ++rit) {
					if ((*rit).y <= rgn.y + rgn.h) {
						it--;
						delete RemoveContent(content);
						break;
					}
				}
			} else {
				break; // no more layout available
			}
			it++;
		}
	}
	ContentContainer::AppendContent(newContent);
}

}
