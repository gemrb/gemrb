// SPDX-FileCopyrightText: 2023 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef POSIX_FILE_H
#define POSIX_FILE_H

#include "File.h"

#include <cstdio>

namespace GemRB {

class GEM_EXPORT PosixFile : public File {
private:
	FILE* file = nullptr;

public:
	PosixFile() noexcept = default;
	PosixFile(const File&) = delete;
	PosixFile(PosixFile&& f) noexcept;
	~PosixFile() override;

	PosixFile& operator=(const PosixFile&) = delete;
	PosixFile& operator=(PosixFile&& f) noexcept;

	strpos_t Length() override;
	bool OpenRO(const path_t& name) override;
	bool OpenRW(const path_t& name) override;
	bool OpenNew(const path_t& name) override;
	strret_t Read(void* ptr, size_t length) override;
	strret_t Write(const void* ptr, strpos_t length) override;
	bool SeekStart(stroff_t offset) override;
	bool SeekCurrent(stroff_t offset) override;
	bool SeekEnd(stroff_t offset) override;
};

}
#endif
