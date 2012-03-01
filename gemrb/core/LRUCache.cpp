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

#include "LRUCache.h"

#include <cassert>
#include <cstdio>

namespace GemRB {

struct VarEntry {
	VarEntry* prev;
	VarEntry* next;
	void* data;
	char* key;
};

LRUCache::LRUCache() : v(), head(0), tail(0) {
	v.SetType(GEM_VARIABLES_POINTER);
	v.ParseKey(1);
}

LRUCache::~LRUCache()
{

}

int LRUCache::GetCount() const
{
	return v.GetCount();
}

void LRUCache::SetAt(const char* key, void* value)
{
	void* p;
	if (v.Lookup(key, p)) {
		VarEntry* e = (VarEntry*) p;
		e->data = value;
		Touch(key);
		return;
	}

	VarEntry* e = new VarEntry();
	e->prev = 0;
	e->next = head;
	e->data = value;
	e->key = new char[strlen(key)+1];
	strcpy(e->key, key);

	if (head)
		head->prev = e;
	head = e;
	if (tail == 0) tail = head;

	v.SetAt(key, (void*)e);
}

bool LRUCache::Lookup(const char* key, void*& value) const
{
	void* p;
	if (v.Lookup(key, p)) {
		VarEntry* e = (VarEntry*) p;
		value = e->data;
		return true;
	}
	return false;
}

bool LRUCache::Touch(const char* key)
{
	void* p;
	if (!v.Lookup(key, p)) return false;
	VarEntry* e = (VarEntry*) p;

	// already head?
	if (!e->prev) return true;

	removeFromList(e);

	// re-add e as head:
	e->prev = 0;
	e->next = head;
	head->prev = e;
	head = e;
	if (tail == 0) tail = head;
	return true;
}

bool LRUCache::Remove(const char* key)
{
	void* p;
	if (!v.Lookup(key, p)) return false;
	VarEntry* e = (VarEntry*) p;
	v.Remove(key);
	removeFromList(e);
	delete[] e->key;
	delete e;
	return true;
}

bool LRUCache::getLRU(unsigned int n, const char*& key, void*& value) const
{
	VarEntry* e = tail;
	for (unsigned int i = 0; i < n; ++i) {
		if (!e) return false;
		e = e->prev;
	}
	if (!e) return false;

	key = e->key;
	value = e->data;
	return true;
}

void LRUCache::removeFromList(VarEntry* e)
{
	if (e->prev) {
		assert(e != head);
		e->prev->next = e->next;
	} else {
		assert(e == head);
		head = e->next;
	}

	if (e->next) {
		assert(e != tail);
		e->next->prev = e->prev;
	} else {
		assert(e == tail);
		tail = e->prev;
	}

	e->prev = e->next = 0;
}


void testLRUCache()
{
	int i;
	LRUCache c;

	int t[100];
	for (i = 0; i < 100; ++i) t[i] = 1000+i;
	char* k[100];
	for (i = 0; i < 100; ++i) {
		k[i] = new char[5];
		sprintf(k[i], "k%03d", i);
	}

	bool r;
	void* p;
	const char* k2 = 0;

	r = c.Lookup("k050", p);
	assert(!r);

	c.SetAt("k050", &t[50]);
	r = c.Lookup("k050", p);
	assert(r);
	assert(p == &t[50]);

	for (i = 0; i < 100; ++i)
		c.SetAt(k[i], &t[i]);

	r = c.getLRU(0, k2, p);
	assert(r);
	assert(strcmp(k2, "k000") == 0);
	assert(p == &t[0]);

	c.Touch("k000");
	r = c.getLRU(0, k2, p);
	assert(r);
	assert(strcmp(k2, "k001") == 0);
	assert(p == &t[1]);

	r = c.getLRU(1, k2, p);
	assert(r);
	assert(strcmp(k2, "k002") == 0);
	assert(p == &t[2]);

	c.Remove("k001");

	r = c.getLRU(0, k2, p);
	assert(r);
	assert(strcmp(k2, "k002") == 0);
	assert(p == &t[2]);

	for (i = 0; i < 98; ++i) {
		r = c.getLRU(0, k2, p);
		assert(r);
		assert(strcmp(k2, k[2+i]) == 0);
		assert(p == &t[2+i]);
		c.Remove(k2);
	}

	assert(c.GetCount() == 1);

	r = c.getLRU(0, k2, p);
	assert(r);
	assert(strcmp(k2, "k000") == 0);
	assert(p == &t[0]);

	assert(!c.getLRU(1, k2, p));
}

}
