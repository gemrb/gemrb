// SPDX-FileCopyrightText: 2023 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "../../core/Streams/FileStream.h"
#include "../../plugins/IDSImporter/IDSImporter.h"

#include <gtest/gtest.h>

namespace GemRB {

static const path_t SAMPLE_FILE = PathJoin("tests", "resources", "IDSImporter", "sample.ids");
static const path_t ENC_SAMPLE_FILE = PathJoin("tests", "resources", "IDSImporter", "sample_encrypted.ids");

class IDSImporterTest : public testing::TestWithParam<path_t> {
protected:
	IDSImporter unit;
	const path_t path;

public:
	void SetUp() override
	{
		auto stream = new FileStream {};

		assert(stream->Open(GetParam()));
		assert(unit.Open(std::unique_ptr<DataStream> { stream }));
	}
};

TEST_P(IDSImporterTest, GetValueByString)
{
	// first matches
	EXPECT_EQ(unit.GetValue(StringView { "AXEFLM" }), 8);
	EXPECT_EQ(unit.GetValue(StringView { "SPSMPUFF" }), 43);
	EXPECT_EQ(unit.GetValue(StringView { "spsMpuff" }), 43);

	EXPECT_EQ(unit.GetValue(StringView { "HMGRML" }), -1);
}

TEST_P(IDSImporterTest, GetValueByInt)
{
	EXPECT_EQ(unit.GetValue(8), std::string { "axeflm" });
	EXPECT_EQ(unit.GetValue(9), std::string { "axeflm" });
	EXPECT_EQ(unit.GetValue(44), std::string { "spsmpuff" });

	EXPECT_EQ(unit.GetValue(2), std::string {});
	EXPECT_EQ(unit.GetValue(200), std::string {});
}

TEST_P(IDSImporterTest, GetStringIndex)
{
	EXPECT_EQ(unit.GetStringIndex(0), std::string { "axeflm" });
	EXPECT_EQ(unit.GetStringIndex(10), std::string { "arghxxl" });

	EXPECT_EQ(unit.GetStringIndex(100), std::string {});
}

TEST_P(IDSImporterTest, GetValueIndex)
{
	EXPECT_EQ(unit.GetValueIndex(0), 8);
	EXPECT_EQ(unit.GetValueIndex(10), 45);

#ifdef _MSC_VER
	GTEST_SKIP() << "Not working on native Windows.";
#else
	ASSERT_DEATH_IF_SUPPORTED({ unit.GetValueIndex(100); }, "");
#endif
}

TEST_P(IDSImporterTest, FindString)
{
	EXPECT_EQ(unit.FindString(StringView { "SPSMPUFF" }), 9);
	EXPECT_EQ(unit.FindString(StringView { "axeflm" }), 1);

	EXPECT_EQ(unit.FindString(StringView { "HMGRML" }), -1);
}

TEST_P(IDSImporterTest, FindValue)
{
	EXPECT_EQ(unit.FindValue(45), 11);
	EXPECT_EQ(unit.FindValue(8), 0);

	EXPECT_EQ(unit.FindValue(2), -1);
	EXPECT_EQ(unit.FindValue(100), -1);
}

TEST_P(IDSImporterTest, GetSize)
{
	EXPECT_EQ(unit.GetSize(), size_t(12));
}

TEST_P(IDSImporterTest, GetHighestValue)
{
	EXPECT_EQ(unit.GetHighestValue(), 58);
}

INSTANTIATE_TEST_SUITE_P(
	IDSImporterInstances,
	IDSImporterTest,
	testing::Values(
		SAMPLE_FILE,
		ENC_SAMPLE_FILE));

}
