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

#include "SDLSurfaceSprite2D.h"
#include "SDLVideo.h"

#include "System/Logging.h"

#include <SDL/SDL.h>

namespace GemRB {

SDLSurfaceSprite2D::SDLSurfaceSprite2D (int Width, int Height, int Bpp, void* pixels,
										Uint32 rmask, Uint32 gmask, Uint32 bmask, Uint32 amask)
	: Sprite2D(Width, Height, Bpp, pixels)
{
	surface = SDL_CreateRGBSurfaceFrom( pixels, Width, Height, Bpp < 8 ? 8 : Bpp, Width * ( Bpp / 8 ),
									   rmask, gmask, bmask, amask );
	SetSurfaceRLE(true);
	colorkeyIdx = 0;
}

SDLSurfaceSprite2D::SDLSurfaceSprite2D(const SDLSurfaceSprite2D &obj)
	: Sprite2D(obj)
{
	// SDL_ConvertSurface should copy colorkey/palette/pixels
	surface = SDL_ConvertSurface(obj.surface, obj.surface->format, obj.surface->flags);
	pixels = surface->pixels;
	colorkeyIdx = obj.colorkeyIdx;

	SetSurfaceRLE(obj.RLE);
}

SDLSurfaceSprite2D* SDLSurfaceSprite2D::copy() const
{
	return new SDLSurfaceSprite2D(*this);
}

SDLSurfaceSprite2D::~SDLSurfaceSprite2D()
{
	SDL_FreeSurface(surface);
}

/** Get the Palette of a Sprite */
Palette* SDLSurfaceSprite2D::GetPalette() const
{
	if (surface->format->BitsPerPixel != 8) {
		return NULL;
	}
	Palette* pal = new Palette();
	memcpy(pal->col, surface->format->palette->colors, surface->format->palette->ncolors * 4);
	return pal;
}

const Color* SDLSurfaceSprite2D::GetPaletteColors() const
{
	return reinterpret_cast<const Color*>(surface->format->palette->colors);
}

void SDLSurfaceSprite2D::SetPalette(Palette* pal)
{
	SetPalette(pal->col);
}

void SDLSurfaceSprite2D::SetPalette(Color* pal)
{
	SDLVideoDriver::SetSurfacePalette(surface, (SDL_Color*)pal, 0x01 << Bpp);
}

ieDword SDLSurfaceSprite2D::GetColorKey() const
{
	if (surface->format->palette) {
		return colorkeyIdx;
	}
	ieDword ck = 0;
#if SDL_VERSION_ATLEAST(1,3,0)
	SDL_GetColorKey(surface, &ck);
#else
	ck = surface->format->colorkey;
#endif
	return ck;
}

void SDLSurfaceSprite2D::SetColorKey(ieDword ck)
{
	if (surface->format->palette) {
		colorkeyIdx = ck;
		// convert the index to a pixel value
		const Color* cols = GetPaletteColors();
		assert(cols);
		Color c = cols[ck];
		ck = SDL_MapRGB(surface->format, c.r, c.g, c.b);
	}
#if SDL_VERSION_ATLEAST(1,3,0)
	// SDL 2 will enforce SDL_RLEACCEL
	SDL_SetColorKey(surface, SDL_TRUE, ck);
#else
	SDL_SetColorKey(surface, SDL_SRCCOLORKEY | SDL_RLEACCEL, ck);
#endif
}

Color SDLSurfaceSprite2D::GetPixel(unsigned short x, unsigned short y) const
{
	Color c = { 0, 0, 0, 0 };
	if (x >= Width || y >= Height) return c;

	SDLVideoDriver::GetSurfacePixel(surface, x, y, c);
	return c;
}

bool SDLSurfaceSprite2D::ConvertFormatTo(int bpp, ieDword rmask, ieDword gmask,
					 ieDword bmask, ieDword amask)
{
	if (bpp >= 8) {
#if SDL_VERSION_ATLEAST(1,3,0)
		Uint32 fmt = SDL_MasksToPixelFormatEnum(bpp, rmask, gmask, bmask, amask);
		if (fmt != SDL_PIXELFORMAT_UNKNOWN) {
			SDL_Surface* ns = SDL_ConvertSurfaceFormat( surface, fmt, 0);
#else
		SDL_Surface* tmp = SDL_CreateRGBSurface(SDL_SWSURFACE, Width, Height, bpp, rmask, gmask, bmask, amask);
		if (tmp) {
			SDL_Surface* ns = SDL_ConvertSurface( surface, tmp->format, 0);
			SDL_FreeSurface(tmp);
#endif
			if (ns) {
				SDL_FreeSurface(surface);
				if (freePixels) {
					free((void*)pixels);
				}
				freePixels = false;
				surface = ns;
				pixels = surface->pixels;
				Bpp = bpp;
				return true;
			} else {
				Log(MESSAGE, "SDLSurfaceSprite2D",
#if SDL_VERSION_ATLEAST(1,3,0)
					"Cannot convert sprite to format: %s\nError: %s", SDL_GetPixelFormatName(fmt),
#else
					"Cannot convert sprite to format: %s",
#endif
					SDL_GetError());
			}
		}
	}
	return false;
}

void SDLSurfaceSprite2D::SetSurfaceRLE(bool rle)
{
#if SDL_VERSION_ATLEAST(1,3,0)
	SDL_SetSurfaceRLE(surface, rle);
#else
	if (rle) {
		surface->flags |= SDL_RLEACCEL;
	} else {
		surface->flags &= ~SDL_RLEACCEL;
	}
#endif
	// regardless of rle or the success of SDL_SetSurfaceRLE
	// we must keep RLE false because SDL hides the actual RLE data from us (see SDL_BlitMap)
	// and we are left to access the pixels in decoded form (updated by SDL_UnlockSurface).
	// SDL Blits will make use of RLE acceleration, but our internal blitters cannot.
	assert(RLE == false);
}

}
