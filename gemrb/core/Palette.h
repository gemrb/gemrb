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
#include "ie_types.h"
#include "Holder.h"

#include <algorithm>
#include <cassert>
#include <cstdint>
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

class Palette;
using PaletteHolder = Holder<Palette>;

class GEM_EXPORT Palette : public Held<Palette>
{
private:
	void CopyColorRangePrivate(const Color* srcBeg, const Color* srcEnd, Color* dst) const noexcept;
	void UpdateAlpha() noexcept;
public:
	Palette(const Color &color, const Color &back) noexcept;

	Palette(const Color* begin, const Color* end) noexcept
	: Palette() {
		std::copy(begin, end, col);
		UpdateAlpha();
	}

	Palette() noexcept = default;

	Color col[256]; //< RGB or RGBA 8 bit palette
	bool named = false; //< true if the palette comes from a bmp and cached

	unsigned short GetVersion() const noexcept { return version; }

	bool HasAlpha() const noexcept { return alpha; }

	void SetupPaperdollColours(const ieDword* Colors, unsigned int type) noexcept;
	void SetupRGBModification(const PaletteHolder& src, const RGBModifier* mods,
		unsigned int type) noexcept;
	void SetupGlobalRGBModification(const PaletteHolder& src,
		const RGBModifier& mod) noexcept;

	PaletteHolder Copy() const noexcept;

	void CopyColorRange(const Color* srcBeg, const Color* srcEnd, uint8_t dst) noexcept;
	bool operator==(const Palette&) const noexcept;
	bool operator!=(const Palette&) const noexcept;
private:
	unsigned short version = 0;
	bool alpha = false; // true if any colors in the palette have an alpha < 255
	// FIXME: version is not enough since `col` is public
	// must make it private to fully capture changes to it
};

}

#endif
