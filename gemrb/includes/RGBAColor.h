// SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef RGBACOLOR_H
#define RGBACOLOR_H

#include <array>
#include <cstdint>

namespace GemRB {

#pragma pack(push, 1)
struct Color {
	unsigned char r = 0, g = 0, b = 0, a = 0;

	constexpr Color() noexcept = default;

	constexpr Color(unsigned char r, unsigned char g, unsigned char b, unsigned char a) noexcept
		: r(r), g(g), b(b), a(a) {}

	constexpr explicit Color(uint32_t px) noexcept
		: r(px >> 24), g(px >> 16), b(px >> 8), a(px) {}

	constexpr bool operator==(const Color& rhs) const noexcept
	{
		return r == rhs.r && g == rhs.g && b == rhs.b && a == rhs.a;
	}

	constexpr bool operator!=(const Color& rhs) const noexcept
	{
		return !operator==(rhs);
	}

	constexpr Color operator+(const Color& rhs) const noexcept
	{
		return Color(r + rhs.r, b + rhs.b, g + rhs.g, a + rhs.a);
	}

	constexpr Color operator-(const Color& rhs) const noexcept
	{
		return Color(r - rhs.r, b - rhs.b, g - rhs.g, a - rhs.a);
	}

	constexpr Color operator/(int rhs) const noexcept
	{
		return Color(r / rhs, b / rhs, g / rhs, a / rhs);
	}

	Color& operator+=(const Color& rhs) noexcept
	{
		r += rhs.r;
		b += rhs.b;
		g += rhs.g;
		a += rhs.a;
		return *this;
	}

	Color& operator*=(int rhs) noexcept
	{
		r *= rhs;
		b *= rhs;
		g *= rhs;
		a *= rhs;
		return *this;
	}

	constexpr uint32_t Packed() const noexcept
	{
		return (r << 24) | (g << 16) | (b << 8) | a;
	}

	constexpr static Color FromBGRA(uint32_t px) noexcept
	{
		return Color(px >> 8, px >> 16, px >> 24, px);
	}

	constexpr static Color FromARGB(uint32_t px) noexcept
	{
		return Color(px >> 16, px >> 8, px, px >> 24);
	}

	constexpr static Color FromABGR(uint32_t px) noexcept
	{
		return Color(px, px >> 8, px >> 16, px >> 24);
	}
}; // close of Color struct
#pragma pack(pop)

static_assert(sizeof(Color) == sizeof(uint32_t), "Incompatible Color struct layout. Please open an issue about this.");

template<int SIZE>
using ColorPal = std::array<Color, SIZE>;

static constexpr Color ColorBlack { 0x00, 0x00, 0x00, 0xff };
static constexpr Color ColorBlue { 0x00, 0x00, 0xff, 0xff };
static constexpr Color ColorBlueDark { 0x00, 0x00, 0x80, 0xff };
static constexpr Color ColorCyan { 0x00, 0xff, 0xff, 0xff };
static constexpr Color ColorGray { 0x80, 0x80, 0x80, 0xff };
static constexpr Color ColorGreen { 0x00, 0xff, 0x00, 0xff };
static constexpr Color ColorGreenDark { 0x00, 0x78, 0x00, 0xff };
static constexpr Color ColorMagenta { 0xff, 0x00, 0xff, 0xff };
static constexpr Color ColorOrange { 0xff, 0xff, 0x00, 0xff };
static constexpr Color ColorRed { 0xff, 0x00, 0x00, 0xff };
static constexpr Color ColorViolet { 0xa0, 0x00, 0xa0, 0xff };
static constexpr Color ColorYellow { 0xff, 0xff, 0x00, 0xff };
static constexpr Color ColorWhite { 0xff, 0xff, 0xff, 0xff };

}

#endif
