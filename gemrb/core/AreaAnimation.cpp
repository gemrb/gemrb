// SPDX-FileCopyrightText: 2025 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

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

	index_t animCount = af->GetCycleCount();
	if (!animCount) {
		Log(ERROR, "Map", "Will not load empty animation: {}", BAM);
		return;
	}

	auto GetAnimationPiece = [&af, this](index_t animCycle) {
		Animation ret;
		auto anim = af->GetCycle(animCycle);
		if (!anim)
			anim = af->GetCycle(0);

		assert(anim);
		ret = std::move(*anim);

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

void AreaAnimation::Update(const AreaAnimation* master)
{
	if (animation.empty()) return;

	auto& firstAnim = animation[0];
	if (master && bool(firstAnim.flags & Animation::Flags::Sync)) {
		firstAnim.GetSyncedNextFrame(master->animation[0]);
	} else {
		firstAnim.NextFrame();
	}

	for (size_t idx = 1; idx < animation.size(); ++idx) {
		auto& anim = animation[idx];
		if (bool(anim.flags & Animation::Flags::Sync)) {
			anim.GetSyncedNextFrame(firstAnim);
		} else {
			anim.NextFrame();
		}
	}
}

}
