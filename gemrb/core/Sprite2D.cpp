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
{
	format = obj.format;
	Frame = obj.Frame;
	renderFlags = obj.renderFlags;

	pixels = obj.pixels;
	freePixels = false;
}

Sprite2D::~Sprite2D() noexcept
{
	if (freePixels) {
		free(pixels);
	}
}

Color Sprite2D::GetPixel(int x, int y) const
{
	return GetPixel(Point(x, y));
}

bool Sprite2D::IsPixelTransparent(const Point& p) const
{
	return GetPixel(p).a == 0;
}

const void* Sprite2D::LockSprite() const
{
	return pixels;
}

void* Sprite2D::LockSprite()
{
	return pixels;
}

void Sprite2D::UnlockSprite() const
{}

}
