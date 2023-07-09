/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

#ifndef BIFIMPORTER_H
#define BIFIMPORTER_H

#include "Plugins/IndexedArchive.h"

#include "globals.h"

#include "Streams/DataStream.h"

namespace GemRB {

struct FileEntry {
	ieDword resLocator;
	ieDword dataOffset;
	ieDword fileSize;
	ieWord  type;
	ieWord  u1; //Unknown Field, part of type dword in ee
};

struct TileEntry {
	ieDword resLocator;
	ieDword dataOffset;
	ieDword tilesCount;
	ieDword tileSize; //named tilesize so it isn't confused
	ieWord  type;
	ieWord  u1; //Unknown Field, part of type dword in ee
};

class BIFImporter : public IndexedArchive {
private:
	FileEntry* fentries = nullptr;
	TileEntry* tentries = nullptr;
	ieDword fentcount = 0;
	ieDword tentcount = 0;
	DataStream* stream = nullptr;
public:
	BIFImporter() noexcept = default;
	BIFImporter(const BIFImporter&) = delete;
	~BIFImporter() override;
	BIFImporter& operator=(const BIFImporter&) = delete;
	int OpenArchive(const path_t& filename) override;
	DataStream* GetStream(unsigned long Resource, unsigned long Type) override;
private:
	static DataStream* DecompressBIF(DataStream* compressed, const path_t& path);
	static DataStream* DecompressBIFC(DataStream* compressed, const path_t& path);
	int ReadBIF();
};

}

#endif
