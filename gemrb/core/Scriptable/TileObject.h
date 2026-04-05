// SPDX-FileCopyrightText: 2023 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef TILEOBJECT_H
#define TILEOBJECT_H

#include "exports.h"
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
