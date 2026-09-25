// SPDX-FileCopyrightText: 2026 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef OCCUPANCYBITMAP_H
#define OCCUPANCYBITMAP_H

#include <array>
#include <cassert>
#include <cstdint>
#include <cstring>

#if defined(_MSC_VER)
	#include <intrin.h>
#endif

namespace GemRB {

/**
 * A three-level bitmap whose NextSetFrom() returns the first occupied slot at or after a given
 * one without walking the empty ones. Used by BucketPriorityQueue::Pop() to find the lowest
 * non-empty bucket in O(1), independent of bucket width.
 *
 * Level 1 has a bit per slot, level 2 a bit per level-1 word, level 3 a bit per level-2 word.
 * Three levels reach 64^3 slots, which the static_assert below holds callers to.
 */
template<uint32_t SLOTS>
class OccupancyBitmap {
public:
	// Level-1 and level-2 words are 64 bits. WORD_SHIFT is log2(WORD_BITS), so `x >> WORD_SHIFT`
	// is the word index and `x & WORD_MASK` is the bit within that word.
	static constexpr uint32_t WORD_BITS = 64;
	static constexpr uint32_t WORD_SHIFT = 6;
	static constexpr uint32_t WORD_MASK = WORD_BITS - 1;

	static constexpr uint32_t L1_WORDS = (SLOTS + WORD_MASK) / WORD_BITS;
	static constexpr uint32_t L2_WORDS = (L1_WORDS + WORD_MASK) / WORD_BITS;

	static_assert(L2_WORDS <= WORD_BITS, "three levels reach 64^3 slots; a fourth would be needed above that");

	void Clear() noexcept
	{
		l1.fill(0);
		l2.fill(0);
		l3 = 0;
	}

	/**
	 * Clears [lo, hi] cheaply, for callers that only ever occupy a contiguous span and want to
	 * drop it without zeroing the whole table (see BucketPriorityQueue's dirty range).
	 *
	 * Every slot outside the range must already be clear. That precondition is what enables the
	 * coarse memsets (they clear whole words overlapping the range) and the blanket
	 * `l3 = 0` - since nothing is set outside [lo, hi], no l3 bit outside the cleared l2 words
	 * needs to survive.
	 * Called with occupied slots outside the range, those slots are silently lost from NextSetFrom().
	 */
	void ClearRange(const uint32_t lo, const uint32_t hi) noexcept
	{
		assert(lo <= hi && hi < SLOTS);
		const uint32_t loWord = lo >> WORD_SHIFT;
		const uint32_t hiWord = hi >> WORD_SHIFT;
		std::memset(l1.data() + loWord, 0, static_cast<size_t>(hiWord - loWord + 1) * sizeof(uint64_t));
		std::memset(l2.data() + (loWord >> WORD_SHIFT), 0, static_cast<size_t>((hiWord >> WORD_SHIFT) - (loWord >> WORD_SHIFT) + 1) * sizeof(uint64_t));
		l3 = 0;
	}

	void Set(const uint32_t slot) noexcept
	{
		assert(slot < SLOTS);
		const uint32_t w1 = slot >> WORD_SHIFT;
		l1[w1] |= Bit(slot & WORD_MASK);
		l2[w1 >> WORD_SHIFT] |= Bit(w1 & WORD_MASK);
		l3 |= Bit(w1 >> WORD_SHIFT);
	}

	void Reset(const uint32_t slot) noexcept
	{
		assert(slot < SLOTS);
		const uint32_t w1 = slot >> WORD_SHIFT;
		l1[w1] &= ~Bit(slot & WORD_MASK);
		if (l1[w1] != 0) return;

		const uint32_t w2 = w1 >> WORD_SHIFT;
		l2[w2] &= ~Bit(w1 & WORD_MASK);
		if (l2[w2] != 0) return;

		l3 &= ~Bit(w2);
	}

	bool Test(const uint32_t slot) const noexcept
	{
		assert(slot < SLOTS);
		return (l1[slot >> WORD_SHIFT] & Bit(slot & WORD_MASK)) != 0;
	}

