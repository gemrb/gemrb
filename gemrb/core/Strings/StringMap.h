/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2023 The GemRB Project
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
 */

#ifndef STRING_MAP_H
#define STRING_MAP_H

#include "CString.h"
#include "StringView.h"

#include <unordered_map>

namespace GemRB {

class GEM_EXPORT HeterogeneousStringKey {
	std::unique_ptr<std::string> keyBuf;
	StringView key;

public:
	explicit HeterogeneousStringKey(std::string str) noexcept;
	HeterogeneousStringKey(StringView sv) noexcept;
	HeterogeneousStringKey(HeterogeneousStringKey&& other) noexcept = default;
	HeterogeneousStringKey(const HeterogeneousStringKey& other) noexcept;

	HeterogeneousStringKey& operator=(const HeterogeneousStringKey& other) noexcept;
	HeterogeneousStringKey& operator=(HeterogeneousStringKey&& other) = default;

	operator StringView() const noexcept;
};

// this exists simply for dumping the map to a string
GEM_EXPORT fmt::string_view format_as(const HeterogeneousStringKey& key);

// String map is case insensitive by default
// lookup is "heterogeneous", so it doesnt depend on constructing a new string for lookup
template<typename V, typename HASH = CstrHashCI, typename CMP = CstrEqCI>
class StringMap {
	using Key_t = HeterogeneousStringKey;
	using Map_t = std::unordered_map<Key_t, V, HASH, CMP>;
	Map_t map;
public:
	using value_type = V;

	const V& Set(const StringView& key, V value) {
		auto it = map.find(key);
		if (it == map.end()) {
			auto ins = map.insert(std::make_pair(key.MakeString(), std::move(value)));
			assert(ins.second);
			it = ins.first;
		} else {
			it->second = std::move(value);
		}
		return it->second;
	}

	template <typename T>
	auto SetAs(const StringView& key, T value) {
		static_assert(sizeof(T) <= sizeof(V), "Cannot truncate value. Use Set() and static_cast if it must be done.");
		return Set(key, static_cast<V>(value));
	}

	void Merge(StringMap&& other) {
		map.insert(std::make_move_iterator(other.map.begin()),
				   std::make_move_iterator(other.map.end()));
	}

	void Erase(const StringView& key) {
		map.erase(key);
	}

	WARN_UNUSED bool Contains(const StringView& key) const {
		return map.count(key);
	}

	WARN_UNUSED const V* Get(const StringView& key) const {
		auto it = map.find(key);
		if (it == map.end()) {
			return nullptr;
		} else {
			return &it->second;
		}
	}

	WARN_UNUSED const V& Get(const StringView& key, const V& fallback) const {
		auto it = map.find(key);
		if (it == map.end()) {
			return fallback;
		} else {
			return it->second;
		}
	}

	template <typename T>
	WARN_UNUSED const T GetAs(const StringView& key, T fallback = V()) const {
		static_assert(sizeof(T) >= sizeof(V), "Cannot truncate value. Use Get() and static_cast if it must be done.");
		return static_cast<T>(Get(key, static_cast<V>(fallback)));
	}

	auto size() const {
		return map.size();
	}

	auto begin() const {
		return map.begin();
	}

	auto end() const {
		return map.end();
	}
};

}

#endif /* STRING_MAP_H */
