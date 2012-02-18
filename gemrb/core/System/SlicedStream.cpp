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

#include "System/SlicedStream.h"

#include "System/MemoryStream.h"

#include "win32def.h"
#include "errors.h"

#include "Interface.h"

SlicedStream::SlicedStream(DataStream* str, int startpos, int size)
{
	this->str = str->Clone();
	assert(this->str);
	this->size = size;
	this->startpos = startpos;
	strncpy(originalfile, str->originalfile, _MAX_PATH);
	strncpy(filename, str->filename, sizeof(filename));
	this->str->Seek(this->startpos, GEM_STREAM_START);
}

SlicedStream::~SlicedStream()
{
	delete str;
}

DataStream* SlicedStream::Clone()
{
	return new SlicedStream(str, startpos, size);
}

int SlicedStream::Read(void* dest, unsigned int length)
{
	//we don't allow partial reads anyway, so it isn't a problem that
	//i don't adjust length here (partial reads are evil)
	if (Pos+length>size ) {
		return GEM_ERROR;
	}

	//str->Seek(startpos + Pos + (Encrypted ? 2 : 0), GEM_STREAM_START);
	unsigned int c = (unsigned int) str->Read(dest, length);
	if (c != length) {
		return GEM_ERROR;
	}
	if (Encrypted) {
		ReadDecrypted( dest, c );
	}
	Pos += c;
	return c;
}

int SlicedStream::Write(const void* src, unsigned int length)
{
	//str->Seek(startpos + Pos, GEM_STREAM_START);
	unsigned int c = (unsigned int) Write(src, length);
	if (c != length) {
		return GEM_ERROR;
	}
	Pos += c;
	//this is needed only if you want to Seek in a written file
	if (Pos>size) {
		size = Pos;
	}
	return c;
}

int SlicedStream::Seek(int newpos, int type)
{
	switch (type) {
		case GEM_CURRENT_POS:
			Pos += newpos;
			break;

		case GEM_STREAM_START:
			Pos = newpos;
			break;

		default:
			return GEM_ERROR;
	}
	str->Seek(startpos + Pos /*+ (Encrypted ? 2 : 0)*/, GEM_STREAM_START);
	//we went past the buffer
	if (Pos>size) {
		print("[Streams]: Invalid seek position: %ld(limit: %ld)", Pos, size);
		return GEM_ERROR;
	}
	return GEM_OK;
}

DataStream* SliceStream(DataStream* str, unsigned long startpos, unsigned long size, bool preservepos)
{
	if (size <= 16384) {
		// small (or empty) substream, just read it into a buffer instead of expensive file I/O
		unsigned long oldpos;
		if (preservepos)
			oldpos = str->GetPos();
		str->Seek(startpos, GEM_STREAM_START);
		char *data = (char*)malloc(size);
		str->Read(data, size);
		if (preservepos)
			str->Seek(oldpos, GEM_STREAM_START);

		DataStream *mem = new MemoryStream(str->originalfile, data, size);
		return mem;
	} else
		return new SlicedStream(str, startpos, size);
}
