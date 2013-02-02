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

#include "System/MemoryStream.h"

#include "win32def.h"
#include "errors.h"

#include "Interface.h"

namespace GemRB {

MemoryStream::MemoryStream(char *name, void* data, unsigned long size)
	: data((char*)data)
{
	this->size = size;
	ExtractFileFromPath(filename, name);
	strlcpy(originalfile, name, _MAX_PATH);
}

MemoryStream::~MemoryStream()
{
	free(data);
}

DataStream* MemoryStream::Clone()
{
	void *copy = malloc(size);
	memcpy(copy, data, size);
	return new MemoryStream(originalfile, copy, size);
}

int MemoryStream::Read(void* dest, unsigned int length)
{
	//we don't allow partial reads anyway, so it isn't a problem that
	//i don't adjust length here (partial reads are evil)
	if (Pos+length>size ) {
		return GEM_ERROR;
	}

	memcpy(dest, data + Pos + (Encrypted ? 2 : 0), length);
	if (Encrypted) {
		ReadDecrypted( dest, length );
	}
	Pos += length;
	return length;
}

int MemoryStream::Write(const void* src, unsigned int length)
{
	if (Pos+length>size ) {
		//error("MemoryStream", "We don't support appending to memory streams yet.");
		return GEM_ERROR;
	}
	memcpy(data+Pos, src, length);
	Pos += length;
	return length;
}

int MemoryStream::Seek(int newpos, int type)
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

		default:
			return GEM_ERROR;
	}
	//we went past the buffer
	if (Pos>size) {
		print("[Streams]: Invalid seek position: %ld(limit: %ld)", Pos, size);
		return GEM_ERROR;
	}
	return GEM_OK;
}

}
