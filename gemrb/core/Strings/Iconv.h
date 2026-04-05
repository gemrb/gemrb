// SPDX-FileCopyrightText: 2023 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

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
