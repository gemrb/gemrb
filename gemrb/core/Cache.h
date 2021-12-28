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

#include "globals.h"

namespace GemRB {

#ifndef ReleaseFun
using ReleaseFun = void (*)(void*);
#endif

class Cache
{
protected:
	// Association
	struct MyAssoc {
		MyAssoc* pNext;
		MyAssoc** pPrev;
		ResRef key;
		ieDword nRefCount;
		void* data;
	};
	struct MemBlock {
		MemBlock* pNext;
	};

public:
	// Construction
	explicit Cache(int nBlockSize = 10, int nHashTableSize = 129);

	// Attributes
	// number of elements
	inline int GetCount() const
	{
		return m_nCount;
	}
	inline bool IsEmpty() const
	{
		return m_nCount==0;
	}
	// Lookup
	void *GetResource(const ResRef& key) const;
	// Operations
	bool SetAt(const ResRef& key, void *rValue);
	// decreases refcount or drops data
	//if name is supplied it is faster, it will use rValue to validate the request
	int DecRef(const void *rValue, const ResRef& name, bool free);
	int RefCount(const ResRef& key) const;
	void RemoveAll(ReleaseFun fun);//removes all refcounts
	void Cleanup();  //removes only zero refcounts
	void InitHashTable(unsigned int hashSize, bool bAllocNow = true);

	// Implementation
protected:
	MyAssoc** m_pHashTable = nullptr;
	unsigned int m_nHashTableSize;
	int m_nCount = 0;
	MyAssoc* m_pFreeList = nullptr;
	MemBlock* m_pBlocks = nullptr;
	int m_nBlockSize;

	Cache::MyAssoc* NewAssoc();
	void FreeAssoc(Cache::MyAssoc*);
	Cache::MyAssoc* GetAssocAt(const ResRef&) const;
	Cache::MyAssoc *GetNextAssoc(Cache::MyAssoc * rNextPosition) const;

public:
	~Cache();
};

}

#endif //CACHE_H
