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
#include "Strings/StringConversion.h"

#include <cstdint>

namespace GemRB {

using ieByte = unsigned char;
using ieByteSigned = signed char;
using ieWord = uint16_t;
using ieWordSigned = int16_t;
using ieDword = uint32_t;
using ieDwordSigned = int32_t;

// it is perfectly valid to create an ieStrRef for any value, but -1 is invalid
// obviously we will define any hardcoded strrefs we use here
enum class ieStrRef : ieDword {
	INVALID = ieDword(-1),
	//the original games used these strings for custom biography (another quirk of the IE)
	BIO_START = 62016,            //first BIO string
	BIO_END   = (BIO_START + 5),  //last BIO string
	OVERRIDE_START = 450000,
	// not actually an ieStrRef, but can be &'ded with an ieStrRef to detrmine which TLK (eg dialogf.tlk) to use
	ALTREF = 0x0100000,
	
	// NOTE: all strrefs below this point are contextual
	// the names given match our hardcoded usage and shouldn't be expected to resolve to something sensical in all games
	// consider adding trailing comments for which games they apply to
	// consider grouping definitions by game
	
	// PST strrefs
	PST_REST_PERM = 38587,
	PST_REST_NOT_HERE = 34601,
	PST_HICCUP = 46633,
	
	NOT_A_SUB = 0, // subtitle 2das contain 0 insstead of -1
	NO_REST = 10309,
	DATE1 = 10699,
	DATE2 = 41277,
	DAYANDMONTH = 15981,
	FIGHTERTYPE = 10174,
	MD_FAIL = 24197,
	MD_SUCCESS = 24198,
	HEAL = 28895,
	
	// params to DisplayRollStringName
	ROLL0 = 112,
	ROLL1 = 20460,
	ROLL2 = 28379,
	ROLL3 = 39257,
	ROLL4 = 39258,
	ROLL5 = 39265,
	ROLL6 = 39266,
	ROLL7 = 39297,
	ROLL8 = 39298,
	ROLL9 = 39299,
	ROLL10 = 39300,
	ROLL11 = 39301,
	ROLL12 = 39302,
	ROLL13 = 39303,
	ROLL14 = 39304,
	ROLL15 = 39306,
	ROLL16 = 39673,
	ROLL17 = 39828,
	ROLL18 = 39829,
	ROLL19 = 39842,
	ROLL20 = 39846,
	ROLL21 = 40955,
	ROLL22 = 40974,
	ROLL23 = 40975
};

class ieVariable
{
	char var[33] {'\0'};
	
public:
	ieVariable() = default;
	ieVariable(std::nullptr_t) = delete;
	
	explicit ieVariable(const std::string& str)
	: ieVariable(str.c_str())
	{}
	
	explicit ieVariable(const String& str)
	: ieVariable(MBStringFromString(str))
	{}

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
	
	char* begin() noexcept {
		return var;
	}
};

using ResRef = FixedSizeString<8, strnicmp>;

}

namespace fmt {

template <>
struct formatter<GemRB::ResRef> : formatter<const char*> {
	template <typename FormatContext>
	auto format(const GemRB::ResRef& resref, FormatContext &ctx) -> decltype(ctx.out()) {
		return format_to(ctx.out(), resref.CString());
	}
};

template <>
struct formatter<GemRB::ieVariable> : formatter<const char*> {
	template <typename FormatContext>
	auto format(const GemRB::ieVariable& var, FormatContext &ctx) -> decltype(ctx.out()) {
		return format_to(ctx.out(), var.CString());
	}
};

}

#endif  //! IE_TYPES_H
