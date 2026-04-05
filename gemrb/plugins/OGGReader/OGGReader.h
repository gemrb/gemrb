// SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef OGGREADER_H
#define OGGREADER_H

#include "SoundMgr.h"

#include <cstdio>
#include <cstring>
#if defined __APPLE_CC__ || defined __MINGW64__
	#define OV_EXCLUDE_STATIC_CALLBACKS
#endif
#include <vorbis/vorbisfile.h>

namespace GemRB {

class OGGReader : public SoundMgr {
private:
	OggVorbis_File OggStream;
	size_t samplesLeft = 0; // count of unread samples

public:
	OGGReader()
	{
		memset(&OggStream, 0, sizeof(OggStream));
	}
	OGGReader(const OGGReader&) = delete;
	OGGReader& operator=(const OGGReader&) = delete;
	~OGGReader() override
	{
		Close();
	}
	void Close()
	{
		ov_clear(&OggStream);
	}
	bool Import(DataStream* stream) override;
	size_t read_samples(short* buffer, size_t count) override;
	size_t ReadSamplesIntoChannels(char* channel1, char* channel2, size_t numSamples) override;
};

}

#endif
