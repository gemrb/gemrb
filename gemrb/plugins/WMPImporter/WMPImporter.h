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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "WorldMapMgr.h"

#include "ie_types.h"

#include "WorldMap.h"


class WMPImporter : public WorldMapMgr {
private:
	DataStream* str;
	bool autoFree;

	ieDword WorldMapsCount;
	ieDword WorldMapsOffset;

public:
	WMPImporter(void);
	~WMPImporter(void);
	bool Open(DataStream* stream, bool autoFree = true);
	WorldMapArray *GetWorldMapArray();

	int GetStoredFileSize(WorldMapArray *wmap);
	int PutWorldMap(DataStream* stream, WorldMapArray *wmap);
private:
	void GetWorldMap(WorldMap *m, unsigned int index);

	WMPAreaEntry* GetAreaEntry(WMPAreaEntry* ae);
	WMPAreaLink* GetAreaLink(WMPAreaLink* al);
	int PutMaps(DataStream *stream, WorldMapArray *wmap);
	int PutLinks(DataStream *stream, WorldMap *wmap);
	int PutAreas(DataStream *stream, WorldMap *wmap);
};


#endif
