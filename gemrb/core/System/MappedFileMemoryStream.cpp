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
#include <cassert>

#ifndef WIN32
#include <sys/mman.h>
#endif

#include "MappedFileMemoryStream.h"
#include "System/VFS.h"

namespace GemRB {

MappedFileMemoryStream::MappedFileMemoryStream(const std::string& fileName)
	: MemoryStream(fileName.c_str(), nullptr, 0),
		fileHandle(nullptr),
		fileOpened(false),
		fileMapped(false)
{
#ifdef WIN32
	TCHAR t_name[MAX_PATH] = {0};
	mbstowcs(t_name, fileName.c_str(), MAX_PATH - 1);

	this->fileHandle =
		CreateFile(
			t_name,
			GENERIC_READ,
			FILE_SHARE_READ,
			nullptr,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL,
			nullptr
		);
	this->fileOpened = fileHandle != INVALID_HANDLE_VALUE;

	if (fileOpened) {
		LARGE_INTEGER fileSize;
		GetFileSizeEx(fileHandle, &fileSize);
		assert(fileSize.QuadPart <= ULONG_MAX);
		size = static_cast<unsigned long>(fileSize.QuadPart);
	}
#else
	this->fileHandle = fopen(fileName.c_str(), "rb");
	this->fileOpened = fileHandle != nullptr;

	if (fileOpened) {
		struct stat statData{};
		fstat(fileno(static_cast<FILE*>(fileHandle)), &statData);
		this->size = statData.st_size;
	}
#endif

	if (fileOpened) {
		this->data = static_cast<char*>(readonly_mmap(fileHandle));
		this->fileMapped = data != nullptr;
	}
}

bool MappedFileMemoryStream::isOk() const {
	return fileOpened && fileMapped;
}

DataStream* MappedFileMemoryStream::Clone() {
	return new MappedFileMemoryStream(originalfile);
}

int MappedFileMemoryStream::Read(void* dest, unsigned int length) {
	if (!fileMapped) {
		return GEM_ERROR;
	}

	return MemoryStream::Read(dest, length);
}

int MappedFileMemoryStream::Seek(int pos, int startPos) {
	if (!fileMapped) {
		return GEM_ERROR;
	}

	return MemoryStream::Seek(pos, startPos);
}

int MappedFileMemoryStream::Write(const void*, unsigned int) {
	return GEM_ERROR;
}

MappedFileMemoryStream::~MappedFileMemoryStream() {
	if (fileMapped) {
		munmap(data, size);
	}

	this->data = nullptr;

	if (fileOpened) {
#ifdef WIN32
		CloseHandle(fileHandle);
#else
		fclose(static_cast<FILE*>(fileHandle));
#endif
	}
}

}
