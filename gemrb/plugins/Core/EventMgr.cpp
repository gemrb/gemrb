/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/EventMgr.cpp,v 1.44 2006/12/09 15:00:25 avenger_teambg Exp $
 *
 */

#include "../../includes/win32def.h"
#include "EventMgr.h"
#include "Interface.h"
#include "Video.h"

EventMgr::EventMgr(void)
{
	lastW = NULL;
	lastF = NULL;
	// Last control we were over. Used to determine MouseEnter and MouseLeave events
	last_ctrl_over = NULL;
	MButtons = 0;
}

EventMgr::~EventMgr(void)
{
}

/** Adds a Window to the Event Manager */
void EventMgr::AddWindow(Window* win)
{
	unsigned int i;

	if (win == NULL) {
		return;
	}
	for (i = 0; i < windows.size(); i++) {
		if (windows[i] == win) {
			SetOnTop( i );
			lastW = win;
			lastF = NULL;
			last_ctrl_over = NULL;
			return;
		}
	}
	bool found = false;
	for (i = 0; i < windows.size(); i++) {
		if (windows[i] == NULL) {
			windows[i] = win;
			SetOnTop( i );
			found = true;
			break;
		}
	}
	if (!found) {
		windows.push_back( win );
		if (windows.size() == 1)
			topwin.push_back( 0 );
		else
			SetOnTop( ( int ) windows.size() - 1 );
	}
	lastW = win;
	lastF = NULL;
	last_ctrl_over = NULL;
}
/** Frees and Removes all the Windows in the Array */
void EventMgr::Clear()
{
	topwin.clear();
	windows.clear();
	lastW = NULL;
	lastF = NULL;
	last_ctrl_over = NULL;
}

/** Remove a Window from the array */
void EventMgr::DelWindow(Window *win)
//unsigned short WindowID, const char *WindowPack)
{
	if (windows.size() == 0) {
		return;
	}
	int pos = -1;
	std::vector< Window*>::iterator m;
	for (m = windows.begin(); m != windows.end(); ++m) {
		pos++;
		if ( (*m)==win) {
			if (lastW == ( *m )) {
				lastW = NULL;
			}
			( *m ) = NULL;
			lastF = NULL;
			last_ctrl_over = NULL;
			std::vector< int>::iterator t;
			for (t = topwin.begin(); t != topwin.end(); ++t) {
				if (( *t ) == pos) {
					topwin.erase( t );
					return;
				}
			}
			printMessage("EventManager","Couldn't find window",YELLOW);
		}
	}
}

/** BroadCast Mouse Move Event */
void EventMgr::MouseMove(unsigned short x, unsigned short y)
{
	if (windows.size() == 0) {
		return;
	}
	if (!lastW) {
		return;
	}
	core->DisplayTooltip( 0, 0, NULL );
	std::vector< int>::iterator t;
	std::vector< Window*>::iterator m;
	for (t = topwin.begin(); t != topwin.end(); ++t) {
		m = windows.begin();
		m += ( *t );
		if (( *m ) == NULL)
			continue;
		if (!( *m )->Visible)
			continue;
		if (( ( *m )->XPos <= x ) && ( ( *m )->YPos <= y )) {
			//Maybe we are on the window, let's check
			if (( ( *m )->XPos + ( *m )->Width >= x ) &&
				( ( *m )->YPos + ( *m )->Height >= y )) {
				//Yes, we are on the Window
				//Let's check if we have a Control under the Mouse Pointer
				Control* ctrl = ( *m )->GetControl( x, y, true );
				if (!ctrl) {
					ctrl = ( *m )->GetControl( x, y, false);
				}
				if (ctrl != last_ctrl_over) {
					if (last_ctrl_over)
						last_ctrl_over->OnMouseLeave( x - lastW->XPos - last_ctrl_over->XPos, y - lastW->YPos - last_ctrl_over->YPos );
					if (ctrl)
						ctrl->OnMouseEnter( x - lastW->XPos - ctrl->XPos, y - lastW->YPos - ctrl->YPos );
					last_ctrl_over = ctrl;
				}
				if (ctrl != NULL) {
					ctrl->OnMouseOver( x - lastW->XPos - ctrl->XPos, y - lastW->YPos - ctrl->YPos );
				}
				lastW = *m;
				core->GetVideoDriver()->SetCursor( core->Cursors[( *m )->Cursor], core->Cursors[( ( *m )->Cursor ) ^ 1] );
				return;
			}
		}
		//stop going further
		if (( *m )->Visible==WINDOW_FRONT)
			break;
	}
}

