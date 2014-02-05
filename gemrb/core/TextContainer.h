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
	const Sprite2D* RenderedSpan() const { return spanSprite; }
	const String& RenderedString() const { return text; }

	void SetPalette(Palette* pal);
private:
	void RenderSpan(const String& string);
};

class TextContainer
{
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
	~TextContainer();

	// Creates a basic "inline" span using the containers font/palette
	void AppendText(const String& text);
	// append the span to the end of the container. The container takes ownership of the span.
	void AppendSpan(TextSpan* span);
	// Insert a span to a new position in the list. The container takes ownership of the span.
	void InsertSpanAfter(TextSpan* newSpan, const TextSpan* existing);
	// removes the span from the container and transfers ownership to the caller.
	// Returns a non-const pointer to the removed span.
	TextSpan* RemoveSpan(const TextSpan* span);

	TextSpan* SpanAtPoint(const Point& p) const;
	const Size& ContainerFrame() const { return frame; }
	void DrawContents(int x, int y) const;
private:
	void LayoutSpansStartingAt(SpanList::const_iterator start);
	void AddExclusionRect(const Region& rect);
	const Region* ExcludedRegionForRect(const Region& rect);
};

}
#endif
