// SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef SOUNDMGR_H
#define SOUNDMGR_H

#include "Resource.h"

namespace GemRB {

/**
 * Base Class for sound plugins
 */
class GEM_EXPORT SoundMgr : public Resource {
public:
	static const TypeID ID;

	SoundMgr() noexcept = default;
	/**
	 * Read up to cnt samples into memory
	 *
	 * @param[out] memory Array to hold samples read.
	 * @param[in] cnt number of samples to read.
	 * @returns Number of samples read.
	 */
	virtual size_t read_samples(short* memory, size_t cnt) = 0;
	virtual size_t ReadSamplesIntoChannels(char* channel1, char* channel2, size_t numSamples) = 0;
	time_t GetLengthMs() const
	{
		if (channels == 0 || sampleRate == 0) {
			return 0;
		}
		return ((samples / channels) * 1000) / sampleRate;
	}
	uint8_t GetChannels() const
	{
		return channels;
	}
	uint32_t GetSampleRate() const
	{
		return sampleRate;
	}
	size_t GetNumSamples() const
	{
		return samples;
	}

protected:
	size_t samples = 0;
	uint8_t channels = 0;
	uint32_t sampleRate = 0;
};

}

#endif
