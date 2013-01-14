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
}

bool BAMFontManager::Open(DataStream* stream)
{
	char tmp[16]; // 16 is the fileneame length in DataStream
	strncpy(tmp, stream->filename, 6); // only copy length of "STATES" characters so we can match "STATES2" or others too
	if (strnicmp(tmp, "STATES", 6) == 0) {
		isStateFont = true;
	}
	return bamImp->Open(stream);
}

Font* BAMFontManager::GetFont(unsigned short /*ptSize*/,
							  FontStyle /*style*/, Palette* pal)
{
	AnimationFactory* af = bamImp->GetAnimationFactory("dummy"); // FIXME: how does this get released?
	//int glyphIndexOffset = 0, limit = 0, Count = 0, glyphCount = 0;
	size_t glyphCount = 0, cycleCount = af->GetCycleCount();

	// FIXME: we will end up with many useless (blank) glyphs
	for (size_t i = 0; i < cycleCount; i++) {
		glyphCount += af->GetCycleSize(i);
	}

	Sprite2D** glyphs = (Sprite2D**)malloc( glyphCount * sizeof(Sprite2D*) );
	ieWord glyphIndex = 0;

	for (size_t i = 0; i < cycleCount; i++) {
		for (int j = 0; j < af->GetCycleSize(i); j++) {
			glyphs[glyphIndex] = af->GetFrame(j, i);

			if (isStateFont) {
				// Hack to work around original data where some status icons have inverted x and y positions (ie level up icon)
				// isStateFont is set in Open() and simply compares the first 6 characters of the file with "STATES"

				// since state icons should all be the same size/position we can just take the position of the first one
				glyphs[glyphIndex]->YPos = glyphs[0]->YPos;
			} else if (cycleCount == 1) {
				// this is a numeric font
				// we want them to have the same baseline as the rest
				glyphs[glyphIndex]->YPos = 13 - glyphs[glyphIndex]->Height;
			}
			glyphIndex++;
		}
	}

	// assume all sprites have same palette
	Palette* palette = glyphs[0]->GetPalette();
	Font* fnt = new Font(glyphs, 1, glyphCount, palette);
	palette->Release();
	if (pal) {
		fnt->SetPalette(pal);
	}
	return fnt;
}
