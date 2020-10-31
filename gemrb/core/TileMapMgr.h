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

#include "Plugin.h"
#include "TileMap.h"
#include "System/DataStream.h"

namespace GemRB {

class GEM_EXPORT TileMapMgr : public Plugin {
public:
	TileMapMgr(void);
	virtual ~TileMapMgr(void);
	virtual bool Open(DataStream* stream) = 0;
	virtual TileMap* GetTileMap(TileMap *tm) const = 0;
	virtual ieWord* GetDoorIndices(char* ResRef, int* count,
		bool& BaseClosed) = 0;
	virtual void SetupOpenDoor(unsigned int &index, unsigned int &count) const = 0;
	virtual void SetupClosedDoor(unsigned int &index, unsigned int &count) const = 0;
	virtual void SetExtendedNight(bool night) = 0;

	virtual Wall_Polygon** GetWallGroups() const = 0;
	//returns only the wall polygon counts
	virtual ieDword GetWallPolygonsCount() const = 0;
	//returns Wall + Door polygon counts
	virtual ieDword GetPolygonsCount() const = 0;
};

}

#endif
