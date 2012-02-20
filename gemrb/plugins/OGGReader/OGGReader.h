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
#include "System/DataStream.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#ifdef __APPLE_CC__
#define OV_EXCLUDE_STATIC_CALLBACKS
#endif
#include <vorbis/vorbisfile.h>

namespace GemRB {

class OGGReader : public SoundMgr {
private:
	OggVorbis_File OggStream;
	int samples_left; // count of unread samples
public:
	OGGReader()
		: samples_left( 0 )
	{
		memset(&OggStream, 0, sizeof(OggStream) );
	}
	virtual ~OGGReader()
	{
		Close();
	}
	void Close()
	{
		ov_clear(&OggStream);
	}
	bool Open(DataStream* stream);
	int read_samples(short* buffer, int count);
};

}

#endif
