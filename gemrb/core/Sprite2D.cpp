/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

#include "Sprite2D.h"

#include "win32def.h"

#include "Interface.h"

namespace GemRB {

const TypeID Sprite2D::ID = { "Sprite2D" };

Sprite2D::Sprite2D(int Width, int Height, int Bpp, const void* pixels)
	: Width(Width), Height(Height), Bpp(Bpp), pixels(pixels)
{
	freePixels = (bool)(pixels);
	BAM = false;
	RLE = false;
	XPos = 0;
	YPos = 0;
	RefCount = 1;
	renderFlags = 0;
}

Sprite2D::Sprite2D(const Sprite2D &obj)
{
	BAM = false;
	RLE = false;
	RefCount = 1;

	XPos = obj.XPos;
	YPos = obj.YPos;
	Width = obj.Width;
	Height = obj.Height;
	Bpp = obj.Bpp;
	renderFlags = obj.renderFlags;

	pixels = obj.pixels;
	freePixels = false;
}

Sprite2D::~Sprite2D()
{
	if (freePixels) {
		// FIXME: casting away const.
		free(const_cast<void*>(pixels));
	}
}

bool Sprite2D::IsPixelTransparent(unsigned short x, unsigned short y) const
{
	// TODO: this wont work for non-bam sprites, but it isn't used for any currently.
	return GetPixel(x, y).a == 0;
}

void Sprite2D::release()
{
	assert(RefCount > 0);
	if (--RefCount == 0) {
		delete this;
	}
}

}
