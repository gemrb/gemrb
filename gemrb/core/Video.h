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
#include "Polygon.h"
#include "ScriptedAnimation.h"

#include <deque>
#include <algorithm>

namespace GemRB {

class EventMgr;
class Palette;
using PaletteHolder = Holder<Palette>;
  
// Note: not all these flags make sense together.
// Specifically: BLIT_GREY overrides BLIT_SEPIA
enum SpriteBlitFlags : uint32_t {
	BLIT_NO_FLAGS = 0,
	BLIT_HALFTRANS = IE_VVC_TRANSPARENT, // 2
	BLIT_BLENDED = IE_VVC_BLENDED, // 8; not implemented in SDLVideo yet
	BLIT_MIRRORX = IE_VVC_MIRRORX, // 0x10
	BLIT_MIRRORY = IE_VVC_MIRRORY, // 0x20
	//BLIT_NOSHADOW = 0x1000, no longer used
	//BLIT_TRANSSHADOW = 0x2000, no longer used
	BLIT_TINTED = 0x00010000, // IE_VVC_TINT = 0x00030000
	BLIT_GREY = IE_VVC_GREYSCALE, // 0x80000; timestop palette
	BLIT_SEPIA = IE_VVC_SEPIA, // 0x02000000; dream scene palette
	BLIT_MULTIPLY = IE_VVC_DARKEN, // 0x00100000; not implemented in SDLVideo yet
	BLIT_GLOW = IE_VVC_GLOWING, // 0x00200000; not implemented in SDLVideo yet
	BLIT_STENCIL_ALPHA = 0x00800000, // blend with the stencil buffer using the stencil's alpha channel as the stencil
	BLIT_STENCIL_RED = 0x01000000, // blend with the stencil buffer using the stencil's r channel as the stencil
	BLIT_STENCIL_GREEN = 0x08000000, // blend with the stencil buffer using the stencil's g channel as the stencil
	BLIT_STENCIL_BLUE = 0x20000000, // blend with the stencil buffer using the stencil's b channel as the stencil
	BLIT_STENCIL_DITHER = 0x10000000 // use dithering instead of transpanency. only affects stencil values of 128.
};

#define BLIT_STENCIL_MASK (BLIT_STENCIL_ALPHA|BLIT_STENCIL_RED|BLIT_STENCIL_GREEN|BLIT_STENCIL_BLUE)

// !!! Keep this synchronized with GUIDefines.py !!!
// used for calculating the tooltip delay limit and the real tooltip delay
#define TOOLTIP_DELAY_FACTOR 250

class GEM_EXPORT VideoBuffer {
protected:
	Region rect;

public:
	VideoBuffer(const Region& r) : rect(r) {}
	virtual ~VideoBuffer() {}
	
	::GemRB::Size Size() const { return rect.Dimensions(); }
	Point Origin() const { return rect.Origin(); }
	Region Rect() const  { return rect; }
	
	void SetOrigin(const Point& p) { rect.x = p.x, rect.y = p.y; }

	virtual void Clear() = 0;
	// CopyPixels takes at least one void* buffer with implied pitch of Region.w, otherwise alternating pairs of buffers and their coresponding pitches
	virtual void CopyPixels(const Region& bufDest, const void* pixelBuf, const int* pitch = NULL, ...) = 0;
	
	virtual bool RenderOnDisplay(void* display) const = 0;
};

using VideoBufferPtr = std::shared_ptr<VideoBuffer>;

/**
 * @class Video
 * Base class for video output plugins.
 */

class GEM_EXPORT Video : public Plugin {
public:
	static const TypeID ID;

	enum BufferFormat {
		DISPLAY, // whatever format the video driver thinks is best for the display
		DISPLAY_ALPHA, // the same RGB format as DISPLAY, but forces an alpha if DISPLAY doesn't provide one
		RGBPAL8,	// 8 bit palettized
		RGB555, // 16 bit RGB (truecolor)
		RGBA8888, // Standard 8 bits per channel with alpha
		YV12    // YUV format for BIK videos
	};

protected:
	unsigned long lastTime;
	EventMgr* EvntManager;
	Region screenClip;
	Size screenSize;
	int bpp;
	bool fullscreen;

