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

#ifndef SDLSURFACESPRITE2D_H
#define SDLSURFACESPRITE2D_H

#include "Holder.h"
#include "Sprite2D.h"

#include <SDL.h>

namespace GemRB {

class SDLSurfaceSprite2D : public Sprite2D {
public:
	using version_t = uint64_t;

protected:
	SDL_Surface* surface = nullptr; // the actual surface backing the sprite, changes to it invalidate 'renderedSurface'
	mutable version_t palVersion = 0; // the format.palette version applied to 'surface'
	mutable SDL_Surface* renderedSurface = nullptr; // a cached surface matching 'version'
	mutable version_t version = 0; // the flags used to render 'renderedSurface'
	
	void UpdateSurfacePalette() const noexcept;
	void UpdatePalette() noexcept override;
	void UpdateColorKey() noexcept override;

	bool IsPaletteStale() const noexcept;
	// restore the sprite to version 0 (aka original) and free the versioned resources
	// an 8 bit sprite will be also implicitly restored by SetPalette()
	virtual void Invalidate() const noexcept;
	
	// return a copy of the surface or the palette if 8 bit
	// this copy is what is returned from GetSurface for rendering
	void* NewVersion(version_t version) const noexcept;
public:
	SDLSurfaceSprite2D(const Region&, void* pixels, const PixelFormat& fmt) noexcept;
	SDLSurfaceSprite2D(const Region&, const PixelFormat& fmt) noexcept;
	SDLSurfaceSprite2D(const SDLSurfaceSprite2D &obj) noexcept;
	~SDLSurfaceSprite2D() noexcept override;

	Holder<Sprite2D> copy() const override;

	const void* LockSprite() const override;
	void* LockSprite() override;
	void UnlockSprite() const override;

	bool HasTransparency() const noexcept override;
	bool ConvertFormatTo(const PixelFormat& tofmt) noexcept override;

	SDL_Surface* GetSurface() const { return renderedSurface; };
	
	// render to 'renderedSurface' any supported options passed in 'flags'
	// returns the flags which were successfully applied
	// operations are not cumulative and don't permanently alter the 'surface'
	BlitFlags RenderWithFlags(BlitFlags flags, const Color* = nullptr) const noexcept;
};

#if SDL_VERSION_ATLEAST(1,3,0)
// TODO: this is a lazy implementation
// it would probably be better to not inherit from SDLSurfaceSprite2D
// the hard part is handling the palettes ourselves
class SDLTextureSprite2D : public SDLSurfaceSprite2D {
	mutable Uint32 texFormat = SDL_PIXELFORMAT_UNKNOWN;
	mutable SDL_Texture* texture = nullptr;
	mutable bool staleTexture = false;
	
	void Invalidate() const noexcept override;
public:
	SDLTextureSprite2D(const SDLTextureSprite2D&) noexcept;
	SDLTextureSprite2D(const Region&, void* pixels, const PixelFormat& fmt) noexcept;
	SDLTextureSprite2D(const Region&, const PixelFormat& fmt) noexcept;
	~SDLTextureSprite2D() noexcept override;

	Holder<Sprite2D> copy() const override;
	
	SDL_Texture* GetTexture(SDL_Renderer* renderer) const;
};
#endif

}

#endif  // ! SDLSURFACESPRITE2D_H
