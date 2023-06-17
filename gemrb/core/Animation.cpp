/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
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

#include "Animation.h"

#include "Game.h"
#include "Interface.h"
#include "Logging/Logging.h"
#include "Map.h"
#include "Sprite2D.h"
#include "RNG.h"

namespace GemRB {

Animation::Animation(std::vector<frame_t> fr) noexcept
: frames(std::move(fr))
{
	size_t count = frames.size();
	assert(count > 0);
	frameIdx = RAND<index_t>(0, count-1);
	Flags = A_ANI_ACTIVE;

	for (const frame_t& frame : frames) {
		if (!frame) continue;
		Region r = frame->Frame;
		r.x = -r.x;
		r.y = -r.y;
		animArea.ExpandToRegion(r);
	}
}

void Animation::SetFrame(index_t index)
{
	if (index < GetFrameCount()) {
		frameIdx = index;
	}
	starttime = 0;
	endReached = false;
}

Animation::index_t Animation::GetCurrentFrameIndex() const
{
	if (playReversed)
		return GetFrameCount() - frameIdx - 1;
	return frameIdx;
}

Animation::frame_t Animation::CurrentFrame() const
{
	return GetFrame(GetCurrentFrameIndex());
}

Animation::frame_t Animation::LastFrame(void)
{
	if (!(Flags&A_ANI_ACTIVE)) {
		Log(MESSAGE, "Sprite2D", "Frame fetched while animation is inactive1!");
		return NULL;
	}
	if (gameAnimation) {
		starttime = core->GetGame()->Ticks;
	} else {
		starttime = GetMilliseconds();
	}
	Holder<Sprite2D> ret;
	if (playReversed)
		ret = frames[GetFrameCount() - frameIdx - 1];
	else
		ret = frames[frameIdx];
	return ret;
}

Animation::frame_t Animation::NextFrame(void)
{
	if (!(Flags&A_ANI_ACTIVE)) {
		Log(MESSAGE, "Sprite2D", "Frame fetched while animation is inactive2!");
		return NULL;
	}

	Holder<Sprite2D> ret;
	if (playReversed)
		ret = frames[GetFrameCount() - frameIdx - 1];
	else
		ret = frames[frameIdx];

	if (endReached && (Flags&A_ANI_PLAYONCE))
		return ret;

	tick_t time;
	tick_t delta = 1000 / fps; // duration per frame in ms
	if (gameAnimation) {
		time = core->Time.Ticks2Ms(core->GetGame()->GameTime);
	} else {
		time = GetMilliseconds();
	}
	if (starttime == 0) starttime = time;

	//it could be that we skip more than one frame in case of slow rendering
	//large, composite animations (dragons, multi-part area anims) require synchronisation
	if (time - starttime >= delta) {
		tick_t inc = (time - starttime) / delta;
		frameIdx += inc;
		starttime = time;
	}
	if (frameIdx >= GetFrameCount()) {
		if (!frames.empty()) {
			if (Flags&A_ANI_PLAYONCE) {
				frameIdx = GetFrameCount() - 1;
				endReached = true;
			} else {
				frameIdx %= GetFrameCount();
				endReached = false; //looping, there is no end
			}
		} else {
			frameIdx = 0;
			endReached = true;
		}
	}
	return ret;
}

Animation::frame_t Animation::GetSyncedNextFrame(const Animation* master)
{
	if (!(Flags&A_ANI_ACTIVE)) {
		Log(MESSAGE, "Sprite2D", "Frame fetched while animation is inactive!");
		return NULL;
	}
	Holder<Sprite2D> ret;
	if (playReversed)
		ret = frames[GetFrameCount() - frameIdx - 1];
	else
		ret = frames[frameIdx];

	starttime = master->starttime;
	endReached = master->endReached;

	//return a valid frame even if the master is longer (e.g. ankhegs)
	frameIdx = master->frameIdx % GetFrameCount();

	return ret;
}


void Animation::release(void)
{
	delete this;
}
/** Gets the i-th frame */
Animation::frame_t Animation::GetFrame(index_t i) const
{
	if (i >= GetFrameCount()) {
		return NULL;
	}
	return frames[i];
}

void Animation::MirrorAnimation(BlitFlags flags)
{
	if (flags == BlitFlags::NONE) {
		return;
	}

	for (frame_t& sprite : frames) {
		sprite = sprite->copy();

		if (flags & BlitFlags::MIRRORX) {
			sprite->renderFlags ^= BlitFlags::MIRRORX;
			sprite->Frame.x = sprite->Frame.w - sprite->Frame.x;
		}
		if (flags & BlitFlags::MIRRORY) {
			sprite->renderFlags ^= BlitFlags::MIRRORY;
			sprite->Frame.y = sprite->Frame.h - sprite->Frame.y;
		}
	}

	if (flags & BlitFlags::MIRRORX) {
		// flip animArea horizontally as well
		animArea.x = -animArea.w - animArea.x;
	}

	if (flags & BlitFlags::MIRRORY) {
		// flip animArea vertically as well
		animArea.y = -animArea.h - animArea.y;
	}
}

void Animation::AddAnimArea(const Animation* slave)
{
	animArea.ExpandToRegion(slave->animArea);
}

}
