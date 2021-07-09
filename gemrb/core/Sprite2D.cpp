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

class RLEIterator : public PixelIterator<uint8_t>
{
	uint8_t* dataPos = nullptr;
	uint8_t colorkey = 0;
	uint16_t transQueue = 0;
public:
	RLEIterator(uint8_t* p, Size s, colorkey_t ck)
	: RLEIterator(p, Forward, Forward, s, ck)
	{}

	RLEIterator(uint8_t* p, Direction x, Direction y, Size s, colorkey_t ck)
	: PixelIterator(p, x, y, s, s.w), dataPos(p), colorkey(ck)
	{}

	IPixelIterator* Clone() const noexcept override {
		return new RLEIterator(*this);
	}

	void Advance(int dx) noexcept override {
		if (dx == 0 || size.IsInvalid()) return;
		
		int pixelsToAdvance = xdir * dx;
		int rowsToAdvance = std::abs(pixelsToAdvance / size.w);
		int xToAdvance = pixelsToAdvance % size.w;
		int tmpx = pos.x + xToAdvance;
		
		if (tmpx < 0) {
			++rowsToAdvance;
			tmpx = size.w + tmpx;
			xToAdvance = tmpx - pos.x;
		} else if (tmpx >= size.w) {
			++rowsToAdvance;
			tmpx = tmpx - size.w;
			xToAdvance = tmpx - pos.x;
		}
		
		if (dx < 0) {
			pos.y -= rowsToAdvance;
		} else {
			pos.y += rowsToAdvance;
		}

		pos.x = tmpx;
		assert(pos.x >= 0 && pos.x < size.w);
		
		while (pixelsToAdvance) {
			if (transQueue) {
				// dataPos is pointing to a "count" field
				// transQueue is the remaining number of pixels we must move "right" to increment dataPos
				// *dataPos - transQueue is the number of pixels we must move "left" to decrement dataPos
				if (pixelsToAdvance > 0) {
					// moving to the right
					if (transQueue < pixelsToAdvance) {
						pixelsToAdvance -= transQueue;
						transQueue = 0;
					} else {
						pixelsToAdvance = 0;
					}
				} else {
					// moving to the left
					if (*dataPos - transQueue < -pixelsToAdvance) {
						pixelsToAdvance += *dataPos - transQueue;
						transQueue = 0;
					} else {
						pixelsToAdvance = 0;
					}
				}
			} else {
				// dataPos is pointing to a pixel
				pixel = dataPos;
				if (*dataPos == colorkey) {
					transQueue = *++dataPos;
				}
				--pixelsToAdvance;
			}
		}
	}
	
	const Point& Position() const noexcept override {
		return pos;
	}
};

IPixelIterator* Sprite2D::Iterator::InitImp(void* pixel, int pitch) const noexcept
{
	if (format->RLE) {
		pixel = [this, pitch](uint8_t* rledata) {
			int x = 0;
			int y = 0;

			if (ydir == Reverse)
				y = clip.h - 1;
			if (xdir == Reverse)
				x = clip.w - 1;

			int skipcount = y * pitch + x;
			while (skipcount > 0) {
				if (*rledata++ == format->ColorKey)
					skipcount -= (*rledata++)+1;
				else
					skipcount--;
			}

			return rledata;
		} (static_cast<uint8_t*>(pixel));
		return new RLEIterator(static_cast<uint8_t*>(pixel), xdir, ydir, Size(clip.w, clip.h), format->ColorKey);
	} else {
		pixel = [this, pitch](uint8_t* pixels) {
			if (xdir == Reverse) {
				pixels += format->Bpp * (clip.w - 1);
			}
			if (ydir == Reverse) {
				pixels += pitch * (clip.h - 1);
			}

			pixels += (clip.y * pitch) + (clip.x * format->Bpp);
			return pixels;
		} (static_cast<uint8_t*>(pixel));
		
		switch (format->Bpp) {
			case 4:
				return new PixelIterator<uint32_t>(static_cast<uint32_t*>(pixel), xdir, ydir, Size(clip.w, clip.h), pitch);
			case 3:
				return new PixelIterator<Pixel24Bit>(static_cast<Pixel24Bit*>(pixel), xdir, ydir, Size(clip.w, clip.h), pitch);
			case 2:
				return new PixelIterator<uint16_t>(static_cast<uint16_t*>(pixel), xdir, ydir, Size(clip.w, clip.h), pitch);
			case 1:
				return new PixelIterator<uint8_t>(static_cast<uint8_t*>(pixel), xdir, ydir, Size(clip.w, clip.h), pitch);
			default:
				ERROR_UNKNOWN_BPP;
		}
	}
}

Sprite2D::Iterator::Iterator(Sprite2D& spr) noexcept
: Iterator(spr, spr.Frame)
{}

Sprite2D::Iterator::Iterator(Sprite2D& spr, const Region& clip) noexcept
: Iterator(spr, Forward, Forward, clip)
{}

Sprite2D::Iterator::Iterator(Sprite2D& spr, Direction x, Direction y) noexcept
: Iterator(spr, x, y, spr.Frame)
{}

Sprite2D::Iterator::Iterator(Sprite2D& spr, Direction x, Direction y, const Region& clip) noexcept
: IPixelIterator(nullptr, spr.pitch, x, y), imp(nullptr), format(&spr.format), clip(clip)
{
	imp = InitImp(spr.pixels, spr.pitch);
	pixel = spr.pixels; // always here regardless of direction
}

