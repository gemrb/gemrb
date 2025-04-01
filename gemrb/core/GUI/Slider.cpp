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

#include "GUI/Slider.h"

#include "Interface.h"

namespace GemRB {

Slider::Slider(const Region& frame, Point pos,
	       short KnobStep, unsigned short KnobStepsCount)
	: Control(frame), KnobPos(pos)
{
	ControlType = IE_GUI_SLIDER;
	this->KnobStep = KnobStep;
	this->KnobStepsCount = KnobStepsCount;
	SetValueRange(1);
}

/** Draws the Control on the Output Display */
void Slider::DrawSelf(const Region& rgn, const Region& /*clip*/)
{
	Point p = rgn.origin + KnobPos;
	p.x += Pos * KnobStep;

	switch (State) {
		case IE_GUI_SLIDER_KNOB:
			VideoDriver->BlitSprite(Knob, p);
			break;

		case IE_GUI_SLIDER_GRABBEDKNOB:
			VideoDriver->BlitSprite(GrabbedKnob, p);
			break;
	}
}

/** Returns the actual Slider Position */
unsigned int Slider::GetPosition() const
{
	return Pos;
}

/** Sets the actual Slider Position trimming to the Max and Min Values */
void Slider::SetPosition(unsigned int pos)
{
	if (pos <= KnobStepsCount) {
		Pos = pos;
	}
	if (IsDictBound()) {
		core->GetDictionary().Set(DictVariable(), pos * GetValue());
	}
	MarkDirty();
}

void Slider::SetPosition(const Point& p)
{
	int mx = KnobPos.x;
	int xmx = p.x - mx;
	unsigned int oldPos = Pos;

	if (p.x < mx) {
		SetPosition(0);
	} else {
		int befst = xmx / KnobStep;
		if (befst >= KnobStepsCount) {
			SetPosition(KnobStepsCount - 1);
		} else {
			short aftst = befst + KnobStep;
			if ((xmx - befst * KnobStep) < (aftst * KnobStep - xmx)) {
				SetPosition(befst);
			} else {
				SetPosition(aftst);
			}
		}
	}

	if (oldPos != Pos) {
		PerformAction(Control::ValueChange);
	}
}

/** Refreshes a slider which is associated with VariableName */
void Slider::UpdateState(value_t Sum)
{
	Sum /= GetValue();
	if (Sum <= KnobStepsCount) {
		Pos = Sum;
	}
}

/** Sets the selected image */
void Slider::SetImage(unsigned char type, const Holder<Sprite2D>& img)
{
	switch (type) {
		case IE_GUI_SLIDER_KNOB:
			Knob = img;
			break;

		case IE_GUI_SLIDER_GRABBEDKNOB:
			GrabbedKnob = img;
			break;

		case IE_GUI_SLIDER_BACKGROUND:
			SetBackground(img);
			break;
	}
	MarkDirty();
}

/** Mouse Button Down */
bool Slider::OnMouseDown(const MouseEvent& me, unsigned short /*Mod*/)
{
	MarkDirty();
	int mx = (KnobPos.x + (Pos * KnobStep) - Knob->Frame.x);
	int my = (KnobPos.y - Knob->Frame.y);
	int Mx = (mx + Knob->Frame.w);
	int My = (my + Knob->Frame.h);

	Point p = ConvertPointFromScreen(me.Pos());

	if ((p.x >= mx) && (p.y >= my)) {
		if ((p.x <= Mx) && (p.y <= My)) {
			State = IE_GUI_SLIDER_GRABBEDKNOB;
			return true;
		}
	}

	SetPosition(ConvertPointFromScreen(me.Pos()));
	return true;
}

/** Mouse Button Up */
bool Slider::OnMouseUp(const MouseEvent& /*me*/, unsigned short /*Mod*/)
{
	if (State != IE_GUI_SLIDER_KNOB) {
		MarkDirty();
	}
	State = IE_GUI_SLIDER_KNOB;
	return true;
}

/** Mouse Over Event */
bool Slider::OnMouseDrag(const MouseEvent& me)
{
	MarkDirty();

	State = IE_GUI_SLIDER_GRABBEDKNOB;
	unsigned int oldPos = Pos;
	SetPosition(ConvertPointFromScreen(me.Pos()));

	if (oldPos != Pos) {
		Control::OnMouseDrag(me);
	}
	return true;
}

}
