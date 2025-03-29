/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
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
 *
 */

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
