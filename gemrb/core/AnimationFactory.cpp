/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
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

#include "AnimationFactory.h"

#include "Interface.h"
#include "Sprite2D.h"

namespace GemRB {

AnimationFactory::AnimationFactory(const ResRef& resref,
				   std::vector<Holder<Sprite2D>> f,
				   std::vector<CycleEntry> c,
				   std::vector<index_t> flt)
	: FactoryObject(resref, IE_BAM_CLASS_ID),
	  frames(std::move(f)),
	  cycles(std::move(c)),
	  FLTable(std::move(flt))
{
	assert(frames.size() < InvalidIndex);
	assert(cycles.size() < InvalidIndex);
	assert(FLTable.size() < InvalidIndex);
	fps = core->GetAnimationFPS(resRef);
}

Animation* AnimationFactory::GetCycle(index_t cycle) const noexcept
{
	if (cycle >= cycles.size() || cycles[cycle].FramesCount == 0) {
		return nullptr;
	}
	index_t ff = cycles[cycle].FirstFrame;
	index_t lf = ff + cycles[cycle].FramesCount;
	std::vector<Animation::frame_t> animframes;
	animframes.reserve(cycles[cycle].FramesCount);
	for (index_t i = ff; i < lf; i++) {
		animframes.push_back(frames[FLTable[i]]);
	}
	assert(cycles[cycle].FramesCount == animframes.size());
	return new Animation(std::move(animframes), fps);
}

/* returns the required frame of the named cycle, cycle defaults to 0 */
Holder<Sprite2D> AnimationFactory::GetFrame(index_t index, index_t cycle) const
{
	if (cycle >= cycles.size()) {
		return nullptr;
	}
	index_t ff = cycles[cycle].FirstFrame;
	index_t fc = cycles[cycle].FramesCount;
	if (index >= fc) {
		return nullptr;
	}
	return frames[FLTable[ff + index]];
}

Holder<Sprite2D> AnimationFactory::GetFrameWithoutCycle(index_t index) const
{
	if (index >= frames.size()) {
		return nullptr;
	}
	return frames[index];
}

AnimationFactory::index_t AnimationFactory::GetCycleSize(index_t idx) const
{
	if (idx >= cycles.size())
		return 0;

	return cycles[idx].FramesCount;
}

}
