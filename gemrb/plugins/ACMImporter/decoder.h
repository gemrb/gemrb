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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/ACMImporter/decoder.h,v 1.3 2004/02/24 22:20:37 balrog994 Exp $
 *
 */

#ifndef _ACM_LAB_SUBBAND_DECODER_H
#define _ACM_LAB_SUBBAND_DECODER_H

#include <stdlib.h>

class CSubbandDecoder {
private:
	int levels, block_size;
	long* memory_buffer;
	void sub_4d3fcc(short* memory, long* buffer, int sb_size, int blocks);
	void sub_4d420c(long* memory, long* buffer, int sb_size, int blocks);
public:
	CSubbandDecoder(int lev_cnt)
		: levels( lev_cnt ), block_size( 1 << lev_cnt ), memory_buffer( NULL )
	{
	};
	virtual ~CSubbandDecoder()
	{
		if (memory_buffer) {
			free( memory_buffer );
		}
	};

	int init_decoder();
	void decode_data(long* buffer, int blocks);
};

#endif
