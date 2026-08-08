// SPDX-FileCopyrightText: 2021 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef TILEPROPS_H
#define TILEPROPS_H

#include "RGBAColor.h"
#include "exports.h"

#include "EnumFlags.h"
#include "Holder.h"
#include "Region.h"
#include "Sprite2D.h"

#include <cstdint>

namespace GemRB {
// upper bound on the radius of the circle painted into / tested against the searchmap
constexpr unsigned int MAX_CIRCLESIZE = 8;

//searchmap conversion bits

enum class PathMapFlags : uint8_t {
	UNMARKED = 0,
	IMPASSABLE = 0,
	PASSABLE = 1,
	TRAVEL = 2,
	NO_SEE = 4,
	SIDEWALL = 8,
	AREAMASK = (IMPASSABLE | PASSABLE | TRAVEL | NO_SEE | SIDEWALL),
	DOOR_OPAQUE = 16,
	DOOR_IMPASSABLE = 32,
	PC = 64,
	NPC = 128,
	ACTOR = (PC | NPC),
	DOOR = (DOOR_OPAQUE | DOOR_IMPASSABLE),
	NOTAREA = (ACTOR | DOOR),
	NOTDOOR = (ACTOR | AREAMASK),
	NOTACTOR = (DOOR | AREAMASK)
};

class GEM_EXPORT TileProps {
protected:
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

	TileProps() = default;
	// OwningTileProps derives from this and owns a heap buffer, so destruction has to dispatch.
	// Copy and move are defaulted explicitly because declaring a destructor otherwise suppresses
	// the implicit moves, and TileProps is passed by value into Map.
	virtual ~TileProps() = default;
	TileProps(const TileProps&) = default;
	TileProps& operator=(const TileProps&) = default;
	TileProps(TileProps&&) = default;
	TileProps& operator=(TileProps&&) = default;

	const uint32_t* GetPropPtr() const
	{
		return propPtr;
	}

	const Size& GetSize() const noexcept;

	void SetTileProp(const SearchmapPoint& p, Property prop, uint8_t val) noexcept;
	uint8_t QueryTileProp(const SearchmapPoint& p, Property prop) const noexcept;

	PathMapFlags QuerySearchMap(const SearchmapPoint& p) const noexcept;
	uint8_t QueryMaterial(const SearchmapPoint& p) const noexcept;
	int QueryElevation(const SearchmapPoint& p) const noexcept;
	Color QueryLighting(const SearchmapPoint& p) const noexcept;

	void PaintSearchMap(const SearchmapPoint&, PathMapFlags value) noexcept;
	void PaintSearchMap(const SearchmapPoint& p, uint16_t blocksize, PathMapFlags value) noexcept;
};

class GEM_EXPORT OwningTileProps : public TileProps {
public:
	static OwningTileProps CopyFrom(const TileProps& tileProps);

	OwningTileProps() noexcept = default;
	OwningTileProps(OwningTileProps&& other) noexcept;
	OwningTileProps(const OwningTileProps& other) noexcept;
	OwningTileProps& operator=(const OwningTileProps& other) noexcept;
	OwningTileProps& operator=(OwningTileProps&& other) noexcept;

	~OwningTileProps() override;

private:
	// bytes currently allocated at propPtr
	size_t bytesSize = 0;

	explicit OwningTileProps(const TileProps& tileProps);
};
}

#endif //TILEPROPS_H
