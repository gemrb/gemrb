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

#include <algorithm>

#include "PosixFile.h"

namespace GemRB {

PosixFile::PosixFile(PosixFile&& other) noexcept {
	this->file = other.file;
	other.file = nullptr;
}

PosixFile::~PosixFile() {
	if (file) {
		fclose(file);
	}
}

PosixFile& PosixFile::operator=(PosixFile&& other) noexcept {
	if (&other != this) {
		std::swap(file, other.file);
	}

	return *this;
}

strpos_t PosixFile::Length() {
	fseek(file, 0, SEEK_END);
	strpos_t size = ftell(file);
	fseek(file, 0, SEEK_SET);

	return size;
}

bool PosixFile::OpenRO(const path_t& name) {
	return (file = fopen(name.c_str(), "rb"));
}

bool PosixFile::OpenRW(const path_t& name) {
	return (file = fopen(name.c_str(), "r+b"));
}

bool PosixFile::OpenNew(const path_t& name) {
	return (file = fopen(name.c_str(), "wb"));
}

strret_t PosixFile::Read(void* ptr, size_t length) {
	return fread(ptr, 1, length, file);
}

strret_t PosixFile::Write(const void* ptr, strpos_t length) {
	return fwrite(ptr, 1, length, file);
}

bool PosixFile::SeekStart(stroff_t offset) {
	return !fseek(file, offset, SEEK_SET);
}

bool PosixFile::SeekCurrent(stroff_t offset) {
	return !fseek(file, offset, SEEK_CUR);
}

bool PosixFile::SeekEnd(stroff_t offset) {
	return !fseek(file, offset, SEEK_END);
}

}
