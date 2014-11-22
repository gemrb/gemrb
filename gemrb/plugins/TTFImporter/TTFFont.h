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

#ifndef __GemRB__TTFFont__
#define __GemRB__TTFFont__

#include "Freetype.h"

#include "GUI/TextSystem/Font.h"
#include "HashMap.h"
#include "Holder.h"

namespace GemRB {

class TTFFont : public Font
{
private:
	FT_Face face;

	const Glyph& AliasBlank(ieWord chr) const;
protected:
	int GetKerningOffset(ieWord leftChr, ieWord rightChr) const;
public:
	TTFFont(Palette* pal, FT_Face face, int lineheight, int baseline);
	~TTFFont(void);

	const Glyph& GetGlyph(ieWord chr) const;
};

}

#endif /* defined(__GemRB__TTFFont__) */
