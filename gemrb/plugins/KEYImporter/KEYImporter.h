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

#include "IndexedArchive.h"
#include "PluginMgr.h"

#include "StringMap.h"

#include <vector>

namespace GemRB {

class DataStream;
class Resource;
class ResourceDesc;

struct BIFEntry {
	char* name;
	ieWord BIFLocator;
	char path[_MAX_PATH];
	int cd;
	bool found;
};

struct KEYCache {
	KEYCache() { bifnum = 0xffffffff; }

	unsigned int bifnum;
	PluginHolder<IndexedArchive> plugin;
};

// the key for this specific hashmap
struct MapKey {
	ieResRef ref;
	ieWord type;

	MapKey()
	{
	}
};

// hash template for the above key for this hashmap
template<>
struct HashKey<MapKey> {
	// hash without MapKey construction
	static inline unsigned int hash(const ieResRef ref, SClass_ID type)
	{
		unsigned int h = type;
		const char *c = ref;

		for (unsigned int i = 0; *c && i < sizeof(ieResRef); ++i)
			h = (h << 5) + h + tolower(*c++);

		return h;
	}

	static inline unsigned int hash(const MapKey &key)
	{
		return hash(key.ref, key.type);
	}

	// equal check without MapKey construction
	static inline bool equals(const MapKey &a, const ieResRef ref, SClass_ID type)
	{
		if (a.type != type)
			return false;

		return stricmp(a.ref, ref) == 0;
	}

	static inline bool equals(const MapKey &a, const MapKey &b)
	{
		return equals(a, b.ref, b.type);
	}

	static inline void copy(MapKey &a, const MapKey &b)
	{
		a.type = b.type;
		strncpy(a.ref, b.ref, sizeof(ieResRef));
	}
};

// FIXME: The captialization of KEYMap is important, since there is also a KeyMap defined in core.
class KEYMap : public HashMap<MapKey, ieDword> {
public:
	// lookup without MapKey construction
	const ieDword *get(const ieResRef ref, SClass_ID type) const
	{
		if (!isInitialized())
			return NULL;

		incAccesses();

		for (Entry *e = getBucketByHash(HashKey<MapKey>::hash(ref, type)); e; e = e->next)
			if (HashKey<MapKey>::equals(e->key, ref, type))
				return &e->value;

		return NULL;
	}

	// lookup without MapKey construction
	bool has(const ieResRef ref, SClass_ID type) const
	{
		return get(ref, type) != NULL;
	}
};

class KEYImporter : public ResourceSource {
private:
	std::vector< BIFEntry> biffiles;
	KEYMap resources;

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
};

}

#endif
