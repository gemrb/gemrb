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

#include "ie_types.h"

#include "Resource.h"
#include "System/DataStream.h"

namespace GemRB {

/**
 * Base Class for sound plugins
 */
class GEM_EXPORT SoundMgr : public Resource {
public:
	static const TypeID ID;
public:
	SoundMgr(void);
	virtual ~SoundMgr(void);
	/**
	 * Read up to cnt samples into memory
	 *
	 * @param[out] memory Array to hold samples read.
	 * @param[in] cnt number of samples to read.
	 * @returns Number of samples read.
	 */
	virtual int read_samples( short* memory, int cnt ) = 0 ;
	int get_channels() const
	{
		return channels;
	}
	int get_samplerate() const
	{
		return samplerate;
	}
	int get_length() const
	{
		return samples;
	} // returns the total samples count
protected:
	int samples; // total count of sound samples
	int channels;
	int samplerate;
};

}

#endif
