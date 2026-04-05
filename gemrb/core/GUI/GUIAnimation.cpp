// SPDX-FileCopyrightText: 2017 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "GUIAnimation.h"

namespace GemRB {

// so far everything we need uses this cycle
static const uint8_t ColorCycleSteps[] = { 6, 4, 2, 0, 2, 4, 6, 8 };
ColorCycle GlobalColorCycle(7);

void ColorCycle::AdvanceTime(tick_t time)
{
	step = ColorCycleSteps[(time >> speed) & 7];
}

Color ColorCycle::Blend(const Color& c1, const Color& c2) const
{
	Color mix;
	// we dont waste our time with the alpha component
	mix.a = c1.a;
	mix.r = (c1.r * step + c2.r * (8 - step)) >> 3;
	mix.g = (c1.g * step + c2.g * (8 - step)) >> 3;
	mix.b = (c1.b * step + c2.b * (8 - step)) >> 3;
	return mix;
}

bool PointAnimation::HasEnded() const
{
	return current == end;
}

Point PointAnimation::GenerateNext(tick_t curTime)
{
	if (curTime < endtime) {
		int deltax = end.x - begin.x;
		int deltay = end.y - begin.y;
		unsigned long deltaT = endtime - begintime;
		Point p;
		p.x = deltax * double(curTime - begintime) / deltaT;
		p.y = deltay * double(curTime - begintime) / deltaT;

		return begin + p;
	} else {
		return end;
	}
}

Color ColorAnimation::GenerateNext(tick_t time)
{
	cycle.AdvanceTime(time - timeOffset);
	return cycle.Blend(end, begin);
}

bool ColorAnimation::HasEnded() const
{
	return repeat ? false : current == end;
}

SpriteAnimation::SpriteAnimation(std::shared_ptr<Animation> a)
	: GUIAnimation(/*anim->starttime*/ 0), anim(std::move(a)), flags(anim->flags), gameAnimation(anim->gameAnimation)
{
	assert(anim);
	current = anim->CurrentFrame();
}

Holder<Sprite2D> SpriteAnimation::GenerateNext(tick_t)
{
	const auto& frame = anim->NextFrame();
	return frame ? frame : current;
}

bool SpriteAnimation::HasEnded() const
{
	return anim->endReached;
}

bool SpriteAnimation::IsActive() const
{
	return anim && anim->IsActive();
}

}
