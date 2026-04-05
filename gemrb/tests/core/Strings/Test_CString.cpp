// SPDX-FileCopyrightText: 2023 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "ie_types.h"

#include "Strings/CString.h"

#include <gtest/gtest.h>

namespace GemRB {

TEST(CStringTest, IsASCII)
{
	auto unit0 = FixedSizeString<0> { "" };
	EXPECT_TRUE(unit0.IsASCII());

	auto unit3 = FixedSizeString<3> { "abc" };
	EXPECT_TRUE(unit3.IsASCII());

	unit3 = FixedSizeString<3> { "äöü" };
	EXPECT_FALSE(unit3.IsASCII());

	unit3 = FixedSizeString<3> { "ab\xFE" };
	EXPECT_FALSE(unit3.IsASCII());
}

TEST(CStringTest, Format)
{
	auto src = FixedSizeString<8> { "spwi103" };
	auto fu = FixedSizeString<8> { "" };
	fu.Format("{}", src);
	EXPECT_STREQ(src.c_str(), fu.c_str());

	fu.clear();
	EXPECT_TRUE(fu.empty());

	fu.Format("{:.4}", src);
	EXPECT_STREQ(fu.c_str(), "spwi");
	EXPECT_EQ(fu.length(), 4);

	// now try with ResRef directly
	auto src2 = ResRef { "spwi103" };
	auto fu2 = ResRef { "" };
	fu2.Format("{}", src2);
	EXPECT_STREQ(src2.c_str(), fu2.c_str());

	fu2.clear();
	EXPECT_TRUE(fu2.empty());

	fu2.Format("{:.4}", src2);
	EXPECT_STREQ(fu2.c_str(), "spwi");
	EXPECT_EQ(fu2.length(), 4);
}

}
