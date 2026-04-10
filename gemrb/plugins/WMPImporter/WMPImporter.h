// SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

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
	std::unique_ptr<WorldMapArray> GetWorldMapArray() const override;

	int GetStoredFileSize(const WorldMapArray* wmap, unsigned int index) override;
	int PutWorldMap(DataStream* stream1, DataStream* stream2, const WorldMapArray* wmap) const override;

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
