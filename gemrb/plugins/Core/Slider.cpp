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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/Slider.cpp,v 1.18 2004/02/24 22:20:36 balrog994 Exp $
 *
 */

#include "../../includes/win32def.h"
#include "Slider.h"
#include "Interface.h"

extern Interface* core;

Slider::Slider(short KnobXPos, short KnobYPos, short KnobStep,
	unsigned short KnobStepsCount, bool Clear)
{
	this->KnobXPos = KnobXPos;
	this->KnobYPos = KnobYPos;
	this->KnobStep = KnobStep;
	this->KnobStepsCount = KnobStepsCount;
	Knob = NULL;
	GrabbedKnob = NULL;
	BackGround = NULL;
	this->Clear = Clear;
	SliderOnChange[0] = 0;
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
		core->GetVideoDriver()->FreeSprite( Knob );
	}
	if (GrabbedKnob) {
		core->GetVideoDriver()->FreeSprite( GrabbedKnob );
	}
	if (BackGround) {
		core->GetVideoDriver()->FreeSprite( BackGround );
	}
}

/** Draws the Control on the Output Display */
void Slider::Draw(unsigned short x, unsigned short y)
{
	if (!Changed) {
		return;
	}
	Changed = false;
	if (XPos == 65535) {
		return;
	}
	if (BackGround) {
		if (( BackGround->Width < Width ) || ( BackGround->Height < Height )) {
			core->GetVideoDriver()->BlitTiled( Region( x + XPos, y + YPos,
												Width, Height, 1 ),
										BackGround, true );
		} else {
			Region r( x + XPos, y + YPos, Width, Height );
			core->GetVideoDriver()->BlitSprite( BackGround, x + XPos,
										y + YPos, true, &r );
		}
	}
	switch (State) {
		case IE_GUI_SLIDER_KNOB:
			 {
				core->GetVideoDriver()->BlitSprite( Knob,
											x +
											XPos +
											KnobXPos +
											( Pos * KnobStep ),
											y +
											YPos +
											KnobYPos, true );
			}
			break;

		case IE_GUI_SLIDER_GRABBEDKNOB:
			 {
				core->GetVideoDriver()->BlitSprite( GrabbedKnob,
											x +
											XPos +
											KnobXPos +
											( Pos * KnobStep ),
											y +
											YPos +
											KnobYPos, true );
			}
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
	Changed = true;
}
/** Redraws a slider which is associated with VariableName */
void Slider::RedrawSlider(char* VariableName, int Sum)
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
	Changed = true;
}
/** Sets the selected image */
void Slider::SetImage(unsigned char type, Sprite2D* img)
{
	switch (type) {
		case IE_GUI_SLIDER_KNOB:
			if (Knob && Clear)
				core->GetVideoDriver()->FreeSprite( Knob );
			Knob = img;
			break;

		case IE_GUI_SLIDER_GRABBEDKNOB:
			if (GrabbedKnob && Clear)
				core->GetVideoDriver()->FreeSprite( GrabbedKnob );
			GrabbedKnob = img;
			break;

		case IE_GUI_SLIDER_BACKGROUND:
			if (BackGround && Clear)
				core->GetVideoDriver()->FreeSprite( BackGround );
			BackGround = img;
			break;
	}
	Changed = true;
}

