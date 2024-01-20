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

#include "ResourceSource.h"

#include "Plugins/IndexedArchive.h"
#include "PluginMgr.h"
#include "Resource.h"
#include "System/VFS.h"

#include <unordered_map>
#include <utility>
#include <vector>

namespace GemRB {

class DataStream;
class ResourceDesc;

struct BIFEntry {
	path_t name;
	ieWord BIFLocator;
	path_t path;
	int cd;
	bool found;
};

struct MapKey {
	ResRef ref;
	uint64_t type = 0;

	MapKey() = default;
	MapKey(const ResRef& ref, uint64_t type) : ref{ref}, type{type} {}
	MapKey(ResRef && ref, uint64_t type) : ref{std::move(ref)}, type{type} {}

	bool operator==(const MapKey& other) const {
		return ref == other.ref && type == other.type;
	}
};

struct MapKeyHash {
	size_t operator()(const MapKey& key) const {
		uint64_t h{key.type};
		
		for (const char c : key.ref)
		{
			h = (h << 5) + h + tolower(c);
		}

		return h;
	}
};

struct KEYCache {
	KEYCache() { bifnum = 0xffffffff; }

	unsigned int bifnum;
	PluginHolder<IndexedArchive> plugin;
};

class KEYImporter : public ResourceSource {
private:
	std::vector< BIFEntry> biffiles;
	std::unordered_map<MapKey, ieDword, MapKeyHash> resources;

	/** Gets the stream associated to a RESKey */
	DataStream *GetStream(const ResRef&, ieWord type);
public:
	bool Open(const path_t& file, std::string desc) override;
	/* predicts the availability of a resource */
	bool HasResource(StringView resname, SClass_ID type) override;
	bool HasResource(StringView resname, const ResourceDesc &type) override;
	/* returns resource */
	DataStream* GetResource(StringView resname, SClass_ID type) override;
	DataStream* GetResource(StringView resname, const ResourceDesc &type) override;
};

}

#endif
