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

#ifndef HASHMAP_H
#define HASHMAP_H

#include <cassert>
#include <cstring>
#include <deque>

#include "System/Logging.h"

// dumps stats upon destruction
//#define HASHMAP_DEBUG

template<typename T>
struct HashKey {
	static inline unsigned int hash(const T &key);
	static inline bool equals(const T &a, const T &b);
	static inline void copy(T &a, const T &b);
};

#define HASHMAP_DEFINE_TRIVIAL_HASHKEY(T)			\
	template<>						\
	inline unsigned int HashKey<T>::hash(const T &key)	\
	{							\
		return static_cast<unsigned int>(key);		\
	}							\
	template<>						\
	inline bool HashKey<T>::equals(const T &a, const T &b)	\
	{							\
		return a == b;					\
	}							\
	template<>						\
	inline void HashKey<T>::copy(T &a, const T &b)		\
	{							\
		a = b;						\
	}

// MSVC6 is convinced that this the same as char[1] from StringMap.h
//HASHMAP_DEFINE_TRIVIAL_HASHKEY(char);
//HASHMAP_DEFINE_TRIVIAL_HASHKEY(signed char);
//HASHMAP_DEFINE_TRIVIAL_HASHKEY(unsigned char);

HASHMAP_DEFINE_TRIVIAL_HASHKEY(short)
HASHMAP_DEFINE_TRIVIAL_HASHKEY(unsigned short)
HASHMAP_DEFINE_TRIVIAL_HASHKEY(int)
HASHMAP_DEFINE_TRIVIAL_HASHKEY(unsigned int)
HASHMAP_DEFINE_TRIVIAL_HASHKEY(long)
HASHMAP_DEFINE_TRIVIAL_HASHKEY(unsigned long)

#undef HASHMAP_DEFINE_TRIVIAL_HASHKEY

template<typename Key, typename Value, typename Hash = HashKey<Key> >
class HashMap {
public:
	HashMap();
	~HashMap();

	void init(unsigned int tableSize, unsigned int blockSize);

	// sets a value and returns true if an existing entry has been replaced
	bool set(const Key &key, const Value &value);
	const Value *get(const Key &key) const;
	bool has(const Key &key) const;

	bool remove(const Key &key);
	void clear();

protected:
	struct Entry {
		Key key;
		Value value;
		Entry *next;
	};

	inline bool isInitialized() const;

	inline Entry *popAvailable();
	inline void pushAvailable(Entry *e);
	inline unsigned int getMapPosByHash(unsigned int hash) const;
	inline unsigned int getMapPosByKey(const Key &key) const;
	inline Entry *getBucketByHash(unsigned int hash) const;
	inline Entry *getBucketByKey(const Key &key) const;

	// walks from e and looks for key. if a match is found, its in
	// result->next, else result is the end of the list.
	inline Entry *findPredecessor(const Key &key, Entry *e) const;
	// just for stats, usable by derived classes
	inline void incAccesses() const;

private:
	unsigned int _tableSize;
	unsigned int _blockSize;
	std::deque<Entry *> _blocks;
	Entry **_buckets;
	Entry *_available;

	void allocBlock();

#ifdef HASHMAP_DEBUG
	struct Debug {
		unsigned int allocs;
		unsigned int accesses;
	};

	mutable Debug _debug;
public:
	void dumpStats(const char* description);
#endif
};

template<typename Key, typename Value, typename HashKey>
HashMap<Key, Value, HashKey>::HashMap() :
		_tableSize(0),
		_blockSize(0),
		_blocks(),
		_buckets(NULL),
		_available(NULL)
{
#ifdef HASHMAP_DEBUG
	memset(&_debug, 0, sizeof(_debug));
#endif
}

template<typename Key, typename Value, typename HashKey>
HashMap<Key, Value, HashKey>::~HashMap()
{
	clear();
}

template<typename Key, typename Value, typename HashKey>
void HashMap<Key, Value, HashKey>::init(unsigned int tableSize, unsigned int blockSize)
{
	clear();

	if (tableSize < 1)
		return;

	_tableSize = tableSize;
	if (_tableSize < 16)
		_tableSize = 16;

	// even sucks for distribution
	_tableSize |= 1u;

	_blockSize = blockSize;
	if (_blockSize < 4)
		_blockSize = 4;

	_buckets = new Entry *[_tableSize];

	memset(_buckets, 0, sizeof(Entry *) * _tableSize);

#ifdef HASHMAP_DEBUG
	memset(&_debug, 0, sizeof(_debug));
#endif
}

template<typename Key, typename Value, typename Hash>
bool HashMap<Key, Value, Hash>::set(const Key &key, const Value &value)
{
	if (!isInitialized())
		error("HashMap", "Not initialized\n");

	unsigned int p = getMapPosByKey(key);
	Entry *e;

	// set as root if empty
	if (!_buckets[p]) {
		e = popAvailable();
		Hash::copy(e->key, key);
		e->value = value;

		_buckets[p] = e;

		return false;
	}

	e = _buckets[p];

	// check root
	if (Hash::equals(e->key, key)) {
		e->value = value;
		return true;
	}

	// walk list
	e = findPredecessor(key, e);
	Entry *hit = e->next;

	// replace match
	if (hit) {
		hit->value = value;
		return true;
	}

	// append new
	hit = popAvailable();
	Hash::copy(hit->key, key);
	hit->value = value;
	e->next = hit;

	return false;
}

template<typename Key, typename Value, typename Hash>
const Value *HashMap<Key, Value, Hash>::get(const Key &key) const
{
	if (!isInitialized())
		return NULL;

	incAccesses();

	for (Entry *e = getBucketByKey(key); e; e = e->next)
		if (Hash::equals(e->key, key))
			return &e->value;

	return NULL;
}

