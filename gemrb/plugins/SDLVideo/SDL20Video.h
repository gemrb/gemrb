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

Uint32 SDLPixelFormatFromBufferFormat(Video::BufferFormat, SDL_Renderer*);
Uint32 SDLPixelFormatFromBufferFormat(Video::BufferFormat fmt, SDL_Renderer* renderer = NULL) {
	switch (fmt) {
		case Video::RGB555:
			return SDL_PIXELFORMAT_RGB555;
		case Video::RGBA8888:
			return SDL_PIXELFORMAT_RGBA8888;
		case Video::YV12:
			return SDL_PIXELFORMAT_YV12;
		case Video::RGBPAL8:
			if (renderer == NULL) {
				return SDL_PIXELFORMAT_INDEX8;
			}
			// the renderer will throw an error for such a format
			// fall-through
		case Video::DISPLAY:
			if (renderer) {
				// I looked at the SDL source code to determine that the format at index 0 is the default
				SDL_RendererInfo info;
				SDL_GetRendererInfo(renderer, &info);
				return info.texture_formats[0];
			}
			// no "display" to query
			// fall-through
		default:
			return SDL_PIXELFORMAT_UNKNOWN;
	}
}

class SDL20VideoDriver : public SDLVideoDriver {
private:
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
	void StartTextInput();
	void StopTextInput();
	bool TouchInputEnabled();

	void DrawLine(const Point& p1, const Point& p2, const Color& color);
	void DrawLines(const std::vector<Point>& points, const Color& color);

	void DrawRect(const Region& rgn, const Color& color, bool fill = true);
	
	void DrawPoint(const Point& p, const Color& color);
	void DrawPoints(const std::vector<Point>& points, const Color& color);

	void Flush();

private:
	VideoBuffer* NewVideoBuffer(const Region&, BufferFormat);

	int ProcessEvent(const SDL_Event & event);

	SDLVideoDriver::vid_buf_t* CurrentRenderBuffer();
	int UpdateRenderTarget(const Color* color = NULL);

	void DrawPoints(const std::vector<SDL_Point>& points, const SDL_Color& color);
	void DrawLines(const std::vector<SDL_Point>& points, const SDL_Color& color);

	void BlitSpriteBAMClipped(const Sprite2D* /*spr*/, const Sprite2D* /*mask*/, const Region& /*src*/, const Region& /*dst*/,
					   unsigned int /*flags*/ = 0, const Color* /*tint*/ = NULL) { assert(false); } // SDL2 does not support this
	void BlitSpriteNativeClipped(const Sprite2D* spr, const Sprite2D* mask, const SDL_Rect& src, const SDL_Rect& dst, unsigned int flags = 0, const SDL_Color* tint = NULL);
};

class SDLTextureVideoBuffer : public VideoBuffer {
	SDL_Texture* texture;
	SDL_Texture* maskLayer;
	SDL_Renderer* renderer;

	 // the format of the pixel data the client thinks we use, we may have to convert in CopyPixels()
	Uint32 inputFormat; // the SDL pixel format equivalent of the requested Video::BufferFormat
	Uint32 nativeFormat; // the SDL pixel format of the texture

	// if the inputFormat is different than the actual texture format we will allocate a buffer to handle conversion
	// this has significant memory overhead, but is much faster than dynamic allocation every frame
	void* conversionBuffer;

private:
	static Region TextureRegion(SDL_Texture* tex, const Point& p) {
		int w, h;
		SDL_QueryTexture(tex, NULL, NULL, &w, &h);
		return Region(p, ::GemRB::Size(w, h));
	}

public:
	SDLTextureVideoBuffer(const Point& p, SDL_Texture* texture, Video::BufferFormat fmt, SDL_Renderer* renderer)
	: VideoBuffer(TextureRegion(texture, p)), texture(texture), renderer(renderer), inputFormat(SDLPixelFormatFromBufferFormat(fmt, NULL))
	{
		assert(texture);
		assert(renderer);
		maskLayer = NULL;

		SDL_QueryTexture(texture, &nativeFormat, NULL, NULL, NULL);

		if (inputFormat != nativeFormat) {
			conversionBuffer = operator new(SDL_BYTESPERPIXEL(nativeFormat) * rect.w * rect.h);
		} else {
			conversionBuffer = NULL;
		}

		Clear();
	}

