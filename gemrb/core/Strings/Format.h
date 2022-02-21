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

namespace fmt {

template <typename STR>
struct formatter<STR, char,
// ignore the junk after STR... its just SFINAE garbage to hide the ctor from certain useages
enable_if_t<std::is_class<STR>::value && !std::is_same<STR, basic_string_view<char>>::value && !std::is_same<STR, std::string>::value>
> : formatter<const char*, char> {
	template <typename FormatContext>
	auto format(const STR& str, FormatContext &ctx) -> decltype(ctx.out()) {
		return format_to(ctx.out(), str.begin());
	}
};

}

#endif /* FORMAT_H */
