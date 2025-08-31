/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2021 The GemRB Project
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

#ifndef EnumFlags_h
#define EnumFlags_h

#include <type_traits>

#ifdef HAVE_ATTRIBUTE_FLAG_ENUM
	#define FLAG_ENUM enum class __attribute__((flag_enum))
#else
	#define FLAG_ENUM enum class
#endif

template<typename ENUM>
using under_t = std::underlying_type_t<ENUM>;

template<typename T, bool = std::is_enum<T>::value>
struct under_sfinae {
	using type = typename std::underlying_type_t<T>;
};

template<typename T>
struct under_sfinae<T, false> {
	using type = T;
};

template<typename ENUM>
using under_sfinae_t = typename under_sfinae<ENUM>::type;

template<typename ENUM_FLAGS>
constexpr under_sfinae_t<ENUM_FLAGS>
	UnderType(ENUM_FLAGS a) noexcept
{
	return static_cast<under_t<ENUM_FLAGS>>(a);
}

template<typename ENUM_FLAGS>
constexpr std::enable_if_t<std::is_enum<ENUM_FLAGS>::value, ENUM_FLAGS>
	operator|(ENUM_FLAGS a, ENUM_FLAGS b) noexcept
{
	return static_cast<ENUM_FLAGS>(UnderType(a) | UnderType(b));
}

template<typename ENUM_FLAGS>
constexpr std::enable_if_t<std::is_enum<ENUM_FLAGS>::value, ENUM_FLAGS&>
	operator|=(ENUM_FLAGS& a, ENUM_FLAGS b) noexcept
{
	return a = a | b;
}

template<typename ENUM_FLAGS>
constexpr std::enable_if_t<std::is_enum<ENUM_FLAGS>::value, ENUM_FLAGS>
	operator&(ENUM_FLAGS a, ENUM_FLAGS b) noexcept
{
	return static_cast<ENUM_FLAGS>(UnderType(a) & UnderType(b));
}

template<typename ENUM_FLAGS>
constexpr std::enable_if_t<std::is_enum<ENUM_FLAGS>::value, ENUM_FLAGS&>
	operator&=(ENUM_FLAGS& a, ENUM_FLAGS b) noexcept
{
	return a = a & b;
}

template<typename ENUM_FLAGS>
constexpr std::enable_if_t<std::is_enum<ENUM_FLAGS>::value, ENUM_FLAGS>
	operator^(ENUM_FLAGS a, ENUM_FLAGS b) noexcept
{
	return static_cast<ENUM_FLAGS>(UnderType(a) ^ UnderType(b));
}

template<typename ENUM_FLAGS>
constexpr std::enable_if_t<std::is_enum<ENUM_FLAGS>::value, ENUM_FLAGS&>
	operator^=(ENUM_FLAGS& a, ENUM_FLAGS b) noexcept
{
	return a = a ^ b;
}

template<typename ENUM_FLAGS>
constexpr std::enable_if_t<std::is_enum<ENUM_FLAGS>::value, ENUM_FLAGS>
	operator~(ENUM_FLAGS a) noexcept
{
	return static_cast<ENUM_FLAGS>(~UnderType(a));
}

template<typename ENUM_FLAGS>
constexpr std::enable_if_t<std::is_enum<ENUM_FLAGS>::value, bool>
	operator!(ENUM_FLAGS a) noexcept
{
	return !UnderType(a);
}

#endif /* EnumFlags_h */
