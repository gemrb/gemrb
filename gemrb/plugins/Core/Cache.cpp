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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/Cache.cpp,v 1.4 2005/02/09 19:54:52 avenger_teambg Exp $
 *
 */

#include "Cache.h"

// private inlines
inline bool Cache::MyCopyKey(char*& dest, ieResRef key) const
{
	unsigned int i, j;

	//use j
	for (i = 0,j = 0; key[i] && j < sizeof(ieResRef) - 1; i++)
		if (key[i] != ' ') {
			j++;
		}
	dest = new char[j + 1];
	if (!dest) {
		return false;
	}
	for (i = 0,j = 0; key[i] && j < sizeof(ieResRef) - 1; i++) {
		if (key[i] != ' ') {
			dest[j++] = toupper( key[i] );
		}
	}
	dest[j] = 0;
	return true;
}

Cache::Cache()
{
}

void Cache::RemoveAll()
{
/*
	HashMapType::iterator m=hashmap.begin();

	while(m!=hashmap.end() ) {
		delete (*m).second.data;
		m++;
	}
*/
	hashmap.clear();
}

Cache::~Cache()
{
	RemoveAll();
	MYASSERT( IsEmpty() );
}
 
void *Cache::GetResource(ieResRef key)
{
	HashMapType::iterator m;

	m=hashmap.find(key);
	if (m == hashmap.end()) {
		return NULL;
	}
	(*m).second.nRefCount++;
	return (*m).second.data;
}

//returns true if it was successful
bool Cache::SetAt(ieResRef key, void *rValue)
{
	HashMapType::iterator m;

	m=hashmap.find(key);
	if (m!=hashmap.end()) {
		return (*m).second.data==rValue;
	}
	//this creates a new element
	ValueType *ret=&hashmap[key];
	ret->data=rValue;
	ret->nRefCount=1;
	return true;
}

//returns RefCount we still have
int Cache::DecRef(ieResRef key, bool remove)
{
	HashMapType::iterator m;

	m=hashmap.find(key);
	if (m == hashmap.end()) {
		return -1;
	}

	if (remove) {
		hashmap.erase(m);
		return 0;
	}
	if ((*m).second.nRefCount) {
		return --(*m).second.nRefCount;
	}
	return -1;
}

int Cache::DecRef(void *data, bool remove)
{
	HashMapType::iterator m;

	for (m = hashmap.begin(); m != hashmap.end(); ++m) {
		if((*m).second.data == data) {
			if (remove) {
				hashmap.erase(m);
				return 0;
			}
			if ((*m).second.nRefCount) {
				return --(*m).second.nRefCount;
			}
		}
	}
	return -1;
}

void Cache::Cleanup()
{
	HashMapType::iterator m;

	for (m = hashmap.begin(); m != hashmap.end(); ++m) {
		if(!(*m).second.nRefCount) {
			hashmap.erase(m);
		}
	}
}
