// SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef KEYIMP_H
#define KEYIMP_H

#include "PluginMgr.h"
#include "ResourceSource.h"

#include "Plugins/IndexedArchive.h"
#include "System/VFS.h"

#include <unordered_map>
#include <utility>
#include <vector>

namespace GemRB {

class DataStream;

struct BIFEntry {
	path_t name;
	ieWord BIFLocator = 0;
	path_t path;
	int cd = 0;
	bool found;
};

struct MapKey {
	ResRef ref;
	uint64_t type = 0;

	MapKey() = default;
	MapKey(const ResRef& ref, uint64_t type)
		: ref { ref }, type { type } {}
	MapKey(ResRef&& ref, uint64_t type)
		: ref { std::move(ref) }, type { type } {}

	bool operator==(const MapKey& other) const
	{
		return ref == other.ref && type == other.type;
	}
};

struct MapKeyHash {
	size_t operator()(const MapKey& key) const
	{
		uint64_t h { key.type };

		for (const char c : key.ref) {
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
	std::vector<BIFEntry> biffiles;
	std::unordered_map<MapKey, ieDword, MapKeyHash> resources;

	/** Gets the stream associated to a RESKey */
	DataStream* GetStream(const ResRef&, ieWord type);

public:
	bool Open(const path_t& file, std::string desc) override;
	void MergeBifsFromPath(const path_t& directory) override;
	/* predicts the availability of a resource */
	bool HasResource(StringView resname, SClass_ID type) override;
	bool HasResource(StringView resname, const ResourceDesc& type) override;
	/* returns resource */
	DataStream* GetResource(StringView resname, SClass_ID type) override;
	DataStream* GetResource(StringView resname, const ResourceDesc& type) override;
};

}

#endif
