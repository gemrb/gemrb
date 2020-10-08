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

		SurfaceHolder(SDL_Surface* surf) : surface(surf), palette(NULL) {}
		~SurfaceHolder() { SDL_FreeSurface(surface); }

		SDL_Surface* operator->() { return surface; }
		operator SDL_Surface* () { return surface; }
	};

	Holder<SurfaceHolder> original;
	mutable Holder<SurfaceHolder> surface;
	mutable version_t version = 0;
	mutable version_t palVersion = 0;

public:
	SDLSurfaceSprite2D(const Region&, int Bpp, void* pixels,
					   ieDword rmask, ieDword gmask, ieDword bmask, ieDword amask);
	SDLSurfaceSprite2D(const Region&, int Bpp,
					   ieDword rmask, ieDword gmask, ieDword bmask, ieDword amask);
	SDLSurfaceSprite2D(const SDLSurfaceSprite2D &obj);
	SDLSurfaceSprite2D* copy() const;

	const void* LockSprite() const;
	void* LockSprite();
	void UnlockSprite() const;

	PaletteHolder GetPalette() const;
	int SetPalette(const Color* pal) const;
	void SetPalette(PaletteHolder pal);
	int32_t GetColorKey() const;
	void SetColorKey(ieDword pxvalue);
	bool HasTransparency() const;
	Color GetPixel(const Point&) const;
	bool ConvertFormatTo(int bpp, ieDword rmask, ieDword gmask,
						 ieDword bmask, ieDword amask);

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
	struct TextureHolder : public Held<TextureHolder>
	{
		SDL_Texture* texture;

		TextureHolder(SDL_Texture* tex) : texture(tex) {}
		~TextureHolder() { SDL_DestroyTexture(texture); }

		SDL_Texture* operator->() { return texture; }
		operator SDL_Texture* () { return texture; }
	};

	mutable Holder<TextureHolder> texture;

public:
	SDLTextureSprite2D(const Region&, int Bpp, void* pixels,
					   ieDword rmask, ieDword gmask, ieDword bmask, ieDword amask);
	SDLTextureSprite2D(const Region&, int Bpp,
					   ieDword rmask, ieDword gmask, ieDword bmask, ieDword amask);
	SDLTextureSprite2D(const SDLTextureSprite2D& obj);
	SDLTextureSprite2D* copy() const;

	using SDLSurfaceSprite2D::SetPalette;
	void SetColorKey(ieDword pxvalue);

	SDL_Texture* GetTexture(SDL_Renderer* renderer) const;

	void* NewVersion(version_t version) const;
	void Restore() const;
};
#endif

}

#endif  // ! SDLSURFACESPRITE2D_H
