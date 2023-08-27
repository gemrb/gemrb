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
