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
	const Region& layoutRgn = Region(rgn.Origin() + p, frame.Dimensions());
	return Regions(1, layoutRgn);
}

TextSpan::TextSpan(const String& string, const Font* fnt, Holder<Palette> pal, const Size* frame)
	: Content((frame) ? *frame : Size()), text(string), font(fnt), palette(pal)
{
	Alignment = IE_FONT_ALIGN_LEFT;
}

inline const Font* TextSpan::LayoutFont() const
{
	if (font) return font;

	TextContainer* container = static_cast<TextContainer*>(parent);
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
			Font::StringSizeMetrics metrics = {maxSize, 0, 0, true};
			drawRegion.w = layoutFont->StringSize(text, &metrics).w;
		}
	}
	if (maxSize.h <= 0) {
		if (maxSize.h == -1) {
			// take remainder of parent height
			drawRegion.h = rgn.w - p.y;
		} else {
			Font::StringSizeMetrics metrics = {maxSize, 0, 0, true};
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
					Font::StringSizeMetrics metrics = {lineSegment.Dimensions(), 0, 0, lineSegment.w == lineRgn.w};
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
		const Font* printFont = LayoutFont();
		Holder<Palette> printPalette = palette;
		TextContainer* container = static_cast<TextContainer*>(parent);
		if (printPalette == NULL && container) {
			printPalette = container->TextPalette();
		}
		assert(printFont && printPalette);
#if (DEBUG_TEXT)
		// FIXME: this shouldnt happen, but it does (BG2 belt03 unidentified).
		// for now only assert when DEBUG_TEXT is set
		// the situation is benign and nothing even looks wrong because all that this means is that there was more space allocated than was actually needed
		assert(charsPrinted < text.length());
		core->GetVideoDriver()->DrawRect(drawRect, ColorRed, true);
#endif
		// FIXME: layout assumes left alignment, so alignment is mostly broken
		// we only use it for TextEdit tho which is single line and therefore works as long as the text ends in a newline
		charsPrinted += printFont->Print(drawRect, text.substr(charsPrinted), printPalette.get(), Alignment);
#if (DEBUG_TEXT)
		core->GetVideoDriver()->DrawRect(drawRect, ColorWhite, false);
#endif
	}
}

ImageSpan::ImageSpan(Sprite2D* im)
	: Content(im->Frame.Dimensions())
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
	core->GetVideoDriver()->BlitSprite(image, r.x, r.y, &r);
}
	
ContentContainer::ContentContainer(const Region& frame)
: View(frame)
{
	SizeChanged(Size());
}

ContentContainer::~ContentContainer()
{
	ContentList::iterator it = contents.begin();
	for (; it != contents.end(); ++it) {
		delete *it;
	}
}

void ContentContainer::SetMargin(Margin m)
{
	margin = m;
	LayoutContentsFrom(contents.begin());
}

void ContentContainer::SetMargin(ieByte top, ieByte right, ieByte bottom, ieByte left)
{
	SetMargin(Margin(top, right, bottom, left));
}

void ContentContainer::DrawSelf(Region drawFrame, const Region& clip)
{
	Video* video = core->GetVideoDriver();
#if DEBUG_TEXT
	video->DrawRect(clip, ColorGreen, true);
#else
	(void)clip;
#endif

	// layout shouldn't be empty unless there is no content anyway...
	if (layout.empty()) return;
	Point dp = drawFrame.Origin() + Point(margin.left, margin.top);
	
	Region sc = video->GetScreenClip();
	sc.x += margin.left;
	sc.y += margin.top;
	sc.w -= margin.left + margin.right;
	sc.h -= margin.top + margin.bottom;
	video->SetScreenClip(&sc);

	ContentLayout::const_iterator it = layout.begin();
	for (; it != layout.end(); ++it) {
		const Layout& l = *it;

		// TODO: pass the clip rect so we can skip non-intersecting regions
		DrawContents(l, dp);
	}
}

void ContentContainer::DrawContents(const Layout& layout, const Point& point)
{
	layout.content->DrawContentsInRegions(layout.regions, point);
}

void ContentContainer::AppendContent(Content* content)
{
	if (contents.empty())
		InsertContentAfter(content, NULL);
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
		SetFlags(RESIZE_WIDTH, OP_OR);
	}
	if (frame.h <= 0) {
		SetFlags(RESIZE_HEIGHT, OP_OR);
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

		ContentRemoved(content);
		return content;
	}
	return NULL;
}

