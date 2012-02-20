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

//#include "Game.h" // needed only for TILE_GREY below
#include "GlobalTimer.h"
#include "Interface.h"
#include "Video.h"

namespace GemRB {

bool RedrawTile = false;

TileOverlay::TileOverlay(int Width, int Height)
{
	w = Width;
	h = Height;
	count = 0;
	tiles = ( Tile * * ) malloc( w * h * sizeof( Tile * ) );
}

TileOverlay::~TileOverlay(void)
{
	for (int i = 0; i < count; i++) {
		delete( tiles[i] );
	}
	free( tiles );
}

void TileOverlay::AddTile(Tile* tile)
{
	tiles[count++] = tile;
}

void TileOverlay::BumpViewport(const Region &viewport, Region &vp)
{
	bool bump = false;
	vp.w = viewport.w;
	vp.h = viewport.h;
	if (( vp.x + vp.w ) > w * 64) {
		vp.x = ( w * 64 - vp.w );
		bump = true;
	}
	if (vp.x < 0) {
		vp.x = 0;
		bump = true;
	}
	if (( vp.y + vp.h ) > h * 64) {
		vp.y = ( h * 64 - vp.h );
		bump = true;
	}
	if (vp.y < 0) {
		vp.y = 0;
		bump = true;
	}
	if(bump && !(core->timer->ViewportIsMoving())) {
		core->timer->SetMoveViewPort( vp.x, vp.y, 0, false );
	}
}

void TileOverlay::Draw(Region viewport, std::vector< TileOverlay*> &overlays, int flags)
{
	Video* vid = core->GetVideoDriver();
	Region vp = vid->GetViewport();

	// if the video's viewport is partially outside of the map, bump it back
	BumpViewport(viewport, vp);
	// determine which tiles are visible
	int sx = vp.x / 64;
	int sy = vp.y / 64;
	int dx = ( vp.x + vp.w + 63 ) / 64;
	int dy = ( vp.y + vp.h + 63 ) / 64;

	for (int y = sy; y < dy && y < h; y++) {
		for (int x = sx; x < dx && x < w; x++) {
			Tile* tile = tiles[( y* w ) + x];

			//draw door tiles if there are any
			Animation* anim = tile->anim[tile->tileIndex];
			if (!anim && tile->tileIndex) {
				anim = tile->anim[0];
			}
			vid->BlitTile( anim->NextFrame(), 0, viewport.x + ( x * 64 ),
				viewport.y + ( y * 64 ), &viewport, flags );
			if (!tile->om || tile->tileIndex) {
				continue;
			}

			//draw overlay tiles, they should be half transparent
			int mask = 2;
			for (size_t z = 1;z<overlays.size();z++) {
				TileOverlay * ov = overlays[z];
				if (ov && ov->count > 0) {
					Tile *ovtile = ov->tiles[0]; //allow only 1x1 tiles now
					if (tile->om & mask) {
						if (RedrawTile) {
							vid->BlitTile( ovtile->anim[0]->NextFrame(),
						                   tile->anim[0]->NextFrame(),
							               viewport.x + ( x * 64 ),
							               viewport.y + ( y * 64 ),
							               &viewport, flags );
						} else {
							Sprite2D* mask = 0;
							if (tile->anim[1])
								mask = tile->anim[1]->NextFrame();
							vid->BlitTile( ovtile->anim[0]->NextFrame(),
						                   mask,
							               viewport.x + ( x * 64 ),
							               viewport.y + ( y * 64 ),
							               &viewport, TILE_HALFTRANS | flags );
						}
					}
				}
				mask<<=1;
			}
		}
	}
}

}
