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

#include "DataStream.h"

#include "exports.h"
#include "globals.h"
#include "SClassID.h"

namespace GemRB {

/**
 * @class FileStream
 * Reads and writes data from/to files on a filesystem
 */

#ifndef WIN32
struct File {
private:
	FILE* file = nullptr;
public:
	explicit File(FILE* f) : file(f) {}
	File() noexcept = default;
	File(const File&) = delete;
	File(File&& f) noexcept {
		file = f.file;
		f.file = nullptr;
	}
	~File() {
		if (file) fclose(file);
	}
	
	File& operator=(const File&) = delete;
	File& operator=(File&& f) noexcept {
		if (&f != this) {
			std::swap(file, f.file);
		}
		return *this;
	}

	strpos_t Length() {
		fseek(file, 0, SEEK_END);
		strpos_t size = ftell(file);
		fseek(file, 0, SEEK_SET);
		return size;
	}
	bool OpenRO(const path_t& name) {
		return (file = fopen(name.c_str(), "rb"));
	}
	bool OpenRW(const path_t& name) {
		return (file = fopen(name.c_str(), "r+b"));
	}
	bool OpenNew(const path_t& name) {
		return (file = fopen(name.c_str(), "wb"));
	}
	strret_t Read(void* ptr, size_t length) {
		return fread(ptr, 1, length, file);
	}
	strret_t Write(const void* ptr, strpos_t length) {
		return fwrite(ptr, 1, length, file);
	}
	bool SeekStart(stroff_t offset)
	{
		return !fseek(file, offset, SEEK_SET);
	}
	bool SeekCurrent(stroff_t offset)
	{
		return !fseek(file, offset, SEEK_CUR);
	}
	bool SeekEnd(stroff_t offset)
	{
		return !fseek(file, offset, SEEK_END);
	}
};
#else
struct File {
private:
	HANDLE file = INVALID_HANDLE_VALUE;
public:
	explicit File(FILE* f) : file(f) {}
	File() noexcept = default;
	File(const File&) = delete;
	File(File&& f) noexcept {
		file = f.file;
		f.file = INVALID_HANDLE_VALUE;
	}
	~File() {
		if (file != INVALID_HANDLE_VALUE) {
			CloseHandle(file);
		}
	}

	File& operator=(const File&) = delete;
	File& operator=(File&& f) noexcept {
		if (&f != this) {
			std::swap(file, f.file);
		}
		return *this;
	}

	strpos_t Length() {
		LARGE_INTEGER fileSize;
		GetFileSizeEx(file, &fileSize);

		return fileSize.QuadPart;
	}
	bool OpenRO(const path_t& name) {
		file = _OpenFile(name, GENERIC_READ, OPEN_EXISTING);
		return file != INVALID_HANDLE_VALUE;
	}
	bool OpenRW(const path_t& name) {
		file = _OpenFile(name, GENERIC_READ | GENERIC_WRITE, OPEN_EXISTING);
		return file != INVALID_HANDLE_VALUE;
	}
	bool OpenNew(const path_t& name) {
		file = _OpenFile(name, GENERIC_WRITE, CREATE_ALWAYS);
		return file != INVALID_HANDLE_VALUE;
	}
	strret_t Read(void* ptr, size_t length) {
		DWORD bytesRead = 0;
		ReadFile(file, ptr, length, &bytesRead, nullptr);
		return bytesRead;
	}
	strret_t Write(const void* ptr, strpos_t length) {
		DWORD bytesWritten = 0;
		WriteFile(file, ptr, length, &bytesWritten, nullptr);
		return bytesWritten;
	}
	bool SeekStart(stroff_t offset) {
		return _SetFilePointer(offset, FILE_BEGIN);
	}
	bool SeekCurrent(stroff_t offset) {
		return _SetFilePointer(offset, FILE_CURRENT);
	}
	bool SeekEnd(stroff_t offset) {
		return _SetFilePointer(offset, FILE_END);
	}

private:
	HANDLE _OpenFile(const path_t& name, DWORD access, DWORD disposition) {
		auto fileName = StringFromUtf8(name.c_str());

		return CreateFile(
			reinterpret_cast<const wchar_t*>(fileName.c_str()),
			access,
			FILE_SHARE_READ | FILE_SHARE_WRITE, // some files are opened r and r+w in parallel
			nullptr,
			disposition,
			FILE_ATTRIBUTE_NORMAL,
			nullptr
		);
	}

	bool _SetFilePointer(stroff_t offset, DWORD method) {
		LARGE_INTEGER liOffset;
		liOffset.QuadPart = offset;

		return SetFilePointerEx(file, liOffset, nullptr, method) != 0;
	}
};
#endif

class GEM_EXPORT FileStream : public DataStream {
private:
	File str;
	bool opened = true;
	bool created = true;
public:
	explicit FileStream(File&&);
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
	/** Opens the specifed file.
	 *
	 *  Returns NULL, if the file can't be opened.
	 */
	static FileStream* OpenFile(const path_t& filename);
private:
	void FindLength();
};

}

#endif  // ! FILESTREAM_H
