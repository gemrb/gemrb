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


#include "SDLSurfaceDrawing.h"
#include "SDLVideo.h"

#if USE_OPENGL_BACKEND
	#include "GLSLProgram.h"
#else
class GLSLProgram {};
#endif

namespace GemRB {

Uint32 SDLPixelFormatFromBufferFormat(Video::BufferFormat, SDL_Renderer*);
Uint32 SDLPixelFormatFromBufferFormat(Video::BufferFormat fmt, SDL_Renderer* renderer = NULL)
{
	switch (fmt) {
		case Video::BufferFormat::RGB555:
			return SDL_PIXELFORMAT_RGB555;
		case Video::BufferFormat::RGBA8888:
			return SDL_PIXELFORMAT_RGBA8888;
		case Video::BufferFormat::YV12:
			return SDL_PIXELFORMAT_YV12;
		case Video::BufferFormat::RGBPAL8:
			if (renderer == NULL) {
				return SDL_PIXELFORMAT_INDEX8;
			}
			// the renderer will throw an error for such a format
			// fall-through
		case Video::BufferFormat::DISPLAY:
			// fall-through
		case Video::BufferFormat::DISPLAY_ALPHA:
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

class SDLTextureVideoBuffer : public VideoBuffer {
	SDL_Texture* texture;
	SDL_Renderer* renderer;

	// the format of the pixel data the client thinks we use, we may have to convert in CopyPixels()
	Uint32 inputFormat; // the SDL pixel format equivalent of the requested Video::BufferFormat
	Uint32 nativeFormat; // the SDL pixel format of the texture

	// if the inputFormat is different than the actual texture format we will allocate a buffer to handle conversion
	// this has significant memory overhead, but is much faster than dynamic allocation every frame
	// this is also used for rendering stencils
	SDL_Surface* conversionBuffer = nullptr;

private:
	static Region TextureRegion(SDL_Texture* tex, const Point& p)
	{
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

		int access;
		SDL_QueryTexture(texture, &nativeFormat, &access, NULL, NULL);

		if (inputFormat != nativeFormat || access == SDL_TEXTUREACCESS_STREAMING) {
			conversionBuffer = SDL_CreateRGBSurfaceWithFormat(0, rect.w, rect.h, SDL_BITSPERPIXEL(nativeFormat), nativeFormat);
		}

		Clear();
	}

	~SDLTextureVideoBuffer() override
	{
		SDL_DestroyTexture(texture);
		SDL_FreeSurface(conversionBuffer);
	}

	void Clear() override
	{
		SDL_SetRenderTarget(renderer, texture);
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_TRANSPARENT);
#if SDL_COMPILEDVERSION == SDL_VERSIONNUM(2, 0, 10)
		/**
		 * See GH issue #410. In some SDL2 backends of this version, a clear
		 * runs over an outdated state of the clipping settings. This can
		 * be overcome by a harmless draw command right before.
		 */
		SDL_RenderDrawPoint(renderer, 0, 0);
#endif
		SDL_RenderClear(renderer);
	}

	void Clear(const SDL_Rect& rgn)
	{
		SDL_SetRenderTarget(renderer, texture);
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_TRANSPARENT);
		SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
		SDL_RenderFillRect(renderer, &rgn);
	}

	void Clear(const Region& rgn) override
	{
		return Clear(RectFromRegion(rgn));
	}

	bool RenderOnDisplay(void* display) const override
	{
		SDL_Renderer* targetRenderer = static_cast<SDL_Renderer*>(display);
		SDL_Rect dst = RectFromRegion(rect);
		SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
		int ret = SDL_RenderCopy(targetRenderer, texture, nullptr, &dst);
		if (ret != 0) {
			Log(ERROR, "SDLVideo", "{}", SDL_GetError());
		}
		return true;
	}

	void CopyPixels(const Region& bufDest, const void* pixelBuf, const int* pitch = NULL, ...) override
	{
		int sdlpitch = bufDest.w * SDL_BYTESPERPIXEL(nativeFormat);
		SDL_Rect dest = RectFromRegion(bufDest);

		if (nativeFormat == SDL_PIXELFORMAT_YV12) {
			va_list args;
			va_start(args, pitch);

			enum Planes { Y,
				      U,
				      V };
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
			SDL_UpdateTexture(texture, &dest, pixelBuf, pitch ? *pitch : sdlpitch);
		} else if (inputFormat == SDL_PIXELFORMAT_INDEX8) {
			// SDL_ConvertPixels doesn't support palettes... must do it ourselves
			va_list args;
			va_start(args, pitch);
			const Palette* pal = va_arg(args, Palette*);
			va_end(args);

			Uint32* dst = static_cast<Uint32*>(conversionBuffer->pixels);
			SDL_PixelFormat* pxfmt = SDL_AllocFormat(nativeFormat);
			bool hasalpha = SDL_ISPIXELFORMAT_ALPHA(nativeFormat);

			const Uint8* src = static_cast<const Uint8*>(pixelBuf);
			for (int xy = 0; xy < bufDest.w * bufDest.h; ++xy) {
				const Color& c = pal->GetColorAt(*src++);
				*dst++ = (c.r << pxfmt->Rshift) | (c.g << pxfmt->Gshift) | (c.b << pxfmt->Bshift) | (c.a << pxfmt->Ashift);
				if (hasalpha == false) {
					dst = (Uint32*) ((Uint8*) dst - 1);
				}
			}

			SDL_FreeFormat(pxfmt);

			int ret = SDL_UpdateTexture(texture, &dest, conversionBuffer->pixels, sdlpitch);
			if (ret != 0) {
				Log(ERROR, "SDL20Video", "{}", SDL_GetError());
			}
		} else {
			int ret = SDL_ConvertPixels(bufDest.w, bufDest.h, inputFormat, pixelBuf, pitch ? *pitch : sdlpitch, nativeFormat, conversionBuffer->pixels, sdlpitch);
			if (ret == 0) {
				ret = SDL_UpdateTexture(texture, &dest, conversionBuffer->pixels, sdlpitch);
			}

			if (ret != 0) {
				Log(ERROR, "SDL20Video", "{}", SDL_GetError());
			}
		}
	}

	SDL_Texture* GetTexture() const
	{
		return texture;
	}
};

class SDL20VideoDriver : public SDLVideoDriver {
private:
	SDL_Window* window;
	SDL_Renderer* renderer;
	int sdl2_runtime_version;

