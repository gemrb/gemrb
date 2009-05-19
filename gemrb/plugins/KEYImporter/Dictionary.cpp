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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * $Id$
 *
 */

#include "../../includes/win32def.h"
#include "../../includes/globals.h"
#include "Dictionary.h"

/////////////////////////////////////////////////////////////////////////////
// inlines

inline unsigned int Dictionary::MyHashKey(const char* key, unsigned int type) const
{
	unsigned int nHash = type;
	for (int i = 0; i < KEYSIZE && key[i]; i++) {
		nHash = ( nHash << 5 ) + nHash + tolower( key[i] );
	}
	return nHash;
}
inline int Dictionary::GetCount() const
{
	return m_nCount;
}
inline bool Dictionary::IsEmpty() const
{
	return m_nCount == 0;
}
/////////////////////////////////////////////////////////////////////////////
// out of lines
Dictionary::Dictionary(int nBlockSize, int nHashTableSize)
{
	MYASSERT( nBlockSize > 0 );
	MYASSERT( nHashTableSize > 16 );

	m_pHashTable = NULL;
	m_nHashTableSize = nHashTableSize;  // default size
	m_nCount = 0;
	m_pFreeList = NULL;
	m_pBlocks = NULL;
	m_nBlockSize = nBlockSize;
}

void Dictionary::InitHashTable(unsigned int nHashSize, bool bAllocNow)
	//Used to force allocation of a hash table or to override the default
	//hash table size of (which is fairly small)
{
	MYASSERT( m_nCount == 0 );
	MYASSERT( nHashSize > 16 );

	if (m_pHashTable != NULL) {
		// free hash table
		free( m_pHashTable);
		m_pHashTable = NULL;
	}

	if (bAllocNow) {
		m_pHashTable = (Dictionary::MyAssoc **) malloc( sizeof( MyAssoc * ) * nHashSize );
		memset( m_pHashTable, 0, sizeof( MyAssoc * ) * nHashSize );
	}
	m_nHashTableSize = nHashSize;
}

void Dictionary::RemoveAll()
{
	//removed the part about freeing values/keys
	//because the complete value/key pair is stored in the
	//node which is freed in the memblocks

	// free hash table
	free( m_pHashTable );
	m_pHashTable = NULL;

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

Dictionary::~Dictionary()
{
	RemoveAll();
}

Dictionary::MyAssoc* Dictionary::NewAssoc()
{
	if (m_pFreeList == NULL) {
		// add another block
		Dictionary::MemBlock* newBlock = ( Dictionary::MemBlock* ) malloc(m_nBlockSize * sizeof( Dictionary::MyAssoc ) + sizeof( Dictionary::MemBlock ));
		MYASSERT( newBlock != NULL );  // we must have something

		newBlock->pNext = m_pBlocks;
		m_pBlocks = newBlock;

		// chain them into free list
		Dictionary::MyAssoc* pAssoc = ( Dictionary::MyAssoc* )
			( newBlock + 1 );		
		for (int i = 0; i < m_nBlockSize; i++) {
			pAssoc->pNext = m_pFreeList;
			m_pFreeList = pAssoc++;
		}
	}
	
	Dictionary::MyAssoc* pAssoc = m_pFreeList;
	m_pFreeList = m_pFreeList->pNext;
	m_nCount++;
	MYASSERT( m_nCount > 0 );  // make sure we don't overflow
#ifdef _DEBUG
	pAssoc->key[0] = 0;
	pAssoc->value = 0xcccccccc;
#endif
	return pAssoc;
}

void Dictionary::FreeAssoc(Dictionary::MyAssoc* pAssoc)
{
	pAssoc->pNext = m_pFreeList;
	m_pFreeList = pAssoc;
	m_nCount--;
	MYASSERT( m_nCount >= 0 );  // make sure we don't underflow

	// if no more elements, cleanup completely
	if (m_nCount == 0) {
		RemoveAll();
	}
}

Dictionary::MyAssoc* Dictionary::GetAssocAt(const ieResRef key,
	unsigned int type, unsigned int& nHash) const
	// find association (or return NULL)
{
	nHash = MyHashKey( key, type ) % m_nHashTableSize;

	if (m_pHashTable == NULL) {
		return NULL;
	}

	// see if it exists
	MyAssoc* pAssoc;
	for (pAssoc = m_pHashTable[nHash];
		pAssoc != NULL;
		pAssoc = pAssoc->pNext) {
		if (type == pAssoc->type) {
			if (!strnicmp( pAssoc->key, key, KEYSIZE )) {
				return pAssoc;
			}
		}
	}
	return NULL;
}

bool Dictionary::Lookup(const ieResRef key, unsigned int type,
	unsigned int& rValue) const
{
	unsigned int nHash;

	MyAssoc* pAssoc = GetAssocAt( key, type, nHash );
	if (pAssoc == NULL) {
		return false;
	}  // not in map

	rValue = pAssoc->value;
	return true;
}

void Dictionary::RemoveAt(const ieResRef key, unsigned int type)
{
	unsigned int nHash;
	MyAssoc* pAssoc = GetAssocAt( key, type, nHash );

	if (pAssoc != NULL) {
		FreeAssoc(pAssoc);
	}
}

void Dictionary::SetAt(const ieResRef key, unsigned int type, unsigned int value)
{
	int i;
	unsigned int nHash;
	MyAssoc* pAssoc=GetAssocAt( key, type, nHash );

	if (pAssoc == NULL) {
		if (m_pHashTable == NULL)
			InitHashTable( m_nHashTableSize );

		// it doesn't exist, add a new Association
		pAssoc = NewAssoc();
		// put into hash table
		pAssoc->pNext = m_pHashTable[nHash];
		m_pHashTable[nHash] = pAssoc;
	}
	for(i=0;i<KEYSIZE && key[i];i++) {
		pAssoc->key[i]=tolower(key[i]);
	}
	for(;i<KEYSIZE;i++) {
		pAssoc->key[i]=0;
	}
	pAssoc->type = type;
	pAssoc->value = value;
}
