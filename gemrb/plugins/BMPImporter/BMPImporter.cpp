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
 *
 */

#include "BMPImporter.h"

#include "RGBAColor.h"

#include "Logging/Logging.h"
#include "Video/Video.h"

using namespace GemRB;

#define BMP_HEADER_SIZE  54

BMPImporter::~BMPImporter(void)
{
	free(PaletteColors);
	free( pixels );
}

bool BMPImporter::Import(DataStream* str)
{
	//we release the previous pixel data
	free( pixels );
	pixels = NULL;
	free(PaletteColors);
	PaletteColors = nullptr;

	//BITMAPFILEHEADER
	char Signature[2];
	ieDword FileSize, DataOffset;

	str->Read( Signature, 2 );
	if (strncmp( Signature, "BM", 2 ) != 0) {
		Log(ERROR, "BMPImporter", "Not a valid BMP File.");
		return false;
	}
	str->ReadDword(FileSize);
	str->Seek( 4, GEM_CURRENT_POS );
	str->ReadDword(DataOffset);

	//BITMAPINFOHEADER

	str->ReadDword(Size);
	//some IE palettes are of a different format (OS/2 BMP)!
	if (Size < 24) {
		Log(ERROR, "BMPImporter", "OS/2 Bitmap, not supported.");
		return false;
	}
	ieDword tmp;
	str->ReadDword(tmp);
	size.w = tmp;
	str->ReadDword(tmp);
	size.h = tmp;
	str->ReadWord(Planes);
	str->ReadWord(BitCount);
	str->ReadDword(Compression);
	str->ReadDword(ImageSize);
	//24 = the already read bytes 3x4+2x2+2x4
	//this is normally 16
	str->Seek( Size-24, GEM_CURRENT_POS );
	//str->ReadDword(Hres);
	//str->ReadDword(Vres);
	//str->ReadDword(ColorsUsed);
	//str->ReadDword(ColorsImportant);
	if (Compression != 0) {
		Log(ERROR, "BMPImporter", "Compressed {}-bits Image, not supported.", BitCount);
		return false;
	}
	//COLORTABLE
	if (BitCount <= 8) {
		if (BitCount == 8)
			NumColors = 256;
		else
			NumColors = 16;
		PaletteColors = (Color *)malloc(4 * NumColors);
//		memset(Palette, 0, 4 * NumColors);
		for (unsigned int i = 0; i < NumColors; i++) {
			str->Read(&PaletteColors[i].b, 1);
			str->Read(&PaletteColors[i].g, 1);
			str->Read(&PaletteColors[i].r, 1);
			str->Read(&PaletteColors[i].a, 1);
			PaletteColors[i].a = (PaletteColors[i].a == 0) ? 0xff : PaletteColors[i].a;
		}
	}
	str->Seek( DataOffset, GEM_STREAM_START );
	//no idea if we have to swap this or not
	//RASTERDATA
	switch (BitCount) {
		case 32:
		case 24:
		case 16:
		case 8:
			PaddedRowLength = size.w * BitCount / 8;
			break;
		case 4:
			PaddedRowLength = (size.w >> 1);
			break;
		default:
			Log(ERROR, "BMPImporter", "BitCount {} is not supported.", BitCount);
			return false;
	}
	//if(BitCount!=4)
	//{
	if (PaddedRowLength & 3) {
		PaddedRowLength += 4 - ( PaddedRowLength & 3 );
	}
	//}
	void* rpixels = malloc(PaddedRowLength * size.h);
	str->Read(rpixels, PaddedRowLength * size.h);
	if (BitCount == 32) {
		int numbytes = size.Area() * 4;
		pixels = malloc(numbytes);
		unsigned int * dest = ( unsigned int * ) pixels;
		dest += size.Area();
		const unsigned char* src = (const unsigned char *) rpixels;
		for (int i = size.h; i; i--) {
			dest -= size.w;
			// BGRX
			for (int j=0; j < size.w; ++j)
				dest[j] = (0xFF << 24) | (src[j*4+0] << 16) |
				          (src[j*4+1] << 8) | (src[j*4+2]);
			src += PaddedRowLength;
		}
	} else if (BitCount == 24) {
		//convert to 32 bits on the fly
		int numbytes = size.Area() * 4;
		pixels = malloc(numbytes);
		unsigned int * dest = ( unsigned int * ) pixels;
		dest += size.Area();
		const unsigned char* src = (const unsigned char *) rpixels;
		for (int i = size.h; i; i--) {
			dest -= size.w;
			// BGR
			for (int j = 0; j < size.w; ++j)
				dest[j] = (0xFF << 24) | (src[j*3+0] << 16) |
				          (src[j*3+1] << 8) | (src[j*3+2]);
			src += PaddedRowLength;
		}
		BitCount = 32;
	} else if (BitCount == 8) {
		Read8To8(rpixels);
	} else if (BitCount == 4) {
		Read4To8(rpixels);
	}
	free( rpixels );
	return true;
}

