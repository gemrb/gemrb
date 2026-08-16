// SPDX-FileCopyrightText: 2025 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef TRAVERSABILITY_CACHE_H
#define TRAVERSABILITY_CACHE_H


#include "exports.h"

#include "FixedSizePool.h"
#include "PagedSparseArray.h"
#include "Region.h"

#include <array>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <vector>


namespace GemRB {

class Actor;

/**
 * This class manages the cached data of actors on a navmap, to be used for speed up the FindPath implementation.
 */
class GEM_EXPORT TraversabilityCache {
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

		// Pads `state` out to the pointer alignment boundary so the compiler inserts no implicit
		// padding of its own
		char padding[7] = {};

		bool operator!=(const TraversabilityCellData& other) const noexcept
		{
			return occupyingActor != other.occupyingActor || state != other.state;
		}
	};

	// Guards the padding above: if the members ever stop summing to the object size, the compiler
	// has inserted padding this type does not control. Written as a sum rather than a literal so
	// it holds for both 32- and 64-bit pointers.
	static_assert(sizeof(TraversabilityCellData) ==
			      sizeof(Actor*) + sizeof(TraversabilityCellState) + sizeof(TraversabilityCellData::padding),
		      "TraversabilityCellData has implicit padding; adjust its padding member, or store it "
		      "in a PagedSparseArray instantiated with DefaultTIsAllZeroBytes = false");

	using Data_t = PagedSparseArray<TraversabilityCellData, true>;

	explicit TraversabilityCache(class Map* inMap)
		: map { inMap }, traversabilityData(dataAllocator)
	{
	}

	TraversabilityCellData GetCellData(const std::size_t inIndex) const
	{
		return traversabilityData[inIndex];
	}

	/** The cache's backing store, handed to PathFinderScheduler::Sync() as a SyncFrom() source. */
	Data_t& GetData()
	{
		return traversabilityData;
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

	/**
	 * Rebuilds the cache from the map's current actors.
	 *
	 * Does the work at most once per frame: the first call after MarkNewFrame() rebuilds, every
	 * later one in the same frame returns immediately.
	 *
	 * @return true only from the frame's first call, and only when an actor was added, removed or
	 *         moved. Every later call in the same frame returns false whether or not the cache
	 *         changed.
	 */
	bool Update();

	TraversabilityCache(const TraversabilityCache& other) = delete;
	TraversabilityCache(TraversabilityCache&& other) = delete;
	TraversabilityCache& operator=(const TraversabilityCache& other) = delete;
	TraversabilityCache& operator=(TraversabilityCache&& other) = delete;
	TraversabilityCache() = delete;

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
		std::vector<uint8_t> sizeCategory;

		explicit CachedActorsState(size_t reserve);

		void reserve(size_t reserve);

		void clear();

		void erase(size_t idx);

		size_t AddCachedActorState(Actor* inActor);

		void ClearOldPosition(size_t i, Data_t& inOutTraversabilityData, int inWidth) const;

		void MarkNewPosition(size_t i, Data_t& inOutTraversabilityData, int inWidth, bool inShouldUpdateSelf = false);

		void UpdateNewState(size_t i);

		void emplace_back(CachedActorsState&& another);

		static FitRegion CalculateRegion(const Actor* inActor);

		// flags manipulation should be inlined
		void SetIsBumpable(const size_t i, const bool isBumpable)
		{
			flags[i] = (flags[i] & ~(1 << FLAG_BUMPABLE)) | (static_cast<uint8_t>(isBumpable) << FLAG_BUMPABLE);
		}

		void SetIsAlive(const size_t i, const bool isAlive)
		{
			flags[i] = (flags[i] & ~(1 << FLAG_ALIVE)) | (static_cast<uint8_t>(isAlive) << FLAG_ALIVE);
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

	Map* map = nullptr;
	// declaration order matters: traversabilityData holds a reference to dataAllocator, so the
	// allocator has to outlive it on both construction and destruction
	FixedSizePool<Data_t::TPage_t> dataAllocator;
	Data_t traversabilityData;
	CachedActorsState cachedActorsState { 0 };
	bool hasBeenUpdatedThisFrame { false };

	void ValidateTraversabilityCacheSize();

	// BlockingShapeCache could have been a map of (actor's size category)->(blocking shape),
	// but it's deliberately not a map; actors' size categories usually range from 0-3 (large creatures, e.g. dragons, having it at 7),
	// direct vector access via idx will be faster on slow HW than going through std::unordered_map buckets
	static std::vector<std::vector<bool>> BlockingShapeCache;

	static const std::vector<bool>& GetBlockingShape(const Actor* actor, uint8_t blockingSizeCategory);

	static uint16_t GetBlockingShapeRegionW(uint8_t blockingSizeCategory);

	static uint16_t GetBlockingShapeRegionH(uint8_t blockingSizeCategory);
};

/**
 *  One immutable snapshot of a map's traversability data.
 *  Owns its allocator, so the pages are freed through it wherever the last reference happens to
 *  be dropped - which may be a worker thread. Sharing one allocator across snapshots would put
 *  that free on a racing thread.
 */
struct TraversabilityDataSnapshot {
	FixedSizePool<TraversabilityCache::Data_t::TPage_t> allocator;
	TraversabilityCache::Data_t data;
	/** Which version of the source data this was taken from; see PathFinderScheduler. */
	uint64_t version = 0;

	TraversabilityDataSnapshot()
		: data(allocator) {}

	TraversabilityDataSnapshot(const TraversabilityDataSnapshot&) = delete;
	TraversabilityDataSnapshot& operator=(const TraversabilityDataSnapshot&) = delete;
};
}

#endif
