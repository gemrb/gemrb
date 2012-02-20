/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2007
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

#ifndef LRUCACHE_H
#define LRUCACHE_H 

#include "exports.h"

#include "Variables.h"

namespace GemRB {

struct VarEntry;

class GEM_EXPORT LRUCache {
public:
	LRUCache();
	~LRUCache();

	// set value, overwriting any previous entry
	void SetAt(const char* key, void* value);
	bool Lookup(const char* key, void*& value) const;
	bool Touch(const char* key);
	bool Remove(const char* key);

	int GetCount() const;

	// return n-th LRU entry. key remains owned by LRUCache.
	// (n = 0 is least recently used, n = 1 the next least recently used,
	//  etc...)
	bool getLRU(unsigned int n, const char*& key, void*& value) const;

private:
	// internal storage
	Variables v;
	VarEntry* head;
	VarEntry* tail;

	void removeFromList(VarEntry* e);
};



}

#endif
