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

void Font::PrintFromLine(int startrow, Region rgn, const unsigned char* string,
	Palette* hicolor, ieByte Alignment, Font* initials,
	Sprite2D* cursor, unsigned int curpos, bool NoColor) const
{
	bool enablecap=false;
	int capital = 0;
	int initials_rows = 0;
	int last_initial_row = 0;
	int initials_x = 0;
	int initials_row = 0;
	ieWord currCap = 0;
	ieWord* tmp = NULL;
	size_t len = GetDoubleByteString(string, tmp);

	ieWord num_empty_rows = 0;

	if (initials && initials != this)
	{
		capital=1;
		enablecap=true;
		initials_rows = 1 + ((initials->maxHeight - 1) / maxHeight); // ceiling
		currCap = tmp[0];
		if ((startrow > 0 && initials_rows > 0) || (len > 0 && isspace(currCap))) { // we need to look back to get the cap
			while(isspace(currCap) && num_empty_rows < len){//we cant cap whiteSpace so keep looking
				currCap = tmp[++num_empty_rows];
				// WARNING: this assumes all preceeding whiteSpace is an empty line
			}
			last_initial_row = startrow - 1; // always the row before current since this cannot be the first row
			initials_rows = initials_rows - (startrow + 1) + num_empty_rows; // startrow + 1 because start row is 0 based, but initials_rows is 1 based
		}
	}

	unsigned int psx = IE_FONT_PADDING;
	Palette *pal = hicolor;
	if (!pal) {
		pal = palette;
	}

	Holder<Palette> blitPalette = pal;

	SetupString( tmp, rgn.w, NoColor, initials, enablecap );

	if (startrow) enablecap=false;
	int ystep;
	if (Alignment & IE_FONT_SINGLE_LINE) {
		ystep = CalcStringHeight(tmp, len, NoColor);
		if (!ystep) ystep = maxHeight;
	} else {
		ystep = maxHeight;
	}
	int x = psx, y = ystep;
	size_t w = CalcStringWidth( tmp, NoColor );
	if (Alignment & IE_FONT_ALIGN_CENTER) {
		x = ( rgn.w - w) / 2;
	} else if (Alignment & IE_FONT_ALIGN_RIGHT) {
		x = ( rgn.w - w ) - IE_FONT_PADDING;
	}
	if (Alignment & IE_FONT_ALIGN_MIDDLE) {
		int h = 0;
		for (size_t i = 0; i <= len; i++) {
			if (( tmp[i] == 0 ) || ( tmp[i] == '\n' ))
				h++;
		}
		h = h * ystep;
		y += ( rgn.h - h ) / 2;
	} else if (Alignment & IE_FONT_ALIGN_BOTTOM) {
		int h = 0;
		for (size_t i = 0; i <= len; i++) {
			if (( tmp[i] == 0 ) || ( tmp[i] == '\n' ))
				h++;
		}
		h = h * ystep;
		y += ( rgn.h - h ) - IE_FONT_PADDING;
	} else if (Alignment & IE_FONT_ALIGN_TOP) {
		y += IE_FONT_PADDING;
	}

	Video* video = core->GetVideoDriver();
	const Sprite2D* currGlyph;
	ieWord currChar = '\0';
	int row = 0;
	for (size_t i = 0; i < len; i++) {
		if (( tmp[i] ) == '[' && !NoColor) {
			i++;
			char tag[256];
			tag[0]=0;

			for (size_t k = 0; k < 256 && i<len; k++) {
				if (tmp[i] == ']') {
					tag[k] = 0;
					break;
				}
				tag[k] = tmp[i++];
			}

			if (strnicmp( tag, "capital=",8)==0) {
				sscanf( tag, "capital=%d", &capital);
				if (capital && (row>=startrow) ) {
					enablecap=true;
				}
				continue;
			}

			if (strnicmp( tag, "color=", 6 ) == 0) {
				unsigned int r,g,b;
				if (sscanf( tag, "color=%02X%02X%02X", &r, &g, &b ) != 3)
					continue;
				const Color c = {(unsigned char)r, (unsigned char)g, (unsigned char)b, 0};
				Palette* newPal = core->CreatePalette( c, palette->back );
				blitPalette = newPal;
				gamedata->FreePalette( newPal );
				continue;
			}
			if (stricmp( tag, "/color" ) == 0) {
				blitPalette = pal;
				continue;
			}
			if (stricmp( "p", tag ) == 0) {
				psx = x;
				continue;
			}
			if (stricmp( "/p", tag ) == 0) {
				psx = IE_FONT_PADDING;
			}
			continue;
		}

		if (row < startrow) {
			if (tmp[i] == 0) {
				row++;
			}
			continue;
		}
		if (( tmp[i] == 0 ) || ( tmp[i] == '\n' )) {
			y += ystep;
			x = psx;
			size_t w = CalcStringWidth( &tmp[i + 1], NoColor );
			if (initials_rows > 0) {
				initials_rows--;
				x += initials_x;
				w += initials_x;
			}
			if (Alignment & IE_FONT_ALIGN_CENTER) {
				x = ( rgn.w - w ) / 2;
			} else if (Alignment & IE_FONT_ALIGN_RIGHT) {
				x = ( rgn.w - w );
			}
			continue;
		}
		currChar = tmp[i];
		if (initials && capital && enablecap) {
			currCap = currChar;
			x = initials->PrintInitial( x, y, rgn, currChar );
			initials_x = x;

			//how many more lines to be indented (one was already indented)
			initials_rows = (initials->maxHeight - 1) / maxHeight;
			initials_rows += num_empty_rows;
			initials_row = row;
			last_initial_row = initials_row;

			enablecap = false;
			continue;
		} else if (initials && currCap
				   && row > last_initial_row
				   && (row - num_empty_rows - initials_row) <= ((initials->maxHeight-1)/maxHeight)){
			// means this row doesnt have a cap, but a preceeding one did and its overlapping this row
			int initY = y;
			if (!num_empty_rows || row > num_empty_rows) {// num_empty_rows is for scrolling text areas
				initY = (y - (maxHeight * (row - initials_row - num_empty_rows)));
			}
			x = initials->PrintInitial( x, initY, rgn, currCap );
			initials_x = x;
			last_initial_row++;
			if (num_empty_rows && row <= num_empty_rows) continue;
			else x += psx;
		}
		if (i > 0) {
			// kerning
			x -= GetKerningOffset(tmp[i-1], currChar);
		}
		currGlyph = GetCharSprite(currChar);
		video->BlitSprite(currGlyph, x + rgn.x, y + rgn.y, true, &rgn, blitPalette.get());
		if (cursor && ( i == curpos )) {
			video->BlitSprite( cursor, x + rgn.x, y + rgn.y, true, &rgn );
		}
		x += currGlyph->Width;
	}
	if (cursor && ( curpos == len )) {
		video->BlitSprite( cursor, x + rgn.x, y + rgn.y, true, &rgn );
	}
	blitPalette = NULL;
	free( tmp );
}

