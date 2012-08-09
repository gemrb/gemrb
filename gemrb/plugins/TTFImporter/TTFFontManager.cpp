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

#include <math.h>

#include "TTFFontManager.h"

#include "win32def.h"

#include "FileCache.h"
#include "GameData.h"
#include "Interface.h"
#include "Palette.h"
#include "Sprite2D.h"
#include "Video.h"
#include "System/FileStream.h"

/* Handy routines for converting from fixed point */
#define FT_FLOOR(X)	((X & -64) / 64)
#define FT_CEIL(X)	(((X + 63) & -64) / 64)

/* Handle a style only if the font does not already handle it */
#define TTF_HANDLE_STYLE_BOLD(font) ((style & BOLD) && \
!(font.face_style & BOLD))
#define TTF_HANDLE_STYLE_ITALIC(font) ((style & ITALIC) && \
!(font.face_style & ITALIC))
#define TTF_HANDLE_STYLE_UNDERLINE(font) (style & UNDERLINE)

using namespace GemRB;

unsigned long TTFFontManager::read(FT_Stream		  stream,
								   unsigned long   offset,
								   unsigned char*  buffer,
								   unsigned long   count )
{
	DataStream* dstream = (DataStream*)stream->descriptor.pointer;
	dstream->Seek(offset, GEM_STREAM_START);
	return dstream->Read(buffer, count);
}

TTFFontManager::~TTFFontManager(void)
{
	Close();
	if (library) {
		FT_Done_FreeType( library );
	}
}

TTFFontManager::TTFFontManager(void)
: ftStream(NULL), font()
{
	FT_Error error = FT_Init_FreeType( &library );
	if ( error ) {
		LogFTError(error);
	}
}

void TTFFontManager::LogFTError(FT_Error errCode) const
{
#undef __FTERRORS_H__
#define FT_ERRORDEF( e, v, s )  { e, s },
#define FT_ERROR_START_LIST     {
#define FT_ERROR_END_LIST       { 0, 0 } };

	static const struct
	{
		int          err_code;
		const char*  err_msg;
	} ft_errors[] =
		 #include FT_ERRORS_H
	int i;
	const char *err_msg;

	err_msg = NULL;
	for ( i=0; i < (int)((sizeof ft_errors)/(sizeof ft_errors[0])); ++i ) {
		if ( errCode == ft_errors[i].err_code ) {
			err_msg = ft_errors[i].err_msg;
			break;
		}
	}
	if ( !err_msg ) {
		err_msg = "unknown FreeType error";
	}
	Log(ERROR, "TTF Manager", "%s", err_msg);
}

bool TTFFontManager::Open(DataStream* stream)
{
	Close();
	if (stream) {
		FT_Error error;
		FT_CharMap found;

		ftStream = (FT_Stream)calloc(sizeof(*ftStream), 1);
		ftStream->read = read;
		ftStream->descriptor.pointer = stream;
		ftStream->pos = stream->GetPos();
		ftStream->size = stream->Size();

		FT_Open_Args args = FT_Open_Args();
		args.flags = FT_OPEN_STREAM;
		args.stream = ftStream;

		font.face = NULL;
		error = FT_Open_Face( library, &args, 0, &font.face );
		if( error ) {
			LogFTError(error);
			Close();
			return false;
		}

		/* Set charmap for loaded font */
		found = 0;
		FT_Face face = font.face;
		for (int i = 0; i < face->num_charmaps; i++) {
			FT_CharMap charmap = face->charmaps[i];
			if ((charmap->platform_id == 3 && charmap->encoding_id == 1) /* Windows Unicode */
				|| (charmap->platform_id == 3 && charmap->encoding_id == 0) /* Windows Symbol */
				|| (charmap->platform_id == 2 && charmap->encoding_id == 1) /* ISO Unicode */
				|| (charmap->platform_id == 0)) { /* Apple Unicode */
				found = charmap;
				break;
			}
		}
		if ( found ) {
			/* If this fails, continue using the default charmap */
			FT_Set_Charmap(face, found);
		}

		return true;
	}
	return false;
}

