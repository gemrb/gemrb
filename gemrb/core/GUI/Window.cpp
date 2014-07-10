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

#include "GUI/Window.h"

#include "GUI/Button.h"
#include "GUI/MapControl.h"
#include "GUI/ScrollBar.h"

#include "win32def.h"

#include "ie_cursors.h"

namespace GemRB {

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
	lastMouseFocus = NULL;
	lastOver = NULL;
	Visible = WINDOW_INVISIBLE;
	Flags = WF_CHANGED;
	Cursor = IE_CURSOR_NORMAL;
	DefaultControl[0] = -1;
	DefaultControl[1] = -1;
	ScrollControl = -1;
	FunctionBar = false;
}

Window::~Window()
{
	std::vector< Control*>::iterator m = Controls.begin();
	while (Controls.size() != 0) {
		Control* ctrl = ( *m );
		delete ctrl;
		Controls.erase( m );
		m = Controls.begin();
	}
	Sprite2D::FreeSprite( BackGround );
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
		Sprite2D::FreeSprite( this->BackGround );
	}
	BackGround = img;
	Invalidate();
}
/** This function Draws the Window on the Output Screen */
void Window::DrawWindow()
{
	if (!Visible) return; // no point in drawing invisible windows
	Video* video = core->GetVideoDriver();
	Region clip( XPos, YPos, Width, Height );
	//Frame && Changed
	if ( (Flags & (WF_FRAME|WF_CHANGED) )== (WF_FRAME|WF_CHANGED) ) {
		Region screen( 0, 0, core->Width, core->Height );
		video->SetClipRect( NULL );
		//removed this?
		video->DrawRect( screen, ColorBlack );
		if (core->WindowFrames[0])
			video->BlitSprite( core->WindowFrames[0], 0, 0, true );
		if (core->WindowFrames[1])
			video->BlitSprite( core->WindowFrames[1], core->Width - core->WindowFrames[1]->Width, 0, true );
		if (core->WindowFrames[2])
			video->BlitSprite( core->WindowFrames[2], (core->Width - core->WindowFrames[2]->Width) / 2, 0, true );
		if (core->WindowFrames[3])
			video->BlitSprite( core->WindowFrames[3], (core->Width - core->WindowFrames[3]->Width) / 2, core->Height - core->WindowFrames[3]->Height, true );
	} else if (clip_regions.size()) {
		// clip drawing (we only do Background right now) for InvalidateForControl
		for (unsigned int i = 0; i < clip_regions.size(); i++) {
			Region fromClip = clip_regions[i];
			Region toClip = fromClip;
			toClip.x += XPos;
			toClip.y += YPos;
			if (BackGround) {
				video->BlitSprite( BackGround, fromClip, toClip);
			}
		}
	}
	clip_regions.clear();
	video->SetClipRect( &clip );
	//Float || Changed
	if (BackGround && (Flags & (WF_FLOAT|WF_CHANGED) ) ) {
		video->BlitSprite( BackGround, XPos, YPos, true );
	}
	std::vector< Control*>::iterator m;
	for (m = Controls.begin(); m != Controls.end(); ++m) {
		( *m )->Draw( XPos, YPos );
	}
	if ( (Flags&WF_CHANGED) && (Visible == WINDOW_GRAYED) ) {
		Color black = { 0, 0, 0, 128 };
		video->DrawRect(clip, black);
	}
	video->SetClipRect( NULL );
	Flags &= ~WF_CHANGED;
}

/** Set window frame used to fill screen on higher resolutions*/
void Window::SetFrame()
{
	if ( (Width < core->Width) || (Height < core->Height) ) {
		Flags|=WF_FRAME;
	}
	Invalidate();
}

Control* Window::GetFunctionControl(int x)
{
	if (!FunctionBar) {
		return NULL;
	}

	std::vector< Control*>::const_iterator m;

	for (m = Controls.begin(); m != Controls.end(); m++) {
		Control *ctrl = *m;
		if ( ctrl->GetFunctionNumber() == x ) return ctrl;
	}
	return NULL;
}

