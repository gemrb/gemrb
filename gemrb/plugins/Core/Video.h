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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/Video.h,v 1.45 2005/10/18 22:43:41 edheldil Exp $
 *
 */

#ifndef VIDEO_H
#define VIDEO_H

#include "../../includes/globals.h"
#include "Plugin.h"
#include "EventMgr.h"
#include "Animation.h"
#include "Polygon.h"

#ifdef WIN32

#ifdef GEM_BUILD_DLL
#define GEM_EXPORT __declspec(dllexport)
#else
#define GEM_EXPORT __declspec(dllimport)
#endif

#else
#define GEM_EXPORT
#endif

class GEM_EXPORT Video : public Plugin {
public:
	Video(void);
	virtual ~Video(void);
	virtual int Init(void) = 0;
	virtual int CreateDisplay(int width, int height, int bpp, bool fullscreen) = 0;
	virtual void SetDisplayTitle(char* title, char* icon) = 0;
	virtual VideoModes GetVideoModes(bool fullscreen = false) = 0;
	virtual bool TestVideoMode(VideoMode& vm) = 0;
	virtual bool ToggleFullscreenMode() = 0;
	virtual int SwapBuffers(void) = 0;
	virtual bool ToggleGrabInput() = 0;
	virtual Sprite2D* CreateSprite(int w, int h, int bpp, ieDword rMask,
		ieDword gMask, ieDword bMask, ieDword aMask, void* pixels, bool cK = false,
		int index = 0) = 0;
	virtual Sprite2D* CreateSprite8(int w, int h, int bpp, void* pixels,
		void* palette, bool cK = false, int index = 0) = 0;
	virtual void FreeSprite(Sprite2D* spr) = 0;
	virtual void BlitSprite(Sprite2D* spr, int x, int y, bool anchor = false,
		Region* clip = NULL) = 0;
	virtual void BlitSpriteRegion(Sprite2D* spr, Region& size, int x, int y,
		bool anchor = true, Region* clip = NULL) = 0;
	virtual void BlitSpriteNoShadow(Sprite2D* spr, int x, int y, Color tint,
		Region* clip = NULL) = 0;
	virtual void BlitSpriteTinted(Sprite2D* spr, int x, int y, Color tint,
		Color *palette = NULL, Region* clip = NULL) = 0;
	virtual void SetCursor(Sprite2D* up, Sprite2D* down) = 0;
	virtual void SetDragCursor(Sprite2D* drag) = 0;
	virtual Sprite2D* GetPreview(int w, int h) = 0;
	virtual Region GetViewport(void) = 0;
	virtual void SetViewport(int x, int y, unsigned int w, unsigned int h) = 0;
	virtual void MoveViewportTo(int x, int y, bool center) = 0;
	virtual void ConvertToVideoFormat(Sprite2D* sprite) = 0;
	virtual void CalculateAlpha(Sprite2D* sprite) = 0;
	/** No descriptions */
	virtual void SetPalette(Sprite2D* spr, Color* pal) = 0;
	/** This function Draws the Border of a Rectangle as described by the Region parameter. The Color used to draw the rectangle is passes via the Color parameter. */
	virtual void DrawRect(Region& rgn, Color& color, bool fill = true, bool clipped = false) = 0;
	virtual void DrawRectSprite(Region& rgn, Color& color, Sprite2D* sprite) = 0;
	/** This functions Draws a Circle */
	virtual void DrawCircle(short cx, short cy, unsigned short r, Color& color) = 0;
	/** This functions Draws an Ellipse */
	virtual void DrawEllipse(short cx, short cy, unsigned short xr,
		unsigned short yr, Color& color, bool clipped = true) = 0;
	/** This function Draws a Polygon on the Screen */
	virtual void DrawPolyline(Gem_Polygon* poly, Color& color,
		bool fill = false) = 0;
	virtual void DrawLine(short x1, short y1, short x2, short y2,
		Color& color) = 0;
	/** Frees a Palette */
	virtual void FreePalette(Color *&palette) = 0;
	/** Creates a Palette from Color */
	virtual Color* CreatePalette(Color color, Color back) = 0;
	/** Blits a Sprite filling the Region */
	virtual void BlitTiled(Region rgn, Sprite2D* img, bool anchor = false) = 0;
	/** Set Event Manager */
	void SetEventMgr(EventMgr* evnt);
	/** Send a Quit Signal to the Event Queue */
	virtual bool Quit(void) = 0;
	/** Get the Palette of a Sprite */
	virtual Color* GetPalette(Sprite2D* spr) = 0;
	/** Flips sprite vertically, returns new sprite */
	virtual Sprite2D *MirrorSpriteVertical(Sprite2D *sprite, bool MirrorAnchor) = 0;
	/** Flips sprite horizontally, returns new sprite */
	virtual Sprite2D *MirrorSpriteHorizontal(Sprite2D *sprite, bool MirrorAnchor) = 0;
	/** Transforms sprite to have an alpha channel */
	virtual void CreateAlpha(Sprite2D *sprite) = 0;

	/** Convers a Screen Coordinate to a Game Coordinate */
	virtual void ConvertToGame(short& x, short& y) = 0;
	/** */
	virtual Sprite2D* PrecalculatePolygon(Gem_Polygon *poly, Color &color) = 0;//Point* points, int count, Color& color, Region& bbox) = 0;
	/** Sets the Fading Color */
	virtual void SetFadeColor(int r, int g, int b) = 0;
	/** Sets the Fading to Color Percentage */
	virtual void SetFadePercent(int percent) = 0;
	/** Set Clip Rect */
	virtual void SetClipRect(Region* clip) = 0;
public:
	/** Event Manager Pointer */
	EventMgr* Evnt;
	short moveX, moveY;
	bool DisableMouse, DisableScroll;
	short xCorr, yCorr;
public:
	virtual void* GetVideoSurface() = 0;
	virtual bool IsSpritePixelTransparent(Sprite2D* sprite, unsigned short x, unsigned short y) = 0;
};

#endif
