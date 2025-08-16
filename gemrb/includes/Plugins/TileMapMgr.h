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

#ifndef TILEMAPMGR_H
#define TILEMAPMGR_H

#include "exports-core.h"

#include "Plugin.h"
#include "TileMap.h"

#include "Streams/DataStream.h"

namespace GemRB {

class GEM_EXPORT_T TileMapMgr : public Plugin {
public:
	virtual bool Open(DataStream* stream) = 0;
	virtual TileMap* GetTileMap(TileMap* tm) const = 0;
	virtual std::vector<ieWord> GetDoorIndices(const ResRef&, bool& BaseClosed) = 0;
	virtual WallPolygonGroup OpenDoorPolygons() const = 0;
	virtual WallPolygonGroup ClosedDoorPolygons() const = 0;
	virtual void SetExtendedNight(bool night) = 0;

	virtual WallPolygonGroup MakeGroupFromTableEntries(size_t idx, size_t cnt) const = 0;
	virtual std::vector<WallPolygonGroup> GetWallGroups() const = 0;
};

}

#endif
