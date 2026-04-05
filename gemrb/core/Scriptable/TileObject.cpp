// SPDX-FileCopyrightText: 2023 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "Scriptable/TileObject.h"

namespace GemRB {

TileObject::~TileObject()
{
	if (openTiles) {
		free(openTiles);
	}
	if (closedTiles) {
		free(closedTiles);
	}
}

void TileObject::SetOpenTiles(unsigned short* tiles, int cnt)
{
	if (openTiles) {
		free(openTiles);
	}
	openTiles = tiles;
	openCount = cnt;
}

void TileObject::SetClosedTiles(unsigned short* tiles, int cnt)
{
	if (closedTiles) {
		free(closedTiles);
	}
	closedTiles = tiles;
	closedCount = cnt;
}

}
