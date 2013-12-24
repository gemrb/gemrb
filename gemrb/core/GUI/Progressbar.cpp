/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

#include "GUI/Progressbar.h"

#include "win32def.h"

#include "Interface.h"
#include "Video.h"
#include "GUI/Window.h"

#include <cstring>

namespace GemRB {

Progressbar::Progressbar(const Region& frame, unsigned short KnobStepsCount, bool Clear)
	: Control(frame)
{
	ControlType = IE_GUI_PROGRESSBAR;
	BackGround = NULL;
	BackGround2 = NULL;
	this->Clear = Clear;
	this->KnobStepsCount = KnobStepsCount;
	PBarAnim = NULL;
	PBarCap = NULL;
	KnobXPos = KnobYPos = 0;
	CapXPos = CapYPos = 0;
	ResetEventHandler( EndReached );
}

Progressbar::~Progressbar()
{
	if (!Clear) {
		return;
	}
	core->GetVideoDriver()->FreeSprite( BackGround );
	core->GetVideoDriver()->FreeSprite( BackGround2 );
	delete PBarAnim;
	core->GetVideoDriver()->FreeSprite( PBarCap );
}

/** Draws the Control on the Output Display */
void Progressbar::DrawInternal(Region& rgn)
{
	Sprite2D *bcksprite;

	if((Value >= 100) && KnobStepsCount && BackGround2) {
		bcksprite=BackGround2; //animated progbar end stage
	}
	else {
		bcksprite=BackGround;
	}
	if (bcksprite) {
		core->GetVideoDriver()->BlitSprite( bcksprite,
			rgn.x, rgn.y, true, &rgn );
		if( bcksprite==BackGround2) {
			return; //done for animated progbar
		}
	}

	unsigned int Count;

	if(!KnobStepsCount) {
		//linear progressbar (pst, iwd)
		int w = BackGround2->Width;
		int h = BackGround2->Height;
		//this is the PST/IWD specific part
		Count = Value*w/100;
		Region r( rgn.x + KnobXPos, rgn.y + KnobYPos, Count, h );
		core->GetVideoDriver()->BlitSprite( BackGround2, 
			r.x, r.y, true, &r );

		core->GetVideoDriver()->BlitSprite( PBarCap,
			rgn.x+CapXPos+Count-PBarCap->Width, rgn.y+CapYPos, true );
		return;
	}

	//animated progressbar (bg2)
	Count=Value*KnobStepsCount/100;
	for(unsigned int i=0; i<Count ;i++ ) {
		Sprite2D *Knob = PBarAnim->GetFrame(i);
		core->GetVideoDriver()->BlitSprite( Knob, 0, 0, true );
	}
}

/** Returns the actual Progressbar Position */
unsigned int Progressbar::GetPosition()
{
	return Value;
}

/** Sets the actual Progressbar Position trimming to the Max and Min Values */
void Progressbar::SetPosition(unsigned int pos)
{
	if(pos>100) pos=100;
	if (Value == pos)
		return;
	Value = pos;
	MarkDirty();
}

void Progressbar::UpdateState(const char* VariableName, unsigned int Sum)
{
	if (strnicmp( VarName, VariableName, MAX_VARIABLE_LENGTH )) {
		return;
	}
	SetPosition(Sum);
	MarkDirty();
	if(Value==100)
		RunEventHandler( EndReached );
}

/** Sets the selected image */
void Progressbar::SetImage(Sprite2D* img, Sprite2D* img2)
{
	if (BackGround && Clear)
		core->GetVideoDriver()->FreeSprite( BackGround );
	BackGround = img;
	if (BackGround2 && Clear)
		core->GetVideoDriver()->FreeSprite( BackGround2 );
	BackGround2 = img2;
	MarkDirty();
}

void Progressbar::SetBarCap(Sprite2D* img3)
{
	core->GetVideoDriver()->FreeSprite( PBarCap );
	PBarCap = img3;
}

void Progressbar::SetAnimation(Animation *arg)
{
	delete PBarAnim;
	PBarAnim = arg;
}

void Progressbar::SetSliderPos(int x, int y, int x2, int y2)
{
	KnobXPos=x;
	KnobYPos=y;
	CapXPos=x2;
	CapYPos=y2;
}

bool Progressbar::SetEvent(int eventType, EventHandler handler)
{
	switch (eventType) {
	case IE_GUI_PROGRESS_END_REACHED:
		EndReached = handler;
		break;
	default:
		return false;
	}

	return true;
}

}
