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

#include <SDL.h>
#include <SDL_ttf.h>

#include "FontManager.h"

namespace GemRB {

class TTFFontManager : public FontManager {
/*
Private ivars
*/
private:
	char FontPath[_MAX_PATH];
	FT_Library library;
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

	Font* GetFont(ieWord FirstChar,
				  ieWord LastChar,
				  unsigned short ptSize,
				  FontStyle style, Palette* pal = NULL);
};

}

#endif
