/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
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
 *
 *
 */

#ifndef BMPIMP_H
#define BMPIMP_H

#include "ImageMgr.h"

namespace GemRB {

class BMPImporter : public ImageMgr {
private:
	//BITMAPINFOHEADER
	ieDword Size, Compression, ImageSize/*, ColorsUsed, ColorsImportant*/;
	ieWord Planes, BitCount;

	//COLORTABLE
	ieDword NumColors;
	Color* PaletteColors = nullptr;

	//RASTERDATA
	void* pixels;

	//OTHER
	unsigned int PaddedRowLength;
public:
	BMPImporter(void);
	~BMPImporter(void) override;
	bool Import(DataStream* stream) override;
	Holder<Sprite2D> GetSprite2D() override;
	int GetPalette(int colors, Color* pal) override;

private:
	void Read8To8(const void *rpixels);
	void Read4To8(const void *rpixels);
};

}

#endif
