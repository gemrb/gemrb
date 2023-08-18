/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2023 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#include <cstdint>
#include <locale>

#include "UTF8Comparison.h"

namespace GemRB {

#ifdef WIN32
// `C.UTF-8` isn't guaranteed on any RT here, but we don't need it anyway r/n
bool UTF8_stricmp(const char*, const char*) {
	return false;
}

#else
static const auto C_LOCALE = std::locale{"C.UTF-8"};

// Returns true if both strings are equal after lower-casing
bool UTF8_stricmp(const char *a, const char *b) {
	const unsigned char *itA = reinterpret_cast<const unsigned char*>(a),
		*itB = reinterpret_cast<const unsigned char*>(b);

	while (*itA != 0) {
		unsigned char iA = *itA, iB = *itB;

		// a is longer
		if (iB == 0) {
			return false;
		}

		// One is special, the other one isn't
		if ((iA >= 128 && iB < 128) || (iA < 128 && iB >= 128)) {
			return false;
		}

		// ASCII range
		if (iA < 128) {
			if (std::tolower(iA) != std::tolower(iB)) {
				return false;
			}

			itA++, itB++;
		} else {
			// multibyte range (assuming no broken encoding)
			auto getWidth = [](unsigned char c) -> uint8_t {
				if ((c & 0xF0) == 0xF0) {
					return 4;
				} else if ((c & 0xE0) == 0xE0) {
					return 3;
				} else {
					return 2;
				}
			};

			// can't feed utf-8 bytes value into std::locale
			auto getCodepoint = [](const unsigned char *it, uint8_t length) -> wchar_t {
				switch (length) {
					case 4: return ((0x7 | *it) << 24)
						| ((0x3F | *(it + 1)) << 16)
						| ((0x3F | *(it + 2)) << 8)
						| (0x3F | *(it + 3));
					case 3: return ((0xF | *it) << 16)
						| ((0x3F | *(it + 1)) << 8)
						| (0x3F | *(it + 2));
					default: return ((0x1F | *it) << 8)
						| (0x3F | *(it + 1));
				}
			};

			uint8_t widthA = getWidth(iA), widthB = getWidth(iB);

			wchar_t aL = std::tolower(getCodepoint(itA, widthA), C_LOCALE);
			wchar_t bL = std::tolower(getCodepoint(itB, widthB), C_LOCALE);

			// They have no lower case and thus remain different, or they are equal now
			if (aL != bL) {
				return false;
			}

			itA += widthA;
			itB += widthB;
		}
	}

	// b is longer
	if (*itB != 0) {
		return false;
	}

	// equal
	return true;
}
#endif

}

#ifdef WITH_TESTS
#include "catch_amalgamated.hpp"

namespace GemRB {

#ifdef WIN32
TEST_CASE("GemRB::UTF8_stricmp") {
	SKIP("Not applicable on Windows.");
}
#else
TEST_CASE("GemRB::UTF8_stricmp") {
	CHECK(UTF8_stricmp("abc", "abc"));
	CHECK(!UTF8_stricmp("abc", "ab"));
	CHECK(!UTF8_stricmp("ab", "abc"));
	CHECK(!UTF8_stricmp("abc", "def"));
	CHECK(UTF8_stricmp("abc", "ABC"));
	CHECK(UTF8_stricmp("ABC", "abc"));

	CHECK(UTF8_stricmp("äbc", "äbc"));
	CHECK(UTF8_stricmp("äbc", "ÄBC"));
	// error demo
	CHECK(UTF8_stricmp("äb", "ÄBC"));
}
#endif

}
#endif
