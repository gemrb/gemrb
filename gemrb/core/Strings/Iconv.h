/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2023 The GemRB Project
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

// work around the difference between the POSIX and SUSv2 iconv signatures
// char** cannot be always safely cast to const char**, so it's not allowed implicitly

#ifndef G_ICONV_H
#define G_ICONV_H

#include <cstddef>
#include <iconv.h>

#if defined(__NetBSD__)
	#include <sys/param.h>
	#if __NetBSD_Prereq__(9, 99, 17)
		#define NETBSD_POSIX_ICONV 1
	#else
		#define NETBSD_POSIX_ICONV 0
	#endif
#endif
#if (defined(__NetBSD__) && !NETBSD_POSIX_ICONV) || defined(__sun)
	#define ICONV_CONST_INBUF 1
#endif

namespace GemRB {

inline size_t portableIconv(iconv_t cd,
			    char** inbuf, size_t* inbytesleft,
			    char** outbuf, size_t* outbytesleft)
{
	size_t ret;
#ifdef ICONV_CONST_INBUF
	ret = iconv(cd, (const char**) inbuf, inbytesleft, outbuf, outbytesleft);
#else
	ret = iconv(cd, inbuf, inbytesleft, outbuf, outbytesleft);
#endif
	return ret;
}

inline size_t portableIconv(iconv_t cd,
			    const char** inbuf, size_t* inbytesleft,
			    char** outbuf, size_t* outbytesleft)
{
	size_t ret;
#ifdef ICONV_CONST_INBUF
	ret = iconv(cd, inbuf, inbytesleft, outbuf, outbytesleft);
#else
	ret = iconv(cd, const_cast<char**>(inbuf), inbytesleft, outbuf, outbytesleft);
#endif
	return ret;
}

} // namespace GemRB

#endif /* G_ICONV_H */
