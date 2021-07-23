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
 *
 */

#ifndef WAVREADER_H
#define WAVREADER_H

#include "SoundMgr.h"

#include "System/DataStream.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace GemRB {

// RAW file reader
class RawPCMReader : public SoundMgr {
protected:
	// one sample consists of
	// channels * (is16bit ? 2 : 1) bytes
	int samples_left; // count of unread samples
	int is16bit; // 1 - if 16 bit file, 0 - otherwise
public:
	explicit RawPCMReader(int bits)
		: samples_left(0), is16bit( bits == 16 )
	{
	}

	bool Open(DataStream* stream) override;
	int read_samples(short* buffer, int count) override;
};

// WAV files
class WavPCMReader : public RawPCMReader {
public:
	WavPCMReader()
		: RawPCMReader( 16 )
	{
	}
	bool Open(DataStream* stream) override;
};

}

#endif
