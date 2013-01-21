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

BAMFont::BAMFont(AnimationFactory* af, int* baseline)
{
	factory = af;
	maxHeight = 0;

	Sprite2D* curGlyph = NULL;
	size_t cycleCount = af->GetCycleCount();
	if (cycleCount > 1) {
		for (size_t i = 0; i < cycleCount; i++) {
			for (int j = 0; j < af->GetCycleSize(i); j++) {
				curGlyph = af->GetFrame(j, i);
				if (curGlyph) {
					if (curGlyph->Height > maxHeight)
						maxHeight = curGlyph->Height;
					if (baseline)
						curGlyph->YPos = *baseline;
					curGlyph->XPos = 0;
					curGlyph->release();
				}
			}
		}
	} else {
		// numeric font
		for (size_t i = 0; i < af->GetFrameCount(); i++) {
			curGlyph = af->GetFrameWithoutCycle(i);
			if (curGlyph) {
				if (curGlyph->Height > maxHeight)
					maxHeight = curGlyph->Height;
				curGlyph->XPos = 0;
				// we want them to have the same baseline as the rest
				curGlyph->YPos = 13 - curGlyph->Height;
				curGlyph->release();
			}
		}
	}

	// assume all sprites have same palette
	Sprite2D* first = af->GetFrameWithoutCycle(0);
	Palette* pal = first->GetPalette();
	SetPalette(pal);
	pal->Release();
	first->release();

	blank = core->GetVideoDriver()->CreateSprite8(0, 0, 8, NULL, palette->col);
}

BAMFont::~BAMFont()
{
	delete factory;
}

const Sprite2D* BAMFont::GetCharSprite(ieWord chr) const
{
	Sprite2D* spr = NULL;
	size_t cycleCount = factory->GetCycleCount();
	if (cycleCount > 1) {
		if (factory->GetFrameCount() <= 256) {
			spr = factory->GetFrame(0, chr-1);
		} else {
			// chr is multibyte
			// TODO: implement multibyte bam support
		}
	} else {
		// numeric font
		spr = factory->GetFrameWithoutCycle(chr - '0');
	}
	if (!spr) {
		spr = blank;
	} else {
		spr->release();
	}
	return spr;
}

}
