// SPDX-FileCopyrightText: 2023 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "Strings/StringView.h"

#include <gtest/gtest.h>

namespace GemRB {

TEST(StringViewTest, Equality)
{
	EXPECT_EQ(StringView { "abc" }, StringView { "abc" });
	EXPECT_EQ(StringView {}, StringView {});

	EXPECT_NE(StringView { "abc" }, StringView { "abcd" });
	EXPECT_NE(StringView { "123" }, StringView { "abc" });
}

TEST(StringViewTest, clear)
{
	StringView unit { "abc" };
	unit.clear();

	EXPECT_EQ(unit.length(), size_t(0));
	EXPECT_TRUE(unit.empty());
}

TEST(StringViewTest, erase)
{
	StringView unit { "abcd" };
	unit.erase(3);

	EXPECT_EQ(unit.length(), size_t(3));
	EXPECT_EQ(unit, StringView { "abc" });
}

}
