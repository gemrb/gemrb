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
#include "Palette.h"
#include "Sprite2D.h"

using namespace GemRB;

BAMFontManager::~BAMFontManager(void)
{
	delete bamImp;
}

BAMFontManager::BAMFontManager(void)
{
	isStateFont = false;
	bamImp = new BAMImporter();
	memset(resRef, 0, sizeof(ieResRef));
}

bool BAMFontManager::Open(DataStream* stream)
{
	ieWord len = strlench(stream->filename, '.');
	len = (len <= sizeof(ieResRef)-1) ? len : sizeof(ieResRef)-1;
	strncpy(resRef, stream->filename, len);
	// compare only first 6 chars so we can match states2 or others
	if (strnicmp(resRef, "STATES", 6) == 0) {
		isStateFont = true;
	}
	return bamImp->Open(stream);
}

Font* BAMFontManager::GetFont(unsigned short /*ptSize*/,
							  FontStyle /*style*/, Palette* pal)
{
	AnimationFactory* af = bamImp->GetAnimationFactory(resRef, IE_NORMAL, false); // released by BAMFont
	bool isNumeric = (af->GetCycleCount() <= 1);

	Sprite2D* first = NULL;
	if (isStateFont) {
		// Hack to work around original data where some status icons have inverted x and y positions (ie level up icon)
		// isStateFont is set in Open() and simply compares the first 6 characters of the file with "STATES"

		// since state icons should all be the same size/position we can just take the position of the first one
		// broken cycles:
		// 254 - level up icon
		// 153 - dialog icon
		// 154 - store icon
		first = af->GetFrameWithoutCycle(0);
	}

	// Cycles 0 and 1 of a BAM appear to be "magic" glyphs
	// I think cycle 1 is for determining line height (maxHeight)
	// this is important because iterating the initials font would give an incorrect maxHeight
	// initials should still have 13 for the line height because they have a descent that covers
	// multiple lines (3 in BG2). numeric and state fonts don't posess these magic glyphs,
	// but it is harmless to use them the same way
	int baseLine = (isNumeric) ? 0 : af->GetFrame(0, 0)->Height;
	int lineHeight = (isNumeric) ? af->GetFrame(0)->Height : af->GetFrame(0, 1)->Height;
	if (!first)
		first = af->GetFrameWithoutCycle(0);
	assert(first);

	Font* fnt = NULL;
	if (!pal) {
		pal = spr->GetPalette();
		fnt = new Font(pal, lineHeight, baseLine);
		pal->release();
	} else {
		fnt = new Font(pal, lineHeight, baseLine);
	}
	first->release();


	std::map<Sprite2D*, ieWord> tmp;
	Sprite2D* spr = NULL;
	for (ieWord cycle = 0; cycle < af->GetCycleCount(); cycle++) {
		for (ieWord frame = 0; frame < af->GetCycleSize(cycle); frame++) {
			spr = af->GetFrame(frame, cycle);
			assert(spr);

			ieWord chr = '\0';
			if (isNumeric) {
				chr = frame + '0';
			} else {
				chr = ((frame << 8) | (cycle&0x00ff)) + 1;
			}

			if (tmp.find(spr) != tmp.end()) {
				// opimization for when glyphs are shared between cycles
				// just alias the existing character
				// this is very useful for choping out huge chunks fo unused character ranges
				fnt->CreateAliasForChar(tmp.at(spr), chr);
			} else {
				fnt->CreateGlyphForCharSprite(chr, spr);
				tmp[spr] = chr;
			}
			spr->release();
		}
	}

	delete af;
	return fnt;
}
