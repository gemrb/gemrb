// SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef TISIMPORTER_H
#define TISIMPORTER_H

#include "ImageMgr.h"

#include "Plugins/TileSetMgr.h"

namespace GemRB {

struct TISPVRBlock {
	ieDword pvrzPage;
	Point source;
};

class TISImporter : public TileSetMgr {
private:
	DataStream* str = nullptr;
	ieDword headerShift = 0;
	ieDword TilesCount = 0;
	ieDword TilesSectionLen = 0;
	ieDword TileSize = 64;
	bool hasPVRData = false;

	Holder<Sprite2D> badTile; // blank tile to use to fill in bad data
	ResourceHolder<ImageMgr> lastPVRZ;
	ieDword lastPVRZPage = 0;

	Holder<Sprite2D> GetTilePaletted(int index);
	Holder<Sprite2D> GetTilePVR(int index);
	void Blit(const TISPVRBlock& dataBlock, uint8_t* frameData);

public:
	TISImporter() noexcept = default;
	TISImporter(const TISImporter&) = delete;
	~TISImporter() override;
	TISImporter& operator=(const TISImporter&) = delete;
	bool Open(DataStream* stream) override;
	Tile* GetTile(const std::vector<ieWord>& indexes,
		      unsigned short* secondary = NULL) override;
	Holder<Sprite2D> GetTile(int index);
};

}

#endif