template<typename Key, typename Value, typename Hash>
bool HashMap<Key, Value, Hash>::has(const Key &key) const
{
	return get(key) != NULL;
}

template<typename Key, typename Value, typename Hash>
inline bool HashMap<Key, Value, Hash>::remove(const Key &key)
{
	if (!isInitialized())
		return false;

	unsigned int p = getMapPosByKey(key);
	Entry *e = _buckets[p];

	if (!e)
		return false;

	// check root
	if (Hash::equals(e->key, key)) {
		_buckets[p] = e->next;
		pushAvailable(e);

		return true;
	}

	// walk the list
	e = findPredecessor(key, e);
	Entry *hit = e->next;

	if (!hit)
		return false;

	e->next = hit->next;
	pushAvailable(hit);

	return true;
}

template<typename Key, typename Value, typename Hash>
void HashMap<Key, Value, Hash>::clear()
{
	if (!isInitialized())
		return;

#ifdef HASHMAP_DEBUG
	dumpStats();
#endif

	_available = NULL;

	delete[] _buckets;
	_buckets = NULL;

	while (!_blocks.empty()) {
		delete[] _blocks.front();
		_blocks.pop_front();
	}
}

template<typename Key, typename Value, typename Hash>
bool inline HashMap<Key, Value, Hash>::isInitialized() const
{
	return _buckets != NULL;
}

template<typename Key, typename Value, typename Hash>
inline typename HashMap<Key, Value, Hash>::Entry *HashMap<Key, Value, Hash>::popAvailable()
{
	if (!_available)
		allocBlock();

	Entry *e = _available;
	_available = e->next;

	e->next = NULL;

	return e;
}

template<typename Key, typename Value, typename Hash>
inline void HashMap<Key, Value, Hash>::pushAvailable(Entry *e) {
	e->next = _available;
	_available = e;
}

template<typename Key, typename Value, typename Hash>
inline unsigned int HashMap<Key, Value, Hash>::getMapPosByHash(unsigned int hash) const
{
	return hash % _tableSize;
}

template<typename Key, typename Value, typename Hash>
inline unsigned int HashMap<Key, Value, Hash>::getMapPosByKey(const Key &key) const
{
	return getMapPosByHash(Hash::hash(key));
}

template<typename Key, typename Value, typename Hash>
inline typename HashMap<Key, Value, Hash>::Entry *HashMap<Key, Value, Hash>::getBucketByHash(unsigned int hash) const
{
	return _buckets[getMapPosByHash(hash)];
}

template<typename Key, typename Value, typename Hash>
inline typename HashMap<Key, Value, Hash>::Entry *HashMap<Key, Value, Hash>::getBucketByKey(const Key &key) const
{
	return _buckets[getMapPosByKey(key)];
}

template<typename Key, typename Value, typename Hash>
inline typename HashMap<Key, Value, Hash>::Entry *HashMap<Key, Value, Hash>::findPredecessor(const Key &key, Entry *e) const
{
	for (; e->next; e = e->next)
		if (Hash::equals(e->next->key, key))
			break;

	return e;
}

template<typename Key, typename Value, typename Hash>
inline void HashMap<Key, Value, Hash>::incAccesses() const
{
#ifdef HASHMAP_DEBUG
	_debug.accesses++;
#endif
}

template<typename Key, typename Value, typename Hash>
void HashMap<Key, Value, Hash>::allocBlock()
{
	Entry *block = new Entry[_blockSize];

	_blocks.push_back(block);

	for (unsigned int i = 0; i < _blockSize; ++i)
		pushAvailable(block++);

#ifdef HASHMAP_DEBUG
	_debug.allocs++;
#endif
}

#ifdef HASHMAP_DEBUG
template<typename Key, typename Value, typename Hash>
void HashMap<Key, Value, Hash>::dumpStats(const char* description)
{
	if (!isInitialized())
		return;

	unsigned int entries = 0;
	unsigned int collisions = 0;
	unsigned int empty = 0;
	unsigned int eq1 = 0;
	unsigned int eq2 = 0;
	unsigned int gt2 = 0;
	unsigned int gt4 = 0;
	unsigned int gt8 = 0;
	unsigned int largest = 0;

	Entry **b = _buckets;
	for (unsigned int i = 0; i < _tableSize; ++i, ++b) {
		if (!(*b)) {
			empty++;
			continue;
		}

		unsigned int c = 0;
		for (Entry *e = *b; e; c++, entries++, e = e->next)
			;

		if (c > 8)
			gt8++;
		else if (c > 4)
			gt4++;
		else if (c > 2)
			gt2++;
		else if (c > 1)
			eq2++;
		else
			eq1++;

		if (c > 1)
			collisions += c;

		if (c > largest)
			largest++;
	}

	unsigned int bytes = sizeof(*this) +
		_tableSize * sizeof(Entry *) +
		_blocks.size() * sizeof(Entry) * _blockSize;

	Log(DEBUG, "HashMap", "stats for %s:\n"
			"size\t\t%u\n"
			"allocs\t\t%u\n"
			"accesses\t%u\n"
			"entries\t\t%u\n"
			"collisions\t%u\n"
			"empty buckets\t%u\n"
			"=1 buckets\t%u\n"
			"=2 buckets\t%u\n"
			">2 buckets\t%u\n"
			">4 buckets\t%u\n"
			">8 buckets\t%u\n"
			"largest bucket\t%u\n"
			"memsize\t\t%ukb\n",
			description,
			_tableSize, _debug.allocs, _debug.accesses,
			entries, collisions, empty, eq1, eq2, gt2,
			gt4, gt8, largest, bytes / 1024);

	memset(&_debug, 0, sizeof(_debug));
}
#endif

#endif
