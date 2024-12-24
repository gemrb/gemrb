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

#include "Logging/Logging.h"

namespace GemRB {

SDLSurfaceSprite2D::SDLSurfaceSprite2D(const Region& rgn, void* px, const PixelFormat& fmt) noexcept
	: Sprite2D(rgn, px, fmt)
{
	if (px) {
		surface = SDL_CreateRGBSurfaceFrom(px, Frame.w, Frame.h, fmt.Depth, Frame.w * fmt.Bpp,
						   fmt.Rmask, fmt.Gmask, fmt.Bmask, fmt.Amask);
	} else {
		assert(fmt.Depth >= 8);
		surface = SDL_CreateRGBSurface(0, Frame.w, Frame.h, fmt.Depth, fmt.Rmask, fmt.Gmask, fmt.Bmask, fmt.Amask);
		SDL_FillRect(surface, nullptr, 0);
		pixels = surface->pixels;
	}

	assert(surface);
	pitch = surface->pitch;

	UpdateColorKey();
	format = PixelFormatForSurface(surface, format.palette);

	if (format.palette) {
		UpdatePalette();
	}
}

SDLSurfaceSprite2D::SDLSurfaceSprite2D(const Region& rgn, const PixelFormat& fmt) noexcept
	: SDLSurfaceSprite2D(rgn, nullptr, fmt)
{}

SDLSurfaceSprite2D::SDLSurfaceSprite2D(const SDLSurfaceSprite2D& obj) noexcept
	: SDLSurfaceSprite2D(obj.Frame, nullptr, obj.format)
{
	renderFlags = obj.renderFlags;
	appliedBlitFlags = obj.appliedBlitFlags;
	appliedTint = obj.appliedTint;

	// note: make sure that src & dest have the same palette applied before this
	// everything else will end up in SDL color transformation magic
	SDL_BlitSurface(obj.surface, nullptr, surface, nullptr);
}

SDLSurfaceSprite2D::~SDLSurfaceSprite2D() noexcept
{
	SDL_FreeSurface(surface);
}

Holder<Sprite2D> SDLSurfaceSprite2D::copy() const
{
	return Holder<Sprite2D>(new SDLSurfaceSprite2D(*this));
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

void SDLSurfaceSprite2D::UnlockSprite()
{
	SDL_UnlockSurface(surface);
	Invalidate();
}

void SDLSurfaceSprite2D::Invalidate() noexcept
{
	surfaceInvalidated = true;
}

void SDLSurfaceSprite2D::UpdateColorKey() noexcept
{
#if SDL_VERSION_ATLEAST(1, 3, 0)
	SDL_SetColorKey(surface, SDL_bool(format.HasColorKey), format.ColorKey);
	// don't RLE with SDL 2
	// this only benefits SDL_BlitSurface which we don't use. its a slowdown for us.
#else
	Uint32 flag = format.HasColorKey ? SDL_SRCCOLORKEY : 0;
	SDL_SetColorKey(surface, flag | SDL_RLEACCEL, format.ColorKey);
#endif
}

bool SDLSurfaceSprite2D::HasTransparency() const noexcept
{
	const SDL_PixelFormat* fmt = surface->format;
#if SDL_VERSION_ATLEAST(1, 3, 0)
	return SDL_ISPIXELFORMAT_ALPHA(fmt->format) || SDL_GetColorKey(surface, NULL) != -1;
#else
	return fmt->Amask > 0 || ((surface)->flags & SDL_SRCCOLORKEY);
#endif
}

bool SDLSurfaceSprite2D::ConvertFormatTo(const PixelFormat& tofmt) noexcept
{
	if (tofmt.Bpp == 0) {
		return false;
	}

#if SDL_VERSION_ATLEAST(1, 3, 0)
	Uint32 fmt = SDL_MasksToPixelFormatEnum(tofmt.Bpp * 8, tofmt.Rmask, tofmt.Gmask, tofmt.Bmask, tofmt.Amask);
	if (fmt != SDL_PIXELFORMAT_UNKNOWN) {
		SDL_Surface* ns = SDL_ConvertSurfaceFormat(surface, fmt, 0);
#else
	SDL_Surface* tmp = SDL_CreateRGBSurface(SDL_SWSURFACE, Frame.w, Frame.h, tofmt.Depth, tofmt.Rmask, tofmt.Gmask, tofmt.Bmask, tofmt.Amask);
	if (tmp) {
		SDL_Surface* ns = SDL_ConvertSurface(surface, tmp->format, 0);
		SDL_FreeSurface(tmp);
#endif
		if (ns) {
			if (freePixels) {
				free(pixels);
			}
			freePixels = false;
			surface = ns;
			format = PixelFormatForSurface(ns);
			if (ns->format->palette) {
				UpdatePaletteForSurface(*format.palette);
				palVersion = format.palette->GetVersion();
			}
			return true;
		} else {
			Log(MESSAGE, "SDLSurfaceSprite2D",
#if SDL_VERSION_ATLEAST(1, 3, 0)
			    "Cannot convert sprite to format: %s\nError: %s", SDL_GetPixelFormatName(fmt),
#else
			    "Cannot convert sprite to format: %s",
#endif
			    SDL_GetError());
		}
	}
	return false;
}

void SDLSurfaceSprite2D::EnsureShadedPalette() const noexcept
{
	if (!shadedPalette) {
		shadedPalette = MakeHolder<Palette>();
	}
}

void SDLSurfaceSprite2D::ShadePalette(BlitFlags renderflags, const Color* tint) const noexcept
{
	Palette::Colors buffer;
	buffer[0] = format.palette->GetColorAt(0);

	size_t startIndex = format.HasColorKey ? 1 : 0;
	for (size_t i = startIndex; i < 256; ++i) {
		buffer[i] = format.palette->GetColorAt(i);

		if (renderflags & BlitFlags::COLOR_MOD && tint) {
			ShaderTint(*tint, buffer[i]);
		}

		if (renderflags & BlitFlags::ALPHA_MOD && tint) {
			buffer[i].a = tint->a;
		}

		if (renderflags & BlitFlags::GREY) {
			ShaderGreyscale(buffer[i]);
		} else if (renderflags & BlitFlags::SEPIA) {
			ShaderSepia(buffer[i]);
		}
	}

	shadedPalette->CopyColors(0, buffer.cbegin(), buffer.cend());
}

bool SDLSurfaceSprite2D::NeedToUpdatePalette() const noexcept
{
	return format.palette && palVersion != format.palette->GetVersion();
}

bool SDLSurfaceSprite2D::NeedToReshade(const Color* tint, BlitFlags flags) const noexcept
{
	return flags != appliedBlitFlags || surfaceInvalidated || NeedToUpdatePalette() || ((flags & BlitFlags::COLOR_MOD) && tint && (appliedTint.Packed() & 0xFFFFFF00) != (tint->Packed() & 0xFFFFFF00)) || ((flags & BlitFlags::ALPHA_MOD) && tint && (appliedTint.Packed() & 0xFF) != (tint->Packed() & 0xFF));
}

BlitFlags SDLSurfaceSprite2D::PrepareForRendering(BlitFlags renderflags, const Color* tint) const noexcept
{
	// Non-paletted surfaces can only have their pixels changed: refresh texture
	if (format.Bpp > 1) {
		if (surfaceInvalidated) {
			OnSurfaceUpdate();
			surfaceInvalidated = false;
		}

		// All surface operations to be done by SDL
		return BlitFlags::NONE;
	}

	auto blitFlags = (BlitFlags::GREY | BlitFlags::SEPIA) & renderflags;
	if (tint) {
		blitFlags |= (BlitFlags::COLOR_MOD | BlitFlags::ALPHA_MOD) & renderflags;
	}

	// Something has changed/is new?
	if (NeedToReshade(tint, blitFlags)) {
		// modifiers for shaded version of palette/surface?
		if (blitFlags) {
			EnsureShadedPalette();
			ShadePalette(blitFlags, tint);
		}

		bool flagsChanged = blitFlags != appliedBlitFlags;
		surfaceInvalidated = false;
		appliedBlitFlags = blitFlags;
		if (tint) {
			appliedTint = *tint;
		}

		UpdatePalettesState(flagsChanged);
	}

	return appliedBlitFlags;
}

void SDLSurfaceSprite2D::UpdatePalette() noexcept
{
	PrepareForRendering(appliedBlitFlags, &appliedTint);
}

void SDLSurfaceSprite2D::UpdatePaletteForSurface(const Palette& pal) const noexcept
{
	SDLVideoDriver::SetSurfacePalette(surface, reinterpret_cast<const SDL_Color*>(pal.ColorData()), 0x01 << format.Depth);
#if SDL_VERSION_ATLEAST(1, 3, 0)
	// must reset the color key or SDL 2 won't render properly
	SDL_SetColorKey(surface, SDL_bool(format.HasColorKey), GetColorKey());
#endif
}

void SDLSurfaceSprite2D::UpdatePalettesState(bool flagsChanged) const noexcept
{
	// effects modified?
	if (appliedBlitFlags && shadedPaletteVersion != shadedPalette->GetVersion()) {
		UpdatePaletteForSurface(*shadedPalette);
		shadedPaletteVersion = shadedPalette->GetVersion();
		// main palette modified or all effects lost?
	} else if (!appliedBlitFlags && (flagsChanged || NeedToUpdatePalette())) {
		UpdatePaletteForSurface(*format.palette);
	}
	// version has been applied in either case
	palVersion = format.palette->GetVersion();

	if (!appliedBlitFlags && shadedPalette) {
		shadedPalette.reset();
		shadedPaletteVersion = 0;
	}

	OnSurfaceUpdate();
}

SDL_Surface* SDLSurfaceSprite2D::GetSurface() const
{
	return surface;
}

#if SDL_VERSION_ATLEAST(1, 3, 0)
SDLTextureSprite2D::SDLTextureSprite2D(const Region& rgn, void* pixels, const PixelFormat& fmt) noexcept
	: SDLSurfaceSprite2D(rgn, pixels, fmt)
{}

SDLTextureSprite2D::SDLTextureSprite2D(const Region& rgn, const PixelFormat& fmt) noexcept
	: SDLSurfaceSprite2D(rgn, fmt)
{}

SDLTextureSprite2D::~SDLTextureSprite2D() noexcept
{
	SDL_DestroyTexture(texture);
}

SDLTextureSprite2D::SDLTextureSprite2D(const SDLTextureSprite2D& other) noexcept
	: SDLSurfaceSprite2D(other), texFormat(other.texFormat)
{}

Holder<Sprite2D> SDLTextureSprite2D::copy() const
{
	return Holder<Sprite2D>(new SDLTextureSprite2D(*this));
}

SDL_Texture* SDLTextureSprite2D::GetTexture(SDL_Renderer* renderer) const
{
	if (texture == nullptr) {
		texture = SDL_CreateTextureFromSurface(renderer, GetSurface());
		SDL_QueryTexture(texture, &texFormat, nullptr, nullptr, nullptr);
	} else if (staleTexture) {
		SDL_Surface* surface = GetSurface();
		if (texFormat == surface->format->format) {
			SDL_UpdateTexture(texture, nullptr, surface->pixels, surface->pitch);
		} else {
			SDL_Surface* temp = SDL_ConvertSurfaceFormat(surface, texFormat, 0);
			assert(temp);
			SDL_UpdateTexture(texture, nullptr, temp->pixels, temp->pitch);
			SDL_FreeSurface(temp);
		}
		staleTexture = false;
	}
	return texture;
}

void SDLTextureSprite2D::OnSurfaceUpdate() const noexcept
{
	staleTexture = true;
}
#endif

}
