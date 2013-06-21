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
	: Viewport(), CursorPos(), fadeColor()
{
	CursorIndex = VID_CUR_UP;
	Cursor[VID_CUR_UP] = NULL;
	Cursor[VID_CUR_DOWN] = NULL;
	Cursor[VID_CUR_DRAG] = NULL;

	EvntManager = NULL;
	// MOUSE_GRAYED and MOUSE_DISABLED are the first 2 bits so shift the config value away from those.
	// we care only about 2 bits at the moment so mask out the remainder
	MouseFlags = ((core->MouseFeedback & 0x3) << 2);

	// Initialize gamma correction tables
	for (int i = 0; i < 256; i++) {
		Gamma22toGamma10[i] = (unsigned char)(0.5 + (pow (i/255.0, 2.2/1.0) * 255.0));
		Gamma10toGamma22[i] = (unsigned char)(0.5 + (pow (i/255.0, 1.0/2.2) * 255.0));
	}
}

Video::~Video(void)
{
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

void Video::SetCursor(Sprite2D* cur, enum CursorType curIdx)
{
	if (cur) {
		//cur will be assigned in the end, increase refcount
		cur->acquire();
		//setting a dragged sprite cursor, it will 'stick' until cleared
		if (curIdx == VID_CUR_DRAG)
			CursorIndex = VID_CUR_DRAG;
	} else {
		//clearing the dragged sprite cursor, replace it with the normal cursor
		if (curIdx == VID_CUR_DRAG)
			CursorIndex = VID_CUR_UP;
	}
	//decrease refcount of the previous cursor
	if (Cursor[curIdx])
		FreeSprite(Cursor[curIdx]);
	Cursor[curIdx] = cur;
}

/** Mouse is invisible and cannot interact */
void Video::SetMouseEnabled(int enabled)
{
	if (enabled) {
		MouseFlags &= ~MOUSE_DISABLED;
	} else {
		MouseFlags |= MOUSE_DISABLED;
	}
}

/** Mouse cursor is grayed and doesn't click (but visible and movable) */
void Video::SetMouseGrayed(bool grayed)
{
	if (grayed) {
		MouseFlags |= MOUSE_GRAYED;
	} else {
		MouseFlags &= ~MOUSE_GRAYED;
	}
}

/** Get the fullscreen mode */
bool Video::GetFullscreenMode() const
{
	return fullscreen;
}

void Video::BlitTiled(Region rgn, const Sprite2D* img, bool anchor)
{
	int xrep = ( rgn.w + img->Width - 1 ) / img->Width;
	int yrep = ( rgn.h + img->Height - 1 ) / img->Height;
	for (int y = 0; y < yrep; y++) {
		for (int x = 0; x < xrep; x++) {
			BlitSprite(img, rgn.x + (x*img->Width),
				 rgn.y + (y*img->Height), anchor, &rgn);
		}
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

//Viewport specific
Region Video::GetViewport() const
{
	return Viewport;
}

void Video::SetMovieFont(Font *stfont, Palette *pal)
{
	subtitlefont = stfont;
	subtitlepal = pal;
}

void Video::SetViewport(int x, int y, unsigned int w, unsigned int h)
{
	if (x>width)
		x=width;
	xCorr = x;
	if (y>height)
		y=height;
	yCorr = y;
	if (w>(unsigned int) width)
		w=0;
	Viewport.w = w;
	if (h>(unsigned int) height)
		h=0;
	Viewport.h = h;
}

void Video::MoveViewportTo(int x, int y)
{
	if (x != Viewport.x || y != Viewport.y) {
		core->GetAudioDrv()->UpdateListenerPos( (x - xCorr) + width / 2, (y - yCorr)
+ height / 2 );
		Viewport.x = x;
		Viewport.y = y;
	}
}

void Video::FreeSprite(Sprite2D*& spr)
{
	if (spr) {
		spr->release();
		spr = NULL;
	}
}

void Video::InitSpriteCover(SpriteCover* sc, int flags)
{
	int i;
	sc->flags = flags;
	sc->pixels = new unsigned char[sc->Width * sc->Height];
	for (i = 0; i < sc->Width*sc->Height; ++i)
		sc->pixels[i] = 0;
	
}

// flags: 0 - never dither (full cover)
//	1 - dither if polygon wants it
//	2 - always dither
void Video::AddPolygonToSpriteCover(SpriteCover* sc, Wall_Polygon* poly)
{
	
	// possible TODO: change the cover to use a set of intervals per line?
	// advantages: faster
	// disadvantages: makes the blitter much more complex
	
	int xoff = sc->worldx - sc->XPos;
	int yoff = sc->worldy - sc->YPos;
	
	std::list<Trapezoid>::iterator iter;
	for (iter = poly->trapezoids.begin(); iter != poly->trapezoids.end();
		 ++iter)
	{
		int y_top = iter->y1 - yoff; // inclusive
		int y_bot = iter->y2 - yoff; // exclusive
		
		if (y_top < 0) y_top = 0;
		if ( y_bot > sc->Height) y_bot = sc->Height;
		if (y_top >= y_bot) continue; // clipped
		
		int ledge = iter->left_edge;
		int redge = iter->right_edge;
		Point& a = poly->points[ledge];
		Point& b = poly->points[(ledge+1)%(poly->count)];
		Point& c = poly->points[redge];
		Point& d = poly->points[(redge+1)%(poly->count)];
		
		unsigned char* line = sc->pixels + (y_top)*sc->Width;
		for (int sy = y_top; sy < y_bot; ++sy) {
			int py = sy + yoff;
			
			// TODO: maybe use a 'real' line drawing algorithm to
			// compute these values faster.
			
			int lt = (b.x * (py - a.y) + a.x * (b.y - py))/(b.y - a.y);
			int rt = (d.x * (py - c.y) + c.x * (d.y - py))/(d.y - c.y) + 1;
			
			lt -= xoff;
			rt -= xoff;
			
			if (lt < 0) lt = 0;
			if (rt > sc->Width) rt = sc->Width;
			if (lt >= rt) { line += sc->Width; continue; } // clipped
			int dither;
			
			if (sc->flags == 1) {
				dither = poly->wall_flag & WF_DITHER;
			} else {
				dither = sc->flags;
			}
			if (dither) {
				unsigned char* pix = line + lt;
				unsigned char* end = line + rt;
				
				if ((lt + xoff + sy + yoff) % 2) pix++; // CHECKME: aliasing?
				for (; pix < end; pix += 2)
					*pix = 1;
			} else {
				// we hope memset is faster
				// condition: lt < rt is true
				memset (line+lt, 1, rt-lt);
			}
			line += sc->Width;
		}
	}
}

void Video::DestroySpriteCover(SpriteCover* sc)
{
	delete[] sc->pixels;
	sc->pixels = NULL;
}

void Video::GetMousePos(int &x, int &y)
{
	x = CursorPos.x;
	y = CursorPos.y;
}

}
