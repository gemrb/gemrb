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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/Cache.h,v 1.1 2005/01/21 18:45:13 avenger_teambg Exp $
 *
 */

#ifndef CACHE_H
#define CACHE_H

#include <ctype.h>
#include "../../includes/win32def.h"
#include "../../includes/globals.h"
#include "../../includes/SClassID.h"
#include <ext/hash_map>

#ifndef SGI_HASH_NAMESPACE
#if defined(__SGI_STL_HASH_MAP) || defined(_STLP_HASH_MAP)
#define SGI_HASH_NAMESPACE std
#elif defined(__SGI_STL_INTERNAL_HASH_MAP_H)
#define SGI_HASH_NAMESPACE __gnu_cxx
#endif
#endif // SGI_HASH_NAMESPACE

class GEM_EXPORT Cache {
protected:
        struct ValueType {
                ieDword nRefCount;
                void* data; 
        } Value;

        inline bool MyCopyKey(char*& dest, ieResRef key) const;
        struct eqstr
        {
                bool operator()(const char* s1, const char* s2) const
                {
                        return !strnicmp( s1, s2, MAX_VARIABLE_LENGTH );
                }
        };
        typedef SGI_HASH_NAMESPACE::hash_map<const char*, ValueType, SGI_HASH_NAMESPACE::hash<const char *>, eqstr> HashMapType;
        HashMapType hashmap;

public:
        // Construction
        Cache();
	~Cache();

        inline int GetCount() const
        {
                return hashmap.size();
        }
        inline bool IsEmpty() const
        {
                return hashmap.empty();
        }
        inline POSITION GetStartPosition() const
        {
                return ( IsEmpty() ) ? NULL : BEFORE_START_POSITION;
        }
        // Operations
	// returns pointer if resource is already loaded
        void *GetResource(ieResRef key);
	// sets resource, returns true if it didn't exist before
	bool SetAt(ieResRef key, void *rValue);
	// decreases refcount or drops data pointed by key
	int DecRef(ieResRef key, bool free);
	// decreases refcount or drops data
	int DecRef(void *rValue, bool free);
	// frees all
	void RemoveAll();
	// cleans up zero refcount
	void Cleanup(); //removes only zero refcounts
};

#endif //CACHE_H