ContentContainer::ContentList::iterator
ContentContainer::EraseContent(ContentList::iterator it)
{
	Content* content = *it;
	content->parent = NULL;
	layout.erase(std::find(layout.begin(), layout.end(), content));
	layoutPoint = Point(); // reset cached layoutPoint
	ContentRemoved(content);
	delete content;

	return contents.erase(it);
}

ContentContainer::ContentList::iterator
ContentContainer::EraseContent(ContentList::iterator beg, ContentList::iterator end)
{
	for (; beg != end;) {
		beg = EraseContent(beg);
	}
	return end;
}

Content* ContentContainer::ContentAtPoint(const Point& p) const
{
	const Layout* layout = LayoutAtPoint(p);
	if (layout) {
		// i know we are casting away const.
		// we could return std::find(contents.begin(), contents.end(), *it) instead, but whats the point?
		return (Content*)(layout->content);
	}
	return NULL;
}

const ContentContainer::Layout* ContentContainer::LayoutAtPoint(const Point& p) const
{
	// attempting to optimize the search by assuming content is evenly distributed vertically
	// we are also assuming that layout regions are always contiguous and ordered ltr-ttb
	// we do know by definition that the content itself is ordered ltr-ttb
	// based on the above a simple binary search should suffice

	int index = 0;
	ContentLayout::const_iterator it = layout.begin();
	size_t count = layout.size();
	while (count > 0) {
		size_t step = count / 2;
		std::advance(it, step);
		index += step;
		if (it->PointInside(p)) {
			// i know we are casting away const.
			// we could return std::find(contents.begin(), contents.end(), *it) instead, but whats the point?
			return &(*it);
		}
		if (*it < p) {
			++it;
			count -= step + 1;
		} else {
			std::advance(it, -step);
			index -= step;
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
		const Regions& rgns = it->regions;
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
			// since 'layout' is sorted alongsize 'contents' we should be able clear everyting following 'i' and bail
			layout.erase(i, layout.end());
			break;
		}
	}

	Size contentBounds = Dimensions();
	Region layoutFrame = Region(Point(), contentBounds);
	if (Flags()&RESIZE_WIDTH) {
		layoutFrame.w = SHRT_MAX;
	} else {
		layoutFrame.w -= margin.left + margin.right;
	}
	if (Flags()&RESIZE_HEIGHT) {
		layoutFrame.h = SHRT_MAX;
	} else {
		layoutFrame.h -= margin.top + margin.bottom;
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

		ieDword flags = Flags();
		if (flags&(RESIZE_HEIGHT|RESIZE_WIDTH)) {
			Region bounds = Region::RegionEnclosingRegions(rgns);
			bounds.w += margin.left + margin.right;
			bounds.h += margin.top + margin.bottom;
			
			if (flags&RESIZE_HEIGHT)
				contentBounds.h = (bounds.y + bounds.h > contentBounds.h) ? bounds.y + bounds.h : contentBounds.h;
			if (flags&RESIZE_WIDTH)
				contentBounds.w = (bounds.x + bounds.w > contentBounds.w) ? bounds.x + bounds.w : contentBounds.w;
		}
	}

	// avoid infinite layout recursion when calling SetFrameSize...
	Size oldSize = Dimensions();
	frame.w = contentBounds.w;
	frame.h = contentBounds.h;
	ResizeSubviews(oldSize);
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

	if (Flags()&RESIZE_HEIGHT) {
		frame.h = 0;
	}
	if (Flags()&RESIZE_WIDTH) {
		frame.w = 0;
	}
	// TODO: we could optimize this to only layout content after exclusion.y
	LayoutContentsFrom(contents.begin());
}


TextContainer::TextContainer(const Region& frame, Font* fnt, Holder<Palette> pal)
	: ContentContainer(frame), font(fnt)
{
	SetPalette(pal);
	alignment = IE_FONT_ALIGN_LEFT;
	textLen = 0;
	cursorPos = 0;
	printPos = 0;
}

void TextContainer::AppendText(const String& text)
{
	AppendText(text, NULL, NULL);
}

void TextContainer::AppendText(const String& text, Font* fnt, Holder<Palette> pal)
{
	if (text.length()) {
		TextSpan* span = new TextSpan(text, fnt, pal);
		span->Alignment = alignment;
		AppendContent(span);
		textLen += text.length();

		MarkDirty();
	}
}

