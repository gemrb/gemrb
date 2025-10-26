/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2025 The GemRB Project
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

// A class for ARE area animation structure handling

#include "AreaAnimation.h"

#include "AnimationFactory.h"
#include "GameData.h"
#include "Interface.h"

#include "Logging/Logging.h"

namespace GemRB {

AreaAnimation& AreaAnimation::operator=(const AreaAnimation& src) noexcept
{
	if (this != &src) {
		animation = src.animation;
		sequence = src.sequence;
		flags = src.flags;
		originalFlags = src.originalFlags;
		Pos = src.Pos;
		appearance = src.appearance;
		frame = src.frame;
		transparency = src.transparency;
		height = src.height;
		startFrameRange = src.startFrameRange;
		skipcycle = src.skipcycle;
		startchance = src.startchance;
		unknown48 = 0;

		PaletteRef = src.PaletteRef;
		Name = src.Name;
		BAM = src.BAM;

		palette = src.palette ? MakeHolder<Palette>(*src.palette) : nullptr;

		// handles the rest: animation, resets animCount
		InitAnimation();
	}
	return *this;
}

AreaAnimation::AreaAnimation(const AreaAnimation& src) noexcept
{
	operator=(src);
}

void AreaAnimation::InitAnimation()
{
	auto af = gamedata->GetFactoryResourceAs<const AnimationFactory>(BAM, IE_BAM_CLASS_ID);
	if (!af) {
		Log(ERROR, "Map", "Cannot load animation: {}", BAM);
		return;
	}

	auto GetAnimationPiece = [af, this](index_t animCycle) {
		Animation ret;
		Animation* anim = af->GetCycle(animCycle);
		if (!anim)
			anim = af->GetCycle(0);

		assert(anim);
		ret = std::move(*anim);
		delete anim;

		// this will make the animation stop when the game is stopped
		// a possible gemrb feature to have this flag settable in .are
		ret.gameAnimation = true;
		ret.SetFrame(frame); // sanity check it first
		ret.flags = animFlags & ~Animation::Flags::AnimMask;
		ret.pos = Pos;
		if (bool(flags & Flags::Mirror)) {
			ret.MirrorAnimation(BlitFlags::MIRRORX);
		}

		return ret;
	};

	index_t animCount = af->GetCycleCount();
	animation.reserve(animCount);
	index_t existingcount = std::min<index_t>(animation.size(), animCount);

	if (bool(flags & Flags::AllCycles) && animCount > 0) {
		index_t i = 0;
		for (; i < existingcount; ++i) {
			animation[i] = GetAnimationPiece(i);
		}
		for (; i < animCount; ++i) {
			animation.push_back(GetAnimationPiece(i));
		}
	} else if (animCount) {
		animation.push_back(GetAnimationPiece(sequence));
	}

	if (bool(flags & Flags::Palette)) {
		SetPalette(PaletteRef);
	}
}

void AreaAnimation::SetPalette(const ResRef& pal)
{
	flags |= Flags::Palette;
	PaletteRef = pal;
	palette = gamedata->GetPalette(PaletteRef);
}

bool AreaAnimation::Schedule(ieDword gametime) const
{
	if (!(flags & Flags::Active)) {
		return false;
	}

	// check if the time is right
	return GemRB::Schedule(appearance, gametime);
}

int AreaAnimation::GetHeight() const
{
	return (bool(flags & Flags::Background)) ? ANI_PRI_BACKGROUND : height;
}

Region AreaAnimation::DrawingRegion() const
{
	Region r(Pos, Size());
	size_t ac = animation.size();
	while (ac--) {
		const Animation& anim = animation[ac];
		Region animRgn = anim.animArea;
		animRgn.x += Pos.x;
		animRgn.y += Pos.y;

		r.ExpandToRegion(animRgn);
	}
	return r;
}

void AreaAnimation::Draw(const Region& viewport, Color tint, BlitFlags bf) const
{
	if (transparency) {
		tint.a = 255 - transparency;
		bf |= BlitFlags::ALPHA_MOD;
	} else {
		tint.a = 255;
	}

	if (bool(flags & Flags::BlendBlack)) {
		bf |= BlitFlags::ONE_MINUS_DST;
	}

	size_t ac = animation.size();
	while (ac--) {
		const Animation& anim = animation[ac];
		VideoDriver->BlitGameSpriteWithPalette(anim.CurrentFrame(), palette, Pos - viewport.origin, bf, tint);
	}
}

void AreaAnimation::Update()
{
	if (animation.empty()) return;

	Animation& firstAnim = animation[0];
	firstAnim.NextFrame();
	for (size_t idx = 1; idx < animation.size(); ++idx) {
		Animation& anim = animation[idx];
		if (bool(anim.flags & Animation::Flags::Sync)) {
			anim.GetSyncedNextFrame(&firstAnim);
		} else {
			anim.NextFrame();
		}
	}
}

}
