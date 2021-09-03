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
	struct SurfaceHolder : public Held<SurfaceHolder>
	{
		SDL_Surface* surface;
		PaletteHolder palette; // simply a cache for comparing against calls to SetPalette for performance reasons.

		explicit SurfaceHolder(SDL_Surface* surf) : surface(surf) {}
		~SurfaceHolder() override { SDL_FreeSurface(surface); }

		SDL_Surface* operator->() const { return surface; }
		operator SDL_Surface* () const { return surface; }
	};

	Holder<SurfaceHolder> original;
	mutable Holder<SurfaceHolder> surface;
	mutable version_t version = 0;
	mutable version_t palVersion = 0;
	
	void SetPaletteFromSurface() const noexcept;
	bool SetPaletteColors(const Color* pal) const noexcept;
	void UpdatePalette(PaletteHolder) noexcept override;
	void UpdateColorKey(colorkey_t key) noexcept override;

public:
	SDLSurfaceSprite2D(const Region&, void* pixels, const PixelFormat& fmt) noexcept;
	SDLSurfaceSprite2D(const Region&, const PixelFormat& fmt) noexcept;
	SDLSurfaceSprite2D(const SDLSurfaceSprite2D &obj) noexcept;
	Holder<Sprite2D> copy() const override;

	const void* LockSprite() const override;
	void* LockSprite() override;
	void UnlockSprite() const override;

	bool HasTransparency() const noexcept override;
	bool ConvertFormatTo(const PixelFormat& tofmt) noexcept override;

	SDL_Surface* GetSurface() const { return *surface; };

	// return a copy of the surface or the palette if 8 bit
	// this copy is what is returned from GetSurface for rendering
	virtual void* NewVersion(version_t version) const;
	version_t GetVersion() const noexcept { return version; }
	bool IsPaletteStale() const;
	// restore the sprite to version 0 (aka original) and free the versioned resources
	// an 8 bit sprite will be also implicitly restored by SetPalette()
	virtual void Restore() const;
};

#if SDL_VERSION_ATLEAST(1,3,0)
// TODO: this is a lazy implementation
// it would probably be better to not inherit from SDLSurfaceSprite2D
// the hard part is handling the palettes ourselves
class SDLTextureSprite2D : public SDLSurfaceSprite2D {
	struct TextureHolder final : public Held<TextureHolder>
	{
		SDL_Texture* texture;

		explicit TextureHolder(SDL_Texture* tex) : texture(tex) {}
		~TextureHolder() final { SDL_DestroyTexture(texture); }

		SDL_Texture* operator->() const { return texture; }
		operator SDL_Texture* () const { return texture; }
	};

	mutable Uint32 texFormat = SDL_PIXELFORMAT_UNKNOWN;
	mutable Holder<TextureHolder> texture;
	mutable bool staleTexture = false;
	
	void UpdatePalette(PaletteHolder) noexcept override;
	void UpdateColorKey(colorkey_t key) noexcept override;
	
public:
	SDLTextureSprite2D(const Region&, void* pixels, const PixelFormat& fmt) noexcept;
	SDLTextureSprite2D(const Region&, const PixelFormat& fmt) noexcept;
	Holder<Sprite2D> copy() const override;
	
	void UnlockSprite() const override;

	SDL_Texture* GetTexture(SDL_Renderer* renderer) const;

	void* NewVersion(version_t version) const override;
	void Restore() const override;
};
#endif

}

#endif  // ! SDLSURFACESPRITE2D_H
