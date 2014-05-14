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

#include "BAMFont.h"
#include "Interface.h"
#include "Sprite2D.h"
#include "Video.h"

namespace GemRB {

BAMFont::BAMFont(Palette* pal, AnimationFactory* af)
	: Font(pal)
{
	bool isNumeric = (af->GetCycleCount() <= 1);
	// Cycles 0 and 1 of a BAM appear to be "magic" glyphs
	// I think cycle 1 is for determining line height (maxHeight)
	// this is important because iterating the initials font would give an incorrect maxHeight
	// initials should still have 13 for the line height because they have a descent that covers
	// multiple lines (3 in BG2). numeric and state fonts don't posess these magic glyphs,
	// but it is harmless to use them the same way
	if (isNumeric) {
		maxHeight = af->GetFrame(0)->Height;
		descent = 0;
	} else {
		maxHeight = af->GetFrame(0, 1)->Height;

		Sprite2D* curGlyph = NULL;
		descent = 0;
		int curDescent = 0;
		for (size_t i = 0; i < af->GetFrameCount(); i++) {
			curGlyph = af->GetFrameWithoutCycle(i);
			if (curGlyph) {
				curDescent = curGlyph->Height - curGlyph->YPos;
				descent = (curDescent > descent) ? curDescent : descent;
				curGlyph->release();
			}
		}
	}

	Sprite2D* spr = NULL;
	for (ieWord cycle = 0; cycle < af->GetCycleCount(); cycle++) {
		for (ieWord frame = 0; frame < af->GetCycleSize(cycle); frame++) {
			spr = af->GetFrame(frame, cycle);
			assert(spr);
			wchar_t chr = ((frame << 8) | (cycle&0x00ff)) + 1;

			CreateGlyphForCharSprite(chr, spr);
			spr->release();
		}
	}
	delete af;
}


}
