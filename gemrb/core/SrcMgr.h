// SPDX-FileCopyrightText: 2022 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef SRCMGR_H
#define SRCMGR_H

#include "exports.h"
#include "ie_types.h"

#include "Resource.h"

#include <vector>

namespace GemRB {

// a list of SRC entries
class GEM_EXPORT SrcVector {
private:
	struct SrcPair {
		ieStrRef ref;
		ieDword weight = 0; // relative weight
	};
	std::vector<SrcPair> strings;
	size_t totalWeight = 0;

public:
	ResRef key;

	explicit SrcVector(const ResRef& resource);

	ieStrRef RandomRef() const;
	bool IsEmpty() const { return strings.empty(); };
};

class GEM_EXPORT SrcMgr {
private:
	ResRefMap<SrcVector> srcs;

public:
	const SrcVector* GetSrc(const ResRef& resource);
};

}

#endif
