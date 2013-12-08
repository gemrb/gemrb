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

namespace GemRB {

enum FontStyle {
	NORMAL = 0x00,
	BOLD = 0x01,
	ITALIC = 0x02,
	UNDERLINE = 0x04
};

class Palette;
class Sprite2D;

#define IE_FONT_ALIGN_LEFT   0x00
#define IE_FONT_ALIGN_CENTER 0x01
#define IE_FONT_ALIGN_RIGHT  0x02
#define IE_FONT_ALIGN_BOTTOM 0x04
#define IE_FONT_ALIGN_TOP    0x10 //Single-Line and Multi-Line Text
#define IE_FONT_ALIGN_MIDDLE 0x20 //Only for single line Text
#define IE_FONT_SINGLE_LINE  0x40

#define IE_FONT_PADDING 5

/**
 * @class Font
 * Class for using and manipulating images serving as fonts
 */

class GEM_EXPORT Font {
protected:
	ieResRef* resRefs;
	int numResRefs;
	char name[20];

	Palette* palette;
	Sprite2D* blank;

	bool multibyte;
	bool utf8;

public:
	int maxHeight;

public:
	Font();
	virtual ~Font(void);

	//allow reading but not setting glyphs
	virtual const Sprite2D* GetCharSprite(ieWord chr) const = 0;

	bool AddResRef(const ieResRef resref);
	bool MatchesResRef(const ieResRef resref);

	const char* GetName() const {return name;};
	void SetName(const char* newName);

	virtual ieWord GetPointSize() const {return 0;};
	virtual FontStyle GetStyle() const {return NORMAL;};

	Palette* GetPalette() const;
	void SetPalette(Palette* pal);

	// Printing methods
	void Print(Region cliprgn, Region rgn, const unsigned char* string,
		Palette* color, ieByte Alignment, bool anchor = false,
		Font* initials = NULL, Sprite2D* cursor = NULL,
		unsigned int curpos = 0, bool NoColor = false) const;
	void Print(Region rgn, const unsigned char* string, Palette* color,
		ieByte Alignment, bool anchor = false,
		Font* initials = NULL, Sprite2D* cursor = NULL,
		unsigned int curpos = 0, bool NoColor = false) const;
	void PrintFromLine(int startrow, Region rgn, const unsigned char* string,
		Palette* color, ieByte Alignment,
		Font* initials = NULL, Sprite2D* cursor = NULL,
		unsigned int curpos = 0, bool NoColor = false) const;

	/** Returns width of the string rendered in this font in pixels */
	size_t CalcStringWidth(const unsigned char* string, bool NoColor = false) const;
	void SetupString(ieWord* string, unsigned int width, bool NoColor = false, Font *initials = NULL, bool enablecap = false) const;
	size_t GetDoubleByteString(const unsigned char* string, ieWord* &dbString) const;

protected:
	virtual int GetKerningOffset(ieWord /*leftChr*/, ieWord /*rightChr*/) const {return 0;};
private:
	int PrintInitial(int x, int y, const Region &rgn, ieWord currChar) const;
	size_t CalcStringWidth(const ieWord* string, bool NoColor = false) const;
	size_t CalcStringHeight(const ieWord* string, unsigned int len, bool NoColor) const;
	static size_t dbStrLen(const ieWord* string);
};

}

#endif
