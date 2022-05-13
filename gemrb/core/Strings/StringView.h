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
template <typename CharT = const char>
class StringViewImp {
public:
	using value_type = CharT;
	using iterator = CharT*;
	using const_iterator = const CharT*;
	using size_type = size_t;
	static constexpr size_type npos = -1;
	
	StringViewImp() noexcept = default;

	// explicit because strlen is inefficient
	explicit StringViewImp(CharT* cstr) noexcept
	: StringViewImp(cstr, std::strlen(cstr)) {}
	
	StringViewImp(CharT* cstr, size_type len) noexcept
	: data(cstr), len(len) {}

	template<typename STR, typename CharT2 = CharT, ENABLE_CHAR_RANGE(STR),
	typename std::enable_if<std::is_const<CharT2>::value && !std::is_convertible<STR, StringViewImp>::value, int>::type = 0>
	StringViewImp(const STR& s) noexcept
	: StringViewImp(&s[0], std::distance(std::begin(s), std::end(s)))
	{}
	
	template<typename STR, typename CharT2 = CharT, ENABLE_CHAR_RANGE(STR),
	typename std::enable_if<!std::is_const<CharT2>::value && !std::is_convertible<STR, StringViewImp>::value, int>::type = 0>
	StringViewImp(STR& s) noexcept
	: StringViewImp(&s[0], std::distance(std::begin(s), std::end(s)))
	{}
	
	template<size_type N>
	// string literals are null terminated so dont include the null byte
	StringViewImp(char const (&s)[N], size_t len = N - 1) noexcept
	: StringViewImp(&s[0], len) // string literals are null terminated so dont include the null byte
	{}
	
	template<size_type LEN, int(*CMP)(const char*, const char*, size_type)>
	StringViewImp(const FixedSizeString<LEN, CMP>& fs) noexcept
	: StringViewImp(fs.begin(), fs.length())
	{}

	// this is mainly replacing std::string&, so i tried to keep it compatible
	const CharT* c_str() const noexcept {
		return data;
	}

	size_type length() const noexcept {
		return len;
	}
	
	bool empty() const noexcept {
		return len == 0;
	}

	iterator begin() const noexcept {
		return data;
	}

	iterator end() const noexcept {
		return data + len;
	}

	const CharT& operator[](size_t i) const noexcept {
		return *(data + i);
	}

	explicit operator bool() const noexcept {
		return data != nullptr;
	}
	
private:
	CharT* data = nullptr;
	size_type len = 0;
};

using StringView = StringViewImp<const char>;
using MutableStringView = StringViewImp<char>;

}

#endif /* STRINGVIEW_H */
