/***************************************************************************
                          Button.cpp  -  description
                             -------------------
    begin                : dom ott 12 2003
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

#include "../../includes/win32def.h"
#include "Button.h"
#include "Interface.h"

extern Interface * core;

Button::Button(bool Clear){
	Unpressed = Pressed = Selected = Disabled = NULL;
	this->Clear = Clear;
	State = IE_GUI_BUTTON_UNPRESSED;
}
Button::~Button(){
	Video * video = core->GetVideoDriver();
	if(Clear) {
		if(Unpressed)
			video->FreeSprite(Unpressed);
		if(Pressed && (Pressed != Unpressed))
			video->FreeSprite(Pressed);
		if(Selected && ((Selected != Pressed) && (Selected != Unpressed)))
			video->FreeSprite(Selected);
		if(Disabled && ((Disabled != Pressed) && (Disabled != Unpressed) && (Disabled != Selected)))
			video->FreeSprite(Disabled);
	}           
}
/** Sets the 'type' Image of the Button to 'img'.
'type' may assume the following values:
- IE_GUI_BUTTON_UNPRESSED
- IE_GUI_BUTTON_PRESSED
- IE_GUI_BUTTON_SELECTED
- IE_GUI_BUTTON_DISABLED */
void Button::SetImage(unsigned char type, Sprite2D * img)
{
	switch(type) {
		case IE_GUI_BUTTON_UNPRESSED:
		{
			if(Unpressed && Clear)
				core->GetVideoDriver()->FreeSprite(Unpressed);
			Unpressed = img;
		}
		break;
		
		case IE_GUI_BUTTON_PRESSED:
    {
			if(Pressed && Clear)
				core->GetVideoDriver()->FreeSprite(Pressed);
			Pressed = img;
		}
		break;
		
		case IE_GUI_BUTTON_SELECTED:
    {
			if(Selected && Clear)
				core->GetVideoDriver()->FreeSprite(Selected);
			Selected = img;
		}
		break;
		
		case IE_GUI_BUTTON_DISABLED:
		{
			if(Disabled && Clear)
				core->GetVideoDriver()->FreeSprite(Disabled);
			Disabled = img;
		}
		break;
	}
}
/** Draws the Control on the Output Display */
void Button::Draw(unsigned short x, unsigned short y)
{
	switch(State) {
		case IE_GUI_BUTTON_UNPRESSED:
		{
			if(Unpressed)
				core->GetVideoDriver()->BlitSprite(Unpressed, x+XPos, y+YPos, true);
		}
		break;
		
		case IE_GUI_BUTTON_PRESSED:
    {
			if(Pressed)
				core->GetVideoDriver()->BlitSprite(Pressed, x+XPos, y+YPos, true);
		}
		break;
		
		case IE_GUI_BUTTON_SELECTED:
    {
			if(Selected)
				core->GetVideoDriver()->BlitSprite(Selected, x+XPos, y+YPos, true);
		}
		break;
		
		case IE_GUI_BUTTON_DISABLED:
		{
			if(Disabled)
				core->GetVideoDriver()->BlitSprite(Disabled, x+XPos, y+YPos, true);
		}
		break;
	}
}
/** Sets the Button State */
void Button::SetState(unsigned char state)
{
	if(state > 3) // If wrong value inserted
		return;
	State = state;
}

/** Mouse Button Down */
void Button::OnMouseDown(unsigned short x, unsigned short y, unsigned char Button, unsigned short Mod)
{
	if(Button == 1) {
		State = IE_GUI_BUTTON_PRESSED;
	}
}
/** Mouse Button Up */
void Button::OnMouseUp(unsigned short x, unsigned short y, unsigned char Button, unsigned short Mod)
{
	State = IE_GUI_BUTTON_UNPRESSED;
}
