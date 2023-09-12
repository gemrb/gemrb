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
 *
 * Font uses an "Atlas" instead of individual sprites for characters.
 * The TTF plugin will dynamically add to its atlas as characters are needed
 * to avoid building a ton of worthless pages. Additionally, fonts (especially
 * BAM based ones) use "alias" entries to avoid making duplicate glyphs.
 * @author The GemRB Project
 */

#ifndef FONT_H
#define FONT_H

#include "globals.h"
#include "exports.h"

#include "SpriteSheet.h"
#include "Strings/String.h"

#include <deque>
#include <map>

namespace GemRB {

enum FontStyle {
	NORMAL = 0x00,
	BOLD = 0x01,
	ITALIC = 0x02,
	UNDERLINE = 0x04
};

#define IE_FONT_ALIGN_LEFT   0x00
#define IE_FONT_ALIGN_CENTER 0x01
#define IE_FONT_ALIGN_RIGHT  0x02
#define IE_FONT_ALIGN_BOTTOM 0x04
#define IE_FONT_ALIGN_TOP    0x10
#define IE_FONT_ALIGN_MIDDLE 0x20
// optimization to make printing faster when text should be on a single line
#define IE_FONT_SINGLE_LINE  0x40
// work around to use the region passed to Font::Print as if it were the actual print size when the text cant actually fit the region
#define IE_FONT_NO_CALC		 0x80

struct Glyph {
	const Size size;
	const Point pos;

	const ieWord pitch;
	const ieByte* pixels;

	Glyph(const Size& size, Point pos, const ieByte* pixels, ieWord pitch)
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
public:
	struct PrintColors {
		Color fg;
		Color bg;
	};
	
	struct StringSizeMetrics {
		// specifiy behavior of StringSize based on values of struct
		// StringSize will modify the struct with the results
		Size size;		// maximum size allowed; updated with actual size <= initial value
		size_t numChars; // maximum characters to size (0 implies no limit); updated with the count of characters that fit within size <= initial value
		uint32_t numLines; // maximum number of lines to allow (use 0 for unlimited); updated with the actual number of lines that were used
		bool forceBreak;// whether or not a break can occur without whitespace; updated to false if initially true and no force break occured
	};

private:
	class GlyphAtlasPage : public SpriteSheet<ieWord> {
		private:
			using GlyphMap = std::map<ieWord, Glyph>;
			GlyphMap glyphs;
			ieByte* pageData; // current raw page being built
			int pageXPos = 0; // current position on building page
			Font* font;
			Holder<Sprite2D> invertedSheet;
		public:
			GlyphAtlasPage(Size pageSize, Font* font);
			GlyphAtlasPage(const GlyphAtlasPage&) = delete;
			GlyphAtlasPage& operator=(const GlyphAtlasPage&) = delete;
			~GlyphAtlasPage() override {
				if (Sheet == NULL) {
					free(pageData);
				}
			}
		bool AddGlyph(ieWord chr, const Glyph& g);
		const Glyph& GlyphForChr(ieWord chr) const;

		// we need a non-const version of Draw here that will call the base const version
		using SpriteSheet<ieWord>::Draw;
		void Draw(ieWord chr, const Region& dest, const PrintColors* colors);
		void DumpToScreen(const Region&) const;
	};

	struct GlyphIndexEntry {
		ieWord chr = 0;
		ieWord pageIdx = ieWord(-1);
		const Glyph* glyph = nullptr;

		GlyphIndexEntry() noexcept = default;
		GlyphIndexEntry(ieWord c, ieWord p, const Glyph* g) : chr(c), pageIdx(p), glyph(g) {}
	};

	using GlyphIndex = std::vector<GlyphIndexEntry>;
	// TODO: Unfortunately, we have no smart pointer suitable for an STL container...
	// if we ever transition to C++11 we can use one here
	using GlyphAtlas = std::deque<GlyphAtlasPage*>;

	GlyphAtlasPage* CurrentAtlasPage = nullptr;
	GlyphIndex AtlasIndex;
	GlyphAtlas Atlas;

protected:
	Holder<Palette> palette;
	bool background = false;

public:
	const int LineHeight;
	const int Baseline;

private:
	void CreateGlyphIndex(ieWord chr, ieWord pageIdx, const Glyph*);
	// Blit to the sprite or screen if canvas is NULL
	size_t RenderText(const String&, Region&, ieByte alignment, const PrintColors*,
					  Point* = NULL, ieByte** canvas = NULL, bool grow = false) const;
	// render a single line of text. called by RenderText()
	size_t RenderLine(const String& string, const Region& rgn,
					  Point& dp, const PrintColors*, ieByte** canvas = NULL) const;
	
	size_t Print(Region rgn, const String& string, ieByte Alignment, const PrintColors* colors, Point* point = nullptr) const;

public:
	Font(Holder<Palette> pal, ieWord lineheight, ieWord baseline, bool bg);
	Font(const Font&) = delete;
	virtual ~Font();
	Font& operator=(const Font&) = delete;

	const Glyph& CreateGlyphForCharSprite(ieWord chr, const Holder<Sprite2D>&);
	// BAM fonts use alisases a lot so this saves quite a bit of space
	// Aliases are 2 glyphs that share identical frames such as 'ā' and 'a'
	void CreateAliasForChar(ieWord chr, ieWord alias);

	//allow reading but not setting glyphs
	virtual const Glyph& GetGlyph(ieWord chr) const;

	virtual int GetKerningOffset(ieWord /*leftChr*/, ieWord /*rightChr*/) const {return 0;};

	// return the number of glyphs printed
	// the "point" parameter can be passed with a start point for rendering
	// it will be filled with the point inside 'rgn' where the string ends upon return
	
	size_t Print(const Region& rgn, const String& string, ieByte Alignment, Point* point = nullptr) const;
	size_t Print(const Region& rgn, const String& string, ieByte Alignment, const PrintColors& colors, Point* point = nullptr) const;
	
	size_t Print(Region rgn, const String& string,
				 Holder<Palette> hicolor, ieByte Alignment, Point* point = nullptr) const;

	/** Returns size of the string rendered in this font in pixels */
	Size StringSize(const String&, StringSizeMetrics* metrics = NULL) const;

	// like StringSize, but single line and doens't take whitespace into consideration
	size_t StringSizeWidth(const String&, size_t width, size_t* numChars = NULL) const;
};

}

#endif
