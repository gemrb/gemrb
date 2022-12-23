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
	if (strncmp( Signature, "MOS V1  ", 8 ) != 0) {
		return false;
	}
	str->ReadSize(size);
	str->ReadWord(Cols);
	str->ReadWord(Rows);
	str->ReadDword(BlockSize);
	str->ReadDword(PalOffset);
	return true;
}

Holder<Sprite2D> MOSImporter::GetSprite2D()
{
	Color Col[256];
	unsigned char * pixels = ( unsigned char * ) malloc(size.Area() * 4);
	unsigned char * blockpixels = ( unsigned char * )
		malloc( BlockSize * BlockSize );
	ieDword blockoffset;
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
