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

#include "Video/RLE.h"

namespace GemRB {

const TypeID Sprite2D::ID = { "Sprite2D" };

Sprite2D::Iterator Sprite2D::GetIterator(IPixelIterator::Direction xdir, IPixelIterator::Direction ydir)
{
	return GetIterator(xdir, ydir, Region(Point(), Frame.size));
}

Sprite2D::Iterator Sprite2D::GetIterator(IPixelIterator::Direction xdir, IPixelIterator::Direction ydir,
					 const Region& clip)
{
	if (renderFlags & BlitFlags::MIRRORX) xdir = IPixelIterator::Direction(int(xdir) * -1);
	if (renderFlags & BlitFlags::MIRRORY) ydir = IPixelIterator::Direction(int(ydir) * -1);
	return Iterator(pixels, pitch, format, xdir, ydir, clip);
}

Sprite2D::Iterator Sprite2D::GetIterator(IPixelIterator::Direction xdir, IPixelIterator::Direction ydir) const
{
	return GetIterator(xdir, ydir, Region(Point(), Frame.size));
}

Sprite2D::Iterator Sprite2D::GetIterator(IPixelIterator::Direction xdir, IPixelIterator::Direction ydir,
					 const Region& clip) const
{
	if (renderFlags & BlitFlags::MIRRORX) xdir = IPixelIterator::Direction(int(xdir) * -1);
	if (renderFlags & BlitFlags::MIRRORY) ydir = IPixelIterator::Direction(int(ydir) * -1);
	return Iterator(pixels, pitch, format, xdir, ydir, clip);
}

Sprite2D::Sprite2D(const Region& rgn, void* pixels, const PixelFormat& fmt, uint16_t pitch) noexcept
	: pixels(pixels), freePixels(pixels), format(fmt), pitch(pitch), Frame(rgn)
{}

Sprite2D::Sprite2D(const Region& frame, void* pixels, const PixelFormat& fmt) noexcept
	: Sprite2D(frame, pixels, fmt, frame.w_get())
{}

Sprite2D::Sprite2D(const Sprite2D& obj) noexcept
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

Color Sprite2D::GetPixel(const Point& p) const noexcept
{
	if (Region(0, 0, Frame.w_get(), Frame.h_get()).PointInside(p)) {
		Iterator it = GetIterator();
		it.Advance(p.y * Frame.w_get() + p.x);
		return it.ReadRGBA();
	}
	return Color();
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

void Sprite2D::SetPalette(const Holder<Palette>& pal)
{
	assert(format.Bpp == 1);
	assert(pal != nullptr);

	if (pal == format.palette) return;

	if (format.RLE) {
		format.palette = pal;
	} else {
		// we don't use shared palettes because it is a performance bottleneck on SDL2
		format.palette = MakeHolder<Palette>(*pal);
	}

	UpdatePalette();
}

void Sprite2D::SetColorKey(colorkey_t key)
{
	format.ColorKey = key;
	UpdateColorKey();
}

bool Sprite2D::ConvertFormatTo(const PixelFormat& newfmt) noexcept
{
	// we rely on the subclasses to handle most format conversions
	if (format.RLE && newfmt.RLE == false && newfmt.Bpp == 1) {
		uint8_t* rledata = static_cast<uint8_t*>(pixels);
		pixels = DecodeRLEData(rledata, Frame.size, format.ColorKey);
		if (freePixels) {
			free(rledata);
		} else {
			freePixels = true;
		}
		format = newfmt;
		assert(format.palette);
		return true;
	}
	return false;
}

}
