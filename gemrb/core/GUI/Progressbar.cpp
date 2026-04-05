// SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "GUI/Progressbar.h"

#include "Animation.h"
#include "Interface.h"

#include <utility>


namespace GemRB {

Progressbar::Progressbar(const Region& frame, unsigned short KnobStepsCount)
	: Control(frame), KnobStepsCount(KnobStepsCount)
{
	ControlType = IE_GUI_PROGRESSBAR;

	SetValueRange(0, 100);
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
		p.x += Count - PBarCap->Frame.w;
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

void Progressbar::SetAnimation(Holder<Animation> arg)
{
	PBarAnim = arg;
}

void Progressbar::SetSliderPos(const Point& knob, const Point& cap)
{
	KnobPos = knob;
	CapPos = cap;
}

}
