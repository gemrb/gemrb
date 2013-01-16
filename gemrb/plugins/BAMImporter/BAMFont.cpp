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

	glyphCount = 0;
	size_t cycleCount = af->GetCycleCount();
	// FIXME: we will end up with many useless (blank) glyphs
	for (size_t i = 0; i < cycleCount; i++) {
		glyphCount += af->GetCycleSize(i);
	}
	if (glyphCount == 0) {
		// minimal test uses bam without cycles
		// this will fall into the "numeric" case below
		glyphCount = af->GetFrameCount();
	}
	assert(glyphCount);

	glyphs = (Sprite2D**)malloc( glyphCount * sizeof(Sprite2D*) );
	ieWord glyphIndex = 0;
	FirstChar = 1;
	maxHeight = 0;

	if (cycleCount > 1) {
		for (size_t i = 0; i < cycleCount; i++) {
			if (af->GetFrameCount() < 256) {
				glyphs[glyphIndex] = af->GetFrame(0, i);
				glyphs[glyphIndex]->XPos = 0;
				if (baseline) {
					glyphs[glyphIndex]->YPos = *baseline;
				}
				if (glyphs[glyphIndex]->Height > maxHeight)
					maxHeight = glyphs[glyphIndex]->Height;
				glyphIndex++;
			} else {
				for (int j = 0; j < af->GetCycleSize(i); j++) {
					glyphs[glyphIndex] = af->GetFrame(j, i);
					glyphs[glyphIndex]->XPos = 0;
					if (glyphs[glyphIndex]->Height > maxHeight)
						maxHeight = glyphs[glyphIndex]->Height;
					glyphIndex++;
				}
			}
		}
	} else {
		// this is a numeric font
		FirstChar = '0';
		for (glyphIndex = 0; glyphIndex < af->GetFrameCount(); glyphIndex++) {
			glyphs[glyphIndex] = af->GetFrameWithoutCycle(glyphIndex);
			// we want them to have the same baseline as the rest
			glyphs[glyphIndex]->YPos = 13 - glyphs[glyphIndex]->Height;
			glyphs[glyphIndex]->XPos = 0;
			if (glyphs[glyphIndex]->Height > maxHeight)
				maxHeight = glyphs[glyphIndex]->Height;
		}
	}
	LastChar = glyphIndex--;

	// assume all sprites have same palette
	Palette* pal = glyphs[0]->GetPalette();
	SetPalette(pal);
	pal->Release();

	whiteSpace[BLANK] = core->GetVideoDriver()->CreateSprite8(0, 0, 8, NULL, palette->col);
	// standard space width is 1/4 ptSize
	whiteSpace[SPACE] = core->GetVideoDriver()->CreateSprite8((int)(maxHeight * 0.25), 0, 8, NULL, palette->col);
	// standard tab width is 4 spaces???
	whiteSpace[TAB] = core->GetVideoDriver()->CreateSprite8((whiteSpace[1]->Width * 4), 0, 8, NULL, palette->col);
}

BAMFont::~BAMFont()
{
	delete factory;
}

const Sprite2D* BAMFont::GetCharSprite(ieWord chr) const
{
	if (chr >= FirstChar && chr <= LastChar) {
		return glyphs[chr - FirstChar];
	}
	if (chr == ' ') return whiteSpace[SPACE];
	if (chr == '\t') return  whiteSpace[TAB];
	//otherwise return an empty sprite
	return whiteSpace[BLANK];
}

}
