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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

#include "Cache.h"

#include <cassert>
#include <ctype.h>

namespace GemRB {

// private inlines
inline unsigned int Cache::MyHashKey(const char* key) const
{
	int nHash = tolower(key[0]);
	for (int i=1;(i<KEYSIZE) && key[i];i++) {
		nHash = (nHash << 5) ^ tolower(key[i]);
	}
	return nHash % m_nHashTableSize;
}

Cache::Cache(int nBlockSize, int nHashTableSize)
{
	assert( nBlockSize > 0 );
	assert( nHashTableSize > 16 );

	m_pHashTable = NULL;
	m_nHashTableSize = nHashTableSize; // default size
	m_nCount = 0;
	m_pFreeList = NULL;
	m_pBlocks = NULL;
	m_nBlockSize = nBlockSize;
}

void Cache::InitHashTable(unsigned int nHashSize, bool bAllocNow)
	//Used to force allocation of a hash table or to override the default
	//hash table size of (which is fairly small)
{
	assert( m_nCount == 0 );
	assert( nHashSize > 16 );

	if (m_pHashTable != NULL) {
		// free hash table
		free( m_pHashTable);
		m_pHashTable = NULL;
	}

	if (bAllocNow) {
		m_pHashTable = (Cache::MyAssoc **) malloc( sizeof( Cache::MyAssoc * ) * nHashSize );
		memset( m_pHashTable, 0, sizeof( Cache::MyAssoc * ) * nHashSize );
	}
	m_nHashTableSize = nHashSize;
}

void Cache::RemoveAll(ReleaseFun fun)
{
	if (m_pHashTable) {
		for (unsigned int nHash = 0; nHash < m_nHashTableSize; nHash++)
		{
			MyAssoc* pAssoc;
			for (pAssoc = m_pHashTable[nHash]; pAssoc != NULL;
				pAssoc = pAssoc->pNext)
			{
				if (fun)
					fun(pAssoc->data);
				pAssoc->MyAssoc::~MyAssoc();
			}
		}
		// free hash table
		free( m_pHashTable );
		m_pHashTable = NULL;
	}

	m_nCount = 0;
	m_pFreeList = NULL;

	// free memory blocks
	MemBlock* p = m_pBlocks;
	while (p != NULL) {
		MemBlock* pNext = p->pNext;
		free( p );
		p = pNext;
	}

	m_pBlocks = NULL;
}

Cache::~Cache()
{
	RemoveAll(NULL);
}
 
Cache::MyAssoc* Cache::NewAssoc()
{
	if (m_pFreeList == NULL) {
		// add another block
		Cache::MemBlock* newBlock = ( Cache::MemBlock* ) malloc(m_nBlockSize * sizeof( Cache::MyAssoc ) + sizeof( Cache::MemBlock ));
		assert( newBlock != NULL ); // we must have something

		newBlock->pNext = m_pBlocks;
		m_pBlocks = newBlock;

		// chain them into free list
		Cache::MyAssoc* pAssoc = ( Cache::MyAssoc* )
			( newBlock + 1 );		
		for (int i = 0; i < m_nBlockSize; i++) {
			pAssoc->pNext = m_pFreeList;
			m_pFreeList = pAssoc++;
		}
	}
	
	Cache::MyAssoc* pAssoc = m_pFreeList;
	m_pFreeList = m_pFreeList->pNext;
	m_nCount++;
	assert( m_nCount > 0 ); // make sure we don't overflow
#ifdef _DEBUG
	pAssoc->key[0] = 0;
	pAssoc->data = 0;
#endif
	pAssoc->nRefCount=1;
	return pAssoc;
}

void Cache::FreeAssoc(Cache::MyAssoc* pAssoc)
{
	if(pAssoc->pNext) {
		pAssoc->pNext->pPrev=pAssoc->pPrev;
	}
	*pAssoc->pPrev = pAssoc->pNext;
	pAssoc->pNext = m_pFreeList;
	m_pFreeList = pAssoc;
	m_nCount--;
	assert( m_nCount >= 0 ); // make sure we don't underflow

	// if no more elements, cleanup completely
	if (m_nCount == 0) {
		RemoveAll(NULL);
	}
}

Cache::MyAssoc *Cache::GetNextAssoc(Cache::MyAssoc *Position) const
{
	if (m_pHashTable == NULL || m_nCount==0) {
		return NULL;
	}

	Cache::MyAssoc* pAssocRet = (Cache::MyAssoc*)Position;

	if (pAssocRet == NULL)
	{
		// find the first association
		for (unsigned int nBucket = 0; nBucket < m_nHashTableSize; nBucket++)
			if ((pAssocRet = m_pHashTable[nBucket]) != NULL)
				break;
		return pAssocRet;
	}
	Cache::MyAssoc* pAssocNext = pAssocRet->pNext;
	if (pAssocNext == NULL)
	{
		// go to next bucket
		for (unsigned int nBucket = MyHashKey(pAssocRet->key) + 1;
			nBucket < m_nHashTableSize; nBucket++)
			if ((pAssocNext = m_pHashTable[nBucket]) != NULL)
				break;
	}

	return pAssocNext;
}

Cache::MyAssoc* Cache::GetAssocAt(const ieResRef key) const
	// find association (or return NULL)
{
	if (m_pHashTable == NULL) {
		return NULL;
	}

	unsigned int nHash = MyHashKey( key );

	// see if it exists
	Cache::MyAssoc* pAssoc;
	for (pAssoc = m_pHashTable[nHash];
		pAssoc != NULL;
		pAssoc = pAssoc->pNext) {
		if (!strnicmp( pAssoc->key, key, KEYSIZE )) {
			return pAssoc;
		}
	}
	return NULL;
}

void *Cache::GetResource(const ieResRef key) const
{
	Cache::MyAssoc* pAssoc = GetAssocAt( key );
	if (pAssoc == NULL) {
		return NULL;
	} // not in map

	pAssoc->nRefCount++;
	return pAssoc->data;
}

//returns true if it was successful
bool Cache::SetAt(const ieResRef key, void *rValue)
{
	int i;

	if (m_pHashTable == NULL) {
		InitHashTable( m_nHashTableSize );
	}

	Cache::MyAssoc* pAssoc=GetAssocAt( key );
	
	if (pAssoc) {
		//already exists, but we return true if it is the same
		return (pAssoc->data==rValue); 
	}

	// it doesn't exist, add a new Association
	pAssoc = NewAssoc();
	for (i=0;i<KEYSIZE && key[i];i++) {
		pAssoc->key[i]=tolower(key[i]);
	}
	for (;i<KEYSIZE;i++) {
		pAssoc->key[i]=0;
	}
	pAssoc->data=rValue;
	// put into hash table
	unsigned int nHash = MyHashKey(pAssoc->key);
	pAssoc->pNext = m_pHashTable[nHash];
	pAssoc->pPrev = &m_pHashTable[nHash];
	if (pAssoc->pNext) {
		pAssoc->pNext->pPrev = &pAssoc->pNext;
	}
	m_pHashTable[nHash] = pAssoc;
	return true;
}

int Cache::RefCount(const ieResRef key) const
{
	Cache::MyAssoc* pAssoc=GetAssocAt( key );
	if (pAssoc) {
		return pAssoc->nRefCount;
	}
	return -1;
}

int Cache::DecRef(void *data, const ieResRef key, bool remove)
{
	Cache::MyAssoc* pAssoc;

	if (key) {
		pAssoc=GetAssocAt( key );
		if (pAssoc && (pAssoc->data==data) ) {
			if (!pAssoc->nRefCount) {
				return -1;
			}
			--pAssoc->nRefCount;
			if (remove && !pAssoc->nRefCount) {
				FreeAssoc(pAssoc);
				return 0;
			}
			return pAssoc->nRefCount;
		}
		return -1;
	}

	pAssoc=(Cache::MyAssoc *) GetNextAssoc(NULL);

	while (pAssoc) {
		if (pAssoc->data == data) {
			if (!pAssoc->nRefCount) {
				return -1;
			}
			--pAssoc->nRefCount;
			if (remove && !pAssoc->nRefCount) {
				FreeAssoc(pAssoc);
				return 0;
			}
			return pAssoc->nRefCount;
		}
		pAssoc=GetNextAssoc(pAssoc);
	}
	return -1;
}

void Cache::Cleanup()
{
	Cache::MyAssoc* pAssoc=(Cache::MyAssoc *) GetNextAssoc(NULL);

	while (pAssoc)
	{
		Cache::MyAssoc* nextAssoc = GetNextAssoc(pAssoc);
		if (pAssoc->nRefCount == 0) {
			FreeAssoc(pAssoc);
		}
		pAssoc=nextAssoc;
	}
}

}
