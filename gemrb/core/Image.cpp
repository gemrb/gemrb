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

Image::Image(unsigned int w, unsigned int h)
	: height(h), width(w), data(new Color[height*width])
{
}

Image::~Image()
{
	delete[] data;
}

Sprite2D* Image::GetSprite2D()
{
	union {
		Color color;
		ieDword Mask;
	} r = {{ 0xFF, 0x00, 0x00, 0x00 }},
	  g = {{ 0x00, 0xFF, 0x00, 0x00 }},
	  b = {{ 0x00, 0x00, 0xFF, 0x00 }},
	  a = {{ 0x00, 0x00, 0x00, 0xFF }};
	void *pixels = malloc(sizeof(Color) * height*width);
	memcpy(pixels, data, sizeof(Color)*height*width);
	return core->GetVideoDriver()->CreateSprite(width, height, 32,
			r.Mask, g.Mask, b.Mask, a.Mask, pixels);
}

}