/** Returns the Control at X,Y Coordinates */
Control* Window::GetControl(unsigned short x, unsigned short y, bool ignore)
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
	std::vector< Control*>::const_iterator m;
	for (m = Controls.begin(); m != Controls.end(); m++) {
		if (ignore && (*m)->ControlID&IGNORE_CONTROL) {
			continue;
		}
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

Control* Window::GetOver() const
{
	return lastOver;
}

Control* Window::GetFocus() const
{
	return lastFocus;
}

Control* Window::GetMouseFocus() const
{
	return lastMouseFocus;
}

/** Sets 'ctrl' as Focused */
void Window::SetFocused(Control* ctrl)
{
	if (lastFocus != NULL) {
		lastFocus->SetFocus(false);
	}
	lastFocus = ctrl;
	if (ctrl != NULL) {
		lastFocus->SetFocus(true);
	}
}

/** Sets 'ctrl' as Mouse Focused */
void Window::SetMouseFocused(Control* ctrl)
{
	if (lastMouseFocus != NULL) {
		lastMouseFocus->MarkDirty();
	}
	lastMouseFocus = ctrl;
	if (ctrl != NULL) {
		lastMouseFocus->MarkDirty();
	}
}

unsigned int Window::GetControlCount() const
{
	return Controls.size();
}

Control* Window::GetControl(unsigned short i) const
{
	if (i < Controls.size()) {
		return Controls[i];
	}
	return NULL;
}

bool Window::IsValidControl(unsigned short ID, Control *ctrl) const
{
	size_t i = Controls.size();
	while (i--) {
		if (Controls[i]==ctrl) {
			return ctrl->ControlID==ID;
		}
	}
	return false;
}

Control* Window::RemoveControl(unsigned short i)
{
	if (i < Controls.size() ) {
		Control *ctrl = Controls[i];
		if (ctrl==lastC) {
			lastC=NULL;
		}
		if (ctrl==lastOver) {
			lastOver=NULL;
		}
		if (ctrl==lastFocus) {
			lastFocus=NULL;
		}
		if (ctrl==lastMouseFocus) {
			lastMouseFocus=NULL;
		}
		Controls.erase(Controls.begin()+i);
		Invalidate();
		return ctrl;
	}
	return NULL;
}

Control* Window::GetDefaultControl(unsigned int ctrltype) const
{
	if (!Controls.size()) {
		return NULL;
	}
	if (ctrltype>=2) {
		return NULL;
	}
	return GetControl( (ieWord) DefaultControl[ctrltype] );
}

Control* Window::GetScrollControl() const
{
	if (!Controls.size()) {
		return NULL;
	}
	return GetControl( (ieWord) ScrollControl );
}

void Window::release(void)
{
	Visible = WINDOW_INVALID;
	lastC = NULL;
	lastFocus = NULL;
	lastMouseFocus = NULL;
	lastOver = NULL;
}

/** Redraw all the Window */
void Window::Invalidate()
{
	DefaultControl[0] = -1;
	DefaultControl[1] = -1;
	ScrollControl = -1;
	for (unsigned int i = 0; i < Controls.size(); i++) {
		if (!Controls[i]) {
			continue;
		}
		Controls[i]->MarkDirty();
		switch (Controls[i]->ControlType) {
			case IE_GUI_SCROLLBAR:
				if ((ScrollControl == -1) || (Controls[i]->Flags & IE_GUI_SCROLLBAR_DEFAULT))
					ScrollControl = i;
				break;
			case IE_GUI_BUTTON:
				if (( Controls[i]->Flags & IE_GUI_BUTTON_DEFAULT )) {
					DefaultControl[0] = i;
				}
				if (( Controls[i]->Flags & IE_GUI_BUTTON_CANCEL )) {
					DefaultControl[1] = i;
				}
				break;
				//falling through
			case IE_GUI_GAMECONTROL:
				DefaultControl[0] = i;
				DefaultControl[1] = i;
				break;
			default: ;
		}
	}
	Flags |= WF_CHANGED;
}

/** Redraw enough to update the specified Control */
void Window::InvalidateForControl(Control *ctrl) {
	// TODO: for this to be general-purpose, we should mark anything inside this
	// region with Changed, and also do mass Invalidate() if we overlap with
	// another window, but for now this just clips the *background*, see DrawWindow()
	clip_regions.push_back( Region(ctrl->XPos, ctrl->YPos, ctrl->Width, ctrl->Height) );
}

void Window::RedrawControls(const char* VarName, unsigned int Sum)
{
	for (std::vector<Control *>::iterator c = Controls.begin(); c != Controls.end(); ++c) {
		(*c)->UpdateState( VarName, Sum);
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

void Window::OnMouseEnter(unsigned short x, unsigned short y, Control *ctrl)
{
	lastOver = ctrl;
	if (!lastOver) {
		return;
	}
	lastOver->OnMouseEnter( x - XPos - lastOver->XPos, y - YPos - lastOver->YPos );
}

void Window::OnMouseLeave(unsigned short x, unsigned short y)
{
	if (!lastOver) {
		return;
	}
	lastOver->OnMouseLeave( x - XPos - lastOver->XPos, y - YPos - lastOver->YPos );
	lastOver = NULL;
}

void Window::OnMouseOver(unsigned short x, unsigned short y)
{
	if (!lastOver) {
		return;
	}
	short cx = x - XPos - lastOver->XPos;
	short cy = y - YPos - lastOver->YPos;
	if (cx < 0) {
		cx = 0;
	}
	if (cy < 0) {
		cy = 0;
	}
	lastOver->OnMouseOver(cx, cy);
}

}
