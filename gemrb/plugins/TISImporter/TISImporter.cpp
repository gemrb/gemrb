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

Tile* TISImporter::GetTile(const std::vector<ieWord>& indexes,
						   unsigned short* secondary)
{
	size_t count = indexes.size();
	std::vector<Animation::frame_t> frames;
	frames.reserve(count);
	for (size_t i = 0; i < count; i++) {
		frames.push_back(GetTile(indexes[i]));
	}
	
	Animation ani = Animation(std::move(frames));
	//pause key stops animation
	ani.gameAnimation = true;
	//the turning crystal in ar3202 (bg1) requires animations to be synced
	ani.frameIdx = 0;
	
	if (secondary) {
		frames.clear();
		for (size_t i = 0; i < count; i++) {
			frames.push_back(GetTile(secondary[i]));
		}
		Animation sec = Animation(frames);
		return new Tile(ani, sec);
	}
	return new Tile(ani);
}

Holder<Sprite2D> TISImporter::GetTile(int index)
{
	strpos_t pos = index *(1024+4096) + headerShift;
	if (str->Size() < pos + 1024 + 4096) {
		// original PS:T AR0609 and AR0612 report far more tiles than are actually present :(
		
		if (badTile == nullptr) {
			// we only want to generate a single bad tile on demand
			PaletteHolder pal = MakeHolder<Palette>();
			PixelFormat fmt = PixelFormat::Paletted8Bit(pal);
			badTile = core->GetVideoDriver()->CreateSprite(Region(0,0,64,64), nullptr, fmt);
		}
		
		// try to only report error once per file
		static const TISImporter *last_corrupt = nullptr;
		if (last_corrupt != this) {
			Log(ERROR, "TISImporter", "Corrupt WED file encountered; couldn't find any more tiles at tile %d", index);
			last_corrupt = this;
		}
	
		return badTile;
	}
	
	PaletteHolder pal = MakeHolder<Palette>();
	PixelFormat fmt = PixelFormat::Paletted8Bit(pal, true, 0);

	str->Seek( pos, GEM_STREAM_START );
	str->Read(pal->col, 1024);
	for (Color& c : pal->col) {
		std::swap(c.b, c.r); // argb format
		c.a = c.a ? c.a : 255; // alpha is unused by the originals but SDL will happily use it
	}

	auto spr = core->GetVideoDriver()->CreateSprite(Region(0,0,64,64), nullptr, fmt);
	uint8_t* pixels = static_cast<uint8_t*>(spr->LockSprite());
	str->Read(pixels, 4096);
	
	// work around bad data in BG2 AR1700
	for (int i = 0; i < 4096; ++i) {
		uint8_t& val = pixels[i];
		const Color& c = pal->col[val];
		if (c.r == 4 && c.b == 4 && c.g >= 78) {
			val = 0;
		}
	}
	
	spr->UnlockSprite();
	return spr;
}

#include "plugindef.h"

GEMRB_PLUGIN(0x19F91578, "TIS File Importer")
PLUGIN_CLASS(IE_TIS_CLASS_ID, TISImporter)
END_PLUGIN()
