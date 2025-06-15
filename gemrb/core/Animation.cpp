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

#include "EnumFlags.h"
#include "Interface.h"
#include "RNG.h"
#include "Sprite2D.h"

#include "Logging/Logging.h"

namespace GemRB {

Animation::Animation(std::vector<frame_t> fr, float customFPS) noexcept
	: frames(std::move(fr))
{
	size_t count = frames.size();
	assert(count > 0);
	frameIdx = RAND<index_t>(0, count - 1);
	flags = Flags::Active;
	fps = customFPS;

	for (const frame_t& frame : frames) {
		if (!frame) continue;
		Region r = frame->Frame;
		r.x_get() = -r.x_get();
		r.y_get() = -r.y_get();
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
	if (!(flags & Flags::Active)) {
		Log(MESSAGE, "Sprite2D", "Frame fetched while animation is inactive1!");
		return nullptr;
	}
	if (gameAnimation && core->IsFreezed()) {
		starttime = lastTime;
	} else if (gameAnimation) {
		starttime = GetMilliseconds() - lastTime;
	} else {
		starttime = GetMilliseconds();
	}
	return frames[GetCurrentFrameIndex()];
}

Animation::frame_t Animation::NextFrame(void)
{
	if (!(flags & Flags::Active)) {
		Log(MESSAGE, "Sprite2D", "Frame fetched while animation is inactive2!");
		return nullptr;
	}

	Holder<Sprite2D> ret = frames[GetCurrentFrameIndex()];

	if (endReached && bool(flags & Flags::Once))
		return ret;

	tick_t time;
	tick_t delta = 1000 / fps; // duration per frame in ms
	if (gameAnimation && core->IsFreezed()) {
		time = lastTime;
		paused = true;
	} else if (gameAnimation) {
		time = GetMilliseconds();
		if (paused) {
			paused = false;
			// adjust timer for the pause duration gap, so there is no frame skipping
			starttime += time - lastTime;
		}
		lastTime = time;
	} else {
		time = GetMilliseconds();
		lastTime = time;
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
			if (bool(flags & Flags::Once)) {
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
	if (!(flags & Flags::Active)) {
		Log(MESSAGE, "Sprite2D", "Frame fetched while animation is inactive!");
		return nullptr;
	}
	Holder<Sprite2D> ret = frames[GetCurrentFrameIndex()];
	starttime = master->starttime;
	endReached = master->endReached;
	lastTime = master->lastTime;
	paused = master->paused;

	//return a valid frame even if the master is longer (e.g. ankhegs)
	frameIdx = master->frameIdx % GetFrameCount();

	return ret;
}

/** Gets the i-th frame */
Animation::frame_t Animation::GetFrame(index_t i) const
{
	if (i >= GetFrameCount()) {
		return nullptr;
	}
	return frames[i];
}

void Animation::MirrorAnimation(BlitFlags bf)
{
	if (bf == BlitFlags::NONE) {
		return;
	}

	for (frame_t& sprite : frames) {
		if (!sprite) continue;
		sprite = sprite->copy();

		if (bf & BlitFlags::MIRRORX) {
			sprite->renderFlags ^= BlitFlags::MIRRORX;
			sprite->Frame.x_get() = sprite->Frame.w_get() - sprite->Frame.x_get();
		}
		if (bf & BlitFlags::MIRRORY) {
			sprite->renderFlags ^= BlitFlags::MIRRORY;
			sprite->Frame.y_get() = sprite->Frame.h_get() - sprite->Frame.y_get();
		}
	}

	if (bf & BlitFlags::MIRRORX) {
		// flip animArea horizontally as well
		animArea.x_get() = -animArea.w_get() - animArea.x_get();
	}

	if (bf & BlitFlags::MIRRORY) {
		// flip animArea vertically as well
		animArea.y_get() = -animArea.h_get() - animArea.y_get();
	}
}

void Animation::AddAnimArea(const Animation* slave)
{
	animArea.ExpandToRegion(slave->animArea);
}

}
