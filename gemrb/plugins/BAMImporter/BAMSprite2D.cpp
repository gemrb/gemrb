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
#include "Video.h"

namespace GemRB {

BAMSprite2D::BAMSprite2D(const Region& rgn, void* pixels,
						 PaletteHolder palette, ieDword ck)
	: Sprite2D(rgn, 8, pixels)
{
	pal = palette;
	colorkey = ck;
	BAM = true;
	freePixels = false; // managed by AnimationFactory

	assert(pal);
}

BAMSprite2D::BAMSprite2D(const BAMSprite2D &obj)
	: Sprite2D(obj)
{
	assert(obj.pal);

	pal = obj.pal;
	colorkey = obj.GetColorKey();

	BAM = true;
	freePixels = false; // managed by AnimationFactory
}

Holder<Sprite2D> BAMSprite2D::copy() const
{
	return new BAMSprite2D(*this);
}

/** Get the Palette of a Sprite */
PaletteHolder BAMSprite2D::GetPalette() const
{
	return pal;
}

void BAMSprite2D::SetPalette(PaletteHolder palette)
{
	if (palette == NULL) {
		Log(WARNING, "BAMSprite2D", "cannot set a NULL palette.");
		return;
	}

	pal = palette;
}

Color BAMSprite2D::GetPixel(const Point& p) const
{
	Color c;

	if (p.x < 0 || p.x >= Frame.w) return c;
	if (p.y < 0 || p.y >= Frame.h) return c;

	int x = p.x;
	int y = p.y;

	if (renderFlags&BlitFlags::MIRRORY)
		y = Frame.h - y - 1;
	if (renderFlags&BlitFlags::MIRRORX)
		x = Frame.w - x - 1;

	int skipcount = y * Frame.w + x;

	const ieByte *rle = (const ieByte*)pixels;
	while (skipcount > 0) {
		if (*rle++ == colorkey)
			skipcount -= (*rle++)+1;
		else
			skipcount--;
	}

	if (skipcount >= 0 && *rle != colorkey) {
		c = pal->col[*rle];
		c.a = 0xff;
	}
	return c;
}

bool BAMSprite2D::HasTransparency() const
{
	// BAMs always use green for transparency and if colorkey > 0 it guarantees we have green
	return colorkey > 0 || (pal->col[0].r == 0 && pal->col[0].g == 0xff && pal->col[0].b == 0);
}

}
