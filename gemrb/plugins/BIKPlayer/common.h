// SPDX-FileCopyrightText: Michael Niedermayer <michaelni@gmx.at>
// SPDX-FileCopyrightText: FFmpeg project <https://ffmpeg.org>
// SPDX-FileCopyrightText: 2009 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef AVUTIL_COMMON_H
#define AVUTIL_COMMON_H

#include "globals.h"

#include <cctype>
#include <cerrno>
#include <climits>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#define av_const
#define av_cold
#define av_flatten
#define attribute_deprecated
#define av_unused
#define av_uninit(x)     x
#define av_always_inline inline

void* av_malloc(unsigned int size);
void av_free(void* ptr);
void av_freep(void** ptr);

/**
 * data needed to decode 4-bit Huffman-coded value 
 */
typedef struct Tree {
	int vlc_num; ///< tree number (in bink_trees[])
	uint8_t syms[16]; ///< leaf value to symbol mapping
} Tree;

#endif /* AVUTIL_COMMON_H */
