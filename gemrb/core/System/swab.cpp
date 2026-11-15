// SPDX-FileCopyrightText: Copyright (C) 1992, 1996, 1997 Free Software Foundation, Inc.
// SPDX-FileCopyrightText: 2010 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "swab.h"

#include <cstdint>

bool IsBigEndian()
{
	const static uint16_t endiantest = 1;
	const static bool isBigEndian = ((char*) &endiantest)[1] == 1;

	return isBigEndian;
}
