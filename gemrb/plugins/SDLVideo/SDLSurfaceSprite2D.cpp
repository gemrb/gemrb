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

#include "System/Logging.h"

#include <SDL/SDL.h>

namespace GemRB {

SDLSurfaceSprite2D::SDLSurfaceSprite2D(int Width, int Height, int Bpp, void* pixels)
	: Sprite2D(Width, Height, Bpp, NULL, pixels)
{
	surface = SDL_CreateRGBSurfaceFrom( pixels, Width, Height, Bpp < 8 ? 8 : Bpp, Width * ( Bpp / 8 ),
									   0, 0, 0, 0 );
	vptr = surface;
	freePixels = true;
#if SDL_VERSION_ATLEAST(1,3,0)
	SDL_SetSurfaceRLE(surface, SDL_TRUE);
#endif
}

SDLSurfaceSprite2D::SDLSurfaceSprite2D (int Width, int Height, int Bpp, void* pixels,
										Uint32 rmask, Uint32 gmask, Uint32 bmask, Uint32 amask)
	: Sprite2D(Width, Height, Bpp, NULL, pixels)
{
	surface = SDL_CreateRGBSurfaceFrom( pixels, Width, Height, Bpp < 8 ? 8 : Bpp, Width * ( Bpp / 8 ),
									   rmask, gmask, bmask, amask );
	vptr = surface;
	freePixels = true;
#if SDL_VERSION_ATLEAST(1,3,0)
	SDL_SetSurfaceRLE(surface, SDL_TRUE);
#endif
}

SDLSurfaceSprite2D::SDLSurfaceSprite2D(const SDLSurfaceSprite2D &obj)
	: Sprite2D(obj)
{
	surface = SDL_ConvertSurface(obj.surface, obj.surface->format, obj.surface->flags);
	pixels = surface->pixels;
	freePixels = false;
	vptr = surface;
#if SDL_VERSION_ATLEAST(1,3,0)
	SDL_SetSurfaceRLE(surface, SDL_TRUE);
#endif
}

SDLSurfaceSprite2D* SDLSurfaceSprite2D::copy() const
{
	return new SDLSurfaceSprite2D(*this);
}

SDLSurfaceSprite2D::~SDLSurfaceSprite2D()
{
	SDL_FreeSurface(surface);
	if (freePixels) {
		// FIXME: casting away const.
		free((void*)pixels);
	}
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
	SDLSurfaceSprite2D::SetSurfacePalette(surface, (SDL_Color*)pal, 0x01 << Bpp);
}

ieDword SDLSurfaceSprite2D::GetColorKey() const
{
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
#if SDL_VERSION_ATLEAST(1,3,0)
	SDL_SetColorKey(surface, SDL_TRUE, ck);
#else
	SDL_SetColorKey(surface, SDL_SRCCOLORKEY | SDL_RLEACCEL, ck);
#endif
}

Color SDLSurfaceSprite2D::GetPixel(unsigned short x, unsigned short y) const
{
	Color c = { 0, 0, 0, 0 };

	if (x >= Width || y >= Height) return c;

	SDL_LockSurface( surface );
	Uint8 Bpp = surface->format->BytesPerPixel;
	unsigned char * pixels = ( ( unsigned char * ) surface->pixels ) +
	( ( y * surface->w + x) * Bpp );
	long val = 0;

	if (Bpp == 1) {
		val = *pixels;
	} else if (Bpp == 2) {
		val = *(Uint16 *)pixels;
	} else if (Bpp == 3) {
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
		val = pixels[0] + ((unsigned int)pixels[1] << 8) + ((unsigned int)pixels[2] << 16);
#else
		val = pixels[2] + ((unsigned int)pixels[1] << 8) + ((unsigned int)pixels[0] << 16);
#endif
	} else if (Bpp == 4) {
		val = *(Uint32 *)pixels;
	}

	SDL_UnlockSurface( surface );

	SDL_GetRGBA( val, surface->format, (Uint8 *) &c.r, (Uint8 *) &c.g, (Uint8 *) &c.b, (Uint8 *) &c.a );
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

void SDLSurfaceSprite2D::SetSurfacePalette(SDL_Surface* surf, SDL_Color* pal, int numcolors)
{
#if SDL_VERSION_ATLEAST(1,3,0)
	SDL_SetPaletteColors( surf->format->palette, pal, 0, numcolors );
#else
	SDL_SetPalette( surf, SDL_LOGPAL | SDL_RLEACCEL, pal, 0, numcolors );
#endif
}

}
