/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "System/Logging.h"
#include "System/String.h"

#include "exports.h"

#include <stdlib.h>
#include <ctype.h>
#ifdef WIN32
#include "win32def.h"
#ifdef _DEBUG
#include <crtdbg.h>
#endif
#endif

namespace GemRB {

String* StringFromEncodedData(const ieByte* string, bool multibyte, bool utf8)
{
	// i know these params are strange
	// but utf8 implies multibyte
	multibyte = (utf8) ? true : multibyte;

	size_t len = strlen((char*)string);
	String* dbString = new String();
	dbString->reserve(len);
	size_t dbLen = 0;
	for(size_t i=0; i<len; ++i) {
		ieWord currentChr = string[i];
		// we are assuming that every multibyte encoding uses single bytes for chars 32 - 127
		if(multibyte && (i+1 < len) && (currentChr >= 128 || currentChr < 32)) { // this is a double byte char
			ieWord ch = 0;
			if (utf8) {
				size_t nb = 0;
				if (currentChr >= 0xC0 && currentChr <= 0xDF) {
					/* c0-df are first byte of two-byte sequences (5+6=11 bits) */
					/* c0-c1 are noncanonical */
					nb = 2;
				} else if (currentChr >= 0xE0 && currentChr <= 0XEF) {
					/* e0-ef are first byte of three-byte (4+6+6=16 bits) */
					/* e0 80-9f are noncanonical */
					nb = 3;
				} else if (currentChr >= 0xF0 && currentChr <= 0XF7) {
					/* f0-f7 are first byte of four-byte (3+6+6+6=21 bits) */
					/* f0 80-8f are noncanonical */
					nb = 4;
				} else if (currentChr >= 0xF8 && currentChr <= 0XFB) {
					/* f8-fb are first byte of five-byte (2+6+6+6+6=26 bits) */
					/* f8 80-87 are noncanonical */
					nb = 5;
				} else if (currentChr >= 0xFC && currentChr <= 0XFD) {
					/* fc-fd are first byte of six-byte (1+6+6+6+6+6=31 bits) */
					/* fc 80-83 are noncanonical */
					nb = 6;
				} else {
					Log(WARNING, "String", "Invalid UTF-8 character: %x", currentChr);
					continue;
				}

				ch = currentChr & ((1 << (7 - nb)) - 1);
				while (--nb)
					ch <<= 6, ch |= string[++i] & 0x3f;
			} else {
				ch = (string[++i] << 8) + currentChr;
			}
			dbString->push_back(ch);
		} else {
			dbString->push_back(currentChr);
		}
		++dbLen;
	}

	// we dont always use everything we allocated.
	// realloc in this case to avoid static anylizer warnings about "garbage values"
	// since this realloc always truncates it *should* be quick
	dbString->resize(dbLen);
	return dbString;
}

String* StringFromCString(const char* string)
{
	// basic expansion of cstring to 2 bytes
	// the only reason this is special, is because it allows characters 128-256.
	return StringFromEncodedData((ieByte*)string, false, false);
}

String* StringFromMBEncodedData(const ieByte* data, bool utf8)
{
	return StringFromEncodedData(data, true, utf8);
}

unsigned char pl_uppercase[256];
unsigned char pl_lowercase[256];

void StringToLower(String& string)
{
	for (size_t i = 0; i < string.length(); i++) {
		if (string[i] < 256) {
			string[i] = pl_lowercase[string[i]];
		} else {
			string[i] = ::towlower(string[i]);
		}
	}
}

void StringToUpper(String& string)
{
	for (size_t i = 0; i < string.length(); i++) {
		if (string[i] < 256) {
			string[i] = pl_uppercase[string[i]];
		} else {
			string[i] = ::towupper(string[i]);
		}
	}
}

// these 3 functions will copy a string to a zero terminated string with a maximum length
void strnlwrcpy(char *dest, const char *source, int count, bool pad)
{
	while(count--) {
		*dest++ = pl_lowercase[(unsigned char) *source];
		if(!*source++) {
			if (!pad)
				return;
			while(count--) *dest++=0;
			break;
		}
	}
	*dest=0;
}

void strnuprcpy(char* dest, const char *source, int count)
{
	while(count--) {
		*dest++ = pl_uppercase[(unsigned char) *source];
		if(!*source++) {
			while(count--) *dest++=0;
			break;
		}
	}
	*dest=0;
}

// this one also filters spaces, used to copy resrefs and variables
void strnspccpy(char* dest, const char *source, int count, bool upper)
{
	memset(dest,0,count);
	while(count--) {
		char c;
		if (upper)
			c = pl_uppercase[(unsigned char) *source];
		else
			c = pl_lowercase[(unsigned char) *source];
		if (c!=' ') {
			*dest++=c;
		}
		if(!*source++) {
			return;
		}
	}
}

/** Convert string to uppercase in-place using selected IE encoding */
char* strtoupper(char* string)
{
	char* s;
	if (string) {
		for (s = string; *s; ++s)
			*s = pl_uppercase[(unsigned char)*s];
	}
	return string;
}

/** Convert string to lowercase in-place using selected IE encoding */
char* strtolower(char* string)
{
	char* s;
	if (string) {
		for (s = string; *s; ++s)
			*s = pl_lowercase[(unsigned char)*s];
	}
	return string;
}

/** Returns the length of string (up to a delimiter) */
GEM_EXPORT int strlench(const char* string, char ch)
{
	int i;
	for (i = 0; string[i] && string[i] != ch; i++)
		;
	return i;
}

} // namespace GemRB

#ifndef HAVE_STRNLEN
int strnlen(const char* string, int maxlen)
{
	if (!string) {
		return -1;
	}
	int i = 0;
	while (maxlen-- > 0) {
		if (!string[i])
			break;
		i++;
	}
	return i;
}
#endif // ! HAVE_STRNLEN

//// Compatibility functions
#ifndef HAVE_STRNDUP
GEM_EXPORT char* strndup(const char* s, size_t l)
{
	size_t len = strlen( s );
	if (len < l) {
		l = len;
	}
	char* string = ( char* ) malloc( l + 1 );
	strlcpy( string, s, l + 1 );
	return string;
}
#endif

#ifndef HAVE_STRLCPY
GEM_EXPORT size_t strlcpy(char *d, const char *s, size_t l)
{
	char *dst = d;
	const char *src = s;

	if (l != 0) {
		while (--l != 0) {
			if ((*dst++ = *src++) == '\0')
				break;
		}
		if (l == 0)
			*dst = '\0';
	}

	if (l == 0)
		while (*src++) ;
	return src - s - 1; /* length of source, excluding NULL */
}
#endif

#ifdef WIN32

#else

char* strupr(char* string)
{
	char* s;
	if (string) {
		for (s = string; *s; ++s)
			*s = toupper( *s );
	}
	return string;
}

char* strlwr(char* string)
{
	char* s;
	if (string) {
		for (s = string; *s; ++s)
			*s = tolower( *s );
	}
	return string;
}

#endif // ! WIN32
