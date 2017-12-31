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
#include "SDLSurfaceSprite2D.h"

namespace GemRB {
	
Uint32 SDLPixelFormatFromBufferFormat(Video::BufferFormat fmt) {
	switch (fmt) {
		case Video::RGBPAL8:
		case Video::RGB565:
		case Video::RGBA8888:
		case Video::DISPLAY:
			// we always use 32bit RGBA and convert from RGBPAL8 and RGB565 to RGBA8888
			// part of the reason is that many devices don't support lower bit depth and SDL doesnt have a way to update paletized textures
			return SDL_PIXELFORMAT_RGBA8888;
		case Video::YV12:
			return SDL_PIXELFORMAT_YV12;
		default:
			return SDL_PIXELFORMAT_UNKNOWN;
	}
}

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
	SDL_Renderer* renderer;
	Video::BufferFormat inputFormat; // the format of the pixel data the client thinks we use, we may have to convert in CopyPixels()
	
private:
	Region TextureRegion(SDL_Texture* tex, const Point& p) {
		int w, h;
		SDL_QueryTexture(tex, NULL, NULL, &w, &h);
		return Region(p, ::GemRB::Size(w, h));
	}
	
public:
	SDLTextureVideoBuffer(const Point& p, SDL_Texture* texture, Video::BufferFormat fmt, SDL_Renderer* renderer)
	: VideoBuffer(TextureRegion(texture, p)), texture(texture), renderer(renderer), inputFormat(fmt)
	{
		assert(texture);
		assert(renderer);
	}
	
	~SDLTextureVideoBuffer() {
		SDL_DestroyTexture(texture);
	}
	
	void Clear() {
		SDL_SetRenderTarget(renderer, texture);
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_TRANSPARENT);
		SDL_RenderFillRect(renderer, NULL);
	}
	
	bool RenderOnDisplay(void* display) const {
		SDL_Renderer* renderer = static_cast<SDL_Renderer*>(display);
		SDL_Rect dst = RectFromRegion(rect);
		SDL_RenderCopy(renderer, texture, NULL, &dst);
		return true;
	}
	
	void CopyPixels(const Region& bufDest, void* pixelBuf, const int* pitch = NULL, ...) {
		int sdlpitch = (pitch) ? *pitch : bufDest.w;
		SDL_Rect dest = RectFromRegion(bufDest);
		
		Uint32 fmt;
		SDL_QueryTexture(texture, &fmt, NULL, NULL, NULL);
		
		if (fmt == SDL_PIXELFORMAT_YV12) {
			va_list args;
			va_start(args, pitch);
			
			enum PLANES {Y, U, V};
			ieByte* planes[3];
			unsigned int strides[3];
			
			planes[Y] = static_cast<ieByte*>(pixelBuf);
			strides[Y] = *pitch;
			planes[U] = va_arg(args, ieByte*);
			strides[U] = *va_arg(args, int*);
			planes[V] = va_arg(args, ieByte*);
			strides[V] = *va_arg(args, int*);
			va_end(args);
			
			SDL_UpdateYUVTexture(texture, &dest, planes[Y], strides[Y], planes[U], strides[U], planes[V], strides[V]);
		} else if (fmt == SDLPixelFormatFromBufferFormat(inputFormat)) {
			SDL_UpdateTexture(texture, &dest, pixelBuf, sdlpitch);
		} else {
			// assuming the texture is a 4 byte per pixel format
			void* pixels = malloc(4 * bufDest.w * bufDest.h);
			SDL_ConvertPixels(bufDest.w, bufDest.h, inputFormat, pixelBuf, sdlpitch, fmt, pixels, bufDest.w);
			SDL_UpdateTexture(texture, &dest, pixels, bufDest.w);
			free(pixels);
		}
	}
};

}

#endif
