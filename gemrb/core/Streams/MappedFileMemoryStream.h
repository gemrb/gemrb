/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2020 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#ifndef MAPPED_FILE_MEMORY_STREAM_H
#define MAPPED_FILE_MEMORY_STREAM_H

#include "FileStream.h"
#include "MemoryStream.h"

namespace GemRB {

class GEM_EXPORT MappedFileMemoryStream : public MemoryStream {
public:
	explicit MappedFileMemoryStream(const std::string& fileName);
	~MappedFileMemoryStream() override;

	bool isOk() const;

	strret_t Read(void* dest, strpos_t len) override;
	strret_t Seek(stroff_t pos, strpos_t startPos) override;
	strret_t Write(const void* src, strpos_t len) override;
	DataStream* Clone() const noexcept override;

private:
	void* fileHandle;
	bool fileOpened;
	bool fileMapped;
};

}

#endif
