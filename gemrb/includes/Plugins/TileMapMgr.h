// SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef TILEMAPMGR_H
#define TILEMAPMGR_H

#include "exports.h"

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
