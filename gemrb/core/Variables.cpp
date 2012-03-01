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

#include "Variables.h"

#include "Interface.h" // for LoadInitialValues
#include "System/FileStream.h" // for LoadInitialValues

namespace GemRB {

/////////////////////////////////////////////////////////////////////////////
// private inlines 
inline bool Variables::MyCopyKey(char*& dest, const char* key) const
{
	int i, j;

	//use j
	for (i = 0,j = 0; key[i] && j < MAX_VARIABLE_LENGTH - 1; i++)
		if (key[i] != ' ') {
			j++;
		}
	dest = (char *) malloc(j + 1);
	if (!dest) {
		return false;
	}
	for (i = 0,j = 0; key[i] && j < MAX_VARIABLE_LENGTH - 1; i++) {
		if (key[i] != ' ') {
			dest[j++] = (char) tolower( key[i] );
		}
	}
	dest[j] = 0;
	return true;
}

inline unsigned int Variables::MyCompareKey(const char* key, const char *str) const
{
	int i,j;

	for (i = 0, j = 0; str[j] && key[i] && i < MAX_VARIABLE_LENGTH - 1 && j < MAX_VARIABLE_LENGTH - 1;) {
		char c1 = tolower(key[i]);
		if (c1 == ' ') { i++; continue; }
		char c2 = tolower(str[j]);
		if (c2 ==' ')  { j++; continue; }
		if (c1!=c2) return 1;
		i++;
		j++;
	}
	if (str[j] || key[i]) return 1;
	return 0;
}

inline unsigned int Variables::MyHashKey(const char* key) const
{
	unsigned int nHash = 0;
	for (int i = 0; key[i] && i < MAX_VARIABLE_LENGTH; i++) {
		//the original engine ignores spaces in variable names
		if (key[i] != ' ')
			nHash = ( nHash << 5 ) + nHash + tolower( key[i] );
	}
	return nHash;
}
/////////////////////////////////////////////////////////////////////////////
// functions
Variables::iterator Variables::GetNextAssoc(iterator rNextPosition, const char*& rKey,
	ieDword& rValue) const
{
	assert( m_pHashTable != NULL ); // never call on empty map

	Variables::MyAssoc* pAssocRet = ( Variables::MyAssoc* ) rNextPosition;

	if (pAssocRet == NULL) {
		// find the first association
		for (unsigned int nBucket = 0; nBucket < m_nHashTableSize; nBucket++)
			if (( pAssocRet = m_pHashTable[nBucket] ) != NULL)
				break;
		assert( pAssocRet != NULL ); // must find something
	}
	Variables::MyAssoc* pAssocNext;
	if (( pAssocNext = pAssocRet->pNext ) == NULL) {
		// go to next bucket
		for (unsigned int nBucket = pAssocRet->nHashValue + 1;
			nBucket < m_nHashTableSize;
			nBucket++)
			if (( pAssocNext = m_pHashTable[nBucket] ) != NULL)
				break;
	}

	// fill in return data
	rKey = pAssocRet->key;
	rValue = pAssocRet->Value.nValue;
	return ( iterator ) pAssocNext;
}

Variables::Variables(int nBlockSize, int nHashTableSize)
{
	assert( nBlockSize > 0 );
	assert( nHashTableSize > 16 );

	m_pHashTable = NULL;
	m_nHashTableSize = nHashTableSize; // default size
	m_nCount = 0;
	m_lParseKey = false;
	m_pFreeList = NULL;
	m_pBlocks = NULL;
	m_nBlockSize = nBlockSize;
	m_type = GEM_VARIABLES_INT;
}

void Variables::InitHashTable(unsigned int nHashSize, bool bAllocNow)
	//
	// Used to force allocation of a hash table or to override the default
	// hash table size of (which is fairly small)
{
	assert( m_nCount == 0 );
	assert( nHashSize > 16 );

	if (m_pHashTable != NULL) {
		// free hash table
		free(m_pHashTable);
		m_pHashTable = NULL;
	}

	if (bAllocNow) {
		m_pHashTable =(Variables::MyAssoc **) malloc(sizeof( Variables::MyAssoc *) * nHashSize);
		memset( m_pHashTable, 0, sizeof( Variables::MyAssoc * ) * nHashSize );
	}
	m_nHashTableSize = nHashSize;
}

void Variables::RemoveAll(ReleaseFun fun)
{
	if (m_pHashTable != NULL) {
		// destroy elements (values and keys)
		for (unsigned int nHash = 0; nHash < m_nHashTableSize; nHash++) {
			Variables::MyAssoc* pAssoc;
			for (pAssoc = m_pHashTable[nHash];
				pAssoc != NULL;
				pAssoc = pAssoc->pNext) {
				if (fun) {
					fun((void *) pAssoc->Value.sValue);
				}
				else if (m_type == GEM_VARIABLES_STRING) {
					if (pAssoc->Value.sValue) {
						free( pAssoc->Value.sValue );
						pAssoc->Value.sValue = NULL;
					}
				}
				if (pAssoc->key) {
					free(pAssoc->key);
					pAssoc->key = NULL;
				}
			}
		}
	}

	// free hash table
	free(m_pHashTable);
	m_pHashTable = NULL;

	m_nCount = 0;
	m_pFreeList = NULL;
	MemBlock* p = m_pBlocks;
	while (p != NULL) {
		MemBlock* pNext = p->pNext;
		free(p);
		p = pNext;
	}
	m_pBlocks = NULL;
}

Variables::~Variables()
{
	RemoveAll(NULL);
}

Variables::MyAssoc* Variables::NewAssoc(const char* key)
{
	if (m_pFreeList == NULL) {
		// add another block
		Variables::MemBlock* newBlock = ( Variables::MemBlock* ) malloc( m_nBlockSize*sizeof( Variables::MyAssoc ) + sizeof( Variables::MemBlock ));
		assert( newBlock != NULL ); // we must have something
		newBlock->pNext = m_pBlocks;
		m_pBlocks = newBlock;

		// chain them into free list
		Variables::MyAssoc* pAssoc = ( Variables::MyAssoc* ) ( newBlock + 1 );
		for (int i = 0; i < m_nBlockSize; i++) {
			pAssoc->pNext = m_pFreeList;
			m_pFreeList = pAssoc++;
		}
	}
	
	Variables::MyAssoc* pAssoc = m_pFreeList;
	m_pFreeList = m_pFreeList->pNext;
	m_nCount++;
	assert( m_nCount > 0 ); // make sure we don't overflow
	if (m_lParseKey) {
		MyCopyKey( pAssoc->key, key );
	} else {
		int len;
		len = strnlen( key, MAX_VARIABLE_LENGTH - 1 );
		pAssoc->key = (char *) malloc(len + 1);
		if (pAssoc->key) {
			memcpy( pAssoc->key, key, len );
			pAssoc->key[len] = 0;
		}
	}
#ifdef _DEBUG
	pAssoc->Value.nValue = 0xcccccccc; //invalid value
	pAssoc->nHashValue = 0xcccccccc; //invalid value
#endif
	return pAssoc;
}

void Variables::FreeAssoc(Variables::MyAssoc* pAssoc)
{
	if (pAssoc->key) {
		free(pAssoc->key);
		pAssoc->key = NULL;
	}
	pAssoc->pNext = m_pFreeList;
	m_pFreeList = pAssoc;
	m_nCount--;
	assert( m_nCount >= 0 ); // make sure we don't underflow

	// if no more elements, cleanup completely
	if (m_nCount == 0) {
		RemoveAll(NULL);
	}
}

Variables::MyAssoc* Variables::GetAssocAt(const char* key, unsigned int& nHash) const
	// find association (or return NULL)
{
	nHash = MyHashKey( key ) % m_nHashTableSize;

	if (m_pHashTable == NULL) {
		return NULL;
	}

	// see if it exists
	Variables::MyAssoc* pAssoc;
	for (pAssoc = m_pHashTable[nHash];
		pAssoc != NULL;
		pAssoc = pAssoc->pNext) {
		if (m_lParseKey) {
			if (!MyCompareKey( pAssoc->key, key) ) {
				return pAssoc;
			}
		} else {
			if (!strnicmp( pAssoc->key, key, MAX_VARIABLE_LENGTH )) {
				return pAssoc;
			}
		}
	}

	return NULL;
}

int Variables::GetValueLength(const char* key) const
{
	unsigned int nHash;
	Variables::MyAssoc* pAssoc = GetAssocAt( key, nHash );
	if (pAssoc == NULL) {
		return 0; // not in map
	}

	return ( int ) strlen( pAssoc->Value.sValue );
}

bool Variables::Lookup(const char* key, char* dest, int MaxLength) const
{
	unsigned int nHash;
	assert( m_type == GEM_VARIABLES_STRING );
	Variables::MyAssoc* pAssoc = GetAssocAt( key, nHash );
	if (pAssoc == NULL) {
		dest[0] = 0;
		return false; // not in map
	}

	strncpy( dest, pAssoc->Value.sValue, MaxLength );
	return true;
}

bool Variables::Lookup(const char* key, char *&dest) const
{
	unsigned int nHash;
	assert(m_type==GEM_VARIABLES_STRING);
	Variables::MyAssoc* pAssoc = GetAssocAt( key, nHash );
	if (pAssoc == NULL) {
		return false;
	} // not in map

	dest = pAssoc->Value.sValue;
	return true;
}

bool Variables::Lookup(const char* key, void *&dest) const
{
	unsigned int nHash;
	assert(m_type==GEM_VARIABLES_POINTER);
	Variables::MyAssoc* pAssoc = GetAssocAt( key, nHash );
	if (pAssoc == NULL) {
		return false;
	} // not in map

	dest = pAssoc->Value.pValue;
	return true;
}

bool Variables::Lookup(const char* key, ieDword& rValue) const
{
	unsigned int nHash;
	assert(m_type==GEM_VARIABLES_INT);
	Variables::MyAssoc* pAssoc = GetAssocAt( key, nHash );
	if (pAssoc == NULL) {
		return false;
	} // not in map

	rValue = pAssoc->Value.nValue;
	return true;
}

void Variables::SetAtCopy(const char* key, const char* value)
{
	size_t len = strlen(value)+1;
	char *str=(char *) malloc(len);
	memcpy(str,value,len);
	SetAt(key, str);
}

void Variables::SetAtCopy(const char* key, int newValue)
{
	char tmpstr[10]; // should be enough
	sprintf(tmpstr, "%d", newValue);
	SetAtCopy(key, tmpstr);
}

void Variables::SetAt(const char* key, char* value)
{
	unsigned int nHash;
	Variables::MyAssoc* pAssoc;

	assert(strlen(key)<256);

#ifdef _DEBUG
	// for Avenger, debugging memory issues
	assert((unsigned char)key[0]!=0xcd);
#endif

	assert( m_type == GEM_VARIABLES_STRING );
	if (( pAssoc = GetAssocAt( key, nHash ) ) == NULL) {
		if (m_pHashTable == NULL)
			InitHashTable( m_nHashTableSize );

		// it doesn't exist, add a new Association
		pAssoc = NewAssoc( key );
		// put into hash table
		pAssoc->pNext = m_pHashTable[nHash];
		m_pHashTable[nHash] = pAssoc;
	} else {
		if (pAssoc->Value.sValue) {
			free( pAssoc->Value.sValue );
			pAssoc->Value.sValue = 0;
		}
	}

	//set value only if we have a key
	if (pAssoc->key) {
		pAssoc->Value.sValue = value;
		pAssoc->nHashValue = nHash;
	}
}

void Variables::SetAt(const char* key, void* value)
{
	unsigned int nHash;
	Variables::MyAssoc* pAssoc;

	assert( m_type == GEM_VARIABLES_POINTER );
	if (( pAssoc = GetAssocAt( key, nHash ) ) == NULL) {
		if (m_pHashTable == NULL)
			InitHashTable( m_nHashTableSize );

		// it doesn't exist, add a new Association
		pAssoc = NewAssoc( key );
		// put into hash table
		pAssoc->pNext = m_pHashTable[nHash];
		m_pHashTable[nHash] = pAssoc;
	} else {
		if (pAssoc->Value.sValue) {
			free( pAssoc->Value.sValue );
			pAssoc->Value.sValue = 0;
		}
	}

	//set value only if we have a key
	if (pAssoc->key) {
		pAssoc->Value.pValue = value;
		pAssoc->nHashValue = nHash;
	}

}


void Variables::SetAt(const char* key, ieDword value, bool nocreate)
{
	unsigned int nHash;
	Variables::MyAssoc* pAssoc;

	assert( m_type == GEM_VARIABLES_INT );
	if (( pAssoc = GetAssocAt( key, nHash ) ) == NULL) {
		if (nocreate) {
			Log(WARNING, "Variables", "Cannot create new variable: %s", key);
			return;
		}

		if (m_pHashTable == NULL)
			InitHashTable( m_nHashTableSize );

		// it doesn't exist, add a new Association
		pAssoc = NewAssoc( key );
		// put into hash table
		pAssoc->pNext = m_pHashTable[nHash];
		m_pHashTable[nHash] = pAssoc;
	}
	//set value only if we have a key
	if (pAssoc->key) {
		pAssoc->Value.nValue = value;
		pAssoc->nHashValue = nHash;
	}
}

void Variables::Remove(const char* key)
{
	unsigned int nHash;
	Variables::MyAssoc* pAssoc;

	pAssoc = GetAssocAt( key, nHash );
	if (!pAssoc) return; // not in there

	if (pAssoc == m_pHashTable[nHash]) {
		// head
		m_pHashTable[nHash] = pAssoc->pNext;
	} else {
		Variables::MyAssoc* prev = m_pHashTable[nHash];
		// Room for optimization: make each bucket a doubly linked
		// list to make removes from a bucket O(1).
		// (This will have limited use in gemrb's case, because we
		// use relatively large tables and small buckets.)
		while (prev->pNext != pAssoc) {
			prev = prev->pNext;
			assert( prev != NULL );
		}
		prev->pNext = pAssoc->pNext;		
	}
	pAssoc->pNext = 0;
	FreeAssoc(pAssoc);
}

void Variables::LoadInitialValues(const char* name)
{
	char nPath[_MAX_PATH];
	// we only support PST's var.var for now
	PathJoin( nPath, core->GamePath, "var.var", NULL );
	FileStream fs;
	if (!fs.Open(nPath)) {
		return;
	}

	char buffer[41];
	ieDword value;
	buffer[40] = 0;
	ieVariable varname;
	
	// first value is useless
	if (!fs.Read(buffer, 40)) return;
	if (fs.ReadDword(&value) != 4) return;
	
	while (fs.Remains()) {
		// read data
		if (!fs.Read(buffer, 40)) return;
		if (fs.ReadDword(&value) != 4) return;
		// is it the type we want? if not, skip
		if (strnicmp(buffer, name, 6)) continue;
		// copy variable (types got 2 extra spaces, and the name is padded too)
		// (true = uppercase, needed for original engine save compat, see 315b8f2e)
		strnspccpy(varname,buffer+8,32, true);
		SetAt(varname, value);
	}  
}

}
