// SPDX-FileCopyrightText: 2024 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef H_GEMRB_MURMUR_HASH
#define H_GEMRB_MURMUR_HASH

#include "exports.h"

#include <cstdint>

namespace GemRB {

struct GEM_EXPORT MurmurHash {
	uint32_t value = 0;

	MurmurHash() = default;
	MurmurHash(uint32_t value);
	bool operator==(const MurmurHash& other) const;
	bool operator!=(const MurmurHash& other) const;
	bool operator==(uint32_t value) const;
};

class GEM_EXPORT MurmurHash3_32 {
public:
	MurmurHash3_32() = default;

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
