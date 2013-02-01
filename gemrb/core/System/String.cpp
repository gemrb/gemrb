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

unsigned char pl_uppercase[256];
unsigned char pl_lowercase[256];

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
	strncpy( string, s, l );
	string[l] = 0;
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
		while (*src++);
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
