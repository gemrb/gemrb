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

#include "MurmurHash.h"

namespace GemRB {

MurmurHash::MurmurHash(uint32_t v)
	: value(v) {}

bool MurmurHash::operator==(const MurmurHash& other) const
{
	return value == other.value;
}

bool MurmurHash::operator!=(const MurmurHash& other) const
{
	return !operator==(other);
}

bool MurmurHash::operator==(uint32_t other) const
{
	return value == other;
}

void MurmurHash3_32::Feed(uint32_t value)
{
	value *= 0xCC9E2D51;
	value = (value << 15) | (value >> 17);
	value *= 0x1B873593;

	state ^= value;
	state = (state << 13) | (state >> 19);
	state = state * 5 + 0xE6546B64;
	calls++;
}

MurmurHash MurmurHash3_32::GetHash() const
{
	uint32_t hash = state;
	hash ^= calls * 4;
	hash ^= hash >> 16;
	hash *= 0x85EBCA6B;
	hash ^= hash >> 13;
	hash *= 0xC2B2AE35;
	hash ^= hash >> 16;

	return { hash };
}

}
