/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2006 The GemRB Project
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
 *
 */

#include "Palette.h"

#include "Interface.h"

namespace GemRB {

#define MUL    2

Palette::Palette(const Color &color, const Color &back) noexcept
: Palette()
{
	col[0] = Color(0, 0xff, 0, 0);
	for (int i = 1; i < 256; i++) {
		float p = i / 255.0f;
		col[i].r = std::min<int>(back.r * (1 - p) + color.r * p, 255);
		col[i].g = std::min<int>(back.g * (1 - p) + color.g * p, 255);
		col[i].b = std::min<int>(back.b * (1 - p) + color.b * p, 255);
		// FIXME: alpha value changed to opaque to fix these palettes on SDL 2
		// I'm not sure if the previous implementation had a purpose, but historically the alpha is ignored in IE
		col[i].a = 0xff;
	}
}

void Palette::UpdateAlpha() noexcept
{
	// skip index 0 which is always the color key
	for (int i = 1; i < 256; ++i) {
		if (col[i].a != 0xff) {
			alpha = true;
			return;
		}
	}
	
	alpha = false;
}

void Palette::CopyColorRangePrivate(const Color* srcBeg, const Color* srcEnd, Color* dst) const noexcept
{
	// no update to alpha or version, hence being private
	std::copy(srcBeg, srcEnd, dst);
}

void Palette::CopyColorRange(const Color* srcBeg, const Color* srcEnd, uint8_t dst) noexcept
{
	CopyColorRangePrivate(srcBeg, srcEnd, &col[dst]);
	UpdateAlpha();
	version++;
}

Holder<Palette> Palette::Copy() const noexcept
{
	return MakeHolder<Palette>(std::begin(col), std::end(col));
}

bool Palette::operator==(const Palette& other) const noexcept {
	return memcmp(col, other.col, sizeof(col)) == 0;
}

bool Palette::operator!=(const Palette& other) const noexcept {
	return !(*this == other);
}

}
