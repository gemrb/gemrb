// SPDX-FileCopyrightText: 2010 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef POLYMORPHCACHE_H
#define POLYMORPHCACHE_H

#include "ie_types.h"

namespace GemRB {

struct GEM_EXPORT PolymorphCache {
	ResRef Resource;
	std::vector<ieDword> stats;
};

}

#endif
