/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2022 The GemRB Project
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
 */

#ifndef STRING_CONV_H
#define STRING_CONV_H

#include "CString.h"
#include "String.h"
#include "ie_types.h"

namespace GemRB {

struct EncodingStruct
{
	std::string encoding = "ISO-8859-1";
	bool widechar = false;
	bool multibyte = false;
	bool zerospace = false;
};

// String creators
GEM_EXPORT char* ConvertCharEncoding(const char * string, const char * from, const char* to);
GEM_EXPORT String StringFromCString(const char* string);
GEM_EXPORT String StringFromResRef(const ResRef& string);
GEM_EXPORT String StringFromUtf8(const char* string);
GEM_EXPORT std::string RecodedStringFromWideStringBytes(const char16_t* bytes, size_t bytesLength, const std::string& encoding);
GEM_EXPORT std::string MBStringFromString(const String& string);
GEM_EXPORT std::string TLKStringFromString(const String& string);

}

namespace fmt {

struct WideToChar {
	const GemRB::String& string;
};

template <>
struct formatter<WideToChar> {
	// FIXME: parser doesnt do anything
	static constexpr auto parse(const format_parse_context& ctx) -> decltype(ctx.begin()) { return ctx.begin(); }

	template <typename FormatContext>
	auto format(const WideToChar& wstr, FormatContext& ctx) -> decltype(ctx.out()) {
		const auto mbstr = GemRB::MBStringFromString(wstr.string);
		return format_to(ctx.out(), "{}", mbstr);
	}
};

}

#endif
