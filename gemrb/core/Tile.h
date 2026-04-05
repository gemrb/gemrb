// SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

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
