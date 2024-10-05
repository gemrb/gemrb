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

#include "Streams/DataStream.h"

#include <cstdio>
#include <cstring>
#include <cstdlib>

namespace GemRB {

// IP's ACM files
class ACMReader : public SoundMgr {
private:
	int samples_left = 0; // count of unread samples
	int levels = 0;
	int subblocks = 0;
	int block_size = 0;
	int* block = nullptr;
	int* values = nullptr;
	int samples_ready = 0;
	CValueUnpacker* unpacker = nullptr; // ACM-stream unpacker
	CSubbandDecoder* decoder = nullptr; // IP's subband decoder

	int make_new_samples();
public:
	ACMReader() noexcept = default;
	ACMReader(const ACMReader&) = delete;
	ACMReader& operator=(const ACMReader&) = delete;
	~ACMReader() override
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

	bool Import(DataStream* stream) override;
	int read_samples(short* buffer, int count) override;
	int ReadSamplesIntoChannels(char *channel1, char *channel2, int numSamples) override;
};

}

#endif
