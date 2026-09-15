// SPDX-FileCopyrightText: 2025 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef BUCKETPRIORITYQUEUE_H
#define BUCKETPRIORITYQUEUE_H


#include "OccupancyBitmap.h"
#include "Region.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <limits>
#include <vector>

namespace GemRB {

/**
 * The 'open set' for PathFinder::FindPath().
 * Answers the question: "which node is the cheapest one", by filing each node in a bucket indexed
 * by its cost, so that only one bucket has to be examined.
 *
 * Two invariants:
 * 1. Pop() returns the cheapest queued node, exactly. FindPath() never reopens a closed node, so
 * the cost a node is closed with is final; returning a more expensive node early leaves its `g`
 * unimproved and the reconstructed route detours.
 *
 * 2. A cell is in the queue at most once. A second push moves the existing entry (its cost may only
 * fall). This bounds occupancy by the number of searchmap cells, lets every array be sized from the map,
 * and means Pop() never returns a stale entry that would re-expand a closed cell.
 *
 * Storage is one pool of fixed-size chunks handed to whichever bucket needs one, rather than a
 * fixed slice per bucket, so buckets can be sized from what they actually hold. Costs live in
 * their own array because Pop()'s scan reads nothing else.
 */
class BucketPriorityQueue {
public:
	BucketPriorityQueue()
	{
		bucketHead.fill(-1);
	}

	/** Sizes the queue for a map of `cellCount` searchmap tiles, no-op if unchanged. */
	void Reserve(const size_t cellCount)
	{
		if (cellCount == cells) return;

		cells = cellCount;

		// How many entry slots Reserve() must allocate. At most one live entry exists per cell,
		// so cellCount is the most entries ever queued at once.
		//
		// The chunked storage needs more slots than that. An entry occupies a slot in a chunk,
		// and a bucket holds a chain of chunks, in which every chunk except the head is full
		// (an emptied chunk is returned to the pool).
		// So the only wasted space is one partly filled head chunk per non-empty bucket, which
		// is at most CHUNK_SIZE - 1 slots each.
		// There can be no more non-empty buckets than live entries, nor more than BUCKETS_COUNT
		// in total, giving:
		//
		//     slots <= cellCount + (CHUNK_SIZE - 1) * min(cellCount, BUCKETS_COUNT)
		const size_t worstCaseSlots = cellCount + size_t(CHUNK_SIZE - 1) * std::min(cellCount, size_t(BUCKETS_COUNT));
		const size_t chunkCount = (worstCaseSlots + CHUNK_SIZE - 1) / CHUNK_SIZE;

		costs.resize(chunkCount * CHUNK_SIZE);
		entries.resize(chunkCount * CHUNK_SIZE);
		chunkNext.resize(chunkCount);
		chunkFill.resize(chunkCount);
		chunkPoolSize = static_cast<int32_t>(chunkCount);

		// gen 0 is never live (Clear() pre-increments), marking every cell as "not queued"
		// without a sentinel per field.
		cellState.assign(cellCount, CellSlot {});
		searchGen = 0;

		Clear();
	}

	/**
	 * Enqueues `point` (the tile centre of searchmap cell `cell`) at `cost`. If an entry for `cell`
	 * is already queued, moves it to `cost` instead of enqueueing a second one.
	 *
	 * `cell` is the caller's own row-major cell index (the same one it uses for its per-cell
	 * arrays); the queue keys on it to deduplicate.
	 */
	void Push(const Point& point, const uint32_t cell, const uint32_t cost)
	{
		assert(cell < cells && "BucketPriorityQueue: cell index outside the reserved map");
		const int32_t bucketIdx = BucketOf(cost);

		CellSlot& state = cellState[cell];
		if (state.gen == searchGen && state.slot >= 0) {
			// Already queued, move it.
			const int32_t slot = state.slot;
			const int32_t oldBucketIdx = BucketOf(costs[slot]);
			if (oldBucketIdx == bucketIdx) {
				// Same bucket: only the key changes; Pop() scans for the minimum inside a bucket.
				costs[slot] = cost;
				return;
			}
			Detach(slot, oldBucketIdx);
			state.slot = Insert(bucketIdx, point, cell, cost);
		} else {
			state.gen = searchGen;
			state.slot = Insert(bucketIdx, point, cell, cost);
			++count;
		}

		minBucket = std::min(minBucket, bucketIdx);
	}

