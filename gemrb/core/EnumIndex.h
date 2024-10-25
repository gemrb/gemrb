/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2023 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */
#ifndef EnumIndex_h
#define EnumIndex_h

#include "EnumFlags.h"

#include <array>
#include <bitset>
#include <cassert>
#include <initializer_list>
#include <type_traits>

namespace GemRB {

// This will get the int value for an enum
// if you want a text value then add a more specific overload for a specific enum
template<typename ENUM>
constexpr std::enable_if_t<std::is_enum<ENUM>::value, under_sfinae_t<ENUM>>
	format_as(const ENUM& e)
{
	return UnderType(e);
}

template<typename ENUM, typename ARG = under_t<ENUM>>
constexpr ENUM EnumIndex(ARG val) noexcept
{
	static_assert(std::is_same<under_t<ENUM>, ARG>::value, "Will not implicitly convert to EnumIndex");
	static_assert(std::is_unsigned<under_t<ENUM>>::value, "EnumIndex must be unsigned");
	val = Clamp(val, ARG(0), ARG(ENUM::count));
	// cannot static assert here without c++17 if constexpr
	// static_assert(val < UnderType(ENUM::count), "Trying to create an EnumIndex beoynd the limit.");
	return static_cast<ENUM>(val);
}

template<typename ENUM, ENUM BEGIN = ENUM(0), ENUM END = ENUM::count>
class EnumIterator {
	static_assert(std::is_unsigned<under_t<ENUM>>::value, "EnumIndex must be unsigned");

	under_t<ENUM> val;

public:
	using reference = ENUM&;
	using pointer = ENUM*;
	using value_type = ENUM;
	using iterator_category = std::forward_iterator_tag;
	using difference_type = int;

	explicit EnumIterator(const ENUM& e)
		: val(UnderType(e)) {}
	EnumIterator()
		: EnumIterator(BEGIN) {}

	EnumIterator& operator++()
	{
		++val;
		return *this;
	}

	EnumIterator operator+(int i)
	{
		return EnumIterator(static_cast<ENUM>(val + i));
	}

	int distance(const EnumIterator& it) const
	{
		return it.val - val;
	}

	ENUM operator*() const { return static_cast<ENUM>(val); }

	EnumIterator begin() const { return *this; }
	EnumIterator end() const { return EnumIterator(END); }

	bool operator!=(const EnumIterator& i) { return val != i.val; }
};

template<typename ENUM, typename T = ENUM>
class EnumArray {
	static_assert(std::is_unsigned<under_t<ENUM>>::value, "EnumIndex must be unsigned");

	using array_t = std::array<T, UnderType(ENUM::count)>;
	array_t array {};

public:
	static constexpr auto size = UnderType(ENUM::count);
	using KeyIterator_t = EnumIterator<ENUM>;

	KeyIterator_t KeyIterator() const { return KeyIterator_t(); }

	template<typename... ELEMS>
	explicit constexpr EnumArray(ELEMS&&... e)
		: array { { std::forward<ELEMS>(e)... } }
	{}

	constexpr EnumArray() = default;

	constexpr const T& operator[](ENUM key) const
	{
		return array[UnderType(key)];
	}

	constexpr T& operator[](ENUM key)
	{
		return array[UnderType(key)];
	}

	constexpr const T& operator[](under_t<ENUM> idx) const
	{
		return array[idx];
	}

	constexpr T& operator[](under_t<ENUM> idx)
	{
		return array[idx];
	}

	typename array_t::iterator begin()
	{
		return array.begin();
	}

	typename array_t::const_iterator begin() const
	{
		return array.begin();
	}

	typename array_t::iterator end()
	{
		return array.end();
	}

	typename array_t::const_iterator end() const
	{
		return array.end();
	}
};

template<typename ENUM>
class EnumBitset {
	static_assert(std::is_unsigned<under_t<ENUM>>::value, "EnumIndex must be unsigned");

	std::bitset<UnderType(ENUM::count)> bits;

public:
	static constexpr auto size = UnderType(ENUM::count);

	explicit constexpr EnumBitset(under_t<ENUM> value)
		: bits(value) {}
	explicit constexpr EnumBitset(ENUM value)
		: bits(1 << UnderType(value)) {}
	constexpr EnumBitset() = default;

	constexpr bool operator[](ENUM key) const
	{
		return bits[UnderType(key)];
	}

	typename std::bitset<UnderType(ENUM::count)>::reference
		operator[](ENUM key)
	{
		return bits[UnderType(key)];
	}

	bool Test(ENUM key) const
	{
		return bits.test(UnderType(key));
	}

	void Set(ENUM key)
	{
		bits.set(UnderType(key));
	}

	void SetAll()
	{
		bits.set();
	}

	void Flip(ENUM key)
	{
		bits.flip(UnderType(key));
	}

	void Clear(ENUM key)
	{
		bits.reset(UnderType(key));
	}

	void ClearAll()
	{
		bits.reset();
	}

	unsigned long to_ulong() const
	{
		return bits.to_ulong();
	}
};

}
#endif /* EnumIndex_h */
