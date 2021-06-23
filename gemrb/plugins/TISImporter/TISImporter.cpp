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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

#include "TISImporter.h"

#include "RGBAColor.h"

#include "Interface.h"
#include "Sprite2D.h"
#include "Video.h"

using namespace GemRB;

TISImporter::TISImporter(void)
{
}

TISImporter::~TISImporter(void)
{
	delete str;
}

bool TISImporter::Open(DataStream* stream)
{
	if (stream == NULL) {
		return false;
	}
	delete str;
	str = stream;
	char Signature[8];
	str->Read( Signature, 8 );
	headerShift = 0;
	if (Signature[0] == 'T' && Signature[1] == 'I' && Signature[2] == 'S') {
		if (strncmp( Signature, "TIS V1  ", 8 ) != 0) {
			Log(ERROR, "TISImporter", "Not a Valid TIS file!");
			return false;
		}
		str->ReadDword(TilesCount);
		str->ReadDword(TilesSectionLen);
		str->ReadDword(headerShift);
		str->ReadDword(TileSize);
	} else {
		str->Seek( -8, GEM_CURRENT_POS );
	}
	return true;
}

Tile* TISImporter::GetTile(unsigned short* indexes, int count,
	unsigned short* secondary)
{
	Animation* ani = new Animation( count );
	//pause key stops animation
	ani->gameAnimation = true;
	//the turning crystal in ar3202 (bg1) requires animations to be synced
	ani->pos = 0;
	for (int i = 0; i < count; i++) {
		ani->AddFrame( GetTile( indexes[i] ), i );
	}
	if (secondary) {
		Animation* sec = new Animation( count );
		for (int i = 0; i < count; i++) {
			sec->AddFrame( GetTile( secondary[i] ), i );
		}
		return new Tile( ani, sec );
	}
	return new Tile( ani );
}

Holder<Sprite2D> TISImporter::GetTile(int index)
{
	Color Col[256];
	Color Palette[256]{};
	void* pixels = calloc(4096, 1);
	unsigned long pos = index *(1024+4096) + headerShift;
	if(str->Size()<pos+1024+4096) {
		// try to only report error once per file
		static TISImporter *last_corrupt = NULL;
		if (last_corrupt != this) {
			Log(ERROR, "TISImporter", "Corrupt WED file encountered; couldn't find any more tiles at tile %d", index);
			last_corrupt = this;
		}
	
		// original PS:T AR0609 and AR0612 report far more tiles than are actually present :(
		Palette[0].g = 200;
		return core->GetVideoDriver()->CreatePalettedSprite( Region(0,0,64,64), 8, pixels, Palette );
	}
	str->Seek( pos, GEM_STREAM_START );
	str->Read( &Col, 1024 );
	int transindex = 0;
	bool transparent = false;
	for (int i = 0; i < 256; i++) {
		// bgra format
		Palette[i].r = Col[i].b;
		Palette[i].g = Col[i].g;
		Palette[i].b = Col[i].r;
		Palette[i].a = (Col[i].a) ? Col[i].a : 255; // alpha is unused by the originals but SDL will happily use it
		if (Palette[i].g==255 && !Palette[i].r && !Palette[i].b) {
			if (transparent) {
				Log(ERROR, "TISImporter", "Tile has two green (transparent) palette entries");
			} else {
				transparent = true;
				transindex = i;
			}
		}
	}
	str->Read( pixels, 4096 );
	return core->GetVideoDriver()->CreatePalettedSprite( Region(0,0,64,64), 8, pixels, Palette, transparent, transindex );
}

#include "plugindef.h"

GEMRB_PLUGIN(0x19F91578, "TIS File Importer")
PLUGIN_CLASS(IE_TIS_CLASS_ID, TISImporter)
END_PLUGIN()
