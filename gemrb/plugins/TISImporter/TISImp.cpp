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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/TISImporter/TISImp.cpp,v 1.6 2004/02/24 22:20:38 balrog994 Exp $
 *
 */

#include "../../includes/win32def.h"
#include "TISImp.h"
#include "../../includes/RGBAColor.h"
#include "../Core/Interface.h"

TISImp::TISImp(void)
{
	str = NULL;
	autoFree = false;
}

TISImp::~TISImp(void)
{
	if (str && autoFree) {
		delete( str );
	}
}

bool TISImp::Open(DataStream* stream, bool autoFree)
{
	if (stream == NULL) {
		return false;
	}
	if (str && this->autoFree) {
		delete( str );
	}
	str = stream;
	this->autoFree = autoFree;
	char Signature[8];
	str->Read( Signature, 1 );
	str->Seek( -1, GEM_CURRENT_POS );
	headerShift = 0;
	if (Signature[0] == 'T') {
		str->Read( Signature, 8 );	
		if (strncmp( Signature, "TIS V1  ", 8 ) != 0) {
			printf( "[TISImporter]: Not a Valid TIS File.\n" );
			return false;
		}
		str->Read( &TilesCount, 4 );
		str->Read( &TilesSectionLen, 4 );
		str->Read( &headerShift, 4 );
		str->Read( &TileSize, 4 );
	}
	return true;
}

Tile* TISImp::GetTile(unsigned short* indexes, int count,
	unsigned short* secondary)
{
	Animation* ani = new Animation( indexes, count );
	ani->x = ani->y = 0;
	for (int i = 0; i < count; i++) {
		ani->AddFrame( GetTile( indexes[i] ), indexes[i] );
	}
	if (secondary) {
		Animation* sec = new Animation( secondary, count );
		sec->x = sec->y = 0;
		for (int i = 0; i < count; i++) {
			sec->AddFrame( GetTile( secondary[i] ), secondary[i] );
		}
		return new Tile( ani, sec );
	}
	return new Tile( ani );
}
Sprite2D* TISImp::GetTile(int index)
{
	RevColor RevCol[256];
	Color Palette[256];
	void* pixels = malloc( 4096 );
	str->Seek( ( index * ( 1024 + 4096 ) + headerShift ), GEM_STREAM_START );
	str->Read( &RevCol, 1024 );
	for (int i = 0; i < 256; i++) {
		Palette[i].r = RevCol[i].r;
		Palette[i].g = RevCol[i].g;
		Palette[i].b = RevCol[i].b;
		Palette[i].a = RevCol[i].a;
	}
	str->Read( pixels, 4096 );
	Sprite2D* spr = core->GetVideoDriver()->CreateSprite8( 64, 64, 8, pixels,
												Palette );
	spr->XPos = spr->YPos = 0;
	return spr;
}
