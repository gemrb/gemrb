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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/TileOverlay.cpp,v 1.15 2005/03/20 15:07:12 avenger_teambg Exp $
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
	//tiles.push_back(tile);
}

void TileOverlay::Draw(Region viewport)
{
	bool bump = false;
	Video* vid = core->GetVideoDriver();
	Region vp = vid->GetViewport();
	vp.x += viewport.x;
	vp.y += viewport.y;
	vp.w = viewport.w;
	vp.h = viewport.h;
	if (( vp.x + vp.w ) > w * 64) {
		vp.x = ( w * 64 - vp.w );
		bump = true;
	}
	if (vp.x < viewport.x) {
		vp.x = viewport.x;
		bump = true;
	}
	if (( vp.y + vp.h ) > h * 64) {
		vp.y = ( h * 64 - vp.h );
		bump = true;
	}
	if (vp.y < viewport.y) {
		vp.y = viewport.y;
		bump = true;
	}
	if( bump ) {
		core->MoveViewportTo( vp.x - viewport.x, vp.y - viewport.y, false );
	}
	int sx = ( vp.x - viewport.x ) / 64;
	int sy = ( vp.y - viewport.y ) / 64;
	int dx = ( vp.x + vp.w + 63 ) / 64;
	int dy = ( vp.y + vp.h + 63 ) / 64;
	vp.x = viewport.x;
	vp.y = viewport.y;
	vp.w = viewport.w;
	vp.h = viewport.h;
	for (int y = sy; y < dy && y < h; y++) {
		for (int x = sx; x < dx && x < w; x++) {
			Tile* tile = tiles[( y* w ) + x];
			//this hack is for alternate tiles with a value of -1
			if (!tile->anim[tile->tileIndex]) {
		        	tile->tileIndex=0;
			}
			if (tile->anim[tile->tileIndex]) {
				vid->BlitSprite( tile->anim[tile->tileIndex]->NextFrame(),
						viewport.x + ( x * 64 ), viewport.y + ( y * 64 ),
						false, &vp );
			}
		}
	}
}