	unsigned char Gamma10toGamma22[256];
	unsigned char Gamma22toGamma10[256];

	typedef std::deque<VideoBuffer*> VideoBuffers;

	// collection of all existing video buffers
	VideoBuffers buffers;
	// collection built by calls to PushDrawingBuffer() and cleared after SwapBuffers()
	// the collection is iterated and drawn in order during SwapBuffers()
	// Note: we can add the same buffer more than once to drawingBuffers!
	VideoBuffers drawingBuffers;
	// the current top of drawingBuffers that draw operations occur on
	VideoBuffer* drawingBuffer;
	VideoBufferPtr stencilBuffer = nullptr;

	Region ClippedDrawingRect(const Region& target, const Region* clip = NULL) const;
	virtual void Wait(unsigned long) = 0;
	void DestroyBuffer(VideoBuffer*);

private:
	virtual VideoBuffer* NewVideoBuffer(const Region&, BufferFormat)=0;
	virtual void SwapBuffers(VideoBuffers&)=0;
	virtual int PollEvents() = 0;
	virtual int CreateDriverDisplay(const Size& s, int bpp, const char* title) = 0;

	// the actual drawing implementations
	virtual void DrawRectImp(const Region& rgn, const Color& color, bool fill, uint32_t flags) = 0;
	virtual void DrawPointImp(const Point&, const Color& color, uint32_t flags) = 0;
	virtual void DrawPointsImp(const std::vector<Point>& points, const Color& color, uint32_t flags) = 0;
	virtual void DrawCircleImp(const Point& origin, unsigned short r, const Color& color, uint32_t flags) = 0;
	virtual void DrawEllipseSegmentImp(const Point& origin, unsigned short xr, unsigned short yr, const Color& color,
									   double anglefrom, double angleto, bool drawlines, uint32_t flags) = 0;
	virtual void DrawEllipseImp(const Point& origin, unsigned short xr, unsigned short yr, const Color& color, uint32_t flags) = 0;
	virtual void DrawPolygonImp(const Gem_Polygon* poly, const Point& origin, const Color& color, bool fill, uint32_t flags) = 0;
	virtual void DrawLineImp(const Point& p1, const Point& p2, const Color& color, uint32_t flags) = 0;
	virtual void DrawLinesImp(const std::vector<Point>& points, const Color& color, uint32_t flags)=0;

public:
	Video(void);
	virtual ~Video(void);

	virtual int Init(void) = 0;

	int CreateDisplay(const Size&, int bpp, bool fullscreen, const char* title);
	virtual void SetWindowTitle(const char *title) = 0;

	/** Toggles GemRB between fullscreen and windowed mode. */
	bool ToggleFullscreenMode();
	virtual bool SetFullscreenMode(bool set) = 0;
	bool GetFullscreenMode() const;
	/** Swaps displayed and back buffers */
	int SwapBuffers(unsigned int fpscap = 30);
	VideoBufferPtr CreateBuffer(const Region&, BufferFormat = DISPLAY);
	void PushDrawingBuffer(const VideoBufferPtr&);
	void PopDrawingBuffer();
	void SetStencilBuffer(const VideoBufferPtr&);
	/** Grabs and releases mouse cursor within GemRB window */
	virtual bool ToggleGrabInput() = 0;
	virtual void CaptureMouse(bool enabled) = 0;
	const Size& GetScreenSize() { return screenSize; }

	virtual void StartTextInput() = 0;
	virtual void StopTextInput() = 0;
	virtual bool InTextInput() = 0;

	virtual bool TouchInputEnabled() = 0;

	virtual Holder<Sprite2D> CreateSprite(const Region&, int bpp, ieDword rMask,
		ieDword gMask, ieDword bMask, ieDword aMask, void* pixels,
		bool cK = false, int index = 0) = 0;
	virtual Holder<Sprite2D> CreateSprite8(const Region&, void* pixels,
									PaletteHolder palette, bool cK = false, int index = 0) = 0;
	virtual Holder<Sprite2D> CreatePalettedSprite(const Region&, int bpp, void* pixels,
										   Color* palette, bool cK = false, int index = 0) = 0;
	virtual bool SupportsBAMSprites() { return false; }

