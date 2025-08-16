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

#include "SClassID.h"
#include "exports-core.h"

#include "DataStream.h"

#ifdef WIN32
	#include "../../platforms/windows/WindowsFile.h"
using FileT = GemRB::WindowsFile;
#else
	#include "PosixFile.h"
using FileT = GemRB::PosixFile;
#endif

namespace GemRB {

/**
 * @class FileStream
 * Reads and writes data from/to files on a filesystem
 */

class GEM_EXPORT FileStream : public DataStream {
private:
	FileT str;
	bool opened = true;
	bool created = true;

public:
	explicit FileStream(FileT&&);
	FileStream(void);

	DataStream* Clone() const noexcept override;

	bool Open(const path_t& filename);
	bool Modify(const path_t& filename);
	bool Create(const path_t& folder, const path_t& filename, SClass_ID ClassID);
	bool Create(const path_t& filename, SClass_ID ClassID);
	bool Create(const path_t& filename);
	strret_t Read(void* dest, strpos_t length) override;
	strret_t Write(const void* src, strpos_t length) override;
	strret_t Seek(stroff_t pos, strpos_t startpos) override;

	void Close();

public:
	/** Opens the specified file.
	 *
	 *  Returns NULL, if the file can't be opened.
	 */
	static FileStream* OpenFile(const path_t& filename);

private:
	void FindLength();
};

}

#endif // ! FILESTREAM_H
