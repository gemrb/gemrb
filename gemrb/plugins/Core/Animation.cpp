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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/Animation.cpp,v 1.31 2005/05/28 19:02:11 avenger_teambg Exp $
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

extern Interface* core;

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
	autofree = false;
	Palette = NULL;
	Flags = A_ANI_ACTIVE;
	fps = 15;
	endReached = false;
	ScriptName = NULL;
	//behaviour flags
	playReversed = false;
}

Animation::~Animation(void)
{
	Video *video = core->GetVideoDriver();
	
	if (autofree) {
		for (unsigned int i = 0; i < indicesCount; i++) {
			video->FreeSprite( frames[i] );
		}
	}
	free(frames);
	if (Palette) video->FreePalette(Palette);
	if (ScriptName) free(ScriptName);
}

void Animation::SetPos(unsigned int index)
{
	if (index<indicesCount) {
		pos=index;
	}
}

/* when adding NULL, it means we already added a frame of index */
void Animation::AddFrame(Sprite2D* frame, unsigned int index)
{
	if (index>=indicesCount) {
		printf("You tried to write past a buffer in animation, BAD!\n");
		abort();
	}
	if (autofree && frames[index]) {
		core->GetVideoDriver()->FreeSprite(frames[index]);
	}
	frames[index]=frame;

	int x = -frame->XPos;
	int y = -frame->YPos;
	int w = frame->Width - frame->XPos;
	int h = frame->Height - frame->YPos;
	if (x < animArea.x) {
		animArea.x = x;
	}
	if (y < animArea.y) {
		animArea.y = y;
	}
	if (w > animArea.w) {
		animArea.w = w;
	}
	if (h > animArea.h) {
		animArea.h = h;
	}
}

int Animation::GetCurrentFrame()
{
	if (playReversed)
		return indicesCount-pos-1;
	return pos;
}

Sprite2D* Animation::NextFrame(void)
{
	if (!Flags&A_ANI_ACTIVE) {
		printf("Frame fetched while animation is inactive!\n");
		return NULL;
	}
	if (starttime == 0) {
		GetTime( starttime );
	}
	Sprite2D* ret;
	if (playReversed) {
		ret = frames[indicesCount-pos-1];
	}
	else {
		ret = frames[pos];
	}

	if (endReached && (Flags&A_ANI_PLAYONCE) ) {
		return ret;
	}
	unsigned long time;
	GetTime(time);

	if (( time - starttime ) >= ( unsigned long ) ( 1000 / fps )) {
		pos++;
		starttime = time;
	}	
	if (pos >= indicesCount ) {
		pos = 0;
		endReached = true;
	}
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

void Animation::SetPalette(Color* Pal, bool local)
{
	Video *video = core->GetVideoDriver();
	if (local) {
		if (!Palette) {
			Palette=video->GetPalette( frames[0] );
		}
		if (Pal)
			memcpy( Palette, Pal, sizeof(Palette) );
	} else {
	//no idea if this part will ever be used
		for (size_t i = 0; i < indicesCount; i++) {
			video->SetPalette( frames[i], Pal );
		}
	}
}

void Animation::MirrorAnimation()
{
	Video *video = core->GetVideoDriver();

	for (size_t i = 0; i < indicesCount; i++) {
		Sprite2D * tmp = frames[i];
		frames[i] = video->MirrorSpriteHorizontal( tmp, true );
		// we free the original sprite if it was referenced only by us
		if (autofree) {
			video->FreeSprite(tmp);
		}
	}
	// This function will create independent sprites we have to free
	autofree = true;
}

void Animation::SetScriptName(const char *name)
{
	if (!ScriptName) ScriptName=(char *) malloc(33);
	strnuprcpy( ScriptName, name, 32 );
}
