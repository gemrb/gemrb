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

namespace GemRB {

struct Color {
	unsigned char r,g,b,a;

	Color() {
		r = g = b = a = 0;
	}

	Color(unsigned char r, unsigned char g, unsigned char b, unsigned char a)
	: r(r), g(g), b(b), a(a) {}

	bool operator==(const Color& rhs) const {
		return r == rhs.r && g == rhs.g && b == rhs.b && a == rhs.a;
	}
	
	bool operator!=(const Color& rhs) const {
		return !operator==(rhs);
	}
}
#if defined(__GNUC__) && defined(HAS_OBJALIGN4)
	__attribute__((aligned(4)))
#endif
; // close of Color struct

static const Color ColorBlack(0x00, 0x00, 0x00, 0xff);
static const Color ColorBlue(0x00, 0x00, 0xff, 0xff);
static const Color ColorBlueDark(0x00, 0x00, 0x80, 0xff);
static const Color ColorCyan(0x00, 0xff, 0xff, 0xff);
static const Color ColorGray(0x80, 0x80, 0x80, 0xff);
static const Color ColorGreen(0x00, 0xff, 0x00, 0xff);
static const Color ColorGreenDark(0x00, 0x78, 0x00, 0xff);
static const Color ColorMagenta(0xff, 0x00, 0xff, 0xff);
static const Color ColorOrange(0xff, 0xff, 0x00, 0xff);
static const Color ColorRed(0xff, 0x00, 0x00, 0xff);
static const Color ColorViolet(0xa0, 0x00, 0xa0, 0xff);
static const Color ColorYellow(0xff, 0xff, 0x00, 0xff);
static const Color ColorWhite(0xff, 0xff, 0xff, 0xff);

}

#endif
