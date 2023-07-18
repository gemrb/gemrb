/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2022 The GemRB Project
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
 */

#ifndef FORMAT_H
#define FORMAT_H

#define FMT_HEADER_ONLY
#define FMT_EXCEPTIONS 0
#include <fmt/format.h>

namespace GemRB {

// C++17 provides void_t...
template<typename... Ts>
struct make_void { typedef void type; };
 
template<typename... Ts>
using void_t = typename make_void<Ts...>::type;

template <typename, typename = void>
struct has_c_str : std::false_type {};

template <typename T>
struct has_c_str<T, void_t<decltype(&T::c_str)>> : std::is_same<char const*, decltype(std::declval<T>().c_str())>
{};

template <typename STR, FMT_ENABLE_IF(has_c_str<STR>::value && !std::is_same<std::string, STR>::value)>
auto format_as(const STR& str) { return str.c_str(); }

}

#endif /* FORMAT_H */
