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

#include "Strings/UTF8Comparison.h"

#include <gtest/gtest.h>

namespace GemRB {

#ifdef WIN32

TEST(UTF8Comparison_Test, UTF8_stricmp_Equality)
{
	GTEST_SKIP() << "Not applicable to Windows.";
}

#else

TEST(UTF8Comparison_Test, UTF8_stricmp_Equality)
{
	EXPECT_TRUE(UTF8_stricmp("abc", "abc"));
	EXPECT_FALSE(UTF8_stricmp("abc", "ab"));
	EXPECT_FALSE(UTF8_stricmp("ab", "abc"));
	EXPECT_FALSE(UTF8_stricmp("abc", "def"));
	EXPECT_TRUE(UTF8_stricmp("abc", "ABC"));
	EXPECT_TRUE(UTF8_stricmp("ABC", "abc"));

	EXPECT_TRUE(UTF8_stricmp("äbc", "äbc"));
	EXPECT_TRUE(UTF8_stricmp("äbc", "ÄBC"));
	EXPECT_TRUE(UTF8_stricmp("", ""));

	EXPECT_FALSE(UTF8_stricmp("abc", nullptr));
	EXPECT_FALSE(UTF8_stricmp(nullptr, "abc"));
	// no philosophical thoughts here
	EXPECT_TRUE(UTF8_stricmp(nullptr, nullptr));
}

#endif

}
