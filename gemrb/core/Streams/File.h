// SPDX-FileCopyrightText: 2023 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef FILE_H
#define FILE_H

#include "DataStream.h"

namespace GemRB {

// encapsulating file handles + API that fit best on the respective platform
class File {
public:
	virtual ~File() = default;

	virtual strpos_t Length() = 0;
	virtual bool OpenRO(const path_t& name) = 0;
	virtual bool OpenRW(const path_t& name) = 0;
	virtual bool OpenNew(const path_t& name) = 0;
	virtual strret_t Read(void* ptr, size_t length) = 0;
	virtual strret_t Write(const void* ptr, strpos_t length) = 0;
	virtual bool SeekStart(stroff_t offset) = 0;
	virtual bool SeekCurrent(stroff_t offset) = 0;
	virtual bool SeekEnd(stroff_t offset) = 0;
};

}
#endif
