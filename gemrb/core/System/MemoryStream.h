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

/**
 * @file MemoryStream.h
 * Declares MemoryStream class, stream reading/writing data from/to a buffer in memory.
 * @author The GemRB Project
 */


#ifndef MEMORYSTREAM_H
#define MEMORYSTREAM_H

#include "System/DataStream.h"

#include "exports.h"
#include "globals.h"

/**
 * @class MemoryStream
 * Reads and writes data from/to a buffer in memory.
 */

class GEM_EXPORT MemoryStream : public DataStream {
private:
	void* ptr;
	//unsigned long length;
	bool autoFree;
public:
	MemoryStream(void* buffer, int length, bool autoFree = true);
	~MemoryStream(void);
	int Read(void* dest, unsigned int length);
	int Write(const void * /*src*/, unsigned int /*length*/)
	{
		return GEM_ERROR;
	}
	int Seek(int pos, int startpos);
	int ReadLine(void* buf, unsigned int maxlen);
};

#endif  // ! MEMORYSTREAM_H
