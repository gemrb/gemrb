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

#define CONTENT_MAX_SIZE (SHRT_MAX / 2) // just something larger than any screen height and small enough to not overflow

namespace GemRB {

Content::Content(const Size& size)
	: frame(Point(-1, -1), size)
{
	parent = NULL;
}

Content::~Content()
{}

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
			if (numPrinted) {
				if (lineSegment.w == 0 || lineSegment.x + lineSegment.w > lineRgn.x + lineRgn.w) {
					// start next line
					lineRgn.x = drawOrigin.x;
					lineRgn.y += font->maxHeight;
					lineRgn.w = rgn.w;
					dp = lineRgn.Origin();
					lineExclusions.clear();
					lineSegment = lineRgn;
				} else {
					// we have to add the segment to the container exclusions so that the next iteration works
					lineExclusions.push_back(lineSegment); // FIXME: if we want an implicit newline, use lineRgn instead.
				}
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
				}
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
						assert(numPrinted);
						goto newline;
					}
				}
			} while (excluded);

			Point printPoint;
			// collapse with previous content (shared borders)
			lineSegment.y--;
			assert(lineSegment.h == font->maxHeight);
			numPrinted += font->Print(lineSegment.Intersect(rgn), text.substr(numPrinted), palette, IE_FONT_ALIGN_LEFT, &printPoint);
			// FIXME: maybe handle this by bailing on the draw
			assert(numPrinted); // if we didnt print at all there will be an infinite loop.
			if (printPoint.x) {
				lineSegment.w = printPoint.x;
				dp.x += printPoint.x;
			}
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
			/*
			// FIXME: probably a better way to do this...
			Sprite2D* spr = font->RenderTextAsSprite(text, frame.Dimensions(), IE_FONT_ALIGN_LEFT);
			drawRegion.w = spr->Width;
			drawRegion.h = spr->Height;
			core->GetVideoDriver()->BlitSprite(spr, drawRegion.x, drawRegion.y, true);
			spr->release();
			*/
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
		assert(drawRegion.h && drawRegion.w);
		layoutRegions.push_back(drawRegion);
	}
}

void TextSpan::SetPalette(Palette* pal)
{
	if (!pal) {
		pal = font->GetPalette();
		pal->release();
	}
	pal->acquire();
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
		return;
	}
	ContentList::iterator it;
	it = std::find(contents.begin(), contents.end(), existing);
	if (it != contents.end()) {
		contents.insert(++it, newContent);
	} else {
		contents.push_back(newContent);
	}
}

Content* ContentContainer::RemoveContent(const Content* span)
{
	ContentList::iterator it;
	it = std::find(contents.begin(), contents.end(), span);
	if (it != contents.end()) {
		contents.erase(it);
		Content* content = *it;
		content->parent = NULL;
		return content;
	}
	return NULL;
}

Content* ContentContainer::ContentAtPoint(const Point& p) const
{
	return ContentAtScreenPoint(Point(p.x + screenOffset.x, p.y + screenOffset.y));
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

Size ContentContainer::ContentFrame() const
{
	if (!layoutRegions.empty()) {
		return layoutRegions.back().Dimensions();
	}
	return Content::ContentFrame();
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
	int maxH = drawPoint.y - drawOrigin.y;
	Content* content = NULL;
	layout.clear();
	ContentList::const_iterator it = contents.begin();
	for (; it != contents.end(); ++it) {
		content = *it;

		content->layoutRegions.clear();
		content->DrawContents(drawPoint - drawOrigin, rgn);

		layout.insert(std::make_pair(content, content->layoutRegions));
		Regions::const_iterator rit = layout[content].begin();
		for (; rit != layout[content].end(); ++rit) {
			int h = ((*rit).y + (*rit).h) - drawOrigin.y;
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

	Region layoutRegion = Region(drawOrigin, Size(frame.w, maxH));
	layoutRegions.push_back(layoutRegion);
	core->GetVideoDriver()->DrawRect(layoutRegion, ColorRed, false);
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

	/*
void TextContainer::AppendText(const String& text, Font* fnt, Palette* pal)
{
	if (text.length()) {
		// FIXME: there is probably a better, more efficient, way to layout text
		// we could use a single span for the entire appended string, however, the way things are now
		// we have no way to give the font->Print methods an indent, nor does it return the information necessary
		// to know where the last line ended so we would always have an implicit "newline".
		// if an implicit "newline" is acceptable (or even desired) we could change this to make a span per line,
		// but that has its own complications so I'm choosing to keep things simple, and we can optimize or refactor later

		// FIXME: this implementation is still broken for words that are longer than a line...

		// any changes to how we build content spans should maintain the following functionality
		// 1. layout needs to reflow *correctly* when the container is resized
		// 2. it should be assumed that text is *editable* and text should reflow accordingly
		// 3. must support a mixture of spans (different fonts or images etc)

		ContentList::const_iterator layoutIt = contents.end()--;
		size_t curTextLen = TextString.length();
		size_t wordBreak = 0;
		size_t wordPos = text.find_first_not_of(L" \n\t\r");
		// iterate the string and make a TextSpan for each word
		// TODO: support word break on '-' (and possibly other) characters.
		while ((wordBreak = text.find_first_of(L" \n\t\r", wordPos)) != String::npos) {
			// FIXME: not sure how we want to handle whitespace
			// I'm taking the HTML approach for now and ignoring contiguous whitespace
			// The easiest way to handle all whitespace (using this method of layout) would be
			// to introduce margins to spans, and measure the whitespace following a word and
			// apply it as a right hand margin

			AppendContent(new TextSpan(*this, Range(wordPos + curTextLen, wordBreak + curTextLen)));
			// skip any white space to find the next word
			wordPos = text.find_first_not_of(L" \n\t\r", wordBreak);
		}

		TextString.append(text);
		LayoutContentStartingAt(++layoutIt);
	}
}
*/
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


/*
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
*/

}
