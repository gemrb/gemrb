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
#include "Video/Video.h"

using namespace GemRB;

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
	std::vector<Animation::frame_t> frames;
	frames.reserve(count);
	for (int i = 0; i < count; i++) {
		frames.push_back(GetTile(indexes[i]));
	}
	
	Animation ani = Animation(std::move(frames));
	//pause key stops animation
	ani.gameAnimation = true;
	//the turning crystal in ar3202 (bg1) requires animations to be synced
	ani.frameIdx = 0;
	
	if (secondary) {
		frames.clear();
		for (int i = 0; i < count; i++) {
			frames.push_back(GetTile(secondary[i]));
		}
		Animation sec = Animation(frames);
		return new Tile(ani, sec);
	}
	return new Tile(ani);
}

Holder<Sprite2D> TISImporter::GetTile(int index)
{
	PaletteHolder pal = MakeHolder<Palette>();
	PixelFormat fmt = PixelFormat::Paletted8Bit(pal);
	
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
		pal->col[0].g = 200;
		return core->GetVideoDriver()->CreateSprite(Region(0,0,64,64), pixels, fmt);
	}
	str->Seek( pos, GEM_STREAM_START );
	Color Col[256];
	str->Read(&Col, 1024 );

	for (int i = 0; i < 256; i++) {
		// bgra format
		pal->col[i].r = Col[i].b;
		pal->col[i].g = Col[i].g;
		pal->col[i].b = Col[i].r;
		pal->col[i].a = (Col[i].a) ? Col[i].a : 255; // alpha is unused by the originals but SDL will happily use it
		if (pal->col[i].g==255 && !pal->col[i].r && !pal->col[i].b) {
			fmt.HasColorKey = true;
			fmt.ColorKey = i;
		}
	}
	str->Read( pixels, 4096 );
	return core->GetVideoDriver()->CreateSprite(Region(0,0,64,64), pixels, fmt);
}

#include "plugindef.h"

GEMRB_PLUGIN(0x19F91578, "TIS File Importer")
PLUGIN_CLASS(IE_TIS_CLASS_ID, TISImporter)
END_PLUGIN()
