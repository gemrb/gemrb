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

#include "Strings/String.h"
#include "Strings/StringConversion.h"

#include <gtest/gtest.h>

namespace GemRB {

TEST(StringTest, StringFromView)
{
	String input { u"abc" };
	auto result = StringFromView<String>(StringViewT<String> { input });
	EXPECT_EQ(result, input);
}

TEST(StringTest, FindFirstOf)
{
	auto result = FindFirstOf(String {}, StringViewT<String> {});
	EXPECT_EQ(result, String::npos);

	result = FindFirstOf(String { u"abc" }, StringViewT<String> {}, 7);
	EXPECT_EQ(result, String::npos);

	result = FindFirstOf(String { u"abc" }, StringViewT<String> { u"cd" });
	EXPECT_EQ(result, size_t(2));

	result = FindFirstOf(String { u"abcd" }, StringViewT<String> { u"c" }, 3);
	EXPECT_EQ(result, String::npos);

	result = FindFirstOf(String { u"abc" }, StringViewT<String> { u"de" });
	EXPECT_EQ(result, String::npos);
}

TEST(StringTest, FindNotOf)
{
	String unit { u"abc" };

	auto result = FindNotOf<String>(unit.cbegin(), unit.cend(), StringViewT<String> { u"def" });
	EXPECT_EQ(result, unit.cbegin());

	result = FindNotOf<String>(unit.cbegin(), unit.cend(), StringViewT<String> { u"abc" });
	EXPECT_EQ(result, unit.cend());

	result = FindNotOf<String>(unit.cbegin(), unit.cend(), StringViewT<String> { u"acd" });
	EXPECT_EQ(result, unit.cbegin() + 1);

	result = FindNotOf<String>(unit.cbegin(), unit.cend(), StringViewT<String> {});
	EXPECT_EQ(result, unit.cbegin());
}

TEST(StringTest, FindFirstNotOf)
{
	auto result = FindFirstNotOf(String {}, StringViewT<String> {});
	EXPECT_EQ(result, String::npos);

	result = FindFirstNotOf(String {}, StringViewT<String> {}, String::npos);
	EXPECT_EQ(result, String::npos);

	result = FindFirstNotOf(String { u"abc" }, StringViewT<String> {}, 7);
	EXPECT_EQ(result, String::npos);

	result = FindFirstNotOf(String { u"abc" }, StringViewT<String> { u"ba" });
	EXPECT_EQ(result, 2);

	result = FindFirstNotOf(String { u"abc" }, StringViewT<String> { u"abc" });
	EXPECT_EQ(result, String::npos);

	result = FindFirstNotOf(String { u"abc" }, StringViewT<String> { u"def" }, 1);
	EXPECT_EQ(result, 1);
}

TEST(StringTest, FindLastNotOf)
{
	auto result = FindLastNotOf(String {}, StringViewT<String> {});
	EXPECT_EQ(result, String::npos);

	result = FindLastNotOf(String {}, StringViewT<String> {}, 0);
	EXPECT_EQ(result, static_cast<String::size_type>(-1));

	result = FindLastNotOf(String { u"abc" }, StringViewT<String> {}, 5);
	EXPECT_EQ(result, 2);

	result = FindLastNotOf(String { u"abc" }, StringViewT<String> {}, 1);
	EXPECT_EQ(result, 1);

	result = FindLastNotOf(String { u"abc" }, StringViewT<String> { u"def" });
	EXPECT_EQ(result, 2);

	result = FindLastNotOf(String { u"abc" }, StringViewT<String> { u"cb" });
	EXPECT_EQ(result, 0);

	result = FindLastNotOf(String { u"abca" }, StringViewT<String> { u"cb" });
	EXPECT_EQ(result, 3);

	result = FindLastNotOf(String { u"abc" }, StringViewT<String> { u"abc" });
	EXPECT_EQ(result, String::npos);
}

TEST(StringTest, FindLastNotOfReverse)
{
	auto result = FindLastNotOf(String { u"abc" }, StringViewT<String> { u"def" }, String::npos, true);
	EXPECT_EQ(result, 2);

	result = FindLastNotOf(String { u"abc" }, StringViewT<String> { u"cb" }, 0, true);
	EXPECT_EQ(result, 0);

	result = FindLastNotOf(String { u"abca" }, StringViewT<String> { u"cb" }, 0, true);
	EXPECT_EQ(result, 3);

	result = FindLastNotOf(String { u"abc" }, StringViewT<String> { u"abc" }, 0, true);
	EXPECT_EQ(result, String::npos);

	result = FindLastNotOf(String { u"abc" }, StringViewT<String> { u"cb" }, 1, true);
	EXPECT_EQ(result, String::npos);

	result = FindLastNotOf(String { u"abca" }, StringViewT<String> { u"cb" }, 1, true);
	EXPECT_EQ(result, 3);

	result = FindLastNotOf(String { u"abc" }, StringViewT<String> { u"abc" }, 1, true);
	EXPECT_EQ(result, String::npos);
}

TEST(StringTest, RTrim)
{
	String testString { u"space  " };
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

TEST(StringTest, RTrimCopy)
{
	String testString { u"space  " };
	EXPECT_EQ(RTrimCopy(testString), u"space");

	testString = u"\n\n\n";
	EXPECT_EQ(RTrimCopy(testString), u"");

	testString = u"space ltd";
	EXPECT_EQ(RTrimCopy(testString), u"space ltd");

	EXPECT_EQ((RTrimCopy(testString, u"xd")), u"space lt");
}

TEST(StringTest, LTrim)
{
	String testString { u"  space" };
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

TEST(StringTest, TrimString)
{
	String testString { u"  space  " };
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

TEST(StringTest, Explode)
{
	String a { u"a" };
	String b { u"b" };
	String c { u"c" };
	String bc { u"b,c" };
	std::vector<StringViewT<String>> expectedList { a, b, c };
	std::vector<StringViewT<String>> emptyList;

	auto input = String { u"a,b,c" };
	auto result = Explode(input, u',');
	EXPECT_EQ(result, expectedList);

	std::vector<StringViewT<String>> shorterList { a, bc };
	result = Explode(input, u',', 1);
	EXPECT_EQ(result, shorterList);

	input = String { u"a/b/c" };
	result = Explode(input, u'/');
	EXPECT_EQ(result, expectedList);

	input = String { u"   a, b, c  " };
	result = Explode(input, u',');
	EXPECT_EQ(result, expectedList);

	input = String { u"a b c " };
	result = Explode(input, u' ');
	expectedList.emplace_back();
	EXPECT_EQ(result, expectedList);
	expectedList.pop_back();

	input = String {};
	result = Explode(input);
	EXPECT_EQ(result, emptyList);

	input = String { u"a/b/c" };
	emptyList.emplace_back(input);
	result = Explode(input);
	EXPECT_EQ(result, emptyList);

	input = String { u"," };
	emptyList.clear();
	emptyList.emplace_back();
	emptyList.emplace_back();
	result = Explode(input);
	EXPECT_EQ(result, emptyList);

	input = String { u",," };
	emptyList.emplace_back();
	result = Explode(input);
	EXPECT_EQ(result, emptyList);

	input = String { u",,," };
	emptyList.emplace_back();
	result = Explode(input);
	EXPECT_EQ(result, emptyList);

	input = String { u" ,, , " };
	result = Explode(input);
	EXPECT_EQ(result, emptyList);
}

TEST(StringTest, SubStr)
{
	String unit { u"some string" };

	auto result = SubStr(unit, 0);
	EXPECT_EQ(result, StringViewT<String> { unit });

	result = SubStr(unit, 1);
	EXPECT_EQ(result, StringViewT<String> { String { u"ome string" } });

	result = SubStr(unit, 1, 2);
	EXPECT_EQ(result, StringViewT<String> { String { u"om" } });
}

TEST(StringDeathTest, Substr)
{
	String unit { u"some string" };

#ifdef _MSC_VER
	GTEST_SKIP() << "Not working on native Windows.";
#else
	ASSERT_DEATH_IF_SUPPORTED({ SubStr(unit, 10, 20); }, "");
#endif
}

TEST(StringTest, AppendFormat)
{
	std::string unit { "msg: " };
	AppendFormat(unit, "{:02} {}", 2, "failures");

	EXPECT_EQ(unit, std::string { "msg: 02 failures" });
}

// The underlying code has no explicit references to locales, so we only check
// for ASCII cases.

TEST(StringTest, StringToLower)
{
	String unit { u" StR.iNG" };
	StringToLower(unit);
	EXPECT_EQ(unit, u" str.ing");
}

TEST(StringTest, StringToUpper)
{
	String unit { u" StR.iNG" };
	StringToUpper(unit);
	EXPECT_EQ(unit, u" STR.ING");

	// cyrilic
	// mac runners leave it lowercase, appveyor mangles it completely
#if !defined(__APPLE__) && !defined(_MSC_VER)
	String unit2 { u"Закончить" };
	StringToUpper(unit2);
	EXPECT_EQ(unit2, u"ЗАКОНЧИТЬ");
#endif
}

TEST(StringTest, RecodedStringFromWideStringBytes)
{
	std::u16string original { u"abc" };
	auto utf8 = RecodedStringFromWideStringBytes(reinterpret_cast<const char16_t*>(original.c_str()), 6, "UTF-8");
	EXPECT_EQ(size_t(3), utf8.length());
	EXPECT_EQ("abc", utf8);
}

}
