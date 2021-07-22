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

AnimationFactory::AnimationFactory(const ResRef &resref,
								   std::vector<Holder<Sprite2D>> frames,
								   std::vector<CycleEntry> cycles,
								   std::vector<index_t> FLTable)
: FactoryObject(resref, IE_BAM_CLASS_ID),
frames(std::move(frames)),
cycles(std::move(cycles)),
FLTable(std::move(FLTable))
{
	assert(frames.size() < InvalidIndex);
	assert(cycles.size() < InvalidIndex);
	assert(FLTable.size() < InvalidIndex);
}

Animation* AnimationFactory::GetCycle(index_t cycle)
{
	if (cycle >= cycles.size() || cycles[cycle].FramesCount == 0) {
		return nullptr;
	}
	index_t ff = cycles[cycle].FirstFrame;
	index_t lf = ff + cycles[cycle].FramesCount;
	Animation* anim = new Animation( cycles[cycle].FramesCount );
	int c = 0;
	for (index_t i = ff; i < lf; i++) {
		anim->AddFrame(frames[FLTable[i]], c++);
	}
	return anim;
}

/* returns the required frame of the named cycle, cycle defaults to 0 */
Holder<Sprite2D> AnimationFactory::GetFrame(index_t index, index_t cycle) const
{
	if (cycle >= cycles.size()) {
		return NULL;
	}
	index_t ff = cycles[cycle].FirstFrame, fc = cycles[cycle].FramesCount;
	if(index >= fc) {
		return NULL;
	}
	return frames[FLTable[ff+index]];
}

Holder<Sprite2D> AnimationFactory::GetFrameWithoutCycle(index_t index) const
{
	if(index >= frames.size()) {
		return NULL;
	}
	return frames[index];
}

Holder<Sprite2D> AnimationFactory::GetPaperdollImage(const ieDword *Colors,
		Holder<Sprite2D> &Picture2, unsigned int type) const
{
	if (frames.size()<2) {
		return NULL;
	}

	// mod paperdolls can be unsorted (Longer Road Irenicus cycle: 1 1 0)
	index_t first = InvalidIndex, second = InvalidIndex; // top and bottom half
	index_t ff = cycles[0].FirstFrame;
	for (index_t f = 0; f < cycles[0].FramesCount; f++) {
		index_t idx = FLTable[ff + f];
		if (first == InvalidIndex) {
			first = idx;
		} else if (second == InvalidIndex && idx != first) {
			second = idx;
			break;
		}
	}
	if (second == InvalidIndex) {
		return NULL;
	}

	Picture2 = frames[second]->copy();
	if (!Picture2) {
		return NULL;
	}
	if (Colors) {
		PaletteHolder palette = Picture2->GetPalette();
		palette->SetupPaperdollColours(Colors, type);
	}

	Picture2->Frame.x = frames[second]->Frame.x;
	Picture2->Frame.y = frames[second]->Frame.y - 80;

	Holder<Sprite2D> spr = frames[first]->copy();
	if (Colors) {
		PaletteHolder palette = spr->GetPalette();
		palette->SetupPaperdollColours(Colors, type);
	}

	spr->Frame.x = frames[first]->Frame.x;
	spr->Frame.y = frames[first]->Frame.y;
	return spr;
}

AnimationFactory::index_t AnimationFactory::GetCycleSize(index_t idx) const
{
	if (idx >= cycles.size())
		return 0;

	return cycles[idx].FramesCount;
}

}
