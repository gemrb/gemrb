/***************************************************************************
                         |Variables.h|  -  stolen Map class from MFC
                          -------------------
    begin                : |oct. 23. 2003|
    copyright            : (C) |2003| by |Avenger|
    email                : |@|
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#if !defined(AFX_VARIABLES_H__4EAE07B5_C375_4191_85AC_E5E15DBCD07B__INCLUDED_)
#define AFX_VARIABLES_H__4EAE07B5_C375_4191_85AC_E5E15DBCD07B__INCLUDED_

#include <ctype.h>
#include "../../includes/win32def.h"
#include "../../includes/globals.h"
#include "../../includes/SClassID.h"

/////////////////////////////////////////////////////////////////////////////
// Variables<unsigned long, VALUE>

class Variables //: public CObject
{
protected:
	// Association
	struct MyAssoc
	{
		MyAssoc* pNext;
		char *key;
		unsigned long value;
	};
public:
// Construction
	Variables(int nBlockSize = 10, int nHashTableSize=2049);

// Attributes
	// number of elements
	int GetCount() const;
	bool IsEmpty() const;

	// Lookup
	bool Lookup(const char *key, unsigned long& rValue) const;

// Operations
	void SetAt(const char *key, unsigned long newValue);
	void RemoveAll();
	void InitHashTable(unsigned int hashSize, bool bAllocNow = true);
	int ParseKey(int arg);

// Implementation
protected:
	MyAssoc** m_pHashTable;
	unsigned int m_nHashTableSize;
	bool m_lParseKey;
	int m_nCount;
	MyAssoc* m_pFreeList;
	struct Plex* m_pBlocks;
	int m_nBlockSize;

	MyAssoc* NewAssoc(const char *key);
	void FreeAssoc(MyAssoc*);
	MyAssoc* GetAssocAt(const char *, unsigned int&) const;
	inline bool MyCopyKey(char *&dest, const char * key) const;
        inline unsigned int MyHashKey(const char *) const;
        POSITION GetStartPosition() const;
        void GetNextAssoc(POSITION& rNextPosition, char*& rKey, unsigned long& rValue) const;

public:
	~Variables();
};

#endif // !defined(AFX_VARIABLES_H__4EAE07B5_C375_4191_85AC_E5E15DBCD07B__INCLUDED_)
