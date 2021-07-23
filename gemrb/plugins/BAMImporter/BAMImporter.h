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
	Region bounds;
	bool RLE = false;
	strpos_t dataOffset = 0;
};

class Palette;
using PaletteHolder = Holder<Palette>;

class BAMImporter : public AnimationMgr {
public:
	BAMImporter(void);
	~BAMImporter(void) override;

	bool Open(DataStream* stream) override;
	index_t GetCycleSize(index_t Cycle) override;
	AnimationFactory* GetAnimationFactory(const ResRef &resref, bool allowCompression = true) override;
	/** Debug Function: Returns the Global Animation Palette as a Sprite2D Object.
	If the Global Animation Palette is NULL, returns NULL. */
	Holder<Sprite2D> GetPalette() override;
	index_t GetCycleCount() override
	{
		return cycles.size();
	}
private:
	using CycleEntry = AnimationFactory::CycleEntry;

	DataStream* str;
	std::vector<FrameEntry> frames;
	std::vector<CycleEntry> cycles;
	PaletteHolder palette;
	ieByte CompressedColorIndex;
	ieDword FramesOffset, PaletteOffset, FLTOffset;
	strpos_t DataStart;
	Holder<Sprite2D> GetFrameInternal(const FrameEntry& frame, bool RLESprite, uint8_t* data);
	std::vector<index_t> CacheFLT();
};

}

#endif
