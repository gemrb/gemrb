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
#include "Video.h"

#include <sstream>

#define DEBUG_FONT 0

namespace GemRB {

static void BlitGlyphToCanvas(const Glyph& glyph, int x, int y,
							 ieByte* canvas, const Size& size)
{
	assert(canvas);

	// TODO: should handle partial glyphs
	if (!Region(0, 0, size.w, size.h).PointInside(x, y)) {
		return; // off the canvas
	}

	// copy the glyph to the canvas
	const ieByte* src = glyph.pixels;
	ieByte* dest = canvas + (size.w * y) + x;
	for(int row = 0; row < glyph.dimensions.h; row++ ) {
		//assert(dest <= canvas + (size.w * size.h));
		if (dest + glyph.dimensions.w > canvas + (size.w * size.h)) {
			break;
		}
		memcpy(dest, src, glyph.dimensions.w);
		dest += size.w;
		src += glyph.pitch;
	}
}


bool Font::GlyphAtlasPage::AddGlyph(ieWord chr, const Glyph& g)
{
	assert(glyphs.find(chr) == glyphs.end());
	if (pageXPos + g.dimensions.w > SheetRegion.w) {
		return false;
	}
	// if we already have a sheet we need to destroy it before we can add more glyphs
	if (Sheet) {
		Sheet->release();
		Sheet = NULL;
	}

	BlitGlyphToCanvas(g, pageXPos, 0, pageData, Size(SheetRegion.w, SheetRegion.h));
	MapSheetSegment(chr, Region(pageXPos, 0, g.dimensions.w, g.dimensions.h));
	// make the non-temporary glyph from our own data
	ieByte* pageLoc = pageData + pageXPos;
	glyphs.insert(std::make_pair(chr, Glyph(g.dimensions, g.descent, pageLoc, SheetRegion.w)));

	pageXPos += g.dimensions.w;
	return true;
}

const Glyph& Font::GlyphAtlasPage::GlyphForChr(ieWord chr) const
{
	if (glyphs.find(chr) != glyphs.end()) {
		return glyphs.at(chr);
	}
	static Glyph blank(Size(0,0), 0, NULL, 0);
	return blank;
}

void Font::GlyphAtlasPage::Draw(ieWord chr, const Region& dest, Palette* pal)
{
	if (!pal) {
		pal = font->GetPalette();
		pal->release();
	}

	// ensure that we have a sprite!
	if (Sheet == NULL) {
		void* pixels = pageData;
		// TODO: implement a video driver check to see if the data can be shared
		if (false) {
			// pixels are *not* shared
			// TODO: allocate a new pixel buffer and copy the pixels in
			// pixels = malloc(size);
			// memcpy(pixels, GlyphPageData, size);
		}
		Sheet = core->GetVideoDriver()->CreateSprite8(SheetRegion.w, SheetRegion.h, pixels, pal, true, 0);
	}
	Palette* oldPal = Sheet->GetPalette();
	Sheet->SetPalette(pal);
	SpriteSheet::Draw(chr, dest);
	Sheet->SetPalette(oldPal);
	oldPal->release();
}


Font::Font(Palette* pal)
: palette(NULL), maxHeight(0)
{
	CurrentAtlasPage = NULL;
	SetPalette(pal);
}

Font::~Font(void)
{
	GlyphAtlas::iterator it;
	for (it = Atlas.begin(); it != Atlas.end(); ++it) {
		delete *it;
	}

	SetPalette(NULL);
}

const Glyph& Font::CreateGlyphForCharSprite(ieWord chr, const Sprite2D* spr)
{
	assert(AtlasIndex.find(chr) == AtlasIndex.end());
	// if this character is already an alias then we have a problem...
	assert(AliasMap.find(chr) == AliasMap.end());
	assert(spr);
	
	Size size(spr->Width, spr->Height);
	int des = maxHeight - spr->YPos;
	Glyph tmp = Glyph(size, des, (ieByte*)spr->pixels, spr->Width);
	// FIXME: should we adjust for spr->XPos too?
	// adjust the location for the glyph
	if (!CurrentAtlasPage || !CurrentAtlasPage->AddGlyph(chr, tmp)) {
		// page is full, make a new one
		CurrentAtlasPage = new GlyphAtlasPage(Size(1024, maxHeight + descent), this);
		Atlas.push_back(CurrentAtlasPage);
		CurrentAtlasPage->AddGlyph(chr, tmp);
	}
	assert(CurrentAtlasPage);
	AtlasIndex[chr] = Atlas.size() - 1;

	return CurrentAtlasPage->GlyphForChr(chr);
}

void Font::CreateAliasForChar(ieWord chr, ieWord alias)
{
	assert(AliasMap.find(alias) == AliasMap.end());
	// we cannot create an alias of an alias...
	assert(AliasMap.find(chr) == AliasMap.end());
	// we cannot create an alias for a chaaracter that doesnt exist
	assert(AtlasIndex.find(chr) != AtlasIndex.end());

	AliasMap[alias] = chr;

	// we need to now find the page for the existing character and add this new one to that page
	size_t pageIdx = AtlasIndex.at(chr);
	AtlasIndex[alias] = pageIdx;
	Atlas[pageIdx]->MapSheetSegment(alias, (*Atlas[pageIdx])[chr]);
}

const Glyph& Font::GetGlyph(ieWord chr) const
{
	// Aliases are 2 glyphs that share identical BAM frames such as 'ā' and 'a'
	// we can save a few bytes of memory this way :)
	if (AliasMap.find(chr) != AliasMap.end()) {
		chr = AliasMap.at(chr);
	}
	size_t idx = 0;
	if (AtlasIndex.find(chr) != AtlasIndex.end()) idx = AtlasIndex.at(chr);
	return Atlas[idx]->GlyphForChr(chr);
}

size_t Font::RenderText(const String& string, Region& rgn,
						Palette* color, ieByte alignment,
						Point* point, ieByte** canvas, bool grow) const
{
	// NOTE: vertical alignment is not handled here.
	// it should have been calculated previously and passed in via the "point" parameter

	bool singleLine = (alignment&IE_FONT_SINGLE_LINE);
	Point dp((point) ? point->x : 0, (point) ? point->y : 0);

	size_t charCount = 0;
	// is this horribly inefficient?
	std::wistringstream stream(string);
	bool lineBreak = false;

	String line;
	while (lineBreak || getline(stream, line)) {
		lineBreak = false;

		// check if we need to extend the canvas
		if (canvas && grow && rgn.h < dp.y) {
			size_t pos = stream.tellg();
			pos -= line.length();
			Size textSize = StringSize(string.substr(pos));
			ieWord numNewPixels = textSize.Area();
			ieWord lineArea = rgn.w * maxHeight;
			// round up
			ieWord numLines = 1 + ((numNewPixels - 1) / lineArea);
			// extend the region and canvas both
			size_t curpos = rgn.h * rgn.w;
			int vGrow = (numLines * maxHeight) + descent;
			rgn.h += vGrow;

#if DEBUG_FONT
			Log(MESSAGE, "Font", "Resizing canvas from %dx%d to %dx%d",
				rgn.w, rgn.h - vGrow, rgn.w, rgn.h);
#endif

			*canvas = (ieByte*)realloc(*canvas, rgn.w * rgn.h);
			assert(canvas);
			// fill the buffer with the color key, or the new area or we will get garbage in the areas we dont blit to
			memset(*canvas + curpos, 0, vGrow * rgn.w);
		}

		dp.x = 0;
		const Region lineRgn(dp + rgn.Origin(), Size(rgn.w, maxHeight + descent));
		const Size s = lineRgn.Dimensions();
		ieWord lineW = StringSize(line, &s).w;

		size_t lineLen = line.length();
		size_t linePos = line.find_first_not_of(L' ');
		if (lineLen && linePos != String::npos) {
			// skip spaces at the beginning of a line
			// FIXME: under what conditions does this not apply? single line? what about console?
			line.erase(0, linePos);
			charCount += linePos;

			Point linePoint;
			if (alignment&(IE_FONT_ALIGN_CENTER|IE_FONT_ALIGN_RIGHT)) {
				linePoint.x += (rgn.w - lineW); // this is right aligned, but we can adjust for center later on
				if (linePoint.x < 0) {
					linePos = String::npos;
					size_t prevPos = linePos;
					String word;
					while (linePoint.x < 0) {
						// yuck, this is not optimal. not sure of a better way.
						// we have to rewind, word by word, until X >= 0
						linePos = line.find_last_of(L' ', prevPos);
						// word should be the space + word for calculation purposes
						word = line.substr(linePos, (prevPos - linePos) + 1);
						linePoint.x += StringSize(word).w;
						prevPos = linePos - 1;
					}
				}
				if (alignment&IE_FONT_ALIGN_CENTER) {
					linePoint.x /= 2;
				}
			}
#if DEBUG_FONT
			core->GetVideoDriver()->DrawRect(lineRgn, ColorRed, false);
			core->GetVideoDriver()->DrawRect(Region(linePoint + lineRgn.Origin(), Size(lineW, maxHeight)), ColorWhite, false);
#endif
			linePos = RenderLine(line, lineRgn, color, linePoint, (canvas) ? canvas + (dp.x * dp.y) : NULL);
			dp = dp + linePoint;
			charCount += linePos;
			if (linePos < line.length() - 1) {
				lineBreak = true;
				if (!singleLine) {
					// dont bother getting the next line if we arent going to print it
					line = line.substr(linePos);
				}
			}
		} else if (linePos == String::npos) {
			// string is only spaces... just skip it
			charCount += line.length();
		}
		if (!lineBreak && !stream.eof())
			charCount++; // for the newline
		dp.y += maxHeight;

		if (singleLine || dp.y >= rgn.h) {
			break;
		}
	}

	// free the unused canvas area (if any)
	if (canvas) {
		int usedh = dp.y + descent;
		if (usedh < rgn.h) {
			// this is more than just saving memory
			// vertical alignment will be off if we have extra space
			*canvas = (ieByte*)realloc(*canvas, rgn.w * usedh);
			rgn.h = usedh;
		}
	}

	if (point) {
		// deal with possible trailing newline
		if (string[charCount - 1] == L'\n') {
			dp.y += maxHeight;
		}
		*point = Point(dp.x, dp.y - maxHeight);
	}

	assert(charCount <= string.length());
	return charCount;
}

size_t Font::RenderLine(const String& line, const Region& lineRgn, Palette* color,
						Point& dp, ieByte** canvas) const
{
	assert(color); // must have a palette

	// NOTE: alignment is not handled here.
	// it should have been calculated previously and passed in via the "point" parameter

	size_t linePos = 0, wordBreak = 0;

	// FIXME: I'm not sure how to handle Asian text
	// should a "word" be a single Asian glyph? that way we wouldnt clip off text (we were doing this before the rewrite too).
	// we could check the core encoding for the 'zerospace' attribute and treat single characters as words
	// that would looks funny with partial translations, however. we would need to handle both simultaniously.
	// TODO: word breaks shouldprobably happen on other characters such as '-' too.
	// not as simple as adding it to find_first_of
	int spaceW = GetGlyph(L' ').dimensions.w;
	bool done = false;
	while ((wordBreak = line.find_first_of(L' ', linePos))) {
		String word = line.substr(linePos, wordBreak - linePos);

		int wordW = StringSize(word).w;
		if (dp.x + wordW > lineRgn.w && dp.x > 0) {
			break;
		}

		// print the word
		wchar_t currChar = '\0';
		size_t i = 0;
		for (; i < word.length(); i++) {
			// process glyphs in word
			currChar = word[i];
			if (currChar == '\r') {
				continue;
			}
			if (i > 0) { // kerning
				dp.x -= KerningOffset(word[i-1], currChar);
			}

			const Glyph& curGlyph = GetGlyph(currChar);
			// should probably use rect intersection, but new lines shouldnt be to the left ever.
			if (!lineRgn.PointInside(dp.x + lineRgn.x, dp.y + lineRgn.y + curGlyph.descent)) {
				if (wordW < lineRgn.w) {
					// this probably doest cover every situation 100%
					// we consider printing done if the blitter is outside the region
					// *and* the word isnt wider then the line
					done = true;
				} else {
#if DEBUG_FONT
					Log(WARNING, "Font", "The word '%ls' (width=%d) overruns available width of %d",
						word.c_str(), wordW, lineRgn.w);
#endif
				}
				break; // either done, or skipping
			}
			if (canvas) {
				BlitGlyphToCanvas(curGlyph, dp.x, dp.y + curGlyph.descent, *canvas, lineRgn.Dimensions());
			} else {
				assert(AtlasIndex.find(currChar) != AtlasIndex.end());

				size_t pageIdx = AtlasIndex.at(currChar);
				assert(pageIdx < AtlasIndex.size());

				GlyphAtlasPage* page = Atlas[pageIdx];
				Region dst = Region(dp.x + lineRgn.x, dp.y + lineRgn.y + curGlyph.descent,
									curGlyph.dimensions.w, curGlyph.dimensions.h);
				page->Draw(currChar, dst, color);
			}
			dp.x += curGlyph.dimensions.w;
		}
		if (done) break;
		linePos += i + 1;

		if (wordBreak == String::npos) {
			linePos--; // we previously counted a non-existant space
			break;
		}
		dp.x += spaceW;
	}
	assert(linePos <= line.length());
	return linePos;
}

Sprite2D* Font::RenderTextAsSprite(const String& string, const Size& size,
								   ieByte alignment, Palette* color, size_t* numPrinted, Point* endPoint) const
{
	Size canvasSize = StringSize(string); // same as size(0, 0)
	// if the string is larger than the region shrink the canvas
	// except 0 means we should size to fit in that dimension
	if (size.w) {
		// potentially resize
		if (size.w < canvasSize.w) {
			if (!(alignment&IE_FONT_SINGLE_LINE)) {
				// we need to resize horizontally which creates new lines
				ieWord trimmedArea = ((canvasSize.w - size.w) * canvasSize.h);
				// this automatically becomes multiline, therefore use maxHeight
				ieWord lineArea = size.w * maxHeight;
				// round up
				ieWord numLines = 1 + ((trimmedArea - 1) / lineArea);
				if (!size.h) {
					// grow as much as needed vertically.
					canvasSize.h += (numLines * maxHeight) + descent;
					// there is a chance we didn't grow enough vertically...
					// we can't possibly know how lines will break ahead of time,
					// over a long enough paragraph we can overflow the canvas
					// this is handled in RenderText() by reallocing the canvas based on
					// the same estimation algorithim (total area of text) used here
				} else if (size.h > canvasSize.h) {
					// grow by line increments until we hit the limit
					// round up, because even a partial line should be blitted (and clipped)
					ieWord maxLines = 1 + (((size.h - canvasSize.h) - 1) / maxHeight);
					if (maxLines < numLines) {
						numLines = maxLines;
					}
					canvasSize.h += (numLines * maxHeight) + descent;
					// if the new canvas size is taller than size.h it will be dealt with later
				}
			}
			canvasSize.w = size.w;
		} else if (alignment&(IE_FONT_ALIGN_CENTER|IE_FONT_ALIGN_RIGHT)) {
			// the size width is how we center or right align so we cant trim the canvas
			canvasSize.w = size.w;
		}
		// else: we already fit in the designated area (horizontally). No need to resize.
	}
	if (!(alignment&IE_FONT_SINGLE_LINE)) {
		if (canvasSize.h < maxHeight) {
			// should be at least maxHeight (+ decender added later then trimmed if too large for size)
			canvasSize.h = maxHeight;
		}
		canvasSize.h += descent; // compensate for last line descenders
	}
	if (size.h && size.h < canvasSize.h) {
		// we can't unbreak lines ("\n") so at best we can clip the text.
		canvasSize.h = size.h;
	}
	assert(size.h || canvasSize.h >= maxHeight + descent);

	// we must calloc because not all glyphs are equal height. set remainder to the color key
	ieByte* canvasPx = (ieByte*)calloc(canvasSize.w, canvasSize.h);

	Region rgn = Region(Point(0,0), canvasSize);
	size_t ret = RenderText(string, rgn, palette, alignment, endPoint, &canvasPx, (size.h) ? false : true);
	if (numPrinted) {
		*numPrinted = ret;
	}
	Palette* pal = color;
	if (!pal)
		pal = palette;
	// must ue rgn! the canvas height might be changed in RenderText()
	Sprite2D* canvas = core->GetVideoDriver()->CreateSprite8(rgn.w, rgn.h,
															 canvasPx, pal, true, 0);
	if (alignment&IE_FONT_ALIGN_CENTER) {
		canvas->XPos = (size.w - rgn.w) / 2;
	} else if (alignment&IE_FONT_ALIGN_RIGHT) {
		canvas->XPos = size.w - rgn.w;
	}
	if (alignment&IE_FONT_ALIGN_MIDDLE) {
		canvas->YPos = -(size.h - rgn.h) / 2;
	} else if (alignment&IE_FONT_ALIGN_BOTTOM) {
		canvas->YPos = -(size.h - rgn.h);
	}
	return canvas;
}

size_t Font::Print(Region rgn, const char* string,
				   Palette* hicolor, ieByte Alignment) const
{
	String* tmp = StringFromCString(string);
	size_t ret = Print(rgn, *tmp, hicolor, Alignment);
	delete tmp;
	return ret;
}

size_t Font::Print(Region rgn, const String& string,
				   Palette* color, ieByte alignment, Point* point) const
{
	if (rgn.Dimensions().IsEmpty()) return 0;

	Palette* pal = color;
	if (!pal) {
		pal = palette;
	}
	Point p = (point) ? *point : Point();
	if (alignment&(IE_FONT_ALIGN_MIDDLE|IE_FONT_ALIGN_BOTTOM)) {
		// we assume that point will be an offset from midde/bottom position
		Size stringSize;
		if (alignment&IE_FONT_SINGLE_LINE) {
			// we can optimize single lines without StringSize()
			stringSize.h = maxHeight;
			if (alignment&IE_FONT_ALIGN_MIDDLE) {
				// FIXME: should this be something that IE_FONT_ALIGN_BOTTOM subtracts instead (for more than IE_FONT_SINGLE_LINE)?
				stringSize.h += descent;
			}
		} else {
			stringSize = rgn.Dimensions();
			stringSize = StringSize(string, &stringSize);
		}
		if (alignment&IE_FONT_ALIGN_MIDDLE) {
			p.y += (rgn.h - stringSize.h) / 2;
		} else { // bottom alignment
			p.y += rgn.h - stringSize.h;
		}
	} else if (alignment&IE_FONT_SINGLE_LINE) {
		// FIXME: I dont remember why this is needed (it is tho... for now)
		p.y -= descent;
	}

	size_t ret = RenderText(string, rgn, pal, alignment, &p);
	if (point) {
		*point = p;
	}
	return ret;
}

Size Font::StringSize(const String& string, const Size* stop, size_t* numChars) const
{
	if (!string.length()) return Size();

	ieWord w = 0, lines = 1;
	ieWord lineW = 0, wordW = 0;
	int spaceW = GetGlyph(L' ').dimensions.w;
	bool newline = false, eos = false;
	size_t i = 0, wordCharCount = 0;
	for (; i < string.length(); i++) {
		if (i == string.length() - 1) {
			eos = true;
		}
		if (string[i] == L'\n') {
			newline = true;
		} else if (string[i] != L' ') {
			const Glyph& curGlyph = GetGlyph(string[i]);
			wordW += curGlyph.dimensions.w;
			if (i > 0) { // kerning
				wordW -= KerningOffset(string[i-1], string[i]);
			}
			wordCharCount++;
		}
		if (string[i] == L' ' || eos) {
			if (stop && stop->w
				&& lineW // let this overflow if the word alone is wider than stop->w
				&& lineW + spaceW + wordW > stop->w) {
				newline = true;
			} else {
				if (lineW) {
					lineW += spaceW;
				}
				lineW += wordW;
			}

			if (newline || eos) {
				w = (lineW > w) ? lineW : w;
			} else {
				wordCharCount = 0;
				wordW = 0;
			}
		}

		if (newline) {
			newline = false;
			if (stop && stop->h && (maxHeight * lines) + descent > stop->h ) {
				// adjust i for numChars
				if (wordCharCount) {
					i -= wordCharCount + 1; // +1 for the space
				}
				break;
			}
			lines++;
			lineW = 0;
		}
	}
	if (numChars) {
		*numChars = i;
	}

	// this can only return w > stop->w if there is a singe word wider than stop.
	return Size(w, (maxHeight * lines) + descent);
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
