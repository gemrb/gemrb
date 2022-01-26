/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2022 The GemRB Project
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

#include "System/CString.h"

namespace GemRB {

/** Returns the length of string (up to a delimiter) */
GEM_EXPORT int strlench(const char* string, char ch)
{
	int i;
	for (i = 0; string[i] && string[i] != ch; i++)
		;
	return i;
}

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

char* strlwr(char* string)
{
	if (string) {
		for (char* s = string; *s; ++s)
			*s = tolower( *s );
	}
	return string;
}

#endif // ! WIN32

} // namespace GemRB
