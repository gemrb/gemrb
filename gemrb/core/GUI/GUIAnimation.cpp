/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2017 The GemRB Project
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

#include "GUIAnimation.h"

#include "AnimationFactory.h"
#include "GUI/Button.h"
#include "Interface.h"
#include "RNG.h"

namespace GemRB {

// so far everything we need uses this cycle
static const uint8_t ColorCycleSteps[] = {6,4,2,0,2,4,6,8};
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
	mix.r = (c1.r * step + c2.r * (8-step)) >> 3;
	mix.g = (c1.g * step + c2.g * (8-step)) >> 3;
	mix.b = (c1.b * step + c2.b * (8-step)) >> 3;
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

SpriteAnimation::SpriteAnimation(std::shared_ptr<const AnimationFactory> af, int cycle)
: bam(std::move(af)), cycle(cycle)
{
	assert(bam);
	nextFrameTime = begintime + CalculateNextFrameDelta();
}

bool SpriteAnimation::SameResource(const SpriteAnimation *anim) const
{
	if (!anim || anim->bam->resRef != bam->resRef) return false;
	uint8_t c = cycle;
	if (flags & PLAY_RANDOM) {
		c &= ~1;
	}
	if (anim->cycle != c) return false;
	return true;
}

void SpriteAnimation::SetPaletteGradients(const ieDword *col)
{
	memcpy(colors, col, 8*sizeof(ieDword));
	has_palette = true;
}

tick_t SpriteAnimation::CalculateNextFrameDelta()
{
	tick_t delta = 0;
	if (flags & PLAY_RANDOM) {
		// simple Finite-State Machine
		if (anim_phase == 0) {
			frame = 0;
			anim_phase = 1;
			// note: the granularity of time should be
			// one of twenty values from [500, 10000]
			// but not the full range.
			delta = 500 + 500 * RAND(0, 19);
			cycle &= ~1;
		} else if (anim_phase == 1) {
			if (!RAND(0,29)) {
				cycle |= 1;
			}
			anim_phase = 2;
			delta = 100;
		} else {
			frame++;
			delta = 100;
		}
	} else {
		frame++;
		if (has_palette) {
			delta = 100;  //hack for slower movement
		} else {
			delta = 15;
		}
	}
	return delta;
}

Holder<Sprite2D> SpriteAnimation::GenerateNext(tick_t time)
{
	if (time < nextFrameTime) return current;

	// even if we are paused we need to generate the first frame
	if (current && !(flags & PLAY_ALWAYS) && core->IsFreezed()) {
		nextFrameTime = time + 1;
		return current;
	}

	nextFrameTime = time + CalculateNextFrameDelta();
	assert(nextFrameTime);
	
	Holder<Sprite2D> pic = bam->GetFrame(frame, cycle);

	if (pic == nullptr) {
		//stopping at end frame
		if (flags & PLAY_ONCE) {
			nextFrameTime = 0;
			return current;
		}
		anim_phase = 0;
		frame = 0;
		pic = bam->GetFrame(0, cycle);
	}

	if (pic == nullptr) {
		// I guess we just return the existing frame...
		// we already did the palette manipulation (probably)
		return current;
	}

	if (has_palette) {
		PaletteHolder palette = pic->GetPalette();
		palette->SetupPaperdollColours(colors, 0);
	}
	
	pic->renderFlags |= blitFlags;

	return pic;
}

bool SpriteAnimation::HasEnded() const
{
	return nextFrameTime == 0;
}

}
