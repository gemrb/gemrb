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

/**
 * @file ie_types.h
 * Defines data types used to load IE structures
 * @author The GemRB Project
 */


#ifndef IE_TYPES_H
#define IE_TYPES_H

#include "Platform.h"

#include <cstring>

namespace GemRB {

typedef unsigned char ieByte;
typedef signed char ieByteSigned;
typedef unsigned short ieWord;
typedef signed short ieWordSigned;

#if (SIZEOF_INT == 4)
typedef unsigned int ieDword;
typedef signed int ieDwordSigned;
#elif (SIZE_LONG_INT == 4)
typedef unsigned long int ieDword;
typedef signed long int ieDwordSigned;
#else
typedef unsigned long int ieDword;
typedef signed long int ieDwordSigned;
#endif

/** string reference into TLK file */
typedef ieDword ieStrRef;

class ieVariable
{
	char var[33] {'\0'};
	
public:
	ieVariable() = default;
	ieVariable(std::nullptr_t) = delete;

	ieVariable(const char* c) noexcept {
		operator=(c);
	}
	
	ieVariable& operator=(const char* c) noexcept {
		strncpy(var, c, sizeof(var) - 1);
		return *this;
	}
	
	operator const char*() const noexcept {
		return var;
	}
	
	operator char*() noexcept {
		return var;
	}
	
	const char* CString() const noexcept {
		return var;
	}
};

}

#endif  //! IE_TYPES_H
