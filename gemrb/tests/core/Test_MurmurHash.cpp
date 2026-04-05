// SPDX-FileCopyrightText: 2024 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "../../core/MurmurHash.h"

#include <gtest/gtest.h>

namespace GemRB {

TEST(MurmurHashTest, Feed)
{
	auto hasher = MurmurHash3_32();
	EXPECT_EQ(hasher.GetHash(), 0);

	hasher.Feed(0x34333231);
	EXPECT_EQ(hasher.GetHash(), 0x721C5DC3);

	hasher.Feed(0x38373635);
	EXPECT_EQ(hasher.GetHash(), 0x91B313CE);
}

}
