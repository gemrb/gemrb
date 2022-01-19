/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
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

#ifndef STRING_H
#define STRING_H

#include "exports.h"

#include <string>

#define FMT_HEADER_ONLY
#include <fmt/format.h>
#include <fmt/xchar.h>

#define WHITESPACE_STRING L"\n\t\r "

namespace GemRB {

using String = std::basic_string<wchar_t>;

// char manipulators
inline wchar_t tolower(wchar_t c) { return ::towlower(c); }
inline wchar_t toupper(wchar_t c) { return ::towupper(c); }

GEM_EXPORT void TrimString(String& string);

template<typename ...ARGS>
std::string& AppendFormat(std::string& str, const std::string& fmt, ARGS&& ...args) {
	std::string formatted = fmt::format(fmt, std::forward<ARGS>(args)...);
	return str += formatted;
}

}

#endif
