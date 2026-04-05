// SPDX-FileCopyrightText: 2024 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

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
