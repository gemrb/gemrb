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

#include <vector>

class Palette;
class Sprite2D;

struct StringList {
	Sprite2D*** strings;
	unsigned int* heights;
	unsigned int* lengths;
	int StringCount;
	int starty;
	int curx;
	int cury;
};

#define IE_FONT_ALIGN_LEFT   0x00
#define IE_FONT_ALIGN_CENTER 0x01
#define IE_FONT_ALIGN_RIGHT  0x02
#define IE_FONT_ALIGN_BOTTOM 0x04
#define IE_FONT_ALIGN_TOP    0x10 //Single-Line and Multi-Line Text
#define IE_FONT_ALIGN_MIDDLE 0x20 //Only for single line Text
#define IE_FONT_SINGLE_LINE  0x40

/**
 * @class Font
 * Class for using and manipulating images serving as fonts
 */

class GEM_EXPORT Font {
public:
	struct GlyphInfo {
		short xPos, yPos; // tracks glyph adjustment
		Region size;
	};
private:
	int count;
	int glyphCount;

	Palette* palette;
	Sprite2D* sprBuffer;
	unsigned char FirstChar;

	// For the temporary bitmap
	unsigned char* tmpPixels;
	unsigned int width, height;
	std::vector<GlyphInfo> glyphInfo;
public:
	/** ResRef of the Font image */
	ieResRef ResRef;
	int maxHeight;
public:
	Font(int w, int h, Palette* palette);
	~Font(void);

	//allow reading but not setting glyphs
	const GlyphInfo& getInfo (ieWord chr) const;

	void AddChar(unsigned char* spr, int w, int h, short xPos, short yPos);
	/** Call this after adding all characters */
	void FinalizeSprite(bool cK, int index);

	void Print(Region cliprgn, Region rgn, const unsigned char* string,
		Palette* color, unsigned char Alignment, bool anchor = false,
		Font* initials = NULL, Sprite2D* cursor = NULL,
		unsigned int curpos = 0, bool NoColor = false) const;
	void Print(Region rgn, const unsigned char* string, Palette* color,
		unsigned char Alignment, bool anchor = false,
		Font* initials = NULL, Sprite2D* cursor = NULL,
		unsigned int curpos = 0, bool NoColor = false) const;
	void PrintFromLine(int startrow, Region rgn, const unsigned char* string,
		Palette* color, unsigned char Alignment,
		Font* initials = NULL, Sprite2D* cursor = NULL,
		unsigned int curpos = 0, bool NoColor = false) const;

	Palette* GetPalette() const;
	void SetPalette(Palette* pal);
	/** Returns width of the string rendered in this font in pixels */
	int CalcStringWidth(const char* string, bool NoColor = false) const;
	void SetupString(char* string, unsigned int width, bool NoColor = false, Font *initials = NULL, bool enablecap = false) const;
	/** Sets ASCII code of the first character in the font.
	 * (it allows remapping numeric fonts from \000 to '0') */
	void SetFirstChar(unsigned char first);

private:
	int PrintInitial(int x, int y, const Region &rgn, unsigned char currChar) const;
};

#endif
