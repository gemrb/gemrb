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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/BMPImporter/BMPImp.h,v 1.9 2003/11/30 23:26:46 balrog994 Exp $
 *
 */

#ifndef BMPIMP_H
#define BMPIMP_H

#include "../Core/ImageMgr.h"

class BMPImp : public ImageMgr
{
private:
	DataStream * str;
	bool autoFree;

	//BITMAPINFOHEADER
	unsigned long Size, Width, Height, Compression, ImageSize, ColorsUsed, ColorsImportant;
	unsigned short Planes, BitCount;

	//COLORTABLE
	unsigned long NumColors;
	Color * Palette;

	//RASTERDATA
	void * pixels;

	//OTHER
	unsigned short PaddedRowLength;
public:
	BMPImp(void);
	~BMPImp(void);
	bool Open(DataStream * stream, bool autoFree = true);
	Sprite2D * GetImage();
	/** No descriptions */
	void GetPalette(int index, int colors, Color * pal);
	unsigned long GetPixelIndex(int x, int y)
	{
		unsigned char *p=(unsigned char *)pixels;
		if(BitCount == 4)
			p+=(PaddedRowLength*y)+(x>>1);
		else
			p+=(PaddedRowLength*y)+(x*(BitCount/8));
		if(BitCount == 24) {
			int ret = *(unsigned long *) p;
			return ret|0xff000000;
		}
		else if(BitCount == 8) {
			return (unsigned long) *p;
		}
		else if(BitCount == 4) {
			unsigned char ret=*p;
			if(x&1) {
				return ret&15;
			}
			else {
				return (ret>>4)&15;
			}
		}
	}
	/** Gets a Pixel from the Image */
	Color GetPixel(int x, int y)
	{
		Color ret;
		unsigned char * p = (unsigned char*)pixels;
		if(BitCount == 4)
			p+=(PaddedRowLength*y)+(x>>1);
		else
			p+=(PaddedRowLength*y)+(x*(BitCount/8));
		if(BitCount == 24) {
			ret.b = *p++;
			ret.g = *p++;
			ret.r = *p++;
			ret.a = 0xff;
		}
		else if(BitCount == 8) {
			ret.r = Palette[*p].r;
			ret.g = Palette[*p].g;
			ret.b = Palette[*p].b;
			ret.a = 0xff;
		}
		else if(BitCount == 4) {
			int tmp=(unsigned long) *p;
			if(x&1)
				tmp&=15;
			else
				tmp=(tmp>>4)&15;
			ret.r = Palette[tmp].r;
			ret.g = Palette[tmp].g;
			ret.b = Palette[tmp].b;
			ret.a = 0xff;
		}
		return ret;
	}
public:
	void release(void)
	{
		delete this;
	}
};

#endif
