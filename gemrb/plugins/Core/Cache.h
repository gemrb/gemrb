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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/Cache.h,v 1.3 2005/01/27 17:27:30 avenger_teambg Exp $
 *
 */

#ifndef CACHE_H
#define CACHE_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <ctype.h>
#ifdef WIN32
#include <MAP>
#else
#include <ext/hash_map>
#endif
#include "../../includes/win32def.h"
#include "../../includes/globals.h"
#include "../../includes/SClassID.h"

#ifndef SGI_HASH_NAMESPACE
#if defined(__SGI_STL_HASH_MAP) || defined(_STLP_HASH_MAP)
#define SGI_HASH_NAMESPACE std
#elif defined(__SGI_STL_INTERNAL_HASH_MAP_H) || defined(_HASH_MAP)
#define SGI_HASH_NAMESPACE __gnu_cxx
#endif
#endif // SGI_HASH_NAMESPACE
#ifdef WIN32

#ifdef GEM_BUILD_DLL
#define GEM_EXPORT __declspec(dllexport)
#else
#define GEM_EXPORT __declspec(dllimport)
#endif

#else
#define GEM_EXPORT
#endif

class GEM_EXPORT Cache {
protected:
        struct ValueType {
                ieDword nRefCount;
                void* data; 
        } Value;

        struct eqstr
        {
                bool operator()(const char* s1, const char* s2) const
                {
                        return !strnicmp( s1, s2, MAX_VARIABLE_LENGTH );
                }
        };
        inline bool MyCopyKey(char*& dest, ieResRef key) const;
#ifdef WIN32
        typedef std::map<const char*, ValueType, eqstr> HashMapType;
#else
        typedef SGI_HASH_NAMESPACE::hash_map<const char*, ValueType, SGI_HASH_NAMESPACE::hash<const char *>, eqstr> HashMapType;
#endif
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
