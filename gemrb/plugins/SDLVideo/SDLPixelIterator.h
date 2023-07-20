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

using SDLPixelIterator = PixelFormatIterator;

inline PixelFormat PixelFormatForSurface(SDL_Surface* surf, Holder<Palette> pal = nullptr)
{
	const SDL_PixelFormat* fmt = surf->format;
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

inline Region SurfaceRect(const SDL_Surface* surf) {
	return Region(0, 0, surf->w, surf->h);
}

struct SDLPixelIteratorWrapper {
	// PixelFormatIterator keeps a reference to the PixelFormat to avoid copies
	// therefore when we call MakeSDLPixelIterator we need to have the PixelFormat returned as well
	PixelFormat format;
	SDLPixelIterator it;
	
	SDLPixelIteratorWrapper(SDL_Surface* surf, IPixelIterator::Direction x, IPixelIterator::Direction y, const Region& clip)
	: format(PixelFormatForSurface(surf)), it(surf->pixels, surf->pitch, format, x, y, clip)
	{}
	
	operator SDLPixelIterator&() {
		return it;
	}
};

inline SDLPixelIteratorWrapper MakeSDLPixelIterator(SDL_Surface* surf, IPixelIterator::Direction x, IPixelIterator::Direction y, const Region& clip)
{
	return SDLPixelIteratorWrapper(surf, x, y, clip);
}

inline SDLPixelIteratorWrapper MakeSDLPixelIterator(SDL_Surface* surf, IPixelIterator::Direction x, IPixelIterator::Direction y)
{
	return SDLPixelIteratorWrapper(surf, x, y, SurfaceRect(surf));
}

inline SDLPixelIteratorWrapper MakeSDLPixelIterator(SDL_Surface* surf, const Region& clip)
{
	return SDLPixelIteratorWrapper(surf, IPixelIterator::Direction::Forward, IPixelIterator::Direction::Forward, clip);
}

template<class BLENDER>
static void ColorFill(const Color& c,
				 SDLPixelIterator dst, const SDLPixelIterator& dstend,
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
				 SDLPixelIterator dst, const SDLPixelIterator& dstend,
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
							BLENDER blender, IAlphaIterator* maskIt)
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
