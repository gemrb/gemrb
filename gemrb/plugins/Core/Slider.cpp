/***************************************************************************
                          Slider.cpp  -  description
                             -------------------
    begin                : lun ott 13 2003
    copyright            : (C) 2003 by GemRB Developement Team
    email                : Balrog994@yahoo.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "win32def.h"
#include "Slider.h"
#include "Interface.h"

extern Interface * core;

Slider::Slider(short KnobXPos, short KnobYPos, short KnobStep, unsigned short KnobStepsCount, bool Clear)
{
	this->KnobXPos = KnobXPos;
	this->KnobYPos = KnobYPos;
	this->KnobStep = KnobStep;
	this->KnobStepsCount = KnobStepsCount;
	Knob = NULL;
	GrabbedKnob = NULL;
	BackGround = NULL;
	this->Clear = Clear;
	Status = IE_GUI_SLIDER_KNOB;
	Pos = 0;
}

Slider::~Slider()
{
	if(!Clear)
		return;
	if(Knob)
		core->GetVideoDriver()->FreeSprite(Knob);
	if(GrabbedKnob)
		core->GetVideoDriver()->FreeSprite(GrabbedKnob);
	if(BackGround)
		core->GetVideoDriver()->FreeSprite(BackGround);
}

/** Draws the Control on the Output Display */
void Slider::Draw(unsigned short x, unsigned short y)
{
	if(BackGround) {
		if((BackGround->Width < Width) || (BackGround->Height < Height)) {
			core->GetVideoDriver()->BlitTiled(Region(x+XPos, y+YPos, Width, Height), BackGround, true);
		}
		else 
			core->GetVideoDriver()->BlitSprite(BackGround, x+XPos, y+YPos, true);
	}
	switch(Status) {
		case IE_GUI_SLIDER_KNOB:
			{
				core->GetVideoDriver()->BlitSprite(Knob, x+XPos+KnobXPos+(Pos*KnobStep), y+YPos+KnobYPos, true);
			}
		break;

		case IE_GUI_SLIDER_GRABBEDKNOB:
			{
				core->GetVideoDriver()->BlitSprite(GrabbedKnob, x+XPos+KnobXPos+(Pos*KnobStep), y+YPos+KnobYPos, true);
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
	if(pos <= KnobStepsCount)
		Pos = pos;
}
/** Sets the selected image */
void Slider::SetImage(unsigned char type, Sprite2D * img)
{
	switch(type) {
		case IE_GUI_SLIDER_KNOB:
			if(Knob && Clear)
				core->GetVideoDriver()->FreeSprite(Knob);
			Knob = img;
		break;

		case IE_GUI_SLIDER_GRABBEDKNOB:
			if(GrabbedKnob && Clear)
				core->GetVideoDriver()->FreeSprite(GrabbedKnob);
			GrabbedKnob = img;
		break;

		case IE_GUI_SLIDER_BACKGROUND:
			if(BackGround && Clear)
				core->GetVideoDriver()->FreeSprite(BackGround);
			BackGround = img;
		break;
	}
}
