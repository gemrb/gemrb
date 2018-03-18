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
		surface = new SurfaceHolder(SDL_CreateRGBSurfaceFrom( pixels, Width, Height, Bpp < 8 ? 8 : Bpp, Width * ( Bpp / 8 ),
										   rmask, gmask, bmask, amask ));
	} else {
		surface = new SurfaceHolder(SDL_CreateRGBSurface(0, Width, Height, Bpp < 8 ? 8 : Bpp, rmask, gmask, bmask, amask));
		SDL_FillRect(*surface, NULL, 0);
	}

	original = surface;
	version = 0;
}

SDLSurfaceSprite2D::SDLSurfaceSprite2D (int Width, int Height, int Bpp,
										Uint32 rmask, Uint32 gmask, Uint32 bmask, Uint32 amask)
: Sprite2D(Width, Height, Bpp, NULL)
{
	surface = new SurfaceHolder(SDL_CreateRGBSurface( Width, Height, Bpp < 8 ? 8 : Bpp, Width * ( Bpp / 8 ),
									   rmask, gmask, bmask, amask ));
	SDL_FillRect(*surface, NULL, 0);
	original = surface;
	version = 0;
}

SDLSurfaceSprite2D::SDLSurfaceSprite2D(const SDLSurfaceSprite2D &obj)
	: Sprite2D(obj)
{
	// SDL_ConvertSurface should copy colorkey/palette/pixels/surface RLE
	// FIXME: our software renderer assumes the pitch is the width*BPP
	// since our sprite class has no Pitch member we have to manually copy the pixels from
	// obj to our own pixel buffer so the pitch will be as expected
	//surface = SDL_ConvertSurface(obj.surface, obj.surface->format, obj.surface->flags);
	/*
	SDL_PixelFormat* fmt = obj.surface->format;
	size_t numPx = obj.Width * obj.Height * fmt->BytesPerPixel;
	pixels = malloc(numPx);
	memcpy(pixels, obj.pixels, numPx);
	surface = SDL_CreateRGBSurfaceFrom( pixels, Width, Height, Bpp < 8 ? 8 : Bpp, Width * ( Bpp / 8 ),
									   fmt->Rmask, fmt->Gmask, fmt->Bmask, fmt->Amask );
	memcpy(surface->format->palette->colors, obj.surface->format->palette->colors, surface->format->palette->ncolors);
	freePixels = true;
	pixels = surface->pixels;
	palette = NULL;
	surface = obj.surface;
	SetColorKey(obj.GetColorKey());
	 */

	// all copies now share underlying. AFIK we don't have actual need for modifying pixels,
	// but just in case something comes up the code above can be used in a pinch

	// a copy does not retain the original
	// it is as if it is a new sprite of version 0
	surface = obj.surface;
	original = surface;
	version = 0;
}

SDLSurfaceSprite2D* SDLSurfaceSprite2D::copy() const
{
	return new SDLSurfaceSprite2D(*this);
}

const void* SDLSurfaceSprite2D::LockSprite() const
{
	SDL_LockSurface(*surface);
	return (*surface)->pixels;
}

void* SDLSurfaceSprite2D::LockSprite()
{
	SDL_LockSurface(*surface);
	return (*surface)->pixels;
}

void SDLSurfaceSprite2D::UnlockSprite() const
{
	SDL_UnlockSurface(*surface);
}

/** Get the Palette of a Sprite */
Palette* SDLSurfaceSprite2D::GetPalette() const
{
	Palette* palette = surface->palette;
	if (palette == NULL) {
		SDL_PixelFormat* fmt = (*surface)->format;
		if (fmt->BytesPerPixel != 1) {
			return NULL;
		}
		assert(fmt->palette->ncolors <= 256);
		palette = new Palette();
		memcpy(palette->col, fmt->palette->colors, fmt->palette->ncolors * 4);
	}
	palette->acquire();
	surface->palette = palette;
	return palette;
}

const Color* SDLSurfaceSprite2D::GetPaletteColors() const
{
	SDL_PixelFormat* fmt = (*surface)->format;
	if (fmt->BytesPerPixel != 1) {
		return NULL;
	}
	return reinterpret_cast<const Color*>(fmt->palette->colors);
}

void SDLSurfaceSprite2D::SetPalette(Palette* pal)
{
	Palette* palette = surface->palette;
	if (pal == palette)
		return;

	if (palette) {
		palette->release();
		palette = NULL;
	}

	Restore();

	ieDword ck = GetColorKey();
	if (pal && SetPalette(pal->col) == 0) {
		palette = pal;
		palette->acquire();

		// must reset the color key or SDL 2 wont render properly
		SetColorKey(ck);
	}
	surface->palette = palette;
}

int SDLSurfaceSprite2D::SetPalette(const Color* pal) const
{
	return SDLVideoDriver::SetSurfacePalette(*surface, reinterpret_cast<const SDL_Color*>(pal), 0x01 << Bpp);
}

ieDword SDLSurfaceSprite2D::GetColorKey() const
{
	ieDword ck = 0;
#if SDL_VERSION_ATLEAST(1,3,0)
	int ret = SDL_GetColorKey(*surface, &ck);
	assert(ret == 0);
#else
	ck = (*surface)->format->colorkey;
#endif
	return ck;
}

