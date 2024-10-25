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

#include "ie_types.h"

#include "WorldMap.h"
#include "WorldMapMgr.h"

namespace GemRB {


class WMPImporter : public WorldMapMgr {
private:
	DataStream* str1 = nullptr;
	DataStream* str2 = nullptr;

	size_t WorldMapsCount = 0;
	ieDword WorldMapsCount1 = 0;
	ieDword WorldMapsCount2 = 0;
	ieDword WorldMapsOffset1 = 0;
	ieDword WorldMapsOffset2 = 0;

public:
	WMPImporter() noexcept = default;
	WMPImporter(const WMPImporter&) = delete;
	~WMPImporter() override;
	WMPImporter& operator=(const WMPImporter&) = delete;
	bool Open(DataStream* stream1, DataStream* stream2) override;
	WorldMapArray* GetWorldMapArray() const override;

	int GetStoredFileSize(WorldMapArray* wmap, unsigned int index) override;
	int PutWorldMap(DataStream* stream1, DataStream* stream2, WorldMapArray* wmap) const override;

private:
	void GetWorldMap(DataStream* str, WorldMap* m, unsigned int index) const;

	WMPAreaEntry GetAreaEntry(DataStream* str) const;
	WMPAreaLink* GetAreaLink(DataStream* str, WMPAreaLink* al) const;
	int PutMaps(DataStream* stream1, DataStream* stream2, const WorldMapArray* wmap) const;
	int PutMap(DataStream* stream, const WorldMapArray* wmap, unsigned int index) const;
	int PutLinks(DataStream* stream, const WorldMap* wmap) const;
	int PutAreas(DataStream* stream, const WorldMap* wmap) const;
};


}

#endif
