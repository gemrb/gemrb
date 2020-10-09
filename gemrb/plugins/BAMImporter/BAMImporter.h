/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

#ifndef BAMIMPORTER_H
#define BAMIMPORTER_H

#include "AnimationMgr.h"

#include "RGBAColor.h"
#include "globals.h"
#include "Holder.h"

namespace GemRB {

struct FrameEntry {
	ieWord Width;
	ieWord  Height;
	ieWord  XPos;
	ieWord  YPos;
	ieDword FrameData;
};

class Palette;
using PaletteHolder = Holder<Palette>;

class BAMImporter : public AnimationMgr {
private:
	DataStream* str;
	FrameEntry* frames;
	CycleEntry* cycles;
	ieWord FramesCount;
	ieByte CyclesCount;
	PaletteHolder palette;
	ieByte CompressedColorIndex;
	ieDword FramesOffset, PaletteOffset, FLTOffset;
	unsigned long DataStart;
private:
	Holder<Sprite2D> GetFrameInternal(unsigned short findex, unsigned char mode,
							   bool RLESprite, unsigned char* data);
	void* GetFramePixels(unsigned short findex);
	ieWord * CacheFLT(unsigned int &count);
public:
	BAMImporter(void);
	~BAMImporter(void);
	bool Open(DataStream* stream);
	int GetCycleSize(unsigned char Cycle);
	AnimationFactory* GetAnimationFactory(const char* ResRef,
		unsigned char mode = IE_NORMAL, bool allowCompression = true);
	/** Debug Function: Returns the Global Animation Palette as a Sprite2D Object.
	If the Global Animation Palette is NULL, returns NULL. */
	Holder<Sprite2D> GetPalette();

	/** Gets a Pixel Index from the Image, unused */
	unsigned int GetPixelIndex(unsigned int /*x*/, unsigned int /*y*/)
	{
		return 0;
	}
	/** Gets a Pixel from the Image, unused */
	Color GetPixel(unsigned int /*x*/, unsigned int /*y*/)
	{
		return Color();
	}
public:
	int GetCycleCount()
	{
		return CyclesCount;
	}
};

}

#endif
