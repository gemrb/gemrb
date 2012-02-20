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

#ifndef WMPIMPORTER_H
#define WMPIMPORTER_H

#include "WorldMapMgr.h"

#include "ie_types.h"

#include "WorldMap.h"

namespace GemRB {


class WMPImporter : public WorldMapMgr {
private:
	DataStream* str1;
	DataStream* str2;

	ieDword WorldMapsCount;
	ieDword WorldMapsCount1, WorldMapsCount2;
	ieDword WorldMapsOffset1, WorldMapsOffset2;

public:
	WMPImporter(void);
	~WMPImporter(void);
	bool Open(DataStream* stream1, DataStream* stream2);
	WorldMapArray *GetWorldMapArray();

	int GetStoredFileSize(WorldMapArray *wmap, unsigned int index);
	int PutWorldMap(DataStream* stream1, DataStream* stream2, WorldMapArray *wmap);
private:
	void GetWorldMap(DataStream *str, WorldMap *m, unsigned int index);

	WMPAreaEntry* GetAreaEntry(DataStream *str, WMPAreaEntry* ae);
	WMPAreaLink* GetAreaLink(DataStream *str, WMPAreaLink* al);
	int PutMaps(DataStream *stream1, DataStream *stream2, WorldMapArray *wmap);
	int PutMap(DataStream *stream, WorldMapArray *wmap, unsigned int index);
	int PutLinks(DataStream *stream, WorldMap *wmap);
	int PutAreas(DataStream *stream, WorldMap *wmap);
};


}

#endif