void Font::Print(Region rgn, const unsigned char* string, Palette* hicolor,
	ieByte Alignment, bool anchor, Font* initials,
	Sprite2D* cursor, unsigned int curpos, bool NoColor) const
{
	Region cliprgn = rgn;
	if (!anchor) {
		Region Viewport = core->GetVideoDriver()->GetViewport();
		cliprgn.x -= Viewport.x;
		cliprgn.y -= Viewport.y;
	}
	Print(cliprgn, rgn, string, hicolor, Alignment, anchor, initials, cursor, curpos, NoColor);
}

void Font::Print(Region cliprgn, Region rgn, const unsigned char* string,
	Palette* hicolor, ieByte Alignment, bool anchor, Font* initials,
	Sprite2D* cursor, unsigned int curpos, bool NoColor) const
{
	int capital = (initials) ? 1 : 0;

	unsigned int psx = IE_FONT_PADDING;
	Palette* pal = hicolor;
	if (!pal) {
		pal = palette;
	}
	if (initials==this) {
		initials = NULL;
	}

	Holder<Palette> blitPalette = pal;
	ieWord* tmp = NULL;
	size_t len = GetDoubleByteString(string, tmp);
	while (len > 0 && (tmp[len - 1] == '\n' || tmp[len - 1] == '\r')) {
		// ignore trailing newlines
		tmp[len - 1] = 0;
		len--;
	}

	SetupString( tmp, rgn.w, NoColor, initials, capital );
	int ystep;
	if (Alignment & IE_FONT_SINGLE_LINE) {
		ystep = CalcStringHeight(tmp, len, NoColor);
		if (!ystep) ystep = maxHeight;
	} else {
		ystep = maxHeight;
	}
	int x = psx, y = ystep;
	Video* video = core->GetVideoDriver();

	if (Alignment & IE_FONT_ALIGN_CENTER) {
		size_t w = CalcStringWidth( tmp, NoColor );
		x = ( rgn.w - w ) / 2;
	} else if (Alignment & IE_FONT_ALIGN_RIGHT) {
		size_t w = CalcStringWidth( tmp, NoColor );
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
		if (( tmp[i] ) == '[' && !NoColor) {
			i++;
			char tag[256];
			tag[0]=0;
			for (size_t k = 0; k < 256 && i<len; k++) {
				if (tmp[i] == ']') {
					tag[k] = 0;
					break;
				}
				tag[k] = tmp[i++];
			}

			if (strnicmp( tag, "capital=",8)==0) {
				sscanf( tag, "capital=%d", &capital);
				continue;
			}

			if (strnicmp( tag, "color=", 6 ) == 0) {
				unsigned int r,g,b;
				if (sscanf( tag, "color=%02X%02X%02X", &r, &g, &b ) != 3)
					continue;
				const Color c = {(unsigned char)r, (unsigned char)g, (unsigned char)b, 0};
				Palette* newPal = core->CreatePalette( c, palette->back );
				blitPalette = newPal;
				gamedata->FreePalette( newPal );
				continue;
			}
			if (stricmp( tag, "/color" ) == 0) {
				blitPalette = pal;
				continue;
			}
			if (stricmp( "p", tag ) == 0) {
				psx = x;
				continue;
			}
			if (stricmp( "/p", tag ) == 0) {
				psx = IE_FONT_PADDING;
				continue;
			}
			continue;
		}

		if (tmp[i] == 0) {
			y += ystep;
			x = psx;
			size_t w = CalcStringWidth( &tmp[i + 1], NoColor );
			if (Alignment & IE_FONT_ALIGN_CENTER) {
				x = ( rgn.w - w ) / 2;
			} else if (Alignment & IE_FONT_ALIGN_RIGHT) {
				x = ( rgn.w - w );
			}
			continue;
		}
		currChar = tmp[i];
		currGlyph = GetCharSprite(currChar);
		if (initials && capital) {
			x = initials->PrintInitial( x, y, rgn, currChar );
			continue;
		}

		if (i > 0) {
			// kerning
			x -= GetKerningOffset(tmp[i-1], currChar);
		}

		video->BlitSprite(currGlyph, x + rgn.x, y + rgn.y, anchor, &cliprgn, blitPalette.get());

		if (cursor && ( curpos == i ))
			video->BlitSprite( cursor, x + rgn.x, y + rgn.y, anchor, &cliprgn );
		x += currGlyph->Width;
	}
	if (cursor && ( curpos == len )) {
		video->BlitSprite( cursor, x + rgn.x, y + rgn.y, anchor, &cliprgn );
	}
	blitPalette = NULL;
	free( tmp );
}

