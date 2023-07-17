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

static String* StringFromEncodedData(const ieByte* string, const EncodingStruct& encoded)
{
	if (!string) return NULL;

	bool convert = encoded.widechar || encoded.multibyte;
	// assert that its something we know how to handle
	// TODO: add support for other encodings?
	assert(!convert || (encoded.widechar || encoded.encoding == "UTF-8"));

	size_t len = strlen((const char*) string);
	String* dbString = new String();
	dbString->reserve(len);
	size_t dbLen = 0;
	for(size_t i=0; i<len; ++i) {
		ieWord currentChr = string[i];
		// we are assuming that every multibyte encoding uses single bytes for chars 32 - 127
		if(convert && (i+1 < len) && (currentChr >= 128 || currentChr < 32) && (currentChr != '\n')) {
			// this is a double byte char, or a multibyte sequence
			ieWord ch = 0;
			if (encoded.encoding == "UTF-8") {
				size_t nb = 0;
				if (currentChr >= 0xC0 && currentChr <= 0xDF) {
					/* c0-df are first byte of two-byte sequences (5+6=11 bits) */
					/* c0-c1 are noncanonical */
					nb = 2;
				} else if (currentChr >= 0xE0 && currentChr <= 0XEF) {
					/* e0-ef are first byte of three-byte (4+6+6=16 bits) */
					/* e0 80-9f are noncanonical */
					nb = 3;
				} else if (currentChr >= 0xF0 && currentChr <= 0XF7) {
					/* f0-f7 are first byte of four-byte (3+6+6+6=21 bits) */
					/* f0 80-8f are noncanonical */
					nb = 4;
				} else if (currentChr >= 0xF8 && currentChr <= 0XFB) {
					/* f8-fb are first byte of five-byte (2+6+6+6+6=26 bits) */
					/* f8 80-87 are noncanonical */
					nb = 5;
				} else if (currentChr >= 0xFC && currentChr <= 0XFD) {
					/* fc-fd are first byte of six-byte (1+6+6+6+6+6=31 bits) */
					/* fc 80-83 are noncanonical */
					nb = 6;
				} else {
					Log(WARNING, "String", "Invalid UTF-8 character: {:#x}", currentChr);
					continue;
				}

				ch = currentChr & ((1 << (7 - nb)) - 1);
				while (--nb) {
					ch <<= 6;
					ch |= string[++i] & 0x3f;
				}
			} else {
				ch = (string[++i] << 8) + currentChr;
			}
			dbString->push_back(ch);
		} else {
			dbString->push_back(currentChr);
		}
		++dbLen;
	}

	// we dont always use everything we allocated.
	// realloc in this case to avoid static anylizer warnings about "garbage values"
	// since this realloc always truncates it *should* be quick
	dbString->resize(dbLen);
	return dbString;
}

// returns new string converted to given encoding
// in case iconv is not available, requested encoding is the same as string encoding,
// or there is encoding error, copy of original string is returned.
char* ConvertCharEncoding(const char* string, const char* from, const char* to) {
	if (strcmp(from, to) == 0) {
		return strdup(string);
	}

	iconv_t cd = iconv_open(to, from);
	if (cd == (iconv_t)-1) {
		Log(ERROR, "String", "iconv_open({}, {}) failed with error: {}", to, from, strerror(errno));
		return strdup(string);
	}


	char * in = (char *) string;
	size_t in_len = strlen(string);
	size_t out_len = (in_len + 1) * 4;
	size_t out_len_left = out_len;
	char* buf = (char*) malloc(out_len);
	char* buf_out = buf;
	size_t ret = iconv(cd, &in, &in_len, &buf_out, &out_len_left);
	iconv_close(cd);

	if (ret == (size_t)-1) {
		Log(ERROR, "String", "iconv failed to convert string {} from {} to {} with error: {}", string, from, to, strerror(errno));
		free(buf);
		return strdup(string);
	}

	size_t used = out_len - out_len_left;
	buf = (char*)realloc(buf, used + 1);
	buf[used] = '\0';
	return buf;
}

String* StringFromCString(const char* string)
{
	// if multibyte is false this is basic expansion of cstring to wchar_t
	// the only reason this is special, is because it allows characters 128-256.
	return StringFromEncodedData((const ieByte*) string, core->TLKEncoding);
}

String* StringFromUtf8(const char* string)
{
	EncodingStruct enc;
	enc.encoding = "UTF-8";
	enc.multibyte = true;
	return StringFromEncodedData((const ieByte*) string, enc);
}

std::string MBStringFromString(const String& string)
{
	std::string ret(string.length() * 2, '\0');

	// FIXME: depends on locale setting
	// FIXME: currently assumes a character-character mapping (Unicode -> ASCII)
	size_t newlen = wcstombs(&ret[0], string.c_str(), ret.capacity());

	if (newlen == static_cast<size_t>(-1)) {
		// invalid multibyte sequence
		Log(ERROR, "String", "wcstombs failed to covert string. error: {}\n@:", strerror(errno), ret);
		return ret;
	}
	assert(newlen <= ret.length());
	ret.resize(newlen);

	return ret;
}

}
