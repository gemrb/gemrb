// SPDX-FileCopyrightText: 2020 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include <cassert>

#ifdef AMIGA
	#include <cstdlib>
#endif

#if !defined(WIN32) && !defined(AMIGA)
	#include <sys/mman.h>
#endif

#include "MappedFileMemoryStream.h"

#include "System/VFS.h"

#include <sys/stat.h>

namespace GemRB {

MappedFileMemoryStream::MappedFileMemoryStream(const std::string& fileName)
	: MemoryStream(fileName, nullptr, 0),
	  fileHandle(nullptr),
	  fileOpened(false),
	  fileMapped(false)
{
#ifdef WIN32
	TCHAR t_name[MAX_PATH] = { 0 };
	mbstowcs(t_name, fileName.c_str(), MAX_PATH - 1);

	this->fileHandle =
		CreateFile(
			t_name,
			GENERIC_READ,
			FILE_SHARE_READ,
			nullptr,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL,
			nullptr);
	this->fileOpened = fileHandle != INVALID_HANDLE_VALUE;

	if (fileOpened) {
		LARGE_INTEGER fileSize;
		GetFileSizeEx(fileHandle, &fileSize);
		assert(fileSize.QuadPart <= ULONG_MAX);
		size = static_cast<strpos_t>(fileSize.QuadPart);
	}
#else
	this->fileHandle = fopen(fileName.c_str(), "rb");
	this->fileOpened = fileHandle != nullptr;

	if (fileOpened) {
		struct stat statData {};
		int ret = fstat(fileno(static_cast<FILE*>(fileHandle)), &statData);
		assert(ret != -1);
		this->size = statData.st_size;
	}
#endif

	if (fileOpened) {
#ifdef AMIGA
		/*
		 * AmigaOS has no mmap(), so load the complete file into
		 * an allocated memory block.
		 */
		const size_t allocationSize = size != 0 ? static_cast<size_t>(size) : 1;
		this->data = static_cast<char*>(std::malloc(allocationSize));
		if (data != nullptr) {
			FILE* file = static_cast<FILE*>(fileHandle);

			if (size == 0) {
				this->fileMapped = true;
			} else {
				const size_t bytesRead = fread(data, 1, static_cast<size_t>(size), file);
				this->fileMapped = bytesRead == static_cast<size_t>(size);
			}

			if (!fileMapped) {
				std::free(data);
				data = nullptr;
			}
		}
#else
		this->data = static_cast<char*>(readonly_mmap(fileHandle));
		this->fileMapped = data != nullptr;
#endif
	}
}

bool MappedFileMemoryStream::isOk() const
{
	return fileOpened && fileMapped;
}

DataStream* MappedFileMemoryStream::Clone() const noexcept
{
	return new MappedFileMemoryStream(originalfile);
}

strret_t MappedFileMemoryStream::Read(void* dest, strpos_t length)
{
	if (!fileMapped) {
		return Error;
	}

	return MemoryStream::Read(dest, length);
}

stroff_t MappedFileMemoryStream::Seek(stroff_t pos, strpos_t startPos)
{
	if (!fileMapped) {
		return InvalidPos;
	}

	return MemoryStream::Seek(pos, startPos);
}

strret_t MappedFileMemoryStream::Write(const void*, strpos_t)
{
	return Error;
}

MappedFileMemoryStream::~MappedFileMemoryStream()
{
	if (fileMapped) {
#ifdef AMIGA
		std::free(data);
#else
		munmap(data, size);
#endif
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
