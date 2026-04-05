// SPDX-FileCopyrightText: 2023 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "Strings/UTF8Comparison.h"

#include <gtest/gtest.h>

namespace GemRB {

#ifdef WIN32

TEST(UTF8ComparisonTest, UTF8stricmpEquality)
{
	GTEST_SKIP() << "Not applicable to Windows.";
}

#else

TEST(UTF8ComparisonTest, UTF8stricmpEquality)
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
