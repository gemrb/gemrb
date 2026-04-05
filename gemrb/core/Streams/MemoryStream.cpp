// SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "MemoryStream.h"

#include "Logging/Logging.h"

namespace GemRB {

MemoryStream::MemoryStream(const path_t& name, void* data, strpos_t size)
	: data((char*) data)
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
	void* copy = malloc(size);
	memcpy(copy, data, size);
	return new MemoryStream(originalfile, copy, size);
}

strret_t MemoryStream::Read(void* dest, strpos_t length)
{
	//we don't allow partial reads anyway, so it isn't a problem that
	//i don't adjust length here (partial reads are evil)
	if (Pos + length > size) {
		return Error;
	}

	memcpy(dest, data + Pos + (Encrypted ? 2 : 0), length);
	if (Encrypted) {
		ReadDecrypted(dest, length);
	}
	Pos += length;
	return length;
}

strret_t MemoryStream::Write(const void* src, strpos_t length)
{
	if (Pos + length > size) {
		//error("MemoryStream", "We don't support appending to memory streams yet.");
		return Error;
	}
	memcpy(data + Pos, src, length);
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
	if (Pos > size) {
		Log(ERROR, "Streams", "Invalid seek position: {} (limit: {})", Pos, size);
		return InvalidPos;
	}
	return 0;
}

}
