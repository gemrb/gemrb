// SPDX-FileCopyrightText: 2024 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "../../plugins/PVRZImporter/PVRZImporter.h"

#include <gtest/gtest.h>

namespace GemRB {

/*
 * Test case is region request over { 1, 1, 2, 13 }:
 *
 * 0 0 0 0 // grid at (0, 0)
 * 0 1 1 0
 * 0 1 1 0
 * 0 1 1 0
 *
 * 0 1 1 0 // at (0, 1)
 * 0 1 1 0
 * 0 1 1 0
 * 0 1 1 0
 *
 * 0 1 1 0 // at (0, 2)
 * 0 1 1 0
 * 0 1 1 0
 * 0 1 1 0
 *
 * 0 1 1 0 // at (0, 3)
 * 0 1 1 0
 * 0 0 0 0
 * 0 0 0 0
 */
TEST(PVRZImporterTest, GetBlockPixelMask1)
{
	Region r { 1, 1, 2, 13 };
	Region grid { 0, 0, 1, 4 };

	EXPECT_EQ(uint16_t(26208), PVRZImporter::GetBlockPixelMask(r, grid, 0, 0));
	EXPECT_EQ(uint16_t(26214), PVRZImporter::GetBlockPixelMask(r, grid, 0, 1));
	EXPECT_EQ(uint16_t(26214), PVRZImporter::GetBlockPixelMask(r, grid, 0, 2));
	EXPECT_EQ(uint16_t(102), PVRZImporter::GetBlockPixelMask(r, grid, 0, 3));
}

/*
 * 0 0 0 0  0 0 0 0
 * 0 1 1 1  1 1 1 0
 * 0 1 1 1  1 1 1 0
 * 0 0 0 0  0 0 0 0
 */
TEST(PVRZImporterTest, GetBlockPixelMask2)
{
	Region r { 1, 1, 6, 2 };
	Region grid { 0, 0, 2, 1 };

	EXPECT_EQ(uint16_t(3808), PVRZImporter::GetBlockPixelMask(r, grid, 0, 0));
	EXPECT_EQ(uint16_t(1904), PVRZImporter::GetBlockPixelMask(r, grid, 1, 0));
}

/*
 * 0 0 0 0
 * 0 1 1 0
 * 0 1 1 0
 * 0 0 0 0
 */
TEST(PVRZImporterTest, GetBlockPixelMask3)
{
	Region r { 1, 1, 2, 2 };
	Region grid { 0, 0, 1, 1 };

	EXPECT_EQ(uint16_t(1632), PVRZImporter::GetBlockPixelMask(r, grid, 0, 0));
}

/*
 * 1 1 0 0
 * 1 1 0 0
 * 0 0 0 0
 * 0 0 0 0
 */
TEST(PVRZImporterTest, GetBlockPixelMask4)
{
	Region r { 0, 0, 2, 2 };
	Region grid { 0, 0, 1, 1 };

	EXPECT_EQ(uint16_t(51), PVRZImporter::GetBlockPixelMask(r, grid, 0, 0));
}

/*
 * 0 0 0 0
 * 0 0 0 0
 * 0 0 1 1
 * 0 0 1 1
 */
TEST(PVRZImporterTest, GetBlockPixelMask5)
{
	Region r { 2, 2, 2, 2 };
	Region grid { 0, 0, 1, 1 };

	EXPECT_EQ(uint16_t(52224), PVRZImporter::GetBlockPixelMask(r, grid, 0, 0));
}
}
