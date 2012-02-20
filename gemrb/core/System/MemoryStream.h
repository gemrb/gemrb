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

#ifndef MEMORYSTREAM_H
#define MEMORYSTREAM_H

#include "System/DataStream.h"

#include "exports.h"

namespace GemRB {

class GEM_EXPORT MemoryStream : public DataStream
{
private:
	char *data;
public:
	MemoryStream(char *name, void* data, unsigned long size);
	~MemoryStream();
	DataStream* Clone();

	int Read(void* dest, unsigned int length);
	int Write(const void* src, unsigned int length);
	int Seek(int pos, int startpos);
};

}

#endif
