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

#include "BAMSprite2D.h"

#include "AnimationFactory.h"

namespace GemRB {

BAMSprite2D::BAMSprite2D(int Width, int Height, const void* pixels,
						 bool rle, AnimationFactory* datasrc,
						 Palette* palette, ieDword ck)
	: Sprite2D(Width, Height, 8, pixels)
{
	palette->acquire();
	pal = palette;
	colorkey = ck;
	RLE = rle;
	source = datasrc;
	datasrc->IncDataRefCount();
	BAM = true;
	freePixels = false; // managed by datasrc
}

BAMSprite2D::BAMSprite2D(const BAMSprite2D &obj)
	: Sprite2D(obj)
{
	assert(obj.pal);
	assert(obj.source);

	pal = obj.pal;
	pal->acquire();
	colorkey = obj.GetColorKey();
	RLE = obj.RLE;
	source = obj.source;
	source->IncDataRefCount();
	BAM = true;
	freePixels = false; // managed by datasrc
}

BAMSprite2D* BAMSprite2D::copy() const
{
	return new BAMSprite2D(*this);
}

BAMSprite2D::~BAMSprite2D()
{
	pal->release();
	source->DecDataRefCount();
}

/** Get the Palette of a Sprite */
Palette* BAMSprite2D::GetPalette() const
{
	pal->acquire();
	return pal;
}

void BAMSprite2D::SetPalette(Palette* palette)
{
	if (palette) {
		palette->acquire();
	}
	if (pal) {
		pal->release();
	}
	pal = palette;
}

Color BAMSprite2D::GetPixel(unsigned short x, unsigned short y) const
{
	Color c = { 0, 0, 0, 0 };
	if (x >= Width || y >= Height) return c;

	if (renderFlags&RENDER_FLIP_VERTICAL)
		y = Height - y - 1;
	if (renderFlags&RENDER_FLIP_HORIZONTAL)
		x = Width - x - 1;

	int skipcount = y * Width + x;

	const ieByte *rle = (const ieByte*)pixels;
	if (RLE) {
		while (skipcount > 0) {
			if (*rle++ == colorkey)
				skipcount -= (*rle++)+1;
			else
				skipcount--;
		}
	} else {
		// uncompressed
		rle += skipcount;
		skipcount = 0;
	}

	if (skipcount >= 0 && *rle != colorkey) {
		c = pal->col[*rle];
		c.a = 0xff;
	}
	return c;
}

}
