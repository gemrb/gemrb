#ifndef _ACM_LAB_SUBBAND_DECODER_H
#define _ACM_LAB_SUBBAND_DECODER_H

#include <stdlib.h>

class CSubbandDecoder {
private:
	int levels, block_size;
	long* memory_buffer;
	void sub_4d3fcc (short* memory, long* buffer, int sb_size, int blocks);
	void sub_4d420c (long* memory, long* buffer, int sb_size, int blocks);
public:
	CSubbandDecoder (int lev_cnt)
		: levels (lev_cnt),
		block_size ( 1 << lev_cnt ),
		memory_buffer (NULL) {};
	virtual ~CSubbandDecoder() { if (memory_buffer) free (memory_buffer); };

	int init_decoder();
	void decode_data (long* buffer, int blocks);
};

#endif
