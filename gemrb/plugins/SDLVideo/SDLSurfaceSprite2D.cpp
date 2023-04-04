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

SDLSurfaceSprite2D::SDLSurfaceSprite2D (const Region& rgn, void* px, const PixelFormat& fmt) noexcept
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
	
	if (format.palette) {
		UpdatePalette();
	}
	UpdateColorKey();
	
	format = PixelFormatForSurface(surface, format.palette);
	
	renderedSurface = surface;
}

SDLSurfaceSprite2D::SDLSurfaceSprite2D (const Region& rgn, const PixelFormat& fmt) noexcept
: SDLSurfaceSprite2D(rgn, nullptr, fmt)
{}

SDLSurfaceSprite2D::SDLSurfaceSprite2D(const SDLSurfaceSprite2D &obj) noexcept
: SDLSurfaceSprite2D(obj.Frame, nullptr, obj.format)
{
	SDL_BlitSurface(obj.surface, nullptr, surface, nullptr);
	renderFlags = obj.renderFlags;
}

SDLSurfaceSprite2D::~SDLSurfaceSprite2D() noexcept
{
	if (renderedSurface != surface) {
		SDL_FreeSurface(renderedSurface);
	}
	SDL_FreeSurface(surface);
}

void SDLSurfaceSprite2D::UpdateSurfacePalette() const noexcept
{
	assert (format.Depth <= 8);
	
	const auto& pal = format.palette;
	SDLVideoDriver::SetSurfacePalette(surface, reinterpret_cast<const SDL_Color*>(pal->col), 0x01 << format.Depth);
#if SDL_VERSION_ATLEAST(1,3,0)
	// must reset the color key or SDL 2 won't render properly
	SDL_SetColorKey(surface, SDL_bool(format.HasColorKey), GetColorKey());
#endif
	palVersion = pal->GetVersion();
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

void SDLSurfaceSprite2D::UnlockSprite() const
{
	SDL_UnlockSurface(surface);
	Invalidate();
}

bool SDLSurfaceSprite2D::IsPaletteStale() const noexcept
{
	// a 'version' implies tha palette was color modified
	// however, the only caller her is RenderWithFlags
	// where we already compare the version with the requested one
	// the version includes the color modification so for now we dont worry about it
	
	const PaletteHolder& pal = format.palette;
	assert(pal);
	return pal->GetVersion() != palVersion;
}

void SDLSurfaceSprite2D::UpdatePalette() noexcept
{
	Invalidate();
}

void SDLSurfaceSprite2D::UpdateColorKey() noexcept
{
#if SDL_VERSION_ATLEAST(1,3,0)
	SDL_SetColorKey(surface, SDL_bool(format.HasColorKey), format.ColorKey);
	// don't RLE with SDL 2
	// this only benifits SDL_BlitSurface which we don't use. its a slowdown for us.
#else
	Uint32 flag = format.HasColorKey ? SDL_SRCCOLORKEY : 0;
	SDL_SetColorKey(surface, flag | SDL_RLEACCEL, format.ColorKey);
#endif
}

bool SDLSurfaceSprite2D::HasTransparency() const noexcept
{
	const SDL_PixelFormat* fmt = surface->format;
#if SDL_VERSION_ATLEAST(1,3,0)
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

#if SDL_VERSION_ATLEAST(1,3,0)
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
				UpdateSurfacePalette();
			}
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
	return false;
}

void SDLSurfaceSprite2D::Invalidate() const noexcept
{
	version = 0;
	if (renderedSurface != surface) {
		SDL_FreeSurface(renderedSurface);
		renderedSurface = surface;
	}
	if (format.Depth <= 8) {
		UpdateSurfacePalette();
	}
}

void* SDLSurfaceSprite2D::NewVersion(version_t newversion) const noexcept
{
	if (newversion == 0 || version != newversion) {
		Invalidate();
		version = newversion;
	}

	if (format.Bpp == 1) {
		// we just allow overwritting the palette
		return surface->format->palette;
	}

	palVersion = 0;

	if (version != newversion) {
		renderedSurface = SDL_ConvertSurface(surface, surface->format, 0);
	}
	return renderedSurface;
}
	
BlitFlags SDLSurfaceSprite2D::RenderWithFlags(BlitFlags renderflags, const Color* tint) const noexcept
{
	SDLSurfaceSprite2D::version_t oldVersion = version;
	SDLSurfaceSprite2D::version_t newVersion = renderflags;
	auto ret = (BlitFlags::GREY | BlitFlags::SEPIA) & newVersion;
	
	if (format.Bpp == 1) {
		if (tint) {
			assert(renderflags & (BlitFlags::COLOR_MOD | BlitFlags::ALPHA_MOD));
			uint64_t tintv = *reinterpret_cast<const uint32_t*>(tint);
			newVersion |= tintv << 32;
		}
		
		if (oldVersion != newVersion || IsPaletteStale()) {
			SDL_Palette* pal = static_cast<SDL_Palette*>(NewVersion(newVersion));

			for (size_t i = 1; i < 256; ++i) {
				Color& dstc = reinterpret_cast<Color&>(pal->colors[i]);

				if (renderflags&BlitFlags::COLOR_MOD) {
					assert(tint);
					ShaderTint(*tint, dstc);
					ret |= BlitFlags::COLOR_MOD;
				}
				
				if (renderflags & BlitFlags::ALPHA_MOD) {
					assert(tint);
					dstc.a = tint->a;
					ret |= BlitFlags::ALPHA_MOD;
				}

				if (renderflags&BlitFlags::GREY) {
					ShaderGreyscale(dstc);
				} else if (renderflags&BlitFlags::SEPIA) {
					ShaderSepia(dstc);
				}
			}
		} else {
			ret |= (BlitFlags::COLOR_MOD | BlitFlags::ALPHA_MOD) & newVersion;
		}
	} else if (oldVersion != newVersion) {
		SDL_Surface* newV = (SDL_Surface*)NewVersion(newVersion);
		SDL_LockSurface(newV);

		const Region& r = {0, 0, newV->w, newV->h};
		SDLPixelIterator beg = MakeSDLPixelIterator(newV, r);
		SDLPixelIterator end = SDLPixelIterator::end(beg);
		StaticAlphaIterator alpha(0xff);

		if (renderflags & BlitFlags::GREY) {
			RGBBlendingPipeline<SHADER::GREYSCALE, true> blender;
			Blit(beg, beg, end, alpha, blender);
		} else if (renderflags & BlitFlags::SEPIA) {
			RGBBlendingPipeline<SHADER::SEPIA, true> blender;
			Blit(beg, beg, end, alpha, blender);
		}
		SDL_UnlockSurface(newV);
	}
	return static_cast<BlitFlags>(ret);
}

#if SDL_VERSION_ATLEAST(1,3,0)
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
	: SDLSurfaceSprite2D(other), texFormat(other.texFormat), texture(nullptr),
	staleTexture(false)
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
		SDL_Surface *surface = GetSurface();
		if (texFormat == surface->format->format) {
			SDL_UpdateTexture(texture, nullptr, surface->pixels, surface->pitch);
		} else {
			SDL_Surface *temp = SDL_ConvertSurfaceFormat(surface, texFormat, 0);
			assert(temp);
			SDL_UpdateTexture(texture, nullptr, temp->pixels, temp->pitch);
			SDL_FreeSurface(temp);
		}
		staleTexture = false;
	}
	return texture;
}

void SDLTextureSprite2D::Invalidate() const noexcept
{
	staleTexture = true;
	SDLSurfaceSprite2D::Invalidate();
}
#endif

}