/** Mouse Button Down */
void Slider::OnMouseDown(unsigned short x, unsigned short y,
	unsigned char Button, unsigned short Mod)
{
	Changed = true;
	int oldPos = Pos;
	unsigned short mx = KnobXPos + ( Pos * KnobStep ) - Knob->XPos,
	my = KnobYPos - Knob->YPos;
	unsigned short Mx = mx + Knob->Width, My = my + Knob->Height;
	if (( x >= mx ) && ( y >= my )) {
		if (( x <= Mx ) && ( y <= My )) {
			State = IE_GUI_SLIDER_GRABBEDKNOB;
		} else {
			unsigned short mx = KnobXPos,
			Mx = mx + ( KnobStep * KnobStepsCount );
			unsigned short xmx = x - mx;
			if (x < mx) {
				SetPosition( 0 );
				if (oldPos != Pos) {
					if (SliderOnChange[0] != 0)
						core->GetGUIScriptEngine()->RunFunction( SliderOnChange );
				}
				return;
			}
			unsigned short befst = xmx / KnobStep;
			if (befst >= KnobStepsCount) {
				SetPosition( KnobStepsCount - 1 );
				if (oldPos != Pos) {
					if (SliderOnChange[0] != 0)
						core->GetGUIScriptEngine()->RunFunction( SliderOnChange );
				}
				return;
			}
			unsigned short aftst = befst + KnobStep;
			if (( xmx - ( befst * KnobStep ) ) <
				( ( aftst * KnobStep ) - xmx )) {
				SetPosition( befst );
			} else {
				SetPosition( aftst );
			}
			if (oldPos != Pos) {
				if (SliderOnChange[0] != 0)
					core->GetGUIScriptEngine()->RunFunction( SliderOnChange );
			}
		}
	} else {
		unsigned short mx = KnobXPos, Mx = mx + ( KnobStep * KnobStepsCount );
		unsigned short xmx = x - mx;
		if (x < mx) {
			SetPosition( 0 );
			if (oldPos != Pos) {
				if (SliderOnChange[0] != 0)
					core->GetGUIScriptEngine()->RunFunction( SliderOnChange );
			}
			return;
		}
		unsigned short befst = xmx / KnobStep;
		if (befst >= KnobStepsCount) {
			SetPosition( KnobStepsCount - 1 );
			if (oldPos != Pos) {
				if (SliderOnChange[0] != 0)
					core->GetGUIScriptEngine()->RunFunction( SliderOnChange );
			}
			return;
		}
		unsigned short aftst = befst + KnobStep;
		if (( xmx - ( befst * KnobStep ) ) < ( ( aftst * KnobStep ) - xmx )) {
			SetPosition( befst );
		} else {
			SetPosition( aftst );
		}
		if (oldPos != Pos) {
			if (SliderOnChange[0] != 0)
				core->GetGUIScriptEngine()->RunFunction( SliderOnChange );
		}
	}
}
/** Mouse Button Up */
void Slider::OnMouseUp(unsigned short x, unsigned short y,
	unsigned char Button, unsigned short Mod)
{
	if (State != IE_GUI_SLIDER_KNOB) {
		Changed = true;
	}
	State = IE_GUI_SLIDER_KNOB;
}
/** Mouse Over Event */
void Slider::OnMouseOver(unsigned short x, unsigned short y)
{
	Changed = true;
	int oldPos = Pos;
	if (State == IE_GUI_SLIDER_GRABBEDKNOB) {
		unsigned short mx = KnobXPos; //, Mx = mx + (KnobStep*KnobStepsCount);
		unsigned short xmx = x - mx;
		if (x < mx) {
			SetPosition( 0 );
			if (oldPos != Pos) {
				if (SliderOnChange[0] != 0)
					core->GetGUIScriptEngine()->RunFunction( SliderOnChange );
			}
			return;
		}
		unsigned short befst = xmx / KnobStep;
		if (befst >= KnobStepsCount) {
			SetPosition( KnobStepsCount - 1 );
			if (oldPos != Pos) {
				if (SliderOnChange[0] != 0)
					core->GetGUIScriptEngine()->RunFunction( SliderOnChange );
			}
			return;
		}
		unsigned short aftst = befst + KnobStep;
		if (( xmx - ( befst * KnobStep ) ) < ( ( aftst * KnobStep ) - xmx )) {
			SetPosition( befst );
		} else {
			SetPosition( aftst );
		}
		if (oldPos != Pos) {
			if (SliderOnChange[0] != 0)
				core->GetGUIScriptEngine()->RunFunction( SliderOnChange );
		}
	}
}

/** Sets the Text of the current control */
int Slider::SetText(const char* string, int pos)
{
	return 0;
}
