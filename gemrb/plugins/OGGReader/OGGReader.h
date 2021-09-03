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

#ifndef OGGREADER_H
#define OGGREADER_H

#include "SoundMgr.h"
#include "System/DataStream.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#if defined __APPLE_CC__ || defined __MINGW64__
#define OV_EXCLUDE_STATIC_CALLBACKS
#endif
#include <vorbis/vorbisfile.h>

namespace GemRB {

class OGGReader : public SoundMgr {
private:
	OggVorbis_File OggStream;
	int samples_left = 0; // count of unread samples
public:
	OGGReader()
	{
		memset(&OggStream, 0, sizeof(OggStream) );
	}
	~OGGReader() override
	{
		Close();
	}
	void Close()
	{
		ov_clear(&OggStream);
	}
	bool Import(DataStream* stream) override;
	int read_samples(short* buffer, int count) override;
};

}

#endif
