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

#include "Freetype.h"

#include "FontManager.h"

namespace GemRB {

class TTFFontManager : public FontManager {
/*
Private ivars
*/
private:
	FT_Stream ftStream;
	FT_Face face;

public:
/*
Public methods
*/
	~TTFFontManager(void);
	TTFFontManager(void);

	bool Open(DataStream* stream) override;
	void Close();

	Font* GetFont(unsigned short pxSize,
				  FontStyle style, PaletteHolder pal = nullptr) override;

	// freetype "callbacks"
	static unsigned long read( FT_Stream       stream,
							  unsigned long   offset,
							  unsigned char*  buffer,
							  unsigned long   count );

	static void close( FT_Stream stream );
};

}

#endif
