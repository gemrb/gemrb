/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

#include "Font.h"

#include "win32def.h"

#include "GameData.h"
#include "Interface.h"
#include "Palette.h"
#include "Sprite2D.h"
#include "Video.h"

#include <cassert>

namespace GemRB {

Font::Font()
: resRefs(NULL), numResRefs(0), palette(NULL), maxHeight(0)
{
	name[0] = '\0';
	multibyte = core->TLKEncoding.multibyte;
	utf8 = false;

	if (stricmp(core->TLKEncoding.encoding.c_str(), "UTF-8") == 0) {
		utf8 = true;
		assert(multibyte);
	}
}

Font::~Font(void)
{
	blank->release();
	SetPalette(NULL);
	free(resRefs);
}

bool Font::AddResRef(const ieResRef resref)
{
	if (resref) {
		resRefs = (ieResRef*)realloc(resRefs, sizeof(ieResRef) * ++numResRefs);
		strnlwrcpy( resRefs[numResRefs - 1], resref, sizeof(ieResRef)-1);
		return true;
	}
	return false;
}

bool Font::MatchesResRef(const ieResRef resref)
{
	for (int i=0; i < numResRefs; i++)
	{
		if (strnicmp( resref, resRefs[i], sizeof(ieResRef)-1) == 0){
			return true;
		}
	}
	return false;
}

void Font::Print(Region rgn, const unsigned char* string, Palette* hicolor,
	ieByte Alignment, bool anchor) const
{
	Region cliprgn = rgn;
	if (!anchor) {
		Region Viewport = core->GetVideoDriver()->GetViewport();
		cliprgn.x -= Viewport.x;
		cliprgn.y -= Viewport.y;
	}
	Print(cliprgn, rgn, string, hicolor, Alignment, anchor);
}

void Font::Print(Region cliprgn, Region rgn, const unsigned char* string,
	Palette* hicolor, ieByte Alignment, bool anchor) const
{
	unsigned int psx = IE_FONT_PADDING;
	Palette* pal = hicolor;
	if (!pal) {
		pal = palette;
	}

	Holder<Palette> blitPalette = pal;
	ieWord* tmp = NULL;
	size_t len = GetDoubleByteString(string, tmp);
	while (len > 0 && (tmp[len - 1] == '\n' || tmp[len - 1] == '\r')) {
		// ignore trailing newlines
		tmp[len - 1] = 0;
		len--;
	}

	SetupString( tmp, rgn.w );
	int ystep;
	if (Alignment & IE_FONT_SINGLE_LINE) {
		ystep = CalcStringHeight(tmp);
		if (!ystep) ystep = maxHeight;
	} else {
		ystep = maxHeight;
	}
	int x = psx, y = ystep;
	Video* video = core->GetVideoDriver();

	if (Alignment & IE_FONT_ALIGN_CENTER) {
		size_t w = CalcStringWidth( tmp );
		x = ( rgn.w - w ) / 2;
	} else if (Alignment & IE_FONT_ALIGN_RIGHT) {
		size_t w = CalcStringWidth( tmp );
		x = ( rgn.w - w ) - IE_FONT_PADDING;
	}

	if (Alignment & IE_FONT_ALIGN_MIDDLE) {
		int h = 0;
		for (size_t i = 0; i <= len; i++) {
			if (tmp[i] == 0)
				h++;
		}
		h = h * ystep;
		y += ( rgn.h - h ) / 2;
	} else if (Alignment & IE_FONT_ALIGN_BOTTOM) {
		int h = 0;
		for (size_t i = 0; i <= len; i++) {
			if (tmp[i] == 0)
				h++;
		}
		h = h * ystep;
		y += ( rgn.h - h ) - IE_FONT_PADDING;
	} else if (Alignment & IE_FONT_ALIGN_TOP) {
		y += IE_FONT_PADDING;
	}

	ieWord currChar = '\0';
	const Sprite2D* currGlyph = NULL;
	for (size_t i = 0; i < len; i++) {
		if (tmp[i] == 0) {
			y += ystep;
			x = psx;
			size_t w = CalcStringWidth( &tmp[i + 1] );
			if (Alignment & IE_FONT_ALIGN_CENTER) {
				x = ( rgn.w - w ) / 2;
			} else if (Alignment & IE_FONT_ALIGN_RIGHT) {
				x = ( rgn.w - w );
			}
			continue;
		}
		currChar = tmp[i];
		currGlyph = GetCharSprite(currChar);

		if (i > 0) {
			// kerning
			x -= GetKerningOffset(tmp[i-1], currChar);
		}

		video->BlitSprite(currGlyph, x + rgn.x, y + rgn.y, anchor, &cliprgn, blitPalette.get());

		x += currGlyph->Width;
	}

	blitPalette = NULL;
	free( tmp );
}

size_t Font::CalcStringWidth(const unsigned char* string) const
{
	ieWord* tmp = NULL;
	GetDoubleByteString(string, tmp);
	int width = CalcStringWidth(tmp);
	free(tmp);
	return width;
}

size_t Font::CalcStringWidth(const ieWord* string) const
{
	// TODO: it is a bit wasteful to recalc the string length when we already know it
	// I think switching to std::string or a custom cstring wrapper class will is the way to go
	size_t ret = 0, len = dbStrLen(string);
	for (size_t i = 0; i < len; i++) {
		ret += GetCharSprite(string[i])->Width;
	}
	return ret;
}

size_t Font::CalcStringHeight(const ieWord* string) const
{
	// TODO: it is a bit wasteful to recalc the string length when we already know it
	// I think switching to std::string or a custom cstring wrapper class will is the way to go
	size_t h = 0, max = 0, len = dbStrLen(string);
	for (unsigned int i = 0; i < len; i++) {
		h = GetCharSprite(string[i])->Height;
		//the space check is here to hack around overly high frames
		//in some bg1 fonts that throw vertical alignment off
		if (h > max && string[i] != ' ') {
			max = h;
		}
	}
	return max;
}

void Font::SetupString(ieWord* string, unsigned int width) const
{
	size_t len = dbStrLen(string);
	unsigned int psx = IE_FONT_PADDING;
	size_t lastpos = 0;
	unsigned int x = psx, wx = 0;
	bool endword = false;

	for (size_t pos = 0; pos < len; pos++) {
		if (x + wx > width) {
			// we wrapped, force a new line somewhere
			if (!endword && ( x == psx ))
				lastpos = pos;
			else
				string[lastpos] = 0;
			x = psx;
		}
		if (string[pos] == 0) {
			continue;
		}
		endword = false;
		if (string[pos] == '\r')
			string[pos] = ' ';
		if (string[pos] == '\n') {
			// force a new line here
			string[pos] = 0;
			x = psx;
			wx = 0;
			lastpos = pos;
			endword = true;
			continue;
		}

		wx += GetCharSprite(string[pos])->Width;

		if (( string[pos] == ' ' ) || ( string[pos] == '-' )) {
			x += wx;
			wx = 0;
			lastpos = pos;
			endword = true;
		}
	}
}

size_t Font::GetDoubleByteString(const unsigned char* string, ieWord* &dbString) const
{
	size_t len = strlen((char*)string);
	dbString = (ieWord*)malloc((len+1) * sizeof(ieWord));
	size_t dbLen = 0;
	for(size_t i=0; i<len; ++i)
	{
		ieWord currentChr = string[i];
		// we are assuming that every multibyte encoding uses single bytes for chars 32 - 127
		if( multibyte && (i+1 < len) && (currentChr >= 128 || currentChr < 32)) { // this is a double byte char
			if (utf8) {
				size_t nb = 0;
				if (currentChr >= 0xC0 && currentChr <= 0xDF) {
					/* c0-df are first byte of two-byte sequences (5+6=11 bits) */
					/* c0-c1 are noncanonical */
					nb = 2;
				} else if (currentChr >= 0xE0 && currentChr <= 0XEF) {
					/* e0-ef are first byte of three-byte (4+6+6=16 bits) */
					/* e0 80-9f are noncanonical */
					nb = 3;
				} else if (currentChr >= 0xF0 && currentChr <= 0XF7) {
					/* f0-f7 are first byte of four-byte (3+6+6+6=21 bits) */
					/* f0 80-8f are noncanonical */
					nb = 4;
				} else if (currentChr >= 0xF8 && currentChr <= 0XFB) {
					/* f8-fb are first byte of five-byte (2+6+6+6+6=26 bits) */
					/* f8 80-87 are noncanonical */
					nb = 5;
				} else if (currentChr >= 0xFC && currentChr <= 0XFD) {
					/* fc-fd are first byte of six-byte (1+6+6+6+6+6=31 bits) */
					/* fc 80-83 are noncanonical */
					nb = 6;
				} else {
					Log(WARNING, "Font", "Invalid UTF-8 character: %x", currentChr);
					continue;
				}

				ieWord ch = currentChr & ((1 << (7 - nb)) - 1);
				while (--nb)
					ch <<= 6, ch |= string[++i] & 0x3f;

				dbString[dbLen] = ch;
			} else {
				dbString[dbLen] = (string[++i] << 8) + currentChr;
			}
		} else {
			dbString[dbLen] = currentChr;
		}
		assert(dbString[dbLen] != 0); // premature end of string
		++dbLen;
	}
	dbString[dbLen] = '\0';

	// we dont always use everything we allocated.
	// realloc in this case to avoid static anylizer warnings about "garbage values"
	// since this realloc always truncates it *should* be quick
	dbString = (ieWord*)realloc(dbString, (dbLen+1) * sizeof(ieWord));

	return dbLen;
}

void Font::SetName(const char* newName)
{
	strnlwrcpy( name, newName, sizeof(name)-1);

	if (strnicmp(name, "STATES", 6) == 0) {
		// state fonts are NEVER multibyte; regardless of TKL encoding.
		multibyte = false;
	}
}

Palette* Font::GetPalette() const
{
	assert(palette);
	palette->acquire();
	return palette;
}

void Font::SetPalette(Palette* pal)
{
	if (pal) pal->acquire();
	if (palette) palette->release();
	palette = pal;
}

size_t Font::dbStrLen(const ieWord* string)
{
	if (string == NULL) return 0;
	int count = 0;
	for ( ; string[count] != 0; count++)
		continue; // intentionally empty loop
	return count;
}

}
