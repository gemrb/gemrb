// SPDX-FileCopyrightText: Copyright (C) 1992, 1996, 1997 Free Software Foundation, Inc.
// SPDX-FileCopyrightText: 2010 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

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
