/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2012 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

#ifndef SDL20VideoDRIVER_H
#define SDL20VideoDRIVER_H

#include "SDLVideo.h"

class SDL20VideoDriver : public SDLVideoDriver {
private:
	SDL_Texture* videoPlayer;
	SDL_Window* window;
	SDL_Renderer* renderer;
public:
	SDL20VideoDriver(void);
	~SDL20VideoDriver(void);

	int CreateDisplay(int w, int h, int b, bool fs, const char* title);
	int SwapBuffers(void);
	void InitMovieScreen(int &w, int &h, bool yuv);
	void showFrame(unsigned char* buf, unsigned int bufw,
									unsigned int bufh, unsigned int sx, unsigned int sy, unsigned int w,
									unsigned int h, unsigned int dstx, unsigned int dsty,
									int g_truecolor, unsigned char *pal, ieDword titleref);
	void showYUVFrame(unsigned char** buf, unsigned int *strides,
					  unsigned int bufw, unsigned int bufh,
					  unsigned int w, unsigned int h,
					  unsigned int dstx, unsigned int dsty,
					  ieDword titleref);

	bool ToggleGrabInput();
	void ShowSoftKeyboard();
	void HideSoftKeyboard();
	void MoveMouse(unsigned int x, unsigned int y);
private:
	bool SetSurfacePalette(SDL_Surface* surface, SDL_Color* colors, int ncolors);
	bool SetSurfaceAlpha(SDL_Surface* surface, unsigned short alpha);
};

#endif
