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

#include <gtest/gtest.h>

#include "../../core/Streams/FileStream.h"
#include "../../plugins/INIImporter/INIImporter.h"

namespace GemRB {

static const path_t SAMPLE_FILE = PathJoin("tests", "resources", "INIImporter", "sample.ini");

class INIImporter_Test : public testing::Test {
protected:
	FileStream* stream;
	INIImporter unit;
public:
	void SetUp() override {
		this->stream = new FileStream{};

		assert(stream->Open(SAMPLE_FILE));
		assert(unit.Open(stream));
		// INIImporter deletes stream internally after use
		this->stream = nullptr;
	}
};

TEST_F(INIImporter_Test, GetTagsCount) {
	EXPECT_EQ(unit.GetTagsCount(), 4);
}

TEST_F(INIImporter_Test, GetTagNameByIndex) {
	EXPECT_EQ(StringView{"sectionA"}, unit.GetTagNameByIndex(0));
	EXPECT_EQ(StringView{"sectionB"}, unit.GetTagNameByIndex(1));
	EXPECT_EQ(StringView{"sectionC"}, unit.GetTagNameByIndex(2));
	EXPECT_EQ(StringView{"sectionC"}, unit.GetTagNameByIndex(3));

	EXPECT_EQ(StringView{}, unit.GetTagNameByIndex(200));
}

TEST_F(INIImporter_Test, GetKeysCount) {
	EXPECT_EQ(unit.GetKeysCount(StringView{"SectionA"}), 6);
	EXPECT_EQ(unit.GetKeysCount(StringView{"sectiona"}), 6);
	EXPECT_EQ(unit.GetKeysCount(StringView{"SectionB"}), 1);
	// first tag match
	EXPECT_EQ(unit.GetKeysCount(StringView{"SectionC"}), 1);

	EXPECT_EQ(unit.GetKeysCount(StringView{"nope"}), 0);
}

TEST_F(INIImporter_Test, GetKeyNameByIndex) {
	auto sectionA = StringView{"SectionA"};
	auto sectionB = StringView{"SectionB"};
	auto sectionC = StringView{"SectionC"};

	EXPECT_EQ(StringView{"stringValue"}, (unit.GetKeyNameByIndex(sectionA, 0)));
	EXPECT_EQ(StringView{"intValue"}, (unit.GetKeyNameByIndex(sectionA, 1)));
	EXPECT_EQ(StringView{"intValue"}, (unit.GetKeyNameByIndex(sectionA, 2)));
	EXPECT_EQ(StringView{"floatValue"}, (unit.GetKeyNameByIndex(sectionA, 3)));
	EXPECT_EQ(StringView{"trueValue"}, (unit.GetKeyNameByIndex(sectionA, 4)));
	EXPECT_EQ(StringView{"anotherStringValue"}, (unit.GetKeyNameByIndex(sectionB, 0)));
	EXPECT_EQ(StringView{"iAm"}, (unit.GetKeyNameByIndex(sectionC, 0)));

	EXPECT_EQ(StringView{}, (unit.GetKeyNameByIndex(sectionA, 100)));
	EXPECT_EQ(StringView{}, (unit.GetKeyNameByIndex(StringView{"section404"}, 0)));
}

TEST_F(INIImporter_Test, GetKeyAsString) {
	auto sectionA = StringView{"SectionA"};

	EXPECT_EQ(StringView{"abc"}, (unit.GetKeyAsString(sectionA, StringView{"stringValue"})));
	EXPECT_EQ(StringView{"abc"}, (unit.GetKeyAsString(sectionA, StringView{"Stringvalue"})));
	EXPECT_EQ(StringView{"n/a"}, (unit.GetKeyAsString(sectionA, StringView{"strongValue"}, StringView{"n/a"})));
	// first match
	EXPECT_EQ(StringView{"123"}, (unit.GetKeyAsString(sectionA, StringView{"intValue"})));
	EXPECT_EQ(StringView{"2.5"}, (unit.GetKeyAsString(sectionA, StringView{"floatValue"})));
	EXPECT_EQ(StringView{"true"}, (unit.GetKeyAsString(sectionA, StringView{"trueValue"})));

	EXPECT_EQ(StringView{"xyz"}, (unit.GetKeyAsString(StringView{"sectionB"}, StringView{"anotherStringValue"})));
}

TEST_F(INIImporter_Test, GetKeyAsInt) {
	auto sectionA = StringView{"SectionA"};

	EXPECT_EQ(123, (unit.GetKeyAsInt(sectionA, StringView{"intValue"}, 0)));
	EXPECT_EQ(123, (unit.GetKeyAsInt(sectionA, StringView{"Intvalue"}, 0)));
	EXPECT_EQ(456, (unit.GetKeyAsInt(sectionA, StringView{"intValueX"}, 456)));
	EXPECT_EQ(0, (unit.GetKeyAsInt(sectionA, StringView{"stringValue"}, 2)));
	EXPECT_EQ(2, (unit.GetKeyAsInt(sectionA, StringView{"floatValue"}, 0)));
}

TEST_F(INIImporter_Test, GetKeyAsFloat) {
	auto sectionA = StringView{"SectionA"};

	EXPECT_EQ(2.5, (unit.GetKeyAsFloat(sectionA, StringView{"floatValue"}, 1.0)));
	EXPECT_EQ(2.5, (unit.GetKeyAsFloat(sectionA, StringView{"Floatvalue"}, 1.0)));
	EXPECT_EQ(1.5, (unit.GetKeyAsFloat(sectionA, StringView{"floatValueX"}, 1.5)));

	EXPECT_EQ(0.0, (unit.GetKeyAsFloat(sectionA, StringView{"stringValue"}, 2.0)));
	EXPECT_EQ(123.0, (unit.GetKeyAsFloat(sectionA, StringView{"intValue"}, 0.0)));
}

TEST_F(INIImporter_Test, GetKeyAsBool) {
	auto sectionA = StringView{"SectionA"};

	EXPECT_TRUE(unit.GetKeyAsBool(sectionA, StringView{"trueValue"}, false));
	EXPECT_TRUE(unit.GetKeyAsBool(sectionA, StringView{"TRUEvalue"}, false));
	EXPECT_FALSE(unit.GetKeyAsBool(sectionA, StringView{"falseValue"}, true));
	EXPECT_TRUE(unit.GetKeyAsBool(sectionA, StringView{"ghost"}, true));

	EXPECT_TRUE(unit.GetKeyAsBool(sectionA, StringView{"intValue"}, false));
	EXPECT_TRUE(unit.GetKeyAsBool(sectionA, StringView{"floatValue"}, false));
	EXPECT_FALSE(unit.GetKeyAsBool(sectionA, StringView{"stringValue"}, true));
}

}
