/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

#include "GUI/EventMgr.h"

#include "GUI/GameControl.h"

#include "win32def.h"
#include "ie_cursors.h"

#include "Game.h"
#include "Interface.h"
#include "KeyMap.h"
#include "Video.h"
#include "GUI/Window.h"

namespace GemRB {

EventMgr::EventMgr(void)
{
	// Function bar window (for function keys)
	function_bar = NULL;
	// Last window focused for keyboard events
	last_win_focused = NULL;	
	// Last window focused for mouse events (eg, with a click). Used to determine MouseUp events
	last_win_mousefocused = NULL;
	// Last window we were over. Used to determine MouseEnter and MouseLeave events
	last_win_over = NULL;
	MButtons = 0;
	dc_x = 0;
	dc_y = 0;
	dc_time = 0;
	dc_delay = 250;
	rk_delay = 250;
	rk_flags = GEM_RK_DISABLE;
	focusLock = NULL;
}

EventMgr::~EventMgr(void)
{
}

void EventMgr::SetOnTop(int Index)
{
	std::vector< int>::iterator t;
	for (t = topwin.begin(); t != topwin.end(); ++t) {
		if (( *t ) == Index) {
			topwin.erase( t );
			break;
		}
	}
	if (topwin.size() != 0) {
			topwin.insert( topwin.begin(), Index );
	} else {
		topwin.push_back( Index );
	}
}

void EventMgr::SetDefaultFocus(Window *win)
{
	if (!last_win_focused) {
		last_win_focused = win;
		last_win_focused->SetFocused(last_win_focused->GetControl(0));
	}
	last_win_over = NULL;
}

/** Adds a Window to the Event Manager */
void EventMgr::AddWindow(Window* win)
{
	unsigned int i;

	if (win == NULL) {
		return;
	}
	bool found = false;
	for (i = 0; i < windows.size(); i++) {
		if (windows[i] == win) {
			goto ok;
		}
		if(windows[i]==NULL) {
			windows[i] = win;
ok:
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
	SetDefaultFocus(win);
}
/** Frees and Removes all the Windows in the Array */
void EventMgr::Clear()
{
	topwin.clear();
	windows.clear();
	last_win_focused = NULL;
	last_win_mousefocused = NULL;
	last_win_over = NULL;
	function_bar = NULL;
}

/** Remove a Window from the array */
void EventMgr::DelWindow(Window *win)
{
	bool focused = (last_win_focused == win);
	if (focused) {
		last_win_focused = NULL;
	}
	if (last_win_mousefocused == win) {
		last_win_mousefocused = NULL;
	}
	if (last_win_over == win) {
		last_win_over = NULL;
	}
	if (function_bar == win) {
		function_bar = NULL;
	}

	if (windows.size() == 0) {
		return;
	}

	int pos = -1;
	std::vector< Window*>::iterator m;
	for (m = windows.begin(); m != windows.end(); ++m) {
		pos++;
		if ( (*m) == win) {
			(*m) = NULL;
			std::vector< int>::iterator t;
			for (t = topwin.begin(); t != topwin.end(); ++t) {
				if ( (*t) == pos) {
					topwin.erase( t );
					if (focused && topwin.size() > 0) {
						//revert focus to new top window
						SetFocused(windows[topwin[0]], NULL);
					}
					return;
				}
			}
			Log(WARNING, "EventManager", "Couldn't delete window!");
		}
	}
}

/** BroadCast Mouse Move Event */
void EventMgr::MouseMove(unsigned short x, unsigned short y)
{
	if (windows.size() == 0) {
		return;
	}
	if (!last_win_focused) {
		return;
	}

	GameControl *gc = core->GetGameControl();
	if (gc && (!focusLock || focusLock == gc)) {
		// for scrolling
		gc->OnGlobalMouseMove(x, y);
	}
	if (last_win_mousefocused && focusLock) {
		last_win_mousefocused->OnMouseOver(x, y);
		RefreshCursor(last_win_mousefocused->Cursor);
		return;
	}
	std::vector< int>::iterator t;
	std::vector< Window*>::iterator m;
	for (t = topwin.begin(); t != topwin.end(); ++t) {
		m = windows.begin();
		m += ( *t );
		Window *win = *m;
		if (win == NULL)
			continue;
		if (!win->Visible)
			continue;
		if (( win->XPos <= x ) && ( win->YPos <= y )) {
			//Maybe we are on the window, let's check
			if (( win->XPos + win->Width >= x ) &&
				( win->YPos + win->Height >= y )) {
				//Yes, we are on the Window
				//Let's check if we have a Control under the Mouse Pointer
				Control* ctrl = win->GetControl( x, y, true );
				//look for the low priority flagged controls (mostly static labels)
				if (ctrl == NULL) {
					ctrl = win->GetControl( x, y, false );
				}
				if (win != last_win_over || ctrl != win->GetOver()) {
					// Remove tooltip if mouse moved to different control
					core->DisplayTooltip( 0, 0, NULL );
					if (last_win_over) {
						last_win_over->OnMouseLeave( x, y );
					}
					last_win_over = win;
					win->OnMouseEnter( x, y, ctrl );
				}
				if (ctrl != NULL) {
					win->OnMouseOver( x, y );
				}
				RefreshCursor(win->Cursor);
				return;
			}
		}
		//stop going further
		if (( *m )->Visible == WINDOW_FRONT)
			break;
	}
	core->DisplayTooltip( 0, 0, NULL );
}

void EventMgr::RefreshCursor(int idx)
{
	Video *video = core->GetVideoDriver();
	if (idx&IE_CURSOR_GRAY) {
		video->SetMouseGrayed(true);
	} else {
		video->SetMouseGrayed(false);
	}
	idx &= IE_CURSOR_MASK;
	video->SetCursor( core->Cursors[idx], VID_CUR_UP );
	video->SetCursor( core->Cursors[idx ^ 1], VID_CUR_DOWN );
}

bool EventMgr::ClickMatch(unsigned short x, unsigned short y, unsigned long thisTime)
{
	if (dc_x+10<x) return false;
	if (dc_x>x+10) return false;
	if (dc_y+10<y) return false;
	if (dc_y>y+10) return false;
	if (dc_time<thisTime) return false;
	return true;
}

/** BroadCast Mouse Move Event */
void EventMgr::MouseDown(unsigned short x, unsigned short y, unsigned short Button,
	unsigned short Mod)
{
	std::vector< int>::iterator t;
	std::vector< Window*>::iterator m;
	Control *ctrl;
	unsigned long thisTime;

	thisTime = GetTickCount();
	if (ClickMatch(x, y, thisTime)) {
		Button |= GEM_MB_DOUBLECLICK;
		dc_x = 0;
		dc_y = 0;
		dc_time = 0;
	} else {
		dc_x = x;
		dc_y = y;
		dc_time = thisTime+dc_delay;
	}
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
				ctrl = ( *m )->GetControl( x, y, true );
				if (!ctrl) {
					ctrl = ( *m )->GetControl( x, y, false);
				}
				last_win_mousefocused = *m;
				if (ctrl != NULL) {
					last_win_mousefocused->SetMouseFocused( ctrl );
					ctrl->OnMouseDown( x - last_win_mousefocused->XPos - ctrl->XPos, y - last_win_mousefocused->YPos - ctrl->YPos, Button, Mod );
					focusLock = ctrl;
					RefreshCursor(last_win_mousefocused->Cursor);
					return;
				}
			}
		}
		if (( *m )->Visible == WINDOW_FRONT) //stop looking further
			break;
	}
	
