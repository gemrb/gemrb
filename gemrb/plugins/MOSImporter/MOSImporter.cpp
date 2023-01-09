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

#include "MOSImporter.h"

#include "Interface.h"

using namespace GemRB;

bool MOSImporter::Import(DataStream* str)
{
	char Signature[8];
	str->Read( Signature, 8 );
	if (strncmp( Signature, "MOSCV1  ", 8 ) == 0) {
		str->Seek( 4, GEM_CURRENT_POS );
		str = DecompressStream(str);
		if (!str)
			return false;
		str->Read( Signature, 8 );
	}

	if (strncmp(Signature, "MOS V2  ", 8) == 0) {
		version = MOSVersion::V2;
	} else if (strncmp(Signature, "MOS V1  ", 8) != 0) {
		return false;
	}

	if (version == MOSVersion::V1) {
		str->ReadSize(size);
		str->ReadWord(Cols);
		str->ReadWord(Rows);
		str->ReadDword(layout.v1.BlockSize);
		str->ReadDword(layout.v1.PalOffset);
	} else {
		str->ReadScalar<int, ieDword>(size.w);
		str->ReadScalar<int, ieDword>(size.h);
		str->ReadDword(layout.v2.NumBlocks);
		str->ReadDword(layout.v2.BlockOffset);
	}

	return true;
}

void MOSImporter::Blit(const MOSV2DataBlock& dataBlock, uint8_t *frameData) {
	// Pages appear to be used multiple times
	if (!lastPVRZ || dataBlock.pvrzPage != lastPVRZPage) {
		auto resRef = fmt::format("mos{:04d}", dataBlock.pvrzPage);
		StringView resRefView(resRef.c_str(), 7);

		lastPVRZ = GetResourceHolder<ImageMgr>(resRefView, true);
		lastPVRZPage = dataBlock.pvrzPage;
	}

	auto sprite = lastPVRZ->GetSprite2D(Region{dataBlock.source.x, dataBlock.source.y, dataBlock.size.w, dataBlock.size.h});
	if (!sprite) {
		return;
	}

	const uint8_t* spritePixels = reinterpret_cast<uint8_t*>(sprite->LockSprite());
	for (int h = 0; h < dataBlock.size.h; ++h) {
		size_t offset = h * sprite->Frame.w * 4;
		size_t destOffset =
			4 * (size.w * (dataBlock.destination.y + h) + dataBlock.destination.x);

		std::copy(
			spritePixels + offset,
			spritePixels + offset + sprite->Frame.w * 4,
			frameData + destOffset
		);
	}

	sprite->UnlockSprite();
}

Holder<Sprite2D> MOSImporter::GetSprite2D() {
	if (version == MOSVersion::V2) {
		return GetSprite2Dv2();
	} else {
		return GetSprite2Dv1();
	}
}

Holder<Sprite2D> MOSImporter::GetSprite2Dv2() {
	size_t imageSize = size.Area() * 4;
	uint8_t *imageData = reinterpret_cast<uint8_t*>(malloc(imageSize));
	std::fill(imageData, imageData + imageSize, 0);

	str->Seek(layout.v2.BlockOffset, GEM_STREAM_START);

	MOSV2DataBlock dataBlock;
	for (uint16_t i = 0; i < layout.v2.NumBlocks; ++i) {
		str->ReadDword(dataBlock.pvrzPage);
		str->ReadScalar<int, ieDword>(dataBlock.source.x);
		str->ReadScalar<int, ieDword>(dataBlock.source.y);
		str->ReadScalar<int, ieDword>(dataBlock.size.w);
		str->ReadScalar<int, ieDword>(dataBlock.size.h);
		str->ReadScalar<int, ieDword>(dataBlock.destination.x);
		str->ReadScalar<int, ieDword>(dataBlock.destination.y);

		Blit(dataBlock, imageData);
	}

	PixelFormat fmt = PixelFormat::ARGB32Bit();
	Region region{0, 0, size.w, size.h};

	return {core->GetVideoDriver()->CreateSprite(region, imageData, fmt)};
}

Holder<Sprite2D> MOSImporter::GetSprite2Dv1() {
	Color Col[256];
	unsigned char * pixels = ( unsigned char * ) malloc(size.Area() * 4);
	unsigned char * blockpixels = ( unsigned char * )
	malloc( layout.v1.BlockSize * layout.v1.BlockSize );
	ieDword blockoffset;
	ieDword& PalOffset = layout.v1.PalOffset;

	for (int y = 0; y < Rows; y++) {
		int bh = ( y == Rows - 1 ) ?
			( ( size.h % 64 ) == 0 ? 64 : size.h % 64 ) :
			64;
		for (int x = 0; x < Cols; x++) {
			int bw = ( x == Cols - 1 ) ?
				( ( size.w % 64 ) == 0 ? 64 : size.w % 64 ) :
				64;
			str->Seek( PalOffset + ( y * Cols * 1024 ) +
				( x * 1024 ), GEM_STREAM_START );
			str->Read( &Col[0], 1024 );
			str->Seek( PalOffset + ( Rows * Cols * 1024 ) +
				( y * Cols * 4 ) + ( x * 4 ),
				GEM_STREAM_START );
			str->ReadDword(blockoffset);
			str->Seek( PalOffset + ( Rows * Cols * 1024 ) +
				( Rows * Cols * 4 ) + blockoffset,
				GEM_STREAM_START );
			str->Read( blockpixels, bw * bh );
			const unsigned char* bp = blockpixels;
			unsigned char * startpixel = pixels +
				( ( size.w * 4 * y ) * 64 ) +
				( 4 * x * 64 );
			for (int h = 0; h < bh; h++) {
				for (int w = 0; w < bw; w++) {
					*startpixel = Col[*bp].r;
					startpixel++;
					*startpixel = Col[*bp].g;
					startpixel++;
					*startpixel = Col[*bp].b;
					startpixel++;
					*startpixel = Col[*bp].a;
					startpixel++;
					bp++;
				}
				startpixel = startpixel + ( size.w * 4 ) - ( 4 * bw );
			}
		}
	}
	free( blockpixels );
	
	constexpr uint32_t red_mask = 0x00ff0000;
	constexpr uint32_t green_mask = 0x0000ff00;
	constexpr uint32_t blue_mask = 0x000000ff;
	PixelFormat fmt(4, red_mask, green_mask, blue_mask, 0);
	fmt.HasColorKey = true;
	fmt.ColorKey = green_mask;
	
	return core->GetVideoDriver()->CreateSprite(Region(0,0, size.w, size.h), pixels,fmt);
}

#include "plugindef.h"

GEMRB_PLUGIN(0x167B73E, "MOS File Importer")
PLUGIN_IE_RESOURCE(MOSImporter, "mos", (ieWord)IE_MOS_CLASS_ID)
END_PLUGIN()