void TextContainer::ContentRemoved(const Content* content)
{
	const TextSpan* ts = static_cast<const TextSpan*>(content);
	textLen -= ts->Text().length();
}

void TextContainer::SetPalette(Holder<Palette> pal)
{
	palette = (pal) ? pal : font->GetPalette();
}

String TextContainer::Text() const
{
	return TextFrom(contents.begin());
}

String TextContainer::TextFrom(const Content* content) const
{
	return TextFrom(std::find(contents.begin(), contents.end(), content));
}

String TextContainer::TextFrom(ContentList::const_iterator it) const
{
	if (it == contents.end()) {
		return L""; // must bail or things will get screwed up!
	}

	// iterate all the content and pick out the TextSpans and concatonate them into a single string
	String text;
	for (; it != contents.end(); ++it) {
		if (const TextSpan* textSpan = static_cast<TextSpan*>(*it)) {
			text.append(textSpan->Text());
		}
	}
	return text;
}

void TextContainer::DrawSelf(Region drawFrame, const Region& clip)
{
	printPos = 0;
	ContentContainer::DrawSelf(drawFrame, clip);

	if (layout.empty() && Editable()) {
		Video* video = core->GetVideoDriver();
		Region sc = video->GetScreenClip();
		video->SetScreenClip(NULL);

		Sprite2D* cursor = core->GetCursorSprite();
		video->BlitSprite(cursor, drawFrame.x + margin.left, drawFrame.y + margin.top + cursor->Frame.y);
		cursor->release();

		video->SetScreenClip(&sc);
	}
}

void TextContainer::DrawContents(const Layout& layout, const Point& dp)
{
	ContentContainer::DrawContents(layout, dp);

	const TextSpan* ts = (const TextSpan*)layout.content;
	const String& text = ts->Text();
	size_t textLen = ts->Text().length();

	if (Editable() && printPos < cursorPos && printPos + textLen >= cursorPos) {
		const Font* printFont = ts->LayoutFont();
		Font::StringSizeMetrics metrics = {Size(0,0), 0, 0, true};

		// diff is the length of the TextSpan we want however, it may fall inside of a word
		size_t diff = cursorPos - printPos;
		size_t start = 0;
		size_t stop = text.find_last_of(WHITESPACE_STRING, diff);
		if (stop == String::npos)
			stop = 0;

		Point p;
		Regions::const_iterator rit = layout.regions.begin();
		for (; rit != layout.regions.end(); ++rit) {
			const Region& rect = *rit;
			p = rect.Origin();
			metrics.size = rect.Dimensions();

			const String& substr = text.substr(start, stop);
			printFont->StringSize(substr, &metrics);

			if (metrics.numChars == stop) {
				if (text[start+stop] == '\n') {
					// FIXME: technically ought to use the next rect rather then assume we can jump down a line
					p.y += printFont->LineHeight;
					++start;
				} else if (metrics.numChars > 0) {
					p.x += metrics.size.w;
				}
				// found it
				if (stop < diff) {
					// inside a word
					const String& substr = text.substr(start + stop, diff - start - metrics.numChars);
					p.x += printFont->StringSizeWidth(substr, 0);
				}
				break;
			}
			start += metrics.numChars;
			stop -= metrics.numChars;
		}

		Video* video = core->GetVideoDriver();
		Region sc = video->GetScreenClip();
		video->SetScreenClip(NULL);

		Sprite2D* cursor = core->GetCursorSprite();
		video->BlitSprite(cursor, p.x + dp.x, p.y + dp.y + cursor->Frame.y);
		cursor->release();

		video->SetScreenClip(&sc);
	}
	printPos += textLen;
}

void TextContainer::MoveCursorToPoint(const Point& p)
{
	if (Editable() == false)
		return;

	const Layout* layout = LayoutAtPoint(p);

	if (layout) {
		TextSpan* ts = (TextSpan*)layout->content;
		const String& text = ts->Text();
		const Font* printFont = ts->LayoutFont();
		Font::StringSizeMetrics metrics = {Size(0,0), 0, 0, true};
		size_t numChars = 0;

		const Regions& regions = layout->regions;
		Regions::const_iterator rit = regions.begin();
		for (; rit != regions.end(); ++rit) {
			const Region& rect = *rit;

			if (rect.PointInside(p)) {
				// find where inside
				int lines = (p.y - rect.y) / printFont->LineHeight;
				if (lines) {
					metrics.size.w = rect.w;
					metrics.size.h = lines * printFont->LineHeight;
					printFont->StringSize(text.substr(numChars), &metrics);
					numChars += metrics.numChars;
				}
				size_t len = 0;
				printFont->StringSizeWidth(text.substr(numChars), p.x, &len);
				cursorPos = numChars + len;
				MarkDirty();
				break;
			} else {
				// not in this one so we need to consume some text
				// TODO: this could be faster if we stored it while calculating layout
				metrics.size = rect.Dimensions();
				printFont->StringSize(text.substr(numChars), &metrics);
				numChars += metrics.numChars;
			}
		}
	} else {
		// FIXME: this isnt _always_ the end (it works out that way for left alignment tho)
		CursorEnd();
	}
}

