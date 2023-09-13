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

#ifndef SDL12VIDEODRIVER_H
#define SDL12VIDEODRIVER_H

#include "SDLVideo.h"

namespace GemRB {

class SDL12VideoDriver : public SDLVideoDriver {
private:
	SDL_Surface* disp;
	bool inTextInput;
	SDL_Joystick* gameController = nullptr;
	DPadSoftKeyboard dPadSoftKeyboard;
	bool vsyncRequest;

public:
	SDL12VideoDriver() noexcept;
	~SDL12VideoDriver() noexcept override;
	
	int Init(void) override;
	void SetWindowTitle(const char *title) override { SDL_WM_SetCaption(title, 0); };

	Holder<Sprite2D> GetScreenshot( Region r, const VideoBufferPtr& buf = nullptr ) override;

	bool SetFullscreenMode(bool set) override;

	bool ToggleGrabInput() override;
	void CaptureMouse(bool /*enabled*/) override {};

	void StartTextInput() override;
	void StopTextInput() override;
	bool InTextInput() override;

	bool TouchInputEnabled() override;
	void SetGamma(int brightness, int contrast) override;

	void BlitVideoBuffer(const VideoBufferPtr& buf, const Point& p, BlitFlags flags,
						 Color tint = Color()) override;

private:
	VideoBuffer* NewVideoBuffer(const Region& rgn, BufferFormat fmt) override;

	int CreateSDLDisplay(const char* title, bool vsync) override;
	void SwapBuffers(VideoBuffers&) override;
	int GetDisplayRefreshRate() const override;
	int GetVirtualRefreshCap() const override;

	SDLVideoDriver::vid_buf_t* ScratchBuffer() const override;
	SDLVideoDriver::vid_buf_t* CurrentRenderBuffer() const override;
	SDLVideoDriver::vid_buf_t* CurrentStencilBuffer() const override;
	
	IAlphaIterator* StencilIterator(BlitFlags flags, const Region& dst) const;

	int ProcessEvent(const SDL_Event & event) override;

	void BlitSpriteRLEClipped(const Holder<Sprite2D>& spr, const Region& src, const Region& dst,
							  BlitFlags flags = BlitFlags::NONE, const Color* tint = NULL) override;
	void BlitSpriteNativeClipped(const sprite_t* spr, const Region& src, const Region& dst,
								 BlitFlags flags = BlitFlags::NONE, const SDL_Color* tint = NULL) override;

	void BlitSpriteNativeClipped(const sprite_t* spr, const Region& src, const Region& dst, BlitFlags flags, Color tint);
	void BlitSpriteNativeClipped(SDL_Surface* surf, SDL_Rect* src, SDL_Rect* dst, BlitFlags flags, Color tint);
	void BlitWithPipeline(SDLPixelIterator& src, SDLPixelIterator& dst, IAlphaIterator* maskit, BlitFlags flags, Color tint);

	void DrawSDLPoints(const std::vector<SDL_Point>& points, const SDL_Color& color, BlitFlags flags) override;

	void DrawLineImp(const Point& p1, const Point& p2, const Color& color, BlitFlags flags) override;
	void DrawLinesImp(const std::vector<Point>& points, const Color& color, BlitFlags flags) override;

	void DrawRectImp(const Region& rgn, const Color& color, bool fill, BlitFlags flags) override;

	void DrawPointImp(const Point& p, const Color& color, BlitFlags flags) override;
	void DrawPointsImp(const std::vector<Point>& points, const Color& color, BlitFlags flags) override;

	void DrawPolygonImp(const Gem_Polygon* poly, const Point& origin, const Color& color, bool fill, BlitFlags flags) override;
};

class SDLSurfaceVideoBuffer : public VideoBuffer {
	SDL_Surface* buffer;

public:
	SDLSurfaceVideoBuffer(SDL_Surface* surf, const Point& p)
	: VideoBuffer(Region(p, ::GemRB::Size(surf->w, surf->h)))
	{
		assert(surf);
		buffer = surf;

		VideoBuffer::Clear();
	}

	~SDLSurfaceVideoBuffer() override {
		SDL_FreeSurface(buffer);
	}

	void Clear(const Region& rgn) override {
		SDL_Rect r = RectFromRegion(rgn);
		if (buffer->flags & SDL_SRCCOLORKEY) {
			SDL_FillRect(buffer, &r, buffer->format->colorkey);
		} else {
			SDL_FillRect(buffer, &r, 0);
		}
	}

