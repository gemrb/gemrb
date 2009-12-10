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
 * $Id: GetBitContext.h 6168 2009-05-28 22:28:33Z mattinm $
 *
 */

//code derived from FFMPEG
 
#include "common.h"


#define AV_RL32(x)                           \
    ((((const uint8_t*)(x))[3] << 24) |         \
     (((const uint8_t*)(x))[2] << 16) |         \
     (((const uint8_t*)(x))[1] <<  8) |         \
      ((const uint8_t*)(x))[0]) 

class GetBitContext
{
public:
    const uint8_t *buffer, *buffer_end;
    int index;
    int size_in_bits;
public:
    float get_float();
    void skip_bits(int x) { index+=x; }
    int get_bits_count() { return index; }
    void get_bits_align32();
    unsigned int get_bits(int x);
    unsigned int get_bits_long(int n);
    void init_get_bits(const uint8_t *b, int bit_size);
};
