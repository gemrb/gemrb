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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/Window.cpp,v 1.42 2005/06/22 15:55:26 avenger_teambg Exp $
 *
 */

#include "../../includes/win32def.h"
#include "Window.h"
#include "Control.h"
#include "Interface.h"
#include "MapControl.h"

//extern Interface* core;

Window::Window(unsigned short WindowID, unsigned short XPos,
	unsigned short YPos, unsigned short Width, unsigned short Height)
{
	this->WindowID = WindowID;
	this->XPos = XPos;
	this->YPos = YPos;
	this->Width = Width;
	this->Height = Height;
	this->BackGround = NULL;
	lastC = NULL;
	lastFocus = NULL;
	Visible = WINDOW_INVISIBLE;
	Changed = true;
	Floating = false;
	Cursor = IE_CURSOR_NORMAL;
	DefaultControl = -1;

	Frame = false;
}

Window::~Window()
{
	std::vector< Control*>::iterator m = Controls.begin();
	while (Controls.size() != 0) {
		Control* ctrl = ( *m );
		delete( ctrl );
		Controls.erase( m );
		m = Controls.begin();
	}
	if (BackGround) {
		core->GetVideoDriver()->FreeSprite( BackGround );
	}
	BackGround = NULL;
}
/** Add a Control in the Window */
void Window::AddControl(Control* ctrl)
{
	if (ctrl == NULL) {
		return;
	}
	ctrl->Owner = this;
	for (size_t i = 0; i < Controls.size(); i++) {
		if (Controls[i]->ControlID == ctrl->ControlID) {
			delete( Controls[i] );
			Controls[i] = ctrl;
			Invalidate();
			return;
		}
	}
	Controls.push_back( ctrl );
	Invalidate();
}
/** Set the Window's BackGround Image. If 'img' is NULL, no background will be set. If the 'clean' parameter is true (default is false) the old background image will be deleted. */
void Window::SetBackGround(Sprite2D* img, bool clean)
{
	if (clean && BackGround) {
		core->GetVideoDriver()->FreeSprite( this->BackGround );
	}
	BackGround = img;
	Invalidate();
}
/** This function Draws the Window on the Output Screen */
void Window::DrawWindow()
{
	Video* video = core->GetVideoDriver();
	Region clip( XPos, YPos, Width, Height );
	if (Changed && Frame) {
		Region screen( 0, 0, core->Width, core->Height );
		video->SetClipRect( NULL );
		Color black = { 0, 0, 0, 255 };
		video->DrawRect( screen, black );
		if (core->WindowFrames[0])
			video->BlitSprite( core->WindowFrames[0], 0, 0, true );
		if (core->WindowFrames[1])
			video->BlitSprite( core->WindowFrames[1], core->Width - core->WindowFrames[1]->Width, 0, true );
		if (core->WindowFrames[2])
			video->BlitSprite( core->WindowFrames[2], (core->Width - core->WindowFrames[2]->Width) / 2, 0, true );
		if (core->WindowFrames[3])
			video->BlitSprite( core->WindowFrames[3], (core->Width - core->WindowFrames[3]->Width) / 2, core->Height - core->WindowFrames[3]->Height, true );
	}
	video->SetClipRect( &clip );
	if (BackGround && (Changed || Floating)) {
		video->BlitSprite( BackGround, XPos, YPos, true );
	}
	std::vector< Control*>::iterator m;
	for (m = Controls.begin(); m != Controls.end(); ++m) {
		( *m )->Draw( XPos, YPos );
	}
	if (Changed && (Visible == WINDOW_GRAYED) ) {
		Color black = { 0, 0, 0, 128 };
		video->DrawRect(clip, black);
	}
	video->SetClipRect( NULL );
	Changed = false;
}

/** Set window frame used to fill screen on higher resolutions*/
void Window::SetFrame()
{
	Frame = (Width < core->Width) || (Height < core->Height);
	Invalidate();

}

