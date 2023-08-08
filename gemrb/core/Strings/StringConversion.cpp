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

#include <iconv.h>

#include "ie_types.h"
#include "Interface.h"
#include "Logging/Logging.h"

namespace GemRB {

static String StringFromEncodedData(const char* in, size_t length, const std::string& encoding)
{
	if (!in || length == 0) {
		return u"";
	}

	iconv_t cd = nullptr;
	// Must check since UTF-16 may fall back to UTF-16BE, even if platform is LE
	if (IsBigEndian()) {
		cd = iconv_open("UTF-16BE", encoding.c_str());
	} else {
		cd = iconv_open("UTF-16LE", encoding.c_str());
	}

	if (cd == (iconv_t)-1) {
		Log(ERROR, "String", "iconv_open(UTF-16, {}) failed with error: {}", encoding, strerror(errno));
		return u"";
	}

	size_t outLen = length * 4;
	size_t outLenLeft = outLen;
	std::u16string buffer(length * 2, u'\0');
	auto outBuf = reinterpret_cast<char*>(const_cast<char16_t*>(buffer.data()));

	size_t ret = iconv(cd, const_cast<char**>(&in), &length, &outBuf, &outLenLeft);
	iconv_close(cd);

	if (ret == static_cast<size_t>(-1)) {
		Log(ERROR, "String", "iconv failed to convert string {} from {} to UTF-16 with error: {}", in, encoding, strerror(errno));
		return u"";
	}

	auto zero = buffer.find(u'\0');
	if (zero != decltype(buffer)::npos) {
		buffer.resize(zero);
	}

	return buffer;
}

static String StringFromEncodedData(const char* string, const EncodingStruct& encoding) {
	size_t inLen = 0;

	if (encoding.widechar) {
		inLen = wcslen(reinterpret_cast<const wchar_t*>(string));
	} else {
		inLen = strlen(string);
	}

	return StringFromEncodedData(string, inLen, encoding.encoding);
}

String StringFromCString(const char* string)
{
	return StringFromEncodedData(string, core->TLKEncoding);
}

String StringFromResRef(const ResRef& string)
{
	return StringFromEncodedData(string.c_str(), ResRef::Size, core->TLKEncoding.encoding);
}

String StringFromUtf8(const char* string)
{
	EncodingStruct enc;
	enc.encoding = "UTF-8";
	enc.multibyte = true;
	return StringFromEncodedData(string, enc);
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

	size_t ret = iconv(cd, &in, &bytesLength, &outBuf, &outLenLeft);
	iconv_close(cd);

	if (ret == static_cast<size_t>(-1)) {
		Log(ERROR, "String", "iconv failed to convert string a string from UTF-16 to {} with error: {}", encoding, strerror(errno));
		return "";
	}

	return buffer;
}

static std::string RecodedStringFromString(const String& string, const std::string& encoding) {
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

std::string TLKStringFromString(const String& string) {
	auto newString = RecodedStringFromString(string, core->TLKEncoding.encoding);

	auto zero = newString.find('\0');
	if (zero != decltype(newString)::npos) {
		newString.resize(zero);
	}

	return newString;
}

}
