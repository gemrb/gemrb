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

inline int GetModState(int modstate)
{
	int value = 0;
	if (modstate&KMOD_SHIFT) value |= GEM_MOD_SHIFT;
	if (modstate&KMOD_CTRL) value |= GEM_MOD_CTRL;
	if (modstate&KMOD_ALT) value |= GEM_MOD_ALT;
	return value;
}

class SDLVideoDriver : public Video {
protected:
	SDL_Surface* disp;
	SDL_Surface* backBuf;
	SDL_Surface* extra;
	std::vector< Region> upd;//Regions of the Screen to Update in the next SwapBuffer operation.
	unsigned long lastTime;
	unsigned long lastMouseMoveTime;
	unsigned long lastMouseDownTime;

	SDL_Rect subtitleregion_sdl;  //we probably have the same stuff, twice
	char *subtitletext;
	ieDword subtitlestrref;
public:
	SDLVideoDriver(void);
	virtual ~SDLVideoDriver(void);
	int Init(void);
	virtual int PollEvents();

	virtual int CreateDisplay(int width, int height, int bpp, bool fullscreen, const char* title)=0;
	virtual bool SetFullscreenMode(bool set)=0;
	virtual int SwapBuffers(void);

	virtual bool ToggleGrabInput()=0;
	short GetWidth() { return ( disp ? disp->w : 0 ); }
	short GetHeight() { return ( disp ? disp->h : 0 ); }

	virtual void ShowSoftKeyboard()=0;
	virtual void HideSoftKeyboard()=0;
	
	void InitSpriteCover(SpriteCover* sc, int flags);
	void AddPolygonToSpriteCover(SpriteCover* sc, Wall_Polygon* poly);
	void DestroySpriteCover(SpriteCover* sc);

	void MouseMovement(int x, int y);
	void ClickMouse(unsigned int button);
	void MouseClickEvent(SDL_EventType type, Uint8 button);
	Sprite2D* CreateSprite(int w, int h, int bpp, ieDword rMask,
		ieDword gMask, ieDword bMask, ieDword aMask, void* pixels,
		bool cK = false, int index = 0);
	Sprite2D* CreateSprite8(int w, int h, void* pixels,
							Palette* palette, bool cK, int index);
	Sprite2D* CreatePalettedSprite(int w, int h, int bpp, void* pixels,
								   Color* palette, bool cK = false, int index = 0);
	bool SupportsBAMSprites() { return true; }
	void BlitTile(const Sprite2D* spr, const Sprite2D* mask, int x, int y, const Region* clip, unsigned int flags);
	void BlitSprite(const Sprite2D* spr, int x, int y, bool anchor = false,
					const Region* clip = NULL, Palette* palette = NULL);
	void BlitGameSprite(const Sprite2D* spr, int x, int y,
		unsigned int flags, Color tint,
		SpriteCover* cover, Palette *palette = NULL,
		const Region* clip = NULL, bool anchor = false);
	void SetCursor(Sprite2D* up, Sprite2D* down);
	void SetDragCursor(Sprite2D* drag);
	Sprite2D* GetScreenshot( Region r );
	/** This function Draws the Border of a Rectangle as described by the Region parameter. The Color used to draw the rectangle is passes via the Color parameter. */
	void DrawRect(const Region& rgn, const Color& color, bool fill = true, bool clipped = false);
	void DrawRectSprite(const Region& rgn, const Color& color, const Sprite2D* sprite);
	/** This functions Draws a Circle */
	void SetPixel(short x, short y, const Color& color, bool clipped = true);
	/** Gets the pixel of the backbuffer surface */
	void GetPixel(short x, short y, Color& color);
	void DrawCircle(short cx, short cy, unsigned short r, const Color& color, bool clipped = true);
	/** This functions Draws an Ellipse Segment */
	void DrawEllipseSegment(short cx, short cy, unsigned short xr, unsigned short yr, const Color& color,
		double anglefrom, double angleto, bool drawlines = true, bool clipped = true);
	/** This functions Draws an Ellipse */
	void DrawEllipse(short cx, short cy, unsigned short xr, unsigned short yr,
		const Color& color, bool clipped = true);
	/** This function Draws a Polygon on the Screen */
	void DrawPolyline(Gem_Polygon* poly, const Color& color, bool fill = false);
	void DrawHLine(short x1, short y, short x2, const Color& color, bool clipped = false);
	void DrawVLine(short x, short y1, short y2, const Color& color, bool clipped = false);
	void DrawLine(short x1, short y1, short x2, short y2, const Color& color, bool clipped = false);
	/** Blits a Sprite filling the Region */
	void BlitTiled(Region rgn, const Sprite2D* img, bool anchor = false);
	/** Flips sprite vertically */
	Sprite2D *MirrorSpriteVertical(const Sprite2D *sprite, bool MirrorAnchor);
	/** Flips sprite horizontally */
	Sprite2D *MirrorSpriteHorizontal(const Sprite2D *sprite, bool MirrorAnchor);
	/** Set Clip Rect */
	void SetClipRect(const Region* clip);
	/** Get Clip Rect */
	void GetClipRect(Region& clip);
	/** Convers a Screen Coordinate to a Game Coordinate */

	void ConvertToGame(short& x, short& y)
	{
		x += Viewport.x;
		y += Viewport.y;
	}

	void ConvertToScreen(short&x, short& y)
	{
		x -= Viewport.x;
		y -= Viewport.y;
	}

	void SetFadeColor(int r, int g, int b);
	void SetFadePercent(int percent);

	virtual void InitMovieScreen(int &w, int &h, bool yuv=false)=0;
	virtual void DestroyMovieScreen() = 0;
	virtual void showFrame(unsigned char* buf, unsigned int bufw,
							unsigned int bufh, unsigned int sx, unsigned int sy, unsigned int w,
							unsigned int h, unsigned int dstx, unsigned int dsty, int truecolor,
							unsigned char *palette, ieDword strRef)=0;
	void showYUVFrame(unsigned char** buf, unsigned int *strides,
							unsigned int bufw, unsigned int bufh,
							unsigned int w, unsigned int h,
							unsigned int dstx, unsigned int dsty,
							ieDword titleref)=0;
	int PollMovieEvents();

protected:
	void DrawMovieSubtitle(ieDword strRef);
	virtual bool SetSurfaceAlpha(SDL_Surface* surface, unsigned short alpha)=0;
	/* used to process the SDL events dequeued by PollEvents or an arbitraty event from another source.*/
	virtual int ProcessEvent(const SDL_Event & event);

public:
	// static functions for manipulating surfaces
	static void SetSurfacePalette(SDL_Surface* surf, SDL_Color* pal, int numcolors = 256);
	static void SetSurfacePixel(SDL_Surface* surf, short x, short y, const Color& color);
	static void GetSurfacePixel(SDL_Surface* surf, short x, short y, Color& c);
};

}

#endif
