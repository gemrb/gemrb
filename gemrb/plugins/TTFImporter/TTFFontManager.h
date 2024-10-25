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

#include "FontManager.h"
#include "Freetype.h"

namespace GemRB {

class TTFFontManager : public FontManager {
private:
	FT_Stream ftStream = nullptr;
	FT_Face face = nullptr;

public:
	TTFFontManager() noexcept = default;
	TTFFontManager(const TTFFontManager&) = delete;
	~TTFFontManager() override;
	TTFFontManager& operator=(const TTFFontManager&) = delete;

	bool Import(DataStream* stream) override;
	void Close();

	Holder<Font> GetFont(unsigned short pxSize, FontStyle style, bool background) override;
};

}

#endif