	/** The cheapest queued point. Undefined on an empty queue - check IsEmpty() first. */
	Point Pop()
	{
		--count;
		minBucket = static_cast<int32_t>(occupancy.NextSetFrom(static_cast<uint32_t>(minBucket)));
		assert(minBucket < BUCKETS_COUNT && "Pop() on an empty queue");

		// Linear scan for the true minimum inside the bucket. Several distinct fixed-point costs
		// share a bucket (see BPQ_COST_SHIFT), and every one of them is cheaper than anything in a
		// higher bucket, so this is the whole of the ordering work.
		int32_t bestSlot = -1;
		uint32_t bestCost = std::numeric_limits<uint32_t>::max();
		for (int32_t chunk = bucketHead[minBucket]; chunk >= 0; chunk = chunkNext[chunk]) {
			const int32_t base = chunk << CHUNK_SHIFT;
			const int32_t fill = chunkFill[chunk];
			for (int32_t i = 0; i < fill; ++i) {
				const uint32_t cost = costs[base + i];
				if (cost < bestCost) {
					bestCost = cost;
					bestSlot = base + i;
				}
			}
		}
		assert(bestSlot >= 0 && "an occupied bucket with no entries in it");

		const Entry entry = entries[bestSlot];
		// Mark not-queued before Detach(), which may move a different entry into this slot and
		// claim the slot for *that* cell.
		cellState[entry.cell].slot = -1;
		Detach(bestSlot, minBucket);

		return entry.point;
	}

	bool IsEmpty() const
	{
		return count <= 0;
	}

	void Clear()
	{
		count = 0;
		minBucket = BUCKETS_COUNT;

		// Bumping the generation makes every cellState entry read as "not queued" without
		// touching it. A wrap costs one memset of the whole index.
		++searchGen;
		if (searchGen == 0) {
			std::fill(cellState.begin(), cellState.end(), CellSlot {});
			++searchGen;
		}

		// The chunk pool is a bump pointer plus a free list, so returning all of it is two stores.
		chunkBump = 0;
		chunkFree = -1;

		if (dirtyLo <= dirtyHi) {
			// zero is a valid index, must clear with 0xFF (-1)
			const size_t clearedBytes = static_cast<size_t>(dirtyHi - dirtyLo + 1) * sizeof(bucketHead[0]);
			std::memset(bucketHead.data() + dirtyLo, 0xFF, clearedBytes);
			occupancy.ClearRange(static_cast<uint32_t>(dirtyLo), static_cast<uint32_t>(dirtyHi));
		}
		dirtyLo = BUCKETS_COUNT;
		dirtyHi = -1;
	}

private:
	// Cost (fixed-point, 1/COST_SCALE tile - see StepCost() in PathFinder.cpp) to bucket-units
	// shift: 64 units, a quarter tile, per bucket. Fine enough that few distinct costs share a
	// bucket, and Reserve()'s pool does not grow with BUCKETS_COUNT.
	constexpr static uint32_t BPQ_COST_SHIFT = 6;

	// Spans BUCKETS_COUNT << BPQ_COST_SHIFT = 13312000 fixed-point units, about 52000 tiles,
	// covering the worst cost measured on any scenario. Overshooting that shares the last bucket,
	// which is still ordered correctly against the rest, just scanned together.
	constexpr static int32_t BUCKETS_COUNT = 208000;

	// Entries per chunk. Pop() scans costs, so a bigger chunk means fewer links to follow, but it
	// is also the pool's slack - Reserve() reserves CHUNK_SIZE * cellCount slots worst case - so
	// this weighs scan locality against reserved storage.
	constexpr static int32_t CHUNK_SHIFT = 2;
	constexpr static int32_t CHUNK_SIZE = 1 << CHUNK_SHIFT;

	struct Entry {
		Point point;
		uint32_t cell;
	};

	// cell -> slot, generation-stamped so Clear() never touches it.
	struct CellSlot {
		uint32_t gen = 0;
		int32_t slot = -1;
	};

