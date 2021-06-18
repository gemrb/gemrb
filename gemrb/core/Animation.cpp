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
#include "Video.h"
#include "RNG.h"

namespace GemRB {

Animation::Animation(int count)
: frames(count, nullptr)
{
	assert(count > 0);
	indicesCount = count;
	pos = RAND(0, count-1);
	starttime = 0;
	x = 0;
	y = 0;
	Flags = A_ANI_ACTIVE;
	fps = ANI_DEFAULT_FRAMERATE;
	endReached = false;
	//behaviour flags
	playReversed = false;
	gameAnimation = false;
}

Animation::~Animation(void)
{
	// Empty, but having it here rather than defined implicitly means
	// we don't have to include Sprite2D.h in the header.
}

void Animation::SetPos(unsigned int index)
{
	if (index<indicesCount) {
		pos=index;
	}
	starttime = 0;
	endReached = false;
}

/* when adding NULL, it means we already added a frame of index */
void Animation::AddFrame(Holder<Sprite2D> frame, unsigned int index)
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

unsigned int Animation::GetCurrentFrameIndex() const
{
	if (playReversed)
		return indicesCount-pos-1;
	return pos;
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
		ret = frames[indicesCount-pos-1];
	else
		ret = frames[pos];
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
		ret = frames[indicesCount-pos-1];
	else
		ret = frames[pos];

	if (endReached && (Flags&A_ANI_PLAYONCE))
		return ret;

	unsigned long time;
	if (gameAnimation) {
		time = core->GetGame()->Ticks;
	} else {
		time = GetTicks();
	}

	//it could be that we skip more than one frame in case of slow rendering
	//large, composite animations (dragons, multi-part area anims) require synchronisation
	if (( time - starttime ) >= ( unsigned long ) ( 1000 / fps )) {
		int inc = (time-starttime)*fps/1000;
		pos += inc;
		starttime += inc*1000/fps;
	}
	if (pos >= indicesCount ) {
		if (indicesCount) {
			if (Flags&A_ANI_PLAYONCE) {
				pos = indicesCount-1;
				endReached = true;
			} else {
				pos = pos%indicesCount;
				endReached = false; //looping, there is no end
			}
		} else {
			pos = 0;
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
		ret = frames[indicesCount-pos-1];
	else
		ret = frames[pos];

	starttime = master->starttime;
	endReached = master->endReached;

	//return a valid frame even if the master is longer (e.g. ankhegs)
	pos = master->pos % indicesCount;

	return ret;
}


void Animation::release(void)
{
	delete this;
}
/** Gets the i-th frame */
Holder<Sprite2D> Animation::GetFrame(unsigned int i) const
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

void Animation::AddAnimArea(Animation* slave)
{
	animArea.ExpandToRegion(slave->animArea);
}

}
