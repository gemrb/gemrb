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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/AnimationFactory.cpp,v 1.11 2005/03/31 10:06:27 avenger_teambg Exp $
 *
 */

#include "../../includes/win32def.h"
#include "AnimationFactory.h"
#include "Interface.h"

extern Interface* core;

AnimationFactory::AnimationFactory(const char* ResRef)
	: FactoryObject( ResRef, IE_BAM_CLASS_ID )
{
	FLTable = NULL;
}

AnimationFactory::~AnimationFactory(void)
{
	for (unsigned int i = 0; i < frames.size(); i++) {
		core->GetVideoDriver()->FreeSprite( frames[i] );
	}
	if (FLTable) {
		free( FLTable );
	}
}

void AnimationFactory::AddFrame(Sprite2D* frame)
{
	frames.push_back( frame );
}

void AnimationFactory::AddCycle(CycleEntry cycle)
{
	cycles.push_back( cycle );
}

void AnimationFactory::LoadFLT(unsigned short* buffer, int count)
{
	if (FLTable) {
		free( FLTable );
	}
	FLTable = ( unsigned short * ) malloc( count * sizeof( unsigned short ) );
	memcpy( FLTable, buffer, count * sizeof( unsigned short ) );
}

Animation* AnimationFactory::GetCycle(unsigned char cycle)
{
	if (cycle >= cycles.size()) {
		return NULL;
	}
	int ff = cycles[cycle]. FirstFrame, lf = ff + cycles[cycle].FramesCount;
	Animation* anim = new Animation( cycles[cycle].FramesCount );
	int c = 0;
	for (int i = ff; i < lf; i++) {
		anim->AddFrame( frames[FLTable[i]], c++ );
	}
	return anim;
}

/* returns the required frame of the named cycle, cycle defaults to 0 */
Sprite2D* AnimationFactory::GetFrame(unsigned short index, unsigned char cycle)
{
	if (cycle >= cycles.size()) {
		return NULL;
	}
	int ff = cycles[cycle]. FirstFrame, fc = cycles[cycle].FramesCount;
	if(index >= fc) {
		return NULL;
	}
	return frames[FLTable[ff+index]];
}
