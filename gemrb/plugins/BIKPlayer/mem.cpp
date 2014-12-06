/*
 * default memory allocator for libavutil
 * Copyright (c) 2002 Fabrice Bellard
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

/**
 * @file libavutil/mem.c
 * default memory allocator for libavutil
 */

#include <limits.h>
#include <stdlib.h>
#include <string.h>
#if HAVE_MALLOC_H
#include <malloc.h>
#endif
#include "common.h"

/* You can redefine av_malloc and av_free in your project to use your
   memory allocator. You do not need to suppress this file because the
   linker will do it automatically. */

void *av_malloc(unsigned int size)
{
	void *ptr = NULL;
	long diff;

	/* let's disallow possible ambiguous cases */
	if(size > (INT_MAX-16) )
		return NULL;

	ptr = malloc(size+16);
	if(!ptr)
		return ptr;
	diff= ((-(long)ptr - 1)&15) + 1;
	ptr = (char*)ptr + diff;
	((char*)ptr)[-1]= diff;
	return ptr;
}

void av_free(void *ptr)
{
	/* XXX: this test should not be needed on most libcs */
	if (ptr)
		free((char*)ptr - ((char*)ptr)[-1]);
}

void av_freep(void **arg)
{
	av_free(*arg);
	*arg = NULL;
}
