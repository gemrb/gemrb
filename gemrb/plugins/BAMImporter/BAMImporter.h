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
	union {
		strpos_t dataOffset = 0;
		struct {
			uint16_t dataBlockIdx;
			uint16_t dataBlockCount;
		} v2;
	} location;
};

class ImageMgr;
class Palette;
using PaletteHolder = Holder<Palette>;

enum class BAMVersion {
	V1, V2
};

struct BAMV2DataBlock {
	ieDword pvrzPage;
	Point source;
	Size size;
	Point destination;
};

class BAMImporter : public AnimationMgr {
public:
	bool Import(DataStream* stream) override;
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

	BAMVersion version = BAMVersion::V1;
	std::vector<FrameEntry> frames;
	std::vector<CycleEntry> cycles;
	PaletteHolder palette;
	ieByte CompressedColorIndex = 0;
	ieDword FLTOffset = 0;
	ieDword CyclesOffset = 0;
	ieDword FramesOffset = 0;
	strpos_t DataStart = 0;
	std::shared_ptr<ImageMgr> lastPVRZ;
	ieDword lastPVRZPage;

	void Blit(const FrameEntry& frame, const BAMV2DataBlock& dataBlock, uint8_t* data);
	std::vector<index_t> CacheFLT();
	Holder<Sprite2D> GetV2Frame(const FrameEntry& frame);
	Holder<Sprite2D> GetFrameInternal(const FrameEntry& frame, bool RLESprite, uint8_t* data);
};

}

#endif
