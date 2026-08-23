// SPDX-FileCopyrightText: 2023 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "Strings/String.h"
#include "Strings/StringConversion.h"

#include <array>
#include <deque>
#include <gtest/gtest.h>
#include <set>

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
	EXPECT_EQ(result, size_t(2));

	result = FindFirstNotOf(String { u"abc" }, StringViewT<String> { u"abc" });
	EXPECT_EQ(result, String::npos);

	result = FindFirstNotOf(String { u"abc" }, StringViewT<String> { u"def" }, 1);
	EXPECT_EQ(result, size_t(1));
}

TEST(StringTest, FindLastNotOf)
{
	auto result = FindLastNotOf(String {}, StringViewT<String> {});
	EXPECT_EQ(result, String::npos);

	result = FindLastNotOf(String {}, StringViewT<String> {}, 0);
	EXPECT_EQ(result, static_cast<String::size_type>(-1));

	result = FindLastNotOf(String { u"abc" }, StringViewT<String> {}, 5);
	EXPECT_EQ(result, size_t(2));

	result = FindLastNotOf(String { u"abc" }, StringViewT<String> {}, 1);
	EXPECT_EQ(result, size_t(1));

	result = FindLastNotOf(String { u"abc" }, StringViewT<String> { u"def" });
	EXPECT_EQ(result, size_t(2));

	result = FindLastNotOf(String { u"abc" }, StringViewT<String> { u"cb" });
	EXPECT_EQ(result, size_t(0));

	result = FindLastNotOf(String { u"abca" }, StringViewT<String> { u"cb" });
	EXPECT_EQ(result, size_t(3));

	result = FindLastNotOf(String { u"abc" }, StringViewT<String> { u"abc" });
	EXPECT_EQ(result, String::npos);
}

TEST(StringTest, FindLastNotOfReverse)
{
	auto result = FindLastNotOf(String { u"abc" }, StringViewT<String> { u"def" }, String::npos, true);
	EXPECT_EQ(result, size_t(2));

	result = FindLastNotOf(String { u"abc" }, StringViewT<String> { u"cb" }, 0, true);
	EXPECT_EQ(result, size_t(0));

	result = FindLastNotOf(String { u"abca" }, StringViewT<String> { u"cb" }, 0, true);
	EXPECT_EQ(result, size_t(3));

	result = FindLastNotOf(String { u"abc" }, StringViewT<String> { u"abc" }, 0, true);
	EXPECT_EQ(result, String::npos);

	result = FindLastNotOf(String { u"abc" }, StringViewT<String> { u"cb" }, 1, true);
	EXPECT_EQ(result, String::npos);

	result = FindLastNotOf(String { u"abca" }, StringViewT<String> { u"cb" }, 1, true);
	EXPECT_EQ(result, size_t(3));

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

TEST(StringTest, AsRanges)
{
	EXPECT_EQ(AsRanges(std::vector<int> {}), "");
	EXPECT_EQ(AsRanges(std::vector<int> { 5 }), "5");
	EXPECT_EQ(AsRanges(std::vector<int> { 0, 1 }), "0-1");
	EXPECT_EQ(AsRanges(std::vector<int> { 0, 2 }), "0, 2");
	EXPECT_EQ(AsRanges(std::vector<int> { 0, 1, 2, 7, 9, 10 }), "0-2, 7, 9-10");
	EXPECT_EQ(AsRanges(std::vector<int> { -3, -2, -1, 4 }), "-3--1, 4");
}

TEST(StringTest, AsRangesElementTypes)
{
	EXPECT_EQ(AsRanges(std::vector<size_t> { 1, 2, 3 }), "1-3");
	EXPECT_EQ(AsRanges(std::vector<int64_t> { 100, 101 }), "100-101");
	// the promotion in `b != a + 1` keeps this from wrapping at the top of the type
	EXPECT_EQ(AsRanges(std::vector<uint8_t> { 250, 251, 255 }), "250-251, 255");
}

TEST(StringTest, AsRangesContainers)
{
	EXPECT_EQ(AsRanges(std::array<int, 4> { 1, 2, 5, 6 }), "1-2, 5-6");
	// no random access, so only std::next and std::prev may be used on the iterators
	EXPECT_EQ(AsRanges(std::deque<int> { 8, 9 }), "8-9");
	EXPECT_EQ(AsRanges(std::set<int> { 4, 1, 2, 3 }), "1-4");

	const int raw[] = { 3, 4, 5 };
	EXPECT_EQ(AsRanges(std::begin(raw), std::end(raw)), "3-5");
}

TEST(StringTest, AsRangesIteratorPairAndSeparator)
{
	const std::vector<int> v { 1, 2, 9 };
	EXPECT_EQ(AsRanges(v.cbegin(), v.cend()), "1-2, 9");
	EXPECT_EQ(AsRanges(v.cbegin() + 1, v.cend()), "2, 9");
	EXPECT_EQ(AsRanges(v, " | "), "1-2 | 9");
}
}
