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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/ImageMgr.cpp,v 1.7 2005/10/18 22:43:41 edheldil Exp $
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

// FIXME: should be abstract, but I don't want to implement it for all
// image managers right now
bool ImageMgr::OpenFromImage(Sprite2D* /*sprite*/, bool /*autoFree*/)
{
	return false;
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

// FIXME: should be abstract, but I don't want to implement it for all
// image managers right now
void ImageMgr::PutImage(DataStream* /*output*/, unsigned int /*ratio*/)
{
}
