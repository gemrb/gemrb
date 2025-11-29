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

#include "Strings/CString.h"
#include "Strings/StringView.h"

#include <cstdint>
#include <unordered_map>

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
	BIO_START = 62016, //first BIO string
	BIO_END = (BIO_START + 5), //last BIO string
	OVERRIDE_START = 450000,
	// not actually an ieStrRef, but can be &'ded with an ieStrRef to determine which TLK (eg dialogf.tlk) to use
	ALTREF = 0x0100000,

	// NOTE: all strrefs below this point are contextual
	// the names given match our hardcoded usage and shouldn't be expected to resolve to something sensical in all games
	// consider adding trailing comments for which games they apply to
	// consider grouping definitions by game

	// ToB strrefs
	TOB_SEPTUPLE = 74094,

	// PST strrefs
	PST_REST_PERM = 38587,
	PST_REST_NOT_HERE = 34601,
	PST_HICCUP = 46633,
	PST_STR_MOD = 34849,

	NO_REST = 10309,
	DATE1 = 10699,
	DATE2 = 41277,
	DAYANDMONTH = 15981,
	FIGHTERTYPE = 10174,
	MD_FAIL = 24197,
	MD_SUCCESS = 24198,
	HEAL = 28895,
	DAMAGE = 22036,

	// iwd2 refs
	REBUKED = 20827,
	TURNED = 20828,
	COMMANDED = 41023,
	SHELL1 = 17067,
	SHELL2 = 17075,
	SHELLB = 17096,

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

using ieVariable = FixedSizeString<32, strnicmp>;
using ResRef = FixedSizeString<8, strnicmp>;

using ieVarsMap = std::unordered_map<ieVariable, ieDword, CstrHashCI>;

template<typename STR>
inline bool IsStar(const STR& str)
{
	return str[0] == '*';
}

inline ieVariable MakeVariable(const StringView& sv)
{
	ieVariable var;
	uint8_t count = ieVariable::Size - 1;
	auto dest = var.begin();
	auto source = sv.begin();
	while (count-- && source != sv.end()) {
		// TODO: we shouldnt call towlower here. ieVariable is case insensitive
		// we probably should be calling WriteVariableLC in the writers instead
		char c = std::towlower(*source++);
		if (!std::isspace(c)) { // the originals ignored whitespace too
			*dest++ = c;
		}
	}
	return var;
}

}

// NOTE: these specializations are only required due to something in fmt/ranges.h being preferred
// over our own format_as (I've also tried formatter and operator<<)
namespace fmt {

template<>
struct formatter<GemRB::ieVariable> : public fmt::formatter<const char*> {
	template<typename FormatContext>
	auto format(const GemRB::ieVariable& str, FormatContext& ctx) const -> decltype(ctx.out())
	{
		return fmt::formatter<const char*>::format(str.c_str(), ctx);
	}
};

template<>
struct formatter<GemRB::ResRef> : public fmt::formatter<const char*> {
	template<typename FormatContext>
	auto format(const GemRB::ResRef& str, FormatContext& ctx) const -> decltype(ctx.out())
	{
		return fmt::formatter<const char*>::format(str.c_str(), ctx);
	}
};

}

#endif //! IE_TYPES_H
