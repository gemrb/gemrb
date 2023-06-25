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
#include "Logging/Logging.h"
#include "Streams/FileStream.h" // for LoadInitialValues
#include "System/VFS.h"

namespace GemRB {

/////////////////////////////////////////////////////////////////////////////
// private inlines 
inline bool Variables::MyCopyKey(char*& dest, const key_t& key) const
{
	size_t j = std::count(key.begin(), key.end(), ' ');
	dest = (char *) malloc(key.length() - j + 1);

	j = 0;
	for (const auto& chr : key) {
		if (chr == ' ')
			continue;
		dest[j++] = tolower(chr);
	}
	dest[j] = 0;
	return true;
}

inline bool Variables::MyCompareKey(const key_t& key, key_t str) const
{
	// we know 'key' cannot contain spaces (created via MyCopyKey)
	// therefore str.length() cannot be < key.length()
	if (str.length() < key.length()) {
		return false;
	}

	size_t s = 0;
	size_t end = str.length();
	for (size_t k = 0; s < end; ++s) {
		if (str[s] == ' ')
			continue;
		if (tolower(key[k++]) != tolower(str[s]))
			return false;
	}

	return s == end;
}

inline unsigned int Variables::MyHashKey(const key_t& key) const
{
	assert(key.c_str());

	unsigned int nHash = 0;
	for (const auto& chr : key) {
		//the original engine ignores spaces in variable names
		if (chr == ' ')
			continue;
		nHash = (nHash << 5) + nHash + tolower(chr);
	}
	return nHash;
}
/////////////////////////////////////////////////////////////////////////////
// functions
Variables::iterator Variables::GetNextAssoc(iterator rNextPosition, key_t& rKey,
	ieDword& rValue) const
{
	assert( m_pHashTable != NULL ); // never call on empty map

	Variables::MyAssoc *pAssocRet = rNextPosition;

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
		for (size_t nBucket = pAssocRet->nHashValue + 1;
			nBucket < m_nHashTableSize;
			nBucket++)
			if (( pAssocNext = m_pHashTable[nBucket] ) != NULL)
				break;
	}

