/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2011 The GemRB Project
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

#ifndef STRINGMAP_H
#define STRINGMAP_H

#include <string>

#include "ie_types.h"

#include "System/String.h"
#include "HashMap.h"

namespace GemRB {

// Use "StringMap" for mapping std::strings to std::strings.
// This does not limit the length of either the keys nor values, but at the
// cost of (re)allocs for each string.
//
// Use HashMap<char[size], ...> to use fixed-sized char arrays as keys.
// This has the advantage that HashMap will alloc the char array as part of
// its blocks.

template<>
struct HashKey<std::string> {
	// hash without std::string construction
	static inline unsigned int hash(const char *key)
	{
		unsigned int h = 0;

		while (*key)
			h = (h << 5) + h + tolower(*key++);

		return h;
	}

	static inline unsigned int hash(const std::string &key)
	{
		return hash(key.c_str());
	}

	// equal check without std::string construction
	static inline bool equals(const std::string &a, const char *b)
	{
		return stricmp(a.c_str(), b) == 0;
	}

	static inline bool equals(const std::string &a, const std::string &b)
	{
		return equals(a, b.c_str());
	}

	static inline void copy(std::string &a, const std::string &b)
	{
		a = b;
	}
};

class StringMap : public HashMap<std::string, std::string> {
public:
	// lookup without std::string construction
	const std::string *get(const char *key) const
	{
		if (!isInitialized())
			return NULL;

		incAccesses();

		for (Entry *e = getBucketByHash(HashKey<std::string>::hash(key)); e; e = e->next)
			if (HashKey<std::string>::equals(e->key, key))
				return &e->value;

		return NULL;
	}

	// lookup without std::string construction
	bool has(const char *key) const
	{
		return get(key) != NULL;
	}
};

// disabled, msvc6 hates it
#if 0
template<unsigned int size>
struct HashKey<char[size]> {
	typedef char array[size];

	static inline unsigned int hash(const array &key)
	{
		unsigned int h = 0;
		const char *c = key;

		for (unsigned int i = 0; *c && i < size; ++i)
			h = (h << 5) + h + tolower(*c++);

		return h;
	}

	static inline bool equals(const array &a, const array &b)
	{
		return strnicmp(a, b, size) == 0;
	}

	static inline void copy(array &a, const array &b)
	{
		strncpy(a, b, size);
	}
};
#endif

}

#endif
