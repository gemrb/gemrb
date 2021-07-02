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

#include "Bitmap.h"
#include "System/Logging.h"
#include "System/StringBuffer.h"

namespace GemRB {

Bitmap::Bitmap(const Size& size)
: size(size), data(new unsigned char[size.Area()])
{
}

Bitmap::~Bitmap()
{
	delete[] data;
}

void Bitmap::dump() const
{
	StringBuffer lines;
	lines.appendFormatted("height: %d, width: %d\n", size.h, size.w);
	for (int y = 0; y < size.h; ++y) {
		for (int x = 0; x < size.w; ++x) {
			lines.appendFormatted("%d ", data[size.w * y + x]);
		}
		lines.append("\n");
	}
	Log(DEBUG, "Bitmap", lines);
}

}
