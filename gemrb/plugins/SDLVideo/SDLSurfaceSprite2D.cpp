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

#include "SDLSurfaceDrawing.h"
#include "SDLVideo.h"

#include "System/Logging.h"

namespace GemRB {

SDLSurfaceSprite2D::SDLSurfaceSprite2D (const Region& rgn, int Bpp, void* pixels,
										Uint32 rmask, Uint32 gmask, Uint32 bmask, Uint32 amask)
	: Sprite2D(rgn, Bpp, pixels)
{
	if (pixels) {
		surface = new SurfaceHolder(SDL_CreateRGBSurfaceFrom( pixels, Frame.w, Frame.h, Bpp < 8 ? 8 : Bpp, Frame.w * ( Bpp / 8 ),
										   rmask, gmask, bmask, amask ));
	} else {
		surface = new SurfaceHolder(SDL_CreateRGBSurface(0, Frame.w, Frame.h, Bpp < 8 ? 8 : Bpp, rmask, gmask, bmask, amask));
		SDL_FillRect(*surface, NULL, 0);
	}

	assert(*surface);
	original = surface;
}

SDLSurfaceSprite2D::SDLSurfaceSprite2D (const Region& rgn, int Bpp,
										Uint32 rmask, Uint32 gmask, Uint32 bmask, Uint32 amask)
: Sprite2D(rgn, Bpp, NULL)
{
	surface = new SurfaceHolder(SDL_CreateRGBSurface( 0, Frame.w, Frame.h, Bpp < 8 ? 8 : Bpp,
													 rmask, gmask, bmask, amask ));
	SDL_FillRect(*surface, NULL, 0);

	assert(*surface);
	original = surface;
}

SDLSurfaceSprite2D::SDLSurfaceSprite2D(const SDLSurfaceSprite2D &obj)
	: Sprite2D(obj)
{
	SDL_PixelFormat* fmt = (*obj.surface)->format;
	pixels = (*obj.surface)->pixels;

	surface = new SurfaceHolder(
			SDL_CreateRGBSurfaceFrom(
				pixels,
				Frame.w,
				Frame.h,
				Bpp < 8 ? 8 : Bpp,
				Frame.w * ( Bpp / 8 ),
				fmt->Rmask,
				fmt->Gmask,
				fmt->Bmask,
				fmt->Amask
			)
		);

	if (Bpp == 8) {
		SDLVideoDriver::SetSurfacePalette(*surface, fmt->palette->colors);
		SetColorKey(obj.GetColorKey());
	}

	original = surface;
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
PaletteHolder SDLSurfaceSprite2D::GetPalette() const
{
	PaletteHolder palette = surface->palette;
	if (palette == NULL) {
		SDL_PixelFormat* fmt = (*surface)->format;
		if (fmt->BytesPerPixel != 1) {
			return NULL;
		}
		assert(fmt->palette->ncolors <= 256);
		const Color* begin = reinterpret_cast<const Color*>(fmt->palette->colors);
		const Color* end = begin + fmt->palette->ncolors;
		palette = new Palette(begin, end);
	}
	surface->palette = palette;

	return palette;
}

bool SDLSurfaceSprite2D::IsPaletteStale() const
{
	PaletteHolder pal = GetPalette();
	return pal && pal->GetVersion() != palVersion;
}

void SDLSurfaceSprite2D::SetPalette(PaletteHolder pal)
{
	PaletteHolder palette = surface->palette;

	// we don't use shared palettes because it is a performance bottleneck on SDL2
	assert(pal != palette);

	if (palette) {
		palette = nullptr;
	}

	if (version == 0) {
		original->palette = palette;
	} else {
		Restore();
	}

	ieDword ck = GetColorKey();
	if (pal && SetPalette(pal->col) == 0) {
		palette = pal->Copy();
		// must reset the color key or SDL 2 wont render properly
		SetColorKey(ck);
	}
	surface->palette = palette;
}

int SDLSurfaceSprite2D::SetPalette(const Color* pal) const
{
	return SDLVideoDriver::SetSurfacePalette(*surface, reinterpret_cast<const SDL_Color*>(pal), 0x01 << Bpp);
}

int32_t SDLSurfaceSprite2D::GetColorKey() const
{
#if SDL_VERSION_ATLEAST(1,3,0)
    Uint32 ck = -1;
	int ret = SDL_GetColorKey(*surface, &ck);
    if (ret == 0) {
        return ck;
    }
#else
    if ((*surface)->flags & SDL_SRCCOLORKEY) {
        return (*surface)->format->colorkey;
    }
#endif
	return -1;
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
}

bool SDLSurfaceSprite2D::HasTransparency() const
{
	SDL_PixelFormat* fmt = (*surface)->format;
#if SDL_VERSION_ATLEAST(1,3,0)
	return SDL_ISPIXELFORMAT_ALPHA(fmt->format) || SDL_GetColorKey(*surface, NULL) != -1;
#else
	return fmt->Amask > 0 || ((*surface)->flags & SDL_SRCCOLORKEY);
#endif
}

Color SDLSurfaceSprite2D::GetPixel(const Point& p) const
{
	Color c;

	if (p.x < 0 || p.x >= Frame.w) return c;
	if (p.y < 0 || p.y >= Frame.h) return c;

	SDL_Surface* surf = *surface;

	SDL_LockSurface( surf );
	Uint8 Bpp = surf->format->BytesPerPixel;
	unsigned char * pixels = ( ( unsigned char * ) surf->pixels ) +
	( ( p.y * surf->pitch + (p.x*Bpp)) );
	Uint32 val = 0;

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

	SDL_UnlockSurface( surf );
	SDL_GetRGBA( val, surf->format, (Uint8 *) &c.r, (Uint8 *) &c.g, (Uint8 *) &c.b, (Uint8 *) &c.a );

	// check color key... SDL_GetRGBA wont get this
#if SDL_VERSION_ATLEAST(1,3,0)
	Uint32 ck;
	if (SDL_GetColorKey(surf, &ck) != -1 && ck == val) c.a = SDL_ALPHA_TRANSPARENT;
#else
	if (surf->format->colorkey == val) c.a = SDL_ALPHA_TRANSPARENT;
#endif
	return c;
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
		SDL_Surface* tmp = SDL_CreateRGBSurface(SDL_SWSURFACE, Frame.w, Frame.h, bpp, rmask, gmask, bmask, amask);
		if (tmp) {
			SDL_Surface* ns = SDL_ConvertSurface( *surface, tmp->format, 0);
			SDL_FreeSurface(tmp);
#endif
			if (ns) {
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
	if (Bpp == 8 && original->palette) {
		SetPalette(original->palette->col);
	}
}

void* SDLSurfaceSprite2D::NewVersion(version_t newversion) const
{
	if (newversion == 0 || version != newversion) {
		Restore();
		version = newversion;
	}

	if (Bpp == 8) {
		PaletteHolder pal = GetPalette(); // generate the backup

		palVersion = pal->GetVersion();

		// we just allow overwritting the palette
		return surface->surface->format->palette;
	}

	palVersion = 0;

	if (version != newversion) {
		SDL_Surface* newVersion = SDL_ConvertSurface(*original, (*original)->format, 0);
		surface = new SurfaceHolder(newVersion);

		return newVersion;
	} else {
		return surface->surface;
	}
}

#if SDL_VERSION_ATLEAST(1,3,0)
SDLTextureSprite2D::SDLTextureSprite2D(const Region& rgn, int Bpp, void* pixels,
									   ieDword rmask, ieDword gmask, ieDword bmask, ieDword amask)
: SDLSurfaceSprite2D(rgn, Bpp, pixels, rmask, gmask, bmask, amask)
{
	texture = NULL;
}

SDLTextureSprite2D::SDLTextureSprite2D(const Region& rgn, int Bpp,
									   ieDword rmask, ieDword gmask, ieDword bmask, ieDword amask)
: SDLSurfaceSprite2D(rgn, Bpp, rmask, gmask, bmask, amask)
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

void* SDLTextureSprite2D::NewVersion(version_t version) const
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
