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
#include <string>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifndef WIN32
# define stricmp strcasecmp
# define strnicmp strncasecmp
#endif

#define WHITESPACE_STRING L"\n\t\r "

namespace GemRB {

//typedef std::basic_string<ieWord> String;
typedef std::wstring String;
// String creators
GEM_EXPORT char* ConvertCharEncoding(const char * string, const char * from, const char* to);
GEM_EXPORT String* StringFromCString(const char* string);
GEM_EXPORT char* MBCStringFromString(const String& string);

// String manipulators
GEM_EXPORT void StringToLower(String& string);
GEM_EXPORT void StringToUpper(String& string);
GEM_EXPORT void TrimString(String& string);

/* this function will work with pl/cz special characters */

extern unsigned char pl_uppercase[256];
extern unsigned char pl_lowercase[256];

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
