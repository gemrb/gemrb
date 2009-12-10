/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2009 The GemRB Project
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
 * $Id: GetBitContext.cpp 6168 2009-05-28 22:28:33Z mattinm $
 *
 */

// code derived from FFMPEG's libavcodec/get_bits.h 
// copyright (c) 2004 Michael Niedermayer <michaelni@gmx.at> 
// and binkaudio.cpp 
// Copyright (c) 2007-2009 Peter Ross (pross@xvid.org)
// Copyright (c) 2009 Daniel Verkamp (daniel@drv.nu) 

#include <math.h>
#include "GetBitContext.h"

//don't return more than 25 bits this way
unsigned int GetBitContext::get_bits(int n) {
    register unsigned int tmp;

    tmp = AV_RL32(buffer+(index>>3))>>(index&7);
    index+=n;
    return tmp& (0xffffffff>>(32-n));
}

float GetBitContext::get_float()
{
    int power = get_bits(5);
    float f = ldexpf((float) get_bits_long(23), power - 23);
    if (get_bits(1))
        f = -f;
    return f;
}

void GetBitContext::get_bits_align32()
{
    int n = (-get_bits_count()) & 31;
    if (n) skip_bits(n);
}

unsigned int GetBitContext::get_bits_long(int n) 
{
    if(n<=17) return get_bits(n);
    else {
        int ret= get_bits(16);
        return ret | (get_bits(n-16) << 16);
    }
} 

void GetBitContext::init_get_bits(const uint8_t *b, int bit_size)
{
    int buffer_size = (bit_size+7)>>3;
    if(buffer_size < 0 || bit_size < 0) {
        buffer_size = bit_size = 0;
        buffer = NULL;
    }

    buffer = b;
    size_in_bits = bit_size;
    buffer_end = buffer + buffer_size;
    index=0;
}
