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

public:
	SDL12VideoDriver(void);
	
	int Init(void);
	int CreateDriverDisplay(const Size&, int bpp, const char* title);
	Sprite2D* GetScreenshot( Region r );

	bool SetFullscreenMode(bool set);

	bool ToggleGrabInput();
	void ShowSoftKeyboard();
	void HideSoftKeyboard();
	void SetGamma(int brightness, int contrast);

private:
	void SwapBuffers(VideoBuffers&);
	VideoBuffer* NewVideoBuffer(const Region& rgn, BufferFormat fmt);
	bool SetSurfaceAlpha(SDL_Surface* surface, unsigned short alpha);

	int ProcessEvent(const SDL_Event & event);
};


class SDLOverlayVideoBuffer : public SDLSurfaceVideoBuffer {
	SDL_Overlay* overlay;
	Point renderPos;

public:
	SDLOverlayVideoBuffer(SDL_Surface* buffer, const Point& p, SDL_Overlay* overlay)
	: SDLSurfaceVideoBuffer(buffer, p)
	{
		assert(overlay);
		this->overlay = overlay;
	}

	~SDLOverlayVideoBuffer() {
		SDL_FreeYUVOverlay(overlay);
	}

	void Clear() {}

	class Size Size() {
		return GemRB::Size(overlay->w, overlay->h);
	}

	void RenderOnDisplay(SDL_Surface* /*display*/) {
		//SDLSurfaceVideoBuffer::RenderOnDisplay(display); // probably does nothing

		SDL_Rect dest = {origin.x, origin.y, overlay->w, overlay->h};
		SDL_DisplayYUVOverlay(overlay, &dest);
	}

	void CopyPixels(const Region& bufDest, void* pixelBuf, const int* pitch = NULL, ...) {
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

		SDL_LockYUVOverlay(overlay);
		for (unsigned int plane = 0; plane < 3; plane++) {
			unsigned char *data = planes[plane];
			unsigned int size = overlay->pitches[plane];
			if (strides[plane] < size) {
				size = strides[plane];
			}
			unsigned int srcoffset = 0, destoffset = 0;
			for (int i = 0; i < ((plane == 0) ? bufDest.h : (bufDest.h / 2)); i++) {
				memcpy(overlay->pixels[plane] + destoffset,
					   data + srcoffset, size);
				srcoffset += strides[plane];
				destoffset += overlay->pitches[plane];
			}
		}
		SDL_UnlockYUVOverlay(overlay);
		renderPos = bufDest.Origin();
	}
};

}

#endif
