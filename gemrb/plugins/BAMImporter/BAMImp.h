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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/BAMImporter/BAMImp.h,v 1.12 2004/08/04 19:21:55 guidoj Exp $
 *
 */

#ifndef BAMIMP_H
#define BAMIMP_H

#include "../Core/AnimationMgr.h"
#include "../../includes/globals.h"
#include "../../includes/RGBAColor.h"

typedef struct FrameEntry {
	ieWord Width;
	ieWord  Height;
	ieWord  XPos;
	ieWord  YPos;
	ieDword FrameData;
} FrameEntry;

class BAMImp : public AnimationMgr {
private:
	DataStream* str;
	bool autoFree;
	FrameEntry* frames;
	CycleEntry* cycles;
	ieWord FramesCount;
	ieByte CyclesCount;
	//std::vector<FrameEntry> frames;
	//std::vector<CycleEntry> cycles;
	Color Palette[256];
	ieByte CompressedColorIndex;
	ieDword FramesOffset, PaletteOffset, FLTOffset;
public:
	BAMImp(void);
	~BAMImp(void);
	bool Open(DataStream* stream, bool autoFree = true);
	Sprite2D* GetFrameFromCycle(unsigned char Cycle, unsigned short frame);
	Animation* GetAnimation(unsigned char Cycle, int x, int y,
		unsigned char mode = IE_NORMAL);
	AnimationFactory* GetAnimationFactory(const char* ResRef,
		unsigned char mode = IE_NORMAL);
	Sprite2D* GetFrame(unsigned short findex, unsigned char mode = IE_NORMAL);
	void* GetFramePixels(unsigned short findex, unsigned char mode = IE_NORMAL);
	/** This function will load the Animation as a Font */
	Font* GetFont();
	/** Debug Function: Returns the Global Animation Palette as a Sprite2D Object.
	If the Global Animation Palette is NULL, returns NULL. */
	Sprite2D* GetPalette();
	/** Gets a Pixel Index from the Image, unused */
	unsigned int GetPixelIndex(unsigned int x, unsigned int y)
	{
		return 0;
	}
	/** Gets a Pixel from the Image, unused */
	Color GetPixel(unsigned int x, unsigned int y)
	{
		Color null = {
			0x00, 0x00, 0x00, 0x00
		};
		return null;
	}
public:
	void release(void)
	{
		delete this;
	}
	int GetCycleCount()
	{
		return CyclesCount;
	}
};

#endif
