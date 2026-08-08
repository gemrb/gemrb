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

#ifndef FIXED_SIZE_POOL_H
#define FIXED_SIZE_POOL_H

#include "Logging/Logging.h"

#include <cstddef>
#include <deque>
#include <memory>
#include <new>
#include <type_traits>
#include <vector>

namespace GemRB {

/**
	 * Freelist pool of same-sized slots, carved out of larger raw blocks.
	 *
	 * Requires trivially copyable T, as this works in concept as malloc/free - treat it
	 * as raw, uninitialized memory pool:
	 * Alloc() returns raw, uninitialized storage, no constructor is run.
	 * Free() only returns a slot to the freelist, running no destructor and releasing nothing to the system.
	 * Memory goes back only when the pool itself is destroyed.
	 */
template<typename T>
class FixedSizePool {
	static_assert(std::is_trivially_copyable<T>::value,
		      "FixedSizePool requires trivially copyable types.");

public:
	explicit FixedSizePool(const size_t initialElements = 20, const size_t allocationGrowthFactor = 1)
		: AllocationGrowthFactor(allocationGrowthFactor), PreviousAllocationSize(initialElements)
	{
		allocateNextBlock();
	}

	void EnsureAtLeastFreeElements(const size_t elementsAtLeast)
	{
		if (availableElements.size() >= elementsAtLeast) {
			return;
		}
		const size_t toBeAllocated = elementsAtLeast - availableElements.size();
		allocateNextBlock(toBeAllocated);
	}

	T* Alloc()
	{
		if (availableElements.empty()) {
			allocateNextBlock();
		}
		T* allocatedElement = availableElements.back();
		availableElements.pop_back();
		return allocatedElement;
	}

	void Free(T* elementToFree) noexcept
	{
		try {
			availableElements.push_back(elementToFree);
		} catch (...) {
			// this path is unreachable, but exists to satisfy the static analysis tools.
			// availableElements.push_back() should never throw under any normal
			// circumstances: T* is trivially copyable, so no exceptions from the
			// assignment, and all slots are pre-allocated so no allocation failure
			// can happen.
		}
	}


private:
	struct Block {
		// raw storage; no T is ever constructed in it, see the class comment
		std::unique_ptr<unsigned char[]> memory;
		size_t numberOfElements;

		explicit Block(const size_t inNumberOfElements)
			: memory(new(std::nothrow) unsigned char[inNumberOfElements * sizeof(T)]),
			  numberOfElements(inNumberOfElements)
		{
			if (memory == nullptr) {
				error("FixedSizePool", "Failed to allocate a block of {} elements of {} bytes.",
				      inNumberOfElements, sizeof(T));
			}
		}

		Block(const Block&) = delete;
		Block(Block&& other) noexcept = default;
	};
	std::deque<Block> blocks;
	std::vector<T*> availableElements;
	size_t AllocationGrowthFactor = 1;
	size_t PreviousAllocationSize = 1;
	size_t TotalElements = 0;

	void allocateNextBlock(size_t newAllocationSize = 0)
	{
		// allocate new block of `newAllocationSize` elements
		if (newAllocationSize == 0) {
			newAllocationSize = PreviousAllocationSize * AllocationGrowthFactor;
		}
		blocks.emplace_back(newAllocationSize);

		// add each new element from the new allocation to the list of available elements
		T* newBlockDataPointer = reinterpret_cast<T*>(blocks.back().memory.get());
		TotalElements += newAllocationSize;
		availableElements.reserve(TotalElements);
		for (size_t i = 0; i < newAllocationSize; ++i) {
			availableElements.push_back(newBlockDataPointer + i);
		}

		// bookkeeping; never shrink, an explicitly sized block is a one-off request and must not
		// become the size of every default allocation after it
		if (newAllocationSize > PreviousAllocationSize) {
			PreviousAllocationSize = newAllocationSize;
		}
	}
};
}

#endif