void SDLSurfaceSprite2D::SetColorKey(ieDword ck)
{
#if SDL_VERSION_ATLEAST(1,3,0)
	SDL_SetColorKey(*surface, SDL_TRUE, ck);
	// don't RLE with SDL 2
	// this only benifits SDL_BlitSurface which we don't use. its a slowdown for us.
	//SDL_SetSurfaceRLE(surface, SDL_TRUE);
#else
	SDL_SetColorKey(*surface, SDL_SRCCOLORKEY | SDL_RLEACCEL, ck);
#endif

	// regardless of rle or the success of SDL_SetSurfaceRLE
	// we must keep RLE false because SDL hides the actual RLE data from us (see SDL_BlitMap)
	// and we are left to access the pixels in decoded form (updated by SDL_UnlockSurface).
	// SDL Blits will make use of RLE acceleration, but our internal blitters cannot.
	assert(RLE == false);
}
	
bool SDLSurfaceSprite2D::HasTransparency() const
{
	SDL_PixelFormat* fmt = (*surface)->format;
#if SDL_VERSION_ATLEAST(1,3,0)
	return SDL_ISPIXELFORMAT_ALPHA(fmt->format) || SDL_GetColorKey(*surface, NULL) != -1;
#else
	return fmt->Amask > 0 || ((*surface)->flags | SDL_SRCCOLORKEY);
#endif
}

Color SDLSurfaceSprite2D::GetPixel(unsigned short x, unsigned short y) const
{
	if (x >= Width || y >= Height) return Color();

	return SDLVideoDriver::GetSurfacePixel(*surface, x, y);
}

bool SDLSurfaceSprite2D::ConvertFormatTo(int bpp, ieDword rmask, ieDword gmask,
					 ieDword bmask, ieDword amask)
{
	if (bpp >= 8) {
#if SDL_VERSION_ATLEAST(1,3,0)
		Uint32 fmt = SDL_MasksToPixelFormatEnum(bpp, rmask, gmask, bmask, amask);
		if (fmt != SDL_PIXELFORMAT_UNKNOWN) {
			SDL_Surface* ns = SDL_ConvertSurfaceFormat( *surface, fmt, 0);
#else
		SDL_Surface* tmp = SDL_CreateRGBSurface(SDL_SWSURFACE, Width, Height, bpp, rmask, gmask, bmask, amask);
		if (tmp) {
			SDL_Surface* ns = SDL_ConvertSurface( *surface, tmp->format, 0);
			SDL_FreeSurface(tmp);
#endif
			if (ns) {
				SDL_FreeSurface(*surface);
				if (freePixels) {
					free((void*)pixels);
				}
				freePixels = false;
				surface = new SurfaceHolder(ns);
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

void SDLSurfaceSprite2D::Restore() const
{
	version = 0;
	surface = original;
	if (Bpp == 8) {
		SetPalette(surface->palette->col);
	}
}

void* SDLSurfaceSprite2D::NewVersion(unsigned int newversion) const
{
	version = newversion;

	if (version == 0) {
		Restore();
	}

	if (Bpp == 8) {
		GetPalette(); // generate the backup
		// we just allow overwritting the palette
		return surface->surface->format->palette;
	} else if (version != newversion) {
		SDL_Surface* newVersion = SDL_ConvertSurface(*original, (*original)->format, 0);
		surface = new SurfaceHolder(newVersion);

		return newVersion;
	} else {
		return surface->surface;
	}
}

#if SDL_VERSION_ATLEAST(1,3,0)
SDLTextureSprite2D::SDLTextureSprite2D(int Width, int Height, int Bpp, void* pixels,
									   ieDword rmask, ieDword gmask, ieDword bmask, ieDword amask)
: SDLSurfaceSprite2D(Width, Height, Bpp, pixels, rmask, gmask, bmask, amask)
{
	texture = NULL;
}

SDLTextureSprite2D::SDLTextureSprite2D(int Width, int Height, int Bpp,
									   ieDword rmask, ieDword gmask, ieDword bmask, ieDword amask)
: SDLSurfaceSprite2D(Width, Height, Bpp, rmask, gmask, bmask, amask)
{
	texture = NULL;
}

SDLTextureSprite2D::SDLTextureSprite2D(const SDLTextureSprite2D& obj)
: SDLSurfaceSprite2D(obj)
{
	texture = obj.texture;
}

SDLTextureSprite2D* SDLTextureSprite2D::copy() const
{
	return new SDLTextureSprite2D(*this);
}

SDL_Texture* SDLTextureSprite2D::GetTexture(SDL_Renderer* renderer) const
{
	if (texture == NULL) {
		texture = new TextureHolder(SDL_CreateTextureFromSurface(renderer, GetSurface()));
	}
	return *texture;
}

void SDLTextureSprite2D::SetColorKey(ieDword pxvalue)
{
	texture = NULL;
	SDLSurfaceSprite2D::SetColorKey(pxvalue);
}

void* SDLTextureSprite2D::NewVersion(unsigned int version) const
{
	texture = NULL;
	return SDLSurfaceSprite2D::NewVersion(version);
}

void SDLTextureSprite2D::Restore() const
{
	texture = NULL;
	SDLSurfaceSprite2D::Restore();
}
#endif

}
