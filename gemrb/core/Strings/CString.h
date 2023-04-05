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

#ifndef CSTRING_H
#define CSTRING_H

#include "exports.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cwctype>

#include "Format.h"
#include "StringView.h"

#ifndef WIN32
# define stricmp strcasecmp
# define strnicmp strncasecmp
#endif

namespace GemRB {

#ifndef HAVE_STRLCPY
GEM_EXPORT size_t strlcpy(char *d, const char *s, size_t l);
#endif

constexpr int NoTransform(int c) { return c; }

template <typename STR_T, int(*TRANS)(int) = NoTransform>
struct GEM_EXPORT CstrHash
{
	size_t operator() (const STR_T &str) const {
		size_t nHash = 0;
		for (const auto& c : str) {
			if (c == '\0')
				break;
			nHash = (nHash << 5) ^ TRANS(c);
		}
		return nHash;
	}
};

template <typename STR_T>
using CstrHashCI = CstrHash<STR_T, std::tolower>;

template<size_t LEN, int(*CMP)(const char*, const char*, size_t) = strncmp>
class FixedSizeString {
	// we use uint8_t and an object of massive size is not wanted
	static_assert(LEN < 255, "Cannot create FixedSizeString larger than 255 characters");
	char str[LEN + 1] {'\0'};
	
public:
	using size_type = uint8_t;
	using iterator = char*;
	using const_iterator = const char*;
	using value_type = char;
	
	static constexpr size_type Size = LEN;
	static constexpr size_type npos = size_type(-1);
	
	FixedSizeString() noexcept = default;
	FixedSizeString(std::nullptr_t) noexcept = delete;
	FixedSizeString& operator=(std::nullptr_t) noexcept = delete;
	
	explicit FixedSizeString(const char* cstr, size_type len = LEN) noexcept {
		if (cstr) {
			assert(len <= LEN);
			strncpy(str, cstr, len);
		} else {
			std::fill(begin(), bufend(), '\0');
		}
	}
	
	template<typename STR, ENABLE_CHAR_RANGE(STR)>
	FixedSizeString(const STR& s) noexcept {
		strncpy(str, &s[0], LEN);
	}

	template<typename STR, ENABLE_CHAR_RANGE(STR)>
	FixedSizeString& operator=(const STR& s) noexcept {
		strncpy(str, &s[0], LEN);
		return *this;
	}
	
	FixedSizeString(const FixedSizeString&) noexcept = default;
	FixedSizeString& operator=(const FixedSizeString&) noexcept = default;
	
	FixedSizeString& erase(size_type pos, size_type len = npos) noexcept {
		// note that erasing from the beginning may not behave as expected
		len = std::min<size_type>(len, LEN - pos);
		std::fill_n(&str[pos], len, '\0');
		return *this;
	}
	
	template<typename ...ARGS>
	bool Format(ARGS&& ...args) noexcept {
		auto ret = fmt::format_to_n(begin(), LEN + 1, std::forward<ARGS>(args)...);
		if (ret.size > LEN) {
			str[LEN] = '\0';
			return false;
		}
		*ret.out = '\0';
		return true;
	}
	
	template <typename STR>
	void Append(const STR& s) {
		size_type len = length();
		strncpy(str + len, std::begin(s), LEN - len);
	}

	const char& operator[](size_type i) const noexcept {
		return str[i];
	}

	char& operator[](size_type i) noexcept {
		return str[i];
	}
	
	bool operator==(StringView sv) const noexcept {
		if (sv.length() != length()) {
			return false;
		}
		return CMP(str, sv.begin(), sv.length()) == 0;
	}
	
	bool operator!=(StringView sv) const noexcept {
		return !operator==(sv);
	}
	
	bool operator<(StringView sv) const noexcept {
		auto max = std::min(sv.length(), LEN);
		return CMP(str, sv.begin(), max) < 0;
	}
	
	bool BeginsWith(StringView sv) const noexcept {
		if (sv.length() > LEN) {
			return false;
		}
		return CMP(str, sv.begin(), sv.length()) == 0;
	}
	
	size_type length() const noexcept {
		return strnlen(str, sizeof(str));
	}
	
	bool empty() const noexcept {
		return length() == 0;
	}

	void clear() noexcept {
		std::fill(begin(), bufend(), '\0');
	}

	void Reset() noexcept {
		std::fill(begin(), bufend(), '\0');
	}
	
	const char* c_str() const noexcept {
		return str;
	}
	
	bool IsEmpty() const noexcept {
		return str[0] == '\0';
	}
	
	explicit operator bool() const noexcept {
		return str[0] != '\0';
	}
	
	const_iterator begin() const noexcept {
		return str;
	}
	
	const_iterator end() const noexcept {
		return begin() + length();
	}
	
	const_iterator bufend() const noexcept {
		return &str[LEN];
	}
	
	std::reverse_iterator<const_iterator> rbegin() const noexcept {
		return std::reverse_iterator<const_iterator>(end());
	}
	
	std::reverse_iterator<const_iterator> rend() const noexcept {
		return std::reverse_iterator<const_iterator>(begin());
	}
	
	iterator begin() noexcept {
		return str;
	}
	
	iterator end() noexcept {
		return begin() + length();
	}
	
	iterator bufend() noexcept {
		return &str[LEN];
	}
	
	std::reverse_iterator<iterator> rbegin() noexcept {
		return std::reverse_iterator<iterator>(end());
	}
	
	std::reverse_iterator<iterator> rend() noexcept {
		return std::reverse_iterator<iterator>(begin());
	}
};

}

#endif
