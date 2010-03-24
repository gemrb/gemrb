/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * $Id$
 *
 */

#include "../../includes/win32def.h"
#include "BMPImp.h"
#include "../../includes/RGBAColor.h"
#include "../Core/Interface.h"
#include "../Core/Video.h"
#include "../Core/ImageFactory.h"

#define BMP_HEADER_SIZE  54

static ieDword red_mask = 0x00ff0000;
static ieDword green_mask = 0x0000ff00;
static ieDword blue_mask = 0x000000ff;

BMPImp::BMPImp(void)
{
	Palette = NULL;
	pixels = NULL;
	if (DataStream::IsEndianSwitch()) {
		red_mask = 0x000000ff;
		green_mask = 0x0000ff00;
		blue_mask = 0x00ff0000;
	}
}

BMPImp::~BMPImp(void)
{
	free( Palette );
	free( pixels );
}

bool BMPImp::Open(DataStream* stream, bool autoFree)
{
	if (!Resource::Open(stream, autoFree))
		return false;
	//we release the previous pixel data
	free( pixels );
	pixels = NULL;
	free( Palette );
	Palette = NULL;

	str = stream;
	this->autoFree = autoFree;
	//BITMAPFILEHEADER
	char Signature[2];
	ieDword FileSize, DataOffset;

	str->Read( Signature, 2 );
	if (strncmp( Signature, "BM", 2 ) != 0) {
		printMessage( "BMPImporter","Not a valid BMP File.\n",LIGHT_RED);
		return false;
	}
	str->ReadDword( &FileSize );
	str->Seek( 4, GEM_CURRENT_POS );
	str->ReadDword( &DataOffset );

	//BITMAPINFOHEADER

	str->ReadDword( &Size );
	//some IE palettes are of a different format (OS/2 BMP)!
	if (Size < 24) {
		printMessage( "BMPImporter","OS/2 Bitmap, not supported.\n", LIGHT_RED);
		return false;
	}
	str->ReadDword( &Width );
	str->ReadDword( &Height );
	str->ReadWord( &Planes );
	str->ReadWord( &BitCount );
	str->ReadDword( &Compression );
	str->ReadDword( &ImageSize );
	//24 = the already read bytes 3x4+2x2+2x4
	//this is normally 16
	str->Seek( Size-24, GEM_CURRENT_POS );
	//str->ReadDword(&Hres );
	//str->ReadDword(&Vres );
	//str->ReadDword(&ColorsUsed );
	//str->ReadDword(&ColorsImportant );
	if (Compression != 0) {
		printMessage( "BMPImporter"," ", LIGHT_RED);
		printf( "Compressed %d-bits Image, not supported.\n", BitCount );
		return false;
	}
	//COLORTABLE
	Palette = NULL;
	if (BitCount <= 8) {
		if (BitCount == 8)
			NumColors = 256;
		else
			NumColors = 16;
		Palette = ( Color * ) malloc( 4 * NumColors );
		for (unsigned int i = 0; i < NumColors; i++) {
			str->Read( &Palette[i].b, 1 );
			str->Read( &Palette[i].g, 1 );
			str->Read( &Palette[i].r, 1 );
			str->Read( &Palette[i].a, 1 );
		}
	}
	str->Seek( DataOffset, GEM_STREAM_START );
	//no idea if we have to swap this or not
	//RASTERDATA
	switch (BitCount) {
		case 32:
			PaddedRowLength = Width * 4;
			break;

		case 24:
			PaddedRowLength = Width * 3;
			break;

		case 16:
			PaddedRowLength = Width * 2;
			break;

		case 8:
			PaddedRowLength = Width;
			break;

		case 4:
			PaddedRowLength = ( Width >> 1 );
			break;
		default:
			printMessage( "BMPImporter"," ", LIGHT_RED);
			printf( "BitCount %d is not supported.\n", BitCount );
			return false;
	}
	//if(BitCount!=4)
	//{
	if (PaddedRowLength & 3) {
		PaddedRowLength += 4 - ( PaddedRowLength & 3 );
	}
	//}
	void* rpixels = malloc( PaddedRowLength* Height );
	str->Read( rpixels, PaddedRowLength * Height );
	if (BitCount == 32) {
		//convert to 24 bits on the fly
		int size = Width * Height * 3;
		pixels = malloc( size );
		unsigned char * dest = ( unsigned char * ) pixels;
		dest += size;
		unsigned char * src = ( unsigned char * ) rpixels;
		for (int i = Height; i; i--) {
			dest -= ( Width * 3 );
			for (unsigned int j=0;j<Width;j++) {
				dest[j*3]=src[j*4];
				dest[j*3+1]=src[j*4+1];
				dest[j*3+2]=src[j*4+2];
			}
			src += PaddedRowLength;
		}
		BitCount = 24;
	} else if (BitCount == 24) {
		int size = Width * Height * 3;
		pixels = malloc( size );
		unsigned char * dest = ( unsigned char * ) pixels;
		dest += size;
		unsigned char * src = ( unsigned char * ) rpixels;
		for (int i = Height; i; i--) {
			dest -= ( Width * 3 );
			memcpy( dest, src, Width * 3 );
			src += PaddedRowLength;
		}
	} else if (BitCount == 8) {
		pixels = malloc( Width * Height );
		unsigned char * dest = ( unsigned char * ) pixels;
		dest += Height * Width;
		unsigned char * src = ( unsigned char * ) rpixels;
		for (int i = Height; i; i--) {
			dest -= Width;
			memcpy( dest, src, Width );
			src += PaddedRowLength;
		}
	} else if (BitCount == 4) {
		Read4To8(rpixels); // FIXME: This is only needed for searchmap: we should really create a seperate representation.
		//Read4To4(rpixels);
	}
	free( rpixels );
	return true;
}