	~SDLTextureVideoBuffer() {
		SDL_DestroyTexture(texture);
		SDL_DestroyTexture(maskLayer);
		operator delete(conversionBuffer);
	}

	void Clear() {
		SDL_SetRenderTarget(renderer, texture);
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_TRANSPARENT);
		SDL_RenderClear(renderer);

		ClearMaskLayer();
	}

	void ClearMaskLayer() {
		if (maskLayer) {
			SDL_SetRenderTarget(renderer, maskLayer);
			SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_TRANSPARENT);
			SDL_RenderClear(renderer);
		}
	}

	bool RenderOnDisplay(void* display) const {
		SDL_Renderer* renderer = static_cast<SDL_Renderer*>(display);
		SDL_Rect dst = RectFromRegion(rect);
		SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
		int ret = SDL_RenderCopy(renderer, texture, NULL, &dst);

		if (maskLayer && ret == 0) {
			SDL_SetTextureBlendMode(maskLayer, SDL_BLENDMODE_BLEND);
			ret = SDL_RenderCopy(renderer, maskLayer, NULL, &dst);
		}

		if (ret != 0) {
			Log(ERROR, "SDLVideo", "%s", SDL_GetError());
		}
		return true;
	}

	void CopyPixels(const Region& bufDest, const void* pixelBuf, const int* pitch = NULL, ...) {
		int sdlpitch = bufDest.w * SDL_BYTESPERPIXEL(nativeFormat);
		SDL_Rect dest = RectFromRegion(bufDest);

		if (nativeFormat == SDL_PIXELFORMAT_YV12) {
			va_list args;
			va_start(args, pitch);

			enum Planes {Y, U, V};
			const ieByte* planes[3];
			unsigned int strides[3];

			planes[Y] = static_cast<const ieByte*>(pixelBuf);
			strides[Y] = *pitch;
			planes[U] = va_arg(args, ieByte*);
			strides[U] = *va_arg(args, int*);
			planes[V] = va_arg(args, ieByte*);
			strides[V] = *va_arg(args, int*);
			va_end(args);

			SDL_UpdateYUVTexture(texture, &dest, planes[Y], strides[Y], planes[V], strides[V], planes[U], strides[U]);
		} else if (nativeFormat == inputFormat) {
			SDL_UpdateTexture(texture, &dest, pixelBuf, (pitch) ? *pitch : sdlpitch);
		} else if (inputFormat == SDL_PIXELFORMAT_INDEX8) {
			// SDL_ConvertPixels doesn't support palettes... must do it ourselves
			va_list args;
			va_start(args, pitch);
			Palette* pal = va_arg(args, Palette*);
			va_end(args);

			Uint32* dst = static_cast<Uint32*>(conversionBuffer);
			SDL_PixelFormat* pxfmt = SDL_AllocFormat(nativeFormat);
			bool hasalpha = SDL_ISPIXELFORMAT_ALPHA(nativeFormat);

			const Uint8* src = static_cast<const Uint8*>(pixelBuf);
			for (int xy = 0; xy < bufDest.w * bufDest.h; ++xy) {
				const Color& c = pal->col[*src++];
				*dst++ = (c.r << pxfmt->Rshift) | (c.g << pxfmt->Gshift) | (c.b << pxfmt->Bshift) | (c.a << pxfmt->Ashift);
				if (hasalpha == false) {
					dst = (Uint32*)((Uint8*)dst - 1);
				}
			}

			SDL_FreeFormat(pxfmt);

			int ret = SDL_UpdateTexture(texture, &dest, conversionBuffer, sdlpitch);
			if (ret != 0) {
				Log(ERROR, "SDL20Video", "%s", SDL_GetError());
			}
		} else {
			int ret = SDL_ConvertPixels(bufDest.w, bufDest.h, inputFormat, pixelBuf, (pitch) ? *pitch : sdlpitch, nativeFormat, conversionBuffer, sdlpitch);
			if (ret == 0) {
				ret = SDL_UpdateTexture(texture, &dest, conversionBuffer, sdlpitch);
			}

			if (ret != 0) {
				Log(ERROR, "SDL20Video", "%s", SDL_GetError());
			}
		}
	}

	SDL_Texture* GetMaskLayer() {
		if (maskLayer == NULL)
			maskLayer = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, rect.w, rect.h);
		return maskLayer;
	}

	SDL_Texture* GetTexture() const
	{
		return texture;
	}
};

}

#endif
