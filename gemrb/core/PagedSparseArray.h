/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2026 The GemRB Project
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

#ifndef PAGED_SPARSE_ARRAY_H
#define PAGED_SPARSE_ARRAY_H

#include "FixedSizePool.h"

#include <cassert>
#include <cstddef>
#include <cstring>
#include <type_traits>
#include <unordered_set>
#include <utility>
#include <vector>

namespace GemRB {

/**
 * Array over a large index space, backed by fixed-size pages allocated on first write and
 * released once all their cells are back to T's default value.
 *
 * Intended for a large index space with sparse, locally dense contents.
 *
 * Use case: the traversability cache is indexed per navmap cell, and the navmap is 16x12 finer
 * than the searchmap. A 200x180-tile searchmap gives 3200x2160 = ~6.9M cells, which at
 * sizeof(TraversabilityCellData) == 16 is ~110 MB per map if stored densely. Only cells under an
 * actor's collision circle are ever non-default, so the live set is orders of magnitude smaller
 * and lies in contiguous runs.
 *
 * Properties:
 * - Cell storage is proportional to occupancy rather than to the index space. The `pageTable`
 *   vector is the exception: it holds one pointer per page slot over the whole index space and
 *   is present regardless of occupancy. See PAGE_BITS.
 * - Indexing is O(1) without hashing: index >> PAGE_BITS is the page number, index & PAGE_MASK
 *   the offset within it. Reading a cell in an absent page returns a default T and allocates
 *   nothing.
 * - Pages are contiguous and trivially copyable, so copy and sync operate PAGE_SIZE cells at a
 *   time via memcpy.
 * - dirtyPages records touched pages, so SyncFrom() refreshes only those.
 * - Pages are taken from a FixedSizePool freelist, so allocation and release are a pop and a
 *   push.
 *
 * DefaultTIsAllZeroBytes describes T and selects how a cell is reset and how a page is tested
 * for emptiness. Both operations are frequent: a cell is reset whenever an actor leaves it, and
 * the emptiness test runs on every reset to decide whether the page can be released.
 *
 * true: a default-constructed T is all-zero bytes, padding included. Cell reset is a memset,
 * page emptiness is one memcmp against a shared zero buffer, and a new page is initialised with
 * a single memset.
 *
 * false: no such guarantee. Cell reset assigns a default-constructed T, and emptiness is tested
 * element by element, which requires T to have operator!=.
 *
 * The padding requirement on the true path is not incidental: memcmp compares padding bytes, so a
 * T with indeterminate padding can hold a logically default cell that compares unequal to the
 * zero buffer. The page is then never detected as empty and never released, and the array grows
 * toward its dense size. This is not diagnosable at compile time, so the constructor asserts it.
 * TraversabilityCellData declares a zeroed padding member to satisfy it.
 *
 * The two paths are selected by tag dispatch, so only the one matching the instantiation is
 * compiled and a T without operator!= remains usable on the true path.
 */
template<typename T, bool DefaultTIsAllZeroBytes = true, size_t PageBits = 6>
struct PagedSparseArray {
	static_assert(std::is_trivially_copyable<T>::value,
		      "PagedSparseArray requires trivially copyable types.");
	static_assert(PageBits > 0 && PageBits < sizeof(size_t) * 8,
		      "PageBits must be a usable power-of-two exponent.");

	/**
     * Page length, as a power-of-two exponent: a page holds PAGE_SIZE cells, an index splits
     * into page number and offset with a shift and a mask, and no division is involved.
     *
     * The default of 6 (64 cells, so 1 KB per page at sizeof(T) == 16) was picked by benchmarking
     * on a low-throughput system.
     * It is exposed as a template parameter because the best value depends on T and on the access pattern.
     *
     * What moves with it:
     * - The `pageTable` vector holds one pointer per page slot across the whole index space,
     *   allocated or not, so its size scales with 1 / PAGE_SIZE. This is the fixed cost that does
     *   not shrink with occupancy, and it is why very small pages get expensive on a large index
     *   space.
     * - Larger pages mean fewer allocator operations and longer contiguous runs for the memcpy in
     *   copy and sync.
     * - Larger pages also mean more unused cells retained per partially occupied page, and more
     *   bytes scanned by the emptiness memcmp, which runs on every cell reset.
     */
	static constexpr size_t PAGE_BITS = PageBits;
	static constexpr size_t PAGE_SIZE = (size_t(1) << PAGE_BITS);
	static constexpr size_t PAGE_MASK = PAGE_SIZE - 1;

	// Only odr-used on the DefaultTIsAllZeroBytes == true path, and static data members of a class
	// template are instantiated on demand, so a false instantiation never emits it.
	static const char ZeroBuffer[PAGE_SIZE * sizeof(T)];

	using TPage_t = T[PAGE_SIZE];

