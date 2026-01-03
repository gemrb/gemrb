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

#include <cmath>

namespace GemRB {

Palette::Palette(bool named) noexcept
	: named(named) {}

Palette::Palette(const Color& color, const Color& back) noexcept
	: Palette()
{
	colors[0] = Color(0, 0xff, 0, 0);
	for (size_t i = 1; i < colors.size(); i++) {
		float_t p = i / 255.0f;
		colors[i].r = std::min<int>(back.r * (1 - p) + color.r * p, 255);
		colors[i].g = std::min<int>(back.g * (1 - p) + color.g * p, 255);
		colors[i].b = std::min<int>(back.b * (1 - p) + color.b * p, 255);
		// FIXME: alpha value changed to opaque to fix these palettes on SDL 2
		// I'm not sure if the previous implementation had a purpose, but historically the alpha is ignored in IE
		colors[i].a = 0xff;
	}

	updateVersion();
}

bool Palette::operator==(const Palette& other) const noexcept
{
	return version == other.version;
}

bool Palette::operator!=(const Palette& other) const noexcept
{
	return !(*this == other);
}

Palette::Colors::const_iterator Palette::cbegin() const noexcept
{
	return colors.cbegin();
}

Palette::Colors::const_iterator Palette::cend() const noexcept
{
	return colors.cend();
}

Palette::Colors::const_reference Palette::operator[](size_t pos) const
{
	return GetColorAt(pos);
}

void Palette::SetColor(size_t index, const Color& c)
{
	colors[index] = c;
	updateVersion();
}

const Color* Palette::ColorData() const
{
	return colors.data();
}

Palette::Colors::const_reference Palette::GetColorAt(size_t pos) const
{
	return colors[pos];
}

void Palette::TranslucentShadowColor(bool enable)
{
	auto shadowColor = colors[1];
	shadowColor.a = enable ? 128 : 255;
	SetColor(1, shadowColor);
}

Hash Palette::GetVersion() const
{
	return version;
}

bool Palette::IsNamed() const
{
	return named;
}

void Palette::updateVersion()
{
	Hasher hasher;

	for (size_t i = 0; i < colors.size(); ++i) {
		const auto& color = colors[i];
		hasher.Feed(color.Packed());
	}

	version = hasher.GetHash();
}

}
