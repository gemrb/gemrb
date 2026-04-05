// SPDX-FileCopyrightText: 2021 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

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
