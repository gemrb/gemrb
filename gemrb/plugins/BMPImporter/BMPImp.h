/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/BMPImporter/BMPImp.h,v 1.18 2005/10/18 22:43:41 edheldil Exp $
 *
 */

#ifndef BMPIMP_H
#define BMPIMP_H

#include "../Core/ImageMgr.h"

class BMPImp : public ImageMgr {
private:
	DataStream* str;
	bool autoFree;

	//BITMAPINFOHEADER
	ieDword Size, Width, Height, Compression, ImageSize, ColorsUsed, ColorsImportant;
	ieWord Planes, BitCount;

	//COLORTABLE
	ieDword NumColors;
	Color* Palette;

	//RASTERDATA
	void* pixels;

	//OTHER
	unsigned int PaddedRowLength;
public:
	BMPImp(void);
	~BMPImp(void);
	bool Open(DataStream* stream, bool autoFree = true);
	bool OpenFromImage(Sprite2D* sprite, bool autoFree = true);
	Sprite2D* GetImage();
	void PutImage(DataStream *output, unsigned int ratio);
	/** No descriptions */
	void GetPalette(int index, int colors, Color* pal);
	/** Searchmap only */
	void SetPixelIndex(unsigned int x, unsigned int y, int idx)
	{
		if(x>=Width || y>=Height) {
			return;
		}
		if (BitCount != 4) {
			return;
		}
		unsigned char * p = ( unsigned char * ) pixels;
		p += ( PaddedRowLength * y ) + ( x >> 1 );
		if (x&1) {
			*p &= ~15;
			*p |= (unsigned char) (idx&15);
		}
		else {
			*p &= 15;
			*p |= (unsigned char) (idx<<4);
		}
	}
	unsigned int GetPixelIndex(unsigned int x, unsigned int y)
	{
		if(x>=Width || y>=Height) {
			return 0;
		}
		unsigned char * p = ( unsigned char * ) pixels;
		if (BitCount == 4) {
			p += ( PaddedRowLength * y ) + ( x >> 1 );
		} else {
			p += ( Width * y +  x) * ( BitCount / 8 );
		}
		if (BitCount == 24) {
			unsigned int ret = *( unsigned int* ) p;
			return ret | 0xff000000;
		} else if (BitCount == 8) {
			return ( unsigned int ) * p;
		} else if (BitCount == 4) {
			unsigned char ret = *p;
			if (x & 1) {
				return ret & 15;
			} else {
				return ( ret >> 4 ) & 15;
			}
		}
		return 0;
	}
	/** Gets a Pixel from the Image */
	Color GetPixel(unsigned int x, unsigned int y)
	{
		Color ret = {0,0,0,0};

		if(x>=Width || y>=Height) {
			return ret;
		}
		unsigned char * p = ( unsigned char * ) pixels;
		if (BitCount == 4) {
			p += ( PaddedRowLength * y ) + ( x >> 1 );
		} else {
			p += ( Width * y +  x) * ( BitCount / 8 );
		}
		if (BitCount == 24) {
			ret.b = *p++;
			ret.g = *p++;
			ret.r = *p++;
			ret.a = 0xff;
		} else if (BitCount == 8) {
			ret.r = Palette[*p].r;
			ret.g = Palette[*p].g;
			ret.b = Palette[*p].b;
			ret.a = 0xff;
		} else if (BitCount == 4) {
			unsigned int tmp = ( unsigned int ) * p;
			if (x & 1)
				tmp &= 15;
			else
				tmp = ( tmp >> 4 ) & 15;
			ret.r = Palette[tmp].r;
			ret.g = Palette[tmp].g;
			ret.b = Palette[tmp].b;
			ret.a = 0xff;
		}
		return ret;
	}
	int GetWidth() { return (int) Width; }
	int GetHeight() { return (int) Height; }
public:
	void release(void)
	{
		delete this;
	}
	int GetCycleCount()
	{
		return 1;
	}
};

#endif
