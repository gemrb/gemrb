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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/BIFImporter/BIFImp.h,v 1.7 2004/08/01 19:13:03 guidoj Exp $
 *
 */

#ifndef BIFIMP_H
#define BIFIMP_H

#include "../../includes/globals.h"
#include "../Core/ArchiveImporter.h"
#include "../Core/CachedFileStream.h"

typedef struct FileEntry {
	ie_uint32 resLocator;
	ie_uint32 dataOffset;
	ie_uint32 fileSize;
	ie_uint16 type;
	ie_uint16 u1; //Unknown Field
} FileEntry;

typedef struct TileEntry {
	ie_uint32 resLocator;
	ie_uint32 dataOffset;
	ie_uint32 tilesCount;
	ie_uint32 tileSize; //named tilesize so it isn't confused
	ie_uint16 type;
	ie_uint16 u1; //Unknown Field
} TileEntry;

class BIFImp : public ArchiveImporter {
private:
	char path[_MAX_PATH];
	FileEntry* fentries;
	TileEntry* tentries;
	unsigned int fentcount, tentcount;
	CachedFileStream* stream;
public:
	BIFImp(void);
	~BIFImp(void);
	int OpenArchive(char* filename);
	DataStream* GetStream(unsigned long Resource, unsigned long Type);
private:
	void ReadBIF(void);
public:
	void release(void)
	{
		delete this;
	}
};

#endif
