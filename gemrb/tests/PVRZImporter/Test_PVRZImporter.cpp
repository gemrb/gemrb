/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2024 The GemRB Project
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

	EXPECT_EQ(26208, PVRZImporter::GetBlockPixelMask(r, grid, 0, 0));
	EXPECT_EQ(26214, PVRZImporter::GetBlockPixelMask(r, grid, 0, 1));
	EXPECT_EQ(26214, PVRZImporter::GetBlockPixelMask(r, grid, 0, 2));
	EXPECT_EQ(102, PVRZImporter::GetBlockPixelMask(r, grid, 0, 3));
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

	EXPECT_EQ(3808, PVRZImporter::GetBlockPixelMask(r, grid, 0, 0));
	EXPECT_EQ(1904, PVRZImporter::GetBlockPixelMask(r, grid, 1, 0));
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

	EXPECT_EQ(1632, PVRZImporter::GetBlockPixelMask(r, grid, 0, 0));
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

	EXPECT_EQ(51, PVRZImporter::GetBlockPixelMask(r, grid, 0, 0));
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

	EXPECT_EQ(52224, PVRZImporter::GetBlockPixelMask(r, grid, 0, 0));
}
}
