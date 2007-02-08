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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/AnimationFactory.cpp,v 1.14 2007/02/08 20:12:12 avenger_teambg Exp $
 *
 */

#include "../../includes/win32def.h"
#include "AnimationFactory.h"
#include "Interface.h"
#include "Video.h"

extern Interface* core;

AnimationFactory::AnimationFactory(const char* ResRef)
	: FactoryObject( ResRef, IE_BAM_CLASS_ID )
{
	FLTable = NULL;
	FrameData = NULL;
}

AnimationFactory::~AnimationFactory(void)
{
	for (unsigned int i = 0; i < frames.size(); i++) {
		core->GetVideoDriver()->FreeSprite( frames[i] );
	}
	if (FLTable)
		free( FLTable);
	// TODO: add assert here on refcount (which needs to be added...)
	if (FrameData)
		free( FrameData);
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
	//FLTable = new unsigned short[count];
	FLTable = (unsigned short *) malloc(count * sizeof( unsigned short ) );
	memcpy( FLTable, buffer, count * sizeof( unsigned short ) );
}

void AnimationFactory::SetFrameData(unsigned char* FrameData)
{
	this->FrameData = FrameData;
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
	Sprite2D* spr = frames[FLTable[ff+index]];
	spr->RefCount++;
	return spr;
}

Sprite2D* AnimationFactory::GetFrameWithoutCycle(unsigned short index)
{
	if(index >= frames.size()) {
		return NULL;
	}
	Sprite2D* spr = frames[index];
	spr->RefCount++;
	return spr;
}

Sprite2D* AnimationFactory::GetPaperdollImage(ieDword *Colors,
		Sprite2D *&Picture2, unsigned int type)
{
	if (frames.size()<2) {
		return NULL;
	}

	Video* video = core->GetVideoDriver();
	Picture2 = video->DuplicateSprite(frames[1]);
	if (Colors) {
		Palette* palette = video->GetPalette(Picture2);
		palette->SetupPaperdollColours(Colors, type);
		video->SetPalette(Picture2, palette);
		palette->Release();
	}

	Picture2->XPos = (short)frames[1]->XPos;
	Picture2->YPos = (short)frames[1]->YPos - 80;


	Sprite2D* spr = core->GetVideoDriver()->DuplicateSprite(frames[0]);
	if (Colors) {
		Palette* palette = video->GetPalette(spr);
		palette->SetupPaperdollColours(Colors, type);
		video->SetPalette(spr, palette);
		palette->Release();
	}

	spr->XPos = (short)frames[0]->XPos;
	spr->YPos = (short)frames[0]->YPos;

	//don't free pixels, createsprite stores it in spr

	return spr;
}
