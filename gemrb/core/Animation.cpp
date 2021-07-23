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
#include "Map.h"
#include "Sprite2D.h"
#include "Video/Video.h"
#include "RNG.h"

namespace GemRB {

Animation::Animation(index_t count)
: frames(count, nullptr)
{
	assert(count > 0);
	indicesCount = count;
	frameIdx = RAND<index_t>(0, count-1);
	starttime = 0;
	Flags = A_ANI_ACTIVE;
	fps = ANI_DEFAULT_FRAMERATE;
	endReached = false;
	//behaviour flags
	playReversed = false;
	gameAnimation = false;
}

void Animation::SetFrame(index_t index)
{
	if (index<indicesCount) {
		frameIdx = index;
	}
	starttime = 0;
	endReached = false;
}

/* when adding NULL, it means we already added a frame of index */
void Animation::AddFrame(const Holder<Sprite2D>& frame, index_t index)
{
	if (index>=indicesCount) {
		error("Animation", "You tried to write past a buffer in animation, BAD!\n");
	}
	frames[index] = frame;

	Region r = frame->Frame;
	r.x = -r.x;
	r.y = -r.y;
	
	animArea.ExpandToRegion(r);
}

Animation::index_t Animation::GetCurrentFrameIndex() const
{
	if (playReversed)
		return indicesCount - frameIdx - 1;
	return frameIdx;
}

Holder<Sprite2D> Animation::CurrentFrame() const
{
	return GetFrame(GetCurrentFrameIndex());
}

Holder<Sprite2D> Animation::LastFrame(void)
{
	if (!(Flags&A_ANI_ACTIVE)) {
		Log(MESSAGE, "Sprite2D", "Frame fetched while animation is inactive1!");
		return NULL;
	}
	if (gameAnimation) {
		starttime = core->GetGame()->Ticks;
	} else {
		starttime = GetTicks();
	}
	Holder<Sprite2D> ret;
	if (playReversed)
		ret = frames[indicesCount - frameIdx - 1];
	else
		ret = frames[frameIdx];
	return ret;
}

Holder<Sprite2D> Animation::NextFrame(void)
{
	if (!(Flags&A_ANI_ACTIVE)) {
		Log(MESSAGE, "Sprite2D", "Frame fetched while animation is inactive2!");
		return NULL;
	}
	if (starttime == 0) {
		if (gameAnimation) {
			starttime = core->GetGame()->Ticks;
		} else {
			starttime = GetTicks();
		}
	}
	Holder<Sprite2D> ret;
	if (playReversed)
		ret = frames[indicesCount - frameIdx - 1];
	else
		ret = frames[frameIdx];

	if (endReached && (Flags&A_ANI_PLAYONCE))
		return ret;

	tick_t time = gameAnimation ? core->GetGame()->Ticks : GetTicks();

	//it could be that we skip more than one frame in case of slow rendering
	//large, composite animations (dragons, multi-part area anims) require synchronisation
	if ((time - starttime) >= tick_t(1000 / fps)) {
		tick_t inc = (time-starttime) * fps / 1000;
		frameIdx += inc;
		starttime += inc*1000/fps;
	}
	if (frameIdx >= indicesCount ) {
		if (indicesCount) {
			if (Flags&A_ANI_PLAYONCE) {
				frameIdx = indicesCount-1;
				endReached = true;
			} else {
				frameIdx %= indicesCount;
				endReached = false; //looping, there is no end
			}
		} else {
			frameIdx = 0;
			endReached = true;
		}
	}
	return ret;
}

Holder<Sprite2D> Animation::GetSyncedNextFrame(const Animation* master)
{
	if (!(Flags&A_ANI_ACTIVE)) {
		Log(MESSAGE, "Sprite2D", "Frame fetched while animation is inactive!");
		return NULL;
	}
	Holder<Sprite2D> ret;
	if (playReversed)
		ret = frames[indicesCount - frameIdx - 1];
	else
		ret = frames[frameIdx];

	starttime = master->starttime;
	endReached = master->endReached;

	//return a valid frame even if the master is longer (e.g. ankhegs)
	frameIdx = master->frameIdx % indicesCount;

	return ret;
}


void Animation::release(void)
{
	delete this;
}
/** Gets the i-th frame */
Holder<Sprite2D> Animation::GetFrame(index_t i) const
{
	if (i >= indicesCount) {
		return NULL;
	}
	return frames[i];
}

void Animation::MirrorAnimation()
{
	Video *video = core->GetVideoDriver();

	for (size_t i = 0; i < indicesCount; i++) {
		frames[i] = video->MirrorSprite(frames[i], BlitFlags::MIRRORX, true);
	}

	// flip animArea horizontally as well
	animArea.x = -animArea.w - animArea.x;
}

void Animation::MirrorAnimationVert()
{
	Video *video = core->GetVideoDriver();

	for (size_t i = 0; i < indicesCount; i++) {
		frames[i] = video->MirrorSprite(frames[i], BlitFlags::MIRRORY, true);
	}

	// flip animArea vertically as well
	animArea.y = -animArea.h - animArea.y;
}

void Animation::AddAnimArea(const Animation* slave)
{
	animArea.ExpandToRegion(slave->animArea);
}

}
