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

#include <algorithm>
#include <cassert>
#include <cstdarg>
#include <cstring>
#include <cwctype>
#include <string>

#ifndef WIN32
# define stricmp strcasecmp
# define strnicmp strncasecmp
#endif

#define WHITESPACE_STRING L"\n\t\r "

namespace GemRB {

extern GEM_EXPORT unsigned char pl_uppercase[256];
extern GEM_EXPORT unsigned char pl_lowercase[256];

#ifdef HAS_WSTRING
using String = std::wstring;
#else
using String = std::basic_string<wchar_t>;
#endif

// String creators
GEM_EXPORT char* ConvertCharEncoding(const char * string, const char * from, const char* to);
GEM_EXPORT String* StringFromCString(const char* string);
GEM_EXPORT char* MBCStringFromString(const String& string);

// char manipulators
inline wchar_t tolower(wchar_t c) { return ::towlower(c); }
inline wchar_t toupper(wchar_t c) { return ::towupper(c); }

template <typename T>
GEM_EXPORT_T T ToLower(T c) {
	if (c < 256) {
		return pl_lowercase[static_cast<unsigned char>(c)];
	} else {
		return tolower(c);
	}
}

template <typename T>
GEM_EXPORT_T T ToUpper(T c) {
	if (c < 256) {
		return pl_uppercase[static_cast<unsigned char>(c)];
	} else {
		return towupper(c);
	}
}

// String manipulators
template <typename T>
GEM_EXPORT_T void StringToLower(T& string) {
	for (size_t i = 0; i < string.length(); i++) {
		string[i] = ToLower(string[i]);
	}
}

template <typename T>
GEM_EXPORT_T void StringToUpper(T& string) {
	for (size_t i = 0; i < string.length(); i++) {
		string[i] = ToUpper(string[i]);
	}
}

GEM_EXPORT void TrimString(String& string);

/* these functions will work with pl/cz special characters */
GEM_EXPORT void strnlwrcpy(char* d, const char *s, int l, bool pad = true);
GEM_EXPORT void strnuprcpy(char* d, const char *s, int l);
GEM_EXPORT void strnspccpy(char* d, const char *s, int l, bool upper = false);
GEM_EXPORT int strlench(const char* string, char ch);
}

#ifndef HAVE_STRNLEN
GEM_EXPORT int strnlen(const char* string, int maxlen);
#endif
#ifndef HAVE_STRLCPY
GEM_EXPORT size_t strlcpy(char *d, const char *s, size_t l);
#endif

#ifndef WIN32
GEM_EXPORT char* strlwr(char* string);
#endif

constexpr int NoTransform(int c) { return c; }

template <typename STR_T, int(*TRANS)(int) = NoTransform>
struct CstrHash
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
using CstrHashCI = CstrHash<STR_T, tolower>;

template<size_t LEN, int(*CMP)(const char*, const char*, size_t) = strncmp>
class FixedSizeString {
	char str[LEN + 1] {'\0'};
	
public:
	FixedSizeString() noexcept = default;
	FixedSizeString(std::nullptr_t) noexcept = delete;
	FixedSizeString& operator=(std::nullptr_t) noexcept = delete;
	
	FixedSizeString(const char* cstr) noexcept {
		operator=(cstr);
	}
	
	FixedSizeString& operator=(const char* c) noexcept {
		if (c) {
			strncpy(str, c, LEN);
		} else {
			std::fill(begin(), end(), '\0');
		}
		return *this;
	}
	
	FixedSizeString(const FixedSizeString&) noexcept = default;
	FixedSizeString& operator=(const FixedSizeString&) noexcept = default;
	
	template<typename T>
	typename std::enable_if<std::is_integral<T>::value, char>::type
	operator[](T i) const noexcept {
		assert(i < static_cast<T>(LEN));
		return str[i];
	}
	
	template<typename T>
	typename std::enable_if<std::is_integral<T>::value, char&>::type
	operator[](T i) noexcept {
		assert(i < static_cast<T>(LEN));
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
	
	bool StartsWith(const char* cstr, size_t len) const noexcept {
		return CMP(str, cstr, len) == 0;
	}
	
	uint8_t CStrLen() const noexcept {
		return strnlen(str, sizeof(str));
	}
	
	void Reset() noexcept {
		std::fill(begin(), end(), '\0');
	}
	
	void SNPrintF(const char* format, ...) noexcept {
		va_list args;
		va_start(args, format);
		vsnprintf(str, sizeof(str), format, args);
		va_end(args);
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
	
	const char* begin() const noexcept {
		return str;
	}
	
	const char* end() const noexcept {
		return &str[LEN + 1];
	}
	
	char* begin() noexcept {
		return str;
	}
	
	char* end() noexcept {
		return &str[LEN + 1];
	}
};

#endif