	SDL_BlendMode stencilAlphaBlender;
	SDL_BlendMode oneMinusDstBlender;
	SDL_BlendMode dstBlender;
	SDL_BlendMode srcBlender;

	SDL_GameController* gameController = nullptr;

	GLSLProgram* blitRGBAShader = nullptr;
	float brightness = 1.0;
	float contrast = 1.0;
	Size customFullscreenSize;

public:
	SDL20VideoDriver() noexcept;
	~SDL20VideoDriver() noexcept override;

	int Init() override;

	void SetWindowTitle(const char* title) override { SDL_SetWindowTitle(window, title); };

	Holder<Sprite2D> GetScreenshot(Region r, const VideoBufferPtr& buf = nullptr) override;
	bool SetFullscreenMode(bool set) override;
	void SetGamma(int brightness, int contrast) override;
	bool ToggleGrabInput() override;
	void CaptureMouse(bool enabled) override;

	void StartTextInput() override;
	void StopTextInput() override;
	bool InTextInput() override;

	bool TouchInputEnabled() override;
	bool CanDrawRawGeometry() const override;

	void BlitVideoBuffer(const VideoBufferPtr& buf, const Point& p, BlitFlags flags,
			     Color tint = Color()) override;
	void BlitVideoBufferFully(const VideoBufferPtr& buf, BlitFlags flags,
				  Color tint = Color()) override;

	void DrawRawGeometry(const std::vector<float>& vertices, const std::vector<Color>& colors, BlitFlags blitFlags) override;

private:
	VideoBuffer* NewVideoBuffer(const Region&, BufferFormat) override;

	int ProcessEvent(const SDL_Event& event) override;

	int CreateSDLDisplay(const char* title, bool vsync) override;
	void SwapBuffers(VideoBuffers& buffers) override;

	SDLVideoDriver::vid_buf_t* ScratchBuffer() const override;
	SDLVideoDriver::vid_buf_t* CurrentRenderBuffer() const override;
	SDLVideoDriver::vid_buf_t* CurrentStencilBuffer() const override;

	void BeginCustomRendering(SDL_Texture*);
	int UpdateRenderTarget(const Color* color = NULL, BlitFlags flags = BlitFlags::NONE);

	void DrawSDLPoints(const std::vector<SDL_Point>& points, const SDL_Color& color, BlitFlags flags = BlitFlags::NONE) override;
	void DrawSDLLines(const std::vector<SDL_Point>& points, const SDL_Color& color, BlitFlags flags = BlitFlags::NONE);

	void DrawLineImp(const BasePoint& start, const BasePoint& end, const Color& color, BlitFlags flags) override;
	void DrawLinesImp(const std::vector<Point>& points, const Color& color, BlitFlags flags) override;

	void DrawRectImp(const Region& rgn, const Color& color, bool fill, BlitFlags flags) override;

	void DrawPointImp(const BasePoint& p, const Color& color, BlitFlags flags) override;
	void DrawPointsImp(const std::vector<BasePoint>& points, const Color& color, BlitFlags flags) override;

	void DrawPolygonImp(const Gem_Polygon* poly, const Point& origin, const Color& color, bool fill, BlitFlags flags) override;

	void BlitSpriteRLEClipped(const Holder<Sprite2D>& /*spr*/, const Region& /*src*/, const Region& /*dst*/,
				  BlitFlags /*flags*/ = BlitFlags::NONE, const Color* /*tint*/ = NULL) override { assert(false); } // SDL2 does not support this
	void BlitSpriteNativeClipped(const sprite_t* spr, const Region& src, const Region& dst,
				     BlitFlags flags = BlitFlags::NONE, const SDL_Color* tint = NULL) override;
	void BlitSpriteNativeClipped(SDL_Texture* spr, const Region& src, const Region& dst, BlitFlags flags = BlitFlags::NONE, const SDL_Color* tint = NULL);

	int RenderCopyShaded(SDL_Texture*, const SDL_Rect* srcrect, const SDL_Rect* dstrect, BlitFlags flags, const SDL_Color* = nullptr);
	void SetTextureBlendMode(SDL_Texture* texture, BlitFlags flags) const;

	int GetTouchFingers(TouchEvent::Finger (&fingers)[FINGER_MAX], SDL_TouchID device) const;

	void CalculateCustomFullscreen(const SDL_DisplayMode* mode);
};

}

#endif
