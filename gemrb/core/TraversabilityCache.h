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


/**
 * This class manages the cached data of actors on a navmap, to be used for speed up the FindPath implementation.
 */
class TraversabilityCache {
public:
	// There can be more than one actor occupying a navmap cell, the cache must be able
	// to represent more than one traversability value per cell at a time.
	//
	// We will use a token strategy:
	// Upon entering a cell, each actor will put its token to the cell - increase the cell's
	// numerical value by (1) if being bumpable and (15) if not being bumpable.
	// The same value is subtracted when leaving a cell (token removal).
	//
	// If a cell has value of 0, it means no actor is occupying it.
	// If a cell has value between 1 and 14, it means there are only bumpable
	// actors.
	// If a cell crosses the threshold of 15, it means there is at least one
	// non-bumpable actor.
	//
	// This gives us ability to properly store 14 bumpable actors and 16
	// non-bumpable actors in one uint8_t cell - more than plenty enough.
	//
	// Treating it as tokens also removes the need of re-drawing neighbouring
	// actors on clearing previous position, as now we don't blindly erase the cells' state,
	// but maintaining it in a more meaningful fashion.

	using TraversabilityCellState = uint8_t;
	static constexpr TraversabilityCellState TraversabilityCellValueEmpty = 0;
	static constexpr TraversabilityCellState TraversabilityCellValueActor = 1;
	static constexpr TraversabilityCellState TraversabilityCellValueActorNonTraversable = 15;

	/**
	 * Struct representing Region, but with reduced data size.
	 * We don't use the regular GemRG::Region class because of its footprint:
	 * currently it weights 48 bytes, while we can work totally fine with 6 bytes.
	 * In TraversabilityCache, we don't pay as much attention to memory size, but
	 * this reduces CPU data cache pressure by a factor of 8 per single loaded region.
	 */
	struct FitRegion {
		uint16_t x;
		uint16_t y;
		uint8_t w;
		uint8_t h;

		FitRegion(const Point& InOrigin, const Size& InSize)
			: x(InOrigin.x), y(InOrigin.y), w(InSize.w), h(InSize.h)
		{
		}
	};

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
	 * bumpable and alive states and their size category.
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

	// BlockingShapeCache could have been a map of (actor's size category)->(blocking shape),
	// but it's deliberately not a map; actors' size categories usually range from 0-3 (large creatures, e.g. dragons, having it at 7),
	// direct vector access via idx will be faster on slow HW than going through std::unordered_map buckets
	static std::vector<std::vector<bool>> BlockingShapeCache;

	static const std::vector<bool>& GetBlockingShape(const Actor* actor, Actor::BlockingSizeCategory blockingSizeCategory);

	static uint16_t GetBlockingShapeRegionW(Actor::BlockingSizeCategory blockingSizeCategory);

	static uint16_t GetBlockingShapeRegionH(Actor::BlockingSizeCategory blockingSizeCategory);
};
}

#endif
