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
 */

#ifndef ACMREADER_H
#define ACMREADER_H

#include "SoundMgr.h"

#include "decoder.h"
#include "general.h"
#include "unpacker.h"

#include "System/DataStream.h"

#include <cstdio>
#include <cstring>
#include <cstdlib>

namespace GemRB {

// IP's ACM files
class ACMReader : public SoundMgr {
private:
	int samples_left; // count of unread samples
	int levels, subblocks;
	int block_size;
	int* block, * values;
	int samples_ready;
	CValueUnpacker* unpacker; // ACM-stream unpacker
	CSubbandDecoder* decoder; // IP's subband decoder

	int make_new_samples();
public:
	ACMReader()
		: samples_left( 0 ), block( NULL ), values( NULL ),
		samples_ready( 0 ), unpacker( NULL ), decoder( NULL )
	{
	}
	virtual ~ACMReader()
	{
		Close();
	}
	void Close()
	{
		if (block) {
			free(block);
		}
		if (unpacker) {
			delete unpacker;
		}
		if (decoder) {
			delete decoder;
		}
	}

	bool Open(DataStream* stream);
	virtual int read_samples(short* buffer, int count);
};

}

#endif
