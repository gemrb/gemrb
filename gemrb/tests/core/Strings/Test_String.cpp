/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2023 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#include <gtest/gtest.h>

#include "Strings/String.h"

namespace GemRB {

TEST(String_Test, RTrim) {
	String testString{u"space  "};
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

}
