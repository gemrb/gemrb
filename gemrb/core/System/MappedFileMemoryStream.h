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

#include "System/FileStream.h"
#include "System/MemoryStream.h"

namespace GemRB {

class GEM_EXPORT MappedFileMemoryStream : public MemoryStream {
	public:
		MappedFileMemoryStream(const std::string& fileName);
		~MappedFileMemoryStream();

		bool isOk() const;

		int Read(void* dest, unsigned int len) override;
		int Seek(int pos, int startPos) override;
		int Write(const void* src, unsigned int len) override;
		DataStream* Clone() override;

	private:
		void *fileHandle;
		bool fileOpened;
		bool fileMapped;
};

}

#endif
