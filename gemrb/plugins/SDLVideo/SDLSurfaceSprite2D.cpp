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

namespace GemRB {

SDLSurfaceSprite2D::SDLSurfaceSprite2D (int Width, int Height, int Bpp, void* pixels,
										Uint32 rmask, Uint32 gmask, Uint32 bmask, Uint32 amask)
	: Sprite2D(Width, Height, Bpp, pixels)
{
	if (pixels) {
		surface = SDL_CreateRGBSurfaceFrom( pixels, Width, Height, Bpp < 8 ? 8 : Bpp, Width * ( Bpp / 8 ),
										   rmask, gmask, bmask, amask );
	} else {
		surface = SDL_CreateRGBSurface(0, Width, Height, Bpp < 8 ? 8 : Bpp, rmask, gmask, bmask, amask);
		SDL_FillRect(surface, NULL, 0);
		pixels = surface->pixels;
	}
	palette = NULL;
}

SDLSurfaceSprite2D::SDLSurfaceSprite2D (int Width, int Height, int Bpp,
										Uint32 rmask, Uint32 gmask, Uint32 bmask, Uint32 amask)
: Sprite2D(Width, Height, Bpp, NULL)
{
	surface = SDL_CreateRGBSurface( Width, Height, Bpp < 8 ? 8 : Bpp, Width * ( Bpp / 8 ),
									   rmask, gmask, bmask, amask );
	SDL_FillRect(surface, NULL, 0);
	pixels = surface->pixels;
	palette = NULL;
}

SDLSurfaceSprite2D::SDLSurfaceSprite2D(const SDLSurfaceSprite2D &obj)
	: Sprite2D(obj)
{
	// SDL_ConvertSurface should copy colorkey/palette/pixels/surface RLE
	surface = SDL_ConvertSurface(obj.surface, obj.surface->format, obj.surface->flags);
	pixels = surface->pixels;
	palette = NULL;
}

SDLSurfaceSprite2D* SDLSurfaceSprite2D::copy() const
{
	return new SDLSurfaceSprite2D(*this);
}

SDLSurfaceSprite2D::~SDLSurfaceSprite2D()
{
	SDL_FreeSurface(surface);
}

const void* SDLSurfaceSprite2D::LockSprite() const
{
	SDL_LockSurface(surface);
	return surface->pixels;
}

void* SDLSurfaceSprite2D::LockSprite()
{
	SDL_LockSurface(surface);
	return surface->pixels;
}

void SDLSurfaceSprite2D::UnlockSprite() const
{
	SDL_UnlockSurface(surface);
}

/** Get the Palette of a Sprite */
Palette* SDLSurfaceSprite2D::GetPalette() const
{
	if (palette == NULL) {
		if (surface->format->BytesPerPixel != 1) {
			return NULL;
		}
		assert(surface->format->palette->ncolors <= 256);
		palette = new Palette();
		memcpy(palette->col, surface->format->palette->colors, surface->format->palette->ncolors * 4);
	}
	palette->acquire();
	return palette;
}

const Color* SDLSurfaceSprite2D::GetPaletteColors() const
{
	if (surface->format->BytesPerPixel != 1) {
		return NULL;
	}
	return reinterpret_cast<const Color*>(surface->format->palette->colors);
}

void SDLSurfaceSprite2D::SetPalette(Palette* pal)
{
	if (pal == palette)
		return;

	if (palette) {
		palette->release();
		palette = NULL;
	}

	if (pal && SetPalette(pal->col) == 0) {
		palette = pal;
		palette->acquire();
	}
}

int SDLSurfaceSprite2D::SetPalette(const Color* pal)
{
	return SDLVideoDriver::SetSurfacePalette(surface, reinterpret_cast<const SDL_Color*>(pal), 0x01 << Bpp);
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
	// SDL 2 will enforce SDL_RLEACCEL
	SDL_SetColorKey(surface, SDL_TRUE, ck);
	// don't RLE with SDL 2
	// this only benifits SDL_BlitSurface which we don't use. its a slowdown for us.
	//SDL_SetSurfaceRLE(surface, SDL_TRUE);
#else
	SDL_SetColorKey(surface, SDL_SRCCOLORKEY | SDL_RLEACCEL, ck);
#endif

	// regardless of rle or the success of SDL_SetSurfaceRLE
	// we must keep RLE false because SDL hides the actual RLE data from us (see SDL_BlitMap)
	// and we are left to access the pixels in decoded form (updated by SDL_UnlockSurface).
	// SDL Blits will make use of RLE acceleration, but our internal blitters cannot.
	assert(RLE == false);
}
	
bool SDLSurfaceSprite2D::HasTransparency() const
{
#if SDL_VERSION_ATLEAST(1,3,0)
	return SDL_ISPIXELFORMAT_ALPHA(surface->format->format) || SDL_GetColorKey(surface, NULL) != -1;
#else
	return surface->format->Amask > 0 || (surface->flags | SDL_SRCCOLORKEY);
#endif
}

Color SDLSurfaceSprite2D::GetPixel(unsigned short x, unsigned short y) const
{
	if (x >= Width || y >= Height) return Color();

	return SDLVideoDriver::GetSurfacePixel(surface, x, y);
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

#if SDL_VERSION_ATLEAST(1,3,0)
SDLTextureSprite2D::SDLTextureSprite2D(int Width, int Height, int Bpp, void* pixels,
									   ieDword rmask, ieDword gmask, ieDword bmask, ieDword amask)
: SDLSurfaceSprite2D(Width, Height, Bpp, pixels, rmask, gmask, bmask, amask)
{
	dirty = true;
	texture = NULL;
}

SDLTextureSprite2D::SDLTextureSprite2D(int Width, int Height, int Bpp,
									   ieDword rmask, ieDword gmask, ieDword bmask, ieDword amask)
: SDLSurfaceSprite2D(Width, Height, Bpp, rmask, gmask, bmask, amask)
{
	dirty = true;
	texture = NULL;
}

SDLTextureSprite2D::SDLTextureSprite2D(const SDLSurfaceSprite2D &obj)
: SDLSurfaceSprite2D(obj)
{
	dirty = true;
	texture = NULL;
}

SDLTextureSprite2D* SDLTextureSprite2D::copy() const
{
	return new SDLTextureSprite2D(*this);
}

SDLTextureSprite2D::~SDLTextureSprite2D()
{
	SDL_DestroyTexture(texture);
}

SDL_Texture* SDLTextureSprite2D::GetTexture(SDL_Renderer* renderer) const
{
	if (dirty) {
		SDL_DestroyTexture(texture);
		texture = SDL_CreateTextureFromSurface(renderer, GetSurface());
		assert(texture);
		dirty = false;
	}
	return texture;
}

void SDLTextureSprite2D::SetPalette(Palette *pal)
{
	if (pal == GetPalette())
		return;

	dirty = true;
	SDLSurfaceSprite2D::SetPalette(pal);
}

void SDLTextureSprite2D::SetColorKey(ieDword pxvalue)
{
	dirty = true;
	SDLSurfaceSprite2D::SetColorKey(pxvalue);
}
#endif

}
