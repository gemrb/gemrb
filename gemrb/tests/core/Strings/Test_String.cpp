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

#include <gtest/gtest.h>

#include "Strings/String.h"

namespace GemRB {

TEST(String_Test, StringFromView) {
	String input{u"abc"};
	auto result = StringFromView<String>(StringViewT<String>{input});
	EXPECT_EQ(result, input);
}

TEST(String_Test, FindFirstOf) {
	auto result = FindFirstOf(String{}, StringViewT<String>{});
	EXPECT_EQ(result, String::npos);

	result = FindFirstOf(String{u"abc"}, StringViewT<String>{}, 7);
	EXPECT_EQ(result, String::npos);

	result = FindFirstOf(String{u"abc"}, StringViewT<String>{u"cd"});
	EXPECT_EQ(result, 2);

	result = FindFirstOf(String{u"abcd"}, StringViewT<String>{u"c"}, 3);
	EXPECT_EQ(result, String::npos);

	result = FindFirstOf(String{u"abc"}, StringViewT<String>{u"de"});
	EXPECT_EQ(result, String::npos);
}

TEST(String_Test, FindNotOf) {
	String unit{u"abc"};

	auto result = FindNotOf<String>(unit.cbegin(), unit.cend(), StringViewT<String>{u"def"});
	EXPECT_EQ(result, unit.cbegin());

	result = FindNotOf<String>(unit.cbegin(), unit.cend(), StringViewT<String>{u"abc"});
	EXPECT_EQ(result, unit.cend());

	result = FindNotOf<String>(unit.cbegin(), unit.cend(), StringViewT<String>{u"acd"});
	EXPECT_EQ(result, unit.cbegin() + 1);

	result = FindNotOf<String>(unit.cbegin(), unit.cend(), StringViewT<String>{});
	EXPECT_EQ(result, unit.cbegin());
}

TEST(String_Test, FindFirstNotOf) {
	auto result = FindFirstNotOf(String{}, StringViewT<String>{});
	EXPECT_EQ(result, String::npos);

	result = FindFirstNotOf(String{}, StringViewT<String>{}, String::npos);
	EXPECT_EQ(result, String::npos);

	result = FindFirstNotOf(String{u"abc"}, StringViewT<String>{}, 7);
	EXPECT_EQ(result, String::npos);

	result = FindFirstNotOf(String{u"abc"}, StringViewT<String>{u"ba"});
	EXPECT_EQ(result, 2);

	result = FindFirstNotOf(String{u"abc"}, StringViewT<String>{u"abc"});
	EXPECT_EQ(result, String::npos);

	result = FindFirstNotOf(String{u"abc"}, StringViewT<String>{u"def"}, 1);
	EXPECT_EQ(result, 1);
}

TEST(String_Test, FindLastNotOf) {
	auto result = FindLastNotOf(String{}, StringViewT<String>{});
	EXPECT_EQ(result, String::npos);

	result = FindLastNotOf(String{}, StringViewT<String>{}, 0);
	EXPECT_EQ(result, static_cast<String::size_type>(-1));

	result = FindLastNotOf(String{u"abc"}, StringViewT<String>{}, 5);
	EXPECT_EQ(result, 2);

	result = FindLastNotOf(String{u"abc"}, StringViewT<String>{}, 1);
	EXPECT_EQ(result, 1);

	result = FindLastNotOf(String{u"abc"}, StringViewT<String>{u"def"});
	EXPECT_EQ(result, 2);

	result = FindLastNotOf(String{u"abc"}, StringViewT<String>{u"cb"});
	EXPECT_EQ(result, 0);

	result = FindLastNotOf(String{u"abca"}, StringViewT<String>{u"cb"});
	EXPECT_EQ(result, 3);

	result = FindLastNotOf(String{u"abc"}, StringViewT<String>{u"abc"});
	EXPECT_EQ(result, String::npos);
}

TEST(String_Test, FindLastNotOf_Reverse) {
	auto result = FindLastNotOf(String{u"abc"}, StringViewT<String>{u"def"}, String::npos, true);
	EXPECT_EQ(result, 2);

	result = FindLastNotOf(String{u"abc"}, StringViewT<String>{u"cb"}, 0, true);
	EXPECT_EQ(result, 0);

	result = FindLastNotOf(String{u"abca"}, StringViewT<String>{u"cb"}, 0, true);
	EXPECT_EQ(result, 3);

	result = FindLastNotOf(String{u"abc"}, StringViewT<String>{u"abc"}, 0, true);
	EXPECT_EQ(result, String::npos);

	result = FindLastNotOf(String{u"abc"}, StringViewT<String>{u"cb"}, 1, true);
	EXPECT_EQ(result, String::npos);

	result = FindLastNotOf(String{u"abca"}, StringViewT<String>{u"cb"}, 1, true);
	EXPECT_EQ(result, 3);

	result = FindLastNotOf(String{u"abc"}, StringViewT<String>{u"abc"}, 1, true);
	EXPECT_EQ(result, String::npos);
}

TEST(String_Test, RTrim) {
	String testString{u"space  "};
	RTrim(testString);
	EXPECT_EQ(testString, u"space");

	testString = u"\n\n\n";
	RTrim(testString);
	EXPECT_EQ(testString, u"");

	testString = u"space ltd";
	RTrim(testString);
	EXPECT_EQ(testString, u"space ltd");

	RTrim(testString, u"xd");
	EXPECT_EQ(testString, u"space lt");
}

TEST(String_Test, RTrimCopy) {
	String testString{u"space  "};
	EXPECT_EQ(RTrimCopy(testString), u"space");

	testString = u"\n\n\n";
	EXPECT_EQ(RTrimCopy(testString), u"");

	testString = u"space ltd";
	EXPECT_EQ(RTrimCopy(testString), u"space ltd");

	EXPECT_EQ((RTrimCopy(testString, u"xd")), u"space lt");
}

TEST(String_Test, LTrim) {
	String testString{u"  space"};
	LTrim(testString);
	EXPECT_EQ(testString, u"space");

	testString = u"\n\n\n";
	LTrim(testString);
	EXPECT_EQ(testString, u"");

	testString = u"ltd space";
	LTrim(testString);
	EXPECT_EQ(testString, u"ltd space");

	LTrim(testString, u"xl");
	EXPECT_EQ(testString, u"td space");
}

TEST(String_Test, TrimString) {
	String testString{u"  space  "};
	TrimString(testString);
	EXPECT_EQ(testString, u"space");

	testString = u"\n\n\n";
	TrimString(testString);
	EXPECT_EQ(testString, u"");

	testString = u"ltd space ltd";
	TrimString(testString);
	EXPECT_EQ(testString, u"ltd space ltd");

	TrimString(testString, u"xld");
	EXPECT_EQ(testString, u"td space lt");
}

TEST(String_Test, Explode) {
	String a{u"a"};
	String b{u"b"};
	String c{u"c"};
	String bc{u"b,c"};
	std::vector<StringViewT<String>> expectedList{a, b, c};

	auto input = String{u"a,b,c"};
	auto result = Explode(input, u',');
	EXPECT_EQ(result, expectedList);

	std::vector<StringViewT<String>> shorterList{a, bc};
	result = Explode(input, u',', 1);
	EXPECT_EQ(result, shorterList);

	input = String{u"a/b/c"};
	result = Explode(input, u'/');
	EXPECT_EQ(result, expectedList);

	input = String{u"   a, b, c  "};
	result = Explode(input, u',');
	EXPECT_EQ(result, expectedList);
}

TEST(String_Test, SubStr) {
	String unit{u"some string"};

	auto result = SubStr(unit, 0);
	EXPECT_EQ(result, StringViewT<String>{unit});

	result = SubStr(unit, 1);
	EXPECT_EQ(result, StringViewT<String>{String{u"ome string"}});

	result = SubStr(unit, 1, 2);
	EXPECT_EQ(result, StringViewT<String>{String{u"om"}});
}

TEST(String_DeathTest, Substr) {
	String unit{u"some string"};

#ifdef _MSC_VER
	GTEST_SKIP() << "Not working on native Windows.";
#else
	ASSERT_DEATH_IF_SUPPORTED({ SubStr(unit, 10, 20); }, "");
#endif
}

TEST(String_Test, AppendFormat) {
	std::string unit{"msg: "};
	AppendFormat(unit, "{:02} {}", 2, "failures");

	EXPECT_EQ(unit, std::string{"msg: 02 failures"});
}

// The underlying code has no explicit references to locales, so we only check
// for ASCII cases.

TEST(String_Test, StringToLower) {
	String unit{u" StR.iNG"};
	StringToLower(unit);
	EXPECT_EQ(unit, u" str.ing");
}

TEST(String_Test, StringToUpper) {
	String unit{u" StR.iNG"};
	StringToUpper(unit);
	EXPECT_EQ(unit, u" STR.ING");
}

}
