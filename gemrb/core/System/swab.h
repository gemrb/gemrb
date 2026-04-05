// SPDX-FileCopyrightText: Copyright (C) 1992, 1996, 1997 Free Software Foundation, Inc.
// SPDX-FileCopyrightText: 2010 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef IE_SWAB_H
#define IE_SWAB_H

#include "exports.h"

#if defined(__cplusplus)
extern "C"
{
#endif

	GEM_EXPORT bool IsBigEndian();
	GEM_EXPORT void swab_const(const void* bfrom, void* bto, long n);
	GEM_EXPORT void swabs(void* buf, long n);

#if defined(__cplusplus)
} /* extern "C" */
#endif
#endif
