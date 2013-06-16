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
	bool freePixels;
	SDL_Surface* surface;
public:
	SDLSurfaceSprite2D(int Width, int Height, int Bpp, void* pixels);
	SDLSurfaceSprite2D(int Width, int Height, int Bpp, void* pixels,
					   ieDword rmask, ieDword gmask, ieDword bmask, ieDword amask);
	SDLSurfaceSprite2D(const SDLSurfaceSprite2D &obj);
	SDLSurfaceSprite2D* copy() const;
	~SDLSurfaceSprite2D();

	Palette *GetPalette() const;
	void SetPalette(Palette *pal);
	void SetPalette(Color* pal);
	ieDword GetColorKey() const;
	void SetColorKey(ieDword ck);
	Color GetPixel(unsigned short x, unsigned short y) const;

	static void SetSurfacePalette(SDL_Surface* surf, SDL_Color* pal, int numcolors = 256);
};

}

#endif  // ! SDLSURFACESPRITE2D_H
