// SPDX-FileCopyrightText: 2021 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "TileProps.h"

#include "globals.h"

#include "Geometry.h"
#include "Palette.h"
#include "Sprite2D.h"

#include "Logging/Logging.h"

#include <cassert>
#include <cstdlib>
#include <cstring>
#include <utility>

namespace GemRB {

const PixelFormat TileProps::pixelFormat(0, 0, 0, 0,
					 searchMapShift, materialMapShift,
					 heightMapShift, lightMapShift,
					 searchMapMask, materialMapMask,
					 heightMapMask, lightMapMask,
					 4, 32, 0, false, false, nullptr);

TileProps::TileProps(Holder<Sprite2D> props) noexcept
	: propImage(std::move(props))
{
	propPtr = static_cast<uint32_t*>(propImage->LockSprite());
	size = propImage->Frame.size;

	assert(propImage->Format().Bpp == 4);
	assert(propImage->GetPitch() == size.w * 4);
}

const Size& TileProps::GetSize() const noexcept
{
	return size;
}

void TileProps::SetTileProp(const SearchmapPoint& p, Property prop, uint8_t val) noexcept
{
	if (!size.PointInside(p)) {
		return;
	}

	uint32_t& c = propPtr[p.y * size.w + p.x];
	switch (prop) {
		case Property::SEARCH_MAP:
			c &= ~searchMapMask;
			c |= val << searchMapShift;
			break;
		case Property::MATERIAL:
			c &= ~materialMapMask;
			c |= val << materialMapShift;
			break;
		case Property::ELEVATION:
			c &= ~heightMapMask;
			c |= val << heightMapShift;
			break;
		case Property::LIGHTING:
			c &= ~lightMapMask;
			c |= val << lightMapShift;
			break;
	}
}

uint8_t TileProps::QueryTileProp(const SearchmapPoint& p, Property prop) const noexcept
{
	if (size.PointInside(p)) {
		const uint32_t& c = propPtr[p.y * size.w + p.x];
		switch (prop) {
			case Property::SEARCH_MAP:
				return (c & searchMapMask) >> searchMapShift;
			case Property::MATERIAL:
				return (c & materialMapMask) >> materialMapShift;
			case Property::ELEVATION:
				return (c & heightMapMask) >> heightMapShift;
			case Property::LIGHTING:
				return (c & lightMapMask) >> lightMapShift;
		}
	}
	switch (prop) {
		case Property::SEARCH_MAP:
			return defaultSearchMap;
		case Property::MATERIAL:
			return defaultMaterial;
		case Property::ELEVATION:
			return defaultElevation;
		case Property::LIGHTING:
			return defaultLighting;
	}
	return -1;
}

PathMapFlags TileProps::QuerySearchMap(const SearchmapPoint& p) const noexcept
{
	return static_cast<PathMapFlags>(QueryTileProp(p, Property::SEARCH_MAP));
}

uint8_t TileProps::QueryMaterial(const SearchmapPoint& p) const noexcept
{
	return QueryTileProp(p, Property::MATERIAL);
}

int TileProps::QueryElevation(const SearchmapPoint& p) const noexcept
{
	// Heightmaps are greyscale images where the top of the world is white and the bottom is black.
	// this covers the range -7 – +7
	// since the image is grey we can use any channel for the mapping
	int val = QueryTileProp(p, Property::ELEVATION);
	constexpr int input_range = 255;
	constexpr int output_range = 14;
	return val * output_range / input_range - 7;
}

Color TileProps::QueryLighting(const SearchmapPoint& p) const noexcept
{
	// an OwningTileProps copy carries the pixels but not the source image, so there is no palette
	// to resolve the index against
	if (!propImage) {
		return Color();
	}
	uint8_t val = QueryTileProp(p, Property::LIGHTING);
	return propImage->GetPalette()->GetColorAt(val);
}

void TileProps::PaintSearchMap(const SearchmapPoint& p, PathMapFlags value) noexcept
{
	if (!size.PointInside(p)) {
		return;
	}

	uint32_t& pixel = propPtr[p.y * size.w + p.x];
	pixel = (pixel & ~searchMapMask) | (uint32_t(value) << pixelFormat.Rshift);
}

// Valid values are - PathMapFlags::UNMARKED, PathMapFlags::PC, PathMapFlags::NPC
void TileProps::PaintSearchMap(const SearchmapPoint& p, uint16_t blocksize, const PathMapFlags value) noexcept
{
	// We block a circle of radius size-1 around (px,py)
	// TODO: recheck that this matches originals
	// these circles are perhaps slightly different for sizes 6 and up.

	// Note: this is a larger circle than the one tested in GetBlocked.
	// This means that an actor can get closer to a wall than to another
	// actor. This matches the behaviour of the original BG2.

	auto PaintIfPassable = [this, value](const SearchmapPoint& pos) {
		PathMapFlags mapval = QuerySearchMap(pos);
		if (mapval != PathMapFlags::IMPASSABLE) {
			PathMapFlags newVal = (mapval & PathMapFlags::NOTACTOR) | value;
			uint32_t& pixel = propPtr[pos.y * size.w + pos.x];
			pixel = (pixel & ~searchMapMask) | (uint32_t(newVal) << pixelFormat.Rshift);
		}
	};

	blocksize = Clamp<uint16_t>(blocksize, 1, MAX_CIRCLESIZE);
	uint16_t r = blocksize - 1;

	const auto points = PlotCircle(p, r);
	for (size_t i = 0; i < points.size(); i += 2) {
		const BasePoint& p1 = points[i];
		const BasePoint& p2 = points[i + 1];
		assert(p1.y == p2.y);
		assert(p2.x <= p1.x);

		for (int x = p2.x; x <= p1.x; ++x) {
			PaintIfPassable(SearchmapPoint(x, p1.y));
		}
	}
}

OwningTileProps OwningTileProps::CopyFrom(const TileProps& tileProps)
{
	return OwningTileProps { tileProps };
}

OwningTileProps::OwningTileProps(OwningTileProps&& other) noexcept
{
	*this = std::move(other);
}

OwningTileProps::OwningTileProps(const OwningTileProps& other) noexcept
	: TileProps() // non-copy c-tor on purpose
{
	*this = other;
}

OwningTileProps& OwningTileProps::operator=(const OwningTileProps& other) noexcept
{
	if (this == &other) {
		return *this;
	}

	// grow only: a buffer that is already large enough is reused, and bytesSize keeps describing
	// what is allocated rather than what was copied, so the destructor still frees all of it
	if (bytesSize < other.bytesSize) {
		free(propPtr);
		propPtr = static_cast<uint32_t*>(malloc(other.bytesSize));
		bytesSize = other.bytesSize;
	}
	size = other.size;
	propImage = nullptr;

	if (other.bytesSize > 0) {
		// pathfinding cannot proceed without its terrain data, and there is nothing sensible to
		// fall back to, so a failed allocation is fatal
		if (propPtr == nullptr) {
			error("TileProps", "Failed to allocate {} bytes for a tile properties buffer.", other.bytesSize);
		}
		memcpy(propPtr, other.propPtr, other.bytesSize);
	}
	return *this;
}

OwningTileProps& OwningTileProps::operator=(OwningTileProps&& other) noexcept
{
	if (this == &other) {
		return *this;
	}

	if (propPtr && bytesSize > 0) {
		free(propPtr);
	}

	propPtr = other.propPtr;
	size = other.size;
	bytesSize = other.bytesSize;
	propImage = nullptr;

	other.propPtr = nullptr;
	other.propImage = nullptr;
	other.size.reset();
	return *this;
}

OwningTileProps::~OwningTileProps()
{
	if (propPtr && bytesSize > 0) {
		free(propPtr);
	}
}

OwningTileProps::OwningTileProps(const TileProps& tileProps)
{
	bytesSize = pixelFormat.Bpp * tileProps.GetSize().Area();
	size = tileProps.GetSize();
	propImage = nullptr;

	if (bytesSize > 0) {
		propPtr = static_cast<uint32_t*>(malloc(bytesSize));
		// pathfinding cannot proceed without its terrain data, and there is nothing sensible to
		// fall back to, so a failed allocation is fatal
		if (propPtr == nullptr) {
			error("TileProps", "Failed to allocate {} bytes for a tile properties buffer.", bytesSize);
		}
		memcpy(propPtr, tileProps.GetPropPtr(), bytesSize);
	}
}
}
