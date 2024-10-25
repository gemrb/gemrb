/* GemRB - Infinity Engine Emulator
* Copyright (C) 2024 The GemRB Project
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 2
* of the License, or (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*
*/

#ifndef H_GEMRB_MURMUR_HASH
#define H_GEMRB_MURMUR_HASH

#include "exports.h"

#include <cstdint>

namespace GemRB {

struct GEM_EXPORT MurmurHash {
	uint32_t value = 0;

	MurmurHash() {};
	MurmurHash(uint32_t value);
	bool operator==(const MurmurHash& other) const;
	bool operator!=(const MurmurHash& other) const;
	bool operator==(uint32_t value) const;
};

class GEM_EXPORT MurmurHash3_32 {
public:
	MurmurHash3_32() {};

	void Feed(uint32_t value);
	MurmurHash GetHash() const;

private:
	uint32_t state = 0;
	uint32_t calls = 0;
};

using Hasher = MurmurHash3_32;
using Hash = MurmurHash;

}

#endif
