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

#ifndef BITMAP_H
#define BITMAP_H

#include "exports.h"

namespace GemRB {

class GEM_EXPORT Bitmap {
public:
	Bitmap(unsigned int height, unsigned int width);
	~Bitmap();
	unsigned char GetAt(unsigned int x, unsigned int y) const
	{
		if (x >= width || y >= height)
			return 0;
		return data[width*y+x];

	}
	void SetAt(unsigned int x, unsigned int y, unsigned char idx)
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
private:
	unsigned int height, width;
	unsigned char *data;
};

}

#endif
