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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

/**
 * @file WorldMapMgr.h
 * Declares WorldMapMgr class, loader for WorldMap objects
 * @author The GemRB Project
 */

#ifndef WORLDMAPMGR_H
#define WORLDMAPMGR_H

#include "Plugin.h"
#include "WorldMap.h"
#include "System/DataStream.h"

namespace GemRB {

/**
 * @class WorldMapMgr
 * Abstract loader for WorldMap objects
 */

class GEM_EXPORT WorldMapMgr : public Plugin {
public:
	WorldMapMgr(void);
	virtual ~WorldMapMgr(void);
	virtual bool Open(DataStream* stream1, DataStream* stream2) = 0;
	virtual WorldMapArray* GetWorldMapArray() = 0;

	virtual int GetStoredFileSize(WorldMapArray *wmap, unsigned int index) = 0;
	virtual int PutWorldMap(DataStream* stream1, DataStream* stream2, WorldMapArray *wmap) = 0;
};

}

#endif
