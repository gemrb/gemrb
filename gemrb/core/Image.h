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

#ifndef IMAGE_H
#define IMAGE_H

#include "RGBAColor.h"
#include "exports.h"

namespace GemRB {

class Sprite2D;

class GEM_EXPORT Image {
public:
	Image(unsigned int height, unsigned int width);
	~Image();
	Color GetPixel(unsigned int x, unsigned int y) const
	{
		if (x >= width || y >= height) {
			return ColorBlack;
		}
		return data[width*y+x];

	}
	void SetPixel(unsigned int x, unsigned int y, Color idx)
	{
		if (x >= width || y >= height)
			return;
		data[width*y+x] = idx;

	}
	unsigned int GetHeight() const
	{
		return height;
	}
	unsigned int GetWidth() const
	{
		return width;
	}
	Sprite2D *GetSprite2D();
private:
	unsigned int height, width;
	Color *data;
};

}

#endif
