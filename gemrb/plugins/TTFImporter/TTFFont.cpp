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

namespace GemRB {

const Glyph& TTFFont::AliasBlank(ieWord chr) const
{
	((TTFFont*)this)->CreateAliasForChar(0, chr);
	return Font::GetGlyph(chr);
}

const Glyph& TTFFont::GetGlyph(ieWord chr) const
{
#if HAVE_ICONV
	if (!core->TLKEncoding.multibyte) {
		char* oldchar = (char*)&chr;
		ieWord unicodeChr = 0;
		char* newchar = (char*)&unicodeChr;
		size_t in = (core->TLKEncoding.widechar) ? 2 : 1, out = 2;

		// TODO: make this work on BE systems
		// TODO: maybe we want to work with non-unicode fonts?
		iconv_t cd = iconv_open("UTF-16LE", core->TLKEncoding.encoding.c_str());
	#if __FreeBSD__
		size_t ret = iconv(cd, (const char **)&oldchar, &in, &newchar, &out);
	#else
		size_t ret = iconv(cd, &oldchar, &in, &newchar, &out);
	#endif
		if (ret != GEM_OK) {
			Log(ERROR, "FONT", "iconv error: %d", errno);
		}
		iconv_close(cd);
		chr = unicodeChr;
	}
#endif
	// first check if the glyph already exists
	const Glyph& g = Font::GetGlyph(chr);
	if (g.pixels) {
		return g;
	}

	// attempt to generate glyph

	// TODO: fix the font styles!
	/*
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
	// x offset = cos(((90.0-12)/360)*2*M_PI), or 12 degree angle
	glyph_italics = 0.207f;
	glyph_italics *= height;
	*/
	FT_Error error = 0;
	FT_UInt index = FT_Get_Char_Index(face, chr);
	if (!index) {
		return AliasBlank(chr);
	}

	error = FT_Load_Glyph( face, index, FT_LOAD_DEFAULT | FT_LOAD_TARGET_MONO);
	if( error ) {
		LogFTError(error);
		return AliasBlank(chr);
	}

	FT_GlyphSlot glyph = face->glyph;
	FT_Glyph_Metrics* metrics = &glyph->metrics;
	/*
	//int maxx, yoffset;
	if ( FT_IS_SCALABLE( face ) ) {
		// Get the bounding box
		maxx = FT_FLOOR(metrics->horiBearingX) + FT_CEIL(metrics->width);
		yoffset = ascent - FT_FLOOR(metrics->horiBearingY);
	} else {
		// Get the bounding box for non-scalable format.
		// Again, freetype2 fills in many of the font metrics
		// with the value of 0, so some of the values we
		// need must be calculated differently with certain
		// assumptions about non-scalable formats.

		maxx = FT_FLOOR(metrics->horiBearingX) + FT_CEIL(metrics->horiAdvance);
		yoffset = 0;
	}

	// TODO: handle styles for fonts that dont do it themselves

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
		return AliasBlank(chr);
	}

	bitmap = &glyph->bitmap;

	Size sprSize(bitmap->width, bitmap->rows);

	/* Ensure the width of the pixmap is correct. On some cases,
	 * freetype may report a larger pixmap than possible.*/
	/*
	if (sprSize.w > maxx) {
		sprSize.w = maxx;
	}*/

	if (sprSize.IsEmpty()) {
		return AliasBlank(chr);
	}

	// we need 1px empty space on each side
	sprSize.w += 2;

	pixels = (uint8_t*)malloc(sprSize.w * sprSize.h);
	uint8_t* dest = pixels;
	uint8_t* src = bitmap->buffer;

	for( int row = 0; row < sprSize.h; row++ ) {
		// TODO: handle italics. we will need to offset the row by font->glyph_italics * row i think.

		// add 1px left padding
		memset(dest++, 0, 1);
		// -2 to account for padding
		memcpy(dest, src, sprSize.w - 2);
		dest += sprSize.w - 2;
		src += bitmap->pitch;
		// add 1px right padding
		memset(dest++, 0, 1);
	}
	// assert that we fill the buffer exactly
	assert((dest - pixels) == (sprSize.w * sprSize.h));

	// TODO: do an underline if requested

	Sprite2D* spr = core->GetVideoDriver()->CreateSprite8(sprSize.w, sprSize.h, pixels, NULL, true, 0);
	spr->YPos = FT_FLOOR(metrics->horiBearingY);
	// FIXME: casting away const
	const Glyph& ret = ((TTFFont*)this)->CreateGlyphForCharSprite(chr, spr);
	spr->release();
	return ret;
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
	return (int)(-kerning.x / 64);
}

TTFFont::TTFFont(Palette* pal, FT_Face face, int lineheight, int baseline)
	: Font(pal, lineheight, baseline), face(face)
{
// on FT < 2.4.2 the manager will defer ownership to this object
#if FREETYPE_VERSION_ATLEAST(2,4,2)
	FT_Reference_Face(face); // retain the face or the font manager will destroy it
#endif
	// ttf fonts dont produce glyphs for whitespace
	Sprite2D* blank = core->GetVideoDriver()->CreateSprite8(0, 0, NULL, NULL);
	// blank for returning when there is an error
	// TODO: ttf fonts have a "box" glyph they use for this
	CreateGlyphForCharSprite(0, blank);
	blank->Width = core->TLKEncoding.zerospace ? 1 : (LineHeight * 0.25);;
	CreateGlyphForCharSprite(' ', blank);
	blank->Width *= 4;
	CreateGlyphForCharSprite('\t', blank);
	blank->release();
}

TTFFont::~TTFFont()
{
	FT_Done_Face(face);
}

}
