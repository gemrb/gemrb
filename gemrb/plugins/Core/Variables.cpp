#include "Variables.h"
#ifdef WIN32
int strnlen(const char * string, int maxlen) {
	if(!string)
		return -1;
	int acc = 0;
	int i = 0;
	while(maxlen > 0) {
		if(string[i++])
			acc++;
		else
			break;
	}
	return acc;
}
#endif

/////////////////////////////////////////////////////////////////////////////
// inlines
inline bool Variables::MyCopyKey(char *&dest, const char * key) const
{
	int i,j;

	for(i=0,j=0;key[i] && i<MAX_VARIABLE_LENGTH;i++) if(key[i]!=' ') j++;
	dest = new char[j+1];
	if(!dest)
		return false;
	for(i=0,j=0;i<MAX_VARIABLE_LENGTH && key[i];i++)
	{
		if(key[i]!=' ') dest[j++]=toupper(key[i]);
   	}
	dest[j]=0;
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
//sets the way we handle keys, no parsing for .ini file entries, parsing for game variables
//you should set this only on an empty mapping
inline int Variables::ParseKey(int arg)
{
	MYASSERT(m_nCount==0);
	m_lParseKey=arg;
}
inline int Variables::GetCount() const
	{ return m_nCount; }
inline bool Variables::IsEmpty() const
	{ return m_nCount == 0; }
inline POSITION Variables::GetStartPosition() const
        { return (m_nCount == 0) ? NULL : BEFORE_START_POSITION; }

/////////////////////////////////////////////////////////////////////////////
// out of lines
void Variables::GetNextAssoc(POSITION& rNextPosition, const char*& rKey, unsigned long& rValue) const
{
        MYASSERT(m_pHashTable != NULL);  // never call on empty map

        Variables::MyAssoc* pAssocRet = (Variables::MyAssoc*)rNextPosition;
        MYASSERT(pAssocRet != NULL);

        if (pAssocRet == (Variables::MyAssoc*) BEFORE_START_POSITION)
        {
                // find the first association
                for (unsigned int nBucket = 0; nBucket < m_nHashTableSize; nBucket++)
                        if ((pAssocRet = m_pHashTable[nBucket]) != NULL)
                                break;
                MYASSERT(pAssocRet != NULL);  // must find something
        }
        Variables::MyAssoc* pAssocNext;
        if ((pAssocNext = pAssocRet->pNext) == NULL)
        {
                // go to next bucket
                for (unsigned int nBucket = pAssocRet->nHashValue + 1;
                  nBucket < m_nHashTableSize; nBucket++)
                        if ((pAssocNext = m_pHashTable[nBucket]) != NULL)
                                break;
        }

        rNextPosition = (POSITION) pAssocNext;

        // fill in return data
        rKey = pAssocRet->key;
        rValue = pAssocRet->nValue;
}
Variables::Variables(int nBlockSize, int nHashTableSize)
{
	MYASSERT(nBlockSize > 0);
	MYASSERT(nHashTableSize > 16);

	m_pHashTable = NULL;
	m_nHashTableSize = nHashTableSize;  // default size
	m_nCount = 0;
	m_lParseKey = false;
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
		m_pHashTable = new Variables::MyAssoc* [nHashSize];
		memset(m_pHashTable, 0, sizeof(Variables::MyAssoc*) * nHashSize);
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
			Variables::MyAssoc* pAssoc;
			for (pAssoc = m_pHashTable[nHash]; pAssoc != NULL;
			  pAssoc = pAssoc->pNext)
			{
				if(pAssoc->key)
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
	if(m_lParseKey) MyCopyKey(pAssoc->key,key);
	else
	{
		int len;
		len=strnlen(key,MAX_VARIABLE_LENGTH)+1;
		pAssoc->key=new char[len];
		if(pAssoc->key) strncpy(pAssoc->key,key,len);
	}
	pAssoc->nValue=0xcccccccc;  //invalid value
	pAssoc->nHashValue=0xcccccccc; //invalid value
	return pAssoc;
}

void Variables::FreeAssoc(Variables::MyAssoc* pAssoc)
{
	if(pAssoc->key) delete[] pAssoc->key;
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
	Variables::MyAssoc* pAssoc;
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
	Variables::MyAssoc* pAssoc = GetAssocAt(key, nHash);
	if (pAssoc == NULL)
		return false;  // not in map

	rValue = pAssoc->nValue;
	return true;
}

void Variables::SetAt(const char *key, unsigned long value)
{
	unsigned int nHash;
	Variables::MyAssoc* pAssoc;

	if ((pAssoc = GetAssocAt(key, nHash)) == NULL)
	{
		if (m_pHashTable == NULL)
			InitHashTable(m_nHashTableSize);

		// it doesn't exist, add a new Association
		pAssoc = NewAssoc(key);
		// put into hash table
		pAssoc->pNext = m_pHashTable[nHash];
		m_pHashTable[nHash] = pAssoc;
	}
//set value only if we have a key
	if(pAssoc->key)
	 {
		pAssoc->nValue=value;
		pAssoc->nHashValue=nHash;
	}
}

