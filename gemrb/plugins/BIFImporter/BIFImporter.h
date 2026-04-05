// SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef BIFIMPORTER_H
#define BIFIMPORTER_H

#include "Plugins/IndexedArchive.h"
#include "Streams/DataStream.h"

namespace GemRB {

struct FileEntry {
	ieDword resLocator;
	ieDword dataOffset;
	ieDword fileSize;
	ieWord type;
	ieWord u1; //Unknown Field, part of type dword in ee
};

struct TileEntry {
	ieDword resLocator;
	ieDword dataOffset;
	ieDword tilesCount;
	ieDword tileSize; //named tilesize so it isn't confused
	ieWord type;
	ieWord u1; //Unknown Field, part of type dword in ee
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