/** BroadCast Mouse Move Event */
void EventMgr::MouseDown(unsigned short x, unsigned short y,
	unsigned char Button, unsigned short Mod)
{
	std::vector< int>::iterator t;
	std::vector< Window*>::iterator m;
	MButtons |= Button;
	for (t = topwin.begin(); t != topwin.end(); ++t) {
		m = windows.begin();
		m += ( *t );
		if (( *m ) == NULL)
			continue;
		if (!( *m )->Visible)
			continue;
		if (( ( *m )->XPos <= x ) && ( ( *m )->YPos <= y )) {
			//Maybe we are on the window, let's check
			if (( ( *m )->XPos + ( *m )->Width >= x ) &&
				( ( *m )->YPos + ( *m )->Height >= y )) {
				//Yes, we are on the Window
				//Let's check if we have a Control under the Mouse Pointer
				Control* ctrl = ( *m )->GetControl( x, y, true );
				if (!ctrl) {
					ctrl = ( *m )->GetControl( x, y, false);
				}
				//printf( "dn: ctrl: %p\n", ctrl );
				if (lastW == NULL)
					lastW = ( *m );
				if (ctrl != NULL) {
					( *m )->SetFocused( ctrl );
					ctrl->OnMouseDown( x - lastW->XPos - ctrl->XPos, y - lastW->YPos - ctrl->YPos, Button, Mod );
					lastF = ctrl;
					//printf( "dn lastF: %p\n", lastF );
				}
				lastW = *m;
				return;
			}
		}
		if (( *m )->Visible==WINDOW_FRONT) //stop looking further
			break;
	}
	if (lastF) {
		lastF->hasFocus = false;
	}
	lastF = NULL;
}
/** BroadCast Mouse Up Event */
void EventMgr::MouseUp(unsigned short x, unsigned short y,
	unsigned char Button, unsigned short Mod)
{
	MButtons &= ~Button;
	//printf( "up lastF: %p\n", lastF );
	if (lastF != NULL) {
		lastF->OnMouseUp( x - lastW->XPos - lastF->XPos, y - lastW->YPos - lastF->YPos, Button, Mod );
	}
}

/** BroadCast Mouse Idle Event */
void EventMgr::MouseIdle(unsigned long /*time*/)
{
	//MButtons &= ~Button;
	if (last_ctrl_over != NULL) {
		last_ctrl_over->DisplayTooltip();
	}
}

/** BroadCast Key Press Event */
void EventMgr::KeyPress(unsigned char Key, unsigned short Mod)
{
	if (lastF) {
		lastF->OnKeyPress( Key, Mod );
	}
}
/** BroadCast Key Release Event */
void EventMgr::KeyRelease(unsigned char Key, unsigned short Mod)
{
	if (lastF) {
		lastF->OnKeyRelease( Key, Mod );
	}
}

/** Special Key Press Event */
void EventMgr::OnSpecialKeyPress(unsigned char Key)
{
	if (lastW) {
		Control* ctrl = lastW->GetDefaultControl();
		if (!ctrl) {
			if (lastF)
				lastF->OnSpecialKeyPress( Key );
			return;
		}
		switch (ctrl->ControlType) {
			case IE_GUI_GAMECONTROL:
				ctrl->OnSpecialKeyPress( Key );
				return;
			case IE_GUI_BUTTON:
				if (Key == GEM_RETURN) {
					ctrl->OnSpecialKeyPress( Key );
					return;
				}
			default:
				if (lastF)
					lastF->OnSpecialKeyPress( Key );
		}
		return;
	}
	if (lastF) {
		lastF->OnSpecialKeyPress( Key );
	}
}

void EventMgr::SetFocused(Window *win, Control *ctrl)
{
	lastW=win;
	lastF=ctrl;
	lastW->SetFocused(lastF);
	//this is to refresh changing mouse cursors should the focus change)
	int x,y;
	core->GetVideoDriver()->GetMousePos(x,y);
	MouseMove((unsigned short) x, (unsigned short) y);
}

