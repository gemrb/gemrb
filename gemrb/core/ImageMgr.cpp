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
	Bitmap *data = new Bitmap(size);

	Log(ERROR, "ImageMgr", "Don't know how to handle 24bit bitmap from %s...",
		str->filename );

	Holder<Sprite2D> spr = GetSprite2D();

	for (int y = 0; y < size.h; y++) {
		for (int x = 0; x < size.w; x++) {
			const Point p(x, y);
			data->SetAt(p, spr->GetPixel(p).r);
		}
	}

	return data;
}

Image* ImageMgr::GetImage()
{
	Image *data = new Image(size);

	Holder<Sprite2D> spr = GetSprite2D();

	for (int y = 0; y < size.h; y++) {
		for (int x = 0; x < size.w; x++) {
			const Point p(x, y);
			data->SetPixel(p, spr->GetPixel(p));
		}
	}

	return data;
}

int ImageMgr::GetPalette(int /*colors*/, Color* /*pal*/)
{
	return -1;
}

ImageFactory* ImageMgr::GetImageFactory(const char* ResRef)
{
	ImageFactory* fact = new ImageFactory( ResRef, GetSprite2D() );
	return fact;
}

}
