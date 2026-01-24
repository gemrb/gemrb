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
#include "Sprite2D.h"

#include <deque>

namespace GemRB {

class EventMgr;
class Gem_Polygon;
class Palette;

class GEM_EXPORT VideoBuffer {
protected:
	Region rect;

public:
	explicit VideoBuffer(const Region& r)
		: rect(r) {}
	virtual ~VideoBuffer() noexcept = default;

	::GemRB::Size Size() const { return rect.size; }
	Point Origin() const { return rect.origin; }
	Region Rect() const { return rect; }

	void SetOrigin(const Point& p) { rect.origin = p; }

	virtual void Clear() { Clear({ 0, 0, rect.w, rect.h }); };
	virtual void Clear(const Region& rgn) = 0;
	// CopyPixels takes at least one void* buffer with implied pitch of Region.w, otherwise alternating pairs of buffers and their corresponding pitches
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

	enum class BufferFormat {
		DISPLAY, // whatever format the video driver thinks is best for the display
		DISPLAY_ALPHA, // the same RGB format as DISPLAY, but forces an alpha if DISPLAY doesn't provide one
		RGBPAL8, // 8 bit palettized
		RGB555, // 16 bit RGB (truecolor)
		RGBA8888, // Standard 8 bits per channel with alpha
		YV12 // YUV format for BIK videos
	};

protected:
	tick_t lastTime = 0;
	EventMgr* EvntManager = nullptr;
	Region screenClip;
	Size screenSize;
	int bpp = 0;
	bool fullscreen = false;

	unsigned char Gamma10toGamma22[256];
	unsigned char Gamma22toGamma10[256];

	using VideoBuffers = std::deque<VideoBuffer*>;

	// collection of all existing video buffers
	VideoBuffers buffers;
	// collection built by calls to PushDrawingBuffer() and cleared after SwapBuffers()
	// the collection is iterated and drawn in order during SwapBuffers()
	// Note: we can add the same buffer more than once to drawingBuffers!
	VideoBuffers drawingBuffers;
	// the current top of drawingBuffers that draw operations occur on
	VideoBuffer* drawingBuffer = nullptr;
	VideoBufferPtr stencilBuffer = nullptr;

	Region ClippedDrawingRect(const Region& target, const Region* clip = NULL) const;
	virtual void Wait(uint32_t) = 0;
	void DestroyBuffer(VideoBuffer*);
	void DestroyBuffers();

private:
	virtual VideoBuffer* NewVideoBuffer(const Region&, BufferFormat) = 0;
	virtual void SwapBuffers(VideoBuffers&) = 0;
	virtual int PollEvents() = 0;
	virtual int CreateDriverDisplay(const char* title, bool vsync) = 0;

	// the actual drawing implementations
	virtual void DrawRectImp(const Region& rgn, const Color& color, bool fill, BlitFlags flags) = 0;
	virtual void DrawPointImp(const BasePoint&, const Color& color, BlitFlags flags) = 0;
	virtual void DrawPointsImp(const std::vector<BasePoint>& points, const Color& color, BlitFlags flags) = 0;
	virtual void DrawCircleImp(const Point& origin, uint16_t r, const Color& color, BlitFlags flags) = 0;
	virtual void DrawEllipseImp(const Region& rect, const Color& color, BlitFlags flags) = 0;
	virtual void DrawPolygonImp(const Gem_Polygon* poly, const Point& origin, const Color& color, bool fill, BlitFlags flags) = 0;
	virtual void DrawLineImp(const BasePoint& start, const BasePoint& end, const Color& color, BlitFlags flags) = 0;
	virtual void DrawLinesImp(const std::vector<Point>& points, const Color& color, BlitFlags flags) = 0;

public:
	Video() noexcept;
	~Video() noexcept override;

	virtual int Init(void) = 0;

	int CreateDisplay(const Size&, int bpp, bool fullscreen, const char* title, bool vsync);
	virtual void SetWindowTitle(const char* title) = 0;

