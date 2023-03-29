/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

#ifndef MOSIMPORTER_H
#define MOSIMPORTER_H

#include "ImageMgr.h"

namespace GemRB {

enum class MOSVersion {
	V1, V2
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
		constexpr U() noexcept : v1{} {} // only for compiler happiness
		struct {
			ieDword BlockSize = 0;
			ieDword PalOffset = 0;
		} v1;
		struct {
			ieDword NumBlocks = 0;
			ieDword BlockOffset = 0;
		} v2;
	} layout;

	std::shared_ptr<ImageMgr> lastPVRZ;
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
