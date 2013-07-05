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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

#include "ImageMgr.h"

#include "win32def.h"

#include "ImageFactory.h"
#include "Interface.h"

namespace GemRB {

const TypeID ImageMgr::ID = { "ImageMgr" };

ImageMgr::ImageMgr(void)
{
}

ImageMgr::~ImageMgr(void)
{
}

Bitmap* ImageMgr::GetBitmap()
{
	unsigned int height = GetHeight();
	unsigned int width = GetWidth();
	Bitmap *data = new Bitmap(width, height);

	Log(ERROR, "ImageMgr", "Don't know how to handle 24bit bitmap from %s...",
		str->filename );

	Sprite2D *spr = GetSprite2D();

	for (unsigned int y = 0; y < height; y++) {
		for (unsigned int x = 0; x < width; x++) {
			data->SetAt(x,y, spr->GetPixel(x,y).r);
		}
	}

	spr->release();

	return data;
}

Image* ImageMgr::GetImage()
{
	unsigned int height = GetHeight();
	unsigned int width = GetWidth();
	Image *data = new Image(width, height);

	Sprite2D *spr = GetSprite2D();

	for (unsigned int y = 0; y < height; y++) {
		for (unsigned int x = 0; x < width; x++) {
			data->SetPixel(x,y, spr->GetPixel(x,y));
		}
	}

	spr->release();

	return data;
}

void ImageMgr::GetPalette(int /*colors*/, Color* /*pal*/)
{
	Log(ERROR, "ImageMgr", "Can't get non-existant palette from %s... ",
		str->filename);
}

ImageFactory* ImageMgr::GetImageFactory(const char* ResRef)
{
	ImageFactory* fact = new ImageFactory( ResRef, GetSprite2D() );
	return fact;
}

}
