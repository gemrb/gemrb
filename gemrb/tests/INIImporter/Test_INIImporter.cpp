/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2023 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#include "../../core/Streams/FileStream.h"
#include "../../plugins/INIImporter/INIImporter.h"

#include <gtest/gtest.h>

namespace GemRB {

static const path_t SAMPLE_FILE = PathJoin("tests", "resources", "INIImporter", "sample.ini");

class INIImporter_Test : public testing::Test {
protected:
	INIImporter unit;

public:
	void SetUp() override
	{
		auto stream = new FileStream {};

		assert(stream->Open(SAMPLE_FILE));
		assert(unit.Open(std::unique_ptr<DataStream> { stream }));
	}
};

TEST_F(INIImporter_Test, GetTagsCount)
{
	EXPECT_EQ(unit.GetTagsCount(), 4);
}

TEST_F(INIImporter_Test, GroupIteration)
{
	auto it = unit.begin();

	EXPECT_EQ("sectionA", (*it++).GetName());
	EXPECT_EQ("sectionB", (*it++).GetName());
	EXPECT_EQ("sectionC", (*it++).GetName());
	EXPECT_EQ("sectionC", (*it++).GetName());
	EXPECT_EQ(unit.end(), it);
}

TEST_F(INIImporter_Test, Find)
{
	auto it = unit.find("sectionC");
	auto it2 = unit.find("Sectionc");
	EXPECT_EQ(it, it2);

	EXPECT_EQ("sectionC", it->GetName());
}

TEST_F(INIImporter_Test, KeyValueIteration)
{
	auto it = unit.begin();
	std::unordered_map<std::string, std::string> values;

	for (const auto& p : *it) {
		values[static_cast<StringView>(p.first).MakeString()] = p.second;
	}

	EXPECT_EQ(5, values.size());
	EXPECT_EQ("abc", values.find("stringValue")->second);
	EXPECT_EQ("1", values.find("intValue")->second);
	EXPECT_EQ("2.5", values.find("floatValue")->second);
	EXPECT_EQ("true", values.find("trueValue")->second);
	EXPECT_EQ("FALSE", values.find("falseValue")->second);
}

TEST_F(INIImporter_Test, GetKeyAsString)
{
	auto sectionA = StringView { "SectionA" };

	EXPECT_EQ(StringView { "abc" }, (unit.GetKeyAsString(sectionA, StringView { "stringValue" })));
	EXPECT_EQ(StringView { "abc" }, (unit.GetKeyAsString(sectionA, StringView { "Stringvalue" })));
	EXPECT_EQ(StringView { "n/a" }, (unit.GetKeyAsString(sectionA, StringView { "strongValue" }, StringView { "n/a" })));
	// last match
	EXPECT_EQ(StringView { "1" }, (unit.GetKeyAsString(sectionA, StringView { "intValue" })));
	EXPECT_EQ(StringView { "2.5" }, (unit.GetKeyAsString(sectionA, StringView { "floatValue" })));
	EXPECT_EQ(StringView { "true" }, (unit.GetKeyAsString(sectionA, StringView { "trueValue" })));

	EXPECT_EQ(StringView { "xyz" }, (unit.GetKeyAsString(StringView { "sectionB" }, StringView { "anotherStringValue" })));
	// match of first sec C
	EXPECT_EQ(StringView { "Here" }, (unit.GetKeyAsString(StringView { "sectionC" }, StringView { "iAm" })));
	EXPECT_EQ(StringView {}, (unit.GetKeyAsString(StringView { "sectionC" }, StringView { "2ndKey" })));
}

TEST_F(INIImporter_Test, GetKeyAsInt)
{
	auto sectionA = StringView { "SectionA" };

	EXPECT_EQ(1, (unit.GetKeyAsInt(sectionA, StringView { "intValue" }, 0)));
	EXPECT_EQ(1, (unit.GetKeyAsInt(sectionA, StringView { "Intvalue" }, 0)));
	EXPECT_EQ(456, (unit.GetKeyAsInt(sectionA, StringView { "intValueX" }, 456)));
	EXPECT_EQ(0, (unit.GetKeyAsInt(sectionA, StringView { "stringValue" }, 2)));
	EXPECT_EQ(2, (unit.GetKeyAsInt(sectionA, StringView { "floatValue" }, 0)));
}

TEST_F(INIImporter_Test, GetKeyAsFloat)
{
	auto sectionA = StringView { "SectionA" };

	EXPECT_EQ(2.5, (unit.GetKeyAsFloat(sectionA, StringView { "floatValue" }, 1.0)));
	EXPECT_EQ(2.5, (unit.GetKeyAsFloat(sectionA, StringView { "Floatvalue" }, 1.0)));
	EXPECT_EQ(1.5, (unit.GetKeyAsFloat(sectionA, StringView { "floatValueX" }, 1.5)));

	EXPECT_EQ(0.0, (unit.GetKeyAsFloat(sectionA, StringView { "stringValue" }, 2.0)));
	EXPECT_EQ(1.0, (unit.GetKeyAsFloat(sectionA, StringView { "intValue" }, 0.0)));
}

TEST_F(INIImporter_Test, GetKeyAsBool)
{
	auto sectionA = StringView { "SectionA" };

	EXPECT_TRUE(unit.GetKeyAsBool(sectionA, StringView { "trueValue" }, false));
	EXPECT_TRUE(unit.GetKeyAsBool(sectionA, StringView { "TRUEvalue" }, false));
	EXPECT_FALSE(unit.GetKeyAsBool(sectionA, StringView { "falseValue" }, true));
	EXPECT_TRUE(unit.GetKeyAsBool(sectionA, StringView { "ghost" }, true));

	EXPECT_TRUE(unit.GetKeyAsBool(sectionA, StringView { "intValue" }, false));
	EXPECT_TRUE(unit.GetKeyAsBool(sectionA, StringView { "floatValue" }, false));
	EXPECT_FALSE(unit.GetKeyAsBool(sectionA, StringView { "stringValue" }, true));
}

TEST_F(INIImporter_Test, GetAsAndGetAsXEquality)
{
	auto sectionA = StringView { "SectionA" };
	auto section = *unit.find(sectionA);

	EXPECT_EQ((section.GetAs<StringView>("stringValue", StringView { "no" })), (unit.GetKeyAsString(sectionA, StringView { "stringValue" })));
	EXPECT_EQ((section.GetAs<int>("intValue", 0)), (unit.GetKeyAsInt(sectionA, StringView { "intValue" }, 4)));
	EXPECT_EQ((section.GetAs<float>("floatValue", 0.0)), (unit.GetKeyAsFloat(sectionA, StringView { "floatValue" }, 17.0)));
	EXPECT_EQ((section.GetAs<bool>("trueValue", false)), (unit.GetKeyAsBool(sectionA, StringView { "trueValue" }, false)));
}

}
