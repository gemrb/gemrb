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

#ifndef BIFIMP_H
#define BIFIMP_H

#include "../../includes/globals.h"
#include "../Core/ArchiveImporter.h"
#include "../Core/CachedFileStream.h"

struct FileEntry {
	ieDword resLocator;
	ieDword dataOffset;
	ieDword fileSize;
	ieWord  type;
	ieWord  u1; //Unknown Field
};

struct TileEntry {
	ieDword resLocator;
	ieDword dataOffset;
	ieDword tilesCount;
	ieDword tileSize; //named tilesize so it isn't confused
	ieWord  type;
	ieWord  u1; //Unknown Field
};

class BIFImp : public ArchiveImporter {
private:
	char path[_MAX_PATH];
	FileEntry* fentries;
	TileEntry* tentries;
	ieDword fentcount, tentcount;
	CachedFileStream* stream;
public:
	BIFImp(void);
	~BIFImp(void);
	int DecompressSaveGame(DataStream *compressed);
	int AddToSaveGame(DataStream *str, DataStream *uncompressed);
	int OpenArchive(const char* filename);
	int CreateArchive(DataStream *compressed);
	DataStream* GetStream(unsigned long Resource, unsigned long Type, bool silent=false);
private:
	void ReadBIF(void);
public:
	void release(void)
	{
		delete this;
	}
};

#endif
