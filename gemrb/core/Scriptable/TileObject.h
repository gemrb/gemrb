/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2023 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#ifndef TILEOBJECT_H
#define TILEOBJECT_H

#include "exports-core.h"
#include "ie_types.h"

namespace GemRB {

// Tiled objects are not used (and maybe not even implemented correctly in IE)
// they seem to be most closer to a door and probably obsoleted by it
// are they scriptable?
class GEM_EXPORT TileObject {
public:
	TileObject() noexcept = default;
	TileObject(const TileObject&) = delete;
	~TileObject();
	TileObject& operator=(const TileObject&) = delete;
	void SetOpenTiles(unsigned short* indices, int count);
	void SetClosedTiles(unsigned short* indices, int count);

public:
	ieVariable name;
	ResRef tileset; // or wed door ID?
	ieDword flags = 0;
	unsigned short* openTiles = nullptr;
	ieDword openCount = 0;
	unsigned short* closedTiles = nullptr;
	ieDword closedCount = 0;
};

}

#endif
