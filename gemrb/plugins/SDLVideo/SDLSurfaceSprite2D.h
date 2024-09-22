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
	using version_t = Hash;

protected:
	SDL_Surface* surface = nullptr;

	mutable BlitFlags appliedBlitFlags = BlitFlags::NONE;
	mutable Color appliedTint;
	mutable bool surfaceInvalidated = false;

	mutable version_t palVersion;
	mutable Holder<Palette> shadedPalette;
	mutable version_t shadedPaletteVersion;

	void UpdatePalette() noexcept override;
	void UpdateColorKey() noexcept override;
	void UpdateSurfaceAndPalette() noexcept;
	virtual void OnSurfaceUpdate() const noexcept {}

private:
	void EnsureShadedPalette() const noexcept;
	bool NeedToReshade(const Color* tint, BlitFlags flags) const noexcept;
	bool NeedToUpdatePalette() const noexcept;
	void ShadePalette(BlitFlags renderFlags, const Color* tint) const noexcept;
	void UpdatePalettesState(bool flagsChanged) const noexcept;
	void UpdatePaletteForSurface(const Palette& pal) const noexcept;
	void Invalidate() noexcept;

public:
	SDLSurfaceSprite2D(const Region&, void* pixels, const PixelFormat& fmt) noexcept;
	SDLSurfaceSprite2D(const Region&, const PixelFormat& fmt) noexcept;
	SDLSurfaceSprite2D(const SDLSurfaceSprite2D &obj) noexcept;
	~SDLSurfaceSprite2D() noexcept override;

	Holder<Sprite2D> copy() const override;

	const void* LockSprite() const override;
	void* LockSprite() override;
	void UnlockSprite() override;

	bool HasTransparency() const noexcept override;
	bool ConvertFormatTo(const PixelFormat& tofmt) noexcept override;

	SDL_Surface* GetSurface() const;
	
	BlitFlags PrepareForRendering(BlitFlags flags, const Color* = nullptr) const noexcept;
};

#if SDL_VERSION_ATLEAST(1,3,0)
// TODO: this is a lazy implementation
// it would probably be better to not inherit from SDLSurfaceSprite2D
// the hard part is handling the palettes ourselves
class SDLTextureSprite2D : public SDLSurfaceSprite2D {
	mutable Uint32 texFormat = SDL_PIXELFORMAT_UNKNOWN;
	mutable SDL_Texture* texture = nullptr;
	mutable bool staleTexture = false;

	void OnSurfaceUpdate() const noexcept override;
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
