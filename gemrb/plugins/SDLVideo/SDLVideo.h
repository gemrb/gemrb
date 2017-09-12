/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
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

#ifndef SDLVIDEODRIVER_H
#define SDLVIDEODRIVER_H

#include "Video.h"

#include "GUI/EventMgr.h"
#include "win32def.h"

#include <vector>
#include <SDL.h>

typedef unsigned char
#ifdef __GNUC__
	__attribute__((aligned(4)))
#endif
	Pixel;

namespace GemRB {

#if SDL_VERSION_ATLEAST(1,3,0)
#define SDL_SRCCOLORKEY SDL_TRUE
#define SDL_SRCALPHA 0
#define SDLKey SDL_Keycode
#define SDLK_SCROLLOCK SDLK_SCROLLLOCK
#define SDLK_KP1 SDLK_KP_1
#define SDLK_KP2 SDLK_KP_2
#define SDLK_KP3 SDLK_KP_3
#define SDLK_KP4 SDLK_KP_4
#define SDLK_KP6 SDLK_KP_6
#define SDLK_KP7 SDLK_KP_7
#define SDLK_KP8 SDLK_KP_8
#define SDLK_KP9 SDLK_KP_9
#else
	typedef Sint32 SDL_Keycode;
#endif

inline int GetModState(int modstate)
{
	int value = 0;
	if (modstate&KMOD_SHIFT) value |= GEM_MOD_SHIFT;
	if (modstate&KMOD_CTRL) value |= GEM_MOD_CTRL;
	if (modstate&KMOD_ALT) value |= GEM_MOD_ALT;
	return value;
}
	
inline SDL_Rect RectFromRegion(const Region& rgn)
{
	SDL_Rect rect = {Sint16(rgn.x), Sint16(rgn.y), Uint16(rgn.w), Uint16(rgn.h)};
	return rect;
}

class SDLVideoDriver : public Video {
public:
	SDLVideoDriver(void);
	virtual ~SDLVideoDriver(void);
	int Init(void);

	virtual bool SetFullscreenMode(bool set)=0;

	virtual bool ToggleGrabInput()=0;
	virtual void ShowSoftKeyboard()=0;
	virtual void HideSoftKeyboard()=0;
	virtual bool TouchInputEnabled()=0;

	void InitSpriteCover(SpriteCover* sc, int flags);
	void AddPolygonToSpriteCover(SpriteCover* sc, Wall_Polygon* poly);
	void DestroySpriteCover(SpriteCover* sc);

	virtual Sprite2D* CreateSprite(int w, int h, int bpp, ieDword rMask,
		ieDword gMask, ieDword bMask, ieDword aMask, void* pixels,
		bool cK = false, int index = 0);
	virtual Sprite2D* CreateSprite8(int w, int h, void* pixels,
							Palette* palette, bool cK, int index);
	virtual Sprite2D* CreatePalettedSprite(int w, int h, int bpp, void* pixels,
								   Color* palette, bool cK = false, int index = 0);

	virtual void BlitTile(const Sprite2D* spr, const Sprite2D* mask, int x, int y,
						  const Region* clip, unsigned int flags);
	virtual void BlitSprite(const Sprite2D* spr, const Region& src, const Region& dst, Palette* pal = NULL);
	virtual void BlitGameSprite(const Sprite2D* spr, int x, int y, unsigned int flags, Color tint,
								SpriteCover* cover, Palette *palette = NULL, const Region* clip = NULL);

	/** This function Draws the Border of a Rectangle as described by the Region parameter. The Color used to draw the rectangle is passes via the Color parameter. */
	virtual void DrawRect(const Region& rgn, const Color& color, bool fill = true);
	/** This functions Draws a Circle */
	void SetPixel(const Point&, const Color& color);
	/** Gets the pixel of the backbuffer surface */
	void GetPixel(short x, short y, Color& color);
	virtual void DrawCircle(short cx, short cy, unsigned short r, const Color& color);
	/** This functions Draws an Ellipse Segment */
	void DrawEllipseSegment(short cx, short cy, unsigned short xr, unsigned short yr, const Color& color,
		double anglefrom, double angleto, bool drawlines = true);
	/** This functions Draws an Ellipse */
	virtual void DrawEllipse(short cx, short cy, unsigned short xr, unsigned short yr, const Color& color);
	/** This function Draws a Polygon on the Screen */
	virtual void DrawPolyline(Gem_Polygon* poly, const Color& color, bool fill = false);
	virtual void DrawHLine(short x1, short y, short x2, const Color& color);
	virtual void DrawVLine(short x, short y1, short y2, const Color& color);
	virtual void DrawLine(short x1, short y1, short x2, short y2, const Color& color);
	/** Blits a Sprite filling the Region */
	void BlitTiled(Region rgn, const Sprite2D* img);

