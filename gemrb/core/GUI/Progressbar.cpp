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

#include "GUI/Window.h"

#include <cstring>
#include <utility>


namespace GemRB {

Progressbar::Progressbar(const Region& frame, unsigned short KnobStepsCount)
	: Control(frame), KnobStepsCount(KnobStepsCount)
{
	ControlType = IE_GUI_PROGRESSBAR;

	SetValueRange(0, 100);
}

Progressbar::~Progressbar()
{
	delete PBarAnim;
}

bool Progressbar::IsOpaque() const
{
	bool isOpaque = Control::IsOpaque();
	if (!isOpaque) {
		isOpaque = BackGround2 && BackGround2->HasTransparency() == false;
	}
	return isOpaque;
}

/** Draws the Control on the Output Display */
void Progressbar::DrawSelf(const Region& rgn, const Region& /*clip*/)
{
	ieDword val = GetValue();

	if ((val >= 100) && KnobStepsCount && BackGround2) {
		//animated progbar end stage
		VideoDriver->BlitSprite(BackGround2, rgn.origin);
		return; //done for animated progbar
	}

	unsigned int Count;

	if (!KnobStepsCount) {
		//linear progressbar (pst, iwd)
		const Size& size = BackGround2->Frame.size;
		//this is the PST/IWD specific part
		Count = val * size.w / 100;
		Region r(rgn.origin + KnobPos, Size(Count, size.h));
		VideoDriver->BlitSprite(BackGround2, r.origin, &r);

		Point p = rgn.origin + CapPos;
		p.x += Count - PBarCap->Frame.w_get();
		VideoDriver->BlitSprite(PBarCap, p);
		return;
	}

	//animated progressbar (bg2)
	Count = val * KnobStepsCount / 100;
	for (unsigned int i = 0; i < Count && PBarAnim; i++) {
		Holder<Sprite2D> Knob = PBarAnim->GetFrame(i);
		VideoDriver->BlitSprite(Knob, Point());
	}
}

void Progressbar::UpdateState(value_t Sum)
{
	if (Sum == INVALID_VALUE) return;

	if (GetValue() == 100) {
		PerformAction(Action::EndReached);
	}
}

/** Sets the selected image */
void Progressbar::SetImages(Holder<Sprite2D> bg, Holder<Sprite2D> cap)
{
	BackGround2 = std::move(bg);
	PBarCap = std::move(cap);
	MarkDirty();
}

void Progressbar::SetAnimation(Animation* arg)
{
	delete PBarAnim;
	PBarAnim = arg;
}

void Progressbar::SetSliderPos(const Point& knob, const Point& cap)
{
	KnobPos = knob;
	CapPos = cap;
}

}
