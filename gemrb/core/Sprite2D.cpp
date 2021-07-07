/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
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

#include "Sprite2D.h"

namespace GemRB {

const TypeID Sprite2D::ID = { "Sprite2D" };

Sprite2D::Sprite2D(const Region& rgn, void* pixels, const PixelFormat& fmt, uint16_t pitch) noexcept
: pixels(pixels), freePixels(pixels), format(fmt), pitch(pitch), Frame(rgn)
{}

Sprite2D::Sprite2D(const Region& frame, void* pixels, const PixelFormat& fmt) noexcept
: Sprite2D(frame, pixels, fmt, frame.w)
{}

Sprite2D::Sprite2D(const Sprite2D &obj) noexcept
: Sprite2D(obj.Frame, obj.pixels, obj.format, obj.pitch)
{
	renderFlags = obj.renderFlags;
	freePixels = false;
}

Sprite2D::Sprite2D(Sprite2D&& obj) noexcept
: Sprite2D(obj.Frame, obj.pixels, obj.format, obj.pitch)
{
	renderFlags = obj.renderFlags;
}

Sprite2D::~Sprite2D() noexcept
{
	if (freePixels) {
		free(pixels);
	}
}

bool Sprite2D::HasTransparency() const noexcept
{
	return format.HasColorKey || format.Amask;
}

bool Sprite2D::IsPixelTransparent(const Point& p) const noexcept
{
	if (HasTransparency()) {
		return GetPixel(p).a == 0;
	}
	return false;
}

const void* Sprite2D::LockSprite() const
{
	return pixels;
}

void* Sprite2D::LockSprite()
{
	return pixels;
}

void Sprite2D::SetPalette(PaletteHolder pal)
{
	assert(format.Bpp == 1);
	assert(pal != nullptr);

	if (pal == format.palette) return;
	
	if (format.RLE) {
		format.palette = pal;
	} else {
		// we don't use shared palettes because it is a performance bottleneck on SDL2
		format.palette = pal->Copy();
	}
	
	UpdatePalette(format.palette);
}

void Sprite2D::SetColorKey(colorkey_t key)
{
	format.ColorKey = key;
	UpdateColorKey(key);
}

}