	FixedSizePool<TPage_t>& pageAllocator;
	std::unordered_set<size_t> dirtyPages;
	std::vector<TPage_t*> pageTable;
	size_t totalSize;
	size_t usedPages = 0;

	explicit PagedSparseArray(FixedSizePool<TPage_t>& inPageAllocator, const size_t size = 0)
		: pageAllocator(inPageAllocator),
		  totalSize(size)
	{
		pageTable.resize((size + PAGE_SIZE - 1) >> PAGE_BITS, nullptr);
		AssertDefaultTIsAllZeroBytes(std::integral_constant<bool, DefaultTIsAllZeroBytes> {});
	}

	PagedSparseArray(PagedSparseArray&& other) noexcept
		: pageAllocator(other.pageAllocator),
		  dirtyPages(std::move(other.dirtyPages)),
		  pageTable(std::move(other.pageTable)),
		  totalSize(other.totalSize),
		  usedPages(other.usedPages)
	{
		other.totalSize = 0;
		other.usedPages = 0;
	}

	// do not allow copy construction nor copy assignment, as we need clear definition of allocator's ownership
	// copying is available as the named CopyFrom() instead
	PagedSparseArray(const PagedSparseArray& other) = delete;
	PagedSparseArray& operator=(const PagedSparseArray& other) = delete;

	// copy `other`'s contents, using `this` instance of the allocator
	void CopyFrom(const PagedSparseArray& other)
	{
		if (this == &other) {
			return;
		}

		clear(other.size());
		dirtyPages.clear();
		pageAllocator.EnsureAtLeastFreeElements(other.usedPages);
		for (size_t i = 0; i < other.pageTable.size(); ++i) {
			if (other.pageTable[i]) {
				this->pageTable[i] = this->pageAllocator.Alloc();
				// Note: memset not needed here since memcpy overwrites the entire page
				std::memcpy(this->pageTable[i], other.pageTable[i], PAGE_SIZE * sizeof(T));
				++usedPages;
			}
		}
	}

	~PagedSparseArray()
	{
		usedPages = 0;
		for (auto& page : pageTable) {
			if (page) {
				pageAllocator.Free(page);
			}
			page = nullptr;
		}
	}

	void clear(const size_t size)
	{
		// dropping a page is a change a consumer still has to see, so it is recorded
		for (size_t pageIdx = 0; pageIdx < pageTable.size(); ++pageIdx) {
			if (pageTable[pageIdx]) {
				pageAllocator.Free(pageTable[pageIdx]);
				dirtyPages.insert(pageIdx);
			}
		}
		if (totalSize != size) {
			// the indices would dangle past the new page table; SyncFrom() copies wholesale after
			// a resize instead of consuming this set
			dirtyPages.clear();
		}

		totalSize = size;
		pageTable.clear();
		pageTable.resize((size + PAGE_SIZE - 1) >> PAGE_BITS, nullptr);
		usedPages = 0;
	}

	/**
     * Brings this array up to date with `other`, and consumes `other`'s dirty set: the source is
     * taken by non-const reference for that reason, and a second call in a row syncs nothing.
     *
     * On the first call, when this array has no pages yet, everything is copied. The dirty set
     * only lists what changed since it was last consumed, which is not the full live set, so it
     * cannot be used to build a snapshot from nothing. Later calls copy only the dirty pages.
     *
     * A source that was resized since the last call is also copied wholesale: its dirty set is
     * numbered against a page table this array no longer matches, and this array's own pages past
     * the source's new end would otherwise be left behind as stale.
     */
	void SyncFrom(PagedSparseArray& other)
	{
		if (pageTable.empty() || totalSize != other.totalSize) {
			CopyFrom(other);
			other.dirtyPages.clear();
			return;
		}

		for (auto dirtyIdx : other.dirtyPages) {
			if (other.pageTable[dirtyIdx]) {
				if (!pageTable[dirtyIdx]) {
					pageTable[dirtyIdx] = pageAllocator.Alloc();
					// Note: memset not needed here since memcpy overwrites the entire page
					++usedPages;
				}
				std::memcpy(pageTable[dirtyIdx], other.pageTable[dirtyIdx], PAGE_SIZE * sizeof(T));
			} else {
				if (pageTable[dirtyIdx]) {
					pageAllocator.Free(pageTable[dirtyIdx]);
					--usedPages;
					pageTable[dirtyIdx] = nullptr;
				}
			}
		}
		other.dirtyPages.clear();
	}


	inline size_t size() const
	{
		return totalSize;
	}

