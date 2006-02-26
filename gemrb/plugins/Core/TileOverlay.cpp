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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/TileOverlay.cpp,v 1.20 2006/02/26 12:30:58 avenger_teambg Exp $
 *
 */

#include "../../includes/win32def.h"
#include "TileOverlay.h"
#include "Interface.h"
#include "Video.h"
#include <math.h>

extern Interface* core;

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

void TileOverlay::Draw(Region viewport, std::vector< TileOverlay*> &overlays)
{
	//half transparency
	Color tint = {255,255,255,0};
	// if the video's viewport is partially outside of the map, bump it back
	bool bump = false;
	Video* vid = core->GetVideoDriver();
	Region vp = vid->GetViewport();
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
		core->MoveViewportTo( vp.x, vp.y, false );
	}

	// determine which tiles are visible
	int sx = vp.x / 64;
	int sy = vp.y / 64;
	int dx = ( vp.x + vp.w + 63 ) / 64;
	int dy = ( vp.y + vp.h + 63 ) / 64;

	for (int y = sy; y < dy && y < h; y++) {
		for (int x = sx; x < dx && x < w; x++) {
			Tile* tile = tiles[( y* w ) + x];
			vid->BlitSprite( tile->anim[0]->NextFrame(),
				viewport.x + ( x * 64 ), viewport.y + ( y * 64 ),
				false, &viewport );
			if (!tile->om) {
				continue;
			}

			//draw overlay tiles, they should be half transparent
			int mask = 2;
			for (int z = 1;z<5;z++) {
				TileOverlay * ov = overlays[z];
				if (ov) {
					Tile *ovtile = ov->tiles[0]; //allow only 1x1 tiles now
					if (tile->om & mask) {
						vid->BlitSpriteTinted( ovtile->anim[0]->NextFrame(),
							viewport.x + ( x * 64 ), viewport.y + ( y * 64 ),
							tint, NULL, &viewport );
					}
				}
				mask<<=1;
			}

			//original frame should be drawn over it
			if (tile->anim[1]) {
				vid->BlitSpriteTinted( tile->anim[1]->NextFrame(),
					viewport.x + ( x * 64 ), viewport.y + ( y * 64 ),
					tint, NULL, &viewport );
			} else {
				//bg1 redraws the original frame
				vid->BlitSpriteTinted( tile->anim[0]->NextFrame(),
					viewport.x + ( x * 64 ), viewport.y + ( y * 64 ),
					tint, NULL, &viewport );
			}
		}
	}
}
