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

#include "BAMFontManager.h"

#include "BAMImporter.h"
#include "Sprite2D.h"

using namespace GemRB;

BAMFontManager::~BAMFontManager(void)
{
	delete bamImp;
}

BAMFontManager::BAMFontManager(void)
{
	bamImp = new BAMImporter();
}

bool BAMFontManager::Import(DataStream* stream)
{
	resRef = stream->filename;
	// compare only first 6 chars so we can match states2 or others
	if (resRef.BeginsWith("STATES")) {
		isStateFont = true;
	}

	str = nullptr; // hand ownership over to bamImp
	return bamImp->Open(stream);
}

Holder<Font> BAMFontManager::GetFont(unsigned short /*ptSize*/, FontStyle /*style*/, bool background)
{
	auto af = bamImp->GetAnimationFactory(resRef, false); // released by BAMFont
	// FIXME: this test only exists to let the minimal test pass
	// we should maybe instead use a *valid* font for such a "test"
	if (af->GetFrame(0) == nullptr) {
		return nullptr;
	}

	if (af->GetFrameCount() == 0) return NULL; // the minimal test will explode without this...
	bool isNumeric = (af->GetCycleCount() <= 1);


	if (isStateFont) {
		// Hack to work around original data where the "top row icons" have inverted x and y positions (ie level up icon)
		// isStateFont is set in Open() and simply compares the first 6 characters of the file with "STATES"
		// since state icons should all be the same size/position we can just take the position of the first one
		static const ieWord topIconCycles[] = { 254 /* level up icon */, 153 /* dialog icon */, 154 /* store icon */, 37 /* separator glyph (like '-')*/ };
		for (size_t i = 0; i < 3; i++) {
			Holder<Sprite2D> spr = af->GetFrame(0, topIconCycles[i]);
			if (spr->Frame.x > 0) // not all datasets are messed up here
				spr->Frame.y = spr->Frame.x;
		}
	}

	// Cycles 0 and 1 of a BAM appear to be "magic" glyphs
	// I think cycle 1 is for determining line height (it appears to be a cursor)
	// this is important because iterating the initials font would give an incorrect LineHeight
	// initials should still have 13 for the line height because they have a descent that covers
	// multiple lines (2 in BG2). Numeric and state fonts don't possess these magic glyphs,
	// but it is harmless to use them the same way
	ieWord baseLine = 0;
	ieWord lineHeight = 0;
	if (isNumeric) {
		baseLine = 0;
		lineHeight = af->GetFrame(0)->Frame.h;
	} else {
		baseLine = af->GetFrame(0, 0)->Frame.h;
		lineHeight = af->GetFrame(0, 1)->Frame.h;
	}

	auto pal = af->GetFrameWithoutCycle(0)->GetPalette();
	auto fnt = MakeHolder<Font>(std::move(pal), lineHeight, baseLine, background);

	std::map<Sprite2D*, ieWord> tmp;
	for (ieWord cycle = 0; cycle < af->GetCycleCount(); cycle++) {
		for (ieWord frame = 0; frame < af->GetCycleSize(cycle); frame++) {
			Holder<Sprite2D> spr = af->GetFrame(frame, cycle);
			assert(spr);

			ieWord chr = '\0';
			if (isNumeric) {
				chr = frame + '0';
			} else {
				chr = ((frame << 8) | (cycle & 0x00ff)) + 1;
			}
			Sprite2D* key = spr.get();
			auto i = tmp.find(key);
			if (i != tmp.end()) {
				// opimization for when glyphs are shared between cycles
				// just alias the existing character
				// this is very useful for chopping out huge chunks of unused character ranges
				fnt->CreateAliasForChar(i->second, chr);
			} else {
				fnt->CreateGlyphForCharSprite(chr, spr);
				tmp[key] = chr;
			}
		}
	}

	return fnt;
}
