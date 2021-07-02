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

#include "Region.h"

namespace GemRB {

class GEM_EXPORT Bitmap {
public:
	Bitmap(const Size& size);
	~Bitmap();
	unsigned char GetAt(const Point& p) const
	{
		if (p.x >= size.w || p.y >= size.h)
			return 0;
		return data[size.w * p.y + p.x];

	}
	void SetAt(const Point& p, unsigned char idx)
	{
		if (p.x >= size.w || p.y >= size.h)
			return;
		data[size.w * p.y + p.x] = idx;

	}
	
	Size GetSize() const
	{
		return size;
	}

	void dump() const;
private:
	Size size;
	unsigned char *data;
};

}

#endif
