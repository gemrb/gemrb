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

#include <cstdint>

namespace GemRB {

struct Color {
	unsigned char r = 0, g = 0, b = 0, a = 0;

	constexpr Color() = default;

	constexpr Color(unsigned char r, unsigned char g, unsigned char b, unsigned char a)
	: r(r), g(g), b(b), a(a) {}
	
	constexpr explicit Color(uint32_t px)
	: r(px >> 24), g(px >> 16), b(px >> 8), a(px) {}

	constexpr bool operator==(const Color& rhs) const {
		return r == rhs.r && g == rhs.g && b == rhs.b && a == rhs.a;
	}
	
	constexpr bool operator!=(const Color& rhs) const {
		return !operator==(rhs);
	}
	
	constexpr uint32_t Packed() const {
		return (r << 24) | (g << 16) | (b << 8) | a;
	}
}; // close of Color struct

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
