/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2021 The GemRB Project
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

#include "Pixels.h"

#include "RLE.h"

#include "Logging/Logging.h"

namespace GemRB {

IPixelIterator* PixelFormatIterator::InitImp(void* pixel, int pitch) const noexcept
{
	if (format.RLE) {
		pixel = [this, pitch](uint8_t* rledata) {
			Point p;

			if (ydir == Reverse)
				p.y = clip.h_get() - 1;
			if (xdir == Reverse)
				p.x = clip.w_get() - 1;

			return FindRLEPos(rledata, pitch, p, format.ColorKey);
		}(static_cast<uint8_t*>(pixel));
		return new RLEIterator(static_cast<uint8_t*>(pixel), xdir, ydir, Size(clip.w_get(), clip.h_get()), format.ColorKey);
	} else {
		pixel = [this, pitch](uint8_t* pixels) {
			if (xdir == Reverse) {
				pixels += format.Bpp * (clip.w_get() - 1);
			}
			if (ydir == Reverse) {
				pixels += pitch * (clip.h_get() - 1);
			}

			pixels += (clip.y_get() * pitch) + (clip.x_get() * format.Bpp);
			return pixels;
		}(static_cast<uint8_t*>(pixel));

		switch (format.Bpp) {
			case 4:
				return new PixelIterator<uint32_t>(static_cast<uint32_t*>(pixel), xdir, ydir, Size(clip.w_get(), clip.h_get()), pitch);
			case 3:
				return new PixelIterator<Pixel24Bit>(static_cast<Pixel24Bit*>(pixel), xdir, ydir, Size(clip.w_get(), clip.h_get()), pitch);
			case 2:
				return new PixelIterator<uint16_t>(static_cast<uint16_t*>(pixel), xdir, ydir, Size(clip.w_get(), clip.h_get()), pitch);
			case 1:
				return new PixelIterator<uint8_t>(static_cast<uint8_t*>(pixel), xdir, ydir, Size(clip.w_get(), clip.h_get()), pitch);
			default:
				ERROR_UNKNOWN_BPP;
		}
	}
}

PixelFormatIterator::PixelFormatIterator(void* pixels, int pitch, const PixelFormat& fmt, const Region& clip) noexcept
	: PixelFormatIterator(pixels, pitch, fmt, Forward, Forward, clip)
{}

PixelFormatIterator::PixelFormatIterator(void* pixels, int pitch, const PixelFormat& fmt, Direction x, Direction y, const Region& clip) noexcept
	: IPixelIterator(pixels, pitch, x, y), format(fmt), clip(clip)
{
	imp = InitImp(pixels, pitch);
}

PixelFormatIterator::PixelFormatIterator(const PixelFormatIterator& orig) noexcept
	: IPixelIterator(orig), format(orig.format)
{
	clip = orig.clip;
	imp = orig.imp->Clone();
}

PixelFormatIterator::~PixelFormatIterator() noexcept
{
	delete imp;
}

PixelFormatIterator PixelFormatIterator::end(const PixelFormatIterator& beg) noexcept
{
	if (beg.clip.w_get() == 0 || beg.clip.h_get() == 0) {
		// already at the end
		return PixelFormatIterator(beg);
	}

	Direction xdir = (beg.xdir == Forward) ? Reverse : Forward;
	Direction ydir = (beg.ydir == Forward) ? Reverse : Forward;
	PixelFormatIterator it(beg);
	// hack for InitImp
	it.ydir = ydir;
	it.xdir = xdir;
	delete it.imp;
	it.imp = it.InitImp(it.pixel, beg.pitch);
	// reset for Advance
	it.xdir = beg.xdir;
	it.ydir = beg.ydir;
	it.imp->xdir = beg.xdir;
	it.imp->ydir = beg.ydir;

	// 'end' iterators are one past the end
	it.Advance(1);
	return it;
}

PixelFormatIterator& PixelFormatIterator::operator++() noexcept
{
	imp->Advance(1);
	return *this;
}

bool PixelFormatIterator::operator!=(const PixelFormatIterator& rhs) const noexcept
{
	return imp->pixel != rhs.imp->pixel;
}

uint8_t& PixelFormatIterator::operator*() const noexcept
{
	return *static_cast<uint8_t*>(imp->pixel);
}

uint8_t* PixelFormatIterator::operator->() const noexcept
{
	return static_cast<uint8_t*>(imp->pixel);
}

uint8_t PixelFormatIterator::Channel(uint32_t mask, uint8_t shift) const noexcept
{
	switch (format.Bpp) {
		case 1:
			return static_cast<PixelIterator<uint8_t>*>(imp)->Channel(mask, shift);
		case 2:
			return static_cast<PixelIterator<uint16_t>*>(imp)->Channel(mask, shift);
		case 3:
			return static_cast<PixelIterator<Pixel24Bit>*>(imp)->Channel(mask, shift);
		case 4:
			return static_cast<PixelIterator<uint32_t>*>(imp)->Channel(mask, shift);
		default:
			ERROR_UNKNOWN_BPP;
	}
}

IPixelIterator* PixelFormatIterator::Clone() const noexcept
{
	return new PixelFormatIterator(*this);
}

void PixelFormatIterator::Advance(int amt) noexcept
{
	imp->Advance(amt);
}

Color PixelFormatIterator::ReadRGBA() const noexcept
{
	Color c;
	ReadRGBA(c.r, c.g, c.b, c.a);
	return c;
}

void PixelFormatIterator::ReadRGBA(uint8_t& r, uint8_t& g, uint8_t& b, uint8_t& a) const noexcept
{
	uint32_t pixel = 0;
	switch (format.Bpp) {
		case 4:
			pixel = *static_cast<uint32_t*>(imp->pixel);
			break;
		case 3:
			pixel = *static_cast<Pixel24Bit*>(imp->pixel);
			break;
		case 2:
			pixel = *static_cast<uint16_t*>(imp->pixel);
			break;
		case 1:
			{
				pixel = *static_cast<uint8_t*>(imp->pixel);
				auto& c = format.palette->GetColorAt(pixel);
				r = c.r;
				g = c.g;
				b = c.b;

				if (format.HasColorKey && pixel == format.ColorKey) {
					a = 0;
				} else {
					a = c.a;
				}
				return;
			}
		default:
			ERROR_UNKNOWN_BPP;
	}

	unsigned v;
	v = (pixel & format.Rmask) >> format.Rshift;
	r = (v << format.Rloss) + (v >> (8 - (format.Rloss << 1)));
	v = (pixel & format.Gmask) >> format.Gshift;
	g = (v << format.Gloss) + (v >> (8 - (format.Gloss << 1)));
	v = (pixel & format.Bmask) >> format.Bshift;
	b = (v << format.Bloss) + (v >> (8 - (format.Bloss << 1)));
	if (format.Amask) {
		v = (pixel & format.Amask) >> format.Ashift;
		a = (v << format.Aloss) + (v >> (8 - (format.Aloss << 1)));
	} else if (format.HasColorKey && pixel == format.ColorKey) {
		a = 0;
	} else {
		a = 255;
	}
}

void PixelFormatIterator::WriteRGBA(uint8_t r, uint8_t g, uint8_t b, uint8_t a) noexcept
{
	if (format.Bpp == 1) {
		assert(false);
		// FIXME: if we want to be able to write back to paletted sprites we need
		// to implement a way to map the color to the palette
		//uint32_t pixel = format.palette->FindColor(Color(r, g, b, a));
		//*static_cast<uint8_t*>(imp->pixel) = pixel;
		return;
	}

	uint32_t pixel = (r >> format.Rloss) << format.Rshift | (g >> format.Gloss) << format.Gshift | (b >> format.Bloss) << format.Bshift | ((a >> format.Aloss) << format.Ashift & format.Amask);

	switch (format.Bpp) {
		case 4:
			*static_cast<uint32_t*>(imp->pixel) = pixel;
			break;
		case 3:
			*static_cast<Pixel24Bit*>(imp->pixel) = pixel;
			break;
		case 2:
			*static_cast<uint16_t*>(imp->pixel) = pixel;
			break;
		default:
			ERROR_UNKNOWN_BPP;
	}
}

const Point& PixelFormatIterator::Position() const noexcept
{
	return imp->Position();
}

}
