// SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

/**
 * @file StoreMgr.h
 * Declares StoreMgr class, loader for Store objects
 * @author The GemRB Project
 */


#ifndef STOREMGR_H
#define STOREMGR_H

#include "Plugin.h"
#include "Store.h"

#include "Streams/DataStream.h"

namespace GemRB {

/**
 * @class StoreMgr
 * Abstract loader for Store objects
 */

class GEM_EXPORT StoreMgr : public Plugin {
public:
	virtual bool Open(DataStream* stream) = 0;
	virtual Store* GetStore(Store* s) = 0;

	virtual bool PutStore(DataStream* stream, Store* s) = 0;
};

}

#endif
