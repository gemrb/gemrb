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

#include "win32def.h"

#include "Interface.h"
#include "Palette.h"
#include "Sprite2D.h"

#include <cmath>

namespace GemRB {

const TypeID Video::ID = { "Video" };

static Color ApplyFlagsForColor(const Color& inCol, uint32_t& flags);

Video::Video(void)
{
	drawingBuffer = NULL;
	EvntManager = NULL;

	// Initialize gamma correction tables
	for (int i = 0; i < 256; i++) {
		Gamma22toGamma10[i] = (unsigned char)(0.5 + (pow (i/255.0, 2.2/1.0) * 255.0));
		Gamma10toGamma22[i] = (unsigned char)(0.5 + (pow (i/255.0, 1.0/2.2) * 255.0));
	}

	// boring inits just to be extra clean
	bpp = 0;
	fullscreen = false;
	lastTime = 0;
}

Video::~Video(void)
{
	VideoBuffers::iterator it;
	it = buffers.begin();
	for (; it != buffers.end(); ++it) {
		delete *it;
	}
}

int Video::CreateDisplay(const Size& s, int bpp, bool fs, const char* title)
{
	int ret = CreateDriverDisplay(s, bpp, title);
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
	if (r.Dimensions().IsEmpty()) { // logically equivalent to no intersection
		r.h = 0;
		r.w = 0;
	}
	return r;
}

VideoBufferPtr Video::CreateBuffer(const Region& r, BufferFormat fmt)
{
	VideoBuffer* buf = NewVideoBuffer(r, fmt);
	assert(buf); // FIXME: we should probably deal with this happening
	buffers.push_back(buf);
	return VideoBufferPtr(buffers.back(), [this](VideoBuffer* buffer) {
		DestroyBuffer(buffer);
	});
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

int Video::SwapBuffers(unsigned int fpscap)
{
	SwapBuffers(drawingBuffers);
	drawingBuffers.clear();
	drawingBuffer = NULL;
	SetScreenClip(NULL);

	if (fpscap) {
		unsigned int lim = 1000/fpscap;
		unsigned long time = GetTickCount();
		if (( time - lastTime ) < lim) {
			Wait(lim - int(time - lastTime));
			time = GetTickCount();
		}
		lastTime = time;
	} else {
		lastTime = GetTickCount();
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

// Flips given sprite according to the flags. If MirrorAnchor=true,
// flips its anchor (i.e. origin/base point) as well
// returns new sprite
Holder<Sprite2D> Video::MirrorSprite(const Holder<Sprite2D> sprite, uint32_t flags, bool MirrorAnchor)
{
	if (!sprite)
		return NULL;

	Holder<Sprite2D> dest = sprite->copy();

	if (flags&BLIT_MIRRORX) {
		dest->renderFlags ^= BLIT_MIRRORX;
		if (MirrorAnchor)
			dest->Frame.x = sprite->Frame.w - sprite->Frame.x;
	}

	if (flags&BLIT_MIRRORY) {
		dest->renderFlags ^= BLIT_MIRRORY;
		if (MirrorAnchor)
			dest->Frame.y = sprite->Frame.h - sprite->Frame.y;
	}

	return dest;
}

/** Get the fullscreen mode */
bool Video::GetFullscreenMode() const
{
	return fullscreen;
}

void Video::BlitSprite(const Holder<Sprite2D> spr, int x, int y,
								const Region* clip)
{
	Region dst(x - spr->Frame.x, y - spr->Frame.y, spr->Frame.w, spr->Frame.h);
	Region fClip = ClippedDrawingRect(dst, clip);

	if (fClip.Dimensions().IsEmpty()) {
		return; // already know blit fails
	}

	Region src(0, 0, spr->Frame.w, spr->Frame.h);
	// adjust the src region to account for the clipping
	src.x += fClip.x - dst.x; // the left edge
	src.w -= dst.w - fClip.w; // the right edge
	src.y += fClip.y - dst.y; // the top edge
	src.h -= dst.h - fClip.h; // the bottom edge

	assert(src.w == fClip.w && src.h == fClip.h);

	// just pass fclip as dst
	// since the next stage is also public, we must readd the Pos becuase it will again be removed
	fClip.x += spr->Frame.x;
	fClip.y += spr->Frame.y;
	BlitSprite(spr, src, fClip);
}

void Video::BlitTiled(Region rgn, const Holder<Sprite2D> img)
{
	int xrep = ( rgn.w + img->Frame.w - 1 ) / img->Frame.w;
	int yrep = ( rgn.h + img->Frame.h - 1 ) / img->Frame.h;
	for (int y = 0; y < yrep; y++) {
		for (int x = 0; x < xrep; x++) {
			BlitSprite(img, rgn.x + (x*img->Frame.w),
				 rgn.y + (y*img->Frame.h), &rgn);
		}
	}
}

void Video::BlitGameSpriteWithPalette(Holder<Sprite2D> spr, PaletteHolder pal, int x, int y,
							   uint32_t flags, Color tint, const Region* clip)
{
	if (pal) {
		PaletteHolder oldpal = spr->GetPalette();
		spr->SetPalette(pal);
		BlitGameSprite(spr, x, y, flags, tint, clip);
		spr->SetPalette(oldpal);
	} else {
		BlitGameSprite(spr, x, y, flags, tint, clip);
	}
}

//Sprite conversion, creation
Holder<Sprite2D> Video::CreateAlpha( const Holder<Sprite2D> sprite)
{
	if (!sprite)
		return 0;

	unsigned int *pixels = (unsigned int *) malloc (sprite->Frame.w * sprite->Frame.h * 4);
	int i=0;
	for (int y = 0; y < sprite->Frame.h; y++) {
		for (int x = 0; x < sprite->Frame.w; x++) {
			int sum = 0;
			int cnt = 0;
			for (int xx=x-3;xx<=x+3;xx++) {
				for(int yy=y-3;yy<=y+3;yy++) {
					if (((xx==x-3) || (xx==x+3)) &&
					    ((yy==y-3) || (yy==y+3))) continue;
					if (xx < 0 || xx >= sprite->Frame.w) continue;
					if (yy < 0 || yy >= sprite->Frame.h) continue;
					cnt++;
					if (sprite->IsPixelTransparent(Point(xx, yy)))
						sum++;
				}
			}
			int tmp=255 - (sum * 255 / cnt);
			tmp = tmp * tmp / 255;
			pixels[i++]=tmp;
		}
	}
	Holder<Sprite2D> newspr = CreateSprite(sprite->Frame, 32, 0xFF000000,
											0x00FF0000, 0x0000FF00, 0x000000FF, pixels);
	newspr->renderFlags = sprite->renderFlags;
	return newspr;
}

Holder<Sprite2D> Video::SpriteScaleDown( const Holder<Sprite2D> sprite, unsigned int ratio )
{
	Region scaledFrame = sprite->Frame;
	scaledFrame.w /= ratio;
	scaledFrame.h /= ratio;

	unsigned int* pixels = (unsigned int *) malloc( scaledFrame.w * scaledFrame.h * 4 );
	int i = 0;

	for (int y = 0; y < scaledFrame.h; y++) {
		for (int x = 0; x < scaledFrame.w; x++) {
			Color c = SpriteGetPixelSum( sprite, x, y, ratio );

			*(pixels + i++) = c.r + (c.g << 8) + (c.b << 16) + (c.a << 24);
		}
	}

	Holder<Sprite2D> small = CreateSprite(scaledFrame, 32, 0x000000ff, 0x0000ff00, 0x00ff0000,
0xff000000, pixels, false, 0 );

	small->Frame.x = sprite->Frame.x / ratio;
	small->Frame.y = sprite->Frame.y / ratio;

	return small;
}

//TODO light could be elliptical in the original engine
//is it difficult?
Holder<Sprite2D> Video::CreateLight(int radius, int intensity)
{
	if(!radius) return NULL;
	Point p, q;
	int a;
	void* pixels = malloc( radius * radius * 4 * 4);
	int i = 0;

	for (p.y = -radius; p.y < radius; p.y++) {
		for (p.x = -radius; p.x < radius; p.x++) {
			a = intensity*(radius-(signed) Distance(p,q))/radius;

			if(a<0) a=0;
			else if(a>255) a = 255;

			*((unsigned int*)pixels + i++) = 0xffffff + ((a/2) << 24);
		}
	}

	Holder<Sprite2D> light = CreateSprite(Region(0,0, radius*2, radius*2), 32, 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000, pixels);

	light->Frame.x = radius;
	light->Frame.y = radius;

	return light;
}

Color Video::SpriteGetPixelSum(const Holder<Sprite2D> sprite, unsigned short xbase, unsigned short ybase, unsigned int ratio)
{
	// TODO: turn this into one of our software "shaders"
	Color sum;
	unsigned int count = ratio*ratio;
	unsigned int r=0, g=0, b=0, a=0;

	for (unsigned int x = 0; x < ratio; x++) {
		for (unsigned int y = 0; y < ratio; y++) {
			Color c = sprite->GetPixel( xbase*ratio+x, ybase*ratio+y );
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

Color ApplyFlagsForColor(const Color& inCol, uint32_t& flags)
{
	Color outC = inCol;
	if (flags & BLIT_HALFTRANS) {
		// set exactly to 128 because it is an optimized value
		// if we end up needing to do half of something already transparent we can change this
		// or do the calculations before calling the video driver and dont pass BLIT_HALFTRANS
		outC.a = 128;
	}

	// TODO: do we need to handle BLIT_GREY, BLIT_SEPIA, or BLIT_TINTED?
	// if so we should do that here instead of in the implementations

	if (flags & BLIT_GREY) {
		//static RGBBlendingPipeline<GREYSCALE, true> blender;
	} else if (flags & BLIT_SEPIA) {
		//static RGBBlendingPipeline<SEPIA, true> blender;
	}

	if (flags & BLIT_TINTED) {
		// FIXME: we would need another parameter for tinting the color
	}

	// clear handled flags
	flags &= ~(BLIT_HALFTRANS|BLIT_GREY|BLIT_SEPIA|BLIT_TINTED);
	return outC;
}

void Video::DrawRect(const Region& rgn, const Color& color, bool fill, uint32_t flags)
{
	Color c = ApplyFlagsForColor(color, flags);
	DrawRectImp(rgn, c, fill, flags);
}

void Video::DrawPoint(const Point& p, const Color& color, uint32_t flags)
{
	Color c = ApplyFlagsForColor(color, flags);
	DrawPointImp(p, c, flags);
}

void Video::DrawPoints(const std::vector<Point>& points, const Color& color, uint32_t flags)
{
	Color c = ApplyFlagsForColor(color, flags);
	DrawPointsImp(points, c, flags);
}

void Video::DrawCircle(const Point& origin, unsigned short r, const Color& color, uint32_t flags)
{
	Color c = ApplyFlagsForColor(color, flags);
	DrawCircleImp(origin, r, c, flags);
}

void Video::DrawEllipseSegment(const Point& origin, unsigned short xr, unsigned short yr, const Color& color,
								double anglefrom, double angleto, bool drawlines, uint32_t flags)
{
	Color c = ApplyFlagsForColor(color, flags);
	DrawEllipseSegmentImp(origin, xr, yr, c, anglefrom, angleto, drawlines, flags);
}

void Video::DrawEllipse(const Point& origin, unsigned short xr, unsigned short yr, const Color& color, uint32_t flags)
{
	Color c = ApplyFlagsForColor(color, flags);
	DrawEllipseImp(origin, xr, yr, c, flags);
}

void Video::DrawPolygon(const Gem_Polygon* poly, const Point& origin, const Color& color, bool fill, uint32_t flags)
{
	Color c = ApplyFlagsForColor(color, flags);
	DrawPolygonImp(poly, origin, c, fill, flags);
}

void Video::DrawLine(const Point& p1, const Point& p2, const Color& color, uint32_t flags)
{
	Color c = ApplyFlagsForColor(color, flags);
	DrawLineImp(p1, p2, c, flags);
}

void Video::DrawLines(const std::vector<Point>& points, const Color& color, uint32_t flags)
{
	Color c = ApplyFlagsForColor(color, flags);
	DrawLinesImp(points, c, flags);
}

}