void TTFFontManager::Close()
{
	if (font.face) {
		FT_Done_Face(font.face);
	}
	if (ftStream) {
		free(ftStream);
		ftStream = NULL;
	}
	font = TTF_Font();
}

Font* TTFFontManager::GetFont(ieWord FirstChar,
							  ieWord LastChar,
							  unsigned short ptSize,
							  FontStyle style, Palette* pal)
{
	Log(MESSAGE, "TTF", "Constructing TTF font.");
	Log(MESSAGE, "TTF", "Creating font of size %i with %i characters...", ptSize, LastChar - FirstChar);

	/* Make sure that our font face is scalable (global metrics) */
	FT_Face face = font.face;
	FT_Fixed scale;
	FT_Error error;
	if ( FT_IS_SCALABLE(face) ) {
		/* Set the character size and use default DPI (72) */
		error = FT_Set_Char_Size( font.face, 0, ptSize * 64, 0, 0 );
		if( error ) {
			LogFTError(error);
			Close();
			return NULL;
		}

		/* Get the scalable font metrics for this font */
		scale = face->size->metrics.y_scale;
		font.ascent = FT_CEIL(FT_MulFix(face->ascender, scale));
		font.descent = FT_CEIL(FT_MulFix(face->descender, scale));
		font.height  = font.ascent - font.descent + 1;
		//font->lineskip = FT_CEIL(FT_MulFix(face->height, scale));
		//font->underline_offset = FT_FLOOR(FT_MulFix(face->underline_position, scale));
		//font->underline_height = FT_FLOOR(FT_MulFix(face->underline_thickness, scale));
	} else {
		/* Non-scalable font case.  ptsize determines which family
		 * or series of fonts to grab from the non-scalable format.
		 * It is not the point size of the font.
		 * */
		if ( ptSize >= font.face->num_fixed_sizes )
			ptSize = font.face->num_fixed_sizes - 1;
		//font.font_size_family = ptsize;
		error = FT_Set_Pixel_Sizes( face,
								   face->available_sizes[ptSize].height,
								   face->available_sizes[ptSize].width );
		/* With non-scalale fonts, Freetype2 likes to fill many of the
		 * font metrics with the value of 0.  The size of the
		 * non-scalable fonts must be determined differently
		 * or sometimes cannot be determined.
		 * */
		font.ascent = face->available_sizes[ptSize].height;
		font.descent = 0;
		font.height = face->available_sizes[ptSize].height;
		//font->lineskip = FT_CEIL(font->ascent);
		//font->underline_offset = FT_FLOOR(face->underline_position);
		//font->underline_height = FT_FLOOR(face->underline_thickness);
	}

	/*
	if ( font->underline_height < 1 ) {
		font->underline_height = 1;
	}
*/

	/* Initialize the font face style */
	font.face_style = NORMAL;
	if ( font.face->style_flags & FT_STYLE_FLAG_BOLD ) {
		font.face_style |= BOLD;
	}
	if ( font.face->style_flags & FT_STYLE_FLAG_ITALIC ) {
		font.face_style |= ITALIC;
	}

	font.glyph_overhang = face->size->metrics.y_ppem / 10;
	/* x offset = cos(((90.0-12)/360)*2*M_PI), or 12 degree angle */
	font.glyph_italics = 0.207f;
	font.glyph_italics *= font.height;

	if (!pal) {
		Color fore = {0xFF, 0xFF, 0xFF, 0}; //white
		Color back = {0x00, 0x00, 0x00, 0}; //black
		pal = core->CreatePalette( fore, back );
		pal->CreateShadedAlphaChannel();
	}

	Sprite2D** glyphs = (Sprite2D**)malloc((LastChar - FirstChar + 1) * sizeof(Sprite2D*));

	// use ieWord for double byte character suport
	ieWord index, ch;

	FT_GlyphSlot glyph;
	FT_Glyph_Metrics* metrics;

	int maxx, maxy, yoffset, advance;
	for (ch = FirstChar; ch <= LastChar; ch++) {
		/* Load the glyph */
		index = FT_Get_Char_Index( face, ch );
		// maybe one day we will subclass Font such that we can be more dynamic and support kerning.
		// until then load the glyphs as monospace.
		error = FT_Load_Glyph( face, index, FT_LOAD_DEFAULT | FT_LOAD_TARGET_MONO);
		if( error ) {
			LogFTError(error);
			glyphs[ch - FirstChar] = NULL;
			continue;
		}

		glyph = face->glyph;
		metrics = &glyph->metrics;

		/* Get the glyph metrics if desired */
		if ( FT_IS_SCALABLE( face ) ) {
			/* Get the bounding box */
			maxx = FT_FLOOR(metrics->horiBearingX) + FT_CEIL(metrics->width);
			maxy = FT_FLOOR(metrics->horiBearingY);
			yoffset = font.ascent - maxy;
			advance = FT_CEIL(metrics->horiAdvance);
		} else {
			/* Get the bounding box for non-scalable format.
			 * Again, freetype2 fills in many of the font metrics
			 * with the value of 0, so some of the values we
			 * need must be calculated differently with certain
			 * assumptions about non-scalable formats.
			 * */
			maxx = FT_FLOOR(metrics->horiBearingX) + FT_CEIL(metrics->horiAdvance);
			maxy = FT_FLOOR(metrics->horiBearingY);
			yoffset = 0;
			advance = FT_CEIL(metrics->horiAdvance);
		}

		/* Adjust for bold and italic text */
		if( TTF_HANDLE_STYLE_BOLD(font) ) {
			maxx += font.glyph_overhang;
		}
		if( TTF_HANDLE_STYLE_ITALIC(font) ) {
			maxx += (int)ceil(font.glyph_italics);
		}

		int sprWidth, sprHeight;
		sprWidth = maxx;
		sprHeight = font.ascent;

		if (sprWidth == 0 || sprHeight == 0) {
			glyphs[ch - FirstChar] = NULL;
			continue;
		}

		FT_Bitmap* bitmap;
		uint8_t* pixels = NULL;

		/* Render the glyph */
		error = FT_Render_Glyph( glyph, ft_render_mode_normal );
		if( error ) {
			LogFTError(error);
			glyphs[ch - FirstChar] = NULL;
			continue;
		}

		bitmap = &glyph->bitmap;
		pixels = (uint8_t*)calloc(sprWidth, sprHeight);

		/* Ensure the width of the pixmap is correct. On some cases,
		 * freetype may report a larger pixmap than possible.*/
		int width = glyph->bitmap.width;
		if (width > sprWidth) {
			width = sprWidth;
		}

		for( int row = 0; row < bitmap->rows; ++row ) {
			memcpy(pixels+(row * sprWidth), bitmap->buffer+(row * bitmap->pitch), bitmap->pitch);
		}

		// TODO: do an underline if requested

		glyphs[ch - FirstChar] = core->GetVideoDriver()->CreateSprite8(sprWidth, sprHeight, 8, pixels, pal->col, true, 0);
		// FIXME: figure out why this 10 is required and find how to dynamically determine
		// presumably this value should change with font size.
		glyphs[ch - FirstChar]->YPos = 10 - yoffset;
	}
	Font* font = new Font(glyphs, FirstChar, LastChar, pal);
	font->ptSize = ptSize;
	font->style = style;
	return font;
}

#include "plugindef.h"

GEMRB_PLUGIN(0x3AD6427C, "TTF Font Importer")
PLUGIN_RESOURCE(TTFFontManager, "ttf")
END_PLUGIN()
