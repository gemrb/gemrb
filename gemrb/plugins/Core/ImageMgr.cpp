/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/ImageMgr.cpp,v 1.6 2005/08/20 16:54:56 avenger_teambg Exp $
 *
 */

#include "../../includes/win32def.h"
#include "ImageMgr.h"

ImageMgr::ImageMgr(void)
{
}

ImageMgr::~ImageMgr(void)
{
}

static int GetScanLineLength(int width, int bitsperpixel)
{
	int paddedwidth;

	paddedwidth=(width*bitsperpixel+7)/8;
	return paddedwidth;
}

Color ImageMgr::GetPixelSum(unsigned int xbase, unsigned int ybase, unsigned int ratio)
{
	Color sum;
	unsigned int count = ratio*ratio;
	unsigned int r=0,g=0,b=0;

	for(unsigned int x=0;x<ratio;x++) {
		for(unsigned int y=0;y<ratio;y++) {
			Color c = GetPixel(xbase*ratio+x,ybase*ratio+y);
			r+=c.r;
			g+=c.g;
			b+=c.b;
		}
	}
	sum.r=r/count;
	sum.g=g/count;
	sum.b=b/count;
	sum.a=0;
	return sum;
}

/* this is here because we have to provide .bmp images from any image type*/
void ImageMgr::PutImage(DataStream *output, unsigned int ratio)
{
	ieDword tmpDword;
	ieWord tmpWord;

	if(ratio<1) {
		ratio=1;
	}
	ieDword Width = GetWidth()/ratio;
	ieDword Height = GetHeight()/ratio;
	char filling[3] = {'B','M'};
	ieDword PaddedRowLength = GetScanLineLength(Width,24);
	int stuff = (4-(PaddedRowLength&3))&3; // rounding it up to 4 bytes boundary
	PaddedRowLength+=stuff;
	ieDword fullsize = PaddedRowLength*Height;

	//always save in truecolor (24 bit), no palette
	output->Write( filling, 2);
	tmpDword = fullsize+BMP_HEADER_SIZE;
	output->WriteDword( &tmpDword);
	tmpDword = 0;
	output->WriteDword( &tmpDword);
	tmpDword = BMP_HEADER_SIZE;
	output->WriteDword( &tmpDword);
	tmpDword = 40;
	output->WriteDword( &tmpDword);
	output->WriteDword( &Width);
	output->WriteDword( &Height);
	tmpWord = 1;
	output->WriteWord( &tmpWord);
	tmpWord = 24; //24 bits
	output->WriteWord( &tmpWord);
	tmpDword = 0;
	output->WriteDword( &tmpDword);
	output->WriteDword( &tmpDword);
	output->WriteDword( &tmpDword);
	output->WriteDword( &tmpDword);
	output->WriteDword( &tmpDword);
	output->WriteDword( &tmpDword);
	
	memset( filling,0,sizeof(filling) );
	for (unsigned int y=0;y<Height;y++) {
		for (unsigned int x=0;x<Width;x++) {
			Color c = GetPixelSum(x,Height-y-1,ratio);
			//Color c = GetPixel(x,Height-y-1);

			output->Write( &c.b, 1);
			output->Write( &c.g, 1);
			output->Write( &c.r, 1);
		}
		output->Write( filling, stuff);
	}
}
