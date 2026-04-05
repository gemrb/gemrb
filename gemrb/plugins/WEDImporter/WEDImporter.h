// SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef WEDIMPORTER_H
#define WEDIMPORTER_H

#include "Polygon.h"

#include "Plugins/TileMapMgr.h"

#include <vector>

namespace GemRB {

struct Overlay {
	Size size;
	ResRef TilesetResRef;
	ieWord UniqueTileCount; // nNumUniqueTiles in the original (currently unused)
	ieWord MovementType; // nMovementType in the original (currently unused)
	ieDword TilemapOffset;
	ieDword TILOffset;
};

class WEDImporter : public TileMapMgr {
private:
	std::vector<Overlay> overlays;
	DataStream* str = nullptr;
	ieDword OverlaysCount = 0;
	ieDword DoorsCount = 0;
	ieDword OverlaysOffset = 0;
	ieDword SecHeaderOffset = 0;
	ieDword DoorsOffset = 0;
	ieDword DoorTilesOffset = 0;
	ieDword WallPolygonsCount = 0;
	ieDword PolygonsOffset = 0;
	ieDword VerticesOffset = 0;
	ieDword WallGroupsOffset = 0;
	ieDword PLTOffset = 0;
	ieDword DoorPolygonsCount = 0;
	//these will change as doors are being read, so get them in time!
	ieWord OpenPolyCount = 0, ClosedPolyCount = 0;
	ieDword OpenPolyOffset = 0, ClosedPolyOffset = 0;
	bool ExtendedNight = false;

	WallPolygonGroup polygonTable;

private:
	void GetDoorPolygonCount(ieWord count, ieDword offset);
	int AddOverlay(TileMap* tm, const Overlay* newOverlays, bool rain) const;
	void ReadWallPolygons();
	WallPolygonGroup MakeGroupFromTableEntries(size_t idx, size_t cnt) const override;

public:
	WEDImporter() noexcept = default;
	WEDImporter(const WEDImporter&) = delete;
	~WEDImporter() override;
	WEDImporter& operator=(const WEDImporter&) = delete;
	bool Open(DataStream* stream) override;
	//if tilemap already exists, don't create it
	TileMap* GetTileMap(TileMap* tm) const override;
	std::vector<ieWord> GetDoorIndices(const ResRef&, bool& BaseClosed) override;

	std::vector<WallPolygonGroup> GetWallGroups() const override;

	WallPolygonGroup OpenDoorPolygons() const override;
	WallPolygonGroup ClosedDoorPolygons() const override;
	void SetExtendedNight(bool night) override { ExtendedNight = night; }
};

}

#endif
