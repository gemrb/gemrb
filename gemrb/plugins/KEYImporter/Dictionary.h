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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/KEYImporter/Dictionary.h,v 1.8 2004/09/13 21:40:29 avenger_teambg Exp $
 *
 */

/***************************************************************************
						 |Dictionary.h|  -  stolen Map class from MFC
 ***************************************************************************/

#if !defined(AFX_DICTIONARY_H__4EAE07B5_C375_4191_85AC_E5E15DBCD07B__INCLUDED_)
#define AFX_DICTIONARY_H__4EAE07B5_C375_4191_85AC_E5E15DBCD07B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <ctype.h>
#include "../../includes/win32def.h"
#include "../../includes/SClassID.h"

#define THIS_FILE "dictionary.h"

/////////////////////////////////////////////////////////////////////////////
// Dictionary<unsigned long, VALUE>

class Dictionary //: public CObject
{
protected:
	// Association
	struct MyAssoc {
		MyAssoc* pNext;
		char* key;
		SClass_ID type;
		unsigned long value;
	};
	struct MemBlock {
		MemBlock* pNext;
	};

public:
	// Construction
	Dictionary(int nBlockSize = 10, int nHashTableSize = 32769);

	// Attributes
	// number of elements
	int GetCount() const;
	bool IsEmpty() const;

	// Lookup
	bool Lookup(const char* key, unsigned int type, unsigned long& rValue) const;

	// Operations
	void SetAt(const char* key, unsigned int type, unsigned long newValue);
	void RemoveAll();
	void InitHashTable(unsigned int hashSize, bool bAllocNow = true);

	// Implementation
protected:
	MyAssoc** m_pHashTable;
	unsigned int m_nHashTableSize;
	int m_nCount;
	MyAssoc* m_pFreeList;
	MemBlock* m_pBlocks;
	int m_nBlockSize;

	MyAssoc* NewAssoc();
	void FreeAssoc(MyAssoc*);
	MyAssoc* GetAssocAt(const char*, unsigned int, unsigned int&) const;
	unsigned int MyHashKey(const char*, unsigned int) const;

public:
	~Dictionary();
};

#endif // !defined(AFX_DICTIONARY_H__4EAE07B5_C375_4191_85AC_E5E15DBCD07B__INCLUDED_)
