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

#include "win32def.h"

#include "Interface.h"
#include "Variables.h"
#include "GUI/Window.h"

#include <cmath>

namespace GemRB {

Slider::Slider(const Region& frame, short KnobXPos, short KnobYPos, short KnobStep,
	unsigned short KnobStepsCount, bool Clear)
	: Control(frame)
{
	ControlType = IE_GUI_SLIDER;
	this->KnobXPos = KnobXPos;
	this->KnobYPos = KnobYPos;
	this->KnobStep = KnobStep;
	this->KnobStepsCount = KnobStepsCount;
	Knob = NULL;
	GrabbedKnob = NULL;
	this->Clear = Clear;
	ResetEventHandler( SliderOnChange );
	State = IE_GUI_SLIDER_KNOB;
	Pos = 0;
	Value = 1;
}

Slider::~Slider()
{
	if (!Clear) {
		return;
	}
	if (Knob) {
		Sprite2D::FreeSprite( Knob );
	}
	if (GrabbedKnob) {
		Sprite2D::FreeSprite( GrabbedKnob );
	}
}

/** Draws the Control on the Output Display */
void Slider::DrawSelf(Region rgn, const Region& /*clip*/)
{
	switch (State) {
		case IE_GUI_SLIDER_KNOB:
			core->GetVideoDriver()->BlitSprite( Knob,
				rgn.x + KnobXPos + ( Pos * KnobStep ),
				rgn.y + KnobYPos, true );
			break;

		case IE_GUI_SLIDER_GRABBEDKNOB:
			core->GetVideoDriver()->BlitSprite( GrabbedKnob,
				rgn.x + KnobXPos + ( Pos * KnobStep ),
				rgn.y + KnobYPos, true );
			break;
	}
}

/** Returns the actual Slider Position */
unsigned int Slider::GetPosition()
{
	return Pos;
}

/** Sets the actual Slider Position trimming to the Max and Min Values */
void Slider::SetPosition(unsigned int pos)
{
	if (pos <= KnobStepsCount) {
		Pos = pos;
	}
	if (VarName[0] != 0) {
		if (!Value)
			Value = 1;
		core->GetDictionary()->SetAt( VarName, pos * Value );
	}
	MarkDirty();
}

/** Refreshes a slider which is associated with VariableName */
void Slider::UpdateState(const char* VariableName, unsigned int Sum)
{
	if (strnicmp( VarName, VariableName, MAX_VARIABLE_LENGTH )) {
		return;
	}
	if (!Value) {
		Value = 1;
	}
	Sum /= Value;
	if (Sum <= KnobStepsCount) {
		Pos = Sum;
	}
	MarkDirty();
}

/** Sets the selected image */
void Slider::SetImage(unsigned char type, Sprite2D* img)
{
	switch (type) {
		case IE_GUI_SLIDER_KNOB:
			if (Knob && Clear)
				Sprite2D::FreeSprite( Knob );
			Knob = img;
			break;

		case IE_GUI_SLIDER_GRABBEDKNOB:
			if (GrabbedKnob && Clear)
				Sprite2D::FreeSprite( GrabbedKnob );
			GrabbedKnob = img;
			break;

		case IE_GUI_SLIDER_BACKGROUND:
			SetBackground(img);
			break;
	}
	MarkDirty();
}

/** Mouse Button Down */
void Slider::OnMouseDown(unsigned short x, unsigned short y, unsigned short /*Button*/,
	unsigned short /*Mod*/)
{
	MarkDirty();
	unsigned int oldPos = Pos;
	int mx = (KnobXPos + ( Pos * KnobStep ) - Knob->XPos);
	int my = (KnobYPos - Knob->YPos);
	int Mx = (mx + Knob->Width);
	int My = (my + Knob->Height);

	if (( x >= mx ) && ( y >= my )) {
		if (( x <= Mx ) && ( y <= My )) {
			State = IE_GUI_SLIDER_GRABBEDKNOB;
		} else {
			int mx = KnobXPos;
			int xmx = x - mx;
			if (x < mx) {
				SetPosition( 0 );
				if (oldPos != Pos) {
					RunEventHandler( SliderOnChange );
				}
				return;
			}
			int befst = xmx / KnobStep;
			if (befst >= KnobStepsCount) {
				SetPosition( KnobStepsCount - 1 );
				if (oldPos != Pos) {
					RunEventHandler( SliderOnChange );
				}
				return;
			}
			int aftst = befst + KnobStep;
			if (( xmx - ( befst * KnobStep ) ) <
				( ( aftst * KnobStep ) - xmx )) {
				SetPosition( befst );
			} else {
				SetPosition( aftst );
			}
			if (oldPos != Pos) {
				RunEventHandler( SliderOnChange );
			}
		}
	} else {
		int mx = KnobXPos;
		int xmx = x - mx;
		if (x < mx) {
			SetPosition( 0 );
			if (oldPos != Pos) {
				RunEventHandler( SliderOnChange );
			}
			return;
		}
		int befst = xmx / KnobStep;
		if (befst >= KnobStepsCount) {
			SetPosition( KnobStepsCount - 1 );
			if (oldPos != Pos) {
				RunEventHandler( SliderOnChange );
			}
			return;
		}
		int aftst = befst + KnobStep;
		if (( xmx - ( befst * KnobStep ) ) < ( ( aftst * KnobStep ) - xmx )) {
			SetPosition( befst );
		} else {
			SetPosition( aftst );
		}
		if (oldPos != Pos) {
			RunEventHandler( SliderOnChange );
		}
	}
}

/** Mouse Button Up */
void Slider::OnMouseUp(unsigned short /*x*/, unsigned short /*y*/, unsigned short /*Button*/,
	unsigned short /*Mod*/)
{
	if (State != IE_GUI_SLIDER_KNOB) {
		MarkDirty();
	}
	State = IE_GUI_SLIDER_KNOB;
}

/** Mouse Over Event */
void Slider::OnMouseOver(unsigned short x, unsigned short /*y*/)
{
	MarkDirty();
	unsigned int oldPos = Pos;
	if (State == IE_GUI_SLIDER_GRABBEDKNOB) {
		int mx = KnobXPos;
		int xmx = x - mx;
		if (x < mx) {
			SetPosition( 0 );
			if (oldPos != Pos) {
				RunEventHandler( SliderOnChange );
			}
			return;
		}
		int befst = xmx / KnobStep;
		if (befst >= KnobStepsCount) {
			SetPosition( KnobStepsCount - 1 );
			if (oldPos != Pos) {
				RunEventHandler( SliderOnChange );
			}
			return;
		}
		short aftst = befst + KnobStep;
		if (( xmx - ( befst * KnobStep ) ) < ( ( aftst * KnobStep ) - xmx )) {
			SetPosition( befst );
		} else {
			SetPosition( aftst );
		}
		if (oldPos != Pos) {
			RunEventHandler( SliderOnChange );
		}
	}
}

/** Sets the slider change event */
bool Slider::SetEvent(int eventType, ControlEventHandler handler)
{
	switch (eventType) {
	case IE_GUI_SLIDER_ON_CHANGE:
		SliderOnChange = handler;
		break;
	default:
		return false;
	}

	return true;
}

}
