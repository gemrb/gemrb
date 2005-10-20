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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/Font.h,v 1.22 2005/10/20 23:13:14 edheldil Exp $
 *
 */

/**
 * @file Font.h
 * Declares Font, class for manipulating images serving as fonts
 * @author The GemRB Project
 */

#ifndef FONT_H
#define FONT_H

#include "../../includes/globals.h"
#include <vector>

typedef struct StringList {
	Sprite2D*** strings;
	unsigned int* heights;
	unsigned int* lengths;
	int StringCount;
	int starty;
	int curx;
	int cury;
} StringList;

#ifdef WIN32

#ifdef GEM_BUILD_DLL
#define GEM_EXPORT __declspec(dllexport)
#else
#define GEM_EXPORT __declspec(dllimport)
#endif

#else
#define GEM_EXPORT
#endif

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
private:
	int count;
	Color* palette;
	Sprite2D* sprBuffer;
	unsigned char FirstChar;

	short xPos[256];
	short yPos[256];
public:
	/** ResRef of the Font image */
	ieResRef ResRef;
	int maxHeight;
	Region size[256];

public:
	Font(int w, int h, void* palette, bool cK, int index);
	~Font(void);
	void AddChar(void* spr, int w, int h, short xPos, short yPos);
	void Print(Region rgn, unsigned char* string, Color* color,
		unsigned char Alignment, bool anchor = false,
		Font* initials = NULL, Sprite2D* cursor = NULL,
		unsigned int curpos = 0);
	void PrintFromLine(int startrow, Region rgn, unsigned char* string,
		Color* color, unsigned char Alignment,
		Font* initials = NULL, Sprite2D* cursor = NULL,
		unsigned int curpos = 0);
	void* GetPalette();
	/** Returns width of the string rendered in this font in pixels */
	int CalcStringWidth(char* string);
	void SetupString(char* string, unsigned int width);
	/** Sets ASCII code of the first character in the font.
	 * (it allows remapping numeric fonts from \000 to '0') */
	void SetFirstChar(unsigned char first);

private:
	int PrintInitial(int x, int y, Region &rgn, unsigned char currChar);
};

#endif
