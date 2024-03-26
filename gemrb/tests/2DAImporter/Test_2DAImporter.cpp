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
#include "../../plugins/2DAImporter/2DAImporter.h"

namespace GemRB {

static const path_t SAMPLE_FILE = PathJoin("tests", "resources", "2DAImporter", "sample.2da");
static const path_t ENC_SAMPLE_FILE = PathJoin("tests", "resources", "2DAImporter", "sample_encrypted.2da");
static const path_t SAMPLE_FILE2 = PathJoin("tests", "resources", "2DAImporter", "sample2.2da");

class p2DAImporter_Test : public testing::TestWithParam<path_t> {
protected:
	p2DAImporter unit;
public:
	void SetUp() override {
		auto stream = new FileStream{};

		assert(stream->Open(GetParam()));
		assert(unit.Open(std::unique_ptr<DataStream>{stream}));
	}
};

TEST_P(p2DAImporter_Test, GetRowCount) {
	EXPECT_EQ(unit.GetRowCount(), 7);
}

TEST_P(p2DAImporter_Test, GetColNamesCount) {
	EXPECT_EQ(unit.GetColNamesCount(), 4);
}

TEST_P(p2DAImporter_Test, GetColumnCount) {
	EXPECT_EQ(unit.GetColumnCount(), 4);
	EXPECT_EQ(unit.GetColumnCount(6), 3);
}

TEST_P(p2DAImporter_Test, QueryField) {
	EXPECT_EQ(unit.QueryField(0, 0), std::string{"11975"});
	EXPECT_EQ(unit.QueryField(0, 3), std::string{"STR"});
	EXPECT_EQ(unit.QueryField(6, 0), std::string{"100"});
	EXPECT_EQ(unit.QueryField(6, 3), std::string{"-1"});
}

TEST_P(p2DAImporter_Test, QueryDefault) {
	EXPECT_EQ(unit.QueryDefault(), std::string{"-1"});
}

TEST_P(p2DAImporter_Test, GetRowIndex) {
	EXPECT_EQ(unit.GetRowIndex(std::string{"STRENGTH"}), 0);
	EXPECT_EQ(unit.GetRowIndex(std::string{"SQUEEZENESS"}), 6);

	EXPECT_EQ(unit.GetRowIndex(std::string{"FLUFFINESS"}), p2DAImporter::npos);
}

TEST_P(p2DAImporter_Test, GetColumnIndex) {
	EXPECT_EQ(unit.GetColumnIndex(std::string{"NAME_REF"}), 0);
	EXPECT_EQ(unit.GetColumnIndex(std::string{"STAT_ID"}), 3);

	EXPECT_EQ(unit.GetColumnIndex(std::string{"COOLNESS"}), p2DAImporter::npos);
}

TEST_P(p2DAImporter_Test, GetColumnName) {
	EXPECT_EQ(unit.GetColumnName(0), std::string{"NAME_REF"});
	EXPECT_EQ(unit.GetColumnName(3), std::string{"STAT_ID"});

	EXPECT_EQ(unit.GetColumnName(4), std::string{});
}

TEST_P(p2DAImporter_Test, GetRowName) {
	EXPECT_EQ(unit.GetRowName(0), std::string{"STRENGTH"});
	EXPECT_EQ(unit.GetRowName(6), std::string{"SQUEEZENESS"});

	EXPECT_EQ(unit.GetRowName(7), std::string{});
}

TEST_P(p2DAImporter_Test, FindTableValue) {
	EXPECT_EQ(unit.FindTableValue(3, std::string{"CON"}, 0), 1);
	EXPECT_EQ(unit.FindTableValue(3, std::string{"CON"}, 2), 2);

	EXPECT_EQ(unit.FindTableValue(3, std::string{"NIX"}, 0), p2DAImporter::npos);
	EXPECT_EQ(unit.FindTableValue(4, std::string{"CON"}, 0), p2DAImporter::npos);
}

TEST_P(p2DAImporter_Test, FindTableValueByLong) {
	EXPECT_EQ(unit.FindTableValue(1, 9584, 0), 0);
	EXPECT_EQ(unit.FindTableValue(1, 9584, 1), 1);

	EXPECT_EQ(unit.FindTableValue(1, 17, 0), p2DAImporter::npos);
	EXPECT_EQ(unit.FindTableValue(4, 17, 0), p2DAImporter::npos);
}

INSTANTIATE_TEST_SUITE_P(
	2DAImporterInstances,
	p2DAImporter_Test,
	testing::Values(SAMPLE_FILE, ENC_SAMPLE_FILE)
);

// single column table
TEST(p2DAImporter_Test, GetColumnCount2)
{
	p2DAImporter unit;
	auto stream = new FileStream {};
	stream->Open(SAMPLE_FILE2);
	unit.Open(std::unique_ptr<DataStream> { stream });
	EXPECT_EQ(unit.GetColumnCount(), 1);
	EXPECT_EQ(unit.GetColumnCount(0), 1);
}

}
