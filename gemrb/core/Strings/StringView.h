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

#include <fmt/format.h>

#include <cstring>
#include <iterator>
#include <type_traits>

// SFINAE garbage to only enable functions for strings of known size
// i'm sure its not perfect, but it meets our needs
#define ENABLE_CHAR_RANGE(PARAM) std::enable_if_t< \
(!std::is_enum<PARAM>::value && !std::is_fundamental<PARAM>::value && !std::is_pointer<PARAM>::value) \
|| std::is_same<PARAM, decltype("")>::value \
, int> = 0

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
	using Traits = std::char_traits<std::remove_const_t<CharT>>;
	static constexpr size_type npos = size_type(-1);

	StringViewImp() noexcept = default;

	// explicit because strlen is inefficient
	explicit StringViewImp(CharT* cstr) noexcept
	: StringViewImp(cstr, Traits::length(cstr)) {}
	
	StringViewImp(CharT* cstr, size_type len) noexcept
	: data(cstr), len(len) {}
	
	StringViewImp(CharT* cstr, size_type begpos, size_type endpos) noexcept
	: StringViewImp(cstr + begpos, endpos - begpos)
	{}

	template<typename STR, typename CharT2 = CharT, ENABLE_CHAR_RANGE(STR),
	std::enable_if_t<std::is_const<CharT2>::value && !std::is_convertible<STR, StringViewImp>::value, int> = 0>
	StringViewImp(const STR& s, size_type begpos = 0, size_type endpos = npos) noexcept
	: StringViewImp(&s[0], begpos, endpos == npos ? std::distance(std::begin(s), std::end(s)) : endpos)
	{}
	
	template<typename STR, typename CharT2 = CharT, ENABLE_CHAR_RANGE(STR),
	std::enable_if_t<!std::is_const<CharT2>::value && !std::is_convertible<STR, StringViewImp>::value, int> = 0>
	StringViewImp(STR& s, size_type begpos = 0, size_type endpos = npos) noexcept
	: StringViewImp(&s[0], begpos, endpos == npos ? std::distance(std::begin(s), std::end(s)) : endpos)
	{}
	
	template<size_type N>
	// string literals are null terminated so dont include the null byte
	StringViewImp(char const (&s)[N], size_t len = N - 1) noexcept
	: StringViewImp(&s[0], len) // string literals are null terminated so dont include the null byte
	{}

	// this is mainly replacing std::string&, so i tried to keep it compatible
	CharT* c_str() const noexcept {
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

	void clear() {
		erase(0);
	}
	
	void erase(size_type index) {
		this->len = index;
	}

	std::reverse_iterator<iterator> rbegin() const noexcept {
		return std::reverse_iterator<iterator>(end());
	}

	std::reverse_iterator<iterator> rend() const noexcept {
		return std::reverse_iterator<iterator>(begin());
	}

	CharT& operator[](size_t i) const noexcept {
		return *(data + i);
	}

	bool operator==(const StringViewImp<CharT>& other) const noexcept {
		if (other.length() != length()) {
			return false;
		}

		return Traits::compare(data, other.data, length()) == 0;
	}

	bool operator!=(const StringViewImp<CharT>& other) const noexcept {
		return !operator==(other);
	}

	explicit operator bool() const noexcept {
		return data != nullptr;
	}

	auto MakeString() const noexcept {
		return std::basic_string<typename Traits::char_type, Traits>(data, len);
	}

private:
	CharT* data = nullptr;
	size_type len = 0;
};

using StringView = StringViewImp<const char>;
using MutableStringView = StringViewImp<char>;

template <typename CHAR>
auto format_as(const StringViewImp<CHAR>& str) {
	return fmt::string_view(str.c_str(), str.length());
}

}

#endif /* STRINGVIEW_H */
