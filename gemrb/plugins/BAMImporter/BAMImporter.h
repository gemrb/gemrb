// SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef BAMIMPORTER_H
#define BAMIMPORTER_H

#include "AnimationMgr.h"
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

enum class BAMVersion {
	V1,
	V2
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
	std::shared_ptr<AnimationFactory> GetAnimationFactory(const ResRef& resref, bool allowCompression = true) override;
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
	Holder<Palette> palette;
	ieByte CompressedColorIndex = 0;
	ieDword FLTOffset = 0;
	ieDword CyclesOffset = 0;
	ieDword FramesOffset = 0;
	strpos_t DataStart = 0;
	ResourceHolder<ImageMgr> lastPVRZ;
	ieDword lastPVRZPage = 0;

	void Blit(const FrameEntry& frame, const BAMV2DataBlock& dataBlock, uint8_t* data);
	std::vector<index_t> CacheFLT();
	Holder<Sprite2D> GetV2Frame(const FrameEntry& frame);
	Holder<Sprite2D> GetFrameInternal(const FrameEntry& frame, bool RLESprite, uint8_t* data) const;
};

}

#endif
