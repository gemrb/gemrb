/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */
#ifndef RGBACOLOR_H
#define RGBACOLOR_H

#include <array>
#include <cstdint>

namespace GemRB {

struct Color {
	unsigned char r = 0, g = 0, b = 0, a = 0;

	constexpr Color() noexcept = default;
	
	constexpr Color(unsigned char r, unsigned char g, unsigned char b, unsigned char a) noexcept
	: r(r), g(g), b(b), a(a) {}
	
	constexpr explicit Color(uint32_t px) noexcept
	: r(px >> 24), g(px >> 16), b(px >> 8), a(px) {}

	constexpr bool operator==(const Color& rhs) const noexcept {
		return r == rhs.r && g == rhs.g && b == rhs.b && a == rhs.a;
	}
	
	constexpr bool operator!=(const Color& rhs) const noexcept {
		return !operator==(rhs);
	}

	constexpr Color operator+(const Color& rhs) const noexcept {
		return Color(r + rhs.r, b + rhs.b, g + rhs.g, a + rhs.a);
	}

	constexpr Color operator-(const Color& rhs) const noexcept {
		return Color(r - rhs.r, b - rhs.b, g - rhs.g, a - rhs.a);
	}

	constexpr Color operator/(int rhs) const noexcept {
		return Color(r / rhs, b / rhs, g / rhs, a / rhs);
	}

	Color& operator+=(const Color& rhs) noexcept {
		r += rhs.r;
		b += rhs.g;
		g += rhs.b;
		a += rhs.a;
		return *this;
	}

	Color& operator*=(int rhs) noexcept {
		r *= rhs;
		b *= rhs;
		g *= rhs;
		a *= rhs;
		return *this;
	}

	constexpr uint32_t Packed() const noexcept {
		return (r << 24) | (g << 16) | (b << 8) | a;
	}
	
	constexpr static Color FromBGRA(uint32_t px) noexcept {
		return Color(px >> 8, px >> 16, px >> 24, px);
	}
	
	constexpr static Color FromARGB(uint32_t px) noexcept {
		return Color(px >> 16, px >> 8, px, px >> 24);
	}
	
	constexpr static Color FromABGR(uint32_t px) noexcept {
		return Color(px, px >> 8, px >> 16, px >> 24);
	}
}; // close of Color struct

template<int SIZE>
using ColorPal = std::array<Color, SIZE>;

static constexpr Color ColorBlack {0x00, 0x00, 0x00, 0xff};
static constexpr Color ColorBlue {0x00, 0x00, 0xff, 0xff};
static constexpr Color ColorBlueDark {0x00, 0x00, 0x80, 0xff};
static constexpr Color ColorCyan {0x00, 0xff, 0xff, 0xff};
static constexpr Color ColorGray {0x80, 0x80, 0x80, 0xff};
static constexpr Color ColorGreen {0x00, 0xff, 0x00, 0xff};
static constexpr Color ColorGreenDark {0x00, 0x78, 0x00, 0xff};
static constexpr Color ColorMagenta {0xff, 0x00, 0xff, 0xff};
static constexpr Color ColorOrange {0xff, 0xff, 0x00, 0xff};
static constexpr Color ColorRed {0xff, 0x00, 0x00, 0xff};
static constexpr Color ColorViolet {0xa0, 0x00, 0xa0, 0xff};
static constexpr Color ColorYellow {0xff, 0xff, 0x00, 0xff};
static constexpr Color ColorWhite {0xff, 0xff, 0xff, 0xff};

}

#endif
