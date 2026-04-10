// SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

/**
 * @file WorldMapMgr.h
 * Declares WorldMapMgr class, loader for WorldMap objects
 * @author The GemRB Project
 */

#ifndef WORLDMAPMGR_H
#define WORLDMAPMGR_H

#include "Plugin.h"
#include "WorldMap.h"

#include "Streams/DataStream.h"

namespace GemRB {

/**
 * @class WorldMapMgr
 * Abstract loader for WorldMap objects
 */

class GEM_EXPORT WorldMapMgr : public Plugin {
public:
	virtual bool Open(DataStream* stream1, DataStream* stream2) = 0;
	virtual WorldMapArray* GetWorldMapArray() const = 0;

	virtual int GetStoredFileSize(const WorldMapArray* wmap, unsigned int index) = 0;
	virtual int PutWorldMap(DataStream* stream1, DataStream* stream2, const WorldMapArray* wmap) const = 0;
};

}

#endif