int Font::PrintInitial(int x, int y, const Region &rgn, ieWord currChar) const
{
	const Sprite2D* glyph = GetCharSprite(currChar);
	core->GetVideoDriver()->BlitSprite(glyph, x + rgn.x, y + rgn.y, true, &rgn);

	x += glyph->Width;
	return x;
}

size_t Font::CalcStringWidth(const unsigned char* string, bool NoColor) const
{
	ieWord* tmp = NULL;
	GetDoubleByteString(string, tmp);
	int width = CalcStringWidth(tmp, NoColor);
	free(tmp);
	return width;
}

size_t Font::CalcStringWidth(const ieWord* string, bool NoColor) const
{
	size_t ret = 0, len = dbStrLen(string);
	for (size_t i = 0; i < len; i++) {
		if (( string[i] ) == '[' && !NoColor) {
			i++; // cannot be ']' when it is '['
			while(i<len && (string[i]) != ']') {
				i++;
			}
		} else {
			ret += GetCharSprite(string[i])->Width;
		}
	}
	return ret;
}

size_t Font::CalcStringHeight(const ieWord* string, unsigned int len, bool NoColor) const
{
	int h, max = 0;
	for (unsigned int i = 0; i < len; i++) {
		if (( string[i] ) == '[' && !NoColor) {
			i++; // cannot be ']' when it is '['
			while(i<len && (string[i]) != ']') {
				i++;
			}
		} else {
			h = GetCharSprite(string[i])->Height;
			//the space check is here to hack around overly high frames
			//in some bg1 fonts that throw vertical alignment off
			if (h > max && string[i] != ' ') {
				max = h;
			}
		}
	}
	return max;
}

void Font::SetupString(ieWord* string, unsigned int width, bool NoColor, Font *initials, bool enablecap) const
{
	size_t len = dbStrLen(string);
	unsigned int psx = IE_FONT_PADDING;
	size_t lastpos = 0;
	unsigned int x = psx, wx = 0;
	bool endword = false;
	int initials_rows = 0;
	int initials_x = 0;
	for (size_t pos = 0; pos < len; pos++) {
		if (x + wx > width) {
			// we wrapped, force a new line somewhere
			if (!endword && ( x == psx ))
				lastpos = pos;
			else
				string[lastpos] = 0;
			x = psx;
			if (initials_rows > 0) {
				initials_rows--;
				x += initials_x;
			}
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
			if (initials_rows > 0) {
				initials_rows--;
				x += initials_x;
			}
			lastpos = pos;
			endword = true;
			continue;
		}
		if (( string[pos] ) == '[' && !NoColor) {
			pos++;
			if (pos>=len)
				break;
			char tag[256];
			int k = 0;
			for (k = 0; k < 256; k++) {
				if (string[pos] == ']') {
					tag[k] = 0;
					break;
				}
				tag[k] = string[pos++];
			}
			if (strnicmp( tag, "capital=",8)==0) {
				int capital = 0;
				sscanf( tag, "capital=%d", &capital);
				if (capital) {
					enablecap=true;
				}
				continue;
			}
			if (stricmp( "p", tag ) == 0) {
				psx = x;
				continue;
			}
			if (stricmp( "/p", tag ) == 0) {
				psx = IE_FONT_PADDING;
				continue;
			}
			continue;
		}

		if (initials && enablecap) {
			wx += initials->GetCharSprite(string[pos])->Width;
			enablecap=false;
			initials_x = wx + psx;
			//how many more lines to be indented (one was already indented)
			initials_rows = (initials->maxHeight - 1) / maxHeight;
			continue;
		} else {
			wx += GetCharSprite(string[pos])->Width;
		}
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
	size_t len = strlen(reinterpret_cast<const char*>(string));
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
