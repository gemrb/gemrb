/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 |Avenger|
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

#ifndef CACHE_H
#define CACHE_H

#include <tuple>
#include <unordered_map>
#include <utility>

#include "globals.h"
#include "ie_types.h"

namespace GemRB {

#ifndef ReleaseFun
using ReleaseFun = void (*)(void*);
#endif

/* Reference counting cache, a layer between STL containers and existing interfaces. */
template<typename K, typename V, typename H>
class RCCache {
	private:
		struct Value {
			V value;
			int64_t refCount = 1;

			explicit Value(V && value) : value(std::forward<V>(value)) {}

			template<typename ... ARGS>
			explicit Value(ARGS && ... args) : value(std::forward<ARGS>(args)...) {}
		};

		std::unordered_map<K, Value, H> map;

	public:
		V* GetResource(const K& key) {
			auto lookup = map.find(key);
			if (lookup != map.cend()) {
				lookup->second.refCount++;

				return &lookup->second.value;
			}

			return nullptr;
		}

		template<typename ... ARGS>
		std::pair<V*, bool> SetAt(const K& key, ARGS && ... args) {
			auto insertion =
				map.emplace(
					std::piecewise_construct,
					std::forward_as_tuple(key),
					std::forward_as_tuple(std::forward<ARGS>(args)...)
				);

			return {&insertion.first->second.value, insertion.second};
		}

		int64_t DecRef(const K& key, bool remove) {
			auto lookup = map.find(key);

			if (lookup != map.end()) {
				auto& valueItem = lookup->second;

				if (valueItem.refCount > 0) {
					valueItem.refCount--;
				}

				if (remove && valueItem.refCount == 0) {
					map.erase(lookup);

					return 0;
				} else {
					return valueItem.refCount;
				}
			}

			return -1;
		}

		int64_t RefCount(const K& key) const {
			auto lookup = map.find(key);

			if (lookup != map.cend()) {
				return lookup->second.RefCount;
			}

			return -1;
		}
};

template<typename V>
using ResRefRCCache = RCCache<ResRef, V, CstrHashCI<ResRef>>;

}

#endif //CACHE_H
