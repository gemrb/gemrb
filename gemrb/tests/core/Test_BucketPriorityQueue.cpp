// SPDX-FileCopyrightText: 2026 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

// Tests for BucketPriorityQueue correctness; important, as a queue defect shows up in found paths as a detour

#include "../../core/BucketPriorityQueue.h"

#include <gtest/gtest.h>
#include <random>
#include <set>
#include <vector>

namespace GemRB {
namespace test {
	namespace {

		// Each distinct point needs its own cell index; the queue deduplicates by cell.
		constexpr size_t TestCells = 65536;

		BucketPriorityQueue& FreshQueue()
		{
			static BucketPriorityQueue queue;
			queue = BucketPriorityQueue();
			queue.Reserve(TestCells);
			return queue;
		}

		// The queue returns only the point, so map each cell to a point whose x identifies it.
		SearchmapPoint PointForCell(uint32_t cell)
		{
			return SearchmapPoint(int(cell), 0);
		}

	} // namespace

	// 300 entries at one cost, all in one bucket: more than a single chunk holds.
	TEST(BucketPriorityQueueTest, SurvivesManySameCostPushes)
	{
		BucketPriorityQueue& queue = FreshQueue();

		constexpr uint32_t N = 300;
		constexpr uint32_t cost = 5;
		std::set<int> pushedX;
		for (uint32_t i = 0; i < N; ++i) {
			queue.Push(PointForCell(i), i, cost);
			pushedX.insert(int(i));
		}
		ASSERT_EQ(pushedX.size(), size_t(N));

		std::set<int> poppedX;
		uint32_t pops = 0;
		while (!queue.IsEmpty() && pops < 4 * N) {
			const SearchmapPoint p = queue.Pop();
			poppedX.insert(p.x);
			++pops;
		}

		EXPECT_EQ(pops, N) << "IsEmpty() should go true after exactly " << N
				   << " pops - it either ran dry early or kept reporting non-empty past that";
		EXPECT_EQ(poppedX, pushedX) << "some of the " << N << " pushed points were never popped back out intact";
	}

	// One bucket holding 3000 entries: a long chunk chain.
	TEST(BucketPriorityQueueTest, SurvivesManySameCostPushesAtScale)
	{
		BucketPriorityQueue& queue = FreshQueue();

		constexpr uint32_t N = 3000;
		constexpr uint32_t cost = 42;
		std::set<int> pushedX;
		for (uint32_t i = 0; i < N; ++i) {
			queue.Push(PointForCell(i), i, cost);
			pushedX.insert(int(i));
		}

		std::set<int> poppedX;
		uint32_t pops = 0;
		while (!queue.IsEmpty() && pops < 4 * N) {
			const SearchmapPoint p = queue.Pop();
			poppedX.insert(p.x);
			++pops;
		}

		EXPECT_EQ(pops, N);
		EXPECT_EQ(poppedX, pushedX);
	}

	// A bucket holding far more than one chunk must not disturb the bucket next to it.
	TEST(BucketPriorityQueueTest, OverfullBucketDoesNotCorruptItsNeighbour)
	{
		BucketPriorityQueue& queue = FreshQueue();

		constexpr uint32_t overfull = 400;
		constexpr uint32_t neighbour = 10;
		std::set<int> pushedX;
		for (uint32_t i = 0; i < overfull; ++i) {
			queue.Push(PointForCell(i), i, 0);
			pushedX.insert(int(i));
		}
		for (uint32_t i = 0; i < neighbour; ++i) {
			queue.Push(PointForCell(1000 + i), 1000 + i, 1024);
			pushedX.insert(int(1000 + i));
		}

		std::set<int> poppedX;
		uint32_t pops = 0;
		const uint32_t total = overfull + neighbour;
		while (!queue.IsEmpty() && pops < 4 * total) {
			const SearchmapPoint p = queue.Pop();
			poppedX.insert(p.x);
			++pops;
		}

		EXPECT_EQ(pops, total);
		EXPECT_EQ(poppedX, pushedX);
	}

	// Costs over many buckets, pushed uncorrelated with cost: Pop() must return them in
	// non-decreasing order.
	TEST(BucketPriorityQueueTest, PopsInExactCostOrder)
	{
		BucketPriorityQueue& queue = FreshQueue();

		constexpr uint32_t N = 20000;
		std::mt19937 rng(20260317);
		std::uniform_int_distribution<uint32_t> costs(0, 400000); // ~390 buckets at 1024 units

		std::vector<uint32_t> pushedCosts(N);
		for (uint32_t i = 0; i < N; ++i) {
			pushedCosts[i] = costs(rng);
			queue.Push(PointForCell(i), i, pushedCosts[i]);
		}

		std::vector<uint32_t> expected = pushedCosts;
		std::sort(expected.begin(), expected.end());

		std::vector<uint32_t> popped;
		popped.reserve(N);
		while (!queue.IsEmpty()) {
			const SearchmapPoint p = queue.Pop();
			ASSERT_GE(p.x, 0);
			ASSERT_LT(uint32_t(p.x), N);
			popped.push_back(pushedCosts[uint32_t(p.x)]);
		}

		ASSERT_EQ(popped.size(), size_t(N));
		EXPECT_EQ(popped, expected) << "Pop() did not return the cheapest queued entry every time";
	}