	if ((Button == GEM_MB_SCRLUP || Button == GEM_MB_SCRLDOWN) && last_win_mousefocused) {
		ctrl = last_win_mousefocused->GetScrollControl();
		if (ctrl) {
			ctrl->OnMouseDown( x - last_win_mousefocused->XPos - ctrl->XPos, y - last_win_mousefocused->YPos - ctrl->YPos, Button, Mod );
		}
	}

	if (last_win_mousefocused) {
		last_win_mousefocused->SetMouseFocused(NULL);
	}
}

/** BroadCast Mouse Up Event */
void EventMgr::MouseUp(unsigned short x, unsigned short y, unsigned short Button,
	unsigned short Mod)
{
	focusLock = NULL;
	MButtons &= ~Button;
	Control *last_ctrl_mousefocused = GetMouseFocusedControl();
	if (last_ctrl_mousefocused == NULL) return;
	last_ctrl_mousefocused->OnMouseUp( x - last_win_mousefocused->XPos - last_ctrl_mousefocused->XPos,
		y - last_win_mousefocused->YPos - last_ctrl_mousefocused->YPos, Button, Mod );
}

/** BroadCast Mouse ScrollWheel Event */
void EventMgr::MouseWheelScroll( short x, short y)//these are signed!
{
	Control *ctrl = GetMouseFocusedControl();
	if (ctrl) {
		ctrl->OnMouseWheelScroll( x, y);
	}
}

/** BroadCast Mouse Idle Event */
void EventMgr::MouseIdle(unsigned long /*time*/)
{
	if (last_win_over == NULL) return;
	Control *ctrl = last_win_over->GetOver();
	if (ctrl == NULL) return;
	ctrl->DisplayTooltip();
}

/** BroadCast Key Press Event */
void EventMgr::KeyPress(unsigned char Key, unsigned short Mod)
{
	if (last_win_focused == NULL) return;
	Control *ctrl = last_win_focused->GetFocus();
	if (!ctrl || !ctrl->OnKeyPress( Key, Mod )) {
		// FIXME: need a better way to determine when to call ResolveKey/SetHotKey
		if (core->GetGameControl()
			&& !MButtons // checking for drag actions
			&& !core->IsPresentingModalWindow()
			&& !core->GetKeyMap()->ResolveKey(Key, 0)) {
			core->GetGame()->SetHotKey(toupper(Key));
		}
		//this is to refresh changing mouse cursors should the focus change)
		FakeMouseMove();
	}
}

