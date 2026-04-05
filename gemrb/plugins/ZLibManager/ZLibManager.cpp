// SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "ZLibManager.h"

#include "errors.h"

#include <zlib.h>

using namespace GemRB;

#define INPUTSIZE  8192
#define OUTPUTSIZE 8192

// ZLib Decompression Routine
int ZLibManager::Decompress(DataStream* dest, DataStream* source, unsigned int size_guess) const
{
	unsigned char bufferin[INPUTSIZE];
	unsigned char bufferout[OUTPUTSIZE];
	z_stream stream {};

	stream.zalloc = Z_NULL;
	stream.zfree = Z_NULL;
	stream.opaque = Z_NULL;

	int result = inflateInit(&stream);
	if (result != Z_OK) {
		return GEM_ERROR;
	}

	stream.avail_in = 0;
	while (true) {
		stream.next_out = bufferout;
		stream.avail_out = OUTPUTSIZE;
		if (stream.avail_in == 0) {
			stream.next_in = bufferin;
			if (size_guess) {
				stream.avail_in = size_guess;
			}
			if (!stream.avail_in || stream.avail_in > source->Remains()) {
				//Read doesn't allow partial reads, but provides Remains
				unsigned long remains = std::min<unsigned long>(source->Remains(), std::numeric_limits<uInt>::max());
				stream.avail_in = static_cast<uInt>(remains);
			}
			if (stream.avail_in > INPUTSIZE) {
				stream.avail_in = INPUTSIZE;
			}
			if (size_guess) {
				size_guess = std::max<unsigned int>(0, size_guess - stream.avail_in);
			}
			if (source->Read(bufferin, stream.avail_in) != (int) stream.avail_in) {
				inflateEnd(&stream);
				return GEM_ERROR;
			}
		}
		result = inflate(&stream, Z_NO_FLUSH);
		if (result != Z_OK && result != Z_STREAM_END) {
			inflateEnd(&stream);
			return GEM_ERROR;
		}
		if (dest->Write(bufferout, OUTPUTSIZE - stream.avail_out) == GEM_ERROR) {
			inflateEnd(&stream);
			return GEM_ERROR;
		}
		if (result == Z_STREAM_END) {
			if (stream.avail_in > 0) {
				source->Seek((stroff_t) (-(int) (stream.avail_in)), GEM_CURRENT_POS);
			}
			result = inflateEnd(&stream);
			return result == Z_OK ? GEM_OK : GEM_ERROR;
		}
	}
}

int ZLibManager::Compress(DataStream* dest, DataStream* source) const
{
	unsigned char bufferin[INPUTSIZE];
	unsigned char bufferout[OUTPUTSIZE];
	z_stream stream {};

	stream.zalloc = Z_NULL;
	stream.zfree = Z_NULL;
	stream.opaque = Z_NULL;

	int result = deflateInit(&stream, Z_BEST_COMPRESSION);
	if (result != Z_OK) {
		return GEM_ERROR;
	}

	stream.avail_in = 0;
	while (true) {
		stream.next_out = bufferout;
		stream.avail_out = OUTPUTSIZE;
		if (stream.avail_in == 0) {
			stream.next_in = bufferin;
			//Read doesn't allow partial reads, but provides Remains
			unsigned long remains = std::min<unsigned long>(source->Remains(), std::numeric_limits<uInt>::max());
			stream.avail_in = std::min<uInt>(static_cast<uInt>(remains), INPUTSIZE);
			if (source->Read(bufferin, stream.avail_in) != (int) stream.avail_in) {
				deflateEnd(&stream);
				return GEM_ERROR;
			}
		}
		if (stream.avail_in == 0) {
			result = deflate(&stream, Z_FINISH);
		} else {
			result = deflate(&stream, Z_NO_FLUSH);
		}
		if (result != Z_OK && result != Z_STREAM_END) {
			deflateEnd(&stream);
			return GEM_ERROR;
		}
		if (dest->Write(bufferout, OUTPUTSIZE - stream.avail_out) == GEM_ERROR) {
			deflateEnd(&stream);
			return GEM_ERROR;
		}
		if (result == Z_STREAM_END) {
			if (stream.avail_in > 0) {
				source->Seek((stroff_t) (-(int) (stream.avail_in)), GEM_CURRENT_POS);
			}
			result = deflateEnd(&stream);
			return result == Z_OK ? GEM_OK : GEM_ERROR;
		}
	}
}

#include "plugindef.h"

GEMRB_PLUGIN(0x2477C688, "ZLib Compression Manager")
PLUGIN_CLASS(PLUGIN_COMPRESSION_ZLIB, ZLibManager)
END_PLUGIN()
