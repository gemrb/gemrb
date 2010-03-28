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

#ifndef CACHEDFILESTREAM_H
#define CACHEDFILESTREAM_H

#include "FileStream.h"
#include "../../includes/exports.h"

class GEM_EXPORT CachedFileStream : public DataStream// : public FileStream
{
private:
	bool autoFree;
	unsigned long startpos;
	_FILE* str;
public:
	CachedFileStream(const char* stream, bool autoFree = true);
	CachedFileStream(CachedFileStream* cfs, int startpos, int size,
		bool autoFree = true);
	~CachedFileStream(void);
	int Read(void* dest, unsigned int length);
	int Write(const void* src, unsigned int length);
	int Seek(int pos, int startpos);
	/** No descriptions */
	int ReadLine(void* buf, unsigned int maxlen);
};
#endif
