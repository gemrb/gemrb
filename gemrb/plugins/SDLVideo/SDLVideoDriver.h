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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/SDLVideo/SDLVideoDriver.h,v 1.30 2004/04/28 12:52:13 edheldil Exp $
 *
 */

#ifndef SDLVIDEODRIVER_H
#define SDLVIDEODRIVER_H

#include "../Core/Video.h"

#include <SDL.h>


class SDLVideoDriver : public Video {
private:
	SDL_Surface* disp;
	SDL_Surface* backBuf;
	SDL_Surface* extra;
	std::vector< Region> upd;	//Regions of the Screen to Update in the next SwapBuffer operation.
	Region Viewport;
	SDL_Surface* Cursor[2];
	SDL_Rect CursorPos;
	short mouseAdjustX[2], mouseAdjustY[2];
	unsigned short CursorIndex;
	int fadePercent;
	unsigned long lastTime;
	unsigned long lastMouseTime;
public:
	SDLVideoDriver(void);
	~SDLVideoDriver(void);
	int Init(void);
	int CreateDisplay(int width, int height, int bpp, bool fullscreen);
	void SetDisplayTitle(char* title, char* icon);
	VideoModes GetVideoModes(bool fullscreen = false);
	bool TestVideoMode(VideoMode& vm);
	bool ToggleFullscreenMode();
	int SwapBuffers(void);
	bool ToggleGrabInput();
	Sprite2D* CreateSprite(int w, int h, int bpp, DWORD rMask, DWORD gMask,
		DWORD bMask, DWORD aMask, void* pixels, bool cK = false, int index = 0);
	Sprite2D* CreateSprite8(int w, int h, int bpp, void* pixels,
		void* palette, bool cK = false, int index = 0);
	void FreeSprite(Sprite2D* spr);
	void BlitSprite(Sprite2D* spr, int x, int y, bool anchor = false,
		Region* clip = NULL);
	void BlitSpriteRegion(Sprite2D* spr, Region& size, int x, int y,
		bool anchor = true, Region* clip = NULL);
	void BlitSpriteTinted(Sprite2D* spr, int x, int y, Color tint,
		Region* clip = NULL);
	void BlitSpriteMode(Sprite2D* spr, int x, int y, int blendMode,
		bool anchor = true, Region* clip = NULL);
	void SetCursor(Sprite2D* up, Sprite2D* down);
	Region GetViewport(void);
	void SetViewport(int x, int y);
	void SetViewport(int x, int y, int w, int h);
	void MoveViewportTo(int x, int y);
	void ConvertToVideoFormat(Sprite2D* sprite);
	void CalculateAlpha(Sprite2D* sprite);
	/** No descriptions */
	void SetPalette(Sprite2D* spr, Color* pal);
	/** This function Draws the Border of a Rectangle as described by the Region parameter. The Color used to draw the rectangle is passes via the Color parameter. */
	void DrawRect(Region& rgn, Color& color);
	/** This functions Draws a Circle */
	void DrawCircle(short cx, short cy, unsigned short r, Color& color);
	/** This functions Draws an Ellipse */
	void DrawEllipse(short cx, short cy, unsigned short xr, unsigned short yr,
		Color& color);
	/** This function Draws a Polygon on the Screen */
	void DrawPolyline(Gem_Polygon* poly, Color& color, bool fill = false);
	inline void DrawLine(short x1, short y1, short x2, short y2, Color& color);
	/** Creates a Palette from Color */
	Color* CreatePalette(Color color, Color back);
	/** Blits a Sprite filling the Region */
	void BlitTiled(Region rgn, Sprite2D* img, bool anchor = false);
	/** Send a Quit Signal to the Event Queue */
	bool Quit();
	/** Get the Palette of a Sprite */
	Color* GetPalette(Sprite2D* spr);
	/** Mirrors an Animation Horizontally */
	void MirrorAnimation(Animation* anim);
	/** Set Clip Rect */
	void SetClipRect(Region* clip);
	/** Convers a Screen Coordinate to a Game Coordinate */

	void ConvertToGame(short& x, short& y)
	{
		x += Viewport.x;
		y += Viewport.y;
	}
	/** */
	Sprite2D* PrecalculatePolygon(Point* points, int count, Color& color);

	void SetFadePercent(int percent);

	void* GetVideoSurface()
	{
		return disp;
	}
private:
	inline void SetPixel(short x, short y, Color& color);
	inline void GetPixel(short x, short y, Color* color);

public:
	void release(void)
	{
		delete this;
	}
};

#endif