void BMPImp::Read8To8(void *rpixels)
{
	pixels = malloc( Width * Height );
	unsigned char * dest = ( unsigned char * ) pixels;
	dest += Height * Width;
	unsigned char * src = ( unsigned char * ) rpixels;
	for (int i = Height; i; i--) {
		dest -= Width;
		memcpy( dest, src, Width );
		src += PaddedRowLength;
	}
}

void BMPImp::Read4To8(void *rpixels)
{
	//converting it up to 8 bits, because we'll use it as
	//whole byte (searchmap)
	BitCount = 8;
	pixels = malloc( Width * Height );
	unsigned char * dest = ( unsigned char * ) pixels;
	dest += Height * Width;
	unsigned char * src = ( unsigned char * ) rpixels;
	for (int i = Height; i; i--) {
		dest -= Width;
		for (unsigned int j=0;j<Width;j++) {
			if (!(j&1)) {
				dest[j] = ((unsigned) src[j/2])>>4;
			} else {
				dest[j] = src[j/2]&15;
			}
		}
		src += PaddedRowLength;
	}
}

void BMPImp::Read4To4(void *rpixels)
{
	int size = PaddedRowLength * Height;
	pixels = malloc( size );
	unsigned char * dest = ( unsigned char * ) pixels;
	dest += size;
	unsigned char * src = ( unsigned char * ) rpixels;
	for (int i = Height; i; i--) {
		dest -= PaddedRowLength;
		memcpy( dest, src, PaddedRowLength );
		src += PaddedRowLength;
	}
}

bool BMPImp::OpenFromImage(Sprite2D* sprite, bool autoFree)
{
	if (sprite == NULL) {
		return false;
	}
	if (this->autoFree) {
		delete str;
	}

	free( pixels );
	pixels = NULL;
	free( Palette );
	Palette = NULL;

	// the previous stream was destructed and there won't be next
	this->autoFree = false;
	// FIXME: we assume 24bit sprite format here!

	Size = 0;
	Width = sprite->Width;
	Height = sprite->Height;
	Planes = 1;
	BitCount = sprite->Bpp;
	Compression = 0;
	ImageSize = 0;
	NumColors = 0;
	PaddedRowLength = Width * 3;
	if (PaddedRowLength & 3) {
		PaddedRowLength += 4 - ( PaddedRowLength & 3 );
	}

	Palette = NULL;
	// left out some other params

	pixels = malloc( Width * Height * (BitCount / 8) );
	memcpy( pixels, sprite->pixels, Width * Height * (BitCount / 8) );

	// FIXME: free the sprite if autoFree?
	if (autoFree) {
		core->GetVideoDriver()->FreeSprite(sprite);
	}

	return true;
}

