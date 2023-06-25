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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

#ifndef VARIABLES_H
#define VARIABLES_H

#include "exports.h"
#include "globals.h"

#include "Cache.h"
#include "Strings/String.h"
#include "Strings/StringView.h"

#include <cassert>

namespace GemRB {

#ifndef ReleaseFun
using ReleaseFun = void (*)(void*);
#endif

#define GEM_VARIABLES_INT      0
#define GEM_VARIABLES_STRING   1
#define GEM_VARIABLES_POINTER  2

class GEM_EXPORT Variables {
protected:
	// Association
	class MyAssoc {
		MyAssoc* pNext;
		char* key;
		union {
			ieDword nValue;
			char* sValue;
			void* pValue;
		} Value;
		unsigned long nHashValue;
		friend class Variables;
	};
	struct MemBlock {
		MemBlock* pNext;
	};
public:
	// abstract iteration position
	using iterator = MyAssoc*;
	using key_t = StringView;

	// Construction
	explicit Variables(int nBlockSize = 10, int nHashTableSize = 2049);
	Variables(const Variables&) = delete;
	~Variables();
	Variables& operator=(const Variables&) = delete;
	void LoadInitialValues(const ResRef& name);

	// Attributes
	//sets the way we handle keys, no parsing for .ini file entries, parsing for game variables
	//you should set this only on an empty mapping
	inline int ParseKey(int arg)
	{
		assert( m_nCount == 0 );
		m_lParseKey = ( arg > 0 );
		return 0;
	}
	//sets the way we handle values
	inline void SetType(int type)
	{
		m_type = type;
	}
	inline int GetCount() const
	{
		return m_nCount;
	}
	inline bool IsEmpty() const
	{
		return m_nCount == 0;
	}

	bool Lookup(const key_t&, ieDword& rValue) const;
	bool Lookup(const key_t&, String& dest) const;
	bool Lookup(const key_t&, std::string& dest) const;
	bool Lookup(const key_t&, void*& dest) const;
	bool HasKey(const key_t&) const;
	
	template<typename NUM>
	typename std::enable_if<std::is_integral<NUM>::value || std::is_enum<NUM>::value, bool>::type
	Lookup(const key_t& key, NUM& rValue) const {
		ieDword val;
		bool ret = Lookup(key, val);
		rValue = static_cast<NUM>(val);
		return ret;
	}
	
	template<typename T>
	void SetAtAsString(const key_t& key, T&& newValue)
	{
		const std::string& newStr = fmt::format("{}", std::forward<T>(newValue));
		SetAtCString(key, newStr.c_str());
	}
	
	void SetAt(const key_t& key, const String& newValue)
	{
		const std::string& mbstr = MBStringFromString(newValue);
		SetAtCString(key, mbstr.c_str());
	}
	
	template<typename VSTR, ENABLE_CHAR_RANGE(VSTR)>
	void SetAt(const key_t& key, const VSTR& newValue)
	{
		SetAtCString(key, &newValue[0]);
	}
	
	void SetAt(const key_t& key, const char* val) = delete;

	void SetAt(const key_t&, void* newValue);
	void SetAt(const key_t&, ieDword newValue, bool nocreate=false);
	void Remove(const key_t&);
	void RemoveAll(ReleaseFun fun);
	void InitHashTable(unsigned int hashSize, bool bAllocNow = true);

	iterator GetNextAssoc(iterator rNextPosition, key_t& rKey,
		ieDword& rValue) const;

	// Debugging
	void DebugDump() const;
	// Implementation
protected:
	Variables::MyAssoc** m_pHashTable;
	unsigned int m_nHashTableSize;
	bool m_lParseKey;
	int m_nCount;
	Variables::MyAssoc* m_pFreeList;
	MemBlock* m_pBlocks;
	int m_nBlockSize;
	int m_type; //could be string or ieDword 

	Variables::MyAssoc* NewAssoc(const key_t&);
	void FreeAssoc(Variables::MyAssoc*);
	Variables::MyAssoc* GetAssocAt(const key_t&, unsigned int&) const;
	inline bool MyCopyKey(char*& dest, const key_t&) const;
	inline bool MyCompareKey(const key_t&, key_t str) const;
	inline unsigned int MyHashKey(const key_t&) const;

	void SetAtCString(const key_t&, const char* newValue);
};

void LoadInitialValues(const ResRef& name, std::unordered_map<ResRef, ieDword, ResRefHash>& map);

}

#endif
