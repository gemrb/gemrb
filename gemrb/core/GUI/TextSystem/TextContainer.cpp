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

#include "Debug.h"
#include "Interface.h"
#include "Palette.h"
#include "Sprite2D.h"
#include "Strings/String.h"
#include "Video/Video.h"

#include <algorithm>
#include <climits>

namespace GemRB {

LayoutRegions Content::LayoutForPointInRegion(Point p, const Region& rgn) const
{
	return { std::make_shared<LayoutRegion>(Region(rgn.origin + p, frame.size)) };
}

TextSpan::TextSpan(String string, const Font* fnt, const Size* frame)
	: Content(frame ? *frame : Size()), text(std::move(string)), font(fnt)
{
}

TextSpan::TextSpan(String string, const Font* fnt, Font::PrintColors cols, const Size* frame)
	: Content(frame ? *frame : Size()), text(std::move(string)), font(fnt), colors(new Font::PrintColors(cols))
{
}

TextSpan::~TextSpan()
{
	delete colors;
}

void TextSpan::ClearColors()
{
	delete colors;
}

void TextSpan::SetColors(const Color& fg, const Color& bg)
{
	ClearColors();
	colors = new Font::PrintColors {fg, bg};
}

inline const Font* TextSpan::LayoutFont() const
{
	if (font) return font;

	const TextContainer* container = static_cast<const TextContainer*>(parent);
	if (container) {
		return container->TextFont();
	}
	return nullptr;
}

inline Region TextSpan::LayoutInFrameAtPoint(const Point& p, const Region& rgn) const
{
	const Font* layoutFont = LayoutFont();
	Size maxSize = frame.size;
	Region drawRegion = Region(p + rgn.origin, maxSize);
	if (maxSize.w <= 0) {
		if (maxSize.w == -1) {
			// take remainder of parent width
			drawRegion.w = rgn.w - p.x;
			maxSize.w = drawRegion.w;
			assert(drawRegion.w);
		} else {
			Font::StringSizeMetrics metrics = {maxSize, 0, 0, true};
			drawRegion.w = layoutFont->StringSize(text, &metrics).w;
			assert(drawRegion.w);
		}
	}
	if (maxSize.h <= 0) {
		if (maxSize.h == -1) {
			// take remainder of parent height
			drawRegion.h = rgn.w - p.y;
			assert(drawRegion.h);
		} else {
			Font::StringSizeMetrics metrics = {maxSize, 0, 0, true};
			drawRegion.h = layoutFont->StringSize(text, &metrics).h;
			assert(drawRegion.h);
		}
	}
	return drawRegion;
}

LayoutRegions TextSpan::LayoutForPointInRegion(Point layoutPoint, const Region& rgn) const
{
	LayoutRegions layoutRegions;
	const Point& drawOrigin = rgn.origin;
	const Font* layoutFont = LayoutFont();
	assert(layoutFont);

	if (frame.size.IsZero()) {
		// this means we get to wrap :)
		// calculate each line and print line by line
		int lineheight = layoutFont->LineHeight;
		Regions lineExclusions;
		Region lineRgn(layoutPoint + drawOrigin, Size(rgn.w, lineheight));
		lineRgn.y -= lineheight;
		Region lineSegment;

#define LINE_REMAINDER (lineRgn.w - lineSegment.x)
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
				layoutPoint = lineRgn.origin;

				lineSegment = lineRgn;
			}
			
			size_t begin = 0;
			size_t end = 0;
			do {
				// process all overlaping exclusion zones until we trim down to the leftmost non conflicting region.
				// check for intersections with other content
				excluded = parent->ContentRegionForRect(lineSegment);
				if (!excluded) {
					// now check if we already used this region ourselves
					for (const auto& lineEx : lineExclusions) {
						if (lineSegment.IntersectsRegion(lineEx)) {
							excluded = &lineEx;
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
				size_t nextLine = text.find_first_of(u'\n', numPrinted);
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
					Font::StringSizeMetrics metrics = {lineSegment.size, 0, 0, lineSegment.w == lineRgn.w};
					Size printSize = layoutFont->StringSize(substr, &metrics);
					numOnLine = metrics.numChars;
					assert(numOnLine || !metrics.forceBreak);
					
					begin = numPrinted;
					end = begin + numOnLine;

					bool noFit = !metrics.forceBreak && numOnLine == 0;
					bool lineFilled = lineSegment.x + lineSegment.w == lineRgn.w;
					bool moreChars = numPrinted + numOnLine < text.length();
					if (subLen != String::npos || noFit || (lineFilled && moreChars)) {
						// optimization for when the segment is the entire line (and we have more text)
						// saves looping again for the known to be useless segment
						newline = true;
						lineSegment.w = LINE_REMAINDER;
						end--;
					} else {
						assert(printSize.w);
						lineSegment.w = printSize.w;
					}
				}
				numPrinted += numOnLine;
			}
			assert(!lineSegment.size.IsInvalid());
			lineExclusions.push_back(lineSegment);

		newline:
			if (newline || numPrinted == text.length()) {
				// must claim the lineExclusions as part of the layout
				// just because we didnt fit doesn't mean something else won't...
				Region lineLayout = Region::RegionEnclosingRegions(lineExclusions);
				assert(lineLayout.h % lineheight == 0);
				layoutRegions.emplace_back(std::make_shared<TextLayoutRegion>(lineLayout, begin, end));
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
		if (!text.empty()) { // kachiko.dlg has a completely void response 224
			Region drawRegion = LayoutInFrameAtPoint(layoutPoint, rgn);
			layoutRegions.emplace_back(std::make_shared<TextLayoutRegion>(drawRegion, 0, text.length()));
		}
	}
	return layoutRegions;
}

void TextSpan::DrawContentsInRegions(const LayoutRegions& rgns, const Point& offset) const
{
	size_t charsPrinted = 0;
	for (const auto& lrgn : rgns) {
		Region drawRect = lrgn->region;
		drawRect.x += offset.x;
		drawRect.y += offset.y;
		const Font* printFont = LayoutFont();
		const Font::PrintColors* pc = colors;
		const TextContainer* container = static_cast<const TextContainer*>(parent);
		if (!pc && container) {
			pc = container->TextColors();
		}
		assert(printFont);
		// FIXME: this shouldnt happen, but it does (BG2 belt03 unidentified).
		// for now only assert when ID_TEXT is set
		// the situation is benign and nothing even looks wrong because all that this means is that there was more space allocated than was actually needed
		if (InDebugMode(DebugMode::TEXT)) {
			assert(charsPrinted < text.length());
			VideoDriver->DrawRect(drawRect, ColorRed, true);
		}
		// FIXME: layout assumes left alignment, so alignment is mostly broken
		// we only use it for TextEdit tho which is single line and therefore works as long as the text ends in a newline
		if (pc) {
			charsPrinted += printFont->Print(drawRect, text.substr(charsPrinted), Alignment, *pc);
		} else {
			charsPrinted += printFont->Print(drawRect, text.substr(charsPrinted), Alignment);
		}

		if (InDebugMode(DebugMode::TEXT)) {
			VideoDriver->DrawRect(drawRect, ColorWhite, false);
		}
	}
}

ImageSpan::ImageSpan(const Holder<Sprite2D>& im)
	: Content(im->Frame.size)
{
	image = im;
}

void ImageSpan::DrawContentsInRegions(const LayoutRegions& rgns, const Point& offset) const
{
	// we only care about the first region... (should only be 1 anyway)
	Region r = rgns.front()->region;
	r.x += offset.x;
	r.y += offset.y;
	VideoDriver->BlitSprite(image, r.origin, &r);
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

void ContentContainer::WillDraw(const Region& drawFrame, const Region& clip)
{
	Region sc = clip;
	
	int diff = clip.x - drawFrame.x;
	if (diff < margin.left) {
		sc.x += margin.left - diff;
		sc.w -= margin.left - diff;
	}
	
	diff = (drawFrame.x + drawFrame.w) - (clip.x + clip.w);
	if (diff < margin.right) {
		sc.w += margin.right - diff;
	}
	
	// TODO: if we ever support horzontal scrollbars these will need to be fixed
	sc.y += margin.top;
	sc.h -= margin.top + margin.bottom;

	VideoDriver->SetScreenClip(&sc);
}

void ContentContainer::DidDraw(const Region& /*drawFrame*/, const Region& clip)
{
	VideoDriver->SetScreenClip(&clip);
}

void ContentContainer::DrawSelf(const Region& drawFrame, const Region& clip)
{
	if (InDebugMode(DebugMode::TEXT)) {
		Region r = clip;
		VideoDriver->DrawRect(r, ColorYellow, true);
		
		r.x += margin.left;
		r.y += margin.top;
		r.w -= margin.left + margin.right;
		r.h -= margin.bottom + margin.top;
		
		VideoDriver->DrawRect(r, ColorGreen, true);
	}

	// layout shouldn't be empty unless there is no content anyway...
	if (layout.empty()) return;
	Point dp = drawFrame.origin + Point(margin.left, margin.top);
	
	ContentLayout::const_iterator it = layout.begin();
	for (; it != layout.end(); ++it) {
		const Layout& l = *it;

		// TODO: pass the clip rect so we can skip non-intersecting regions
		DrawContents(l, dp);
	}
}

void ContentContainer::DrawContents(const Layout& contentLayout, Point point)
{
	contentLayout.content->DrawContentsInRegions(contentLayout.regions, point);
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
		SetFlags(RESIZE_WIDTH, BitOp::OR);
	}
	if (frame.h <= 0) {
		SetFlags(RESIZE_HEIGHT, BitOp::OR);
	}
	LayoutContentsFrom(contents.begin());
}

void ContentContainer::SubviewAdded(View* view, View* parent)
{
	if (parent == this) {
		// ContentContainer should grow to the size of its (immidiate) subviews automatically
		const Region& subViewFrame = view->Frame();
		Size s;
		s.h = std::max(subViewFrame.y + subViewFrame.h, frame.h);
		s.w = std::max(subViewFrame.x + subViewFrame.w, frame.w);
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

		layoutPoint.reset(); // reset cached layoutPoint
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
	layoutPoint.reset(); // reset cached layoutPoint
	ContentRemoved(content);
	delete content;

	return contents.erase(it);
}

ContentContainer::ContentList::iterator
ContentContainer::EraseContent(ContentList::iterator beg, ContentList::iterator end)
{
	while (beg != end) {
		beg = EraseContent(beg);
	}
	return end;
}

Content* ContentContainer::ContentAtPoint(const Point& p) const
{
	const Layout* contentLayout = LayoutAtPoint(p);
	if (contentLayout) {
		// i know we are casting away const.
		// we could return std::find(contents.begin(), contents.end(), *it) instead, but whats the point?
		return (Content*)(contentLayout->content);
	}
	return NULL;
}

const ContentContainer::Layout* ContentContainer::LayoutAtPoint(const Point& p) const
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
		if (it->PointInside(p)) {
			// i know we are casting away const.
			// we could return std::find(contents.begin(), contents.end(), *it) instead, but whats the point?
			return &(*it);
		}
		if (*it < p) {
			++it;
			count -= step + 1;
		} else {
			std::advance(it, -std::make_signed_t<size_t>(step));
			count = step;
		}
	}
	return NULL;
}

Region ContentContainer::BoundingBoxForContent(const Content* c) const
{
	return BoundingBoxForLayout(LayoutForContent(c).regions);
}

Region ContentContainer::BoundingBoxForLayout(const LayoutRegions& layoutRgns) const
{
	auto it = layoutRgns.begin();
	if (it != layoutRgns.end()) {
		Region r = (*it++)->region;
		for (; it != layoutRgns.end(); ++it) {
			r.ExpandToRegion((*it)->region);
		}
		return r;
	}
	
	return Region();
}

const ContentContainer::Layout& ContentContainer::LayoutForContent(const Content* c) const
{
	ContentLayout::const_iterator it = std::find(layout.begin(), layout.end(), c);
	if (it != layout.end()) {
		return *it;
	}
	static Layout NullLayout(nullptr, LayoutRegions());
	return NullLayout;
}

const Region* ContentContainer::ContentRegionForRect(const Region& r) const
{
	for (const auto& layoutRgn : layout) {
		for (const auto& lrgn : layoutRgn.regions) {
			const Region& rect = lrgn->region;
			if (rect.IntersectsRegion(r)) {
				return &rect;
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
			layoutPoint.reset(); // reset cached layoutPoint
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

	assert(!layoutFrame.size.IsInvalid());
	while (it != contents.end()) {
		const Content* content = *it++;
		while (exContent) {
			const LayoutRegions& rgns = LayoutForContent(exContent).regions;
			for (const auto& lrgn : rgns) {
				const Region& r = lrgn->region;
				if (r.PointInside(layoutPoint)) {
					excluded = &r;
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
		const LayoutRegions& rgns = content->LayoutForPointInRegion(layoutPoint, layoutFrame);
		if (rgns.empty()) return;
		layout.emplace_back(content, rgns);
		exContent = content;

		ieDword flags = Flags();
		if (flags&(RESIZE_HEIGHT|RESIZE_WIDTH)) {
			Region bounds = BoundingBoxForLayout(rgns);
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

void ContentContainer::DeleteContentsInRect(const Region& exclusion)
{
	int top = exclusion.y;
	int bottom = top;
	const Content* content;
	while (const Region* rgn = ContentRegionForRect(exclusion)) {
		content = ContentAtPoint(rgn->origin);
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


TextContainer::TextContainer(const Region& frame, Font* fnt)
	: ContentContainer(frame), font(fnt)
{
}

TextContainer::~TextContainer()
{
	delete colors;
}

void TextContainer::AppendText(String text)
{
	AppendText(std::move(text), nullptr, colors);
}

void TextContainer::AppendText(String text, const Font* fnt, const Font::PrintColors* cols)
{
	size_t len = text.length();
	if (len) {
		TextSpan* span = new TextSpan(std::move(text), fnt);
		if (cols) {
			span->SetColors(cols->fg, cols->bg);
		}
		span->Alignment = alignment;
		AppendContent(span);
		textLen += len;

		MarkDirty();
	}
}

void TextContainer::ContentRemoved(const Content* content)
{
	const TextSpan* ts = static_cast<const TextSpan*>(content);
	textLen -= ts->Text().length();
}

void TextContainer::ClearColors()
{
	delete colors;
	MarkDirty();
}

void TextContainer::SetColors(const Color& fg, const Color& bg)
{
	ClearColors();
	colors = new Font::PrintColors {fg, bg};
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
		return u""; // must bail or things will get screwed up!
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

void TextContainer::DrawSelf(const Region& drawFrame, const Region& clip)
{
	printPos = 0;
	ContentContainer::DrawSelf(drawFrame, clip);

	if (layout.empty() && Editable()) {
		Holder<Sprite2D> cursor = core->GetCursorSprite();
		Point p(drawFrame.x + margin.left, drawFrame.y + margin.top + cursor->Frame.y);
		VideoDriver->BlitSprite(cursor, p);
	}
}

LayoutRegions::const_iterator
TextContainer::FindCursorRegion(const Layout& layout) const
{
	auto end = layout.regions.end();
	for (auto it = layout.regions.begin(); it != end; ++it) {
		const TextLayout& tlrgn = static_cast<const TextLayout&>(**it);
		
		if (tlrgn.beginCharIdx <= cursorPos && tlrgn.endCharIdx >= cursorPos) {
			return it;
		}
	}
	return end;
}

void TextContainer::DrawContents(const Layout& layout, Point dp)
{	
	ContentContainer::DrawContents(layout, dp);

	const TextSpan* ts = (const TextSpan*)layout.content;
	const String& text = ts->Text();
	size_t textLength = ts->Text().length();

	if (Editable() && printPos <= cursorPos && printPos + textLength >= cursorPos) {
		const Font* printFont = ts->LayoutFont();
		
		auto it = FindCursorRegion(layout);
		if (it != layout.regions.end()) {
			auto cursorRegion = static_cast<const TextLayout&>(**it);
			size_t begin = cursorRegion.beginCharIdx;
			const Region& rect = cursorRegion.region;
			cursorPoint = rect.origin;
			const String& substr = text.substr(begin, cursorPos - begin);
			cursorPoint.x += printFont->StringSizeWidth(substr, 0);
		}

		Holder<Sprite2D> cursor = core->GetCursorSprite();
		dp.y += cursor->Frame.y;
		VideoDriver->BlitSprite(cursor, cursorPoint + dp);
	}
	printPos += textLength;
}

void TextContainer::SizeChanged(const Size& oldSize)
{
	ContentContainer::SizeChanged(oldSize);
	if (Editable() && frame.h == 0) {
		// we need at least one line of height to draw the cursor
		frame.h = font->LineHeight;
	}
}

void TextContainer::MoveCursorToPoint(const Point& p)
{
	if (Editable() == false)
		return;

	const Layout* layout = LayoutAtPoint(p);

	if (layout) {
		const TextSpan* ts = (const TextSpan*) layout->content;
		const String& text = ts->Text();
		const Font* printFont = ts->LayoutFont();
		Font::StringSizeMetrics metrics = {Size(0,0), 0, 0, true};
		size_t numChars = 0;

		for (const auto& lrgn : layout->regions) {
			const Region& rect = lrgn->region;

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
				metrics.size = rect.size;
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
	VideoDriver->StartTextInput();
}

void TextContainer::DidUnFocus()
{
	VideoDriver->StopTextInput();
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

	VideoDriver->StartTextInput();

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
		case GEM_UP:
		case GEM_DOWN:
			{
				size_t offset = 0;
				const Layout* layout = LayoutAtPoint(cursorPoint);
				LayoutRegions::const_iterator it;
				if (!layout && key.keycode == GEM_UP) {
					// end of text workaround for GEM_UP to compute the proper cursor position to move to
					layout = LayoutAtPoint(Point(cursorPoint.x - 1, cursorPoint.y));
				}
				if (layout) {
					it = FindCursorRegion(*layout);
					assert(it != layout->regions.end());
					auto cursorRegion = static_cast<const TextLayout&>(**it);
					offset = cursorPos - cursorRegion.beginCharIdx;
				} else { // cursor is at end of text
					return true;
				}
				
				if (key.keycode == GEM_UP) {
					if (it == layout->regions.begin()) {
						return true;
					}
					it--;
				} else {
					if (it == layout->regions.end()) {
						return true;
					}
					it++;
				}
				
				if (it != layout->regions.end()) {
					auto cursorRegion = static_cast<const TextLayout&>(**it);
					size_t length = cursorRegion.endCharIdx - cursorRegion.beginCharIdx;
					cursorPos = cursorRegion.beginCharIdx + std::min(offset, length);
					MarkDirty();
				}
			}
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
	VideoDriver->StartTextInput();
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
		const TextSpan* ts = static_cast<const TextSpan*>(*it);
		size_t textLength = ts->Text().length();
		if (charCount + textLength >= idx) {
			break;
		}
		charCount += textLength;
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
	AppendText(std::move(newtext));
	AdvanceCursor(int(text.length()));

	if (callback) {
		callback(*this);
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
	AppendText(std::move(newtext));
	AdvanceCursor(-int(len));

	if (callback) {
		callback(*this);
	}
}

}
