// SPDX-FileCopyrightText: 2022 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef STRING_CONV_H
#define STRING_CONV_H

#include "String.h"
#include "StringView.h"

namespace GemRB {

struct EncodingStruct {
	std::string encoding = "ISO-8859-1";
	bool widechar = false;
	bool multibyte = false;
	bool zerospace = false;
};

// String creators
GEM_EXPORT char* ConvertCharEncoding(const char* string, const char* from, const char* to);
GEM_EXPORT String StringFromEncodedView(const StringView& view, const EncodingStruct& encoding);
GEM_EXPORT String StringFromASCII(const StringView& asciiview);
GEM_EXPORT String StringFromTLK(const StringView& tlkview);
GEM_EXPORT String StringFromUtf8(const char* data);
GEM_EXPORT String StringFromUtf8(const StringView& tlkview);
GEM_EXPORT std::string RecodedStringFromWideStringBytes(const char16_t* bytes, size_t bytesLength, const std::string& encoding);
GEM_EXPORT std::string MBStringFromString(const String& string);
GEM_EXPORT std::string TLKStringFromString(const String& string);

}

namespace fmt {

struct WideToChar {
	const GemRB::String& string;
};

template<>
struct formatter<WideToChar> {
	// FIXME: parser doesnt do anything
	static constexpr auto parse(const format_parse_context& ctx) -> decltype(ctx.begin()) { return ctx.begin(); }

	template<typename FormatContext>
	auto format(const WideToChar& wstr, FormatContext& ctx) const -> decltype(ctx.out())
	{
		const auto mbstr = GemRB::MBStringFromString(wstr.string);
		return format_to(ctx.out(), "{}", mbstr);
	}
};

}

#endif
