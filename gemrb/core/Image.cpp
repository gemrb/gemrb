/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2007 The GemRB Project
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "Image.h"

#include "Interface.h"
#include "Video.h"

namespace GemRB {

Image::Image(const Size& size)
: size(size), data(new Color[size.h * size.w])
{
}

Image::~Image()
{
	delete[] data;
}

Holder<Sprite2D> Image::GetSprite2D()
{
	static const PixelFormat fmt(4, 0xff000000, 0x00ff0000, 0x0000ff00, 0x000000ff);
	void *pixels = malloc(sizeof(Color) * size.h * size.w);
	memcpy(pixels, data, sizeof(Color) * size.h * size.w);
	return core->GetVideoDriver()->CreateSprite(Region(0,0, size.w, size.h), pixels, fmt);
}

}
