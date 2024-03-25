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

#include "../../core/Orientation.h"
#include "../../core/Region.h"

#include <gtest/gtest.h>

namespace GemRB {

TEST(Orientation_Test, GetOrient)
{
	// self
	auto start = Point(1000, 1000);
	int dir = GetOrient(start, start);
	EXPECT_EQ(dir, int(S));

	// test all 16 possible orientations / segments
	// segment centers
	constexpr int distance = 100;
	for (int i = 0; i < 16; i++) {
		float angle = i * M_PI / 8.0F;
		int dx = std::cos(angle) * distance;
		int dy = std::sin(angle) * distance;
		auto target = Point(start.x + dx, start.y - dy);
		dir = GetOrient(target, start);
		int expectedDir = PrevOrientation(E, i);
		EXPECT_EQ(dir, expectedDir) << "i: " << i << " target: " << target.x << " " << target.y << std::endl;
	}
}

}
