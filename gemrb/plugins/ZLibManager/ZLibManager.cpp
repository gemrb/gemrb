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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

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
				return GEM_ERROR;
			}
		}
		result = inflate(&stream, Z_NO_FLUSH);
		if (result != Z_OK && result != Z_STREAM_END) {
			return GEM_ERROR;
		}
		if (dest->Write(bufferout, OUTPUTSIZE - stream.avail_out) == GEM_ERROR) {
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
				return GEM_ERROR;
			}
		}
		if (stream.avail_in == 0) {
			result = deflate(&stream, Z_FINISH);
		} else {
			result = deflate(&stream, Z_NO_FLUSH);
		}
		if (result != Z_OK && result != Z_STREAM_END) {
			return GEM_ERROR;
		}
		if (dest->Write(bufferout, OUTPUTSIZE - stream.avail_out) == GEM_ERROR) {
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
