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

#include "SDLPixelIterator.h"
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

	original = surface;
	if (Bpp <= 8) {
		SetPaletteColors(reinterpret_cast<const Color*>(fmt->palette->colors));
	}
	SetColorKey(obj.GetColorKey());
}

void SDLSurfaceSprite2D::SetPaletteFromSurface() const noexcept
{
	SDL_PixelFormat* fmt = (*surface)->format;
	if (fmt->BytesPerPixel != 1) {
		return;
	}
	assert(fmt->palette->ncolors <= 256);
	const Color* begin = reinterpret_cast<const Color*>(fmt->palette->colors);
	const Color* end = begin + fmt->palette->ncolors;
	
	if (surface->palette == nullptr) {
		surface->palette = new Palette(begin, end);
	} else {
		surface->palette->CopyColorRange(begin, end, 0);
	}
}

int SDLSurfaceSprite2D::SetPaletteColors(const Color* pal) const noexcept
{
	assert(Bpp <= 8);
	int ret = SDLVideoDriver::SetSurfacePalette(*surface, reinterpret_cast<const SDL_Color*>(pal), 0x01 << Bpp);
	if (ret == 0) {
		SetPaletteFromSurface();
	}
	return ret;
}

Holder<Sprite2D> SDLSurfaceSprite2D::copy() const
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

bool SDLSurfaceSprite2D::IsPaletteStale() const
{
	PaletteHolder pal = GetPalette();
	return pal && pal->GetVersion() != palVersion;
}

void SDLSurfaceSprite2D::SetPalette(PaletteHolder pal)
{
	// we don't use shared palettes because it is a performance bottleneck on SDL2
	assert(pal != surface->palette);

	if (version == 0) {
		original->palette = nullptr;
	} else {
		Restore();
	}

	ieDword ck = GetColorKey();
	if (pal && SetPaletteColors(pal->col) == 0) {
		surface->palette = pal->Copy();
		// must reset the color key or SDL 2 wont render properly
		SetColorKey(ck);
	} else {
		SetPaletteFromSurface();
	}
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
	if (Region(0, 0, Frame.w, Frame.h).PointInside(p)) {
		IPixelIterator::Direction xdir = (renderFlags & BlitFlags::MIRRORX) ? IPixelIterator::Reverse : IPixelIterator::Forward;
		IPixelIterator::Direction ydir = (renderFlags & BlitFlags::MIRRORY) ? IPixelIterator::Reverse : IPixelIterator::Forward;
		SDLPixelIterator it(*surface, xdir, ydir);
		it.Advance(p.y * Frame.w + p.x);
		return it.ReadRGBA();
	}
	return Color();
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
					free(pixels);
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
		SetPaletteColors(original->palette->col);
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
}

SDLTextureSprite2D::SDLTextureSprite2D(const Region& rgn, int Bpp,
									   ieDword rmask, ieDword gmask, ieDword bmask, ieDword amask)
: SDLSurfaceSprite2D(rgn, Bpp, rmask, gmask, bmask, amask)
{
}

Holder<Sprite2D> SDLTextureSprite2D::copy() const
{
	return new SDLTextureSprite2D(*this);
}

SDL_Texture* SDLTextureSprite2D::GetTexture(SDL_Renderer* renderer) const
{
	if (texture == nullptr) {
		SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, GetSurface());
		SDL_QueryTexture(tex, &texFormat, nullptr, nullptr, nullptr);
		texture = new TextureHolder(tex);
	} else if (staleTexture) {
		SDL_Surface *surface = GetSurface();
		if (texFormat == surface->format->format) {
			SDL_UpdateTexture(*texture, nullptr, surface->pixels, surface->pitch);
		} else {
			/* Set up a destination surface for the texture update */
			SDL_PixelFormat *dst_fmt = SDL_AllocFormat(texFormat);
			assert(dst_fmt);
			
			SDL_Surface *temp = SDL_ConvertSurface(surface, dst_fmt, 0);
			SDL_FreeFormat(dst_fmt);
			assert(temp);
			SDL_UpdateTexture(*texture, nullptr, temp->pixels, temp->pitch);
			SDL_FreeSurface(temp);
		}
		staleTexture = false;
	}
	return *texture;
}
	
void SDLTextureSprite2D::UnlockSprite() const
{
	SDLSurfaceSprite2D::UnlockSprite();
	staleTexture = true;
}

void SDLTextureSprite2D::SetColorKey(ieDword pxvalue)
{
	staleTexture = true;
	SDLSurfaceSprite2D::SetColorKey(pxvalue);
}

void* SDLTextureSprite2D::NewVersion(version_t version) const
{
	staleTexture = true;
	return SDLSurfaceSprite2D::NewVersion(version);
}

void SDLTextureSprite2D::Restore() const
{
	staleTexture = true;
	SDLSurfaceSprite2D::Restore();
}
#endif

}