	void SetFadeColor(int r, int g, int b);
	void SetFadePercent(int percent);

protected:
	inline SDL_Surface* CurrentSurfaceBuffer();
	void BlitSurfaceClipped(SDL_Surface*, const Region& src, const Region& dst);
	virtual bool SetSurfaceAlpha(SDL_Surface* surface, unsigned short alpha)=0;
	void SetPixel(short x, short y, const Color& color);
	int PollEvents();
	/* used to process the SDL events dequeued by PollEvents or an arbitraty event from another source.*/
	virtual int ProcessEvent(const SDL_Event & event);
	void Wait(int w) { SDL_Delay(w); }

public:
	// static functions for manipulating surfaces
	static void SetSurfacePalette(SDL_Surface* surf, SDL_Color* pal, int numcolors = 256);
	static void SetSurfacePixel(SDL_Surface* surf, short x, short y, const Color& color);
	static void GetSurfacePixel(SDL_Surface* surf, short x, short y, Color& c);
};

class SDLSurfaceVideoBuffer : public VideoBuffer {
	SDL_Surface* buffer;

public:
	SDLSurfaceVideoBuffer(SDL_Surface* surf, const Point& p)
	: VideoBuffer(Region(p, ::GemRB::Size(surf->w, surf->h)))
	{
		assert(surf);
		buffer = surf;

		Clear();
	}

	~SDLSurfaceVideoBuffer() {
		SDL_FreeSurface(buffer);
	}

	void Clear() {
		Uint32 ck = 0;
#if SDL_VERSION_ATLEAST(1,3,0)
		if (SDL_GetColorKey(buffer, &ck) == 0) {
#else
		ck = buffer->format->colorkey;
		if (ck) {
#endif
			SDL_FillRect(buffer, NULL, ck);
		} else {
			SDL_FillRect(buffer, NULL, SDL_MapRGBA(buffer->format, 0, 0, 0, SDL_ALPHA_TRANSPARENT));
		}
	}

	SDL_Surface* Surface() {
		return buffer;
	}

	bool RenderOnDisplay(void* display) {
		SDL_Surface* sdldisplay = static_cast<SDL_Surface*>(display);
		SDL_Rect dst = RectFromRegion(rect);
		SDL_BlitSurface( buffer, NULL, sdldisplay, &dst );
		return true;
	}

	void CopyPixels(const Region& bufDest, void* pixelBuf, const int* pitch = NULL, ...) {
		SDL_Surface* sprite = NULL;

		// FIXME: this shold support everything from Video::BufferFormat
		if (buffer->format->BitsPerPixel == 16) { // RGB565
			sprite = SDL_CreateRGBSurfaceFrom( pixelBuf, bufDest.w, bufDest.h, 16, 2 * bufDest.w, 0x7C00, 0x03E0, 0x001F, 0 );
		} else { // RGBPAL8
			sprite = SDL_CreateRGBSurfaceFrom( pixelBuf, bufDest.w, bufDest.h, 8, bufDest.w, 0, 0, 0, 0 );
			va_list args;
			va_start(args, pitch);
			ieByte* pal = va_arg(args, ieByte*);
			for (int i = 0; i < 256; i++) {
				sprite->format->palette->colors[i].r = ( *pal++ ) << 2;
				sprite->format->palette->colors[i].g = ( *pal++ ) << 2;
				sprite->format->palette->colors[i].b = ( *pal++ ) << 2;
#if SDL_VERSION_ATLEAST(1,3,0)
				sprite->format->palette->colors[i].a = 0;
#else
				sprite->format->palette->colors[i].unused = 0;
#endif
			}
			va_end(args);
		}

		SDL_Rect dst = RectFromRegion(bufDest);
		SDL_BlitSurface(sprite, NULL, buffer, &dst);
		SDL_FreeSurface(sprite);
	}
};

}

#endif
