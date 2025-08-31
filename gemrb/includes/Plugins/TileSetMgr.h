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
