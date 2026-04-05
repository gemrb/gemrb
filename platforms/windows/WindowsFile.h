// SPDX-FileCopyrightText: 2023 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef WINDOWS_FILE_H
#define WINDOWS_FILE_H

#include "../../gemrb/core/Streams/File.h"

namespace GemRB {

class GEM_EXPORT WindowsFile : public File {
private:
	HANDLE file = INVALID_HANDLE_VALUE;

public:
	WindowsFile() noexcept = default;
	WindowsFile(const WindowsFile&) = delete;
	WindowsFile(WindowsFile&& f) noexcept;
	~WindowsFile() override;

	WindowsFile& operator=(const WindowsFile&) = delete;
	WindowsFile& operator=(WindowsFile&& f) noexcept;

	strpos_t Length() override;
	bool OpenRO(const path_t& name) override;
	bool OpenRW(const path_t& name) override;
	bool OpenNew(const path_t& name) override;
	strret_t Read(void* ptr, size_t length) override;
	strret_t Write(const void* ptr, strpos_t length) override;
	bool SeekStart(stroff_t offset) override;
	bool SeekCurrent(stroff_t offset) override;
	bool SeekEnd(stroff_t offset) override;

private:
	HANDLE _OpenFile(const path_t& name, DWORD access, DWORD disposition);
	bool _SetFilePointer(stroff_t offset, DWORD method);
};

}

#endif
