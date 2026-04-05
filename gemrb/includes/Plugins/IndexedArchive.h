// SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef INDEXEDARCHIVE_H
#define INDEXEDARCHIVE_H

#include "exports.h"
#include "globals.h"

#include "Plugin.h"

namespace GemRB {

class GEM_EXPORT_T IndexedArchive : public Plugin {
public:
	virtual int OpenArchive(const path_t& filename) = 0;
	virtual DataStream* GetStream(unsigned long Resource, unsigned long Type) = 0;
};

}

#endif
