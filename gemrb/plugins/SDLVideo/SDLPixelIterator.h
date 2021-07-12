/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2018 The GemRB Project
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

#ifndef SDL_PIXEL_ITERATOR_H
#define SDL_PIXEL_ITERATOR_H

#include "Video/Pixels.h"

namespace GemRB {

inline PixelFormat PixelFormatForSurface(SDL_Surface* surf, PaletteHolder pal = nullptr)
{
	SDL_PixelFormat* fmt = surf->format;
	if (fmt->palette && pal == nullptr) {
		assert(fmt->palette->ncolors <= 256);
		const Color* begin = reinterpret_cast<const Color*>(fmt->palette->colors);
		const Color* end = begin + fmt->palette->ncolors;
		
		pal = MakeHolder<Palette>(begin, end);
	}
	return PixelFormat {
		fmt->Rloss, fmt->Gloss, fmt->Bloss, fmt->Aloss,
		fmt->Rshift, fmt->Gshift, fmt->Bshift, fmt->Ashift,
		fmt->Rmask, fmt->Gmask, fmt->Bmask, fmt->Amask,
		fmt->BytesPerPixel, fmt->BitsPerPixel,
#if SDL_VERSION_ATLEAST(1,3,0)
		[surf]() { Uint32 ck; SDL_GetColorKey(surf, &ck); return ck; }(),
		bool(SDL_HasColorKey(surf)),
#else
		fmt->colorkey,
		bool(surf->flags & SDL_SRCCOLORKEY),
#endif
		false, pal
	};
}

struct SDLPixelIterator : IPixelIterator
{
private:
	IPixelIterator* imp = nullptr;

	void InitImp(void* pixel, int pitch, int bpp) {
		delete imp;
		switch (bpp) {
			case 4:
				imp = new PixelIterator<Uint32>(static_cast<Uint32*>(pixel), xdir, ydir, Size(clip.w, clip.h), pitch);
				break;
			case 3:
				imp = new PixelIterator<Pixel24Bit>(static_cast<Pixel24Bit*>(pixel), xdir, ydir, Size(clip.w, clip.h), pitch);
				break;
			case 2:
				imp = new PixelIterator<Uint16>(static_cast<Uint16*>(pixel), xdir, ydir, Size(clip.w, clip.h), pitch);
				break;
			case 1:
				imp = new PixelIterator<Uint8>(static_cast<Uint8*>(pixel), xdir, ydir, Size(clip.w, clip.h), pitch);
				break;
			default:
				ERROR_UNKNOWN_BPP;
		}
	}

	inline static Uint8* FindStart(Uint8* pixels, int pitch, int bpp, const SDL_Rect& clip, Direction xdir, Direction ydir) {
		if (xdir == Reverse) {
			pixels += bpp * (clip.w-1);
		}
		if (ydir == Reverse) {
			pixels += pitch * (clip.h-1);
		}

		pixels += (clip.y * pitch) + (clip.x * bpp);
		return pixels;
	}

	inline static SDL_Rect SurfaceRect(SDL_Surface* surf) {
		SDL_Rect r {};
		r.w = surf->w;
		r.h = surf->h;
		return r;
	}

public:
	PixelFormat format;
	SDL_Rect clip;
	int colorKey = -1;

	SDLPixelIterator(SDL_Surface* surf)
	: SDLPixelIterator(surf, SurfaceRect(surf))
	{}

	SDLPixelIterator(SDL_Surface* surf, const SDL_Rect& clip)
	: SDLPixelIterator(surf, Forward, Forward, clip)
	{}
	
	SDLPixelIterator(SDL_Surface* surf, Direction x, Direction y)
	: SDLPixelIterator(surf, x, y, SurfaceRect(surf))
	{}

	SDLPixelIterator(SDL_Surface* surf, Direction x, Direction y, const SDL_Rect& clip)
	: IPixelIterator(NULL, surf->pitch, x, y), imp(NULL), format(PixelFormatForSurface(surf)), clip(clip)
	{
		Uint8* pixels = static_cast<Uint8*>(surf->pixels);
		pixels = FindStart(pixels, surf->pitch, format.Bpp, clip, xdir, ydir);

		InitImp(pixels, surf->pitch, format.Bpp);

		pixel = surf->pixels; // always here regardless of direction
		
		assert(SDL_MUSTLOCK(surf) == 0);
		
#if SDL_VERSION_ATLEAST(1,3,0)
		SDL_GetColorKey(surf, reinterpret_cast<Uint32*>(&colorKey));
#else
		if (surf->flags & SDL_SRCCOLORKEY) {
			colorKey = format.ColorKey;
		}
#endif
	}

	SDLPixelIterator(const SDLPixelIterator& orig)
	: IPixelIterator(orig)
	{
		clip = orig.clip;
		format = orig.format;
		imp = orig.imp->Clone();
		colorKey = orig.colorKey;
	}

	~SDLPixelIterator() override {
		delete imp;
	}

