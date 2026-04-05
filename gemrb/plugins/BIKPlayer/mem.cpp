// SPDX-FileCopyrightText: Copyright (c) 2002 Fabrice Bellard
// SPDX-FileCopyrightText: FFmpeg project <https://ffmpeg.org>
// SPDX-FileCopyrightText: 2009 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

// @file libavutil/mem.c
// default memory allocator for libavutil

#include "common.h"

#include <cstdlib>

/* You can redefine av_malloc and av_free in your project to use your
   memory allocator. You do not need to suppress this file because the
   linker will do it automatically. */

void* av_malloc(unsigned int size)
{
	void* ptr = NULL;

	/* let's disallow possibly ambiguous cases */
	if (size > (INT_MAX - 16) || !size)
		return NULL;

#if HAVE_POSIX_MEMALIGN
	if (posix_memalign(&ptr, 16, size))
		ptr = NULL;
#elif HAVE_ALIGNED_MALLOC
	ptr = _aligned_malloc(size, 16);
#elif HAVE_MEMALIGN
	ptr = memalign(16, size);
#else
	ptr = malloc(size + 16);
	if (!ptr)
		return ptr;
	long diff = ((-(size_t) ptr - 1) & 15) + 1;
	ptr = (char*) ptr + diff;
	((char*) ptr)[-1] = diff;
#endif

	return ptr;
}

void av_free(void* ptr)
{
	/* XXX: this test should not be needed on most libcs */
	if (ptr)
#if !defined(HAVE_POSIX_MEMALIGN) && !defined(HAVE_ALIGNED_MALLOC) && !defined(HAVE_MEMALIGN)
		free((char*) ptr - ((char*) ptr)[-1]);
#elif HAVE_ALIGNED_MALLOC
		_aligned_free(ptr);
#else
		free(ptr);
#endif
}

void av_freep(void** arg)
{
	av_free(*arg);
	*arg = NULL;
}
