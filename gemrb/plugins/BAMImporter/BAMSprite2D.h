/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2013 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

#ifndef BAMSPRITE2D_H
#define BAMSPRITE2D_H

#include "Sprite2D.h"

namespace GemRB {

class AnimationFactory;

class BAMSprite2D : public Sprite2D {
private:
	PaletteHolder pal;
	ieByte colorkey;

public:
	// all BAMs have a palette and colorkey so force them at construction
	// for BAMs the actual colorkey is always green (RGB(0,255,0)) so use colorkey to store the transparency index
	BAMSprite2D(const Region&, void* pixels,
				PaletteHolder palette, ieDword colorkey);
	BAMSprite2D(const BAMSprite2D &obj);
	BAMSprite2D* copy() const;

	PaletteHolder GetPalette() const;
	void SetPalette(PaletteHolder pal);
	Color GetPixel(const Point&) const;
	int32_t GetColorKey() const { return colorkey; };
	void SetColorKey(ieDword ck) { colorkey = (ieByte)ck; };
	bool HasTransparency() const;
};

}

#endif  // ! BAMSPRITE2D_H
