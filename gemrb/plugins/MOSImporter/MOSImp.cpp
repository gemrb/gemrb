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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/MOSImporter/MOSImp.cpp,v 1.10 2004/09/14 22:09:51 avenger_teambg Exp $
 *
 */

#include "../../includes/win32def.h"
#include "MOSImp.h"
#include "../../includes/RGBAColor.h"
#include "../Core/Compressor.h"
#include "../Core/FileStream.h"
#include "../Core/CachedFileStream.h"
#include "../Core/Interface.h"

MOSImp::MOSImp(void)
{
	str = NULL;
	autoFree = false;
}

MOSImp::~MOSImp(void)
{
	if (str && autoFree) {
		delete( str );
	}
}

bool MOSImp::Open(DataStream* stream, bool autoFree)
{
	if (stream == NULL) {
		return false;
	}
	if (str && this->autoFree) {
		delete( str );
	}
	this->autoFree = autoFree;
	str = stream;
	char Signature[8];
	str->Read( Signature, 8 );
	if (strncmp( Signature, "MOSCV1  ", 8 ) == 0) {
		char cpath[_MAX_PATH];
		strcpy( cpath, core->CachePath );
		strcat( cpath, stream->filename );
		FILE* exist_in_cache = fopen( cpath, "rb" );
		if (exist_in_cache) {
			//File was previously cached, using local copy
			if (autoFree)
				delete( str );
			fclose( exist_in_cache );
			FileStream* s = new FileStream();
			s->Open( cpath );
			str = s;
			str->Read( Signature, 8 );
		} else {
			//No file found in Cache, Decompressing and storing for further use
			str->Seek( 4, GEM_CURRENT_POS );
			//TODO: Decompress Mos File
			if (!core->IsAvailable( IE_COMPRESSION_CLASS_ID )) {
				printf( "No Compression Manager Available.\nCannot Load Compressed Mos File.\n" );
				return false;
			}	
			FILE* newfile = fopen( cpath, "wb" );
			Compressor* comp = ( Compressor* )
				core->GetInterface( IE_COMPRESSION_CLASS_ID );
			comp->Decompress( newfile, str );
			core->FreeInterface( comp );
			fclose( newfile );
			if (autoFree)
				delete( str );
			FileStream* s = new FileStream();
			s->Open( cpath );
			str = s;
			str->Read( Signature, 8 );
		}
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

Sprite2D* MOSImp::GetImage()
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
			str->Seek( PalOffset + ( y * Cols * 1024 ) + ( x * 1024 ),
					GEM_STREAM_START );
			str->Read( &RevCol[0], 1024 );
			//for(int i = 0; i < 256; i++) {
			//	str->Read(&RevCol[i], 4);
			//}
			str->Seek( PalOffset +
					( Rows * Cols * 1024 ) +
					( y * Cols * 4 ) +
					( x * 4 ),
					GEM_STREAM_START );
			str->Read( &blockoffset, 4 );
			str->Seek( PalOffset +
					( Rows * Cols * 1024 ) +
					( Rows * Cols * 4 ) +
					blockoffset,
					GEM_STREAM_START );
			str->Read( blockpixels, bw * bh );
			unsigned char * bp = blockpixels;
			unsigned char * startpixel = pixels +
				( ( Width * 4 * y ) * 64 ) +
				( 4 * x * 64 );
			for (int h = 0; h < bh; h++) {
				for (int w = 0; w < bw; w++) {
					*startpixel = RevCol[*bp].a;
					startpixel++;
					*startpixel = RevCol[*bp].b;
					startpixel++;
					*startpixel = RevCol[*bp].g;
					startpixel++;
					*startpixel = RevCol[*bp].r;
					startpixel++;
					bp++;
				}
				//startpixel = pixels + (Width*4*y*64) + (4*x*64) + (Width*4*h);
				startpixel = startpixel + ( Width * 4 ) - ( 4 * bw );
			}
		}
	}
	free( blockpixels );
	Sprite2D* ret = core->GetVideoDriver()->CreateSprite( Width, Height, 32,
												0xff000000, 0x00ff0000,
												0x0000ff00, 0x00000000,
												pixels, true, 0x00ff0000 );
	ret->XPos = ret->YPos = 0;
	return ret;
}
/** No descriptions */
void MOSImp::GetPalette(int /*index*/, int /*colors*/, Color* /*pal*/)
{
}
