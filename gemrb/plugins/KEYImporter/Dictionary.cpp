#include "../../includes/win32def.h"
#include "../../includes/globals.h"
#include "Dictionary.h"

#define THIS_FILE "dictionary.cpp"

#define MYASSERT(f) \
  if(!(f))  \
  {  \
  printf("Assertion failed: %s %d",THIS_FILE, __LINE__); \
		abort(); \
  }

/////////////////////////////////////////////////////////////////////////////
// inlines
inline unsigned int Dictionary::MyHashKey(const char * key, unsigned int type) const
{
   unsigned int nHash = type;

   while (*key)
	nHash = (nHash<<5) + nHash + toupper(*key++);
   return nHash;
}
inline int Dictionary::GetCount() const
	{ return m_nCount; }
inline bool Dictionary::IsEmpty() const
	{ return m_nCount == 0; }
/////////////////////////////////////////////////////////////////////////////
// out of lines
Dictionary::Dictionary(int nBlockSize, int nHashTableSize)
{
	MYASSERT(nBlockSize > 0);
	MYASSERT(nHashTableSize > 16);

	m_pHashTable = NULL;
	m_nHashTableSize = nHashTableSize;  // default size
	m_nCount = 0;
	m_pFreeList = NULL;
	m_pBlocks = NULL;
	m_nBlockSize = nBlockSize;
}

void Dictionary::InitHashTable(unsigned int nHashSize, bool bAllocNow)
//
// Used to force allocation of a hash table or to override the default
//   hash table size of (which is fairly small)
{
	MYASSERT(m_nCount == 0);
	MYASSERT(nHashSize > 16);

	if (m_pHashTable != NULL)
	{
		// free hash table
		delete[] m_pHashTable;
		m_pHashTable = NULL;
	}

	if (bAllocNow)
	{
		m_pHashTable = new MyAssoc* [nHashSize];
		memset(m_pHashTable, 0, sizeof(MyAssoc*) * nHashSize);
	}
	m_nHashTableSize = nHashSize;
}

void Dictionary::RemoveAll()
{
	if (m_pHashTable != NULL)
	{
		// destroy elements (values and keys)
		for (unsigned int nHash = 0; nHash < m_nHashTableSize; nHash++)
		{
			MyAssoc* pAssoc;
			for (pAssoc = m_pHashTable[nHash]; pAssoc != NULL;
			  pAssoc = pAssoc->pNext)
			{
          delete [] pAssoc->key;          
			}
		}
	}

	// free hash table
	delete[] m_pHashTable;
	m_pHashTable = NULL;

	m_nCount = 0;
	m_pFreeList = NULL;
	m_pBlocks->FreeDataChain();
	m_pBlocks = NULL;
}

Dictionary::~Dictionary()
{
	RemoveAll();
	MYASSERT(m_nCount == 0);
}

Dictionary::MyAssoc *
Dictionary::NewAssoc()
{
	if (m_pFreeList == NULL)
	{
		// add another block
		Plex* newBlock = Plex::Create(m_pBlocks, m_nBlockSize, sizeof(Dictionary::MyAssoc));
		// chain them into free list
		Dictionary::MyAssoc* pAssoc = (Dictionary::MyAssoc*) newBlock->data();
		// free in reverse order to make it easier to debug
		pAssoc += m_nBlockSize - 1;
		for (int i = m_nBlockSize-1; i >= 0; i--, pAssoc--)
		{
			pAssoc->pNext = m_pFreeList;
			m_pFreeList = pAssoc;
		}
	}
	MYASSERT(m_pFreeList != NULL);  // we must have something

	Dictionary::MyAssoc* pAssoc = m_pFreeList;
	m_pFreeList = m_pFreeList->pNext;
	m_nCount++;
	MYASSERT(m_nCount > 0);  // make sure we don't overflow
  pAssoc->key=NULL;
	return pAssoc;
}

void Dictionary::FreeAssoc(Dictionary::MyAssoc* pAssoc)
{
	delete[] pAssoc->key;
	pAssoc->pNext = m_pFreeList;
	m_pFreeList = pAssoc;
	m_nCount--;
	MYASSERT(m_nCount >= 0);  // make sure we don't underflow

	// if no more elements, cleanup completely
	if (m_nCount == 0)
		RemoveAll();
}

Dictionary::MyAssoc*
Dictionary::GetAssocAt(const char *key, unsigned int type, unsigned int& nHash) const
// find association (or return NULL)
{
	nHash = MyHashKey(key,type) % m_nHashTableSize;

	if (m_pHashTable == NULL)
		return NULL;

	// see if it exists
	MyAssoc* pAssoc;
	for (pAssoc = m_pHashTable[nHash]; pAssoc != NULL; pAssoc = pAssoc->pNext)
	{
		if(!stricmp(pAssoc->key, key) )
			return pAssoc;
	}
	return NULL;
}

bool Dictionary::Lookup(const char *key, unsigned int type, unsigned long& rValue) const
{
	unsigned int nHash;
	MyAssoc* pAssoc = GetAssocAt(key, type, nHash);
	if (pAssoc == NULL)
		return false;  // not in map

	rValue = pAssoc->value;
	return true;
}

void Dictionary::SetAt(const char *key, unsigned int type, unsigned long value)
{
	unsigned int nHash;
	MyAssoc* pAssoc;
	if ((pAssoc = GetAssocAt(key, type, nHash)) == NULL)
	{
		if (m_pHashTable == NULL)
			InitHashTable(m_nHashTableSize);

		// it doesn't exist, add a new Association
		pAssoc = NewAssoc();
		pAssoc->key = key;

		// put into hash table
		pAssoc->pNext = m_pHashTable[nHash];
		m_pHashTable[nHash] = pAssoc;
	}
  else
  {//keep the stuff consistent (we need only one key in case of duplications)
    delete [] pAssoc->key; 
    pAssoc->key=key;
  }
  pAssoc->value=value;
}

