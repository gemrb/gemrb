/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2022 The GemRB Project
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
 */

#include "StringConversion.h"

#include "ie_types.h"

#include "Iconv.h"
#include "Interface.h"

#include "Logging/Logging.h"

namespace GemRB {

static String StringFromEncodedData(const void* inptr, size_t length, const std::string& encoding)
{
	if (!inptr || length == 0) {
		return u"";
	}

	iconv_t cd = nullptr;
	// Must check since UTF-16 may fall back to UTF-16BE, even if platform is LE
	if (IsBigEndian()) {
		cd = iconv_open("UTF-16BE", encoding.c_str());
	} else {
		cd = iconv_open("UTF-16LE", encoding.c_str());
	}

	if (cd == (iconv_t) -1) {
		Log(ERROR, "String", "iconv_open(UTF-16, {}) failed with error: {}", encoding, strerror(errno));
		return u"";
	}

	std::u16string buffer(length, u'\0');
	auto outBuf = reinterpret_cast<char*>(const_cast<char16_t*>(buffer.data()));
	const char* in = static_cast<const char*>(inptr);

	size_t outLen = length * sizeof(char16_t);
	size_t ret = portableIconv(cd, &in, &length, &outBuf, &outLen);
	iconv_close(cd);

	if (ret == static_cast<size_t>(-1)) {
		Log(ERROR, "String", "iconv failed to convert string from {} to UTF-16 with error: {}", encoding, strerror(errno));
		return u"";
	}

	if (outLen) {
		buffer.resize(buffer.size() - (outLen / sizeof(char16_t)));
	}

	return buffer;
}

template<typename VIEW>
String StringFromEncodedView(const VIEW& view, const EncodingStruct& encoding)
{
	return StringFromEncodedData(view.c_str(), view.length() * sizeof(typename VIEW::value_type), encoding.encoding);
}

String StringFromEncodedView(const StringView& view, const EncodingStruct& encoding)
{
	return StringFromEncodedData(view.c_str(), view.length(), encoding.encoding);
}

String StringFromASCII(const StringView& asciiview)
{
	static const EncodingStruct enc { "ASCII", false, false, false };
	return StringFromEncodedView(asciiview, enc);
}

String StringFromTLK(const StringView& tlkview)
{
	const EncodingStruct& encoding = core->TLKEncoding;
	if (encoding.widechar) {
		const wchar_t* str = reinterpret_cast<const wchar_t*>(tlkview.c_str());
		size_t len = tlkview.length() / sizeof(wchar_t);
		StringViewImp<const wchar_t> wideView(str, len);
		return StringFromEncodedView(wideView, core->TLKEncoding);
	} else {
		return StringFromEncodedView(tlkview, core->TLKEncoding);
	}
}

String StringFromUtf8(const StringView& utf8view)
{
	static const EncodingStruct enc { "UTF-8", false, true, false };
	return StringFromEncodedView(utf8view, enc);
}

String StringFromUtf8(const char* data)
{
	return StringFromUtf8(StringView(data));
}

std::string RecodedStringFromWideStringBytes(const char16_t* bytes, size_t bytesLength, const std::string& encoding)
{
	char* in = reinterpret_cast<char*>(const_cast<char16_t*>(bytes));

	if (bytesLength == 0) {
		return "";
	}

	iconv_t cd = nullptr;
	if (IsBigEndian()) {
		cd = iconv_open(encoding.c_str(), "UTF-16BE");
	} else {
		cd = iconv_open(encoding.c_str(), "UTF-16LE");
	}

	size_t outLen = bytesLength * 2;
	size_t outLenLeft = outLen;
	std::string buffer(outLen, '\0');
	auto outBuf = const_cast<char*>(buffer.data());

	size_t ret = portableIconv(cd, &in, &bytesLength, &outBuf, &outLenLeft);
	iconv_close(cd);

	if (ret == static_cast<size_t>(-1)) {
		Log(ERROR, "String", "iconv failed to convert string a string from UTF-16 to {} with error: {}", encoding, strerror(errno));
		return "";
	}

	return buffer;
}

static std::string RecodedStringFromString(const String& string, const std::string& encoding)
{
	return RecodedStringFromWideStringBytes(string.c_str(), string.length() * sizeof(char16_t), encoding);
}

std::string MBStringFromString(const String& string)
{
	auto newString = RecodedStringFromString(string, "UTF-8");

	auto zero = newString.find('\0');
	if (zero != decltype(newString)::npos) {
		newString.resize(zero);
	}

	return newString;
}

std::string TLKStringFromString(const String& string)
{
	auto newString = RecodedStringFromString(string, core->TLKEncoding.encoding);

	auto zero = newString.find('\0');
	if (zero != decltype(newString)::npos) {
		newString.resize(zero);
	}

	return newString;
}

}