	// The same, with many entries packed into a few buckets, so the chain walk and the in-chunk
	// scan both do real work.
	TEST(BucketPriorityQueueTest, PopsInExactCostOrderUnderHeavyCollision)
	{
		BucketPriorityQueue& queue = FreshQueue();

		constexpr uint32_t N = 8000;
		std::mt19937 rng(20260318);
		std::uniform_int_distribution<uint32_t> costs(0, 4095); // four buckets, 2000 entries each

		std::vector<uint32_t> pushedCosts(N);
		for (uint32_t i = 0; i < N; ++i) {
			pushedCosts[i] = costs(rng);
			queue.Push(PointForCell(i), i, pushedCosts[i]);
		}

		std::vector<uint32_t> expected = pushedCosts;
		std::sort(expected.begin(), expected.end());

		std::vector<uint32_t> popped;
		popped.reserve(N);
		while (!queue.IsEmpty()) {
			const SearchmapPoint p = queue.Pop();
			popped.push_back(pushedCosts[uint32_t(p.x)]);
		}

		EXPECT_EQ(popped, expected);
	}

	// A second push for a queued cell moves its entry rather than adding one: within a bucket, to
	// another bucket, and without growing the count.
	TEST(BucketPriorityQueueTest, SecondPushForACellMovesItRatherThanDuplicating)
	{
		BucketPriorityQueue& queue = FreshQueue();

		queue.Push(PointForCell(1), 1, 5000);
		queue.Push(PointForCell(2), 2, 6000);
		queue.Push(PointForCell(3), 3, 7000);

		queue.Push(PointForCell(3), 3, 100); // to a different, much lower bucket
		queue.Push(PointForCell(1), 1, 4999); // within the same bucket

		std::vector<int> order;
		while (!queue.IsEmpty()) {
			order.push_back(queue.Pop().x);
		}

		const std::vector<int> want { 3, 1, 2 };
		EXPECT_EQ(order, want) << "a re-pushed cell should come back once, at its newest cost";
	}

	// Re-pushing a cell must not consume storage: the pool is bounded at one entry per cell.
	TEST(BucketPriorityQueueTest, RepeatedDecreaseKeysDoNotConsumeStorage)
	{
		BucketPriorityQueue& queue = FreshQueue();

		constexpr uint32_t cellCount = 50;
		constexpr uint32_t rounds = 4000;
		for (uint32_t cell = 0; cell < cellCount; ++cell) {
			queue.Push(PointForCell(cell), cell, 1000000);
		}
		for (uint32_t round = 0; round < rounds; ++round) {
			for (uint32_t cell = 0; cell < cellCount; ++cell) {
				// strictly decreasing, as FindPath()'s only pushes are improvements
				queue.Push(PointForCell(cell), cell, 1000000 - round * 100 - cell);
			}
		}

		uint32_t pops = 0;
		std::set<int> seen;
		while (!queue.IsEmpty()) {
			seen.insert(queue.Pop().x);
			++pops;
			ASSERT_LE(pops, cellCount) << "the queue held more entries than it had cells";
		}
		EXPECT_EQ(pops, cellCount);
		EXPECT_EQ(seen.size(), size_t(cellCount));
	}

	// Clear() must return every chunk to the pool and every cell to "not queued", so the same
	// queue can be reused across searches.
	TEST(BucketPriorityQueueTest, ClearFullyResetsAcrossSearches)
	{
		BucketPriorityQueue& queue = FreshQueue();

		for (int round = 0; round < 5; ++round) {
			queue.Clear();
			constexpr uint32_t N = 2000;
			for (uint32_t i = 0; i < N; ++i) {
				queue.Push(PointForCell(i), i, (i % 97) * 512);
			}
			// leave a third of them queued, so the next Clear() has live chunks to reclaim
			uint32_t pops = 0;
			uint32_t last = 0;
			while (!queue.IsEmpty() && pops < N / 3) {
				const SearchmapPoint p = queue.Pop();
				const uint32_t cost = (uint32_t(p.x) % 97) * 512;
				EXPECT_GE(cost, last) << "order broke in round " << round;
				last = cost;
				++pops;
			}
			EXPECT_EQ(pops, N / 3);
		}
	}
}
}
