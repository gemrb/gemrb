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

Video::Video(void)
	: fadeColor()
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

VideoBuffer* Video::CreateBuffer(const Region& r, BufferFormat fmt)
{
	VideoBuffer* buf = NewVideoBuffer(r, fmt);
	assert(buf); // FIXME: we should probably deal with this happening
	buffers.push_back(buf);
	return buffers.back();
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

void Video::PushDrawingBuffer(VideoBuffer* buf)
{
	assert(buf);
	drawingBuffers.push_back(buf);
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

// Flips given sprite vertically (up-down). If MirrorAnchor=true,
// flips its anchor (i.e. origin//base point) as well
// returns new sprite
Sprite2D* Video::MirrorSpriteVertical(const Sprite2D* sprite, bool MirrorAnchor)
{
	if (!sprite)
		return NULL;

	Sprite2D* dest = sprite->copy();

	const void* src = sprite->LockSprite();
	void* buffer = dest->LockSprite();

	if (src != buffer) {
		assert(!sprite->BAM);
		// if the sprite pixel buffers are not the same we need to manually mirror the pixels
		for (int x = 0; x < dest->Width; x++) {
			unsigned char * dst = ( unsigned char * )buffer  + x;
			unsigned char * src = dst + ( dest->Height - 1 ) * dest->Width;
			for (int y = 0; y < dest->Height / 2; y++) {
				unsigned char swp = *dst;
				*dst = *src;
				*src = swp;
				dst += dest->Width;
				src -= dest->Width;
			}
		}
	} else {
		// if the pixel buffers are the same then either there are no pixels (NULL)
		// or the sprites support sharing pixel data and we only need to set a render flag on the copy
		// toggle the bit because it could be a mirror of a mirror
		dest->renderFlags ^= BLIT_MIRRORY;
	}

	sprite->UnlockSprite();
	dest->UnlockSprite();

	if (MirrorAnchor)
		dest->YPos = sprite->Height - sprite->YPos;

	return dest;
}

// Flips given sprite horizontally (left-right). If MirrorAnchor=true,
//   flips its anchor (i.e. origin//base point) as well
Sprite2D* Video::MirrorSpriteHorizontal(const Sprite2D* sprite, bool MirrorAnchor)
{
	if (!sprite)
		return NULL;

	Sprite2D* dest = sprite->copy();

	const void* src = sprite->LockSprite();
	void* buffer = dest->LockSprite();

	if (src != buffer) {
		assert(!sprite->BAM);
		// if the sprite pixel buffers are not the same we need to manually mirror the pixels
		for (int y = 0; y < dest->Height; y++) {
			unsigned char * dst = (unsigned char *) buffer + ( y * dest->Width );
			unsigned char * src = dst + dest->Width - 1;
			for (int x = 0; x < dest->Width / 2; x++) {
				unsigned char swp=*dst;
				*dst++ = *src;
				*src-- = swp;
			}
		}
	} else {
		// if the pixel buffers are the same then either there are no pixels (NULL)
		// or the sprites support sharing pixel data and we only need to set a render flag on the copy
		// toggle the bit because it could be a mirror of a mirror
		dest->renderFlags ^= BLIT_MIRRORX;
	}

	sprite->UnlockSprite();
	dest->UnlockSprite();

	if (MirrorAnchor)
		dest->XPos = sprite->Width - sprite->XPos;

	return dest;
}

/** Get the fullscreen mode */
bool Video::GetFullscreenMode() const
{
	return fullscreen;
}

void Video::BlitSprite(const Sprite2D* spr, int x, int y,
								const Region* clip)
{
	Region dst(x - spr->XPos, y - spr->YPos, spr->Width, spr->Height);
	Region fClip = ClippedDrawingRect(dst, clip);

	if (fClip.Dimensions().IsEmpty()) {
		return; // already know blit fails
	}

	Region src(0, 0, spr->Width, spr->Height);
	// adjust the src region to account for the clipping
	src.x += fClip.x - dst.x; // the left edge
	src.w -= dst.w - fClip.w; // the right edge
	src.y += fClip.y - dst.y; // the top edge
	src.h -= dst.h - fClip.h; // the bottom edge

	assert(src.w == fClip.w && src.h == fClip.h);

	// just pass fclip as dst
	BlitSprite(spr, src, fClip);
}

void Video::BlitTiled(Region rgn, const Sprite2D* img)
{
	int xrep = ( rgn.w + img->Width - 1 ) / img->Width;
	int yrep = ( rgn.h + img->Height - 1 ) / img->Height;
	for (int y = 0; y < yrep; y++) {
		for (int x = 0; x < xrep; x++) {
			BlitSprite(img, rgn.x + (x*img->Width),
				 rgn.y + (y*img->Height), &rgn);
		}
	}
}

void Video::BlitGameSpriteWithPalette(Sprite2D* spr, Palette* pal, int x, int y,
							   unsigned int flags, Color tint,
							   SpriteCover* cover,
							   const Region* clip)
{
	if (pal) {
		Palette* oldpal = spr->GetPalette();
		spr->SetPalette(pal);
		BlitGameSprite(spr, x, y, flags, tint, cover, clip);
		spr->SetPalette(oldpal);
		oldpal->release();
	} else {
		BlitGameSprite(spr, x, y, flags, tint, cover, clip);
	}
}

//Sprite conversion, creation
Sprite2D* Video::CreateAlpha( const Sprite2D *sprite)
{
	if (!sprite)
		return 0;

	unsigned int *pixels = (unsigned int *) malloc (sprite->Width * sprite->Height * 4);
	int i=0;
	for (int y = 0; y < sprite->Height; y++) {
		for (int x = 0; x < sprite->Width; x++) {
			int sum = 0;
			int cnt = 0;
			for (int xx=x-3;xx<=x+3;xx++) {
				for(int yy=y-3;yy<=y+3;yy++) {
					if (((xx==x-3) || (xx==x+3)) &&
					    ((yy==y-3) || (yy==y+3))) continue;
					if (xx < 0 || xx >= sprite->Width) continue;
					if (yy < 0 || yy >= sprite->Height) continue;
					cnt++;
					if (sprite->IsPixelTransparent(xx, yy))
						sum++;
				}
			}
			int tmp=255 - (sum * 255 / cnt);
			tmp = tmp * tmp / 255;
			pixels[i++]=tmp;
		}
	}
	return CreateSprite( sprite->Width, sprite->Height, 32, 0xFF000000,
		0x00FF0000, 0x0000FF00, 0x000000FF, pixels );
}

Sprite2D* Video::SpriteScaleDown( const Sprite2D* sprite, unsigned int ratio )
{
	unsigned int Width = sprite->Width / ratio;
	unsigned int Height = sprite->Height / ratio;

	unsigned int* pixels = (unsigned int *) malloc( Width * Height * 4 );
	int i = 0;

	for (unsigned int y = 0; y < Height; y++) {
		for (unsigned int x = 0; x < Width; x++) {
			Color c = SpriteGetPixelSum( sprite, x, y, ratio );

			*(pixels + i++) = c.r + (c.g << 8) + (c.b << 16) + (c.a << 24);
		}
	}

	Sprite2D* small = CreateSprite( Width, Height, 32, 0x000000ff, 0x0000ff00, 0x00ff0000,
0xff000000, pixels, false, 0 );

	small->XPos = sprite->XPos / ratio;
	small->YPos = sprite->YPos / ratio;

	return small;
}

//TODO light could be elliptical in the original engine
//is it difficult?
Sprite2D* Video::CreateLight(int radius, int intensity)
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

	Sprite2D* light = CreateSprite( radius*2, radius*2, 32, 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000, pixels);

	light->XPos = radius;
	light->YPos = radius;

	return light;
}

Color Video::SpriteGetPixelSum(const Sprite2D* sprite, unsigned short xbase, unsigned short ybase, unsigned int ratio)
{
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



}
