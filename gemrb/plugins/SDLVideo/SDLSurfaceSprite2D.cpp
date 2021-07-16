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

SDLSurfaceSprite2D::SDLSurfaceSprite2D (const Region& rgn, void* px, const PixelFormat& fmt) noexcept
: Sprite2D(rgn, px, fmt)
{
	if (px) {
		surface = MakeHolder<SurfaceHolder>(SDL_CreateRGBSurfaceFrom(px, Frame.w, Frame.h, fmt.Depth, Frame.w * fmt.Bpp,
															 fmt.Rmask, fmt.Gmask, fmt.Bmask, fmt.Amask));
	} else {
		surface = MakeHolder<SurfaceHolder>(SDL_CreateRGBSurface(0, Frame.w, Frame.h, fmt.Depth, fmt.Rmask, fmt.Gmask, fmt.Bmask, fmt.Amask));
		SDL_FillRect(*surface, NULL, 0);
		pixels = (*surface)->pixels;
	}

	assert(*surface);
	pitch = (*surface)->pitch;
	
	if (format.palette)
		SetPaletteColors(format.palette->col);
	UpdateColorKey(format.ColorKey);
	
	format = PixelFormatForSurface(*surface, format.palette);
	
	original = surface;
}

SDLSurfaceSprite2D::SDLSurfaceSprite2D (const Region& rgn, const PixelFormat& fmt) noexcept
: SDLSurfaceSprite2D(rgn, nullptr, fmt)
{}

SDLSurfaceSprite2D::SDLSurfaceSprite2D(const SDLSurfaceSprite2D &obj) noexcept
: Sprite2D(obj)
{
	pixels = (*obj.surface)->pixels;

	surface = MakeHolder<SurfaceHolder>(
			SDL_CreateRGBSurfaceFrom(
				pixels,
				Frame.w,
				Frame.h,
				format.Depth,
				Frame.w * format.Bpp,
				format.Rmask,
				format.Gmask,
				format.Bmask,
				format.Amask
			)
		);
	
	if (format.palette) {
		SetPaletteColors(format.palette->col);
	}
	UpdateColorKey(format.ColorKey);
	
	format = PixelFormatForSurface(*surface, obj.format.palette);
	if (pixels == nullptr) {
		pixels = (*surface)->pixels;
	}
	
	original = surface;
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
		surface->palette = MakeHolder<Palette>(begin, end);
	} else {
		surface->palette->CopyColorRange(begin, end, 0);
	}
}

bool SDLSurfaceSprite2D::SetPaletteColors(const Color* pal) const noexcept
{
	assert(format.Depth <= 8);
	bool ret = SDLVideoDriver::SetSurfacePalette(*surface, reinterpret_cast<const SDL_Color*>(pal), 0x01 << format.Depth);
	if (ret) {
		SetPaletteFromSurface();
#if SDL_VERSION_ATLEAST(1,3,0)
		// must reset the color key or SDL 2 wont render properly
		SDL_SetColorKey(*surface, SDL_TRUE, GetColorKey());
#endif
	}
	return ret;
}