void TextContainer::DidFocus()
{
	core->GetVideoDriver()->StartTextInput();
}

void TextContainer::DidUnFocus()
{
	core->GetVideoDriver()->StopTextInput();
}

bool TextContainer::OnMouseDown(const MouseEvent& me, unsigned short /*Mod*/)
{
	Point p = ConvertPointFromScreen(me.Pos());
	MoveCursorToPoint(p);
	return true;
}

bool TextContainer::OnMouseDrag(const MouseEvent& me)
{
	// TODO: should be able to highlight a range
	Point p = ConvertPointFromScreen(me.Pos());
	MoveCursorToPoint(p);
	return true;
}

bool TextContainer::OnKeyPress(const KeyboardEvent& key, unsigned short /*Mod*/)
{
	if (Editable() == false)
		return false;

	core->GetVideoDriver()->StartTextInput();

	switch (key.keycode) {
		case GEM_HOME:
			CursorHome();
			return true;
		case GEM_END:
			CursorEnd();
			return true;
		case GEM_LEFT:
			AdvanceCursor(-1);
			return true;
		case GEM_RIGHT:
			AdvanceCursor(1);
			return true;
		case GEM_DELETE:
			if (cursorPos < textLen) {
				AdvanceCursor(1);
				DeleteText(1);
			}
			return true;
		case GEM_BACKSP:
			if (cursorPos > 0) {
				DeleteText(1);
			}
			return true;
		case GEM_RETURN:
			InsertText(String(1, '\n'));
			return true;
		default:
			return false;
	}
}

void TextContainer::OnTextInput(const TextEvent& te)
{
	InsertText(te.text);
	core->GetVideoDriver()->StartTextInput();
}

// move cursor to beginning of text
void TextContainer::CursorHome()
{
	// top right of first region in first layout area
	cursorPos = 0;
	MarkDirty();
}

// move cursor to end of text
void TextContainer::CursorEnd()
{
	// bottom left of last region in last layout area
	cursorPos = textLen;
	MarkDirty();
}

void TextContainer::AdvanceCursor(int delta)
{
	cursorPos += delta;
	if (int(cursorPos) < 0) {
		CursorHome();
	} else if (cursorPos >= textLen) {
		CursorEnd();
	} else {
		MarkDirty();
	}
}

TextContainer::ContentIndex TextContainer::FindContentForChar(size_t idx)
{
	size_t charCount = 0;
	ContentList::iterator it = contents.begin();
	while (it != contents.end()) {
		TextSpan* ts = static_cast<TextSpan*>(*it);
		size_t textLen = ts->Text().length();
		if (charCount + textLen >= idx) {
			break;
		}
		charCount += textLen;
		++it;
	}
	return std::make_pair(charCount, it);
}

void TextContainer::InsertText(const String& text)
{
	ContentIndex idx = FindContentForChar(cursorPos);
	String newtext = TextFrom(idx.second);

	if (cursorPos < textLen) {
		size_t pos = cursorPos - idx.first;
		newtext.insert(pos, text);
	} else {
		newtext.append(text);
	}

	EraseContent(idx.second, contents.end());
	AppendText(newtext);
	AdvanceCursor(int(text.length()));

	if (callback) {
		(*callback)(*this);
	}
}

void TextContainer::DeleteText(size_t len)
{
	ContentIndex idx = FindContentForChar(cursorPos);
	String newtext = TextFrom(idx.second);

	if (newtext.length()) {
		newtext.erase(cursorPos - idx.first - 1, len);
	}

	EraseContent(idx.second, contents.end());
	AppendText(newtext);
	AdvanceCursor(-int(len));

	if (callback) {
		(*callback)(*this);
	}
}

}
