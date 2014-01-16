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

#include <sstream>

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

Sprite2D* Font::RenderText(const String& string, const Region rgn, size_t* numPrinted) const
{
	int sW = CalcStringWidth(string);
	int w = 0, h = 0;
	if (sW <= rgn.w) {
		// single line of text
		w = sW;
		h = CalcStringHeight(string);
	} else {
		// text will be multiple lines
		w = rgn.w;
		h = maxHeight * (sW / rgn.w);
	}

	ieDword ck = GetCharSprite(string[0])->GetColorKey();
	// we must calloc/memset because not all glyphs are equal height. set remainder to the color key
	ieByte* canvasPx = (ieByte*)calloc(w, h);
	ieByte* dest = canvasPx;
	if (ck != 0) {
		// start with transparent canvas
		memset(canvasPx, ck, w * h);
	}

	const Sprite2D* curGlyph = NULL;
	size_t len = string.length();
	size_t chrIdx = 0;
	int wCurrent = 0, hCurrent = 0;
	bool newline = false;
	for (; chrIdx < len; chrIdx++) {
		curGlyph = GetCharSprite(string[chrIdx]);
		assert(!curGlyph->BAM);
		if (newline) {
			hCurrent += maxHeight;
			if ((hCurrent + curGlyph->Height) > h) {
				// the next line would be clipped. we are done.
				break;
			}
		}

		// copy the glyph to the canvas
		ieByte* src = (ieByte*)curGlyph->pixels;
		dest = canvasPx + wCurrent;
		for( int row = 0; row < curGlyph->Height; row++ ) {
			memcpy(dest, src, curGlyph->Width);
			dest += w;
			src += curGlyph->Width;
		}

		wCurrent += curGlyph->Width;
		if (wCurrent >= w) {
			// we've run off the edge of the region
			newline = true;
		}
	}
	// should have filled the canvas 100% <- NO! we probably have extra space on the last line
	//assert((dest - canvasPx) == (w * h));

	if (numPrinted) {
		*numPrinted = chrIdx;
	}
	Sprite2D* canvas = core->GetVideoDriver()->CreateSprite8(w, h, canvasPx,
															 palette, true,
															 curGlyph->GetColorKey());
	// TODO: adjust canvas position based on alignment flags and rgn
	return canvas;
}

size_t Font::Print(Region rgn, const char* string, Palette* hicolor,
	ieByte Alignment, bool anchor) const
{
	Region cliprgn = rgn;
	if (!anchor) {
		Region Viewport = core->GetVideoDriver()->GetViewport();
		cliprgn.x -= Viewport.x;
		cliprgn.y -= Viewport.y;
	}
	return Print(cliprgn, rgn, string, hicolor, Alignment, anchor);
}

size_t Font::Print(Region cliprgn, Region rgn, const char* string,
	Palette* hicolor, ieByte Alignment, bool anchor) const
{
	String* tmp = StringFromCString(string);
	size_t ret = Print(cliprgn, rgn, *tmp, hicolor, Alignment, anchor);
	delete tmp;
	return ret;
}

size_t Font::Print(Region rgn, const String& string, Palette* hicolor,
				   ieByte Alignment, bool anchor) const
{
	Region cliprgn = rgn;
	if (!anchor) {
		Region Viewport = core->GetVideoDriver()->GetViewport();
		cliprgn.x -= Viewport.x;
		cliprgn.y -= Viewport.y;
	}
	return Print(cliprgn, rgn, string, hicolor, Alignment, anchor);
}

size_t Font::Print(Region cliprgn, Region rgn, const String& string, Palette* color,
				   ieByte Alignment, bool anchor) const
{
	unsigned int psx = IE_FONT_PADDING;
	Palette* pal = color;
	if (!pal) {
		pal = palette;
	}

	int ystep = maxHeight;
	int x = psx, y = ystep;
	Video* video = core->GetVideoDriver();

	if (Alignment & (IE_FONT_ALIGN_MIDDLE|IE_FONT_ALIGN_BOTTOM)) {
		int h = 0;
		for (size_t i = 0; i <= string.length(); i++) {
			if (string[i] == L'\n')
				h++;
		}
		h = h * ystep;
		if (Alignment & IE_FONT_ALIGN_MIDDLE) {
			y += ( rgn.h - h ) / 2;
		} else {
			y += ( rgn.h - h ) - IE_FONT_PADDING;
		}
	} else if (Alignment & IE_FONT_ALIGN_TOP) {
		y += IE_FONT_PADDING;
	}

	// is this horribly inefficient?
	std::wistringstream stream(string);
	String line, word;
	const Sprite2D* currGlyph = NULL;
	bool done = false, lineBreak = false;
	size_t charCount = 0;

	while (!done && (lineBreak || getline(stream, line))) {
		lineBreak = false;

		std::wistringstream lineStream(line);

		size_t lineW = CalcStringWidth(line);
		if (Alignment & IE_FONT_ALIGN_CENTER) {
			x = ( rgn.w - lineW ) / 2;
		} else if (Alignment & IE_FONT_ALIGN_RIGHT) {
			x = ( rgn.w - lineW ) - IE_FONT_PADDING;
		} else {
			x = psx;
		}

		size_t lineLen = line.length();
		if (lineLen) {
			size_t linePos = 0, wordBreak = 0;
			while (!lineBreak && (wordBreak = line.find_first_of(L' ', linePos))) {
				if (wordBreak == String::npos) {
					word = line.substr(linePos);
				} else {
					word = line.substr(linePos, wordBreak - linePos);
				}

				if (!(Alignment&IE_FONT_SINGLE_LINE)) {
					if (word == line) {
						Log(WARNING, "Font", "Printing beyond boundary because a word is wider than %d", rgn.w);
					} else if (x + (int)CalcStringWidth(word) > rgn.w) {
						// wrap to new line
						lineBreak = true;
						line = line.substr(linePos);
					}
				}

				if (!lineBreak) {
					// print the word
					wchar_t currChar = '\0';
					size_t i;
					for (i = 0; i < word.length(); i++) {
						// process glyphs in word
						currChar = word[i];
						if (currChar == '\r') {
							continue;
						}
						if (i > 0) { // kerning
							x -= GetKerningOffset(word[i-1], currChar);
						}
						currGlyph = GetCharSprite(currChar);
						if (!cliprgn.PointInside(x + rgn.x - currGlyph->XPos,
												 y + rgn.y - currGlyph->YPos)) {
							done = true;
							break;
						}
						video->BlitSprite(currGlyph, x + rgn.x, y + rgn.y, anchor, &cliprgn, pal);

						x += currGlyph->Width;
					}
					x += GetCharSprite(' ')->Width;
					linePos += i + 1;
					charCount += i + 1;
				}
				if (wordBreak == String::npos) break;
			}
		}

		if (!lineBreak && !stream.eof())
			charCount++; // for the newline
		y += ystep;
	}
	//assert(charCount <= string.length());
	return charCount;
}

size_t Font::CalcStringWidth(const String string) const
{
	size_t ret = 0, len = string.length();
	for (size_t i = 0; i < len; i++) {
		if (string[i] == L'\n') break;
		ret += GetCharSprite(string[i])->Width;
	}
	return ret;
}

size_t Font::CalcStringHeight(const String string) const
{
	size_t h = 0, max = 0, len = string.length();
	for (size_t i = 0; i < len; i++) {
		h = GetCharSprite(string[i])->Height;
		//the space check is here to hack around overly high frames
		//in some bg1 fonts that throw vertical alignment off
		if (h > max && string[i] != ' ') {
			max = h;
		}
	}
	return max;
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

}
