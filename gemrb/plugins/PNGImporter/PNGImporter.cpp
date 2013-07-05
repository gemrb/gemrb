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

#include "PNGImporter.h"

#include "RGBAColor.h"
#include "globals.h"

#include "ImageFactory.h"
#include "Interface.h"
#include "Video.h"

// CHECKME: how should we include png.h ? (And how should we check for it?)
#include <png.h>

using namespace GemRB;

static void DataStream_png_read_data(png_structp png_ptr,
		 png_bytep data, png_size_t length)
{
	void *read_io_ptr = png_get_io_ptr(png_ptr);
	DataStream* str = reinterpret_cast<DataStream*>(read_io_ptr);
	str->Read(data, length);
}

struct GemRB::PNGInternal {
	png_structp png_ptr;
	png_infop info_ptr;
	png_infop end_info;
};


static ieDword blue_mask = 0x00ff0000;
static ieDword green_mask = 0x0000ff00;
static ieDword red_mask = 0x000000ff;
static ieDword alpha_mask = 0xff000000;

PNGImporter::PNGImporter(void)
{
	inf = new PNGInternal();
	inf->png_ptr = 0;
	inf->info_ptr = 0;
	inf->end_info = 0;
}

PNGImporter::~PNGImporter(void)
{
	Close();
	delete inf;
}

void PNGImporter::Close()
{
	if (inf) {
		if (inf->png_ptr) {
			png_destroy_read_struct(&inf->png_ptr, &inf->info_ptr,
				&inf->end_info);
		}
		inf->png_ptr = 0;
		inf->info_ptr = 0;
		inf->end_info = 0;
	}
}

bool PNGImporter::Open(DataStream* stream)
{
	str = stream;
	Close();

	png_byte header[8];
	if (stream->Read(header, 8) < 8) return false;
	if (png_sig_cmp(header, 0, 8)) {
		return false;
	}
	inf->png_ptr = png_create_read_struct(
		PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (!inf->png_ptr)
		return false;

	inf->info_ptr = png_create_info_struct(inf->png_ptr);
	if (!inf->info_ptr)
	{
		png_destroy_read_struct(&inf->png_ptr, (png_infopp)NULL,
			(png_infopp)NULL);
		return false;
	}

	inf->end_info = png_create_info_struct(inf->png_ptr);
	if (!inf->end_info)
	{
		png_destroy_read_struct(&inf->png_ptr, &inf->info_ptr,
			(png_infopp)NULL);
		return false;
	}

	if (setjmp(png_jmpbuf(inf->png_ptr))) {
		png_destroy_read_struct(&inf->png_ptr, &inf->info_ptr, &inf->end_info);
		return false;
	}

	png_set_read_fn(inf->png_ptr, stream, DataStream_png_read_data);
	png_set_sig_bytes(inf->png_ptr, 8);

	png_read_info(inf->png_ptr, inf->info_ptr);

	png_uint_32 width, height;
	int bit_depth, color_type;
	int interlace_type, compression_type, filter_method;
	png_get_IHDR(inf->png_ptr, inf->info_ptr, &width, &height,
		 &bit_depth, &color_type,
		 &interlace_type, &compression_type, &filter_method);

	if (color_type != PNG_COLOR_TYPE_PALETTE &&
		png_get_valid(inf->png_ptr, inf->info_ptr, PNG_INFO_tRNS))
	{
		// if not indexed, turn transparency info into alpha
		// (if indexed, we use it directly for colorkeying)
		png_set_tRNS_to_alpha(inf->png_ptr);
	}

	if (bit_depth == 16)
		png_set_strip_16(inf->png_ptr);

	if (color_type == PNG_COLOR_TYPE_RGB)
		png_set_filler(inf->png_ptr, 0xFF, PNG_FILLER_AFTER);

	png_read_update_info(inf->png_ptr, inf->info_ptr);


	Width = width;
	Height = height;

	hasPalette = (color_type == PNG_COLOR_TYPE_PALETTE);

	return true;
}

Sprite2D* PNGImporter::GetSprite2D()
{
	Sprite2D* spr = 0;
	unsigned char* buffer = 0;
	png_bytep* row_pointers = new png_bytep[Height];
	buffer = (unsigned char *) malloc((hasPalette?1:4)*Width*Height);
	for (unsigned int i = 0; i < Height; ++i)
		row_pointers[i] = reinterpret_cast<png_bytep>(&buffer[(hasPalette?1:4)*i*Width]);

	if (setjmp(png_jmpbuf(inf->png_ptr))) {
		delete[] row_pointers;
		free( buffer );
		png_destroy_read_struct(&inf->png_ptr, &inf->info_ptr, &inf->end_info);
		return NULL;
	}

	png_read_image(inf->png_ptr, row_pointers);

	delete[] row_pointers;
	row_pointers = 0;

	// the end_info struct isn't used, but passing it anyway for now
	png_read_end(inf->png_ptr, inf->end_info);

	if (hasPalette) {
		Color pal[256];
		GetPalette(256, pal);
		// TODO: colorkey
		spr = core->GetVideoDriver()->CreatePalettedSprite(Width, Height, 8, buffer, pal);
	} else {
		spr = core->GetVideoDriver()->CreateSprite(Width, Height, 32,
												   red_mask, green_mask,
												   blue_mask, alpha_mask,
												   buffer, false, 0);
	}

	png_destroy_read_struct(&inf->png_ptr, &inf->info_ptr, &inf->end_info);

	return spr;
}

void PNGImporter::GetPalette(int colors, Color* pal)
{
	if (!hasPalette) {
		ImageMgr::GetPalette(colors, pal);
		return;
	}

	png_color* palette;
	int num_palette;
	png_get_PLTE(inf->png_ptr, inf->info_ptr, &palette, &num_palette);
	for (int i = 0; i < colors; i++) {
		pal[i].r = palette[i%num_palette].red;
		pal[i].g = palette[i%num_palette].green;
		pal[i].b = palette[i%num_palette].blue;
		pal[i].a = 0xff;
	}
}

#include "plugindef.h"

GEMRB_PLUGIN(0x11C3EB12, "PNG File Importer")
PLUGIN_IE_RESOURCE(PNGImporter, "png", (ieWord)IE_PNG_CLASS_ID)
END_PLUGIN()
