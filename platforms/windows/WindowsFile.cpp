/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2023 The GemRB Project
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
 */

#include "WindowsFile.h"

#include "../../gemrb/core/Strings/StringConversion.h"

namespace GemRB {

WindowsFile::WindowsFile(WindowsFile&& f) noexcept
{
	this->file = f.file;
	f.file = INVALID_HANDLE_VALUE;
}

WindowsFile::~WindowsFile()
{
	if (file != INVALID_HANDLE_VALUE) {
		CloseHandle(file);
	}
}

WindowsFile& WindowsFile::operator=(WindowsFile&& f) noexcept
{
	if (&f != this) {
		std::swap(file, f.file);
	}
	return *this;
}

strpos_t WindowsFile::Length()
{
	LARGE_INTEGER fileSize;
	GetFileSizeEx(file, &fileSize);

	return fileSize.QuadPart;
}

bool WindowsFile::OpenRO(const path_t& name)
{
	this->file = _OpenFile(name, GENERIC_READ, OPEN_EXISTING);
	return file != INVALID_HANDLE_VALUE;
}

bool WindowsFile::OpenRW(const path_t& name)
{
	this->file = _OpenFile(name, GENERIC_READ | GENERIC_WRITE, OPEN_EXISTING);
	return file != INVALID_HANDLE_VALUE;
}

bool WindowsFile::OpenNew(const path_t& name)
{
	this->file = _OpenFile(name, GENERIC_WRITE, CREATE_ALWAYS);
	return file != INVALID_HANDLE_VALUE;
}

strret_t WindowsFile::Read(void* ptr, size_t length)
{
	DWORD bytesRead = 0;
	ReadFile(file, ptr, length, &bytesRead, nullptr);
	return bytesRead;
}

strret_t WindowsFile::Write(const void* ptr, strpos_t length)
{
	DWORD bytesWritten = 0;
	WriteFile(file, ptr, length, &bytesWritten, nullptr);
	return bytesWritten;
}

bool WindowsFile::SeekStart(stroff_t offset)
{
	return _SetFilePointer(offset, FILE_BEGIN);
}

bool WindowsFile::SeekCurrent(stroff_t offset)
{
	return _SetFilePointer(offset, FILE_CURRENT);
}

bool WindowsFile::SeekEnd(stroff_t offset)
{
	return _SetFilePointer(offset, FILE_END);
}

HANDLE WindowsFile::_OpenFile(const path_t& name, DWORD access, DWORD disposition)
{
	auto fileName = StringFromUtf8(name.c_str());

	return CreateFile(
		reinterpret_cast<const wchar_t*>(fileName.c_str()),
		access,
		FILE_SHARE_READ | FILE_SHARE_WRITE, // some files are opened r and r+w in parallel
		nullptr,
		disposition,
		FILE_ATTRIBUTE_NORMAL,
		nullptr);
}

bool WindowsFile::_SetFilePointer(stroff_t offset, DWORD method)
{
	LARGE_INTEGER liOffset;
	liOffset.QuadPart = offset;

	return SetFilePointerEx(file, liOffset, nullptr, method) != 0;
}

}
