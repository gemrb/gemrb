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

#ifndef KEYIMP_H
#define KEYIMP_H

#include <vector>
#include "../Core/ResourceSource.h"
#include "Dictionary.h"

class Resource;
class ResourceDesc;
class DataStream;

struct RESEntry {
	ieResRef ResRef;
	ieWord   Type;
	ieDword  ResLocator;
};

struct BIFEntry {
	char* name;
	ieWord BIFLocator;
	char path[_MAX_PATH];
	int cd;
	bool found;
};


class KEYImporter : public ResourceSource {
private:
	std::vector< BIFEntry> biffiles;
	Dictionary resources;

	/** Gets the stream assoicated to a RESKey */
	DataStream *GetStream(const char *resname, ieWord type);
public:
	KEYImporter(void);
	~KEYImporter(void);
	bool Open(const char *file, const char *desc);
	/* predicts the availability of a resource */
	bool HasResource(const char* resname, SClass_ID type);
	bool HasResource(const char* resname, const ResourceDesc &type);
	/* returns resource */
	DataStream* GetResource(const char* resname, SClass_ID type);
	DataStream* GetResource(const char* resname, const ResourceDesc &type);
public:
	void release(void)
	{
		delete this;
	}
};

#endif