	static SDLPixelIterator end(const SDLPixelIterator& beg)
	{
		if (beg.clip.w == 0 || beg.clip.h == 0) {
			// already at the end
			return SDLPixelIterator(beg);
		}
		
		Direction xdir = (beg.xdir == Forward) ? Reverse : Forward;
		Direction ydir = (beg.ydir == Forward) ? Reverse : Forward;
		SDLPixelIterator it(beg);

		Uint8* pixels = static_cast<Uint8*>(it.pixel);
		pixels = FindStart(pixels, beg.pitch, beg.format.Bpp, beg.clip, xdir, ydir);
		it.xdir = xdir; // hack for InitImp
		it.InitImp(pixels, beg.pitch, beg.format.Bpp);
		it.xdir = beg.xdir; // reset for Advance
		it.imp->xdir = beg.xdir;

		// 'end' iterators are one past the end
		it.Advance(1);
		return it;
	}

	SDLPixelIterator& operator++() {
		imp->Advance(1);
		return *this;
	}

	bool operator!=(const SDLPixelIterator& rhs) const {
		return imp->pixel != rhs.imp->pixel;
	}

	Uint8& operator*() const {
		return *static_cast<Uint8*>(imp->pixel);
	}

	Uint8* operator->() const {
		return static_cast<Uint8*>(imp->pixel);
	}

	Uint8 Channel(Uint32 mask, Uint8 shift) const noexcept override {
		switch (format.Bpp) {
			case 1:
				return static_cast<PixelIterator<Uint8>*>(imp)->Channel(mask, shift);
			case 2:
				return static_cast<PixelIterator<Uint16>*>(imp)->Channel(mask, shift);
			case 3:
				return static_cast<PixelIterator<Pixel24Bit>*>(imp)->Channel(mask, shift);
			case 4:
				return static_cast<PixelIterator<Uint32>*>(imp)->Channel(mask, shift);
			default:
				ERROR_UNKNOWN_BPP;
		}
	}

	IPixelIterator* Clone() const noexcept override {
		return new SDLPixelIterator(*this);
	}

	void Advance(int amt) noexcept override {
		imp->Advance(amt);
	}
	
	Color ReadRGBA() const {
		Color c;
		ReadRGBA(c.r, c.g, c.b, c.a);
		return c;
	}

	void ReadRGBA(uint8_t& r, uint8_t& g, uint8_t& b, uint8_t& a) const noexcept
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
				pixel = *static_cast<uint8_t*>(imp->pixel);
				r = format.palette->col[pixel].r;
				g = format.palette->col[pixel].g;
				b = format.palette->col[pixel].b;

				if (format.HasColorKey && pixel == format.ColorKey) {
					a = 0;
				} else {
					a = format.palette->col[pixel].a;
				}
				return;
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
		if(format.Amask) {
			v = (pixel & format.Amask) >> format.Ashift;
			a = (v << format.Aloss) + (v >> (8 - (format.Aloss << 1)));
		} else if (format.HasColorKey && pixel == format.ColorKey) {
			a = 0;
		} else {
			a = 255;
		}
	}

	void WriteRGBA(uint8_t r, uint8_t g, uint8_t b, uint8_t a) noexcept
	{
		if (format.Bpp == 1) {
			assert(false);
			// FIXME: if we want to be able to write back to paletted sprites we need
			// to implement a way to map the color to the palette
			//uint32_t pixel = format.palette->FindColor(Color(r, g, b, a));
			//*static_cast<uint8_t*>(imp->pixel) = pixel;
			return;
		}

		uint32_t pixel = (r >> format.Rloss) << format.Rshift
		| (g >> format.Gloss) << format.Gshift
		| (b >> format.Bloss) << format.Bshift
		| ((a >> format.Aloss) << format.Ashift & format.Amask);

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
	
	const Point& Position() const noexcept override {
		return imp->Position();
	}
};


template<class BLENDER>
static void ColorFill(const Color& c,
				 SDLPixelIterator dst, SDLPixelIterator dstend,
				 const BLENDER& blender)
{
	for (; dst != dstend; ++dst) {
		Color dstc;
		dst.ReadRGBA(dstc.r, dstc.g, dstc.b, dstc.a);

		blender(c, dstc, 0);

		dst.WriteRGBA(dstc.r, dstc.g, dstc.b, dstc.a);
	}
}

template<class BLENDER>
static void Blit(SDLPixelIterator src,
				 SDLPixelIterator dst, SDLPixelIterator dstend,
				 IAlphaIterator& mask,
				 const BLENDER& blender)
{
	for (; dst != dstend; ++dst, ++src, ++mask) {
		Color srcc, dstc;
		src.ReadRGBA(srcc.r, srcc.g, srcc.b, srcc.a);
		dst.ReadRGBA(dstc.r, dstc.g, dstc.b, dstc.a);

		blender(srcc, dstc, *mask);

		dst.WriteRGBA(dstc.r, dstc.g, dstc.b, dstc.a);
	}
}

template <typename BLENDER>
static void BlitBlendedRect(SDLPixelIterator& src, SDLPixelIterator& dst,
							BLENDER blender, Uint32 flags, IAlphaIterator* maskIt)
{
	SDLPixelIterator dstend = SDLPixelIterator::end(dst);

	if (maskIt) {
		Blit(src, dst, dstend, *maskIt, blender);
	} else {
		StaticAlphaIterator alpha(0);
		Blit(src, dst, dstend, alpha, blender);
	}
}

}

#endif // SDL_PIXEL_ITERATOR_H
