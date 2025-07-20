/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2025 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

#ifndef TRAVERSABILITY_CACHE_H
#define TRAVERSABILITY_CACHE_H

//#include "Map.h"
#include "Scriptable/Actor.h"


namespace GemRB {


struct FitRegion {
	Point origin;
	Size size;

	FitRegion(const Point& InOrigin, const Size& InSize)
		: origin(InOrigin), size(InSize)
	{
	}

	FitRegion(const Region& r) noexcept
		: origin(r.origin), size(r.size)
	{
	}

	FitRegion() noexcept = default;

	FitRegion(const FitRegion& other)
		: origin(other.origin),
		  size(other.size)
	{
	}

	FitRegion(FitRegion&& other) noexcept
		: origin(std::move(other.origin)),
		  size(std::move(other.size))
	{
	}

	FitRegion& operator=(const FitRegion& other)
	{
		if (this == &other)
			return *this;
		origin = other.origin;
		size = other.size;
		return *this;
	}

	FitRegion& operator=(FitRegion&& other) noexcept
	{
		if (this == &other)
			return *this;
		origin = std::move(other.origin);
		size = std::move(other.size);
		return *this;
	}

	bool IntersectsRegion(const FitRegion& rgn) const noexcept
	{
		if (origin.x >= (rgn.origin.x + rgn.size.w)) {
			return false; // entirely to the right of rgn
		}
		if ((origin.x + size.w) <= rgn.origin.x) {
			return false; // entirely to the left of rgn
		}
		if (origin.y >= (rgn.origin.y + rgn.size.h)) {
			return false; // entirely below rgn
		}
		if ((origin.y + size.h) <= rgn.origin.y) {
			return false; // entirely above rgn
		}
		return true;
	}
};


/**
 * This class manages the cached data of actors on a navmap, to be used for speed up the FindPath implementation.
 */
class TraversabilityCache {
public:
	using TraversabilityCellState = uint8_t;
	static constexpr TraversabilityCellState TraversabilityCellValueEmpty = 0;
	static constexpr TraversabilityCellState TraversabilityCellValueActor = 1;
	static constexpr TraversabilityCellState TraversabilityCellValueActorNonTraversable = 15;

	/**
	 * Struct holding data describing traversability of a navmap point: its state and potential actor data.
	 */
	struct TraversabilityCellData {
		Actor* occupyingActor = nullptr;
		TraversabilityCellState state = TraversabilityCellValueEmpty;
	};

	explicit TraversabilityCache(class Map* inMap)
		: map { inMap }, cachedActorsState(0)
	{
	}

	TraversabilityCellData GetCellData(const std::size_t inIndex) const
	{
		return traversabilityData[inIndex];
	}

	bool HasUpdatedTraversabilityThisFrame() const
	{
		return hasBeenUpdatedThisFrame;
	}

	void MarkNewFrame()
	{
		hasBeenUpdatedThisFrame = false;
	}

	size_t Size() const
	{
		return traversabilityData.size();
	}

	void Update();

private:
	/**
	 * Struct for storing cached state of actors on the map: position, occupied region on the navmap,
	 * bumpable state and alive state.
	 */
	struct CachedActorsState {
		constexpr static uint8_t FLAG_BUMPABLE = 1;
		constexpr static uint8_t FLAG_ALIVE = 2;

		std::vector<FitRegion> region;
		std::vector<Actor*> actor;
		std::vector<Point> pos;
		std::vector<uint8_t> flags;
		std::vector<Actor::BlockingSizeCategory> sizeCategory;

		explicit CachedActorsState(size_t reserve);

		void reserve(size_t reserve);

		void clear();

		void erase(size_t idx);

		size_t AddCachedActorState(Actor* inActor);

		void ClearOldPosition(size_t i, std::vector<TraversabilityCellData>& inOutTraversabilityData, int inWidth) const;

		void MarkNewPosition(size_t i, std::vector<TraversabilityCellData>& inOutTraversabilityData, int inWidth, bool inShouldUpdateSelf = false);

		void UpdateNewState(size_t i);

		void emplace_back(CachedActorsState&& another);

		static FitRegion CalculateRegion(const Actor* inActor);

		// flags manipulation should be inlined
		void SetIsBumpable(const size_t i, const bool isBumpable)
		{
			flags[i] = (flags[i] & ~(1 << FLAG_BUMPABLE)) | (isBumpable << FLAG_BUMPABLE);
		}

		void SetIsAlive(const size_t i, const bool isAlive)
		{
			flags[i] = (flags[i] & ~(1 << FLAG_ALIVE)) | (isAlive << FLAG_ALIVE);
		}

		bool GetIsBumpable(const size_t i) const
		{
			return flags[i] & (1 << FLAG_BUMPABLE);
		}

		bool GetIsAlive(const size_t i) const
		{
			return flags[i] & (1 << FLAG_ALIVE);
		}

		TraversabilityCellState GetCellStateFromFlags(size_t i) const;
	};

	Map* map;
	std::vector<TraversabilityCellData> traversabilityData;
	CachedActorsState cachedActorsState;
	bool hasBeenUpdatedThisFrame { false };

	void ValidateTraversabilityCacheSize();

	// this could have been a map of (actor's size category)->(blocking shape),
	// but it's deliberately not a map; actors' size categories usually range from 0-3 (large creatures, e.g. dragons, having it at 7),
	// direct vector access via idx will be faster on slow HW than going through std::unordered_map buckets
	static std::vector<std::vector<bool>> BlockingShapeCache;

	static const std::vector<bool>& GetBlockingShape(const Actor* actor, Actor::BlockingSizeCategory blockingSizeCategory);

	static uint16_t GetBlockingShapeRegionW(const Actor::BlockingSizeCategory& blockingSizeCategory, float sizeFactor);

	static uint16_t GetBlockingShapeRegionH(const Actor::BlockingSizeCategory& blockingSizeCategory, float sizeFactor);
};
}

#endif