	SDL_Surface* Surface() {
		return buffer;
	}

	bool RenderOnDisplay(void* display) const override {
		SDL_Surface* sdldisplay = static_cast<SDL_Surface*>(display);
		SDL_Rect dst = RectFromRegion(rect);
		SDL_BlitSurface( buffer, NULL, sdldisplay, &dst );
		return true;
	}

	void CopyPixels(const Region& bufDest, const void* pixelBuf, const int* pitch = NULL, ...) override {
		SDL_Surface* sprite = NULL;

		// we can safely const_cast pixelBuf because the surface is destroyed before return and we dont alter it

		// FIXME: this shold support everything from Video::BufferFormat
		if (buffer->format->BitsPerPixel == 16) { // RGB555
			sprite = SDL_CreateRGBSurfaceFrom( const_cast<void*>(pixelBuf), bufDest.w, bufDest.h, 16, 2 * bufDest.w, 0x7C00, 0x03E0, 0x001F, 0 );
		} else { // RGBPAL8
			sprite = SDL_CreateRGBSurfaceFrom( const_cast<void*>(pixelBuf), bufDest.w, bufDest.h, 8, bufDest.w, 0, 0, 0, 0 );
			va_list args;
			va_start(args, pitch);
			Palette *pal = va_arg(args, Palette *);
			memcpy(sprite->format->palette->colors, pal->col, sprite->format->palette->ncolors * 4);
			va_end(args);
		}

		SDL_Rect dst = RectFromRegion(bufDest);
		SDL_BlitSurface(sprite, NULL, buffer, &dst);
		SDL_FreeSurface(sprite);
	}
};

class SDLOverlayVideoBuffer : public VideoBuffer {
	SDL_Overlay* overlay;
	Point renderPos;
	mutable bool changed;

public:
	SDLOverlayVideoBuffer(const Point& p, SDL_Overlay* overlay)
	: VideoBuffer(Region(p, ::GemRB::Size(overlay->w, overlay->h)))
	{
		assert(overlay);
		this->overlay = overlay;
		changed = false;
	}

	~SDLOverlayVideoBuffer() override {
		SDL_FreeYUVOverlay(overlay);
	}

	void Clear(const Region&) override {}

	bool RenderOnDisplay(void* /*display*/) const override {
		if (changed) {
			SDL_Rect dest = RectFromRegion(rect);
			SDL_DisplayYUVOverlay(overlay, &dest);
			changed = false;
			
			// IMPORTANT: if we ever wanted to combine rendering of overlay buffers with other buffers
			// we would need to blit the result back to the display buffer
			// I'm commenting it out because we currently only use these overlays for video
			// and we need all the CPU we can get for that
			// additionally, the changed flag probably won't work at that point
			
			
			//SDL_Surface* sdldisplay = static_cast<SDL_Surface*>(display);
			//SDL_Surface* sdl_disp = SDL_GetVideoSurface();
			//SDL_LowerBlit(sdl_disp, &dest, sdldisplay, &dest);
		}
		return false;
	}

	void CopyPixels(const Region& bufDest, const void* pixelBuf, const int* pitch = NULL, ...) override {
		va_list args;
		va_start(args, pitch);

		enum {Y, U, V};
		const ieByte* planes[3];
		unsigned int strides[3];

		planes[Y] = static_cast<const ieByte*>(pixelBuf);
		strides[Y] = *pitch;
		planes[U] = va_arg(args, ieByte*);
		strides[U] = *va_arg(args, int*);
		planes[V] = va_arg(args, ieByte*);
		strides[V] = *va_arg(args, int*);

		va_end(args);

		SDL_LockYUVOverlay(overlay);
		for (unsigned int plane = 0; plane < 3; plane++) {
			const unsigned char *data = planes[plane];
			unsigned int size = overlay->pitches[plane];
			if (strides[plane] < size) {
				size = strides[plane];
			}
			unsigned int srcoffset = 0;
			unsigned int destoffset = 0;
			for (int i = 0; i < ((plane == 0) ? bufDest.h : (bufDest.h / 2)); i++) {
				memcpy(overlay->pixels[plane] + destoffset,
					   data + srcoffset, size);
				srcoffset += strides[plane];
				destoffset += overlay->pitches[plane];
			}
		}
		SDL_UnlockYUVOverlay(overlay);
		renderPos = bufDest.origin;
		changed = true;
	}
};

}

#endif