void BMPImporter::Read8To8(const void *rpixels)
{
	pixels = malloc(size.Area());
	unsigned char * dest = ( unsigned char * ) pixels;
	dest += size.Area();
	const unsigned char* src = (const unsigned char *) rpixels;
	for (int i = size.h; i; i--) {
		dest -= size.w;
		memcpy(dest, src, size.w);
		src += PaddedRowLength;
	}
}

void BMPImporter::Read4To8(const void *rpixels)
{
	BitCount = 8;
	pixels = malloc(size.Area());
	unsigned char * dest = ( unsigned char * ) pixels;
	dest += size.Area();
	const unsigned char* src = (const unsigned char *) rpixels;
	for (int i = size.h; i; i--) {
		dest -= size.w;
		for (int j = 0; j < size.w; ++j) {
			if (!(j&1)) {
				dest[j] = ((unsigned) src[j/2])>>4;
			} else {
				dest[j] = src[j/2]&15;
			}
		}
		src += PaddedRowLength;
	}
}

Holder<Sprite2D> BMPImporter::GetSprite2D()
{
	Holder<Sprite2D> spr;
	if (BitCount == 32) {
		constexpr uint32_t red_mask = 0x000000ff;
		constexpr uint32_t green_mask = 0x0000ff00;
		constexpr uint32_t blue_mask = 0x00ff0000;
		PixelFormat fmt(4, red_mask, green_mask, blue_mask, 0);
		fmt.HasColorKey = true;
		fmt.ColorKey = green_mask | (0xff << 24);

		spr = VideoDriver->CreateSprite(Region(0,0, size.w, size.h), nullptr, fmt);
		memcpy(spr->LockSprite(), pixels, size.Area() * 4);
		spr->UnlockSprite();
	} else if (BitCount == 8) {
		PaletteHolder pal = MakeHolder<Palette>(PaletteColors, PaletteColors + NumColors);
		PixelFormat fmt = PixelFormat::Paletted8Bit(pal, pal->col[0] == ColorGreen, 0);
		spr = VideoDriver->CreateSprite(Region(0,0, size.w, size.h), nullptr, fmt);
		const uint8_t* src = static_cast<uint8_t*>(pixels);
		const uint8_t* end = src + size.Area();
		auto dst = spr->GetIterator();
		for (; src != end; ++src, ++dst) {
			*dst = *src;
		}
	}

	return spr;
}

int BMPImporter::GetPalette(int colors, Color* pal)
{
	if (BitCount > 8) {
		return ImageMgr::GetPalette(colors, pal);
	}

	for (int i = 0; i < colors; i++) {
		pal[i].r = PaletteColors[i % NumColors].r;
		pal[i].g = PaletteColors[i % NumColors].g;
		pal[i].b = PaletteColors[i % NumColors].b;
		pal[i].a = 0xff;
	}
	return -1;
}

#include "plugindef.h"

GEMRB_PLUGIN(0xD768B1, "BMP File Reader")
PLUGIN_IE_RESOURCE(BMPImporter, "bmp", (ieWord)IE_BMP_CLASS_ID)
END_PLUGIN()