	// reset a cell to default value, deallocate page if empty
	void reset(const size_t index)
	{
		const size_t pageIdx = index >> PAGE_BITS;
		TPage_t* page = pageTable[pageIdx];

		if (!page) {
			// already empty, nothing to do
			return;
		}

		dirtyPages.insert(pageIdx);

		using DefaultIsZeroTag = std::integral_constant<bool, DefaultTIsAllZeroBytes>;

		ResetCell(page, index & PAGE_MASK, DefaultIsZeroTag {});

		// check if entire page is now empty, if not - exit, if yes - delete the page
		if (!IsPageEmpty(page, DefaultIsZeroTag {})) {
			return;
		}

		pageAllocator.Free(page);
		--usedPages;
		pageTable[pageIdx] = nullptr;
	}

	// Proxy class for lazy write allocation
	class Proxy {
		PagedSparseArray& arr;
		size_t index;

	public:
		Proxy(PagedSparseArray& a, const size_t i)
			: arr(a), index(i) {}

		// Read - no allocation
		// Explanation for sonar exemption:
		// Deliberately implicit: Proxy is a proxy reference, and its whole
		// purpose is to be invisible at the call site so that `arr[i]` reads and `arr[i] = v`
		// writes without the caller knowing which one it holds. `explicit` would defeat that.
		operator T() const // NOSONAR
		{
			const TPage_t* page = arr.pageTable[index >> PAGE_BITS];
			return page ? (*page)[index & PAGE_MASK] : T();
		}

		// Write - allocate on demand
		Proxy& operator=(const T& value)
		{
			const size_t pageIdx = index >> PAGE_BITS;
			arr.dirtyPages.insert(pageIdx);
			if (!arr.pageTable[pageIdx]) {
				arr.pageTable[pageIdx] = arr.pageAllocator.Alloc();
				std::memset(arr.pageTable[pageIdx], 0, PAGE_SIZE * sizeof(T));
				++arr.usedPages;
			}
			(*arr.pageTable[pageIdx])[index & PAGE_MASK] = value;
			return *this;
		}

		// Support for move assignment
		Proxy& operator=(T&& value)
		{
			const size_t pageIdx = index >> PAGE_BITS;
			arr.dirtyPages.insert(pageIdx);
			if (!arr.pageTable[pageIdx]) {
				arr.pageTable[pageIdx] = arr.pageAllocator.Alloc();
				std::memset(arr.pageTable[pageIdx], 0, PAGE_SIZE * sizeof(T));
				++arr.usedPages;
			}
			(*arr.pageTable[pageIdx])[index & PAGE_MASK] = std::move(value);
			return *this;
		}
	};

	// Read-only indexing (const version)
	inline T operator[](const size_t index) const
	{
		const TPage_t* page = pageTable[index >> PAGE_BITS];
		return page ? (*page)[index & PAGE_MASK] : T();
	}

	// Write-capable indexing (non-const version)
	inline Proxy operator[](const size_t index)
	{
		return Proxy(*this, index);
	}

private:
	// The std::true_type / std::false_type overloads below are selected by tag dispatch on
	// DefaultTIsAllZeroBytes. Only the selected one is ever instantiated, so the requirements of
	// the other (operator!= for the false path) are not imposed on every T.

	static void AssertDefaultTIsAllZeroBytes(std::true_type)
	{
		// the memset/memcmp path is only valid if a defaulted T really is all-zero bytes, padding
		// included; two elements are checked so any padding between them is covered too
		constexpr size_t AssertedArraySize = 2;
		const T DefaultT[AssertedArraySize] { T(), T() };
		assert((std::memcmp(DefaultT, ZeroBuffer, AssertedArraySize * sizeof(T)) == 0) &&
		       "A default-constructed T is not all-zero bytes. Instantiate PagedSparseArray with "
		       "DefaultTIsAllZeroBytes = false, or give T a zeroed default (including struct padding).");
	}
	static void AssertDefaultTIsAllZeroBytes(std::false_type)
	{
		// intentionally empty: nothing to check, the false path claims nothing about T's bytes
	}

	static void ResetCell(TPage_t* page, const size_t cellIdx, std::true_type)
	{
		std::memset(static_cast<void*>(&(*page)[cellIdx]), 0, sizeof(T));
	}
	static void ResetCell(TPage_t* page, const size_t cellIdx, std::false_type)
	{
		(*page)[cellIdx] = T();
	}

	static bool IsPageEmpty(const TPage_t* page, std::true_type)
	{
		return std::memcmp(page, ZeroBuffer, PAGE_SIZE * sizeof(T)) == 0;
	}
	static bool IsPageEmpty(const TPage_t* page, std::false_type)
	{
		const T defaultT = T();
		for (size_t i = 0; i < PAGE_SIZE; ++i) {
			if ((*page)[i] != defaultT) {
				return false;
			}
		}
		return true;
	}
};

template<typename T, bool DefaultTIsAllZeroBytes, size_t PageBits>
const char PagedSparseArray<T, DefaultTIsAllZeroBytes, PageBits>::ZeroBuffer[PAGE_SIZE * sizeof(T)] = {};
}

#endif
