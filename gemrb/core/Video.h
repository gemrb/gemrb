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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

/**
 * @file Video.h
 * Declares Video, base class for video output plugins.
 * @author The GemRB Project
 */

#ifndef VIDEO_H
#define VIDEO_H

#include "globals.h"

#include "Plugin.h"
#include "ScriptedAnimation.h"

class AnimationFactory;
class EventMgr;
class Font;
class Gem_Polygon;
class Palette;
class SpriteCover;
class Wall_Polygon;

// Note: not all these flags make sense together. Specifically:
// NOSHADOW overrides TRANSSHADOW, and BLIT_GREY overrides BLIT_RED
enum SpriteBlitFlags {
	BLIT_HALFTRANS = IE_VVC_TRANSPARENT, // 2
	BLIT_BLENDED = IE_VVC_BLENDED, // 8; not implemented in SDLVideo yet
	BLIT_MIRRORX = IE_VVC_MIRRORX, // 0x10
	BLIT_MIRRORY = IE_VVC_MIRRORY, // 0x20
	BLIT_NOSHADOW = 0x1000,
	BLIT_TRANSSHADOW = 0x2000,
	BLIT_TINTED = 0x00010000, // IE_VVC_TINT = 0x00030000
	BLIT_GREY = IE_VVC_GREYSCALE, // 0x80000; timestop palette
	BLIT_RED = IE_VVC_SEPIA, // 0x02000000; dream scene palette
	BLIT_DARK = IE_VVC_DARKEN, // 0x00100000; not implemented in SDLVideo yet
	BLIT_GLOW = IE_VVC_GLOWING // 0x00200000; not implemented in SDLVideo yet
	// Note: bits 29,30,31 are used by SDLVideo internally
};

// TILE_GREY overrides TILE_SEPIA
enum TileBlitFlags {
	TILE_HALFTRANS = 1,
	TILE_GREY = 2,
	TILE_SEPIA = 4
};

enum CursorType {
	VID_CUR_UP = 0,
	VID_CUR_DOWN = 1,
	VID_CUR_DRAG = 2
};

//disable mouse flags
const int MOUSE_GRAYED		= 1;
const int MOUSE_DISABLED	= 2;
//used (primarily with touchscreens) to control graphical feedback related to the mouse
const int MOUSE_HIDDEN		= 4; // show cursor
const int MOUSE_NO_TOOLTIPS	= 8; // show tooltips

// !!! Keep this synchronized with GUIDefines.py !!!
// used for calculating the tooltip delay limit and the real tooltip delay
#define TOOLTIP_DELAY_FACTOR 250

/**
 * @class Video
 * Base class for video output plugins.
 */

class GEM_EXPORT Video : public Plugin {
public:
	static const TypeID ID;
protected:
	int MouseFlags;
	short xCorr, yCorr;
	EventMgr* EvntManager;
	Region Viewport;
	int width,height,bpp;
	bool fullscreen;
	Sprite2D* Cursor[3];// 0=up, 1=down, 2=drag
	CursorType CursorIndex;
	Region CursorPos;
	bool softKeyboardShowing;

	unsigned char Gamma10toGamma22[256];
	unsigned char Gamma22toGamma10[256];
	//subtitle specific variables
	Font *subtitlefont;
	Palette *subtitlepal;
	Region subtitleregion;
	Color fadeColor;
public:
	Video(void);
	virtual ~Video(void);
	virtual int Init(void) = 0;
	virtual int CreateDisplay(int width, int height, int bpp, bool fullscreen, const char* title) = 0;
	/** Toggles GemRB between fullscreen and windowed mode. */
	bool ToggleFullscreenMode();
	virtual bool SetFullscreenMode(bool set) = 0;
	/** Swaps displayed and back buffers */
	virtual int SwapBuffers(void) = 0;
	/** Grabs and releases mouse cursor within GemRB window */
	virtual bool ToggleGrabInput() = 0;
	virtual short GetWidth() = 0;
	virtual short GetHeight() = 0;
	/** Displays or hides a virtual (software) keyboard*/
	virtual void ShowSoftKeyboard() = 0;
	virtual void HideSoftKeyboard() = 0;
	