	/** Toggles GemRB between fullscreen and windowed mode. */
	bool ToggleFullscreenMode();
	virtual bool SetFullscreenMode(bool set) = 0;
	bool GetFullscreenMode() const;
	/** Swaps displayed and back buffers */
	int SwapBuffers(int fpscap = 30);
	VideoBufferPtr CreateBuffer(const Region&, BufferFormat = BufferFormat::DISPLAY);
	void PushDrawingBuffer(const VideoBufferPtr&);
	void PopDrawingBuffer();
	void SetStencilBuffer(const VideoBufferPtr&);
	/** Grabs and releases mouse cursor within GemRB window */
	virtual bool ToggleGrabInput() = 0;
	virtual void CaptureMouse(bool enabled) = 0;
	const Size& GetScreenSize() const { return screenSize; }
	virtual int GetDisplayRefreshRate() const = 0;
	virtual int GetVirtualRefreshCap() const = 0;

	virtual void StartTextInput() = 0;
	virtual void StopTextInput() = 0;
	virtual bool InTextInput() = 0;

	virtual bool TouchInputEnabled() = 0;
	virtual bool CanDrawRawGeometry() const { return false; }
	virtual void SetPointerSpeed(int pointerSpeed) = 0;

	virtual Holder<Sprite2D> CreateSprite(const Region&, void* pixels, const PixelFormat&) = 0;

	void BlitSprite(const Holder<Sprite2D>& spr, Point p,
			const Region* clip = nullptr, BlitFlags = BlitFlags::NONE);

	virtual void BlitSprite(const Holder<Sprite2D>& spr, const Region& src, Region dst,
				BlitFlags flags, Color tint = Color()) = 0;

	virtual void BlitGameSprite(const Holder<Sprite2D>& spr, const Point& p,
				    BlitFlags flags, Color tint = Color()) = 0;

	void BlitGameSpriteWithPalette(const Holder<Sprite2D>& spr, const Holder<Palette>& pal, const Point& p,
				       BlitFlags flags, Color tint);

	virtual void BlitVideoBuffer(const VideoBufferPtr& buf, const Point& p, BlitFlags flags,
				     Color tint = Color()) = 0;
	virtual void BlitVideoBufferFully(const VideoBufferPtr& buf, BlitFlags flags,
					  Color tint = Color()) = 0;

	/** Return GemRB window screenshot.
	 * It's generated from the momentary back buffer */
	virtual Holder<Sprite2D> GetScreenshot(Region r, const VideoBufferPtr& buf = nullptr) = 0;
	/** This function Draws the Border of a Rectangle as described by the Region parameter. The Color used to draw the rectangle is passes via the Color parameter. */
	void DrawRect(const Region& rgn, const Color& color, bool fill = true, BlitFlags flags = BlitFlags::NONE);

	void DrawPoint(const BasePoint&, const Color& color, BlitFlags flags = BlitFlags::NONE);
	void DrawPoints(const std::vector<BasePoint>& points, const Color& color, BlitFlags flags = BlitFlags::NONE);

	// draw a circle at origin with radius r
	void DrawCircle(const Point& origin, uint16_t r, const Color& color, BlitFlags flags = BlitFlags::NONE);
	// Draw an ellipse bounded by rect
	void DrawEllipse(const Region& rect, const Color& color, BlitFlags flags = BlitFlags::NONE);
	/** Draws a polygon on the screen */
	void DrawPolygon(const Gem_Polygon* poly, const Point& origin, const Color& color, bool fill = false, BlitFlags flags = BlitFlags::NONE);
	/** Draws a line segment */
	void DrawLine(const BasePoint& p1, const BasePoint& p2, const Color& color, BlitFlags flags = BlitFlags::NONE);
	void DrawLines(const std::vector<Point>& points, const Color& color, BlitFlags flags = BlitFlags::NONE);
	virtual void DrawRawGeometry(
		const std::vector<float>& /*vertices*/,
		const std::vector<Color>& /*colors*/,
		BlitFlags /*blitFlags*/
	) {};
	/** Sets Event Manager */
	void SetEventMgr(EventMgr* evnt);

	/** Sets Clip Rectangle */
	void SetScreenClip(const Region* clip, bool toScreen = true);
	/** Gets Clip Rectangle */
	const Region& GetScreenClip() const { return screenClip; }

	virtual void SetGamma(int brightness, int contrast) = 0;

	/** Scales down a sprite by a ratio */
	Holder<Sprite2D> SpriteScaleDown(const Holder<Sprite2D>& sprite, unsigned int ratio);

	Color SpriteGetPixelSum(const Holder<Sprite2D>& sprite, unsigned short xbase, unsigned short ybase, unsigned int ratio) const;
};

extern GEM_EXPORT PluginHolder<Video> VideoDriver;

}

#endif
