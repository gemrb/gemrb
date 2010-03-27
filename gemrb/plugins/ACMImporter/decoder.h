/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * $Id$
 *
 */

#ifndef _ACM_LAB_SUBBAND_DECODER_H
#define _ACM_LAB_SUBBAND_DECODER_H

#include <stdlib.h>

class CSubbandDecoder {
private:
	int levels, block_size;
	int* memory_buffer;
	void sub_4d3fcc(short* memory, int* buffer, int sb_size, int blocks);
	void sub_4d420c(int* memory, int* buffer, int sb_size, int blocks);
public:
	CSubbandDecoder(int lev_cnt)
		: levels( lev_cnt ), block_size( 1 << lev_cnt ), memory_buffer( NULL )
	{
	}
	virtual ~CSubbandDecoder()
	{
		if (memory_buffer) {
			free( memory_buffer );
		}
	}

	int init_decoder();
	void decode_data(int* buffer, int blocks);
};

#endif
