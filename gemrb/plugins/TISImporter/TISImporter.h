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
