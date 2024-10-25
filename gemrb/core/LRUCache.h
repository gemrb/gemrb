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

#include "Strings/StringView.h"

#include <tuple>
#include <unordered_map>
#include <utility>

namespace GemRB {

struct VarEntry;

/* Not thread-safe, PRED is an eviction predicate */
template<typename T, class PRED>
class LRUCache {
public:
	using key_t = std::string;
	using public_key_t = StringView;

private:
	/* Queue items are placed into a doubly-linked list, the front element is the LRU. */
	struct QueueItem {
		QueueItem* prev = nullptr;
		QueueItem* next = nullptr;
		const key_t& key;

		explicit QueueItem(const key_t& key)
			: key(key) {}
		QueueItem(const key_t& key, QueueItem* prev)
			: prev(prev), key(key)
		{}
	};

	/* Cache items hold the value and a short cut to the queue item to be moved to the end. */
	struct CacheItem {
		QueueItem* queueItem = nullptr;
		T value;

		template<typename... ARGS>
		explicit CacheItem(ARGS&&... args)
			: value(std::forward<ARGS>(args)...)
		{}
	};

	QueueItem* front = nullptr;
	QueueItem* back = nullptr;
	std::unordered_map<key_t, CacheItem> map;
	size_t cacheSize;
	PRED predicate;

public:
	explicit LRUCache(size_t size)
		: cacheSize(size) {}
	~LRUCache()
	{
		auto next = front;

		while (next != nullptr) {
			auto _next = next->next;
			delete next;
			next = _next;
		}
	}

	template<typename... ARGS>
	void SetAt(const public_key_t& key, ARGS&&... args)
	{
		if (map.size() == cacheSize) {
			evict();
		}

		auto insertion =
			map.emplace(
				std::piecewise_construct,
				std::forward_as_tuple(key.c_str()),
				std::forward_as_tuple(std::forward<ARGS>(args)...));

		if (!insertion.second) {
			return;
		}

		if (back == nullptr) {
			this->back = new QueueItem(insertion.first->first);
		} else {
			auto _back = back;
			this->back = new QueueItem(insertion.first->first, _back);
			_back->next = back;
		}

		if (front == nullptr) {
			this->front = back;
		}

		insertion.first->second.queueItem = back;
	}

	const T* Lookup(const public_key_t& key) const
	{
		std::string _key { key.c_str() };
		auto lookup = map.find(_key);

		return lookup != map.cend() ? &lookup->second.value : nullptr;
	}

	bool Touch(const public_key_t& key)
	{
		std::string _key { key.c_str() };
		auto lookup = map.find(_key);

		if (lookup != map.cend()) {
			moveToBack(lookup->second.queueItem);
			return true;
		}

		return false;
	}

	bool Remove(const public_key_t& key)
	{
		std::string _key { key.c_str() };
		auto lookup = map.find(_key);

		if (lookup != map.cend()) {
			unlink(lookup->second.queueItem);
			delete lookup->second.queueItem;
			map.erase(lookup);

			return true;
		}

		return false;
	}

private:
	void evict()
	{
		auto next = front;
		while (next != nullptr) {
			auto lookup = map.find(next->key);

			if (next->next == nullptr || predicate(lookup->second.value)) {
				/* This is because OpenAL could have done stuff in predicate. */
				lookup->second.value.evictionNotice();

				map.erase(lookup);
				unlink(next);
				delete next;
				break;
			}
			next = next->next;
		}
	}

	void moveToBack(QueueItem* item)
	{
		unlink(item);
		item->prev = back;
		if (item->prev != nullptr) {
			item->prev->next = item;
		}
		this->back = item;
	}

	void unlink(QueueItem* item)
	{
		if (item->prev != nullptr) {
			item->prev->next = item->next;
		} else {
			this->front = item->next;
		}

		if (item->next != nullptr) {
			item->next->prev = item->prev;
		} else {
			this->back = item->prev;
		}

		item->prev = nullptr;
		item->next = nullptr;
	}
};

}

#endif
