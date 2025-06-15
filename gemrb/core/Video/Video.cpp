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

#include "Video.h"

#include "Palette.h"
#include "Sprite2D.h"

#include <cmath>

namespace GemRB {

const TypeID Video::ID = { "Video" };

Video::Video() noexcept
{
	// Initialize gamma correction tables
	for (int i = 0; i < 256; i++) {
		Gamma22toGamma10[i] = (unsigned char) (0.5 + (pow(i / 255.0, 2.2 / 1.0) * 255.0));
		Gamma10toGamma22[i] = (unsigned char) (0.5 + (pow(i / 255.0, 1.0 / 2.2) * 255.0));
	}
}

Video::~Video() noexcept
{
	DestroyBuffers();
}

void Video::DestroyBuffers()
{
	for (auto buffer : buffers) {
		delete buffer;
	}
	buffers.clear();
}

int Video::CreateDisplay(const Size& s, int bits, bool fs, const char* title, bool vsync)
{
	bpp = bits;
	screenSize = s;

	int ret = CreateDriverDisplay(title, vsync);
	if (ret == GEM_OK) {
		SetScreenClip(NULL);
		if (fs) {
			ToggleFullscreenMode();
		}
	}
	return ret;
}

Region Video::ClippedDrawingRect(const Region& target, const Region* clip) const
{
	// clip to both screen and the target buffer
	Region bufRgn(Point(), drawingBuffer->Size());
	Region r = target.Intersect(screenClip).Intersect(bufRgn);
	if (clip) {
		// Intersect clip with both screen and target rectangle
		r = clip->Intersect(r);
	}
	// the clip must be "safe". no negative values or crashy crashy
	if (r.size.IsInvalid()) { // logically equivalent to no intersection
		r.h_get() = 0;
		r.w_get() = 0;
	}
	return r;
}

VideoBufferPtr Video::CreateBuffer(const Region& r, BufferFormat fmt)
{
	VideoBuffer* buf = NewVideoBuffer(r, fmt);
	if (buf) {
		buffers.push_back(buf);
		return VideoBufferPtr(buffers.back(), [this](VideoBuffer* buffer) {
			DestroyBuffer(buffer);
		});
	}
	return nullptr;
	//assert(buf); // FIXME: we should probably deal with this happening
}

void Video::DestroyBuffer(VideoBuffer* buffer)
{
	// FIXME: this is poorly implemented
	VideoBuffers::iterator it = std::find(drawingBuffers.begin(), drawingBuffers.end(), buffer);
	if (it != drawingBuffers.end()) {
		drawingBuffers.erase(it);
	}

	it = std::find(buffers.begin(), buffers.end(), buffer);
	if (it != buffers.end()) {
		buffers.erase(it);
	}
	delete buffer;
}

void Video::PushDrawingBuffer(const VideoBufferPtr& buf)
{
	assert(buf);
	drawingBuffers.push_back(buf.get());
	drawingBuffer = drawingBuffers.back();
}

void Video::PopDrawingBuffer()
{
	if (drawingBuffers.size() <= 1) {
		// can't pop last buffer
		return;
	}
	drawingBuffers.pop_back();
	drawingBuffer = drawingBuffers.back();
}

void Video::SetStencilBuffer(const VideoBufferPtr& stencil)
{
	stencilBuffer = stencil;
}

int Video::SwapBuffers(int fpscap)
{
	SwapBuffers(drawingBuffers);
	drawingBuffers.clear();
	drawingBuffer = NULL;
	SetScreenClip(NULL);

	int deviceCap = GetVirtualRefreshCap();
	if (deviceCap > 0) {
		fpscap = fpscap > 0 ? std::min(deviceCap, fpscap) : deviceCap;
	}

	if (fpscap > 0) {
		tick_t lim = 1000 / fpscap;
		tick_t time = GetMilliseconds();
		if ((time - lastTime) < lim) {
			Wait(lim - int(time - lastTime));
			time = GetMilliseconds();
		}
		lastTime = time;
	} else {
		lastTime = GetMilliseconds();
	}

	return PollEvents();
}

void Video::SetScreenClip(const Region* clip)
{
	screenClip = Region(Point(), screenSize);
	if (clip) {
		screenClip = screenClip.Intersect(*clip);
	}
}

bool Video::ToggleFullscreenMode()
{
	return SetFullscreenMode(!fullscreen);
}

/** Set Event Manager */
void Video::SetEventMgr(EventMgr* evnt)
{
	//if 'evnt' is NULL then no Event Manager will be used
	EvntManager = evnt;
}

/** Get the fullscreen mode */
bool Video::GetFullscreenMode() const
{
	return fullscreen;
}

void Video::BlitSprite(const Holder<Sprite2D>& spr, Point p, const Region* clip, BlitFlags flags)
{
	p -= spr->Frame.origin;
	Region dst(p, spr->Frame.size);
	Region fClip = ClippedDrawingRect(dst, clip);

	if (fClip.size.IsInvalid()) {
		return; // already know blit fails
	}

	Region src(0, 0, spr->Frame.w_get(), spr->Frame.h_get());
	// adjust the src region to account for the clipping
	src.x_get() += fClip.x_get() - dst.x_get(); // the left edge
	src.w_get() -= dst.w_get() - fClip.w_get(); // the right edge
	src.y_get() += fClip.y_get() - dst.y_get(); // the top edge
	src.h_get() -= dst.h_get() - fClip.h_get(); // the bottom edge

	assert(src.w_get() == fClip.w_get() && src.h_get() == fClip.h_get());

	// just pass fclip as dst
	// since the next stage is also public, we must readd the Pos because it will again be removed
	fClip.x_get() += spr->Frame.x_get();
	fClip.y_get() += spr->Frame.y_get();
	BlitSprite(spr, src, fClip, flags | BlitFlags::BLENDED);
}

void Video::BlitGameSpriteWithPalette(const Holder<Sprite2D>& spr, const Holder<Palette>& pal, const Point& p,
				      BlitFlags flags, Color tint)
{
	if (pal) {
		Holder<Palette> oldpal = spr->GetPalette();
		spr->SetPalette(pal);
		BlitGameSprite(spr, p, flags, tint);
		spr->SetPalette(oldpal);
	} else {
		BlitGameSprite(spr, p, flags, tint);
	}
}

Holder<Sprite2D> Video::SpriteScaleDown(const Holder<Sprite2D>& sprite, unsigned int ratio)
{
	Region scaledFrame = sprite->Frame;
	scaledFrame.w_get() /= ratio;
	scaledFrame.h_get() /= ratio;

	unsigned int* pixels = (unsigned int*) malloc(scaledFrame.w_get() * scaledFrame.h_get() * 4);
	int i = 0;

	for (int y = 0; y < scaledFrame.h_get(); y++) {
		for (int x = 0; x < scaledFrame.w_get(); x++) {
			Color c = SpriteGetPixelSum(sprite, x, y, ratio);

			*(pixels + i++) = c.r + (c.g << 8) + (c.b << 16) + (c.a << 24);
		}
	}

	static const PixelFormat fmt(4, 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000);
	Holder<Sprite2D> small = CreateSprite(scaledFrame, pixels, fmt);

	small->Frame.x_get() = sprite->Frame.x_get() / ratio;
	small->Frame.y_get() = sprite->Frame.y_get() / ratio;

	return small;
}

Color Video::SpriteGetPixelSum(const Holder<Sprite2D>& sprite, unsigned short xbase, unsigned short ybase, unsigned int ratio) const
{
	// TODO: turn this into one of our software "shaders"
	Color sum;
	unsigned int count = ratio * ratio;
	unsigned int r = 0, g = 0, b = 0, a = 0;

	for (unsigned int x = 0; x < ratio; x++) {
		for (unsigned int y = 0; y < ratio; y++) {
			const Point p(xbase * ratio + x, ybase * ratio + y);
			Color c = sprite->GetPixel(p);
			r += Gamma22toGamma10[c.r];
			g += Gamma22toGamma10[c.g];
			b += Gamma22toGamma10[c.b];
			a += Gamma22toGamma10[c.a];
		}
	}

	sum.r = Gamma10toGamma22[r / count];
	sum.g = Gamma10toGamma22[g / count];
	sum.b = Gamma10toGamma22[b / count];
	sum.a = Gamma10toGamma22[a / count];

	return sum;
}

static Color ApplyFlagsForColor(const Color& inCol, BlitFlags& flags)
{
	Color outC = inCol;
	if (flags & BlitFlags::HALFTRANS) {
		// set exactly to 128 because it is an optimized value
		// if we end up needing to do half of something already transparent we can change this
		// or do the calculations before calling the video driver and dont pass BlitFlags::HALFTRANS
		outC.a = 128;
	}

	// TODO: do we need to handle BlitFlags::GREY, BlitFlags::SEPIA, or BlitFlags::COLOR_MOD?
	// if so we should do that here instead of in the implementations

	if (flags & BlitFlags::GREY) {
		//static RGBBlendingPipeline<GREYSCALE, true> blender;
	} else if (flags & BlitFlags::SEPIA) {
		//static RGBBlendingPipeline<SEPIA, true> blender;
	}

	if (flags & BlitFlags::COLOR_MOD) {
		flags |= BlitFlags::MOD;
	}

	// clear handled flags
	flags &= ~(BlitFlags::HALFTRANS | BlitFlags::GREY | BlitFlags::SEPIA | BlitFlags::COLOR_MOD);
	return outC;
}

void Video::DrawRect(const Region& rgn, const Color& color, bool fill, BlitFlags flags)
{
	Color c = ApplyFlagsForColor(color, flags);
	DrawRectImp(rgn, c, fill, flags);
}

void Video::DrawPoint(const BasePoint& p, const Color& color, BlitFlags flags)
{
	Color c = ApplyFlagsForColor(color, flags);
	DrawPointImp(p, c, flags);
}

void Video::DrawPoints(const std::vector<BasePoint>& points, const Color& color, BlitFlags flags)
{
	Color c = ApplyFlagsForColor(color, flags);
	DrawPointsImp(points, c, flags);
}

void Video::DrawCircle(const Point& origin, uint16_t r, const Color& color, BlitFlags flags)
{
	Color c = ApplyFlagsForColor(color, flags);
	DrawCircleImp(origin, r, c, flags);
}

void Video::DrawEllipse(const Region& rect, const Color& color, BlitFlags flags)
{
	Color c = ApplyFlagsForColor(color, flags);
	DrawEllipseImp(rect, c, flags);
}

void Video::DrawPolygon(const Gem_Polygon* poly, const Point& origin, const Color& color, bool fill, BlitFlags flags)
{
	Color c = ApplyFlagsForColor(color, flags);
	DrawPolygonImp(poly, origin, c, fill, flags);
}

void Video::DrawLine(const BasePoint& p1, const BasePoint& p2, const Color& color, BlitFlags flags)
{
	Color c = ApplyFlagsForColor(color, flags);
	DrawLineImp(p1, p2, c, flags);
}

void Video::DrawLines(const std::vector<Point>& points, const Color& color, BlitFlags flags)
{
	Color c = ApplyFlagsForColor(color, flags);
	DrawLinesImp(points, c, flags);
}

}
