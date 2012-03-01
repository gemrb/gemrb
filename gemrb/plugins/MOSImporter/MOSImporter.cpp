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

#include "RGBAColor.h"
#include "win32def.h"

#include "FileCache.h"
#include "Interface.h"
#include "Video.h"

using namespace GemRB;

static ieDword red_mask = 0x00ff0000;
static ieDword green_mask = 0x0000ff00;
static ieDword blue_mask = 0x000000ff;

MOSImporter::MOSImporter(void)
{
	if (DataStream::IsEndianSwitch()) {
		red_mask = 0x0000ff00;
		green_mask = 0x00ff0000;
		blue_mask = 0xff000000;
	}
}

MOSImporter::~MOSImporter(void)
{
}

bool MOSImporter::Open(DataStream* stream)
{
	str = stream;
	char Signature[8];
	str->Read( Signature, 8 );
	if (strncmp( Signature, "MOSCV1  ", 8 ) == 0) {
		str->Seek( 4, GEM_CURRENT_POS );
		DataStream* cached = CacheCompressedStream(stream, stream->filename);
		delete str;
		if (!cached)
			return false;
		str = cached;
		str->Read( Signature, 8 );
	}
	if (strncmp( Signature, "MOS V1  ", 8 ) != 0) {
		return false;
	}
	str->ReadWord( &Width );
	str->ReadWord( &Height );
	str->ReadWord( &Cols );
	str->ReadWord( &Rows );
	str->ReadDword( &BlockSize );
	str->ReadDword( &PalOffset );
	return true;
}

Sprite2D* MOSImporter::GetSprite2D()
{
	RevColor RevCol[256];
	unsigned char * pixels = ( unsigned char * ) malloc( Width * Height * 4 );
	unsigned char * blockpixels = ( unsigned char * )
		malloc( BlockSize * BlockSize );
	ieDword blockoffset;
	for (int y = 0; y < Rows; y++) {
		int bh = ( y == Rows - 1 ) ?
			( ( Height % 64 ) == 0 ? 64 : Height % 64 ) :
			64;
		for (int x = 0; x < Cols; x++) {
			int bw = ( x == Cols - 1 ) ?
				( ( Width % 64 ) == 0 ? 64 : Width % 64 ) :
				64;
			str->Seek( PalOffset + ( y * Cols * 1024 ) +
				( x * 1024 ), GEM_STREAM_START );
			str->Read( &RevCol[0], 1024 );
			str->Seek( PalOffset + ( Rows * Cols * 1024 ) +
				( y * Cols * 4 ) + ( x * 4 ),
				GEM_STREAM_START );
			str->ReadDword( &blockoffset );
			str->Seek( PalOffset + ( Rows * Cols * 1024 ) +
				( Rows * Cols * 4 ) + blockoffset,
				GEM_STREAM_START );
			str->Read( blockpixels, bw * bh );
			unsigned char * bp = blockpixels;
			unsigned char * startpixel = pixels +
				( ( Width * 4 * y ) * 64 ) +
				( 4 * x * 64 );
			for (int h = 0; h < bh; h++) {
				for (int w = 0; w < bw; w++) {
					*startpixel = RevCol[*bp].b;
					startpixel++;
					*startpixel = RevCol[*bp].g;
					startpixel++;
					*startpixel = RevCol[*bp].r;
					startpixel++;
					*startpixel = RevCol[*bp].a;
					startpixel++;
					bp++;
				}
				startpixel = startpixel + ( Width * 4 ) - ( 4 * bw );
			}
		}
	}
	free( blockpixels );
	Sprite2D* ret = core->GetVideoDriver()->CreateSprite( Width, Height, 32,
		red_mask, green_mask, blue_mask, 0,
		pixels, true, green_mask );
	return ret;
}

#include "plugindef.h"

GEMRB_PLUGIN(0x167B73E, "MOS File Importer")
PLUGIN_IE_RESOURCE(MOSImporter, "mos", (ieWord)IE_MOS_CLASS_ID)
END_PLUGIN()
