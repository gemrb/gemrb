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

#ifndef TILE_H
#define TILE_H

#include "RGBAColor.h"
#include "exports.h"

#include "Animation.h"

namespace GemRB {

class GEM_EXPORT Tile {
public:
	Tile(Animation* anim, Animation* sec = NULL);
	~Tile(void);
	unsigned char tileIndex;
	unsigned char om;
	Color SearchMap[16];
	Color HeightMap[16];
	Color LightMap[16];
	Color NLightMap[16];
	Animation* anim[2];
};

}

#endif
