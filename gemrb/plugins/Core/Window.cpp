/***************************************************************************
                          Window.cpp  -  description
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
#include "Window.h"
#include "Interface.h"

extern Interface * core;

Window::Window(unsigned short WindowID, unsigned short XPos, unsigned short YPos, unsigned short Width, unsigned short Height)
{
	this->WindowID = WindowID;
	this->XPos = XPos;
	this->YPos = YPos;
	this->Width = Width;
	this->Height = Height;
	this->BackGround = NULL;
	lastC = NULL;
	lastFocus = NULL;
}

Window::~Window()
{
	std::vector<Control*>::iterator m = Controls.begin();
	for(int i = 0; Controls.size() != 0;) {
		delete(*m);
		Controls.erase(m);
		m = Controls.begin();
	}
}
/** Add a Control in the Window */
void Window::AddControl(Control * ctrl)
{
	if(ctrl != NULL)
		Controls.push_back(ctrl);
}
/** Set the Window's BackGround Image. If 'img' is NULL, no background will be set. If the 'clean' parameter is true (default is false) the old background image will be deleted. */
void Window::SetBackGround(Sprite2D * img, bool clean)
{
	if(clean && BackGround) {
		core->GetVideoDriver()->FreeSprite(this->BackGround);
	}
	BackGround = img;
}
/** This function Draws the Window on the Output Screen */
void Window::DrawWindow()
{
	Video * video = core->GetVideoDriver();
	if(BackGround)
		video->BlitSprite(BackGround, XPos, YPos, true);
	std::vector<Control*>::iterator m;
	for(m = Controls.begin(); m != Controls.end(); ++m) {
		(*m)->Draw(XPos, YPos);
	}
}

/** Returns the Control at X,Y Coordinates */
Control * Window::GetControl(unsigned short x, unsigned short y)
{
	Control * ctrl = NULL;
	//Check if we are always on the last control
	if(lastC != NULL) {
		if((XPos+lastC->XPos <= x) && (YPos+lastC->YPos <= y)) { //Maybe we are always there
			if((XPos+lastC->XPos+lastC->Width >= x) && (YPos+lastC->YPos+lastC->Height >= y)) { //Yes, we are on the last returned Control
				return lastC;
			}
		}
	}
	std::vector<Control*>::iterator m;
	for(m = Controls.begin(); m != Controls.end(); m++) {
		if((XPos+(*m)->XPos <= x) && (YPos+(*m)->YPos <= y)) { //Maybe we are on this control
			if((XPos+(*m)->XPos+(*m)->Width >= x) && (YPos+(*m)->YPos+(*m)->Height >= y)) { //Yes, we are here
				ctrl = *m;
				break;
			}
		}
	}
	lastC = ctrl;
	return ctrl;
}

/** Sets 'ctrl' as Focused */
void Window::SetFocused(Control * ctrl)
{
	if(lastFocus != NULL)
		lastFocus->hasFocus = false;
	lastFocus = ctrl;
	lastFocus->hasFocus = true;
}

Control * Window::GetControl(unsigned short i)
{
	if(i < Controls.size())
		return Controls[i];
	return NULL;
}

void Window::release(void)
{
	delete this;
}
