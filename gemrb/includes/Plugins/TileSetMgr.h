// SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef TILESETMGR_H
#define TILESETMGR_H

#include "exports.h"

#include "Plugin.h"
#include "Tile.h"

#include "Streams/DataStream.h"

namespace GemRB {

class GEM_EXPORT_T TileSetMgr : public Plugin {
public:
	virtual bool Open(DataStream* stream) = 0;
	virtual Tile* GetTile(const std::vector<ieWord>& indexes,
			      unsigned short* secondary = NULL) = 0;
};

}

#endif
