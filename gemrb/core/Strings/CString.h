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

#ifndef WIN32
# define stricmp strcasecmp
# define strnicmp strncasecmp
#endif

namespace GemRB {

#ifndef HAVE_STRNLEN
GEM_EXPORT int strnlen(const char* string, int maxlen);
#endif
#ifndef HAVE_STRLCPY
GEM_EXPORT size_t strlcpy(char *d, const char *s, size_t l);
#endif

GEM_EXPORT int strlench(const char* string, char ch);

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

// SFINAE garbage to only enable funtions for strings of known size
// i'm sure its not perfect, but it meets our needs
#define ENABLE_CHAR_RANGE typename std::enable_if<(!std::is_fundamental<STR>::value && !std::is_pointer<STR>::value) || std::is_same<STR, decltype("")>::value, int>::type = 0

template<size_t LEN, int(*CMP)(const char*, const char*, size_t) = strncmp>
class FixedSizeString {
	// we use uint8_t and an object of massive size is not wanted
	static_assert(LEN < 255, "Cannot create FixedSizeString larger than 255 characters");
	char str[LEN + 1] {'\0'};
	
public:
	using index_t = uint8_t;
	static constexpr index_t Size = LEN;
	using iterator = char*;
	using const_iterator = const char*;
	
	FixedSizeString() noexcept = default;
	FixedSizeString(std::nullptr_t) noexcept = delete;
	FixedSizeString& operator=(std::nullptr_t) noexcept = delete;
	
	explicit FixedSizeString(const char* cstr) noexcept {
		if (cstr) {
			strncpy(str, cstr, LEN);
		} else {
			std::fill(begin(), end(), '\0');
		}
	}
	
	template<typename STR, ENABLE_CHAR_RANGE>
	FixedSizeString(const STR& s) noexcept {
		strncpy(str, &s[0], LEN);
	}

	template<typename STR, ENABLE_CHAR_RANGE>
	FixedSizeString& operator=(const STR& s) noexcept {
		strncpy(str, &s[0], LEN);
		return *this;
	}
	
	FixedSizeString(const FixedSizeString&) noexcept = default;
	FixedSizeString& operator=(const FixedSizeString&) noexcept = default;
	
	// remove trailing spaces
	index_t RTrim() {
		index_t len = CStrLen();
		index_t i = 0;
		for (; i < len; ++i) {
			index_t idx = len - i - 1;
			if (std::isspace(str[idx])) str[idx] = '\0';
			else break;
		}

		return len - i;
	}
	
	int SNPrintF(const char* format, ...) noexcept {
		va_list args;
		va_start(args, format);
		int ret = vsnprintf(str, sizeof(str), format, args);
		va_end(args);
		return ret;
	}
	
	template <typename STR>
	void Append(const STR& s) {
		index_t len = CStrLen();
		strncpy(str + len, std::begin(s), LEN - len);
	}

	template<typename T>
	typename std::enable_if<std::is_integral<T>::value, const char&>::type
	operator[](T i) const noexcept {
		return str[i];
	}

	template<typename T>
	typename std::enable_if<std::is_integral<T>::value, char&>::type
	operator[](T i) noexcept {
		return str[i];
	}
	
	bool operator==(const char* cstr) const noexcept {
		return CMP(str, cstr, LEN) == 0;
	}
	
	bool operator==(const FixedSizeString& other) const noexcept {
		return CMP(str, other.str, LEN) == 0;
	}
	
	bool operator!=(const char* cstr) const noexcept {
		return CMP(str, cstr, LEN) != 0;
	}
	
	bool operator!=(const FixedSizeString& other) const noexcept {
		return CMP(str, other.str, LEN) != 0;
	}
	
	bool operator<(const char* cstr) const noexcept {
		return CMP(str, cstr, LEN) < 0;
	}
	
	bool operator<(const FixedSizeString& other) const noexcept {
		return CMP(str, other.str, LEN) < 0;
	}
	
	bool StartsWith(const char* cstr, index_t len) const noexcept {
		return CMP(str, cstr, len) == 0;
	}
	
	index_t CStrLen() const noexcept {
		return strnlen(str, sizeof(str));
	}
	
	void Reset() noexcept {
		std::fill(begin(), end(), '\0');
	}
	
	const char* CString() const noexcept {
		return str;
	}
	
	bool IsEmpty() const noexcept {
		return str[0] == '\0';
	}
	
	operator const char*() const noexcept {
		return str;
	}
	
	explicit operator bool() const noexcept {
		return str[0] != '\0';
	}
	
	const_iterator begin() const noexcept {
		return str;
	}
	
	const_iterator end() const noexcept {
		return &str[LEN + 1];
	}
	
	iterator begin() noexcept {
		return str;
	}
	
	iterator end() noexcept {
		return &str[LEN + 1];
	}

	template<index_t N> FixedSizeString<N, CMP> SubStr(index_t pos) {
		static_assert(N <= LEN, "Substring must not exceed the original length.");
		assert(pos + LEN <= N);
		return FixedSizeString<N, CMP>{CString() + pos};
	}
};

}

#endif
