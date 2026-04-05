// SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef TILEOVERLAY_H
#define TILEOVERLAY_H

#include "exports.h"

#include "Tile.h"

#include <vector>

namespace GemRB {

class GEM_EXPORT TileOverlay {
public:
	Size size;
	std::vector<Tile> tiles;

public:
	using TileOverlayPtr = Holder<TileOverlay>;

	explicit TileOverlay(Size size) noexcept;
	TileOverlay(const TileOverlay&) noexcept = delete;
	TileOverlay& operator=(const TileOverlay&) noexcept = delete;

	TileOverlay(TileOverlay&&) noexcept = default;
	TileOverlay& operator=(TileOverlay&&) noexcept = default;

	void AddTile(Tile&& tile);
	void Draw(const Region& viewport, std::vector<TileOverlayPtr>& overlays, BlitFlags flags) const;
};

}

#endif
