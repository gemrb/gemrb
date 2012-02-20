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

#ifndef MOSIMPORTER_H
#define MOSIMPORTER_H

#include "ImageMgr.h"

namespace GemRB {

class MOSImporter : public ImageMgr {
private:
	ieWord Width, Height, Cols, Rows;
	ieDword BlockSize, PalOffset;
public:
	MOSImporter(void);
	~MOSImporter(void);
	bool Open(DataStream* stream);
	Sprite2D* GetSprite2D();
	int GetWidth() { return (int) Width; }
	int GetHeight() { return (int) Height; }
};

}

#endif
