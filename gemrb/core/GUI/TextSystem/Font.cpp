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

#include "Debug.h"
#include "GameData.h"
#include "Logging/Logging.h"
#include "Palette.h"

#include "Video/Video.h"

#include <cwctype>
#include <utility>


namespace GemRB {

static void BlitGlyphToCanvas(const Glyph& glyph, const Point& p,
							  ieByte* canvas, const Size& size)
{
	const ieByte* src = glyph.pixels;
	if (canvas == NULL || src == NULL) {
		return; // need both a src and dst
	}

	// find the origin and clip to it.
	// only worry about origin < 0.
	Point blitPoint = p + glyph.pos;
	Size srcSize = glyph.size;
	if (blitPoint.y < 0) {
		int offset = (-blitPoint.y * glyph.size.w);
		src += offset;
		srcSize.h -= offset;
		blitPoint.y = 0;
	}
	if (blitPoint.x < 0) {
		int offset = -blitPoint.x;
		src += offset;
		srcSize.w -= offset;
		blitPoint.x = 0;
	}
	ieByte* dest = canvas + (size.w * blitPoint.y) + blitPoint.x;
	assert(src >= glyph.pixels);
	assert(dest >= canvas);
	// copy the glyph to the canvas
	for(int row = 0; row < srcSize.h; row++ ) {
		if (dest + srcSize.w > canvas + (size.w * size.h)) {
			break;
		}
		memcpy(dest, src, srcSize.w);
		dest += size.w;
		src += glyph.pitch;
	}
}

Font::GlyphAtlasPage::GlyphAtlasPage(Size pageSize, Font* font)
: SpriteSheet<ieWord>(), font(font)
{
	SheetRegion.w = pageSize.w;
	SheetRegion.h = pageSize.h;

	pageData = (ieByte*)calloc(pageSize.h, pageSize.w);
}

bool Font::GlyphAtlasPage::AddGlyph(ieWord chr, const Glyph& g)
{
	assert(glyphs.find(chr) == glyphs.end());
	int newX = pageXPos + g.size.w;
	if (newX > SheetRegion.w) {
		return false;
	}
	
	int glyphH = g.size.h + abs(g.pos.y);
	if (glyphH > SheetRegion.h) {
		// must grow to accommodate this glyph
		if (Sheet) {
			// if we already have a sheet we need to destroy it before we can add more glyphs
			pageData = (ieByte*)calloc(SheetRegion.w, glyphH);
			const ieByte* pixels = static_cast<const ieByte*>(Sheet->LockSprite());
			std::copy(pixels, pixels + (Sheet->Frame.w * Sheet->Frame.h), pageData);
			Sheet->UnlockSprite();
			Sheet = nullptr;
		} else {
			pageData = (ieByte*)realloc(pageData, SheetRegion.w * glyphH);
		}
		
		assert(pageData);
		SheetRegion.h = glyphH;
	} else if (Sheet) {
		// we need to lock/unlock the sprite because we are updating its pixels
		const void* pixels = Sheet->LockSprite();
		assert(pixels == pageData);
	}

	// have to adjust the x because BlitGlyphToCanvas will use g.pos.x, but we dont want that here.
	BlitGlyphToCanvas(g, Point(pageXPos - g.pos.x, (g.pos.y < 0) ? -g.pos.y : 0), pageData, SheetRegion.size);
	MapSheetSegment(chr, Region(pageXPos, (g.pos.y < 0) ? 0 : g.pos.y, g.size.w, g.size.h));
	// make the non-temporary glyph from our own data
	const ieByte* pageLoc = pageData + pageXPos;
	glyphs.emplace(chr, Glyph(g.size, g.pos, pageLoc, SheetRegion.w));

	pageXPos = newX;
	
	if (Sheet) {
		Sheet->UnlockSprite();
	}
	
	return true;
}

const Glyph& Font::GlyphAtlasPage::GlyphForChr(ieWord chr) const
{
	GlyphMap::const_iterator it = glyphs.find(chr);
	if (it != glyphs.end()) {
		return it->second;
	}
	const static Glyph blank(Size(0,0), Point(0, 0), NULL, 0);
	return blank;
}

void Font::GlyphAtlasPage::Draw(ieWord chr, const Region& dest, const PrintColors* colors)
{
	// ensure that we have a sprite!
	if (Sheet == NULL) {
		//Sheet = core->GetVideoDriver()->CreateSprite8(SheetRegion.w, SheetRegion.h, pageData, pal, true, 0);
		PixelFormat fmt = PixelFormat::Paletted8Bit(font->palette, true, 0);
		Sheet = VideoDriver->CreateSprite(SheetRegion, pageData, fmt);
		if (font->background) {
			invertedSheet = Sheet->copy();
			auto invertedPalette = MakeHolder<Palette>(*font->palette);
			for (auto& c : invertedPalette->col) {
				c.r = 255 - c.r;
				c.g = 255 - c.g;
				c.b = 255 - c.b;
			}
			invertedSheet->SetPalette(invertedPalette);
		}
	}
	
	if (colors) {
		if (font->background) {
			SpriteSheet<ieWord>::Draw(chr, dest, BlitFlags::BLENDED | BlitFlags::COLOR_MOD, colors->bg);
			// no point in BlitFlags::ADD with black so let's optimize away some blits
			if (colors->fg != ColorBlack) {
				std::swap(Sheet, invertedSheet);
				SpriteSheet<ieWord>::Draw(chr, dest, BlitFlags::ADD | BlitFlags::COLOR_MOD, colors->fg);
				std::swap(Sheet, invertedSheet);
			}
		} else {
			SpriteSheet<ieWord>::Draw(chr, dest, BlitFlags::BLENDED | BlitFlags::COLOR_MOD, colors->fg);
		}
	} else {
		SpriteSheet<ieWord>::Draw(chr, dest, BlitFlags::BLENDED, ColorWhite);
	}
}

void Font::GlyphAtlasPage::DumpToScreen(const Region& r) const
{
	VideoDriver->SetScreenClip(NULL);
	Region drawRgn = Region(0, 0, 1024, Sheet->Frame.h);
	VideoDriver->DrawRect(drawRgn, ColorBlack, true);
	VideoDriver->DrawRect(Sheet->Frame.Intersect(r), ColorWhite, false);
	VideoDriver->BlitSprite(Sheet, Sheet->Frame.Intersect(r), drawRgn, BlitFlags::BLENDED);
}

Font::Font(Holder<Palette> pal, ieWord lineheight, ieWord baseline, bool bg)
: palette(std::move(pal)), background(bg), LineHeight(lineheight), Baseline(baseline)
{}

Font::~Font(void)
{
	for (const auto& page : Atlas) {
		delete page;
	}
}

void Font::CreateGlyphIndex(ieWord chr, ieWord pageIdx, const Glyph* g)
{
	if (chr >= AtlasIndex.size()) {
		// potentially wasteful I guess, but much faster than a map.
		AtlasIndex.resize(chr+1);
	} else {
		assert(AtlasIndex[chr].pageIdx == static_cast<ieWord>(-1));
	}
	AtlasIndex[chr] = GlyphIndexEntry(chr, pageIdx, g);
}

const Glyph& Font::CreateGlyphForCharSprite(ieWord chr, const Holder<Sprite2D>& spr)
{
	assert(AtlasIndex.size() <= chr || AtlasIndex[chr].pageIdx == static_cast<ieWord>(-1));
	assert(spr);
	
	Size size(spr->Frame.w, spr->Frame.h);
	// FIXME: should we adjust for spr->Frame.x too?
	Point pos(0, Baseline - spr->Frame.y);

	Glyph tmp = Glyph(size, pos, (ieByte*)spr->LockSprite(), spr->Frame.w);
	spr->UnlockSprite(); // FIXME: this is assuming it is ok to hang onto to pixel buffer returned from LockSprite()
	// adjust the location for the glyph
	if (!CurrentAtlasPage || !CurrentAtlasPage->AddGlyph(chr, tmp)) {
		// page is full, make a new one
		CurrentAtlasPage = new GlyphAtlasPage(Size(1024, LineHeight), this);
		Atlas.push_back(CurrentAtlasPage);
		bool ok = CurrentAtlasPage->AddGlyph(chr, tmp);
		assert(ok);
	}
	assert(CurrentAtlasPage);
	const Glyph& g = CurrentAtlasPage->GlyphForChr(chr);
	CreateGlyphIndex(chr, Atlas.size() - 1, &g);
	return g;
}

void Font::CreateAliasForChar(ieWord chr, ieWord alias)
{
	// we cannot create an alias for a character that doesn't exist
	assert(AtlasIndex.size() > chr && AtlasIndex[chr].pageIdx != static_cast<ieWord>(-1));

	// we need to now find the page for the existing character and add this new one to that page
	const GlyphIndexEntry& idx = AtlasIndex[chr]; // this reference may become invalid after call to CreateGlyphIndex!
	ieWord pageIdx = idx.pageIdx;
	CreateGlyphIndex(alias, pageIdx, idx.glyph);
	Atlas[pageIdx]->MapSheetSegment(alias, (*Atlas[pageIdx])[chr]);
}

const Glyph& Font::GetGlyph(ieWord chr) const
{
	if (chr < AtlasIndex.size()) {
		const Glyph* g = AtlasIndex[chr].glyph;
		if (g) {
			return *g;
		}
	}
	const static Glyph blank(Size(0,0), Point(0, 0), NULL, 0);
	return blank;
}

size_t Font::RenderText(const String& string, Region& rgn, ieByte alignment, const PrintColors* colors,
						Point* point, ieByte** canvas, bool grow) const
{
	// NOTE: vertical alignment is not handled here.
	// it should have been calculated previously and passed in via the "point" parameter

	bool singleLine = (alignment&IE_FONT_SINGLE_LINE);
	Point dp = point ? *point : Point();
	const Region& sclip = VideoDriver->GetScreenClip();

	size_t charCount = 0;
	bool lineBreak = false;
	size_t stringPos = 0;
	String line;
	while (lineBreak || stringPos < string.length()) {
		if (lineBreak) {
			lineBreak = false;
		} else {
			size_t eolpos = string.find_first_of(u'\n', stringPos);
			if (eolpos == String::npos) {
				eolpos = string.length();
			} else {
				eolpos++; // convert from index
			}
			line = string.substr(stringPos, eolpos - stringPos);
			stringPos = eolpos;
		}

		// check if we need to extend the canvas
		if (canvas && grow && rgn.h < dp.y) {
			size_t pos = (stringPos < string.length()) ? stringPos : string.length() - 1;
			pos -= line.length();
			Size textSize = StringSize(string.substr(pos));
			ieWord numNewPixels = textSize.Area();
			ieWord lineArea = rgn.w * LineHeight;
			// round up
			ieWord numLines = 1 + ((numNewPixels - 1) / lineArea);
			// extend the region and canvas both
			size_t curpos = rgn.h * rgn.w;
			int vGrow = (numLines * LineHeight);
			rgn.h += vGrow;

			if (InDebugMode(DebugMode::FONTS)) {
				Log(MESSAGE, "Font", "Resizing canvas from {}x{} to {}x{}",
					rgn.w, rgn.h - vGrow, rgn.w, rgn.h);
			}

			*canvas = (ieByte*)realloc(*canvas, rgn.w * rgn.h);
			assert(canvas);
			// fill the buffer with the color key, or the new area or we will get garbage in the areas we dont blit to
			memset(*canvas + curpos, 0, vGrow * rgn.w);
		}

		dp.x = 0;
		size_t lineLen = line.length();
		if (lineLen) {
			const Region lineRgn(dp + rgn.origin, Size(rgn.w, LineHeight));
			StringSizeMetrics metrics = {lineRgn.size, 0, 0, true};
			const Size lineSize = StringSize(line, &metrics);
			size_t linePos = metrics.numChars;
			Point linePoint;

			// check to see if the line is on screen
			// TODO: technically we could be *even more* optimized by passing lineRgn, but this breaks dropcaps
			// this isn't a big deal ATM, because the big text containers do line-by-line layout
			if (!sclip.IntersectsRegion(rgn)) {
				// offscreen, optimize by bypassing RenderLine, we pre-calculated linePos above
				// alignment is completely irrelevant here since the width is the same for all alignments
				linePoint.x = lineSize.w;
			} else {
				// on screen
				if (alignment&(IE_FONT_ALIGN_CENTER|IE_FONT_ALIGN_RIGHT)) {
					linePoint.x += (rgn.w - lineSize.w); // this is right aligned, but we can adjust for center later on
					if (linePoint.x < 0) {
						linePos = String::npos;
						size_t prevPos = linePos;
						String word;
						while (linePoint.x < 0) {
							// yuck, this is not optimal. not sure of a better way.
							// we have to rewind, word by word, until X >= 0
							linePos = line.find_last_of(u' ', prevPos);
							if (linePos == String::npos) {
								if (InDebugMode(DebugMode::FONTS)) {
									Log(MESSAGE, "Font", "Horizontal alignment invalidated for '{}' due to insufficient width {}", fmt::WideToChar{line}, lineSize.w);
								}
								linePoint.x = 0;
								break;
							}
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
				if (InDebugMode(DebugMode::FONTS)) {
					VideoDriver->DrawRect(lineRgn, ColorGreen, false);
					VideoDriver->DrawRect(Region(linePoint + lineRgn.origin,
												 Size(lineSize.w, LineHeight)), ColorWhite, false);
				}
				linePos = RenderLine(line, lineRgn, linePoint, colors, canvas);
			}
			if (linePos == 0) {
				break; // if linePos == 0 then we would loop till we are out of bounds so just stop here
			}

			dp = dp + linePoint;
			if (linePos < line.length() - 1) {
				// ignore whitespace between current pos and next word, if any (we are wrapping... maybe)
				linePos = line.find_first_not_of(WHITESPACE_STRING_W, linePos);
				if (linePos == String::npos) {
					linePos = line.length() - 1; // newline char accounted for later
				} else {
					lineBreak = true;
					if (!singleLine) {
						// dont bother getting the next line if we arent going to print it
						line = line.substr(linePos);
					}
				}
			}
			charCount += linePos;
		}
		dp.y += LineHeight;

		if (singleLine || dp.y >= rgn.h) {
			break;
		}
	}

	// free the unused canvas area (if any)
	if (canvas) {
		int usedh = dp.y;
		if (usedh < rgn.h) {
			// this is more than just saving memory
			// vertical alignment will be off if we have extra space
			*canvas = (ieByte*)realloc(*canvas, rgn.w * usedh);
			rgn.h = usedh;
		}
	}

	if (point) {
		// deal with possible trailing newline
		if (charCount > 0 && string[charCount - 1] == u'\n') {
			dp.y += LineHeight;
		}
		*point = Point(dp.x, dp.y - LineHeight);
	}

	assert(charCount <= string.length());
	return charCount;
}

size_t Font::RenderLine(const String& line, const Region& lineRgn,
						Point& dp, const PrintColors* colors, ieByte** canvas) const
{
	assert(lineRgn.h == LineHeight);

	// NOTE: alignment is not handled here.
	// it should have been calculated previously and passed in via the "point" parameter

	size_t linePos = 0, wordBreak = 0;

	// FIXME: I'm not sure how to handle Asian text
	// should a "word" be a single Asian glyph? that way we wouldnt clip off text (we were doing this before the rewrite too).
	// we could check the core encoding for the 'zerospace' attribute and treat single characters as words
	// that would looks funny with partial translations, however. we would need to handle both simultaneously.

	// TODO: word breaks should probably happen on other characters such as '-' too.
	// not as simple as adding it to find_first_of
	bool done = false;
	do {
		wordBreak = line.find_first_of(u' ', linePos);
		String word;
		if (wordBreak == linePos) {
			word = u' ';
		} else {
			word = line.substr(linePos, wordBreak - linePos);
		}

		StringSizeMetrics metrics = {lineRgn.size, 0, 0, true};
		int wordW = StringSize(word, &metrics).w;
		if (dp.x == 0 && metrics.forceBreak) {
			done = true;
			word.resize(metrics.numChars);
			assert(metrics.size.w <= lineRgn.w);
		} else if (dp.x + wordW > lineRgn.w) {
			// overflow with no wrap allowed; abort.
			break;
		}

		// print the word
		char16_t currChar = u'\0';
		size_t i = 0;
		for (; i < word.length(); i++) {
			// process glyphs in word
			currChar = word[i];
			if (currChar == u'\r' || currChar == u'\n') {
				continue;
			}
			if (i > 0) { // kerning
				dp.x -= GetKerningOffset(word[i-1], currChar);
			}

			const Glyph& curGlyph = GetGlyph(currChar);
			Point blitPoint = dp + lineRgn.origin + curGlyph.pos;
			// use intersection because some rare glyphs can sometimes overlap lines
			if (!lineRgn.IntersectsRegion(Region(blitPoint, curGlyph.size))) {
				if (InDebugMode(DebugMode::FONTS)) {
					VideoDriver->DrawRect(lineRgn, ColorRed, false);
				}
				assert(metrics.forceBreak == false || dp.x > 0);
				done = true;
				break;
			}

			if (canvas) {
				BlitGlyphToCanvas(curGlyph, blitPoint, *canvas, lineRgn.size);
			} else {
				size_t pageIdx = AtlasIndex[currChar].pageIdx;
				GlyphAtlasPage* page = Atlas[pageIdx];
				page->Draw(currChar, Region(blitPoint, curGlyph.size), colors);
			}
			dp.x += curGlyph.size.w;
		}
		linePos += i;
		if (done) break;
	} while (linePos < line.length());
	assert(linePos <= line.length());
	return linePos;
}

size_t Font::Print(const Region& rgn, const String& string, ieByte alignment, Point* point) const
{
	return Print(rgn, string, alignment, nullptr, point);
}

size_t Font::Print(const Region& rgn, const String& string, ieByte alignment, const PrintColors& colors, Point* point) const
{
	return Print(rgn, string, alignment, &colors, point);
}

size_t Font::Print(Region rgn, const String& string, ieByte alignment, const PrintColors* colors, Point* point) const
{
	if (rgn.size.IsInvalid()) return 0;

	Point p = point ? *point : Point();
	if (alignment&(IE_FONT_ALIGN_MIDDLE|IE_FONT_ALIGN_BOTTOM)) {
		// we assume that point will be an offset from midde/bottom position
		Size stringSize;
		if (alignment&IE_FONT_SINGLE_LINE) {
			// we can optimize single lines without StringSize()
			stringSize.h = LineHeight;
		} else {
			stringSize = rgn.size;
			StringSizeMetrics metrics = {stringSize, 0, 0, true};
			stringSize = StringSize(string, &metrics);
			if (alignment&IE_FONT_NO_CALC && metrics.numChars < string.length()) {
				// PST GUISTORE, not sure what else
				stringSize.h = rgn.h;
			}
		}

		// important: we must do this adjustment even if it leads to -p.y!
		// some labels depend on this behavior (BG2 GUIINV) :/
		if (alignment&IE_FONT_ALIGN_MIDDLE) {
			p.y += (rgn.h - stringSize.h) / 2;
		} else { // bottom alignment
			p.y += rgn.h - stringSize.h;
		}
	}

	size_t ret = RenderText(string, rgn, alignment, colors, &p);

	if (point) {
		*point = p;
	}
	return ret;
}

size_t Font::StringSizeWidth(const String& string, size_t width, size_t* numChars) const
{
	size_t size = 0;
	size_t i = 0;
	size_t length = string.length();
	for (; i < length; ++i) {
		wchar_t c = string[i];
		if (c == u'\n') {
			break;
		}

		const Glyph& curGlyph = GetGlyph(c);
		ieWord chrW = curGlyph.size.w;
		if (i > 0) {
			chrW -= GetKerningOffset(string[i-1], string[i]);
		}

		if (width > 0 && size + chrW >= width) {
			break;
		}

		size += chrW;
	}

	if (numChars) {
		*numChars = i;
	}
	return size;
}

Size Font::StringSize(const String& string, StringSizeMetrics* metrics) const
{
	if (!string.length()) return Size();
#define WILL_WRAP(val) \
	(stop && stop->w && lineW + (val) > stop->w)

#define APPEND_TO_LINE(val) \
	lineW += val; charCount = i + 1; val = 0
	
	ieWord w = 0, lines = 1;
	ieWord lineW = 0, wordW = 0, spaceW = 0;
	bool newline = false, eos = false, ws = false, forceBreak = false;
	size_t i = 0, charCount = 0;
	const Size* stop = metrics ? &metrics->size : nullptr;
	for (; i < string.length(); i++) {
		const Glyph& curGlyph = GetGlyph(string[i]);
		eos = (i == string.length() - 1);
		ws = std::iswspace(string[i]);
		if (!ws) {
			ieWord chrW = curGlyph.size.w;
			if (lineW > 0) { // kerning
				chrW -= GetKerningOffset(string[i-1], string[i]);
			}
			if (lineW == 0 && wordW > 0 && WILL_WRAP(wordW + chrW) && metrics->forceBreak && wordW <= stop->w) {
				// the word is longer than the line allows, but we allow a break mid-word
				forceBreak = true;
				newline = true;
				APPEND_TO_LINE(wordW);
			}
			wordW += chrW;
			// spaceW is the *cumulative* whitespace between the 2 words
			wordW += spaceW;
			spaceW = 0;
		} // no else
		if (ws || eos) {
			if (WILL_WRAP(spaceW + wordW)) {
				newline = true;
			} else {
				if (string[i] == u'\n') {
					// always append *everything* if there is \n
					lineW += spaceW; // everything else appended later
					newline = true;
				} else if (ws && string[i] != u'\r') {
					spaceW += curGlyph.size.w;
				}
				APPEND_TO_LINE(wordW);
			}
		}

		if (newline || eos) {
			w = std::max(w, lineW);
			if (stop) {
				if (stop->h && (LineHeight * (lines + 1)) > stop->h ) {
					break;
				}
				if (eos && stop->w && wordW < stop->w ) {
					// its possible the last word of string is longer than any of the previous lines
					w = std::max(w, wordW);
				}
			} else {
				w = std::max(w, wordW);
			}

			if (metrics && metrics->numLines > 0 && metrics->numLines <= lines) {
				break;
			}

			if (newline) {
				newline = false;
				lines++;
				lineW = 0;
				spaceW = 0;
			}
		}
	}
#undef WILL_WRAP
#undef APPEND_TO_LINE

	w += ((w == 0 && wordW == 0) || !newline) ? spaceW : 0;

	if (metrics) {
		if (forceBreak) charCount--;
		metrics->forceBreak = forceBreak;
		metrics->numChars = charCount;
		metrics->size = Size(w, (LineHeight * lines));
		metrics->numLines = lines;
		if (InDebugMode(DebugMode::FONTS)) {
			assert(metrics->numChars <= string.length());
			assert(w <= stop->w);
		}
		return metrics->size;
	}

	return Size(w, (LineHeight * lines));
}

}
