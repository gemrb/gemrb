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

#ifndef STRINGVIEW_H
#define STRINGVIEW_H

#include "CString.h"

namespace GemRB {

// effectively a pre c++17 std::string_view
// it is just a view into an existing buffer
// the buffer must have a lifespan beyond the view
// the view is entirely constant and cannot be used
// to modify the underlying buffer
class StringView {
	const char* data = nullptr;
	size_t len = 0;
public:
	using const_iterator = const char*;

	// explicit because strlen is inefficient
	explicit StringView(const char* cstr) noexcept
	: StringView(cstr, std::strlen(cstr)) {}
	
	StringView(const char* cstr, size_t len) noexcept
	: data(cstr), len(len) {}

	template<typename STR, ENABLE_CHAR_RANGE(STR)>
	StringView(const STR& s) noexcept
	: StringView(&s[0], std::distance(std::begin(s), std::end(s)))
	{}
	
	template<size_t N>
	// string literals are null terminated so dont include the null byte
	StringView(char const (&s)[N], size_t len = N - 1) noexcept
	: StringView(&s[0], len)
	{}
	
	template<size_t LEN, int(*CMP)(const char*, const char*, size_t)>
	StringView(const FixedSizeString<LEN, CMP>& fs) noexcept
	: StringView(fs.begin(), fs.CStrLen())
	{}

	// this is mainly replacing std::string&, so i tried to keep it compatible
	const char* c_str() const noexcept {
		return data;
	}

	size_t length() const noexcept {
		return len;
	}

	const_iterator begin() const noexcept {
		return data;
	}

	const_iterator end() const noexcept {
		return data + len;
	}

	const char& operator[](size_t i) const noexcept {
		return *(data + i);
	}
};

}

#endif /* STRINGVIEW_H */
