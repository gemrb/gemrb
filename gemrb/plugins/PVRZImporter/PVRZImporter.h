// SPDX-FileCopyrightText: 2023 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef PVRZIMP_H
#define PVRZIMP_H

#include "ImageMgr.h"

#include <array>
#include <tuple>
#include <vector>

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

	static uint16_t GetBlockPixelMask(const Region& region, const Region& grid, int x, int y);

private:
	std::tuple<uint16_t, uint16_t> extractPalette(size_t offset, std::array<uint8_t, 6>& colors) const;
	Holder<Sprite2D> getSprite2DDXT1(Region&&) const;
	Holder<Sprite2D> getSprite2DDXT5(Region&&) const;

	PVRZFormat format = PVRZFormat::UNSUPPORTED;
	std::vector<uint8_t> data;
};

}

#endif
