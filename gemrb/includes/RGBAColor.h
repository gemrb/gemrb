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

struct RevColor {
	unsigned char b,g,r,a;
};

struct Color {
	unsigned char r,g,b,a;
}
#ifdef __GNUC__
	__attribute__((aligned(4)))
#endif
; // close of Color struct

static const Color ColorBlack = {0x00, 0x00, 0x00, 0xff};
// FIXME: why does blue have an alpha of 0x80?
static const Color ColorBlue = {0x00, 0x00, 0xff, 0x80};
static const Color ColorCyan = {0x00, 0xff, 0xff, 0xff};
static const Color ColorGray = {0x80, 0x80, 0x80, 0xff};
static const Color ColorGreen = {0x00, 0xff, 0x00, 0xff};
static const Color ColorGreenDark = {0x00, 0x78, 0x00, 0xff};
static const Color ColorMagenta = {0xff, 0x00, 0xff, 0xff};
static const Color ColorRed = {0xff, 0x00, 0x00, 0xff};
static const Color ColorWhite = {0xff, 0xff, 0xff, 0xff};
}

#endif
