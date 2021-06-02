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
#include "ie_types.h"

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

//typedef std::basic_string<ieWord> String;
typedef std::wstring String;

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

#endif
