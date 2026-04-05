// SPDX-FileCopyrightText: 2024 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "../../core/Streams/FileStream.h"
#include "../../plugins/BMPImporter/BMPImporter.h"

#include <gtest/gtest.h>

namespace GemRB {

static const path_t resources = PathJoin("tests", "resources", "BMPImporter");
static const path_t SAMPLE_FILE = PathJoin(resources, "sample.bmp");
static const path_t SAMPLE_FILE_8B = PathJoin(resources, "sample_8bit.bmp");
static const path_t SAMPLE_FILE_V3 = PathJoin(resources, "sample_v3.bmp");
static const path_t SAMPLE_FILE_V5 = PathJoin(resources, "sample_v5.bmp");

class BMPImporterTest : public testing::TestWithParam<path_t> {
protected:
	BMPImporter unit;
	const path_t path;

public:
	void SetUp() override
	{
		auto stream = new FileStream {};

		assert(stream->Open(GetParam()));
		assert(unit.Open(stream));
	}
};

// More like a smoke test
TEST_P(BMPImporterTest, GetPalette)
{
	Palette pal;
	EXPECT_EQ(unit.GetPalette(2, pal), -1);
}

INSTANTIATE_TEST_SUITE_P(
	BMPImporterInstances,
	BMPImporterTest,
	testing::Values(
		SAMPLE_FILE,
		SAMPLE_FILE_8B,
		SAMPLE_FILE_V3,
		SAMPLE_FILE_V5));

}
