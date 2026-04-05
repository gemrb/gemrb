// SPDX-FileCopyrightText: 2024 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "../../core/Orientation.h"
#include "../../core/Region.h"

#include <cmath>
#include <gtest/gtest.h>

namespace GemRB {

TEST(OrientationTest, GetOrient)
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
		dir = GetOrient(start, target);
		int expectedDir = GetMathyOrientation(static_cast<orient_t>(i));
		EXPECT_EQ(dir, expectedDir) << "i: " << i << " target: " << target.x << " " << target.y << std::endl;
	}
}

}
