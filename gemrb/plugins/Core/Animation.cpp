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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/Animation.cpp,v 1.14 2004/01/16 22:52:58 balrog994 Exp $
 *
 */

#include "../../includes/win32def.h"
#include "Animation.h"
#include "Interface.h"
#ifndef WIN32
#include <sys/time.h>
#else
#include <windows.h>
#endif

extern Interface * core;

Animation::Animation(unsigned short * frames, int count)
{
	indices = new unsigned short[count];
	indicesCount = count;
	memcpy(indices, frames, count*sizeof(unsigned short));
	//for(int i = 0; i < count; i++)
	//	indices.push_back(frames[i]);
	pos = rand()%count;
	starttime = 0;
	x = 0;
	y = 0;
	free = true;
	ChangePalette = true;
	BlitMode = IE_NORMAL;
	fps = 15;
	autoSwitchOnEnd = false;
	endReached = false;
	nextAnimID = 0;
	pastLastFrame = false;
	playReversed = false;
	playOnce = false;
	ResRef[0] = 0;
	Active = true;
}

Animation::~Animation(void)
{
	delete(indices);
	if(!free)
		return;
	for(unsigned int i = 0; i < frames.size(); i++) {
		core->GetVideoDriver()->FreeSprite(frames[i]);
	}
}

void Animation::AddFrame(Sprite2D * frame, int index)
{
	frames.push_back(frame);
	link.push_back(index);
	int x = -frame->XPos;
	int y = -frame->YPos;
	int w = frame->Width-frame->XPos;
	int h = frame->Height-frame->YPos;
	if(x < animArea.x)
		animArea.x = x;
	if(y < animArea.y)
		y = y;
	if(w > animArea.w)
		animArea.w = w;
	if(h > animArea.h)
		animArea.h = h;
}

Sprite2D * Animation::NextFrame(void)
{
	if(!Active)
		return NULL;
	if(starttime == 0) {
#ifdef WIN32
		starttime = GetTickCount();
#else
		struct timeval tv;
		gettimeofday(&tv, NULL);
		starttime = (tv.tv_usec/1000) + (tv.tv_sec*1000);
#endif
	}
	Sprite2D * ret = NULL;
	if(playReversed) {
		int max = (int)link.size()-1;
		for(unsigned int i = 0; i < link.size(); i++) {
			if(link[i] == indices[max-pos]) {
				ret = frames[i];
				break;
			}
		}
	} else {
		for(unsigned int i = 0; i < link.size(); i++) {
			if(link[i] == indices[pos]) {
				ret = frames[i];
				break;
			}
		}
	}
	if(pastLastFrame && playOnce) {
		endReached = true;
		return ret;
	}
#ifdef WIN32
	unsigned long time = GetTickCount();
#else
	struct timeval tv;
	unsigned long time;
	gettimeofday(&tv, NULL);
	time = (tv.tv_usec/1000) + (tv.tv_sec*1000);
#endif
	if((time - starttime) >= (unsigned long)(1000/fps)) {
		pos++;
		starttime = time;
	}
	pos %= frames.size();
	if(pos == (frames.size()-1))
		pastLastFrame = true;
	if(autoSwitchOnEnd && (!endReached)) {
		if(pastLastFrame && (pos == 0)) {
			endReached = true;
		}
	}
	return ret;
}


void Animation::release(void)
{
	delete this;
}
/** Gets the i-th frame */
Sprite2D * Animation::GetFrame(unsigned long i)
{
	if(i >= frames.size())
		return NULL;
	return frames[i];
}

void Animation::SetPalette(Color * Palette)
{
	for(size_t i = 0; i < frames.size(); i++) {
		core->GetVideoDriver()->SetPalette(frames[i], Palette);
	}
}
