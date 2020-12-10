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
#include "GUI/Window.h"

#include <cstring>

namespace GemRB {

Progressbar::Progressbar(const Region& frame, unsigned short KnobStepsCount)
: Control(frame)
{
	ControlType = IE_GUI_PROGRESSBAR;

	this->KnobStepsCount = KnobStepsCount;
	PBarAnim = NULL;
	PBarCap = NULL;
	KnobXPos = KnobYPos = 0;
	CapXPos = CapYPos = 0;

	SetValueRange(0, 100);
}

Progressbar::~Progressbar()
{
	delete PBarAnim;
}

/** Draws the Control on the Output Display */
void Progressbar::DrawSelf(Region rgn, const Region& /*clip*/)
{
	ieDword val = GetValue();

	if((val >= 100) && KnobStepsCount && BackGround2) {
		//animated progbar end stage
		core->GetVideoDriver()->BlitSprite( BackGround2.get(), rgn.Origin(), &rgn );
		return; //done for animated progbar
	} else if (BackGround) {
		core->GetVideoDriver()->BlitSprite( BackGround.get(), rgn.Origin(), &rgn );
	}

	unsigned int Count;

	if(!KnobStepsCount) {
		//linear progressbar (pst, iwd)
		const Size& size = BackGround2->Frame.Dimensions();
		//this is the PST/IWD specific part
		Count = val * size.w / 100;
		Region r( rgn.x + KnobXPos, rgn.y + KnobYPos, Count, size.h );
		core->GetVideoDriver()->BlitSprite(BackGround2, r.x, r.y, &r );

		core->GetVideoDriver()->BlitSprite(PBarCap,
			rgn.x+CapXPos+Count-PBarCap->Frame.w, rgn.y+CapYPos);
		return;
	}

	//animated progressbar (bg2)
	Count=val*KnobStepsCount/100;
	for(unsigned int i=0; i<Count ;i++ ) {
		Holder<Sprite2D> Knob = PBarAnim->GetFrame(i);
		core->GetVideoDriver()->BlitSprite(Knob, Point());
	}
}

void Progressbar::UpdateState(unsigned int Sum)
{
	SetValue(Sum);
    if(GetValue() == 100) {
        PerformAction(Action::EndReached);
    }
}

/** Sets the selected image */
void Progressbar::SetImage(Holder<Sprite2D> img, Holder<Sprite2D> img2)
{
	BackGround = img;
	BackGround2 = img2;
	MarkDirty();
}

void Progressbar::SetBarCap(Holder<Sprite2D> img3)
{
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

}
