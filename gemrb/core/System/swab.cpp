/* Copyright (C) 1992, 1996, 1997 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307 USA.  */

#include "swab.h"

#include <cstdint>

// we use this because it works with overlapping buffers (undefined in POSIX)
void swab_const(const void* bfrom, void* bto, long n)
{
	const char* from = (const char*) bfrom;
	char* to = (char*) bto;

	n &= ~((long) 1);
	while (n > 1) {
		const char b0 = from[--n];
		const char b1 = from[--n];
		to[n] = b0;
		to[n + 1] = b1;
	}
}

void swabs(void* buf, long n)
{
	swab_const(buf, buf, n);
}

bool IsBigEndian()
{
	const static uint16_t endiantest = 1;
	const static bool isBigEndian = ((char*) &endiantest)[1] == 1;

	return isBigEndian;
}
