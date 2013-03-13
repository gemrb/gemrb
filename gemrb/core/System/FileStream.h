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
 * @file FileStream.h
 * Declares FileStream class, stream reading/writing data from/to a file in a filesystem.
 * @author The GemRB Project
 */


#ifndef FILESTREAM_H
#define FILESTREAM_H

#include "System/DataStream.h"

#include "exports.h"
#include "globals.h"

namespace GemRB {

/**
 * @class FileStream
 * Reads and writes data from/to files on a filesystem
 */

class GEM_EXPORT FileStream : public DataStream {
private:
#ifdef _DEBUG
	static int FileStreamPtrCount;
#endif
	struct File;
	File* str;
	bool opened, created;
public:
	FileStream(void);
	~FileStream(void);
	DataStream* Clone();

	bool Open(const char* filename);
	bool Modify(const char* filename);
	bool Create(const char* folder, const char* filename, SClass_ID ClassID);
	bool Create(const char* filename, SClass_ID ClassID);
	bool Create(const char* filename);
	int Read(void* dest, unsigned int length);
	int Write(const void* src, unsigned int length);
	int Seek(int pos, int startpos);

	void Close();
public:
	/** Opens the specifed file.
	 *
	 *  Returns NULL, if the file can't be opened.
	 */
	static FileStream* OpenFile(const char* filename);
private:
	void FindLength();
};

}

#endif  // ! FILESTREAM_H
