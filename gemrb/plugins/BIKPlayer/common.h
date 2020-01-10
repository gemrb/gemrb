/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
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
 *
 *
 * Code derived from FFMPeg/libavutil/common.h
 * @author Michael Niedermayer <michaelni@gmx.at>
 */

#ifndef AVUTIL_COMMON_H
#define AVUTIL_COMMON_H

#include "win32def.h"
#include "globals.h"

#include <ctype.h>
#include <cstdint>
#include <errno.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
 
#define av_const
#define av_cold
#define av_flatten
#define attribute_deprecated
#define av_unused
#define av_uninit(x) x
#define av_always_inline inline 

#define FFABS(a) ((a) >= 0 ? (a) : (-(a)))
#define FFSWAP(type,a,b) do{type SWAP_tmp= b; b= a; a= SWAP_tmp;}while(0)

#ifndef M_PI
#define M_PI           3.14159265358979323846  /* pi */
#endif

#ifndef M_SQRT1_2
#define M_SQRT1_2      0.70710678118654752440  /* 1/sqrt(2) */
#endif

void *av_malloc(unsigned int size);
void av_free(void *ptr);
void av_freep(void **ptr);

/**
 * data needed to decode 4-bit Huffman-coded value 
 */
typedef struct Tree {
    int     vlc_num;  ///< tree number (in bink_trees[])
    uint8_t syms[16]; ///< leaf value to symbol mapping
} Tree;
  
#endif /* AVUTIL_COMMON_H */