	static constexpr int32_t BucketOf(const uint32_t cost)
	{
		return std::min(static_cast<int32_t>(cost >> BPQ_COST_SHIFT), BUCKETS_COUNT - 1);
	}

	int32_t TakeChunk()
	{
		int32_t chunk;
		if (chunkFree >= 0) {
			// Reuse before growing: a chunk just returned is the warmest memory available.
			chunk = chunkFree;
			chunkFree = chunkNext[chunk];
		} else {
			assert(chunkBump < chunkPoolSize && "BucketPriorityQueue: chunk pool exhausted, which Reserve()'s bound says cannot happen");
			chunk = chunkBump++;
		}
		chunkFill[chunk] = 0;
		return chunk;
	}

	void FreeChunk(const int32_t chunk)
	{
		chunkNext[chunk] = chunkFree;
		chunkFree = chunk;
	}

	/** Appends to `bucketIdx`'s head chunk, taking a fresh one if it is full or absent. */
	int32_t Insert(const int32_t bucketIdx, const Point& point, const uint32_t cell, const uint32_t cost)
	{
		int32_t head = bucketHead[bucketIdx];
		if (head < 0) {
			head = TakeChunk();
			chunkNext[head] = -1;
			bucketHead[bucketIdx] = head;
			occupancy.Set(static_cast<uint32_t>(bucketIdx));
			if (bucketIdx < dirtyLo) dirtyLo = bucketIdx;
			if (bucketIdx > dirtyHi) dirtyHi = bucketIdx;
		} else if (chunkFill[head] == CHUNK_SIZE) {
			const int32_t fresh = TakeChunk();
			chunkNext[fresh] = head;
			bucketHead[bucketIdx] = fresh;
			head = fresh;
		}

		const int32_t slot = (head << CHUNK_SHIFT) + chunkFill[head]++;
		costs[slot] = cost;
		entries[slot] = Entry { point, cell };
		return slot;
	}

	/**
	 * Removes the entry at `slot` from `bucketIdx` by moving the bucket's last entry into the
	 * hole - which is always in the head chunk, since that is the only partially filled one.
	 */
	void Detach(const int32_t slot, const int32_t bucketIdx)
	{
		const int32_t head = bucketHead[bucketIdx];
		assert(head >= 0 && "detaching from an empty bucket");
		const int32_t lastSlot = (head << CHUNK_SHIFT) + (--chunkFill[head]);

		if (slot != lastSlot) {
			costs[slot] = costs[lastSlot];
			entries[slot] = entries[lastSlot];
			cellState[entries[slot].cell].slot = slot;
		}

		if (chunkFill[head] == 0) {
			bucketHead[bucketIdx] = chunkNext[head];
			FreeChunk(head);
			if (bucketHead[bucketIdx] < 0) {
				occupancy.Reset(static_cast<uint32_t>(bucketIdx));
			}
		}
	}

	// Scanned by Pop(), and nothing else lives here: one chunk's costs are one cache line.
	std::vector<uint32_t> costs;
	// Touched once per pop and once per moved entry, never by the scan.
	std::vector<Entry> entries;

	std::vector<int32_t> chunkNext; // chain link in a bucket, or free-list link
	std::vector<uint8_t> chunkFill; // entries used, 0..CHUNK_SIZE
	int32_t chunkPoolSize = 0;
	int32_t chunkBump = 0; // chunks never yet handed out
	int32_t chunkFree = -1; // head of the free list

	std::vector<CellSlot> cellState;
	size_t cells = 0;
	uint32_t searchGen = 0;

	// Head chunk of each bucket, -1 for empty
	std::array<int32_t, BUCKETS_COUNT> bucketHead;

	// [dirtyLo, dirtyHi]: buckets Insert() has touched since the last Clear(), so Clear() memsets
	// that range instead of the whole array. Empty is dirtyLo > dirtyHi.
	int32_t dirtyLo = BUCKETS_COUNT;
	int32_t dirtyHi = -1;

	// Which buckets are non-empty, so Pop() can jump to the lowest one instead of walking to it.
	OccupancyBitmap<BUCKETS_COUNT> occupancy;

	int32_t minBucket = BUCKETS_COUNT;
	int32_t count = 0;
};

}
#endif // BUCKETPRIORITYQUEUE_H
