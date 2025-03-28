/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2005-2006 The GemRB Project
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
 */

#ifndef PALETTE_H
#define PALETTE_H

#include "RGBAColor.h"
#include "exports.h"

#include "MurmurHash.h"

#include <algorithm>
#include <array>
#include <cstring>

namespace GemRB {

struct RGBModifier {
	Color rgb;
	int speed;
	int phase;
	enum Type {
		NONE,
		ADD,
		TINT,
		BRIGHTEN
	} type;
	bool locked;
};

class GEM_EXPORT Palette {
public:
	using Colors = std::array<Color, 256>;

	Palette(bool named = false) noexcept;
	Palette(const Color& color, const Color& back) noexcept;

	template<class Iterator>
	Palette(const Iterator begin, const Iterator end) noexcept
		: Palette()
	{
		std::copy(begin, end, colors.begin());
		updateVersion();
	}

	bool HasAlpha() const noexcept { return true; /* Drop when SDL1 is gone. */ }

	bool operator==(const Palette&) const noexcept;
	bool operator!=(const Palette&) const noexcept;

	Colors::const_iterator cbegin() const noexcept;
	Colors::const_iterator cend() const noexcept;
	Colors::const_reference operator[](size_t pos) const;

	template<class Iterator>
	void CopyColors(size_t offset, Iterator begin, Iterator end)
	{
		std::copy(begin, end, colors.begin() + offset);
		updateVersion();
	}

	const Color* ColorData() const;
	Colors::const_reference GetColorAt(size_t pos) const;
	Hash GetVersion() const;
	bool IsNamed() const;
	void SetColor(size_t index, const Color& c);

private:
	Colors colors;
	Hash version;
	bool named = false; // true if the palette comes from a bmp and cached

	void updateVersion();
};

}

#endif