	virtual void InitSpriteCover(SpriteCover* sc, int flags) = 0;
	virtual void AddPolygonToSpriteCover(SpriteCover* sc, Wall_Polygon* poly) = 0;
	virtual void DestroySpriteCover(SpriteCover* sc) = 0;

	virtual Sprite2D* CreateSprite(int w, int h, int bpp, ieDword rMask,
		ieDword gMask, ieDword bMask, ieDword aMask, void* pixels,
		bool cK = false, int index = 0) = 0;
	virtual Sprite2D* CreateSprite8(int w, int h, int bpp, void* pixels,
		void* palette, bool cK = false, int index = 0) = 0;
	virtual Sprite2D* CreateSpriteBAM8(int /*w*/, int /*h*/, bool /* RLE */,
		 const unsigned char* /*pixeldata*/,
		 AnimationFactory* /*datasrc*/,
		 Palette* /*palette*/,
		 int /*transindex*/) { return 0; }
	virtual bool SupportsBAMSprites() { return false; }
	virtual void FreeSprite(Sprite2D* &spr) = 0;
	virtual Sprite2D* DuplicateSprite(const Sprite2D* spr) = 0;
	virtual void BlitTile(const Sprite2D* spr, const Sprite2D* mask, int x, int y, const Region* clip, unsigned int flags) = 0;
	virtual void BlitSprite(const Sprite2D* spr, int x, int y, bool anchor = false,
		const Region* clip = NULL) = 0;

	// Note: BlitSpriteRegion's clip region is shifted by Viewport.x/y if
	// anchor is false. This is different from the other BlitSprite functions.
	virtual void BlitSpriteRegion(const Sprite2D* spr, const Region& size, int x, int y,
		bool anchor = true, const Region* clip = NULL) = 0;
	// Note: Tint cannot be constified, because it is modified locally
	// not a pretty interface :)
	virtual void BlitGameSprite(const Sprite2D* spr, int x, int y,
		unsigned int flags, Color tint,
		SpriteCover* cover, Palette *palette = NULL,
		const Region* clip = NULL, bool anchor = false) = 0;
	/** Return GemRB window screenshot.
	 * It's generated from the momentary back buffer */
	virtual Sprite2D* GetScreenshot( Region r ) = 0;
	virtual void ConvertToVideoFormat(Sprite2D* sprite) = 0;
	/** Sets the palette of a plugin specific sprite */
	virtual void SetPalette(void* data, Palette* pal) = 0;
	/** This function Draws the Border of a Rectangle as described by the Region parameter. The Color used to draw the rectangle is passes via the Color parameter. */
	virtual void DrawRect(const Region& rgn, const Color& color, bool fill = true, bool clipped = false) = 0;
	/** this function draws a clipped sprite */
	virtual void DrawRectSprite(const Region& rgn, const Color& color, const Sprite2D* sprite) = 0;
	virtual void SetPixel(short x, short y, const Color& color, bool clipped = false) = 0;
	virtual void GetPixel(short x, short y, Color& color) = 0;
	virtual long GetPixel(void *, unsigned short x, unsigned short y) = 0;
	virtual void GetPixel(void *, unsigned short x, unsigned short y, Color &color) = 0;
	/** Draws a circle */
	virtual void DrawCircle(short cx, short cy, unsigned short r, const Color& color, bool clipped = true) = 0;
	/** Draws an Ellipse Segment */
	virtual void DrawEllipseSegment(short cx, short cy, unsigned short xr, unsigned short yr, const Color& color,
		double anglefrom, double angleto, bool drawlines = true, bool clipped = true) = 0;
	/** Draws an ellipse */
	virtual void DrawEllipse(short cx, short cy, unsigned short xr,
		unsigned short yr, const Color& color, bool clipped = true) = 0;
	/** Draws a polygon on the screen */
	virtual void DrawPolyline(Gem_Polygon* poly, const Color& color,
		bool fill = false) = 0;
	/** Draws a line segment */
	virtual void DrawLine(short x1, short y1, short x2, short y2,
		const Color& color, bool clipped = false) = 0;
	/** Blits a Sprite filling the Region */
	void BlitTiled(Region rgn, const Sprite2D* img, bool anchor = false);
	/** Sets Event Manager */
	void SetEventMgr(EventMgr* evnt);
	/** Sends a Quit Signal to the Event Queue */
	virtual bool Quit(void) = 0;
	/** Gets the Palette of a surface */
	virtual Palette* GetPalette(void* surface) = 0;
	/** Flips sprite vertically, returns new sprite */
	virtual Sprite2D *MirrorSpriteVertical(const Sprite2D *sprite, bool MirrorAnchor) = 0;
	/** Flips sprite horizontally, returns new sprite */
	virtual Sprite2D *MirrorSpriteHorizontal(const Sprite2D *sprite, bool MirrorAnchor) = 0;
	/** Duplicates and transforms sprite to have an alpha channel */
	Sprite2D* CreateAlpha(const Sprite2D *sprite);

