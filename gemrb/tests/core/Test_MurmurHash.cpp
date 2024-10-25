#include "../../core/MurmurHash.h"

#include <gtest/gtest.h>

namespace GemRB {

TEST(MurmurHash_Test, Feed)
{
	auto hasher = MurmurHash3_32();
	EXPECT_EQ(hasher.GetHash(), 0);

	hasher.Feed(0x34333231);
	EXPECT_EQ(hasher.GetHash(), 0x721C5DC3);

	hasher.Feed(0x38373635);
	EXPECT_EQ(hasher.GetHash(), 0x91B313CE);
}

}
