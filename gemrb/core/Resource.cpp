/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

#include "Resource.h"

#include "System/DataStream.h"

namespace GemRB {

Resource::Resource(void)
{
	str = NULL;
}

Resource::~Resource(void)
{
	if (str) {
		delete( str );
	}
}

// Very similar to Cache::MyHashKey, but returns size_t and can be used from
// things like unordered_map.
size_t ResRef::Hash::operator() (const ResRef &k) const
{
	size_t nHash = 0;
	for (char c: k.ref) {
		if (c == 0)
			break;
		nHash = (nHash << 5) ^ tolower(c);
	}
	return nHash;
}

}
