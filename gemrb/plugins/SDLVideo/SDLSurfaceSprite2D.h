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

#include "Sprite2D.h"

struct SDL_Surface;
struct SDL_Color;

namespace GemRB {

class SDLSurfaceSprite2D : public Sprite2D {
private:
	SDL_Surface* surface;
public:
	SDLSurfaceSprite2D(int Width, int Height, int Bpp, void* pixels,
					   ieDword rmask = 0, ieDword gmask = 0, ieDword bmask = 0, ieDword amask = 0);
	SDLSurfaceSprite2D(const SDLSurfaceSprite2D &obj);
	SDLSurfaceSprite2D* copy() const;
	~SDLSurfaceSprite2D();

	Palette *GetPalette() const;
	const Color* GetPaletteColors() const;
	void SetPalette(Palette *pal);
	void SetPalette(Color* pal);
	ieDword GetColorKey() const;
	void SetColorKey(ieDword pxvalue);
	Color GetPixel(unsigned short x, unsigned short y) const;
	bool ConvertFormatTo(int bpp, ieDword rmask, ieDword gmask,
						 ieDword bmask, ieDword amask);

	SDL_Surface* GetSurface() const { return surface; };
};

}

#endif  // ! SDLSURFACESPRITE2D_H
