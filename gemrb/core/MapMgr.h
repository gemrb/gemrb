// SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

/**
 * @file MapMgr.h
 * Declares MapMgr class, loader for Map objects
 * @author The GemRB Project
 */

#ifndef MAPMGR_H
#define MAPMGR_H

#include "Plugin.h"

namespace GemRB {

class Map;

/**
 * @class MapMgr
 * Abstract loader for Map objects
 */

class GEM_EXPORT MapMgr : public ImporterBase {
public:
	virtual bool ChangeMap(Map* map, bool day_or_night) = 0;
	virtual Map* GetMap(const ResRef& ResRef, bool day_or_night) = 0;

	virtual int GetStoredFileSize(Map* map) = 0;
	virtual int PutArea(DataStream* stream, const Map* map) const = 0;
};

}

#endif
