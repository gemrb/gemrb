#include "Variables.h"

#define THIS_FILE "variables.cpp"

#define MYASSERT(f) \
  if(!(f))  \
  {  \
  printf("Assertion failed: %s %d",THIS_FILE, __LINE__); \
		abort(); \
  }

/////////////////////////////////////////////////////////////////////////////
// inlines
inline bool Variables::MyCopyKey(char *&dest, const char * key) const
{
	int i,j;

	for(i=0;key[i] && i<MAX_VARIABLE_LENGTH;i++) if(key[i]!=' ') j++;
	dest = new char[j];
	if(!dest)
		return false;
	for(i=0,j=0;i<MAX_VARIABLE_LENGTH && key[i];i++)
	{
		if(key[i]!=' ') dest[j++]=toupper(key[i]);
   	}
	return true;
}
inline unsigned int Variables::MyHashKey(const char * key) const
{
   unsigned int nHash = 0;
   for(int i=0;i<MAX_VARIABLE_LENGTH && key[i];i++)
   {
//the original engine ignores spaces in variable names
	if(key[i]!=' ') nHash = (nHash<<5) + nHash + toupper(key[i]);
   }
   return nHash;
}
inline int Variables::GetCount() const
	{ return m_nCount; }
inline bool Variables::IsEmpty() const
	{ return m_nCount == 0; }
/////////////////////////////////////////////////////////////////////////////
// out of lines
Variables::Variables(int nBlockSize, int nHashTableSize)
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

void Variables::InitHashTable(unsigned int nHashSize, bool bAllocNow)
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

void Variables::RemoveAll()
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

Variables::~Variables()
{
	RemoveAll();
	MYASSERT(m_nCount == 0);
}

Variables::MyAssoc *
Variables::NewAssoc(const char *key)
{
	if (m_pFreeList == NULL)
	{
		// add another block
		Plex* newBlock = Plex::Create(m_pBlocks, m_nBlockSize, sizeof(Variables::MyAssoc));
		// chain them into free list
		Variables::MyAssoc* pAssoc = (Variables::MyAssoc*) newBlock->data();
		// free in reverse order to make it easier to debug
		pAssoc += m_nBlockSize - 1;
		for (int i = m_nBlockSize-1; i >= 0; i--, pAssoc--)
		{
			pAssoc->pNext = m_pFreeList;
			m_pFreeList = pAssoc;
		}
	}
	MYASSERT(m_pFreeList != NULL);  // we must have something

	Variables::MyAssoc* pAssoc = m_pFreeList;
	m_pFreeList = m_pFreeList->pNext;
	m_nCount++;
	MYASSERT(m_nCount > 0);    // make sure we don't overflow
	MyCopyKey(pAssoc->key,key);
	pAssoc->value=0xcccccccc;  //invalid value
	return pAssoc;
}

void Variables::FreeAssoc(Variables::MyAssoc* pAssoc)
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

Variables::MyAssoc*
Variables::GetAssocAt(const char *key, unsigned int& nHash) const
// find association (or return NULL)
{
	nHash = MyHashKey(key) % m_nHashTableSize;

	if (m_pHashTable == NULL)
		return NULL;

	// see if it exists
	MyAssoc* pAssoc;
	for (pAssoc = m_pHashTable[nHash]; pAssoc != NULL; pAssoc = pAssoc->pNext)
	{
		if(!strnicmp(pAssoc->key, key,MAX_VARIABLE_LENGTH) )
			return pAssoc;
	}
	return NULL;
}

bool Variables::Lookup(const char *key, unsigned long& rValue) const
{
	unsigned int nHash;
	MyAssoc* pAssoc = GetAssocAt(key, nHash);
	if (pAssoc == NULL)
		return false;  // not in map

	rValue = pAssoc->value;
	return true;
}

void Variables::SetAt(const char *key, unsigned long value)
{
	unsigned int nHash;
	MyAssoc* pAssoc;
	if ((pAssoc = GetAssocAt(key, nHash)) == NULL)
	{
		if (m_pHashTable == NULL)
			InitHashTable(m_nHashTableSize);

		// it doesn't exist, add a new Association
		pAssoc = NewAssoc(pAssoc->key);

		// put into hash table
		pAssoc->pNext = m_pHashTable[nHash];
		m_pHashTable[nHash] = pAssoc;
	}
	pAssoc->value=value;
}

