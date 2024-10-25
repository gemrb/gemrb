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

#ifndef SLICEDSTREAM_H
#define SLICEDSTREAM_H

#include "exports.h"

#include "DataStream.h"

namespace GemRB {

class GEM_EXPORT SlicedStream : public DataStream {
private:
	//	bool autoFree;
	strpos_t startpos;
	DataStream* str;

public:
	SlicedStream(const DataStream* cfs, strpos_t startPos, strpos_t streamSize);
	~SlicedStream() override;
	DataStream* Clone() const noexcept override;

	strret_t Read(void* dest, strpos_t length) override;
	strret_t Write(const void* src, strpos_t length) override;
	stroff_t Seek(stroff_t pos, strpos_t startpos) override;
};

GEM_EXPORT DataStream* SliceStream(DataStream* str, strpos_t startpos, strpos_t size, bool preservepos = false);

}

#endif
