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
protected:
	struct SurfaceHolder : public Held<SurfaceHolder>
	{
		SDL_Surface* surface;
		Palette* palette; // simply a cache for comparing against calls to SetPalette for performance reasons.

		SurfaceHolder(SDL_Surface* surf) : surface(surf), palette(NULL) {}
		~SurfaceHolder() { SDL_FreeSurface(surface); }

		SDL_Surface* operator->() { return surface; }

		operator SDL_Surface* () { return surface; }
	};

	mutable Holder<SurfaceHolder> surface;

public:
	SDLSurfaceSprite2D(int Width, int Height, int Bpp, void* pixels,
					   ieDword rmask, ieDword gmask, ieDword bmask, ieDword amask);
	SDLSurfaceSprite2D(int Width, int Height, int Bpp,
					   ieDword rmask, ieDword gmask, ieDword bmask, ieDword amask);
	SDLSurfaceSprite2D(const SDLSurfaceSprite2D &obj);
	SDLSurfaceSprite2D* copy() const;

	const void* LockSprite() const;
	void* LockSprite();
	void UnlockSprite() const;

	Palette *GetPalette() const;
	const Color* GetPaletteColors() const;
	int SetPalette(const Color* pal);
	void SetPalette(Palette *pal);
	ieDword GetColorKey() const;
	void SetColorKey(ieDword pxvalue);
	bool HasTransparency() const;
	Color GetPixel(unsigned short x, unsigned short y) const;
	bool ConvertFormatTo(int bpp, ieDword rmask, ieDword gmask,
						 ieDword bmask, ieDword amask);

	SDL_Surface* GetSurface() const { return *surface; };
};

#if SDL_VERSION_ATLEAST(1,3,0)
// TODO: this is a lazy implementation
// it would probably be better to not inherit from SDLSurfaceSprite2D
// the hard part is handling the palettes ourselves
class SDLTextureSprite2D : public SDLSurfaceSprite2D {
	struct TextureHolder : public Held<TextureHolder>
	{
		SDL_Texture* texture;
		unsigned int version;

		TextureHolder(SDL_Texture* tex) : texture(tex), version(0) {}
		~TextureHolder() { SDL_DestroyTexture(texture); }

		SDL_Texture* operator->() { return texture; }

		operator SDL_Texture* () { return texture; }
	};

	mutable unsigned int texVersion;
	Holder<SurfaceHolder> original;
	mutable Holder<TextureHolder> texture;

public:
	SDLTextureSprite2D(int Width, int Height, int Bpp, void* pixels,
					   ieDword rmask, ieDword gmask, ieDword bmask, ieDword amask);
	SDLTextureSprite2D(int Width, int Height, int Bpp,
					   ieDword rmask, ieDword gmask, ieDword bmask, ieDword amask);
	SDLTextureSprite2D(const SDLTextureSprite2D& obj);
	SDLTextureSprite2D* copy() const;

	using SDLSurfaceSprite2D::SetPalette;
	void SetPalette(Palette *pal);
	void SetColorKey(ieDword pxvalue);

	SDL_Texture* GetTexture(SDL_Renderer* renderer) const;

	SDL_Surface* NewVersion(unsigned int version) const;
	unsigned int GetVersion() const { return texVersion; }
};
#endif

}

#endif  // ! SDLSURFACESPRITE2D_H
