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
#include "System/CString.h"
#include "System/String.h"

namespace GemRB {

using ieByte = unsigned char;
using ieByteSigned = signed char;
using ieWord = unsigned short;
using ieWordSigned = signed short;

#if (SIZEOF_INT == 4)
using ieDword = unsigned int;
using ieDwordSigned = signed int;
#elif (SIZE_LONG_INT == 4)
using ieDword = unsigned long int;
using ieDwordSigned = signed long int;
#else
using ieDword = unsigned long int;
using ieDwordSigned = signed long int;
#endif

/** string reference into TLK file */
using ieStrRef = ieDword;

class ieVariable
{
	char var[33] {'\0'};
	
public:
	ieVariable() = default;
	ieVariable(std::nullptr_t) = delete;

	explicit ieVariable(const char* c) noexcept {
		operator=(c);
	}
	
	ieVariable& operator=(const char* c) noexcept {
		if (c) {
			strncpy(var, c, sizeof(var) - 1);
		} else {
			memset(var, 0, sizeof(var));
		}
		return *this;
	}
	
	explicit operator const char*() const noexcept {
		return var;
	}

	char operator[](size_t i) const noexcept {
		return var[i];
	}
	
	char& operator[](size_t i) noexcept {
		return var[i];
	}

	operator char*() noexcept {
		return var;
	}
	
	const char* CString() const noexcept {
		return var;
	}
};

using ResRef = FixedSizeString<8, strnicmp>;

}

namespace fmt {

template <>
struct fmt::formatter<GemRB::ResRef> : formatter<const char*> {
	template <typename FormatContext>
	auto format(const GemRB::ResRef& resref, FormatContext &ctx) -> decltype(ctx.out()) {
		return format_to(ctx.out(), resref.CString());
	}
};

template <>
struct fmt::formatter<GemRB::ieVariable> : formatter<const char*> {
	template <typename FormatContext>
	auto format(const GemRB::ieVariable& var, FormatContext &ctx) -> decltype(ctx.out()) {
		return format_to(ctx.out(), var.CString());
	}
};

}

#endif  //! IE_TYPES_H
