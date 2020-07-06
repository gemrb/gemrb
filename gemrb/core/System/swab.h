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
#ifndef IE_SWAB_H
#define IE_SWAB_H

#include "exports.h"

#if defined(__cplusplus)
extern "C" {
#endif

#ifndef HAVE_SIZEOF_SSIZE_T
# define HAVE_SIZEOF_SSIZE_T
 typedef long int ssize_t;
#endif

GEM_EXPORT void swabs(void *buf, ssize_t n);

#if defined(__cplusplus)
}  /* extern "C" */
#endif
#endif
