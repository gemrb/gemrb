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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/Font.h,v 1.16 2004/09/11 07:43:55 edheldil Exp $
 *
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

class GEM_EXPORT Font {
private:
	//std::vector<Sprite2D*> chars;
	int count;
	Color* palette;
	Sprite2D* sprBuffer;
public:
	Font(int w, int h, void* palette, bool cK, int index);
	~Font(void);
	void AddChar(void* spr, int w, int h, short xPos, short yPos);
	void Print(Region rgn, unsigned char* string, Color* color,
		unsigned char Alignment, bool anchor = false, Font* initials = NULL,
		Color* initcolor = NULL, Sprite2D* cursor = NULL, unsigned int curpos = 0);
	void PrintFromLine(int startrow, Region rgn, unsigned char* string,
		Color* color, unsigned char Alignment, bool anchor = false,
		Font* initials = NULL, Color* initcolor = NULL,
		Sprite2D* cursor = NULL, unsigned int curpos = 0);
	void* GetPalette();
	char ResRef[9];
	Region size[256];
	short xPos[256];
	short yPos[256];
	int maxHeight;
	int CalcStringWidth(char* string);
public:
	void SetupString(char* string, unsigned int width);
};

#endif