	/** Converts a Screen Coordinate to a Game Coordinate */
	virtual void ConvertToGame(short& x, short& y) = 0;
	/** Converts a Game Coordinate to a Screen Coordinate */
	virtual void ConvertToScreen(short& x, short& y) = 0;
	/** Sets the Fading Color */
	virtual void SetFadeColor(int r, int g, int b) = 0;
	/** Sets the Fading to Color Percentage */
	virtual void SetFadePercent(int percent) = 0;
	/** Sets Clip Rectangle */
	virtual void SetClipRect(const Region* clip) = 0;
	/** Gets Clip Rectangle */
	virtual void GetClipRect(Region& clip) = 0;
	/** returns the current mouse coordinates */
	virtual void GetMousePos(int &x, int &y) = 0;
	/** clicks the mouse forcibly */
	virtual void ClickMouse(unsigned int button) = 0;
	/** moves the mouse forcibly */
	virtual void MoveMouse(unsigned int x, unsigned int y) = 0;
	/** initializes the screen for movie */
	virtual void InitMovieScreen(int &w, int &h, bool yuv=false) = 0;
	/** sets the font and color of the movie subtitles */
	void SetMovieFont(Font *stfont, Palette *pal);
	/** draws a movie frame */
	virtual void showFrame(unsigned char* buf, unsigned int bufw,
		unsigned int bufh, unsigned int sx, unsigned int sy,
		unsigned int w, unsigned int h, unsigned int dstx,
		unsigned int dsty, int truecolor, unsigned char *palette,
		ieDword titleref) = 0;
	virtual void showYUVFrame(unsigned char** buf, unsigned int *strides,
		unsigned int bufw, unsigned int bufh,
		unsigned int w, unsigned int h,
		unsigned int dstx, unsigned int dsty,
		ieDword titleref) = 0;
	virtual void DrawMovieSubtitle(ieStrRef text) = 0;
	/** handles events during movie */
	virtual int PollMovieEvents() = 0;
	virtual void SetGamma(int brightness, int contrast) = 0;

	void SetMouseEnabled(int enabled);
	void SetMouseGrayed(bool grayed);
	bool GetFullscreenMode() const;
	/** Sets the mouse cursor sprite to be used for mouseUp, mouseDown, and mouseDrag. See VID_CUR_* defines. */
	void SetCursor(Sprite2D* cur, enum CursorType curIdx);

	/** Scales down a sprite by a ratio */
	Sprite2D* SpriteScaleDown( const Sprite2D* sprite, unsigned int ratio );
	/** Creates an ellipse or circle shaped sprite with various intensity
	 *  for projectile light spots */
	Sprite2D* CreateLight(int radius, int intensity);

	Color SpriteGetPixelSum (const Sprite2D* sprite, unsigned short xbase, unsigned short ybase, unsigned int ratio);
	Region GetViewport(void) const;
	void SetViewport(int x, int y, unsigned int w, unsigned int h);
	void MoveViewportTo(int x, int y);
};

#endif
