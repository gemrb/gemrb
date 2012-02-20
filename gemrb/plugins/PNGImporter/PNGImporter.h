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

#ifndef PNGIMPORTER_H
#define PNGIMPORTER_H

#include "ImageMgr.h"

namespace GemRB {

struct PNGInternal;

class PNGImporter : public ImageMgr {
private:
	PNGInternal* inf;

	ieDword Width, Height;
	bool hasPalette;
public:
	PNGImporter(void);
	~PNGImporter(void);
	void Close();
	bool Open(DataStream* stream);
	Sprite2D* GetSprite2D();
	void GetPalette(int colors, Color* pal);
	int GetWidth() { return (int) Width; }
	int GetHeight() { return (int) Height; }
};

}

#endif
