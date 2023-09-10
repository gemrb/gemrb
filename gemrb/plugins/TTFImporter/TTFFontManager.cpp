/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2011 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

#include "Palette.h"
#include "TTFFont.h"
#include "TTFFontManager.h"

namespace GemRB {

struct CoreSettings;

FT_Library library = NULL;

static void loadFT(const CoreSettings&)
{
	FT_Error error = FT_Init_FreeType( &library );
	if ( error ) {
		LogFTError(error);
	}
}

static void destroyFT()
{
	if (library) {
		FT_Done_FreeType( library );
		library = NULL;
	}
}

TTFFontManager::~TTFFontManager(void)
{
	Close();
}

bool TTFFontManager::Import(DataStream* stream)
{
	Close();
	if (stream) {
		FT_Error error;
		ftStream = new std::remove_pointer_t<FT_Stream>{};
		ftStream->read = [](FT_Stream TTFStream, unsigned long	offset, unsigned char* buffer, unsigned long count)
		-> unsigned long {
			DataStream* dstream = static_cast<DataStream*>(TTFStream->descriptor.pointer);
			dstream->Seek(offset, GEM_STREAM_START);
			return dstream->Read(buffer, count);
		};
		
		ftStream->close = [](FT_Stream TTFStream){
			delete static_cast<DataStream*>(TTFStream->descriptor.pointer);
			delete TTFStream;
		};
		ftStream->descriptor.pointer = stream->Clone();
		ftStream->pos = stream->GetPos();
		ftStream->size = stream->Size();
		
		FT_Open_Args args = FT_Open_Args();
		args.flags = FT_OPEN_STREAM;
		args.stream = ftStream;
		
		error = FT_Open_Face( library, &args, 0, &face );
		if( error ) {
			LogFTError(error);
			Close();
			return false;
		}
		
		// we always convert to UTF-16
		// TODO: maybe we should allow an override encoding?
		FT_Select_Charmap(face, FT_ENCODING_UNICODE);
		return true;
	}
	return false;
}

void TTFFontManager::Close()
{
	if (face) {
		FT_Done_Face(face);
	}
}

Holder<Font> TTFFontManager::GetFont(unsigned short pxSize, FontStyle /*style*/, bool /*background*/)
{
	Holder<Palette> pal = MakeHolder<Palette>(ColorWhite, ColorBlack);
	
	// FIXME: we probably dont need to do this
	// Font already can do blending between fg and bg colors
	for (int i = 1; i < 256; ++i) {
		Color& c = pal->col[i];
		unsigned int m = (c.r + c.g + c.b) / 3;
		if (m > 2) {
			int tmp = m * MUL;
			c.a = std::min(tmp, 0xff);
		} else {
			c.a = 0;
		}
	}
	
	FT_Error error = 0;
	ieWord lineHeight = 0, baseline = 0;
	/* Make sure that our font face is scalable (global metrics) */
	if ( FT_IS_SCALABLE(face) ) {
		FT_Fixed scale;
		/* Set the character size and use default DPI (72) */
		error = FT_Set_Pixel_Sizes( face, 0, pxSize );
		if( error ) {
			LogFTError(error);
		} else {
			/* Get the scalable font metrics for this font */
			scale = face->size->metrics.y_scale;
			baseline = FT_CEIL(FT_MulFix(face->ascender, scale));
			int descent = FT_CEIL(FT_MulFix(face->descender, scale));
			lineHeight = baseline - descent;
			//font->lineskip = FT_CEIL(FT_MulFix(face->height, scale));
			//font->underline_offset = FT_FLOOR(FT_MulFix(face->underline_position, scale));
			//font->underline_height = FT_FLOOR(FT_MulFix(face->underline_thickness, scale));
		}
	} else {
		/* Non-scalable font case.  ptsize determines which family
		 * or series of fonts to grab from the non-scalable format.
		 * It is not the point size of the font.
		 * */
		if ( pxSize >= face->num_fixed_sizes )
			pxSize = face->num_fixed_sizes - 1;
		
		error = FT_Set_Pixel_Sizes( face,
								   face->available_sizes[pxSize].height,
								   face->available_sizes[pxSize].width );
		
		if (error) {
			LogFTError(error);
		}
		/* With non-scalale fonts, Freetype2 likes to fill many of the
		 * font metrics with the value of 0.  The size of the
		 * non-scalable fonts must be determined differently
		 * or sometimes cannot be determined.
		 * */
		
		lineHeight = face->available_sizes[pxSize].height;
		//font->lineskip = FT_CEIL(font->ascent);
		//font->underline_offset = FT_FLOOR(face->underline_position);
		//font->underline_height = FT_FLOOR(face->underline_thickness);
	}
	
	return MakeHolder<TTFFont>(pal, face, lineHeight, baseline);
}

}

#include "plugindef.h"

GEMRB_PLUGIN(0x3AD6427C, "TTF Font Importer")
PLUGIN_RESOURCE(TTFFontManager, "ttf")
PLUGIN_INITIALIZER(loadFT)
PLUGIN_CLEANUP(destroyFT)
END_PLUGIN()
