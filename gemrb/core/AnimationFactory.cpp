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

#include "win32def.h"

#include "Interface.h"
#include "Sprite2D.h"
#include "Video.h"

namespace GemRB {

AnimationFactory::AnimationFactory(const char* ResRef)
	: FactoryObject( ResRef, IE_BAM_CLASS_ID )
{
	FLTable = NULL;
	FrameData = NULL;
	datarefcount = 0;
}

AnimationFactory::~AnimationFactory(void)
{
	for (unsigned int i = 0; i < frames.size(); i++) {
		core->GetVideoDriver()->FreeSprite( frames[i] );
	}
	if (FLTable)
		free( FLTable);

	// FIXME: track down where sprites are being leaked
	if (datarefcount) {
		Log(ERROR, "AnimationFactory", "AnimationFactory %s has refcount %d", ResRef, datarefcount);
		//assert(datarefcount == 0);
	}
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
	int ff = cycles[cycle].FirstFrame;
	int lf = ff + cycles[cycle].FramesCount;
	Animation* anim = new Animation( cycles[cycle].FramesCount );
	int c = 0;
	for (int i = ff; i < lf; i++) {
		frames[FLTable[i]]->acquire();
		anim->AddFrame( frames[FLTable[i]], c++ );
	}
	return anim;
}

/* returns the required frame of the named cycle, cycle defaults to 0 */
Sprite2D* AnimationFactory::GetFrame(unsigned short index, unsigned char cycle) const
{
	if (cycle >= cycles.size()) {
		return NULL;
	}
	int ff = cycles[cycle]. FirstFrame, fc = cycles[cycle].FramesCount;
	if(index >= fc) {
		return NULL;
	}
	Sprite2D* spr = frames[FLTable[ff+index]];
	spr->acquire();
	return spr;
}

Sprite2D* AnimationFactory::GetFrameWithoutCycle(unsigned short index) const
{
	if(index >= frames.size()) {
		return NULL;
	}
	Sprite2D* spr = frames[index];
	spr->acquire();
	return spr;
}

Sprite2D* AnimationFactory::GetPaperdollImage(ieDword *Colors,
		Sprite2D *&Picture2, unsigned int type) const
{
	if (frames.size()<2) {
		return NULL;
	}

	Video* video = core->GetVideoDriver();
	Picture2 = video->DuplicateSprite(frames[1]);
	if (!Picture2) {
		return NULL;
	}
	if (Colors) {
		Palette* palette = Picture2->GetPalette();
		palette->SetupPaperdollColours(Colors, type);
		Picture2->SetPalette(palette);
		palette->release();
	}

	Picture2->XPos = (short)frames[1]->XPos;
	Picture2->YPos = (short)frames[1]->YPos - 80;


	Sprite2D* spr = core->GetVideoDriver()->DuplicateSprite(frames[0]);
	if (Colors) {
		Palette* palette = spr->GetPalette();
		palette->SetupPaperdollColours(Colors, type);
		spr->SetPalette(palette);
		palette->release();
	}

	spr->XPos = (short)frames[0]->XPos;
	spr->YPos = (short)frames[0]->YPos;

	//don't free pixels, createsprite stores it in spr

	return spr;
}

void AnimationFactory::IncDataRefCount()
{
	++datarefcount;
}

void AnimationFactory::DecDataRefCount()
{
	assert(datarefcount > 0);
	--datarefcount;
}

}