Sprite2D::Iterator::Iterator(const Iterator& orig) noexcept
: IPixelIterator(orig)
{
	clip = orig.clip;
	format = orig.format;
	imp = orig.imp->Clone();
}

Sprite2D::Iterator::~Iterator() noexcept
{
	delete imp;
}

Sprite2D::Iterator Sprite2D::Iterator::end(const Iterator& beg) noexcept
{
	if (beg.clip.w == 0 || beg.clip.h == 0) {
		// already at the end
		return Iterator(beg);
	}
	
	Direction xdir = (beg.xdir == Forward) ? Reverse : Forward;
	Direction ydir = (beg.ydir == Forward) ? Reverse : Forward;
	Iterator it(beg);
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

Sprite2D::Iterator& Sprite2D::Iterator::operator++() noexcept
{
	imp->Advance(1);
	return *this;
}

bool Sprite2D::Iterator::operator!=(const Iterator& rhs) const noexcept
{
	return imp->pixel != rhs.imp->pixel;
}

uint8_t& Sprite2D::Iterator::operator*() const noexcept
{
	return *static_cast<uint8_t*>(imp->pixel);
}

uint8_t* Sprite2D::Iterator::operator->() const noexcept
{
	return static_cast<uint8_t*>(imp->pixel);
}

uint8_t Sprite2D::Iterator::Channel(uint32_t mask, uint8_t shift) const noexcept
{
	switch (format->Bpp) {
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

IPixelIterator* Sprite2D::Iterator::Clone() const noexcept
{
	return new Iterator(*this);
}

void Sprite2D::Iterator::Advance(int amt) noexcept
{
	imp->Advance(amt);
}

Color Sprite2D::Iterator::ReadRGBA() const noexcept
{
	Color c;
	ReadRGBA(c.r, c.g, c.b, c.a);
	return c;
}

void Sprite2D::Iterator::ReadRGBA(uint8_t& r, uint8_t& g, uint8_t& b, uint8_t& a) const noexcept
{
	uint32_t pixel = 0;
	switch (format->Bpp) {
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
			pixel = *static_cast<uint8_t*>(imp->pixel);
			r = format->palette->col[pixel].r;
			g = format->palette->col[pixel].g;
			b = format->palette->col[pixel].b;

			if (format->HasColorKey && pixel == format->ColorKey) {
				a = 0;
			} else {
				a = format->palette->col[pixel].a;
			}
			return;
		default:
			ERROR_UNKNOWN_BPP;
	}

	unsigned v;
	v = (pixel & format->Rmask) >> format->Rshift;
	r = (v << format->Rloss) + (v >> (8 - (format->Rloss << 1)));
	v = (pixel & format->Gmask) >> format->Gshift;
	g = (v << format->Gloss) + (v >> (8 - (format->Gloss << 1)));
	v = (pixel & format->Bmask) >> format->Bshift;
	b = (v << format->Bloss) + (v >> (8 - (format->Bloss << 1)));
	if(format->Amask) {
		v = (pixel & format->Amask) >> format->Ashift;
		a = (v << format->Aloss) + (v >> (8 - (format->Aloss << 1)));
	} else if (format->HasColorKey && pixel == format->ColorKey) {
		a = 0;
	} else {
		a = 255;
	}
}

void Sprite2D::Iterator::WriteRGBA(uint8_t r, uint8_t g, uint8_t b, uint8_t a) noexcept
{
	if (format->Bpp == 1) {
		assert(false);
		// FIXME: if we want to be able to write back to paletted sprites we need
		// to implement a way to map the color to the palette
		//uint32_t pixel = format->palette->FindColor(Color(r, g, b, a));
		//*static_cast<uint8_t*>(imp->pixel) = pixel;
		return;
	}

	uint32_t pixel = (r >> format->Rloss) << format->Rshift
	| (g >> format->Gloss) << format->Gshift
	| (b >> format->Bloss) << format->Bshift
	| ((a >> format->Aloss) << format->Ashift & format->Amask);

	switch (format->Bpp) {
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

const Point& Sprite2D::Iterator::Position() const noexcept
{
	return imp->Position();
}

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

Color Sprite2D::GetPixel(const Point& p) const noexcept
{
	if (Region(0, 0, Frame.w, Frame.h).PointInside(p)) {
		IPixelIterator::Direction xdir = (renderFlags & BlitFlags::MIRRORX) ? IPixelIterator::Reverse : IPixelIterator::Forward;
		IPixelIterator::Direction ydir = (renderFlags & BlitFlags::MIRRORY) ? IPixelIterator::Reverse : IPixelIterator::Forward;
		// I simply dont want to bother writing const overloads for everything in Iterator
		// this is a private method so im being lazy and using const_cast, we clearly dont alter it
		Iterator it = Iterator(*const_cast<Sprite2D*>(this), xdir, ydir);
		it.Advance(p.y * Frame.w + p.x);
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

bool Sprite2D::ConvertFormatTo(const PixelFormat& newfmt) noexcept
{
	// we rely on the subclasses to handle most format conversions
	if (format.RLE && newfmt.RLE == false && newfmt.Bpp == 1) {
		void* rledata = pixels;
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
