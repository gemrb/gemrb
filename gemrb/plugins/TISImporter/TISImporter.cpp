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
#include "Logging/Logging.h"
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
		if (TilesSectionLen == 0xc) { // 0xc for PVR, 0x1400 for palette
			hasPVRData = true;
		}
	} else {
		if (core->HasFeature(GFFlags::HAS_EE_EFFECTS)) { // hack!
			hasPVRData = true;
			TilesSectionLen = 0xc;
		}
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
		Animation sec = Animation(std::move(frames));
		return new Tile(std::move(ani), std::move(sec));
	}
	return new Tile(std::move(ani));
}

Holder<Sprite2D> TISImporter::GetTile(int index)
{
	if (hasPVRData) return GetTilePVR(index);
	return GetTilePaletted(index);
}

Holder<Sprite2D> TISImporter::GetTilePVR(int index)
{
	size_t imageSize = TileSize * TileSize * 4;
	uint8_t* imageData = static_cast<uint8_t*>(malloc(imageSize));
	std::fill(imageData, imageData + imageSize, 0);

	str->Seek(headerShift + index * TilesSectionLen, GEM_STREAM_START);

	TISPVRBlock dataBlock;
	str->ReadDword(dataBlock.pvrzPage);
	str->ReadScalar<int, ieDword>(dataBlock.source.x);
	str->ReadScalar<int, ieDword>(dataBlock.source.y);
	Blit(dataBlock, imageData);

	PixelFormat fmt = PixelFormat::ARGB32Bit();
	Region region{0, 0, static_cast<int>(TileSize), static_cast<int>(TileSize)};

	return {VideoDriver->CreateSprite(region, imageData, fmt)};
}

void TISImporter::Blit(const TISPVRBlock& dataBlock, uint8_t* frameData)
{
	// optimization for when pages get used multiple times
	if (!lastPVRZ || dataBlock.pvrzPage != lastPVRZPage) {
		// AR2600N.TIS would refer to A2600Nxx.PVRZ, supposedly:
		//   - the first character of the TIS filename
		//   - the four digits of the area code, the optional 'N' from night tilesets
		//   - this page value as a zero-padded two digits number
		// we cheat and just derive the middle from the tis name as well
		ResRef suffix(&str->filename[2], 5);
		if (suffix[4] == '.') suffix.erase(4, 1);
		auto resRef = fmt::format("{}{:.4}{:02d}", str->filename[0], suffix, dataBlock.pvrzPage);

		lastPVRZ = gamedata->GetResourceHolder<ImageMgr>(resRef, true);
		lastPVRZPage = dataBlock.pvrzPage;
	}

	if (!lastPVRZ) return;
	auto sprite = lastPVRZ->GetSprite2D(Region{dataBlock.source.x, dataBlock.source.y, static_cast<int>(TileSize), static_cast<int>(TileSize)});
	if (!sprite) {
		return;
	}

	const uint8_t* spritePixels = static_cast<uint8_t*>(sprite->LockSprite());
	for (ieDword h = 0; h < TileSize; ++h) {
		size_t offset = h * sprite->Frame.w * 4;
		size_t destOffset = 4 * (TileSize * h);

		std::copy(
			spritePixels + offset,
			spritePixels + offset + sprite->Frame.w * 4,
			frameData + destOffset
		);
	}

	sprite->UnlockSprite();
}

Holder<Sprite2D> TISImporter::GetTilePaletted(int index)
{
	strpos_t pos = index *(1024+4096) + headerShift;
	if (str->Size() < pos + 1024 + 4096) {
		// original PS:T AR0609 and AR0612 report far more tiles than are actually present :(
		
		if (badTile == nullptr) {
			// we only want to generate a single bad tile on demand
			Holder<Palette> pal = MakeHolder<Palette>();
			PixelFormat fmt = PixelFormat::Paletted8Bit(std::move(pal));
			badTile = VideoDriver->CreateSprite(Region(0,0,64,64), nullptr, fmt);
		}
		
		// try to only report error once per file
		static const TISImporter *last_corrupt = nullptr;
		if (last_corrupt != this) {
			Log(ERROR, "TISImporter", "Corrupt WED file encountered; couldn't find any more tiles at tile {}", index);
			last_corrupt = this;
		}
	
		return badTile;
	}
	
	Holder<Palette> pal = MakeHolder<Palette>();
	PixelFormat fmt = PixelFormat::Paletted8Bit(pal);
	colorkey_t ck = 0;
	
	auto ckTest = [](const Color& c) {
		// work around bad data in BG2 AR1700
		return c.r <= 4 && c.b <= 4 && c.g >= 78;
	};

	str->Seek( pos, GEM_STREAM_START );
	str->Read(pal->col, 1024);
	for (Color& c : pal->col) {
		std::swap(c.b, c.r); // argb format
		c.a = c.a ? c.a : 255; // alpha is unused by the originals but SDL will happily use it
		if (ck == 0 && ckTest(c)) {
			c = ColorGreen;
			ck = colorkey_t(&c - pal->col);
		}
	}
	
	fmt.ColorKey = ck;
	fmt.HasColorKey = pal->col[ck] == ColorGreen;

	auto spr = VideoDriver->CreateSprite(Region(0,0,64,64), nullptr, fmt);
	uint8_t* pixels = static_cast<uint8_t*>(spr->LockSprite());
	str->Read(pixels, 4096);
	
	// work around bad data in BG2 AR1700
	for (int i = 0; i < 4096; ++i) {
		uint8_t& val = pixels[i];
		const Color& c = pal->col[val];
		if (ckTest(c)) {
			assert(fmt.HasColorKey);
			val = ck;
		}
	}
	
	spr->UnlockSprite();
	return spr;
}

#include "plugindef.h"

GEMRB_PLUGIN(0x19F91578, "TIS File Importer")
PLUGIN_CLASS(IE_TIS_CLASS_ID, TISImporter)
END_PLUGIN()
