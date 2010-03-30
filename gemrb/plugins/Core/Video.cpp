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

#include "../../includes/win32def.h"
#include "Video.h"
#include "Palette.h"
#include "Audio.h"
#include "Interface.h"

Video::Video(void)
{
	Evnt = NULL;
}

Video::~Video(void)
{
}

/** Set Event Manager */
void Video::SetEventMgr(EventMgr* evnt)
{
	//if 'evnt' is NULL then no Event Manager will be used
	Evnt = evnt;
}

/** Mouse is invisible and cannot interact */
void Video::SetMouseEnabled(int enabled)
{
	DisableMouse = enabled^MOUSE_DISABLED;
}

/** Mouse cursor is grayed and doesn't click (but visible and movable) */
void Video::SetMouseGrayed(bool grayed)
{
	if (grayed) {
		DisableMouse |= MOUSE_GRAYED;
	} else {
		DisableMouse &= ~MOUSE_GRAYED;
	}
}

void Video::BlitTiled(Region rgn, Sprite2D* img, bool anchor)
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
Sprite2D* Video::CreateAlpha( Sprite2D *sprite)
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

Sprite2D* Video::SpriteScaleDown( Sprite2D* sprite, unsigned int ratio )
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

Color Video::SpriteGetPixelSum(Sprite2D* sprite, unsigned short xbase, unsigned short ybase, unsigned int ratio)
{
	Color sum;
	unsigned int count = ratio*ratio;
	unsigned int r=0, g=0, b=0, a=0;

	for (unsigned int x = 0; x < ratio; x++) {
		for (unsigned int y = 0; y < ratio; y++) {
			Color c = sprite->GetPixel( xbase*ratio+x, ybase*ratio+y );
			r += c.r;
			g += c.g;
			b += c.b;
			a += c.a;
		}
	}

	sum.r = r / count;
	sum.g = g / count;
	sum.b = b / count;
	sum.a = a / count;

	return sum;
}

//Viewport specific
Region Video::GetViewport()
{
	return Viewport;
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