	// fill in return data
	rKey = key_t(pAssocRet->key);
	rValue = pAssocRet->Value.nValue;
	return pAssocNext;
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
			for (auto pAssoc = m_pHashTable[nHash]; pAssoc != nullptr; pAssoc = pAssoc->pNext) {
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

Variables::MyAssoc* Variables::NewAssoc(const key_t& key)
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
		size_t len = key.length();
		pAssoc->key = (char *) malloc(len + 1);
		if (pAssoc->key) {
			memcpy(pAssoc->key, key.begin(), len);
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

Variables::MyAssoc* Variables::GetAssocAt(const key_t& key, unsigned int& nHash) const
	// find association (or return NULL)
{
	if (key.empty() || key.c_str() == nullptr) {
		nHash = 0;
		return nullptr;
	}

	nHash = MyHashKey( key ) % m_nHashTableSize;

	if (m_pHashTable == NULL) {
		return NULL;
	}

	// see if it exists
	for (auto pAssoc = m_pHashTable[nHash]; pAssoc != nullptr; pAssoc = pAssoc->pNext) {
		if (m_lParseKey) {
			if (MyCompareKey(key_t(pAssoc->key), key)) {
				return pAssoc;
			}
		} else {
			if (!strnicmp(pAssoc->key, key.c_str(), key.length())) {
				return pAssoc;
			}
		}
	}

	return NULL;
}

bool Variables::Lookup(const key_t& key, std::string& dest) const
{
	unsigned int nHash;
	assert(m_type==GEM_VARIABLES_STRING);
	const Variables::MyAssoc* pAssoc = GetAssocAt(key, nHash);
	if (pAssoc == NULL) {
		return false;
	} // not in map

	dest = pAssoc->Value.sValue;
	return true;
}

bool Variables::Lookup(const key_t& key, String& dest) const
{
	std::string mbstr;
	bool ret = Lookup(key, mbstr);
	if (ret) {
		String* tmp = StringFromCString(mbstr.c_str());
		assert(tmp);
		std::swap(dest, *tmp);
		delete tmp;
	}
	return ret;
}

bool Variables::Lookup(const key_t& key, void *&dest) const
{
	unsigned int nHash;
	assert(m_type==GEM_VARIABLES_POINTER);
	const Variables::MyAssoc* pAssoc = GetAssocAt(key, nHash);
	if (pAssoc == NULL) {
		return false;
	} // not in map

	dest = pAssoc->Value.pValue;
	return true;
}

bool Variables::Lookup(const key_t& key, ieDword& rValue) const
{
	unsigned int nHash;
	assert(m_type==GEM_VARIABLES_INT);
	const Variables::MyAssoc* pAssoc = GetAssocAt(key, nHash);
	if (pAssoc == NULL) {
		return false;
	} // not in map

	rValue = pAssoc->Value.nValue;
	return true;
}

bool Variables::HasKey(const key_t& key) const
{
	unsigned int nHash;
	return GetAssocAt(key, nHash) != nullptr;
}

void Variables::SetAtCString(const key_t& key, const char* str)
{
	unsigned int nHash;
	Variables::MyAssoc* pAssoc;

	assert(key.length() < 256);

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
		pAssoc->Value.sValue = strdup(str);
		pAssoc->nHashValue = nHash;
	}
}

void Variables::SetAt(const key_t& key, void* value)
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


void Variables::SetAt(const key_t& key, ieDword value, bool nocreate)
{
	unsigned int nHash;
	Variables::MyAssoc* pAssoc;

	if (!key.c_str()) return;

	assert( m_type == GEM_VARIABLES_INT );
	if (( pAssoc = GetAssocAt( key, nHash ) ) == NULL) {
		if (nocreate) {
			Log(WARNING, "Variables", "Cannot create new variable: {}", key);
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

void Variables::Remove(const key_t& key)
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

void Variables::LoadInitialValues(const ResRef& name)
{
	char nPath[_MAX_PATH];
	// we only support PST's var.var for now
	PathJoin(nPath, core->config.GamePath, "var.var", nullptr);
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
	if (fs.ReadDword(value) != 4) return;
	
	while (fs.Remains()) {
		// read data
		if (!fs.Read(buffer, 40)) return;
		if (fs.ReadDword(value) != 4) return;
		// is it the type we want? if not, skip
		if (!name.BeginsWith(StringView(buffer, 6))) continue;
		// copy variable (types got 2 extra spaces, and the name is padded too)
		// (true = uppercase, needed for original engine save compat, see 315b8f2e)
		varname = MakeVariable(StringView(buffer + 8, 32));
		StringToUpper(varname);
		SetAt(varname, value);
	}
}

void LoadInitialValues(const ResRef& name, std::unordered_map<ResRef, ieDword, ResRefHash>& map) {
	char nPath[_MAX_PATH];
	// we only support PST's var.var for now
	PathJoin(nPath, core->config.GamePath, "var.var", nullptr);
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
	if (fs.ReadDword(value) != 4) return;

	while (fs.Remains()) {
		// read data
		if (!fs.Read(buffer, 40)) return;
		if (fs.ReadDword(value) != 4) return;
		// is it the type we want? if not, skip
		if (!name.BeginsWith(StringView(buffer, 6))) continue;
		// copy variable (types got 2 extra spaces, and the name is padded too)
		// (true = uppercase, needed for original engine save compat, see 315b8f2e)
		varname = MakeVariable(StringView(buffer + 8, 32));
		StringToUpper(varname);
		map[varname] = value;
	}
}

void Variables::DebugDump() const
{
	const char *poi;

	switch(m_type) {
	case GEM_VARIABLES_STRING:
		poi = "string";
		break;
	case GEM_VARIABLES_INT:
		poi = "int";
		break;
	case GEM_VARIABLES_POINTER:
		poi = "other";
		break;
	default:
		poi = "invalid";
	}
	Log (DEBUG, "Variables", "Item type: {}", poi);
	Log (DEBUG, "Variables", "Item count: {}", m_nCount);
	Log (DEBUG, "Variables", "HashTableSize: {}", m_nHashTableSize);
	for (unsigned int nHash = 0; nHash < m_nHashTableSize; nHash++) {
		for (auto pAssoc = m_pHashTable[nHash]; pAssoc != nullptr; pAssoc = pAssoc->pNext) {
			switch(m_type) {
			case GEM_VARIABLES_STRING:
				Log (DEBUG, "Variables", "{} = {}", pAssoc->key, pAssoc->Value.sValue);
				break;
			default:
				Log (DEBUG, "Variables", "{} = {}", pAssoc->key, pAssoc->Value.nValue);
				break;
			}
		}
	}
}

}
