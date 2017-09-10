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
	// touch input vars
	int ignoreNextFingerUp;
	SDL_TouchFingerEvent firstFingerDown;
	unsigned long firstFingerDownTime;
	MultiGesture currentGesture;

protected:
	SDL_Window* window;
	SDL_Renderer* renderer;

public:
	SDL20VideoDriver(void);
	~SDL20VideoDriver(void);

	int CreateDisplay(int w, int h, int b, bool fs, const char* title);
	int CreateDriverDisplay(const Size& s, int bpp, const char* title);
	void SwapBuffers(VideoBuffers& buffers);
	int PollEvents();

	Sprite2D* GetScreenshot( Region r );
	bool SetFullscreenMode(bool set);
	void SetGamma(int brightness, int contrast);
	bool ToggleGrabInput();
	void ShowSoftKeyboard();
	void HideSoftKeyboard();
	bool TouchInputEnabled();

private:
	VideoBuffer* NewVideoBuffer(const Region&, BufferFormat);

	bool SetSurfaceAlpha(SDL_Surface* surface, unsigned short alpha);

	int ProcessEvent(const SDL_Event & event);
	// the first finger touch of a gesture is delayed until another touch event or until
	// enough time passes for it to become a right click
	bool ProcessFirstTouch( int mouseButton );
	void ClearFirstTouch();
	void BeginMultiGesture(MultiGestureType type);
	void EndMultiGesture(bool success = false);

	// temporary methods to scale input coordinates from the renderer to the backbuf
	// once we have a real SDL2 render pipeline in place we shouldnt require this.
	// this should only apply to devices where the window size cannot be guaranteed
	// to match the render size (iOS, Android)

	// TODO: probably need to apply this to mouse input
	float ScaleCoordinateHorizontal(float x);
	float ScaleCoordinateVertical(float y);
};
	
class SDLTextureVideoBuffer : public VideoBuffer {
	SDL_Texture* texture;
	
private:
	Region TextureRegion(SDL_Texture* tex, const Point& p) {
		int w, h;
		SDL_QueryTexture(tex, NULL, NULL, &w, &h);
		return Region(p, ::GemRB::Size(w, h));
	}
	
public:
	SDLTextureVideoBuffer(const Point& p, SDL_Texture* texture)
	: VideoBuffer(TextureRegion(texture, p)), texture(texture)
	{
		assert(texture);
	}
	
	~SDLTextureVideoBuffer() {
		SDL_DestroyTexture(texture);
	}
	
	void Clear() {
		void *pixels;
		int pitch;
		if(SDL_LockTexture(texture, NULL, &pixels, &pitch) != GEM_OK) {
			Log(ERROR, "SDL 2 driver", "Unable to lock screen texture: %s", SDL_GetError());
			return;
		}
		
		ieByte* dest = (ieByte*)pixels;
		for(int row = 0; row < rect.h; row++) {
			memset(dest, SDL_ALPHA_TRANSPARENT, pitch);
			dest += pitch;
		}
		SDL_UnlockTexture(texture);
	}
	
	bool RenderOnDisplay(void* display) {
		SDL_Renderer* renderer = static_cast<SDL_Renderer*>(display);
		SDL_Rect dst = RectFromRegion(rect);
		SDL_RenderCopy(renderer, texture, NULL, &dst);
		return true;
	}
	
	void CopyPixels(const Region& bufDest, void* pixelBuf, const int* pitch = NULL, ...) {
		int sdlpitch = (pitch) ? *pitch : rect.w;
		SDL_Rect dest = RectFromRegion(bufDest);
		SDL_UpdateTexture(texture, &dest, pixelBuf, sdlpitch);
		// FIXME: don't know if this comment still applies
		/*
		 Commenting this out because I get better performance (on iOS) with SDL_UpdateTexture
		 Don't know how universal it is yet so leaving this in commented out just in case
		 
		 void *pixels;
		 int pitch;
		 if(SDL_LockTexture(screenTexture, NULL, &pixels, &pitch) != GEM_OK) {
		 Log(ERROR, "SDL 2 driver", "Unable to lock screen texture: %s", SDL_GetError());
		 return GEM_ERROR;
		 }
		 
		 ieByte* src = (ieByte*)backBuf->pixels;
		 ieByte* dest = (ieByte*)pixels;
		 for( int row = 0; row < height; row++ ) {
		 memcpy(dest, src, width * backBuf->format->BytesPerPixel);
		 dest += pitch;
		 src += backBuf->pitch;
		 }
		 SDL_UnlockTexture(screenTexture);
		 */
	}
};

}

#endif