/** Returns the Control at X,Y Coordinates */
Control* Window::GetControl(unsigned short x, unsigned short y)
{
	Control* ctrl = NULL;

	//Check if we are still on the last control
	if (( lastC != NULL )) {
		if (( XPos + lastC->XPos <= x ) 
		    && ( YPos + lastC->YPos <= y )
		    && ( XPos + lastC->XPos + lastC->Width >= x )
		    && ( YPos + lastC->YPos + lastC->Height >= y )
		    && ! lastC->IsPixelTransparent (x - XPos - lastC->XPos, y - YPos - lastC->YPos)) {
			//Yes, we are on the last returned Control
			return lastC;
		}
	}
	std::vector< Control*>::iterator m;
	for (m = Controls.begin(); m != Controls.end(); m++) {
		if (( XPos + ( *m )->XPos <= x ) 
		    && ( YPos + ( *m )->YPos <= y )
		    && ( XPos + ( *m )->XPos + ( *m )->Width >= x ) 
		    && ( YPos + ( *m )->YPos + ( *m )->Height >= y )
		    && ! ( *m )->IsPixelTransparent (x - XPos - ( *m )->XPos, y - YPos - ( *m )->YPos)) {
			ctrl = *m;
			break;
		}
	}
	lastC = ctrl;
	return ctrl;
}

/** Sets 'ctrl' as Focused */
void Window::SetFocused(Control* ctrl)
{
	if (lastFocus != NULL) {
		lastFocus->hasFocus = false;
	}
	lastFocus = ctrl;
	lastFocus->hasFocus = true;
	lastFocus->Changed = true;
}

Control* Window::GetControl(unsigned short i)
{
	if (i < Controls.size()) {
		return Controls[i];
	}
	return NULL;
}

void Window::DelControl(unsigned short i)
{
	if (i < Controls.size() ) {
		delete Controls[i];
		Controls.erase(Controls.begin()+i);
	}
	lastC=NULL;
	Invalidate();
}

Control* Window::GetDefaultControl()
{
	if (!Controls.size()) {
		return NULL;
	}
	return GetControl( DefaultControl );
}

void Window::release(void)
{
	delete this;
}

/** Redraw all the Window */
void Window::Invalidate()
{
	DefaultControl = -1;
	for (unsigned int i = 0; i < Controls.size(); i++) {
		if (!Controls[i]) {
			continue;
		}
		Controls[i]->Changed = true;
		switch (Controls[i]->ControlType) {
			case IE_GUI_BUTTON:
				if (!( Controls[i]->Flags & IE_GUI_BUTTON_DEFAULT ))
					break;
				//falling through
			case IE_GUI_GAMECONTROL:
				DefaultControl = i;
				break;
			default: ;
		}
	}
	Changed = true;
}

void Window::RedrawControls(char* VarName, unsigned int Sum)
{
	for (unsigned int i = 0; i < Controls.size(); i++) {
		switch (Controls[i]->ControlType) {
			case IE_GUI_MAP:
			{
				MapControl *mc = ( MapControl* ) (Controls[i]);
				mc->RedrawMapControl( VarName, Sum );
				
				break;
			}
			case IE_GUI_BUTTON:
			{
				Button* bt = ( Button* ) ( Controls[i] );
				bt->RedrawButton( VarName, Sum );
				break;
			}
			case IE_GUI_TEXTAREA:
			{
				TextArea* pb = ( TextArea* ) ( Controls[i] );
				pb->RedrawTextArea( VarName, Sum );
				break;
			}
			case IE_GUI_PROGRESSBAR:
			{
				Progressbar* pb = ( Progressbar* ) ( Controls[i] );
				pb->RedrawProgressbar( VarName, Sum );
				break;
			}
			case IE_GUI_SLIDER:
			{
				Slider* sl = ( Slider* ) ( Controls[i] );
				sl->RedrawSlider( VarName, Sum );
				break;
			}
			case IE_GUI_SCROLLBAR:
			{
				ScrollBar* sb = ( ScrollBar* ) ( Controls[i] );
				sb->RedrawScrollBar( VarName, Sum );
				break;
			}
		}
	}
}

/** Searches for a ScrollBar and a TextArea to link them */
void Window::Link(unsigned short SBID, unsigned short TAID)
{
	ScrollBar* sb = NULL;
	TextArea* ta = NULL;
	std::vector< Control*>::iterator m;
	for (m = Controls.begin(); m != Controls.end(); m++) {
		if (( *m )->Owner != this)
			continue;
		if (( *m )->ControlType == IE_GUI_SCROLLBAR) {
			if (( *m )->ControlID == SBID) {
				sb = ( ScrollBar * ) ( *m );
				if (ta != NULL)
					break;
			}
		} else if (( *m )->ControlType == IE_GUI_TEXTAREA) {
			if (( *m )->ControlID == TAID) {
				ta = ( TextArea * ) ( *m );
				if (sb != NULL)
					break;
			}
		}
	}
	if (sb && ta) {
		sb->ta = ta;
		ta->SetScrollBar( sb );
	}
}
