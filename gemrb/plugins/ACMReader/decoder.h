// SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef _ACM_LAB_SUBBAND_DECODER_H
#define _ACM_LAB_SUBBAND_DECODER_H

#include <cstdlib>

class CSubbandDecoder {
private:
	int levels, block_size;
	int* memory_buffer = nullptr;
	void sub_4d3fcc(short* memory, int* buffer, int sb_size, int blocks) const;
	void sub_4d420c(int* memory, int* buffer, int sb_size, int blocks) const;

public:
	explicit CSubbandDecoder(int lev_cnt)
		: levels(lev_cnt), block_size(1 << lev_cnt)
	{
	}
	virtual ~CSubbandDecoder()
	{
		if (memory_buffer) {
			free(memory_buffer);
		}
	}

	int init_decoder();
	void decode_data(int* buffer, int blocks);
};

#endif
