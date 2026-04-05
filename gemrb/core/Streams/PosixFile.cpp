// SPDX-FileCopyrightText: 2023 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "PosixFile.h"

#include <algorithm>

namespace GemRB {

PosixFile::PosixFile(PosixFile&& other) noexcept
{
	this->file = other.file;
	other.file = nullptr;
}

PosixFile::~PosixFile()
{
	if (file) {
		fclose(file);
	}
}

PosixFile& PosixFile::operator=(PosixFile&& other) noexcept
{
	if (&other != this) {
		std::swap(file, other.file);
	}

	return *this;
}

strpos_t PosixFile::Length()
{
	fseek(file, 0, SEEK_END);
	strpos_t size = ftell(file);
	fseek(file, 0, SEEK_SET);

	return size;
}

bool PosixFile::OpenRO(const path_t& name)
{
	return (file = fopen(name.c_str(), "rb"));
}

bool PosixFile::OpenRW(const path_t& name)
{
	return (file = fopen(name.c_str(), "r+b"));
}

bool PosixFile::OpenNew(const path_t& name)
{
	return (file = fopen(name.c_str(), "wb"));
}

strret_t PosixFile::Read(void* ptr, size_t length)
{
	return fread(ptr, 1, length, file);
}

strret_t PosixFile::Write(const void* ptr, strpos_t length)
{
	return fwrite(ptr, 1, length, file);
}

bool PosixFile::SeekStart(stroff_t offset)
{
	return !fseek(file, offset, SEEK_SET);
}

bool PosixFile::SeekCurrent(stroff_t offset)
{
	return !fseek(file, offset, SEEK_CUR);
}

bool PosixFile::SeekEnd(stroff_t offset)
{
	return !fseek(file, offset, SEEK_END);
}

}
