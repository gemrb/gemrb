// SPDX-FileCopyrightText: 2005 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

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
	int speed = 0;
	int phase = 0;
	enum Type {
		NONE,
		ADD,
		TINT,
		BRIGHTEN
	} type = NONE;
	bool locked = false;
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

	void CopyColors(const Colors&);

	const Color* ColorData() const;
	Colors::const_reference GetColorAt(size_t pos) const;
	Hash GetVersion() const;
	bool IsNamed() const;
	void SetColor(size_t index, const Color& c);
	void TranslucentShadowColor(bool enable);

private:
	Colors colors;
	Hash version;
	bool named = false; // true if the palette comes from a bmp and cached

	void updateVersion();
};

}

#endif
