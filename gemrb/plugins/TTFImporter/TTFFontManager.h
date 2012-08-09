/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2011 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

#ifndef GemRB_TTFFont_h
#define GemRB_TTFFont_h

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H

#include "FontManager.h"

namespace GemRB {

struct TTF_Font {
	FT_Face face;

	int height;
	int ascent;
	int descent;

	int glyph_overhang;
	float glyph_italics;

	int face_style;
};

class TTFFontManager : public FontManager {
/*
Private ivars
*/
private:
	FT_Library library;
	FT_Stream ftStream;

	TTF_Font font;
public:
/*
Public ivars
*/
private:
/*
Private methods
*/
	void LogFTError(FT_Error errCode) const;
public:
/*
Public methods
*/
	~TTFFontManager(void);
	TTFFontManager(void);

	bool Open(DataStream* stream);
	void Close();

	Font* GetFont(ieWord FirstChar,
				  ieWord LastChar,
				  unsigned short ptSize,
				  FontStyle style, Palette* pal = NULL);

	// freetype "callbacks"
	static unsigned long read( FT_Stream       stream,
							  unsigned long   offset,
							  unsigned char*  buffer,
							  unsigned long   count );
};

}

#endif
