// SPDX-FileCopyrightText: 2025 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef TRAVERSABILITY_CACHE_H
#define TRAVERSABILITY_CACHE_H


#include "exports.h"

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
 * This class manages the cached data of actors on a searchmap, to be used for speed up the FindPath implementation.
 */
class GEM_EXPORT TraversabilityCache {
public:
	// There can be more than one actor occupying a searchmap cell, the cache must be able
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
	 * The searchmap tiles an actor of a given size covers, centred on its position. Shared by the
	 * cache when it stamps or unstamps an actor and by FindPath when it decides whether a cell is
	 * the mover's own, so the two cannot drift apart.
	 */
	struct ActorFootprint {
		int firstX = 0;
		int lastX = -1;
		int firstY = 0;
		int lastY = -1;
		Point centre;
		int ellipseR = 0;

		bool covers(const SearchmapPoint& tile) const noexcept
		{
			// unsigned wrap makes each two-sided bounds test branchless
			if (static_cast<unsigned>(tile.x - firstX) > static_cast<unsigned>(lastX - firstX)) return false;
			if (static_cast<unsigned>(tile.y - firstY) > static_cast<unsigned>(lastY - firstY)) return false;
			if (ellipseR < 1) return true;
			return tile.ToNavmapCenter().IsWithinEllipse(ellipseR, centre);
		}
	};

	// One cell per searchmap tile, holding the token sum of the actors covering it.
	using Data_t = std::vector<TraversabilityCellState>;

	explicit TraversabilityCache(class Map* inMap)
		: map { inMap }
	{
	}

	/** The cache's backing store, copied by PathFinderScheduler::Sync() into its per-map entry. */
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
	 * Cached state of one tracked actor: position, bumpable/alive flags and size.
	 */
	struct CachedActorsState {
		constexpr static uint8_t FLAG_BUMPABLE = 1;
		constexpr static uint8_t FLAG_ALIVE = 2;

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
	Data_t traversabilityData;
	CachedActorsState cachedActorsState { 0 };
	bool hasBeenUpdatedThisFrame { false };

	void ValidateTraversabilityCacheSize();
};

/** The footprint of an actor of this circle size standing at this position. */
GEM_EXPORT TraversabilityCache::ActorFootprint ComputeActorFootprint(const Point& pos, int circleSize, int mapWidth, int mapHeight);

/**
 *  One immutable snapshot of a map's traversability data, shared by reference between workers.
 */
struct TraversabilityDataSnapshot {
	TraversabilityCache::Data_t data;
	/** Which version of the source data this was taken from; see PathFinderScheduler. */
	uint64_t version = 0;

	TraversabilityDataSnapshot() = default;
	TraversabilityDataSnapshot(const TraversabilityDataSnapshot&) = delete;
	TraversabilityDataSnapshot& operator=(const TraversabilityDataSnapshot&) = delete;
};
}

#endif
