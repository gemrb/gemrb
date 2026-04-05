// SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef WAVREADER_H
#define WAVREADER_H

#include "SoundMgr.h"

#include "Streams/DataStream.h"

#include <cstdio>

namespace GemRB {

// RAW file reader
class RawPCMReader : public SoundMgr {
protected:
	// one sample consists of
	// channels * (is16bit ? 2 : 1) bytes
	size_t samplesLeft = 0; // count of unread samples
	int is16bit; // 1 - if 16 bit file, 0 - otherwise

public:
	explicit RawPCMReader(int bits) noexcept
		: is16bit(bits == 16)
	{
	}

	bool Import(DataStream* stream) override;
	size_t read_samples(short* buffer, size_t count) override;
	size_t ReadSamplesIntoChannels(char* channel1, char* channel2, size_t numSamples) override;
};

// WAV files
class WavPCMReader : public RawPCMReader {
public:
	WavPCMReader()
		: RawPCMReader(16)
	{
	}
	bool Import(DataStream* stream) override;
};

}

#endif
