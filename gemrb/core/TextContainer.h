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

#ifndef TEXTCONTAINER_H
#define TEXTCONTAINER_H

#include "Font.h"
#include "Region.h"
#include "System/String.h"

#include <list>
#include <map>
#include <vector>

namespace GemRB {

class Font;
class Palette;
class Sprite2D;

class TextSpan
{
private:
	String text;
	size_t stringLen;
	Size frame;
	Font* font;
	Palette* palette;
	Sprite2D* spanSprite;
	ieByte alignment;
public:
	// construct a "inline" span that calculates its own region based on font, palette, and string
	TextSpan(const String& string, Font* font, Palette* pal = NULL);
	// construct a "block" span with dimentions determined by rgn
	TextSpan(const String& string, Font* font, Palette* pal, const Size& rgn, ieByte align);
	~TextSpan();

	const Size& SpanFrame() const { return frame; }
	int SpanDescent() const { return font->descent; }
	const Sprite2D* RenderedSpan() const { return spanSprite; }
	const String& RenderedString() const { return text; }

	void SetPalette(Palette* pal);
	void SetPadding(int lft, int top, int right, int bottom);
private:
	void RenderSpan(const String& string);
};

class TextContainer
{
protected:
	typedef std::list<TextSpan*> SpanList;
	SpanList spans;
	typedef std::map<TextSpan*, Region> SpanLayout;
	SpanLayout layout;
	std::vector<Region> ExclusionRects;

	Size maxFrame;
	Size frame;
	Font* font;
	Palette* pallete;
public:
	TextContainer(const Size& frame, Font* font, Palette* pal);
	virtual ~TextContainer();

	// Creates a basic "inline" span using the containers font/palette
	void AppendText(const String& text);
	// append the span to the end of the container. The container takes ownership of the span.
	virtual void AppendSpan(TextSpan* span);
	// Insert a span to a new position in the list. The container takes ownership of the span.
	virtual void InsertSpanAfter(TextSpan* newSpan, const TextSpan* existing);
	// removes the span from the container and transfers ownership to the caller.
	// Returns a non-const pointer to the removed span.
	TextSpan* RemoveSpan(const TextSpan* span);
	// excludes all attached spans such that new spans cannot flow adjacent
	void ClearSpans();

	void SetSpanPadding(TextSpan* span, Size pad);
	TextSpan* SpanAtPoint(const Point& p) const;
	Point PointForSpan(const TextSpan*);
	const Size& ContainerFrame() const { return frame; }
	void SetMaxFrame(const Size&);
	void DrawContents(int x, int y) const;
	// public so clients can allocate an area for drawing images or whatever they want
	void AddExclusionRect(const Region& rect);
private:
	void LayoutSpansStartingAt(SpanList::const_iterator start);
	const Region* ExcludedRegionForRect(const Region& rect);
};

/*
 We could choose to limit text in any number of ways (by "line", by size, or by span count)
 Since the goal here is simply to keep the amount of text from becoming burdonsome the method doesn't matter
 as long as we remain close to the given limit.
 
 I have selected to limit by number of spans since it seemed easiest. This isn't a hard limit as the number of spans may be
 reduced below the limit based on how many spans intersect the area from the top of the container to the bottom of the top span.
 
 This effectively pops messages one at a time from the MessageWindow instead of droping single lines of text.
 I feel this is a supirior method because it prevents having truncated messages.
*/

class RestrainedTextContainer : public TextContainer
{
private:
	size_t spanLimit;

public:
	RestrainedTextContainer(const Size& frame, Font* font, Palette* pal, size_t limit)
	: TextContainer(frame, font, pal) { spanLimit = limit; }

	void AppendSpan(TextSpan* span);
};

}
#endif
