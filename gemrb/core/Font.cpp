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

namespace GemRB {

void BlitGlyphToCanvas(const Glyph& glyph, int x, int y,
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

void Font::GlyphAtlasPage::Draw(ieWord key, const Region& dest)
{
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
		Sheet = core->GetVideoDriver()->CreateSprite8(SheetRegion.w, SheetRegion.h, pixels, palette, true, 0);
	}
	SpriteSheet::Draw(key, dest);
}


Font::Font(Palette* pal)
: resRefs(NULL), numResRefs(0), palette(NULL), maxHeight(0)
{
	CurrentAtlasPage = NULL;
	name[0] = '\0';
	SetPalette(pal);
}

Font::~Font(void)
{
	GlyphAtlas::iterator it;
	for (it = Atlas.begin(); it != Atlas.end(); ++it) {
		delete *it;
	}

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

const Glyph& Font::CreateGlyphForCharSprite(ieWord chr, const Sprite2D* spr)
{
	assert(AtlasIndex.find(chr) == AtlasIndex.end());
	assert(spr);
	
	Size size(spr->Width, spr->Height);
	int des = maxHeight - spr->YPos;
	Glyph tmp = Glyph(size, des, (ieByte*)spr->pixels, spr->Width);
	// FIXME: should we adjust for spr->XPos too?
	// adjust the location for the glyph
	if (!CurrentAtlasPage || !CurrentAtlasPage->AddGlyph(chr, tmp)) {
		// page is full, make a new one
		CurrentAtlasPage = new GlyphAtlasPage(Size(1024, maxHeight + descent), palette);
		Atlas.push_back(CurrentAtlasPage);
		CurrentAtlasPage->AddGlyph(chr, tmp);
	}
	assert(CurrentAtlasPage);
	AtlasIndex[chr] = Atlas.size() - 1;

	return CurrentAtlasPage->GlyphForChr(chr);
}

const Glyph& Font::GetGlyph(ieWord chr) const
{
	return Atlas[AtlasIndex.at(chr)]->GlyphForChr(chr);
}

size_t Font::RenderText(const String& string, Region& rgn,
						Palette* color, ieByte alignment, ieByte** canvas, bool grow) const
{
	assert(color); // must have a palette

	// we dont do vertical alignment here!
	bool singleLine = (alignment&IE_FONT_SINGLE_LINE);
	assert(canvas || singleLine || !(alignment&(IE_FONT_ALIGN_BOTTOM|IE_FONT_ALIGN_MIDDLE)));

	int x = 0, y = 0;
	if (singleLine) {
		// optimization for single line vertical alignment
		if (alignment&IE_FONT_ALIGN_MIDDLE) {
			y = (rgn.h - maxHeight);
			if (y > 0) y /= 2;
			y -= descent / 2;
		} else if (alignment&IE_FONT_ALIGN_BOTTOM) {
			y = (rgn.h - maxHeight);
		} else {
			y = -descent;
		}
	}

	// is this horribly inefficient?
	std::wistringstream stream(string);
	String line, word;
	//const Sprite2D* currGlyph = NULL;
	bool done = false, lineBreak = false;
	size_t charCount = 0;
	ieByte* lineBuffer = NULL;
	if (!singleLine && alignment&(IE_FONT_ALIGN_CENTER|IE_FONT_ALIGN_RIGHT)) {
		// blit to a line buffer then blit it to screen/canvas
		// we dont need it for left alignment
		lineBuffer = (ieByte*)calloc(maxHeight + descent, rgn.w); // enough for maximum line
	}
	Glyph lineGlyphs(Size(rgn.w, maxHeight + descent), 0, lineBuffer, rgn.w);

	while (!done && (lineBreak || getline(stream, line))) {
		lineBreak = false;

		// check if we need to extend the canvas
		if (canvas && grow && rgn.h < y) {
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
		// reset the buffer
		if (lineBuffer)
			memset(lineBuffer, 0, rgn.w * lineGlyphs.dimensions.h);

		ieWord lineW = StringSize(line).w;
		if (singleLine && alignment&(IE_FONT_ALIGN_CENTER|IE_FONT_ALIGN_RIGHT)) {
			// optimization for single line horizontal alignmnet
			if (alignment & IE_FONT_ALIGN_CENTER) {
				x = ( rgn.w - lineW ) / 2;
			} else if (alignment & IE_FONT_ALIGN_RIGHT) {
				x = ( rgn.w - lineW );
			}
		} else {
			x = 0;
		}

		size_t lineLen = line.length();
		if (lineLen) {
			size_t linePos = 0, wordBreak = 0;
			while (line[linePos] == L' ') {
				// skip spaces at the beginning of a line
				linePos++;
			}
			// FIXME: I'm not sure how to handle Asian text
			// should a "word" be a single Asian glyph? that way we wouldnt clip off text (we were doing this before the rewrite too).
			// we could check the core encoding for the 'zerospace' attribute and treat single characters as words
			// that would looks funny with partial translations, however. we would need to handle both simultaniously.
			// TODO: word breaks shouldprobably happen on other characters such as '-' too.
			// not as simple as adding it to find_first_of
			while (!lineBreak && (wordBreak = line.find_first_of(L' ', linePos))) {
				word = line.substr(linePos, wordBreak - linePos);

				int wordW = StringSize(word).w;
				if (!(alignment&IE_FONT_SINGLE_LINE)) {
					if (x + wordW > rgn.w && wordW <= rgn.w) {
						// wrap to new line, only if the word isnt >= the entire line
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

						const Glyph& curGlyph = GetGlyph(currChar);
						// should probably use rect intersection, but new lines shouldnt be to the left ever.
						if (!rgn.PointInside(x + rgn.x, y + rgn.y + curGlyph.descent)) {
							if (wordW < (int)lineW) {
								// this probably doest cover every situation 100%
								// we consider printing done if the blitter is outside the region
								// *and* the word isnt wider then the line
								done = true;
							} else {
#if DEBUG_FONT
								Log(WARNING, "Font", "The word '%ls' (width=%d) overruns %d",
									word.c_str(), wordW, rgn.w);
#endif
							}
							break; // either done, or skipping
						}
						if (lineBuffer) {
								BlitGlyphToCanvas(curGlyph, x, curGlyph.descent, lineBuffer, rgn.Dimensions());
						} else if (canvas) {
								BlitGlyphToCanvas(curGlyph, x, y + curGlyph.descent, *canvas, rgn.Dimensions());
						} else {
							assert(AtlasIndex.find(currChar) != AtlasIndex.end());

							size_t pageIdx = AtlasIndex.at(currChar);
							assert(pageIdx < AtlasIndex.size());

							GlyphAtlasPage* page = Atlas[pageIdx];
							Region dst = Region(x + rgn.x, y + rgn.y + curGlyph.descent,
												curGlyph.dimensions.w, curGlyph.dimensions.h);
							page->Draw(currChar, dst);
						}
						x += curGlyph.dimensions.w;
					}
					if (done) break;
					linePos += i + 1;
				}
				if (wordBreak == String::npos) {
					linePos--; // we previously counted a non-existant space
					break;
				}
				x += GetGlyph(' ').dimensions.w;
			}
			charCount += linePos;
		}
		if (singleLine) break;

		if (lineBuffer) {
			if (alignment&IE_FONT_ALIGN_CENTER) {
				x = (rgn.w - x) / 2;
			} else { // right alignment
				x = rgn.w - x;
			}
			if (canvas) {
				BlitGlyphToCanvas(lineGlyphs, x, y - maxHeight, *canvas, rgn.Dimensions());
			} else {
				// FIXME: probably not very efficient.
				// we do this because we dont have ability to update GL texture pixels...
				Video* video = core->GetVideoDriver();
				Sprite2D* lineSprite = video->CreateSprite8(rgn.w, maxHeight, lineBuffer, color);
				video->BlitSprite(lineSprite, x + rgn.x, y + rgn.y, true, &rgn);
				lineSprite->release();
			}
		}

		if (!lineBreak && !stream.eof() && !done)
			charCount++; // for the newline
		y += maxHeight;
	}
	if (lineBuffer)
		free(lineBuffer);

	// free the unused canvas area (if any)
	if (canvas) {
		int usedh = y + descent;
		if (usedh < rgn.h) {
			// this is more than just saving memory
			// vertical alignment will be off if we have extra space
			*canvas = (ieByte*)realloc(*canvas, rgn.w * usedh);
			rgn.h = usedh;
		}
	}

	assert(charCount <= string.length());
	return charCount;
}

Sprite2D* Font::RenderTextAsSprite(const String& string, const Size& size,
								   ieByte alignment, Palette* color, size_t* numPrinted) const
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
	size_t ret = RenderText(string, rgn, palette, alignment, &canvasPx, (size.h) ? false : true);
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
				   Palette* color, ieByte alignment) const
{
	if (rgn.Dimensions().IsEmpty()) return 0;

	Palette* pal = color;
	if (!pal) {
		pal = palette;
	}
	if (!(alignment&IE_FONT_SINGLE_LINE) && alignment&(IE_FONT_ALIGN_MIDDLE|IE_FONT_ALIGN_BOTTOM)) {
		// easier to render the text as an image then vertically align it within rgn

		// FIXME: this is probably a bad idea to create and destroy the text every call to print.
		// the GL driver probably causes print to be called every frame which makes it worse
		// we should consider migrating any clients that use print + these alignments to TextSpans/Contaners
		size_t ret;
		Sprite2D* rendered = RenderTextAsSprite(string, rgn.Dimensions(), alignment, color, &ret);
		core->GetVideoDriver()->BlitSprite(rendered, rgn.x, rgn.y, true, &rgn); // this does the alignment so no need to adjust
		rendered->release();
		return ret;
	}
	return RenderText(string, rgn, pal, alignment);
}

Size Font::StringSize(const String& string, const Size* stop) const
{
	if (!string.length()) return Size();

	ieWord w = 0, h = 0, lines = 1;
	ieWord curh = 0, curw = 0;
	bool multiline = false;
	for (size_t i = 0; i < string.length(); i++) {
		if (string[i] == L'\n') {
			if (curw > w)
				w = curw;
			curw = 0;
			multiline = true;
			lines++;
		} else {
			const Glyph& curGlyph = GetGlyph(string[i]);
			curh = curGlyph.dimensions.h;
			curh += 0;
			if (curh > h)
				h = curh;
			curw += curGlyph.dimensions.w;
			if (i > 0) { // kerning
				curw -= GetKerningOffset(string[i-1], string[i]);
			}
		}
		if (stop && (curw > stop->w || curh > stop->h))
			return Size((curw > stop->w) ? stop->w : curw,
						(curh > stop->h) ? stop->h : curh);
	}
	if (!multiline) {
		h = maxHeight + descent;
	} else {
		h = (maxHeight * lines) + descent;
	}
	w = (curw > w) ? curw : w;
	return Size(w, h);
}

void Font::SetName(const char* newName)
{
	strnlwrcpy( name, newName, sizeof(name)-1);
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
