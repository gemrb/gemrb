// SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef MOSIMPORTER_H
#define MOSIMPORTER_H

#include "ImageMgr.h"

namespace GemRB {

enum class MOSVersion {
	V1,
	V2
};

struct MOSV2DataBlock {
	ieDword pvrzPage;
	Point source;
	Size size;
	Point destination;
};

class MOSImporter : public ImageMgr {
private:
	MOSVersion version = MOSVersion::V1;
	ieWord Cols = 0;
	ieWord Rows = 0;
	union U {
		constexpr U() noexcept
			: v1 {} {} // only for compiler happiness
		struct {
			ieDword BlockSize = 0;
			ieDword PalOffset = 0;
		} v1;
		struct {
			ieDword NumBlocks = 0;
			ieDword BlockOffset = 0;
		} v2;
	} layout;

	ResourceHolder<ImageMgr> lastPVRZ;
	ieDword lastPVRZPage = 0;

	void Blit(const MOSV2DataBlock& dataBlock, uint8_t* data);
	Holder<Sprite2D> GetSprite2Dv1();
	Holder<Sprite2D> GetSprite2Dv2();

public:
	MOSImporter() noexcept = default;
	bool Import(DataStream* stream) override;
	Holder<Sprite2D> GetSprite2D() override;
	Holder<Sprite2D> GetSprite2D(Region&&) override { return {}; };
};

}

#endif
