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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/KEYImporter/Dictionary.h,v 1.10 2005/03/05 10:13:58 guidoj Exp $
 *
 */

#if !defined(DICTIONARY_H)
#define DICTIONARY_H

#include <ctype.h>
#include "../../includes/win32def.h"
#include "../../includes/SClassID.h"

/////////////////////////////////////////////////////////////////////////////
// Dictionary<unsigned long, VALUE>

#define KEYSIZE 8

class Dictionary //: public CObject
{
protected:
	// Association
	struct MyAssoc {
		MyAssoc* pNext;
		char key[KEYSIZE]; //not ieResRef!
		SClass_ID type;
		unsigned int value;
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
	bool Lookup(const ieResRef key, unsigned int type, unsigned int& rValue) const;

	// Operations
	void SetAt(const ieResRef key, unsigned int type, unsigned int newValue);
	void RemoveAt(const ieResRef key, unsigned int type);
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
	MyAssoc* GetAssocAt(const ieResRef, unsigned int, unsigned int&) const;
	unsigned int MyHashKey(const ieResRef, unsigned int) const;

public:
	~Dictionary();
};

#endif // !defined(DICTIONARY_H)
