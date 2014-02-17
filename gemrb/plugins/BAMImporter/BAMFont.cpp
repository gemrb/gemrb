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
	factory = af;
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

	blank = core->GetVideoDriver()->CreateSprite8(0, 0, NULL, palette);
}

BAMFont::~BAMFont()
{
	delete factory;
}

const Sprite2D* BAMFont::GetCharSprite(ieWord chr) const
{
	if (chr == 0) return blank;
	Sprite2D* spr = NULL;
	size_t cycleCount = factory->GetCycleCount();
	if (cycleCount > 1) {
		ieByte frame = ((chr >> 8) > 0) ? (chr >> 8) - 1 : 0; // multibyte char when > 0
		ieByte cycle = chr; // purposely truncating bits
		spr = factory->GetFrame(frame, cycle-1);
	} else {
		// numeric font
		spr = factory->GetFrameWithoutCycle(chr - '0');
	}
	if (!spr) {
		Log(ERROR, "BAMFont", "%s missing glyph for character '%x' using %s encoding.", name, chr, core->TLKEncoding.encoding.c_str());
		spr = blank;
	} else {
		spr->release();
	}
	return spr;
}

}
