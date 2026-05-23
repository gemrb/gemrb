// SPDX-FileCopyrightText: 2021 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef TILEPROPS_H
#define TILEPROPS_H

#include "RGBAColor.h"
#include "exports.h"

#include "Holder.h"
#include "PathFinder.h"
#include "Region.h"
#include "Sprite2D.h"

namespace GemRB {

class GEM_EXPORT TileProps {
	// tileProps contains the searchmap, the lightmap, the heightmap, and the material map
	// the assigned palette is the palette for the lightmap
	uint32_t* propPtr = nullptr;
	Size size;
	Holder<Sprite2D> propImage;

	static constexpr uint32_t searchMapMask = 0xff000000;
	static constexpr uint32_t materialMapMask = 0x00ff0000;
	static constexpr uint32_t heightMapMask = 0x0000ff00;
	static constexpr uint32_t lightMapMask = 0x000000ff;

	static constexpr uint32_t searchMapShift = 24;
	static constexpr uint32_t materialMapShift = 16;
	static constexpr uint32_t heightMapShift = 8;
	static constexpr uint32_t lightMapShift = 0;

public:
	static const PixelFormat pixelFormat;

	static constexpr uint8_t defaultSearchMap = uint8_t(PathMapFlags::IMPASSABLE);
	static constexpr uint8_t defaultMaterial = 0; // Black, impassable
	static constexpr uint8_t defaultElevation = 128; // sea level
	static constexpr uint8_t defaultLighting = 0; // color index 0? no better idea what a good default is

	enum class Property : uint8_t {
		SEARCH_MAP,
		MATERIAL,
		ELEVATION,
		LIGHTING
	};

	explicit TileProps(Holder<Sprite2D> props) noexcept;

	const Size& GetSize() const noexcept;

	void SetTileProp(const SearchmapPoint& p, Property prop, uint8_t val) noexcept;
	uint8_t QueryTileProp(const SearchmapPoint& p, Property prop) const noexcept;

	PathMapFlags QuerySearchMap(const SearchmapPoint& p) const noexcept;
	uint8_t QueryMaterial(const SearchmapPoint& p) const noexcept;
	int QueryElevation(const SearchmapPoint& p) const noexcept;
	Color QueryLighting(const SearchmapPoint& p) const noexcept;

	void PaintSearchMap(const SearchmapPoint&, PathMapFlags value) const noexcept;
	void PaintSearchMap(const SearchmapPoint& p, uint16_t blocksize, PathMapFlags value) const noexcept;
};

}
#endif
