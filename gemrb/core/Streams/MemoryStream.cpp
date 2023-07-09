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

#include "MemoryStream.h"

#include "errors.h"

#include "Interface.h"
#include "Logging/Logging.h"

namespace GemRB {

MemoryStream::MemoryStream(const char *name, void* data, strpos_t size)
	: data((char*)data)
{
	this->size = size;
	originalfile = name;
	filename = ExtractFileFromPath(name);
}

MemoryStream::~MemoryStream()
{
	free(data);
}

DataStream* MemoryStream::Clone() const noexcept
{
	void *copy = malloc(size);
	memcpy(copy, data, size);
	return new MemoryStream(originalfile.c_str(), copy, size);
}

strret_t MemoryStream::Read(void* dest, strpos_t length)
{
	//we don't allow partial reads anyway, so it isn't a problem that
	//i don't adjust length here (partial reads are evil)
	if (Pos+length>size ) {
		return Error;
	}

	memcpy(dest, data + Pos + (Encrypted ? 2 : 0), length);
	if (Encrypted) {
		ReadDecrypted( dest, length );
	}
	Pos += length;
	return length;
}

strret_t MemoryStream::Write(const void* src, strpos_t length)
{
	if (Pos+length>size ) {
		//error("MemoryStream", "We don't support appending to memory streams yet.");
		return Error;
	}
	memcpy(data+Pos, src, length);
	Pos += length;
	return length;
}

stroff_t MemoryStream::Seek(stroff_t newpos, strpos_t type)
{
	switch (type) {
		case GEM_CURRENT_POS:
			Pos += newpos;
			break;

		case GEM_STREAM_START:
			Pos = newpos;
			break;

		case GEM_STREAM_END:
			Pos = size - newpos;
			break;

		default:
			return InvalidPos;
	}
	//we went past the buffer
	if (Pos>size) {
		Log(ERROR, "Streams", "Invalid seek position: {} (limit: {})", Pos, size);
		return InvalidPos;
	}
	return 0;
}

}
