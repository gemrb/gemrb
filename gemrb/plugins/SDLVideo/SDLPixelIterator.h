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

#include "Pixels.h"

// FIXME: could handle 24bpp with a packed struct
#define ERROR_UNHANDLED_24BPP error("SDLVideo", "24bpp is not supported.")
#define ERROR_UNKNOWN_BPP error("SDLVideo", "Invalid bpp.")

namespace GemRB {

struct SDLPixelIterator : IPixelIterator
{
private:
	IPixelIterator* imp;

	void InitImp(void* pixel, int pitch, int bpp) {
		switch (bpp) {
			case 4:
				imp = new PixelIterator<Uint32>(static_cast<Uint32*>(pixel), xdir, ydir, Size(clip.w, clip.h), pitch);
				break;
			case 3:
				ERROR_UNHANDLED_24BPP;
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
	SDL_PixelFormat* format;
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
	: IPixelIterator(NULL, surf->pitch, x, y), imp(NULL), format(surf->format), clip(clip)
	{
		Uint8* pixels = static_cast<Uint8*>(surf->pixels);
		pixels = FindStart(pixels, surf->pitch, format->BytesPerPixel, clip, xdir, ydir);

		InitImp(pixels, surf->pitch, surf->format->BytesPerPixel);

		pixel = surf->pixels; // always here regardless of direction
		
		assert(SDL_MUSTLOCK(surf) == 0);
		
#if SDL_VERSION_ATLEAST(1,3,0)
		SDL_GetColorKey(surf, reinterpret_cast<Uint32*>(&colorKey));
#else
		if (surf->flags & SDL_SRCCOLORKEY) {
			colorKey = surf->format->colorkey;
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

	~SDLPixelIterator() {
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
		pixels = FindStart(pixels, beg.pitch, beg.format->BytesPerPixel, beg.clip, xdir, ydir);
		it.xdir = xdir; // hack for InitImp
		it.InitImp(pixels, beg.pitch, beg.format->BytesPerPixel);
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

	Uint8 Channel(Uint32 mask, Uint8 shift) const {
		switch (format->BytesPerPixel) {
			case 1:
				return static_cast<PixelIterator<Uint8>*>(imp)->Channel(mask, shift);
			case 2:
				return static_cast<PixelIterator<Uint16>*>(imp)->Channel(mask, shift);
			case 3:
				ERROR_UNHANDLED_24BPP;
			case 4:
				return static_cast<PixelIterator<Uint32>*>(imp)->Channel(mask, shift);
			default:
				ERROR_UNKNOWN_BPP;
		}
	}

	IPixelIterator* Clone() const {
		return new SDLPixelIterator(*this);
	}

	void Advance(int amt) {
		imp->Advance(amt);
	}
	
	Color ReadRGBA() const {
		Color c;
		ReadRGBA(c.r, c.g, c.b, c.a);
		return c;
	}

	void ReadRGBA(Uint8& r, Uint8& g, Uint8& b, Uint8& a) const {
		Uint32 pixel = 0;
		switch (format->BytesPerPixel) {
			case 4:
				pixel = *reinterpret_cast<Uint32*>(imp->pixel);
				break;
			case 3:
				ERROR_UNHANDLED_24BPP;
			case 2:
				pixel = *reinterpret_cast<Uint16*>(imp->pixel);
				break;
			case 1:
				pixel = *reinterpret_cast<Uint8*>(imp->pixel);
				r = format->palette->colors[pixel].r;
				g = format->palette->colors[pixel].g;
				b = format->palette->colors[pixel].b;

				if (colorKey != -1 && pixel == Uint32(colorKey)) {
					a = SDL_ALPHA_TRANSPARENT;
				} else {
#if SDL_VERSION_ATLEAST(1,3,0)
					a = format->palette->colors[pixel].a;
#else
					a = format->palette->colors[pixel].unused; // unused is alpha
#endif
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
		} else {
			a = SDL_ALPHA_OPAQUE;
		}
	}

	void WriteRGBA(Uint8 r, Uint8 g, Uint8 b, Uint8 a) {
		if (format->BytesPerPixel == 1) {
			Uint32 pixel = SDL_MapRGBA(format, r, g, b, a);
			*reinterpret_cast<Uint8*>(imp->pixel) = pixel;
			return;
		}

		Uint32 pixel = (r >> format->Rloss) << format->Rshift
		| (g >> format->Gloss) << format->Gshift
		| (b >> format->Bloss) << format->Bshift
		| ((a >> format->Aloss) << format->Ashift & format->Amask);

		switch (format->BytesPerPixel) {
			case 4:
				*reinterpret_cast<Uint32*>(imp->pixel) = pixel;
				break;
			case 3:
				ERROR_UNHANDLED_24BPP;
			case 2:
				*reinterpret_cast<Uint16*>(imp->pixel) = pixel;
				break;
			default:
				ERROR_UNKNOWN_BPP;
		}
	}
	
	const Point& Position() const {
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
static void BlitBlendedRect(SDL_Surface* src, SDL_Surface* dst,
							const SDL_Rect& srcrgn, const SDL_Rect& dstrgn,
							BLENDER blender, Uint32 flags, IAlphaIterator* maskIt)
{
	assert(src && dst);
	assert(srcrgn.h == dstrgn.h && srcrgn.w == dstrgn.w);

	SDLPixelIterator::Direction xdir = (flags&BLIT_MIRRORX) ? SDLPixelIterator::Reverse : SDLPixelIterator::Forward;
	SDLPixelIterator::Direction ydir = (flags&BLIT_MIRRORY) ? SDLPixelIterator::Reverse : SDLPixelIterator::Forward;

	SDLPixelIterator dstbeg(dst, SDLPixelIterator::Forward, SDLPixelIterator::Forward, dstrgn);
	SDLPixelIterator dstend = SDLPixelIterator::end(dstbeg);
	SDLPixelIterator srcbeg(src, xdir, ydir, srcrgn);

	if (maskIt) {
		Blit(srcbeg, dstbeg, dstend, *maskIt, blender);
	} else {
		StaticAlphaIterator alpha(0);
		Blit(srcbeg, dstbeg, dstend, alpha, blender);
	}
}

}

#endif // SDL_PIXEL_ITERATOR_H
