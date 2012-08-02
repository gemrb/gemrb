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
	bamImp = new BAMImporter();
}

bool BAMFontManager::Open(DataStream* stream)
{
	return bamImp->Open(stream);
}

Font* BAMFontManager::GetFont(ieWord FirstChar,
			  ieWord LastChar,
			  unsigned short /*ptSize*/,
			  FontStyle /*style*/, Palette* pal)
{
	AnimationFactory* af = bamImp->GetAnimationFactory("dummy"); // FIXME: how does this get released?
	unsigned int i = 0, glyphIndexOffset = 0, limit = 0, Count = 0, glyphCount = 0;
	unsigned int CyclesCount = af->GetCycleCount();

	// Numeric fonts have all frames in single cycle
	if (CyclesCount > 1) {
		Count = CyclesCount;
		glyphCount = (LastChar - FirstChar + 1);
		if (Count < glyphCount){
			LastChar = LastChar - (glyphCount - Count);
			glyphCount = Count;
		}
		i = (FirstChar) ? FirstChar - 1 : FirstChar;
		limit = (FirstChar) ? LastChar - 1 : LastChar;
		glyphIndexOffset = i;
	} else { //numeric font
		Count = af->GetFrameCount();
		glyphCount = Count;
		if (FirstChar+Count != (unsigned int) LastChar+1) {
			Log(ERROR, "BAMFontManager", "inconsistent font %s: FirstChar=%d LastChar=%d Count=%d",
				str->filename, FirstChar, LastChar, Count);
			return NULL;
		}
		limit = glyphCount - 1;
	}

	Sprite2D** glyphs = (Sprite2D**)malloc( glyphCount * sizeof(Sprite2D*) );

	for (; i <= limit; i++) {
		if (CyclesCount > 1) {
			glyphs[i - glyphIndexOffset] = af->GetFrame(0, i);
		} else {
			glyphs[i - glyphIndexOffset] = af->GetFrameWithoutCycle(i);
		}
	}

	// assume all sprites have same palette
	Palette* palette = glyphs[0]->GetPalette();
	Font* fnt = new Font(glyphs, FirstChar, LastChar, palette);
	palette->Release();
	if (pal) {
		fnt->SetPalette(pal);
	}
	return fnt;
}
