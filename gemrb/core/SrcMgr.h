/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

#ifndef SRCMGR_H
#define SRCMGR_H

#include "exports.h"
#include "ie_types.h"

#include "Cache.h"

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

	SrcVector(const ResRef& resource);

	ieStrRef RandomRef() const;
	bool IsEmpty() const { return strings.empty(); };
};

class GEM_EXPORT SrcMgr {
private:
	std::vector<const SrcVector*> srcs;
	Cache SrcCache;

public:
	~SrcMgr();

	const SrcVector* GetSrc(const ResRef& resource);
};

}

#endif
