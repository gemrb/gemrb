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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

#include "TileOverlay.h"

#include "Game.h" // for GetGlobalTint
#include "GlobalTimer.h"
#include "Interface.h"

namespace GemRB {

TileOverlay::TileOverlay(Size size) noexcept
: size(size)
{}

void TileOverlay::AddTile(Tile&& tile)
{
	tiles.push_back(std::move(tile));
}

void TileOverlay::Draw(const Region& viewport, std::vector<TileOverlayPtr> &overlays, BlitFlags flags) const
{
	// determine which tiles are visible
	int sx = std::max(viewport.x / 64, 0);
	int sy = std::max(viewport.y / 64, 0);
	int dx = ( std::max(viewport.x, 0) + viewport.w + 63 ) / 64;
	int dy = ( std::max(viewport.y, 0) + viewport.h + 63 ) / 64;

	const Game* game = core->GetGame();
	assert(game);
	const Color* globalTint = game->GetGlobalTint();
	if (globalTint) {
		flags |= BlitFlags::COLOR_MOD;
	}
	const Color tintcol = globalTint ? * globalTint : Color();

	for (int y = sy; y < dy && y < size.h; y++) {
		for (int x = sx; x < dx && x < size.w; x++) {
			const Tile &tile = tiles[(y * size.w) + x];

			//draw door tiles if there are any
			Animation* anim = tile.GetAnimation();
			assert(anim);

			// this is the base terrain tile
			Point p = Point(x * 64, y * 64) - viewport.origin;
			VideoDriver->BlitGameSprite(anim->NextFrame(), p, flags, tintcol);

			if (!tile.om || tile.tileIndex) {
				continue;
			}

			int mask = 2;
			for (size_t z = 1; z < overlays.size(); ++z) {
				const auto& ov = overlays[z];
				if (ov && !ov->tiles.empty()) {
					const Tile &ovtile = ov->tiles[0]; //allow only 1x1 tiles now
					if (tile.om & mask) {
						//draw overlay tiles, they should be half transparent except for BG1
						BlitFlags transFlag = (core->HasFeature(GFFlags::LAYERED_WATER_TILES)) ? BlitFlags::HALFTRANS : BlitFlags::NONE;
						// this is the water (or whatever)
						VideoDriver->BlitGameSprite(ovtile.GetAnimation(0)->NextFrame(), p, flags | transFlag, tintcol);

						if (core->HasFeature(GFFlags::LAYERED_WATER_TILES)) {
							Animation* anim1 = tile.GetAnimation(1);
							if (anim1) {
								// this is the mask to blend the terrain tile with the water for everything but BG1
								VideoDriver->BlitGameSprite(anim1->NextFrame(), p,
													flags | BlitFlags::BLENDED, tintcol);
							}
						} else {
							// in BG 1 this is the mask to blend the terrain tile with the water
							VideoDriver->BlitGameSprite(tile.GetAnimation(0)->NextFrame(), p,
												flags | BlitFlags::BLENDED, tintcol);
						}
					}
				}
				mask<<=1;
			}
		}
	}
}

}
