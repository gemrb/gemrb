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

#include "SlicedStream.h"

#include "MemoryStream.h"

#include "errors.h"

#include "Interface.h"
#include "Logging/Logging.h"

namespace GemRB {

SlicedStream::SlicedStream(const DataStream* cfs, strpos_t startPos, strpos_t streamSize)
{
	str = cfs->Clone();
	assert(str);
	size = streamSize;
	startpos = startPos;
	originalfile = cfs->originalfile;
	filename = cfs->filename;
	str->Seek(startpos, GEM_STREAM_START);
}

SlicedStream::~SlicedStream()
{
	delete str;
}

DataStream* SlicedStream::Clone() const noexcept
{
	return new SlicedStream(str, startpos, size);
}

strret_t SlicedStream::Read(void* dest, strpos_t length)
{
	//we don't allow partial reads anyway, so it isn't a problem that
	//i don't adjust length here (partial reads are evil)
	if (Pos+length>size ) {
		return Error;
	}

	//str->Seek(startpos + Pos + (Encrypted ? 2 : 0), GEM_STREAM_START);
	unsigned int c = (unsigned int) str->Read(dest, length);
	if (c != length) {
		return Error;
	}
	if (Encrypted) {
		ReadDecrypted( dest, c );
	}
	Pos += c;
	return c;
}

strret_t SlicedStream::Write(const void* /*src*/, strpos_t /*length*/)
{
	error("SlicedStream", "Attempted to use unimplemented SlicedStream::Write method!");
}

stroff_t SlicedStream::Seek(stroff_t newpos, strpos_t type)
{
	switch (type) {
		case GEM_CURRENT_POS:
			Pos += newpos;
			break;

		case GEM_STREAM_START:
			Pos = newpos;
			break;

		default:
			return Error;
	}
	str->Seek(startpos + Pos /*+ (Encrypted ? 2 : 0)*/, GEM_STREAM_START);
	//we went past the buffer
	if (Pos>size) {
		Log(ERROR, "Streams", "Invalid seek position: {} (limit: {})", Pos, size);
		return Error;
	}
	return 0;
}

DataStream* SliceStream(DataStream* str, strpos_t startpos, strpos_t size, bool preservepos)
{
	if (size <= 16384) {
		// small (or empty) substream, just read it into a buffer instead of expensive file I/O
		strpos_t oldpos;
		if (preservepos)
			oldpos = str->GetPos();
		str->Seek(startpos, GEM_STREAM_START);
		char *data = (char*)malloc(size);
		str->Read(data, size);
		if (preservepos)
			str->Seek(oldpos, GEM_STREAM_START);

		DataStream *mem = new MemoryStream(str->originalfile.c_str(), data, size);
		return mem;
	} else
		return new SlicedStream(str, startpos, size);
}

}