	virtual void BlitTile(const Holder<Sprite2D> spr, int x, int y, const Region* clip,
						  uint32_t flags, const Color* tint = NULL) = 0;
	void BlitSprite(const Holder<Sprite2D> spr, int x, int y,
					const Region* clip = NULL);
	virtual void BlitSprite(const Holder<Sprite2D> spr, const Region& src, Region dst) = 0;

	// Note: Tint cannot be constified, because it is modified locally
	// not a pretty interface :)
	virtual void BlitGameSprite(const Holder<Sprite2D> spr, int x, int y,
								uint32_t flags, Color tint,
								const Region* clip = NULL) = 0;

	void BlitGameSpriteWithPalette(Holder<Sprite2D> spr, PaletteHolder pal, int x, int y,
				   uint32_t flags, Color tint, const Region* clip = NULL);

	virtual void BlitVideoBuffer(const VideoBufferPtr& buf, const Point& p, uint32_t flags,
								 const Color* tint = nullptr, const Region* clip = nullptr) = 0;

	/** Return GemRB window screenshot.
	 * It's generated from the momentary back buffer */
	virtual Holder<Sprite2D> GetScreenshot(Region r, const VideoBufferPtr& buf = nullptr) = 0;
	/** This function Draws the Border of a Rectangle as described by the Region parameter. The Color used to draw the rectangle is passes via the Color parameter. */
	void DrawRect(const Region& rgn, const Color& color, bool fill = true, uint32_t flags = 0);

	void DrawPoint(const Point&, const Color& color, uint32_t flags = 0);
	void DrawPoints(const std::vector<Point>& points, const Color& color, uint32_t flags = 0);

	/** Draws a circle */
	void DrawCircle(const Point& origin, unsigned short r, const Color& color, uint32_t flags = 0);
	/** Draws an Ellipse Segment */
	void DrawEllipseSegment(const Point& origin, unsigned short xr, unsigned short yr, const Color& color,
									double anglefrom, double angleto, bool drawlines = true, uint32_t flags = 0);
	/** Draws an ellipse */
	void DrawEllipse(const Point& origin, unsigned short xr, unsigned short yr, const Color& color, uint32_t flags = 0);
	/** Draws a polygon on the screen */
	void DrawPolygon(const Gem_Polygon* poly, const Point& origin, const Color& color, bool fill = false, uint32_t flags = 0);
	/** Draws a line segment */
	void DrawLine(const Point& p1, const Point& p2, const Color& color, uint32_t flags = 0);
	void DrawLines(const std::vector<Point>& points, const Color& color, uint32_t flags = 0);
	/** Blits a Sprite filling the Region */
	void BlitTiled(Region rgn, const Holder<Sprite2D> img);
	/** Sets Event Manager */
	void SetEventMgr(EventMgr* evnt);
	/** Flips sprite, returns new sprite */
	Holder<Sprite2D> MirrorSprite(const Holder<Sprite2D> sprite, uint32_t flags, bool MirrorAnchor);
	/** Duplicates and transforms sprite to have an alpha channel */
	Holder<Sprite2D> CreateAlpha(const Holder<Sprite2D> sprite);

	/** Sets Clip Rectangle */
	void SetScreenClip(const Region* clip);
	/** Gets Clip Rectangle */
	const Region& GetScreenClip() { return screenClip; }
	virtual void SetGamma(int brightness, int contrast) = 0;

	/** Scales down a sprite by a ratio */
	Holder<Sprite2D> SpriteScaleDown(const Holder<Sprite2D> sprite, unsigned int ratio);
	/** Creates an ellipse or circle shaped sprite with various intensity
	 *  for projectile light spots */
	Holder<Sprite2D> CreateLight(int radius, int intensity);

	Color SpriteGetPixelSum(const Holder<Sprite2D> sprite, unsigned short xbase, unsigned short ybase, unsigned int ratio);
};

}

#endif