Holder<Sprite2D> SDLSurfaceSprite2D::copy() const
{
	return Holder<Sprite2D>(new SDLSurfaceSprite2D(*this));
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

void SDLSurfaceSprite2D::UpdatePalette(PaletteHolder pal) noexcept
{
	// we don't use shared palettes because it is a performance bottleneck on SDL2
	assert(pal != surface->palette);

	if (version == 0) {
		original->palette = nullptr;
	} else {
		Restore();
	}

	if (pal && SetPaletteColors(pal->col) == 0) {
		surface->palette = pal;
	} else {
		SetPaletteFromSurface();
	}
}

void SDLSurfaceSprite2D::UpdateColorKey(colorkey_t ck) noexcept
{
#if SDL_VERSION_ATLEAST(1,3,0)
	int ret = SDL_SetColorKey(*surface, SDL_bool(format.HasColorKey), ck);
	// don't RLE with SDL 2
	// this only benifits SDL_BlitSurface which we don't use. its a slowdown for us.
#else
	Uint32 flag = format.HasColorKey ? SDL_SRCCOLORKEY : 0;
	int ret = SDL_SetColorKey(*surface, flag | SDL_RLEACCEL, ck);
#endif
	assert(ret == 0);
}

bool SDLSurfaceSprite2D::HasTransparency() const noexcept
{
	SDL_PixelFormat* fmt = (*surface)->format;
#if SDL_VERSION_ATLEAST(1,3,0)
	return SDL_ISPIXELFORMAT_ALPHA(fmt->format) || SDL_GetColorKey(*surface, NULL) != -1;
#else
	return fmt->Amask > 0 || ((*surface)->flags & SDL_SRCCOLORKEY);
#endif
}

bool SDLSurfaceSprite2D::ConvertFormatTo(const PixelFormat& tofmt) noexcept
{
	if (tofmt.Bpp >= 1) {
#if SDL_VERSION_ATLEAST(1,3,0)
		Uint32 fmt = SDL_MasksToPixelFormatEnum(tofmt.Bpp * 8, tofmt.Rmask, tofmt.Gmask, tofmt.Bmask, tofmt.Amask);
		if (fmt != SDL_PIXELFORMAT_UNKNOWN) {
			SDL_Surface* ns = SDL_ConvertSurfaceFormat( *surface, fmt, 0);
#else
		SDL_Surface* tmp = SDL_CreateRGBSurface(SDL_SWSURFACE, Frame.w, Frame.h, tofmt.Depth, tofmt.Rmask, tofmt.Gmask, tofmt.Bmask, tofmt.Amask);
		if (tmp) {
			SDL_Surface* ns = SDL_ConvertSurface( *surface, tmp->format, 0);
			SDL_FreeSurface(tmp);
#endif
			if (ns) {
				if (freePixels) {
					free(pixels);
				}
				freePixels = false;
				surface = MakeHolder<SurfaceHolder>(ns);
				format.Bpp = tofmt.Bpp;
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
	if (format.Bpp == 1 && original->palette) {
		SetPaletteColors(original->palette->col);
	}
}

void* SDLSurfaceSprite2D::NewVersion(version_t newversion) const
{
	if (newversion == 0 || version != newversion) {
		Restore();
		version = newversion;
	}

	if (format.Bpp == 1) {
		PaletteHolder pal = GetPalette(); // generate the backup

		palVersion = pal->GetVersion();

		// we just allow overwritting the palette
		return surface->surface->format->palette;
	}

	palVersion = 0;

	if (version != newversion) {
		SDL_Surface* newVersion = SDL_ConvertSurface(*original, (*original)->format, 0);
		surface = MakeHolder<SurfaceHolder>(newVersion);

		return newVersion;
	} else {
		return surface->surface;
	}
}

#if SDL_VERSION_ATLEAST(1,3,0)
SDLTextureSprite2D::SDLTextureSprite2D(const Region& rgn, void* pixels, const PixelFormat& fmt) noexcept
: SDLSurfaceSprite2D(rgn, pixels, fmt)
{}

SDLTextureSprite2D::SDLTextureSprite2D(const Region& rgn, const PixelFormat& fmt) noexcept
: SDLSurfaceSprite2D(rgn, fmt)
{}

Holder<Sprite2D> SDLTextureSprite2D::copy() const
{
	return Holder<Sprite2D>(new SDLTextureSprite2D(*this));
}

SDL_Texture* SDLTextureSprite2D::GetTexture(SDL_Renderer* renderer) const
{
	if (texture == nullptr) {
		SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, GetSurface());
		SDL_QueryTexture(tex, &texFormat, nullptr, nullptr, nullptr);
		texture = MakeHolder<TextureHolder>(tex);
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

void SDLTextureSprite2D::UpdatePalette(PaletteHolder pal) noexcept
{
	SDLSurfaceSprite2D::UpdatePalette(pal);
	staleTexture = true;
}

void SDLTextureSprite2D::UpdateColorKey(colorkey_t key) noexcept
{
	SDLSurfaceSprite2D::UpdateColorKey(key);
	staleTexture = true;
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
