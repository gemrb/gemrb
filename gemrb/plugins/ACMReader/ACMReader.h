// SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef ACMREADER_H
#define ACMREADER_H

#include "SoundMgr.h"
#include "decoder.h"
#include "unpacker.h"

#include "Streams/DataStream.h"

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
	size_t read_samples(short* buffer, size_t count) override;
	size_t ReadSamplesIntoChannels(char* channel1, char* channel2, size_t numSamples) override;
};

}

#endif
