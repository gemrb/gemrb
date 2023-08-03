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

static String StringFromEncodedData(const ieByte* string, const EncodingStruct& encoded)
{
	if (!string) return u"";

	char* in = reinterpret_cast<char*>(const_cast<ieByte*>(string));
	size_t inLen = 0;

	if (encoded.widechar) {
		inLen = wcslen(reinterpret_cast<const wchar_t*>(in));
	} else {
		inLen = strlen(in);
	}

	if (inLen == 0) {
		return u"";
	}

	iconv_t cd = nullptr;
	// Must check since UTF-16 may fall back to UTF-16BE, even if platform is LE
	if (IsBigEndian()) {
		cd = iconv_open("UTF-16BE", encoded.encoding.c_str());
	} else {
		cd = iconv_open("UTF-16LE", encoded.encoding.c_str());
	}

	if (cd == (iconv_t)-1) {
		Log(ERROR, "String", "iconv_open(UTF-16, {}) failed with error: {}", encoded.encoding, strerror(errno));
		return u"";
	}

	size_t outLen = inLen * 4;
	size_t outLenLeft = outLen;
	std::u16string buffer(inLen * 2, u'\0');
	auto outBuf = reinterpret_cast<char*>(const_cast<char16_t*>(buffer.data()));

	size_t ret = iconv(cd, &in, &inLen, &outBuf, &outLenLeft);
	iconv_close(cd);

	if (ret == static_cast<size_t>(-1)) {
		Log(ERROR, "String", "iconv failed to convert string {} from {} to UTF-16 with error: {}", reinterpret_cast<const char*>(string), encoded.encoding, strerror(errno));
		return u"";
	}

	auto zero = buffer.find(u'\0');
	if (zero != decltype(buffer)::npos) {
		buffer.resize(zero);
	}

	return buffer;
}

String StringFromCString(const char* string)
{
	return StringFromEncodedData((const ieByte*) string, core->TLKEncoding);
}

String StringFromFSString(const char* string)
{
	EncodingStruct enc;
	enc.encoding = core->config.SystemEncoding.c_str();
	enc.multibyte = true; // nobody ever complained so far
	return StringFromEncodedData((const ieByte*) string, enc);
}

String StringFromUtf8(const char* string)
{
	EncodingStruct enc;
	enc.encoding = "UTF-8";
	enc.multibyte = true;
	return StringFromEncodedData((const ieByte*) string, enc);
}

std::string MBStringFromString(const String& string)
{
	char* in = reinterpret_cast<char*>(const_cast<char16_t*>(string.c_str()));
	size_t inLen = string.length() * sizeof(char16_t);

	if (inLen == 0) {
		return "";
	}

	iconv_t cd = nullptr;
	if (IsBigEndian()) {
		cd = iconv_open("UTF-8", "UTF-16BE");
	} else {
		cd = iconv_open("UTF-8", "UTF-16LE");
	}

	size_t outLen = string.length() * 4;
	size_t outLenLeft = outLen;
	std::string buffer(outLen, '\0');
	auto outBuf = const_cast<char*>(buffer.data());

	size_t ret = iconv(cd, &in, &inLen, &outBuf, &outLenLeft);
	iconv_close(cd);

	if (ret == static_cast<size_t>(-1)) {
		Log(ERROR, "String", "iconv failed to convert string a string from UTF-16 to UTF-8 with error: {}", strerror(errno));
		return "";
	}

	auto zero = buffer.find('\0');
	if (zero != decltype(buffer)::npos) {
		buffer.resize(zero);
	}

	return buffer;
}

}