Sprite2D* BMPImp::GetImage()
{
	Sprite2D* spr = NULL;
	if (BitCount == 24) {
		void* p = malloc( Width * Height * 3 );
		memcpy( p, pixels, Width * Height * 3 );
		spr = core->GetVideoDriver()->CreateSprite( Width, Height, 24,
			red_mask, green_mask, blue_mask, 0x00000000, p,
			true, green_mask );
	} else if (BitCount == 8) {
		void* p = malloc( Width* Height );
		memcpy( p, pixels, Width * Height );
		spr = core->GetVideoDriver()->CreateSprite8( Width, Height, NumColors==16?4:8,
			p, Palette, true, 0 );
	} else if (BitCount == 4) {
		void* p = malloc( Width* Height );
		unsigned char * dst = ( unsigned char * ) p;
		for (unsigned int y = 0; y < Height; y++) {
			unsigned char * src = ( unsigned char * ) pixels +
				( PaddedRowLength * y );
			for (unsigned int x = 0; x < Width; x++) {
				*dst = ( *src >> 4 );
				dst++;
				if (x == ( Width - 1 ))
					if (!( Width & 1 ))
						continue;
				*dst = ( *src & 0x0f );
				dst++;
				src++;
				x++;
			}
		}
		spr = core->GetVideoDriver()->CreateSprite8( Width, Height, 4,
			p, Palette, false );
	}
	return spr;
}
/** No descriptions */
void BMPImp::GetPalette(int index, int colors, Color* pal)
{
	if ((unsigned int) index>=Height) {
		index = 0;
	}
	if (BitCount == 24) {
		unsigned char * p = ( unsigned char * ) pixels;
		p += PaddedRowLength * index;
		for (int i = 0; i < colors; i++) {
			pal[i].b = *p++;
			pal[i].g = *p++;
			pal[i].r = *p++;
			pal[i].a = 0xff;
		}
	} else if (BitCount == 8) {
		int p = index;
		for (int i = 0; i < colors; i++) {
			pal[i].r = Palette[p].r;
			pal[i].g = Palette[p].g;
			pal[i].b = Palette[p++].b;
			pal[i].a = 0xff;
		}
	}
}


#define GET_SCANLINE_LENGTH(width, bitsperpixel)  (((width)*(bitsperpixel)+7)/8)

void BMPImp::PutImage(DataStream *output)
{
	ieDword tmpDword;
	ieWord tmpWord;

	// FIXME
	ieDword Width = GetWidth();
	ieDword Height = GetHeight();
	char filling[3] = {'B','M'};
	ieDword PaddedRowLength = GET_SCANLINE_LENGTH(Width,24);
	int stuff = (4-(PaddedRowLength&3))&3; // rounding it up to 4 bytes boundary
	PaddedRowLength+=stuff;
	ieDword fullsize = PaddedRowLength*Height;

	//always save in truecolor (24 bit), no palette
	output->Write( filling, 2);
	tmpDword = fullsize+BMP_HEADER_SIZE;  // FileSize
	output->WriteDword( &tmpDword);
	tmpDword = 0;
	output->WriteDword( &tmpDword);       // ??
	tmpDword = BMP_HEADER_SIZE;           // DataOffset
	output->WriteDword( &tmpDword);
	tmpDword = 40;                        // Size
	output->WriteDword( &tmpDword);
	output->WriteDword( &Width);
	output->WriteDword( &Height);
	tmpWord = 1;                          // Planes
	output->WriteWord( &tmpWord);
	tmpWord = 24; //24 bits               // BitCount
	output->WriteWord( &tmpWord);
	tmpDword = 0;                         // Compression
	output->WriteDword( &tmpDword);
	output->WriteDword( &tmpDword);       // ImageSize
	output->WriteDword( &tmpDword);
	output->WriteDword( &tmpDword);
	output->WriteDword( &tmpDword);
	output->WriteDword( &tmpDword);
	
	memset( filling,0,sizeof(filling) );
	for (unsigned int y=0;y<Height;y++) {
		for (unsigned int x=0;x<Width;x++) {
			Color c = GetPixel(x,Height-y-1);

			output->Write( &c.b, 1);
			output->Write( &c.g, 1);
			output->Write( &c.r, 1);
		}
		output->Write( filling, stuff);
	}
}

ImageFactory* BMPImp::GetImageFactory(const char* ResRef)
{
	ImageFactory* fact = new ImageFactory( ResRef, GetImage() );
	return fact;
}

#include "../../includes/plugindef.h"

GEMRB_PLUGIN(0xD768B1, "BMP File Reader")
PLUGIN_CLASS(IE_BMP_CLASS_ID, BMPImp)
PLUGIN_IE_RESOURCE(&ImageMgr::ID, BMPImp, ".bmp", (ieWord)IE_BMP_CLASS_ID)
END_PLUGIN()
