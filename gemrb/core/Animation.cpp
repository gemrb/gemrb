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

#include "win32def.h"

#include "Game.h"
#include "Interface.h"
#include "Map.h"
#include "Sprite2D.h"
#include "Video.h"

Animation::Animation(int count)
{
	frames = (Sprite2D **) calloc(count, sizeof(Sprite2D *));
	indicesCount = count;
	if (count) {
		pos = rand() % count;
	}
	else {
		pos = 0;
	}
	starttime = 0;
	x = 0;
	y = 0;
	Flags = A_ANI_ACTIVE;
	fps = 15;
	endReached = false;
	//behaviour flags
	playReversed = false;
	gameAnimation = false;
}

Animation::~Animation(void)
{
	Video *video = core->GetVideoDriver();

	for (unsigned int i = 0; i < indicesCount; i++) {
		video->FreeSprite( frames[i] );
	}
	free(frames);
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
void Animation::AddFrame(Sprite2D* frame, unsigned int index)
{
	if (index>=indicesCount) {
		error("Animation", "You tried to write past a buffer in animation, BAD!\n");
	}
	core->GetVideoDriver()->FreeSprite(frames[index]);
	frames[index]=frame;

	int x = -frame->XPos;
	int y = -frame->YPos;
	int w = frame->Width;
	int h = frame->Height;
	if (x < animArea.x) {
		animArea.w += (animArea.x - x);
		animArea.x = x;
	}
	if (y < animArea.y) {
		animArea.h += (animArea.y - y);
		animArea.y = y;
	}
	if (x+w > animArea.x+animArea.w) {
		animArea.w = x+w-animArea.x;
	}
	if (y+h > animArea.y+animArea.h) {
		animArea.h = y+h-animArea.y;
	}
}

unsigned int Animation::GetCurrentFrame() const
{
	if (playReversed)
		return indicesCount-pos-1;
	return pos;
}

Sprite2D* Animation::LastFrame(void)
{
	if (!(Flags&A_ANI_ACTIVE)) {
		Log(MESSAGE, "Sprite2D", "Frame fetched while animation is inactive1!");
		return NULL;
	}
	if (gameAnimation) {
		starttime = core->GetGame()->Ticks;
	} else {
		starttime = GetTickCount();
	}
	Sprite2D* ret;
	if (playReversed)
		ret = frames[indicesCount-pos-1];
	else
		ret = frames[pos];
	return ret;
}

Sprite2D* Animation::NextFrame(void)
{
	if (!(Flags&A_ANI_ACTIVE)) {
		Log(MESSAGE, "Sprite2D", "Frame fetched while animation is inactive2!");
		return NULL;
	}
	if (starttime == 0) {
		if (gameAnimation) {
			starttime = core->GetGame()->Ticks;
		} else {
			starttime = GetTickCount();
		}
	}
	Sprite2D* ret;
	if (playReversed)
		ret = frames[indicesCount-pos-1];
	else
		ret = frames[pos];

	if (endReached && (Flags&A_ANI_PLAYONCE) )
		return ret;

	unsigned long time;
	if (gameAnimation) {
		time = core->GetGame()->Ticks;
	} else {
		time = GetTickCount();
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
		starttime = 0;
	}
	return ret;
}

Sprite2D* Animation::GetSyncedNextFrame(Animation* master)
{
	if (!(Flags&A_ANI_ACTIVE)) {
		Log(MESSAGE, "Sprite2D", "Frame fetched while animation is inactive3!");
		return NULL;
	}
	Sprite2D* ret;
	if (playReversed)
		ret = frames[indicesCount-pos-1];
	else
		ret = frames[pos];

	starttime = master->starttime;
	pos = master->pos;
	endReached = master->endReached;

	return ret;
}


void Animation::release(void)
{
	delete this;
}
/** Gets the i-th frame */
Sprite2D* Animation::GetFrame(unsigned int i)
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
		Sprite2D * tmp = frames[i];
		frames[i] = video->MirrorSpriteHorizontal( tmp, true );
		video->FreeSprite(tmp);
	}

	// flip animArea horizontally as well
	animArea.x = -animArea.w - animArea.x;
}

void Animation::MirrorAnimationVert()
{
	Video *video = core->GetVideoDriver();

	for (size_t i = 0; i < indicesCount; i++) {
		Sprite2D * tmp = frames[i];
		frames[i] = video->MirrorSpriteVertical( tmp, true );
		video->FreeSprite(tmp);
	}

	// flip animArea vertically as well
//	animArea.y = -animArea.h - animArea.y;
}

void Animation::AddAnimArea(Animation* slave)
{
	int x = slave->animArea.x;
	int y = slave->animArea.y;
	int w = slave->animArea.w;
	int h = slave->animArea.h;
	if (x < animArea.x) {
		animArea.w += (animArea.x - x);
		animArea.x = x;
	}
	if (y < animArea.y) {
		animArea.h += (animArea.y - y);
		animArea.y = y;
	}
	if (x+w > animArea.x+animArea.w) {
		animArea.w = x+w-animArea.x;
	}
	if (y+h > animArea.y+animArea.h) {
		animArea.h = y+h-animArea.y;
	}
}
