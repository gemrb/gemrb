/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2023 The GemRB Project
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

#ifndef PVRZIMP_H
#define PVRZIMP_H

#include <array>
#include <tuple>
#include <vector>

#include "ImageMgr.h"

namespace GemRB {

enum class PVRZFormat {
	DXT1 = 0x7,
	DXT5 = 0xB,
	UNSUPPORTED = 0xFF
};

class PVRZImporter : public ImageMgr {
public:
	PVRZImporter() noexcept = default;
	PVRZImporter(const PVRZImporter&) = delete;
	PVRZImporter& operator=(const PVRZImporter&) = delete;

	bool Import(DataStream* stream) override;
	Holder<Sprite2D> GetSprite2D() override;
	Holder<Sprite2D> GetSprite2D(Region&&) override;
	int GetPalette(int colors, Palette& pal) override;

private:
	std::tuple<uint16_t, uint16_t> extractPalette(size_t offset, std::array<uint8_t, 6>& colors) const;
	Holder<Sprite2D> getSprite2DDXT1(Region&&) const;
	Holder<Sprite2D> getSprite2DDXT5(Region&&) const;

	PVRZFormat format = PVRZFormat::UNSUPPORTED;
	std::vector<uint8_t> data;

	static uint16_t getBlockPixelMask(const Region& region, const Region& grid, int x, int y);
};

}

#endif
