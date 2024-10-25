#include "../../core/Palette.h"

#include <gtest/gtest.h>

namespace GemRB {

TEST(Palette_Test, Constructor)
{
	Palette unit;
	EXPECT_FALSE(unit.IsNamed());
}

TEST(Palette_Test, SetAndGetColor)
{
	Color c { 0x12, 0x34, 0x56, 0 };
	Palette unit;

	unit.SetColor(17, c);
	EXPECT_EQ(c, unit[17]);
}

TEST(Palette_Test, CopyColors)
{
	Palette::Colors buffer;
	Palette unit;

	for (size_t i = 0; i < 256; ++i) {
		Color c { static_cast<uint8_t>(i), 1, 2, 3 };
		buffer[i] = c;
	}

	unit.CopyColors(1, buffer.cbegin() + 1, buffer.cend());

	EXPECT_EQ(unit[0], Color {});
	for (size_t i = 1; i < 256; ++i) {
		Color c { static_cast<uint8_t>(i), 1, 2, 3 };
		EXPECT_EQ(unit[i], c);
	}
}

TEST(Palette_Test, SetColor_Version)
{
	Color c { 1, 0, 0, 255 };
	Palette unit;

	auto v1 = unit.GetVersion();
	unit.SetColor(0, c);
	auto v2a = unit.GetVersion();
	EXPECT_NE(v1, v2a);

	unit.SetColor(0, c);
	auto v2b = unit.GetVersion();
	EXPECT_EQ(v2a, v2b);

	c.a = 123;
	unit.SetColor(1, c);
	auto v3 = unit.GetVersion();
	EXPECT_NE(v2a, v3);

	unit.SetColor(0, c);
	auto v4 = unit.GetVersion();
	EXPECT_NE(v3, v4);
}

TEST(Palette_Test, CopyColors_Version)
{
	Palette::Colors buffer;
	Palette unit;

	for (size_t i = 0; i < 256; ++i) {
		Color c { static_cast<uint8_t>(i), 1, 2, 3 };
		buffer[i] = c;
	}

	unit.CopyColors(1, buffer.cbegin() + 1, buffer.cend());
	auto v1 = unit.GetVersion();

	for (size_t i = 1; i < 256; ++i) {
		Color c { static_cast<uint8_t>(i), 2, 3, 4 };
		buffer[i] = c;
	}

	unit.CopyColors(1, buffer.cbegin() + 1, buffer.cend());
	auto v2 = unit.GetVersion();

	EXPECT_NE(v1, v2);
}

}
