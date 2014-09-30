/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
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
 *
 *
 */

/**
 * @file Font.h
 * Declares Font, class for manipulating images serving as fonts
 * @author The GemRB Project
 */

#ifndef FONT_H
#define FONT_H

#include "globals.h"
#include "exports.h"

#include "Sprite2D.h"

#include <deque>
#include <map>

namespace GemRB {

enum FontStyle {
	NORMAL = 0x00,
	BOLD = 0x01,
	ITALIC = 0x02,
	UNDERLINE = 0x04
};

class Palette;

#define IE_FONT_ALIGN_LEFT   0x00
#define IE_FONT_ALIGN_CENTER 0x01
#define IE_FONT_ALIGN_RIGHT  0x02
#define IE_FONT_ALIGN_BOTTOM 0x04
#define IE_FONT_ALIGN_TOP    0x10
#define IE_FONT_ALIGN_MIDDLE 0x20
#define IE_FONT_SINGLE_LINE  0x40

struct Glyph {
	const Size size;
	const Point pos;

	const ieWord pitch;
	const ieByte* pixels;

	Glyph(const Size& size, Point pos, ieByte* pixels, ieWord pitch)
	: size(size), pos(pos), pitch(pitch), pixels(pixels) {};
};

/**
 * @class Font
 * Class for using and manipulating images serving as fonts
 */

class GEM_EXPORT Font {
	/*
	 we cannot assume direct pixel access to a Sprite2D (opengl, BAM, etc), but we need raw pixels for rendering text
	 However, some clients still want to blit text directly to the screen, in which case we *must* use a Sprite2D.
	 Using a single sprite for each glyph is inefficient for OpenGL so we want large sprites composed of many glyphs.
	 However, we would also like to avoid creating many unused pages (for TTF fonts, etc) so we cannot simply create a single large page.
	 Furthermore, a single page may grow too large to be used as a texture in OpenGL (esp for mobile devices).
	 Therefore we generate pages of glyphs (1024 x LineHeight + descent) as sprites suitable for segmented blitting to the screen.
	 The pixel data will be safely shared between the Sprite2D objects and the Glyph objects when the video driver is using software rendering
	 so this isn't horribly wasteful useage of memory. Hardware acceleration will force the font to keep its own copy of pixel data.
	 
	 One odd quirk in this implementation is that, since TTF fonts cannot be pregenerated (efficiently), we must defer creation of a page sprite 
	 until either the page is full, or until a call to a print method is made.
	 To avoid generating more than one partial page, subsequent calls to add new glyphs will destroy the partial Sprite (not the source pixels of course)
	*/
private:
	class GlyphAtlasPage : public SpriteSheet<ieWord> {
		private:
			std::map<ieWord, Glyph> glyphs;
			ieByte* pageData; // current raw page being built
			int pageXPos; // current position on building page
			Font* font;
		public:
			GlyphAtlasPage(Size pageSize, Font* font)
			: SpriteSheet<ieWord>(), font(font)
			{
				pageXPos = 0;
				SheetRegion.w = pageSize.w;
				SheetRegion.h = pageSize.h;

				pageData = (ieByte*)calloc(pageSize.h, pageSize.w);
			}

			~GlyphAtlasPage() {
				if (Sheet == NULL) {
					free(pageData);
				} else {
					// TODO: need the driver check for shared pixel data
					// in this case the sprite took ownership of the pixels
				}
			}
		bool AddGlyph(ieWord chr, const Glyph& g);
		const Glyph& GlyphForChr(ieWord chr) const;

		// we need a non-const version of Draw here that will call the base const version
		using SpriteSheet::Draw;
		void Draw(ieWord chr, const Region& dest, Palette* pal = NULL);
	};

	// TODO: Unfortunately, we have no smart pointer suitable for an STL container...
	// if we ever transition to C++11 we can use one here
	typedef std::deque<GlyphAtlasPage*> GlyphAtlas;
	typedef std::map< ieWord, size_t > GlyphIndex;

	GlyphAtlasPage* CurrentAtlasPage;
	GlyphIndex AtlasIndex;
	GlyphAtlas Atlas;

	// BAM fonts use alisases a lot so this saves quite a bit of space
	std::map<ieWord, ieWord> AliasMap; // Aliases are 2 glyphs that share identical frames such as 'ā' and 'a'

	Palette* palette;
public:
	const int LineHeight;
	const int Baseline;
private:
	// Blit to the sprite or screen if canvas is NULL
	size_t RenderText(const String&, Region&, Palette*, ieByte alignment,
					  Point* = NULL, ieByte** canvas = NULL, bool grow = false) const;
	// render a single line of text. called by RenderText()
	size_t RenderLine(const String& string, const Region& rgn, Palette* hicolor,
					  Point& dp, ieByte** canvas = NULL) const;

public:
	Font(Palette*, ieWord lineheight, ieWord baseline);
	virtual ~Font();

	const Glyph& CreateGlyphForCharSprite(ieWord chr, const Sprite2D*);
	void CreateAliasForChar(ieWord chr, ieWord alias);

	//allow reading but not setting glyphs
	virtual const Glyph& GetGlyph(ieWord chr) const;

	Palette* GetPalette() const;
	void SetPalette(Palette* pal);

	int KerningOffset(ieWord /*leftChr*/, ieWord /*rightChr*/) const {return 0;};

	Sprite2D* RenderTextAsSprite(const String& string, const Size& size, ieByte alignment,
								 Palette* pal = NULL, size_t* numPrinted = NULL, Point* = NULL) const;

	// Printing methods
	// return the number of glyphs printed
	size_t Print(Region rgn, const char* string,
				 Palette* color, ieByte Alignment) const;
	// the "point" parameter can be passed with a start point for rendering
	// it will be filled with the point inside 'rgn' where the string ends upon return
	size_t Print(Region rgn, const String& string,
				 Palette* hicolor, ieByte Alignment, Point* point = NULL) const;

	/** Returns size of the string rendered in this font in pixels */
	Size StringSize(const String&, const Size* = NULL, size_t* = NULL) const;
};

}

#endif
