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
#include "Control.h"
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
	Visible = false;
	Changed = true;
}

Window::~Window()
{
	std::vector<Control*>::iterator m = Controls.begin();
	while(Controls.size() != 0) {
		Control * ctrl = (*m);
		delete(ctrl);
		Controls.erase(m);
		m = Controls.begin();
	}
	if(BackGround)
		core->GetVideoDriver()->FreeSprite(BackGround);
	BackGround = NULL;
}
/** Add a Control in the Window */
void Window::AddControl(Control * ctrl)
{
	if(ctrl == NULL)
		return;
	for(int i = 0; i < Controls.size(); i++) {
		if(Controls[i]->ControlID == ctrl->ControlID) {
			delete(Controls[i]);
			Controls[i] = ctrl;
			return;
		}
	}
	Controls.push_back(ctrl);
	ctrl->Owner = this;
	Changed = true;
}
/** Set the Window's BackGround Image. If 'img' is NULL, no background will be set. If the 'clean' parameter is true (default is false) the old background image will be deleted. */
void Window::SetBackGround(Sprite2D * img, bool clean)
{
	if(clean && BackGround) {
		core->GetVideoDriver()->FreeSprite(this->BackGround);
	}
	BackGround = img;
	Changed = true;
}
/** This function Draws the Window on the Output Screen */
void Window::DrawWindow()
{
	Video * video = core->GetVideoDriver();
	if(BackGround && Changed)
		video->BlitSprite(BackGround, XPos, YPos, true);
	std::vector<Control*>::iterator m;
	for(m = Controls.begin(); m != Controls.end(); ++m) {
		(*m)->Draw(XPos, YPos);
	}
	Changed = false;
}

/** Returns the Control at X,Y Coordinates */
Control * Window::GetControl(unsigned short x, unsigned short y)
{
	Control * ctrl = NULL;
	//Check if we are always on the last control
	if((lastC != NULL) && (lastC->ControlType != IE_GUI_LABEL)) {
		if((XPos+lastC->XPos <= x) && (YPos+lastC->YPos <= y)) { //Maybe we are always there
			if((XPos+lastC->XPos+lastC->Width >= x) && (YPos+lastC->YPos+lastC->Height >= y)) { //Yes, we are on the last returned Control
				return lastC;
			}
		}
	}
	std::vector<Control*>::iterator m;
	for(m = Controls.begin(); m != Controls.end(); m++) {
		if((*m)->ControlType == IE_GUI_LABEL)
			continue;
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

/** Redraw all the Window */
void Window::Invalidate()
{
	for(unsigned int i = 0; i < Controls.size(); i++) {
		Controls[i]->Changed = true;
	}
	Changed = true;
}

void Window::RedrawControls(char *VarName, unsigned long Sum)
{
	for(unsigned int i = 0; i < Controls.size(); i++) {
		switch(Controls[i]->ControlType)
                {
			case IE_GUI_BUTTON:
			{
				Button *bt = (Button *) (Controls[i]);
				bt->RedrawButton(VarName, Sum);
				break;
			}
			case IE_GUI_SLIDER:
			{
				Slider *sl =(Slider *) (Controls[i]);
				sl->RedrawSlider(VarName, Sum);
				break;
			}
			case IE_GUI_SCROLLBAR:
			{
				ScrollBar *sb=(ScrollBar *) (Controls[i]);
				sb->RedrawScrollBar(VarName, Sum);
				break;
			}
		}
	}
}

/** Searches for a ScrollBar and a TextArea to link them */
void Window::Link(unsigned short SBID, unsigned short TAID)
{
	ScrollBar *sb = NULL;
	TextArea *ta = NULL;
	std::vector<Control*>::iterator m;
	for(m = Controls.begin(); m != Controls.end(); m++) {
		if((*m)->Owner != this)
			continue;
		if((*m)->ControlType == IE_GUI_SCROLLBAR) {
			if((*m)->ControlID == SBID) {
				sb = (ScrollBar*)(*m);
				if(ta != NULL)
					break;
			}
		}
		else if((*m)->ControlType == IE_GUI_TEXTAREA) {
			if((*m)->ControlID == TAID) {
				ta = (TextArea*)(*m);
				if(sb != NULL)
					break;
			}
		}
	}
	if(sb && ta) {
		sb->ta = ta;
		ta->SetScrollBar(sb);
	}
}
