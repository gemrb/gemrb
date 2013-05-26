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

#include "TTFFont.h"
#include "Interface.h"
#include "Sprite2D.h"
#include "Video.h"

#define uint8_t unsigned char

#if HAVE_ICONV
#include <iconv.h>
#include <errno.h>
#endif

/* Handy routines for converting from fixed point */
#define FT_FLOOR(X)	((X & -64) / 64)
#define FT_CEIL(X)	(((X + 63) & -64) / 64)

namespace GemRB {

const Sprite2D* TTFFont::GetCharSprite(ieWord chr) const
{
#if HAVE_ICONV
	if (!utf8) {
		char* oldchar = (char*)&chr;
		ieWord unicodeChr = 0;
		char* newchar = (char*)&unicodeChr;
		size_t in = (multibyte) ? 2 : 1, out = 2;

		// TODO: make this work on BE systems
		// TODO: maybe we want to work witn non-unicode fonts?
		iconv_t cd = iconv_open("UTF-16LE", core->TLKEncoding.encoding.c_str());
	#if __FreeBSD__
		int ret = iconv(cd, (const char **)&oldchar, &in, &newchar, &out);
	#else
		int ret = iconv(cd, &oldchar, &in, &newchar, &out);
	#endif
		if (ret != GEM_OK) {
			Log(ERROR, "FONT", "iconv error: %d", errno);
		}
		iconv_close(cd);
		chr = unicodeChr;
	}
#endif
	const Holder<Sprite2D>* sprCache = glyphCache->get(chr);
	if (sprCache) {
		// found in cache
		return sprCache->get();
	}

	// generate glyph

	// TODO: fix the font styles!
	FT_Error error = 0;
	FT_UInt index = FT_Get_Char_Index(face, chr);
	if (!index) {
		return blank;
	}

	int maxx, yoffset;
	error = FT_Load_Glyph( face, index, FT_LOAD_DEFAULT | FT_LOAD_TARGET_MONO);
	if( error ) {
		LogFTError(error);
		return blank;
	}

	FT_GlyphSlot glyph = face->glyph;
	FT_Glyph_Metrics* metrics = &glyph->metrics;

	/* Get the glyph metrics if desired */
	if ( FT_IS_SCALABLE( face ) ) {
		/* Get the bounding box */
		maxx = FT_FLOOR(metrics->horiBearingX) + FT_CEIL(metrics->width);
		yoffset = ascent - FT_FLOOR(metrics->horiBearingY);
	} else {
		/* Get the bounding box for non-scalable format.
		 * Again, freetype2 fills in many of the font metrics
		 * with the value of 0, so some of the values we
		 * need must be calculated differently with certain
		 * assumptions about non-scalable formats.
		 * */
		maxx = FT_FLOOR(metrics->horiBearingX) + FT_CEIL(metrics->horiAdvance);
		yoffset = 0;
	}

	// TODO: handle styles for fonts that dont do it themselves

	/*
	 FIXME: maxx is currently unused.
	 glyph spacing is non existant right now
	 font styles are non functional too
	 */

	FT_Bitmap* bitmap;
	uint8_t* pixels = NULL;

	/* Render the glyph */
	error = FT_Render_Glyph( glyph, ft_render_mode_normal );
	if( error ) {
		LogFTError(error);
		return blank;
	}

	bitmap = &glyph->bitmap;

	int sprHeight = bitmap->rows;
	int sprWidth = bitmap->width;

	/* Ensure the width of the pixmap is correct. On some cases,
	 * freetype may report a larger pixmap than possible.*/
	if (sprWidth > maxx) {
		sprWidth = maxx;
	}

	if (sprWidth == 0 || sprHeight == 0) {
		return blank;
	}

	// we need 1px empty space on each side
	sprWidth += 2;

	pixels = (uint8_t*)malloc(sprWidth * sprHeight);
	uint8_t* dest = pixels;
	uint8_t* src = bitmap->buffer;

	for( int row = 0; row < sprHeight; row++ ) {
		// TODO: handle italics. we will need to offset the row by font->glyph_italics * row i think.

		// add 1px left padding
		memset(dest++, 0, 1);
		// -2 to account for padding
		memcpy(dest, src, sprWidth - 2);
		dest += sprWidth - 2;
		src += bitmap->pitch;
		// add 1px right padding
		memset(dest++, 0, 1);
	}
	// assert that we fill the buffer exactly
	assert((dest - pixels) == (sprWidth * sprHeight));

	// TODO: do an underline if requested

	Sprite2D* spr = core->GetVideoDriver()->CreateSprite8(sprWidth, sprHeight, 8, pixels, palette, true, 0);
	// for some reason BAM fonts are all based of a YPos of 13
	spr->YPos = 13 - yoffset;
	// cache the glyph
	glyphCache->set(chr, spr);
	spr->release(); // retained by the cache holder
	return spr;
}

int TTFFont::GetKerningOffset(ieWord leftChr, ieWord rightChr) const
{
	FT_UInt leftIndex = FT_Get_Char_Index(face, leftChr);
	FT_UInt rightIndex = FT_Get_Char_Index(face, rightChr);
	FT_Vector kerning = {0,0};
	// FT_KERNING_DEFAULT is "grid fitted". we will end up with a scaled multiple of 64
	FT_Error error = FT_Get_Kerning(face, leftIndex, rightIndex, FT_KERNING_DEFAULT, &kerning);
	if (error) {
		LogFTError(error);
		return 0;
	}
	// kerning is in 26.6 format. basically divide by 64 to get the number of pixels.
	return -kerning.x / 64;
}

TTFFont::TTFFont(FT_Face face, ieWord ptSize, FontStyle style, Palette* pal)
	: style(style), ptSize(ptSize), face(face)
{
	glyphCache = new HashMap<ieWord, Holder<Sprite2D> >();
	glyphCache->init(256, 128);

// on FT < 2.4.2 the manager will difer ownership to this object
#if FREETYPE_VERSION_ATLEAST(2,4,2)
	FT_Reference_Face(face); // retain the face or the font manager will destroy it
#endif
	FT_Error error = 0;

	/* Make sure that our font face is scalable (global metrics) */
	if ( FT_IS_SCALABLE(face) ) {
		FT_Fixed scale;
		/* Set the character size and use default DPI (72) */
		error = FT_Set_Char_Size( face, 0, ptSize * 64, 0, 0 );
		if( error ) {
			LogFTError(error);
		} else {
			/* Get the scalable font metrics for this font */
			scale = face->size->metrics.y_scale;
			ascent = FT_CEIL(FT_MulFix(face->ascender, scale));
			descent = FT_CEIL(FT_MulFix(face->descender, scale));
			maxHeight = ascent - descent + 1;
			//font->lineskip = FT_CEIL(FT_MulFix(face->height, scale));
			//font->underline_offset = FT_FLOOR(FT_MulFix(face->underline_position, scale));
			//font->underline_height = FT_FLOOR(FT_MulFix(face->underline_thickness, scale));
		}
	} else {
		/* Non-scalable font case.  ptsize determines which family
		 * or series of fonts to grab from the non-scalable format.
		 * It is not the point size of the font.
		 * */
		if ( ptSize >= face->num_fixed_sizes )
			ptSize = face->num_fixed_sizes - 1;

		error = FT_Set_Pixel_Sizes( face,
								   face->available_sizes[ptSize].height,
								   face->available_sizes[ptSize].width );

		if (error) {
			LogFTError(error);
		}
		/* With non-scalale fonts, Freetype2 likes to fill many of the
		 * font metrics with the value of 0.  The size of the
		 * non-scalable fonts must be determined differently
		 * or sometimes cannot be determined.
		 * */
		ascent = face->available_sizes[ptSize].height;
		descent = 0;
		maxHeight = face->available_sizes[ptSize].height;
		//font->lineskip = FT_CEIL(font->ascent);
		//font->underline_offset = FT_FLOOR(face->underline_position);
		//font->underline_height = FT_FLOOR(face->underline_thickness);
	}

	/*
	 if ( font->underline_height < 1 ) {
	 font->underline_height = 1;
	 }
	 */

	// Initialize the font face style
	// currently gemrb has exclusive styles...
	// TODO: make styles ORable
	style = NORMAL;
	if ( face->style_flags & FT_STYLE_FLAG_ITALIC ) {
		style = ITALIC;
	}
	if ( face->style_flags & FT_STYLE_FLAG_BOLD ) {
		// bold overrides italic
		// TODO: allow bold and italic together
		style = BOLD;
	}

	glyph_overhang = face->size->metrics.y_ppem / 10;
	/* x offset = cos(((90.0-12)/360)*2*M_PI), or 12 degree angle */
	glyph_italics = 0.207f;
	glyph_italics *= height;

	SetPalette(pal);

	// TODO: ttf fonts have a "box" glyph they use for this
	blank = core->GetVideoDriver()->CreateSprite8(0, 0, 8, NULL, palette->col);
	// ttf fonts dont produce glyphs for whitespace
	int SpaceWidth = core->TLKEncoding.zerospace ? 1 : (ptSize * 0.25);
	Sprite2D* space = core->GetVideoDriver()->CreateSprite8(SpaceWidth, 0, 8, NULL, palette->col);;
	Sprite2D* tab = core->GetVideoDriver()->CreateSprite8((space->Width)*4, 0, 8, NULL, palette->col);

	// now cache these glyphs for quick access
	// WARNING: if we ever did something to purge the cache these would be lost
	glyphCache->set(' ', space);
	glyphCache->set('\t', tab);
	// retained by the cache
	space->release();
	tab->release();
}

TTFFont::~TTFFont()
{
	FT_Done_Face(face);
	delete glyphCache;
}

}
