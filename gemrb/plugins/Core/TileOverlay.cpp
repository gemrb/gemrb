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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * $Id$
 *
 */

#include "../../includes/win32def.h"
#include "TileOverlay.h"
#include "Interface.h"
#include "Video.h"

extern Interface* core;
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

void TileOverlay::BumpViewport(Region &viewport, Region &vp)
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
	if( bump ) {
		core->timer->SetMoveViewPort( vp.x, vp.y, 0, false );
	}
}

void TileOverlay::Draw(Region viewport, std::vector< TileOverlay*> &overlays)
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
			vid->BlitSprite( anim->NextFrame(), viewport.x + ( x * 64 ),
				viewport.y + ( y * 64 ), false, &viewport );
			if (!tile->om || tile->tileIndex) {
				continue;
			}

			//draw overlay tiles, they should be half transparent
			int mask = 2;
			for (int z = 1;z<5;z++) {
				TileOverlay * ov = overlays[z];
				if (ov) {
					Tile *ovtile = ov->tiles[0]; //allow only 1x1 tiles now
					if (tile->om & mask) {
						if (RedrawTile) {
							vid->BlitSprite( ovtile->anim[0]->NextFrame(),
								viewport.x + ( x * 64 ), viewport.y + ( y * 64 ),
								false, &viewport );
						} else {
							vid->BlitSpriteHalfTrans( ovtile->anim[0]->NextFrame(),
								viewport.x + ( x * 64 ), viewport.y + ( y * 64 ),
								false, &viewport );
						}
					}
				}
				mask<<=1;
			}

			//original frame should be drawn over it
			if (tile->anim[1]) {
				vid->BlitSprite( tile->anim[1]->NextFrame(),
					viewport.x + ( x * 64 ), viewport.y + ( y * 64 ),
					false, &viewport );
			} else if (RedrawTile) {
				//bg1 redraws the original frame
				vid->BlitSprite( tile->anim[0]->NextFrame(),
					viewport.x + ( x * 64 ), viewport.y + ( y * 64 ),
					false, &viewport );
			}
		}
	}
}