/** BroadCast Key Release Event */
void EventMgr::KeyRelease(unsigned char Key, unsigned short Mod)
{
	if (last_win_focused == NULL) return;
	if (Key == GEM_GRAB) {
		core->GetVideoDriver()->ToggleGrabInput();
		return;
	}
	Control *ctrl = last_win_focused->GetFocus();
	if (ctrl == NULL) return;
	ctrl->OnKeyRelease( Key, Mod );
}

/** Special Key Press Event */
void EventMgr::OnSpecialKeyPress(unsigned char Key)
{
	if (!last_win_focused) {
		return;
	}
	Control *ctrl = NULL;

	// tab shows tooltips
	if (Key == GEM_TAB) {
		if (last_win_over != NULL) {
			Control *ctrl = last_win_over->GetOver();
			if (ctrl != NULL) {
				ctrl->DisplayTooltip();
			}
		}
	}
	//the default control will get only GEM_RETURN
	else if (Key == GEM_RETURN) {
		ctrl = last_win_focused->GetDefaultControl(0);
	}
	//the default cancel control will get only GEM_ESCAPE
	else if (Key == GEM_ESCAPE) {
		ctrl = last_win_focused->GetDefaultControl(1);
	} else if (Key >= GEM_FUNCTION1 && Key <= GEM_FUNCTION16) {
		if (function_bar) {
			ctrl = function_bar->GetFunctionControl(Key-GEM_FUNCTION1);
		} else {
			ctrl = last_win_focused->GetFunctionControl(Key-GEM_FUNCTION1);
		}
	}

	//if there was no default button set, then the current focus will get it (except function keys)
	if (!ctrl) {
		if (Key<GEM_FUNCTION1 || Key > GEM_FUNCTION16) {
			ctrl = last_win_focused->GetFocus();
		}
	}
	//if one is under focus, use the default scroll focus
	if (!ctrl) {
		if (Key == GEM_UP || Key == GEM_DOWN) {
			ctrl = last_win_focused->GetScrollControl();
		}
	}
	if (ctrl) {
		switch (ctrl->ControlType) {
			//scrollbars will receive only mousewheel events
			case IE_GUI_SCROLLBAR:
				if (Key != GEM_UP && Key != GEM_DOWN) {
					return;
				}
				break;
			//buttons will receive only GEM_RETURN
			case IE_GUI_BUTTON:
				if (Key >= GEM_FUNCTION1 && Key <= GEM_FUNCTION16) {
					//fake mouse button
					ctrl->OnMouseDown(0,0,GEM_MB_ACTION,0);
					ctrl->OnMouseUp(0,0,GEM_MB_ACTION,0);
					return;
				}
				if (Key != GEM_RETURN && Key!=GEM_ESCAPE) {
					return;
				}
				break;
				// shouldnt be any harm in sending these events to any control
		}
		ctrl->OnSpecialKeyPress( Key );
	}
}

/** Trigger a fake MouseMove event with current coordinates (typically used to
 *  refresh the cursor without actual user input) */
void EventMgr::FakeMouseMove()
{
	int x, y;
	core->GetVideoDriver()->GetMousePos(x, y);
	MouseMove((unsigned short) x, (unsigned short) y);
}

void EventMgr::SetFocused(Window *win, Control *ctrl)
{
	last_win_focused = win;
	last_win_focused->SetFocused(ctrl);
	//this is to refresh changing mouse cursors should the focus change)
	FakeMouseMove();
}

void EventMgr::SetDCDelay(unsigned long t)
{
	dc_delay = t;
}

void EventMgr::SetRKDelay(unsigned long t)
{
	rk_delay = t;
}

unsigned long EventMgr::GetRKDelay()
{
	if (rk_flags&GEM_RK_DISABLE) return (unsigned long) ~0;
	if (rk_flags&GEM_RK_DOUBLESPEED) return rk_delay/2;
	if (rk_flags&GEM_RK_QUADRUPLESPEED) return rk_delay/4;
	return rk_delay;
}

unsigned long EventMgr::SetRKFlags(unsigned long arg, unsigned int op)
{
	unsigned long tmp = rk_flags;
	switch (op) {
		case BM_SET: tmp = arg; break;
		case BM_OR: tmp |= arg; break;
		case BM_NAND: tmp &= ~arg; break;
		case BM_XOR: tmp ^= arg; break;
		case BM_AND: tmp &= arg; break;
		default: tmp = 0; break;
	}
	rk_flags=tmp;
	return rk_flags;
}

Control* EventMgr::GetMouseFocusedControl()
{
	if (last_win_mousefocused) {
		return last_win_mousefocused->GetMouseFocus();
	}
	return NULL;
}
}
