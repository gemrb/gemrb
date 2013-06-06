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

namespace GemRB {

enum MultiGestureType {
	GESTURE_NONE = 0,
	GESTURE_FORMATION_ROTATION = 1
};

struct MultiGesture {
	MultiGestureType type;
	Point endPoint;
	ieWord endButton;
	// for future consideration
	// float theta;
	// int dx;
	// int dy;
};

class SDL20VideoDriver : public SDLVideoDriver {
private:
	SDL_Texture* screenTexture;
	SDL_Window* window;
	SDL_Renderer* renderer;

	// touch input vars
	int ignoreNextFingerUp;
	SDL_TouchFingerEvent firstFingerDown;
	unsigned long firstFingerDownTime;
	MultiGesture currentGesture;
public:
	SDL20VideoDriver(void);
	~SDL20VideoDriver(void);

	int CreateDisplay(int w, int h, int b, bool fs, const char* title);
	int SwapBuffers(void);
	int PollEvents();

	void InitMovieScreen(int &w, int &h, bool yuv);
	virtual void DestroyMovieScreen();

	void showFrame(unsigned char* buf, unsigned int bufw,
									unsigned int bufh, unsigned int sx, unsigned int sy, unsigned int w,
									unsigned int h, unsigned int dstx, unsigned int dsty,
									int g_truecolor, unsigned char *pal, ieDword titleref);
	void showYUVFrame(unsigned char** buf, unsigned int *strides,
					  unsigned int bufw, unsigned int bufh,
					  unsigned int w, unsigned int h,
					  unsigned int dstx, unsigned int dsty,
					  ieDword titleref);

	bool SetFullscreenMode(bool set);
	void SetGamma(int brightness, int contrast);
	bool ToggleGrabInput();
	void ShowSoftKeyboard();
	void HideSoftKeyboard();
	void MoveMouse(unsigned int x, unsigned int y);
private:
	bool SetSurfacePalette(SDL_Surface* surface, SDL_Color* colors, int ncolors);
	bool SetSurfaceAlpha(SDL_Surface* surface, unsigned short alpha);

	int ProcessEvent(const SDL_Event & event);
	// the first finger touch of a gesture is delayed until another touch event or until
	// enough time passes for it to become a right click
	bool ProcessFirstTouch( int mouseButton );
	void ClearFirstTouch();
	void ClearGesture();

	// temporary methods to scale input coordinates from the renderer to the backbuf
	// once we have a real SDL2 render pipeline in place we shouldnt require this.
	// this should only apply to devices where the window size cannot be guaranteed
	// to match the render size (iOS, Android)

	// TODO: probably need to apply this to mouse input
	float ScaleCoordinateHorizontal(float x);
	float ScaleCoordinateVertical(float y);
};

}

#endif
