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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/Variables.h,v 1.13 2003/12/03 18:34:58 doc_wagon Exp $
 *
 */

// |Variables.h|  -  stolen Map class from MFC

#if !defined(AFX_VARIABLES_H__4EAE07B5_C375_4191_85AC_E5E15DBCD07B__INCLUDED_)
#define AFX_VARIABLES_H__4EAE07B5_C375_4191_85AC_E5E15DBCD07B__INCLUDED_

#include <ctype.h>
#include "../../includes/win32def.h"
#include "../../includes/globals.h"
#include "../../includes/SClassID.h"

/////////////////////////////////////////////////////////////////////////////
// Variables<unsigned long, VALUE>

#define GEM_VARIABLES_INT      0
#define GEM_VARIABLES_STRING   1

#ifdef WIN32

#ifdef GEM_BUILD_DLL
#define GEM_EXPORT __declspec(dllexport)
#else
#define GEM_EXPORT __declspec(dllimport)
#endif

#else
#define GEM_EXPORT
#endif

class GEM_EXPORT Variables
{
protected:
	// Association
	struct MyAssoc
	{
		MyAssoc* pNext;
		char *key;
		unsigned long nValue;
		unsigned long nHashValue;
	};
public:
// Construction
	Variables(int nBlockSize = 10, int nHashTableSize=2049);

// Attributes
//sets the way we handle keys, no parsing for .ini file entries, parsing for game variables
//you should set this only on an empty mapping
inline int ParseKey(int arg) {
        MYASSERT(m_nCount==0);
        m_lParseKey=(arg > 0);
        return 0;
}
//sets the way we handle values
inline void SetType(int type)
        { m_type=type; }
inline int GetCount() const
        { return m_nCount; }
inline bool IsEmpty() const
        { return m_nCount == 0; }
inline POSITION GetStartPosition() const
        { return (m_nCount == 0) ? NULL : BEFORE_START_POSITION; }

	// Lookup
	int GetValueLength(const char *key) const;
        bool Lookup(const char *key, char *dest, int MaxLength) const;
	bool Lookup(const char *key, unsigned long& rValue) const;

// Operations
	void SetAt(const char *key, const char *newValue);
	void SetAt(const char *key, unsigned long newValue);
	void RemoveAll();
	void InitHashTable(unsigned int hashSize, bool bAllocNow = true);

// Implementation
protected:
	MyAssoc** m_pHashTable;
	unsigned int m_nHashTableSize;
	bool m_lParseKey;
	int m_nCount;
	MyAssoc* m_pFreeList;
	struct Plex* m_pBlocks;
	int m_nBlockSize;
	int m_type; //could be string or unsigned long

	MyAssoc* NewAssoc(const char *key);
	void FreeAssoc(MyAssoc*);
	MyAssoc* GetAssocAt(const char *, unsigned int&) const;
	inline bool MyCopyKey(char *&dest, const char * key) const;
        inline unsigned int MyHashKey(const char *) const;
//	 POSITION GetStartPosition() const;
        void GetNextAssoc(POSITION& rNextPosition, const char*& rKey, unsigned long& rValue) const;

public:
	~Variables();
};

#endif // !defined(AFX_VARIABLES_H__4EAE07B5_C375_4191_85AC_E5E15DBCD07B__INCLUDED_)