	/** Returns first occupied slot at or after `from`, or SLOTS if there is none. */
	uint32_t NextSetFrom(const uint32_t from) const noexcept
	{
		if (from >= SLOTS) return SLOTS;

		const uint32_t w1 = from >> WORD_SHIFT;
		const uint64_t here = l1[w1] & MaskFrom(from & WORD_MASK);
		if (here != 0) return (w1 << WORD_SHIFT) | CountTrailingZeros(here);

		// nothing left in `from`'s own level-1 word; find the next non-empty one
		uint32_t w2 = w1 >> WORD_SHIFT;
		uint64_t cand = l2[w2] & MaskFrom((w1 & WORD_MASK) + 1);
		if (cand == 0) {
			const uint64_t upper = l3 & MaskFrom(w2 + 1);
			if (upper == 0) return SLOTS;
			w2 = CountTrailingZeros(upper);
			cand = l2[w2];
		}

		const uint32_t nextW1 = (w2 << WORD_SHIFT) | CountTrailingZeros(cand);
		return (nextW1 << WORD_SHIFT) | CountTrailingZeros(l1[nextW1]);
	}

private:
	static constexpr uint64_t Bit(const uint32_t n) noexcept { return static_cast<uint64_t>(1) << n; }

	/** Bits at or above position `p`; p == WORD_BITS means "none", which a shift by 64 may not express. */
	static constexpr uint64_t MaskFrom(const uint32_t p) noexcept
	{
		constexpr uint64_t zero = 0;
		return p >= WORD_BITS ? zero : (~zero << p);
	}

	/**
	 * Returns an index of the lowest set bit, 0..WORD_MASK.
	 * `v` must be non-zero.
	 *
	 * The intrinsics are preferred because GCC/Clang and MSVC each lower them to a single
	 * hardware instruction (BSF/TZCNT).
	 * Other compilers fall back to a de Bruijn multiply:
	 * `v & -v` isolates the lowest set bit, so multiplying by the de Bruijn constant is a
	 * shift-and-copy whose top six bits uniquely identify the bit position for all 64 inputs;
	 * that index selects the answer from `index`. Constant time, no intrinsic required.
	 */
	static uint32_t CountTrailingZeros(uint64_t v) noexcept
	{
		assert(v != 0);
#if defined(__GNUC__) || defined(__clang__)
		return static_cast<uint32_t>(__builtin_ctzll(v));
#elif defined(_MSC_VER)
		unsigned long idx;
		_BitScanForward64(&idx, v);
		return static_cast<uint32_t>(idx);
#else
		// de Bruijn fallback
		//
		// `v & (~v + 1)` (v & -v) isolates the lowest set bit, making the value a power of
		// two.
		// 0x03f79d71b4cb0a89 is a de Bruijn sequence B(2,6): a 64-bit cyclic bit string
		// in which every possible 6-bit window occurs exactly once.
		// This is the prefer-one sequence, packed MSB first.
		// Multiplying a power of two by it is a shift, selecting the 6-bit window that starts
		// at that bit's position; `>> (WORD_BITS - WORD_SHIFT)` keeps those top
		// (WORD_BITS - WORD_SHIFT) bits, which are a unique key 0..WORD_MASK for each of the
		// WORD_BITS positions.
		//
		// index[] is the inverse of that map (key -> position), so its entries look
		// scrambled: index[key] is the bit position the multiply would map to `key`.
		static constexpr uint64_t deBruijn = 0x03f79d71b4cb0a89ull;
		static constexpr uint8_t index[WORD_BITS] = {
			0, 1, 48, 2, 57, 49, 28, 3, 61, 58, 50, 42, 38, 29, 17, 4,
			62, 55, 59, 36, 53, 51, 43, 22, 45, 39, 33, 30, 24, 18, 12, 5,
			63, 47, 56, 27, 60, 41, 37, 16, 54, 35, 52, 21, 44, 32, 23, 11,
			46, 26, 40, 15, 34, 20, 31, 10, 25, 14, 19, 9, 13, 8, 7, 6
		};
		return index[((v & (~v + 1)) * deBruijn) >> (WORD_BITS - WORD_SHIFT)];
#endif
	}

	std::array<uint64_t, L1_WORDS> l1 {}; // one bit per slot
	std::array<uint64_t, L2_WORDS> l2 {}; // one bit per l1 word
	uint64_t l3 = 0; // one bit per l2 word
};

}

#endif // OCCUPANCYBITMAP_H
