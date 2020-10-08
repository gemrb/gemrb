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

#include "SDLSurfaceDrawing.h"
#include "SDLSurfaceSprite2D.h"

#include <vector>

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

class SDLVideoDriver : public Video {
public:
	SDLVideoDriver(void);
	virtual ~SDLVideoDriver(void);
	int Init(void) override;

	Sprite2D* CreateSprite(const Region& rgn, int bpp, ieDword rMask,
		ieDword gMask, ieDword bMask, ieDword aMask, void* pixels,
		bool cK = false, int index = 0) override;
	Sprite2D* CreateSprite8(const Region& rgn, void* pixels,
							PaletteHolder palette, bool cK, int index) override;
	Sprite2D* CreatePalettedSprite(const Region& rgn, int bpp, void* pixels,
								   Color* palette, bool cK = false, int index = 0) override;

	void BlitTile(const Sprite2D* spr, int x, int y, const Region* clip,
						  uint32_t flags, const Color* tint = NULL) override;
	void BlitSprite(const Sprite2D* spr, const Region& src, Region dst) override;
	void BlitGameSprite(const Sprite2D* spr, int x, int y, uint32_t flags, Color tint,
								const Region* clip = NULL) override;

	/** Blits a Sprite filling the Region */
	void BlitTiled(Region rgn, const Sprite2D* img);

protected:
#if SDL_VERSION_ATLEAST(1,3,0)
	typedef SDL_Texture vid_buf_t;
	typedef SDLTextureSprite2D sprite_t;
#else
	typedef SDL_Surface vid_buf_t;
	typedef SDLSurfaceSprite2D sprite_t;
	typedef Point SDL_Point;
#endif

	virtual inline vid_buf_t* CurrentRenderBuffer() const=0;
	virtual inline vid_buf_t* CurrentStencilBuffer() const=0;
	Region CurrentRenderClip() const;
	
	void RenderSpriteVersion(const SDLSurfaceSprite2D* spr, uint32_t renderflags, const Color* = NULL);

	virtual void BlitSpriteBAMClipped(const Sprite2D* spr, const Region& src, const Region& dst, uint32_t flags = 0, const Color* tint = NULL)=0;
	virtual void BlitSpriteNativeClipped(const sprite_t* spr, const SDL_Rect& src, const SDL_Rect& dst, uint32_t flags = 0, const SDL_Color* tint = NULL)=0;
	void BlitSpriteClipped(const Sprite2D* spr, Region src, const Region& dst, uint32_t flags = 0, const Color* tint = NULL);

	int PollEvents() override;
	/* used to process the SDL events dequeued by PollEvents or an arbitraty event from another source.*/
	virtual int ProcessEvent(const SDL_Event & event);
	void Wait(unsigned long w) override { SDL_Delay(w); }

private:
	virtual void DrawSDLPoints(const std::vector<SDL_Point>& points, const SDL_Color& color, uint32_t flags = 0)=0;

	void DrawCircleImp(const Point& origin, unsigned short r, const Color& color, uint32_t flags) override;
	void DrawEllipseSegmentImp(const Point& origin, unsigned short xr, unsigned short yr, const Color& color,
							   double anglefrom, double angleto, bool drawlines, uint32_t flags) override;
	void DrawEllipseImp(const Point& origin, unsigned short xr, unsigned short yr, const Color& color, uint32_t flags) override;

public:
	// static functions for manipulating surfaces
	static int SetSurfacePalette(SDL_Surface* surf, const SDL_Color* pal, int numcolors = 256);
};

}

#endif
