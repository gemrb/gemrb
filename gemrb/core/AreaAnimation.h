/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2025 The GemRB Project
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

#ifndef AREA_ANIMATION_H
#define AREA_ANIMATION_H

#include "exports.h"

#include "Animation.h"
#include "Holder.h"
#include "Region.h"

#include <vector>

namespace GemRB {

constexpr int ANI_PRI_BACKGROUND = -9999;

class GEM_EXPORT AreaAnimation {
public:
	using index_t = Animation::index_t;

	enum class Flags : uint32_t {
		None = 0,
		Active = UnderType(Animation::Flags::Active), // if not set, animation is invisible
		BlendBlack = UnderType(Animation::Flags::BlendBlack), // blend black as transparent
		NoShadow = 4, // lightmap doesn't affect it
		Once = UnderType(Animation::Flags::Once), // stop after endframe
		Sync = UnderType(Animation::Flags::Sync), // synchronised draw (skip frames if needed)
		RandStart = UnderType(Animation::Flags::RandStart), // starts with a random frame in the start range
		NoWall = 64, // draw after walls (walls don't cover it)
		NotInFog = 0x80, // not visible in fog of war
		Background = 0x100, // draw before actors (actors cover it)
		AllCycles = 0x200, // draw all cycles, not just the cycle specified
		Palette = 0x400, // has own palette set
		Mirror = 0x800, // mirrored
		Combat = 0x1000, // draw in combat too
		PSTBit14 = 0x2000 // PST-only: unknown and rare, see #163 for area list
		// TODO: BGEE extended flags:
		// 0x2000: Use WBM resref
		// 0x4000: Draw stenciled (can be used to stencil animations using the water overlay mask of the tileset, eg. to give water surface a more natural look)
		// 0x8000: Use PVRZ resref
	};

	std::vector<Animation> animation;
	// dwords, or stuff combining to a dword
	Point Pos;
	ieDword appearance = 0;

	union {
		Animation::Flags animFlags;
		Flags flags = Flags::None;
	};

	// flags that must be touched by PST a bit only
	Flags originalFlags = Flags::None;
	// these are on one dword
	index_t sequence = 0;
	index_t frame = 0;
	// these are on one dword
	ieWord transparency = 0;
	ieWordSigned height = 0;
	// these are on one dword
	ieWord startFrameRange = 0;
	ieByte skipcycle = 0;
	ieByte startchance = 0;
	ieDword unknown48 = 0;
	// string values, not in any particular order
	ieVariable Name;
	ResRef BAM; // not only for saving back (StaticSequence depends on this)
	ResRef PaletteRef;
	// TODO: EE stores also the width/height for WBM and PVRZ resources (see Flags bit 13/15)
	Holder<Palette> palette;
	AreaAnimation() noexcept = default;
	AreaAnimation(const AreaAnimation& src) noexcept;
	~AreaAnimation() noexcept = default;
	AreaAnimation& operator=(const AreaAnimation&) noexcept;

	void InitAnimation();
	void SetPalette(const ResRef& PaletteRef);
	bool Schedule(ieDword gametime) const;
	Region DrawingRegion() const;
	void Draw(const Region& screen, Color tint, BlitFlags flags) const;
	void Update(const AreaAnimation* master);
	int GetHeight() const;
};

}

#endif
