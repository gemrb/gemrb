/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

#ifndef TILE_H
#define TILE_H

#include "RGBAColor.h"
#include "exports.h"

#include "Animation.h"

#include <array>

namespace GemRB {

class GEM_EXPORT Tile {
public:
	Tile(Animation a1, Animation a2) noexcept
		: anim { std::make_unique<Animation>(std::move(a1)), std::make_unique<Animation>(std::move(a2)) }
	{}

	explicit Tile(Animation animation) noexcept
		: anim { std::make_unique<Animation>(std::move(animation)), nullptr }
	{}

	Tile(const Tile&) noexcept = delete;
	Tile& operator=(const Tile& rhs) noexcept = delete;

	Tile(Tile&&) noexcept = default;
	Tile& operator=(Tile&&) noexcept = default;

	Animation* GetAnimation() const noexcept
	{
		if (anim[tileIndex]) {
			return anim[tileIndex].get();
		}
		return anim[0].get();
	}

	Animation* GetAnimation(int idx) const noexcept
	{
		return anim[idx].get();
	}

	unsigned char tileIndex = 0;
	unsigned char om = 0;

private:
	std::unique_ptr<Animation> anim[2];
};

}

#endif
