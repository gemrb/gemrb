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
	mouseTrackingWin = NULL;
	// Last window we were over. Used to determine MouseEnter and MouseLeave events
	last_win_over = NULL;
	MButtons = 0;

	dc_time = 0;
	dc_delay = 250;
	rk_delay = 250;
	rk_flags = GEM_RK_DISABLE;
}

void EventMgr::SetOnTop(Window* win)
{
	WindowList::iterator it;
	it = std::find(windows.begin(), windows.end(), win);

	if (it != windows.end()) {
		windows.erase(it);
		windows.push_front(win);
	}
	SetDefaultFocus(win);
}

void EventMgr::SetDefaultFocus(Window *win)
{
	if (!last_win_focused) {
		last_win_focused = win;
		last_win_focused->SetFocused(NULL);
	}
	last_win_over = NULL;
}

/** Adds a Window to the Event Manager */
void EventMgr::AddWindow(Window* win)
{
	if (!win) return;

	WindowList::iterator it;
	it = std::find(windows.begin(), windows.end(), win);

	if (it != windows.end()) {
		SetOnTop(win);
	} else {
		windows.push_front(win);
		SetDefaultFocus(win);
	}
}

/** Frees and Removes all the Windows in the Array */
void EventMgr::Clear()
{
	windows.clear();
	last_win_focused = NULL;
	mouseTrackingWin = NULL;
	last_win_over = NULL;
	function_bar = NULL;
}

/** Remove a Window from the array */
void EventMgr::DelWindow(Window *win)
{
	WindowList::iterator it;
	it = std::find(windows.begin(), windows.end(), win);

	if (it == windows.end()) {
		return;
	}

	windows.erase(it);

	if (last_win_focused == win) {
		if (windows.size()) {
			SetFocused(windows.front(), NULL);
		} else {
			last_win_focused = NULL;
		}
	}
	if (mouseTrackingWin == win) {
		mouseTrackingWin = NULL;
	}
	if (last_win_over == win) {
		last_win_over = NULL;
	}
	if (function_bar == win) {
		function_bar = NULL;
	}
}

/** BroadCast Mouse Move Event */
void EventMgr::MouseMove(unsigned short x, unsigned short y)
{
	Point p(x, y);
	GameControl *gc = core->GetGameControl();
	if (gc && (!mouseTrackingWin || mouseTrackingWin == gc->Owner)) {
		gc->OnGlobalMouseMove(p);
	}
	if (mouseTrackingWin) {
		mouseTrackingWin->DispatchMouseOver(mouseTrackingWin->ConvertPointFromScreen(p));
		RefreshCursor(mouseTrackingWin->Cursor);
		return;
	}

	WindowList::iterator it;
	for (it = windows.begin(); it != windows.end(); ++it) {
		Window* win = *it;

		if (!win->WindowVisibility())
			continue;

		if (win->Frame().PointInside(p)) {
			Point winPoint = win->ConvertPointFromScreen(p);
			if (last_win_over && win != last_win_over) {
				last_win_over->OnMouseLeave(winPoint, NULL);
			}
			last_win_over = win;
			win->DispatchMouseOver(winPoint);
			RefreshCursor(win->Cursor);
			break;
		}
	}
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

bool EventMgr::ClickMatch(const Point& p, unsigned long thisTime)
{
	if (dc_time<thisTime) return false;

	return Region(dc, Size(10, 10)).PointInside(p);
}

/** BroadCast Mouse Move Event */
void EventMgr::MouseDown(unsigned short x, unsigned short y, unsigned short Button, unsigned short Mod)
{
	Point p(x, y);

	std::vector< Window*>::iterator m;
	unsigned long thisTime;

	thisTime = GetTickCount();
	if (ClickMatch(p, thisTime)) {
		Button |= GEM_MB_DOUBLECLICK;
		dc = Point(0, 0);
		dc_time = 0;
	} else {
		dc = p;
		dc_time = thisTime+dc_delay;
	}
	MButtons |= Button;

	WindowList::iterator it;
	for (it = windows.begin(); it != windows.end(); ++it) {
		Window* win = *it;

		if (!win->WindowVisibility())
			continue;

		if (win->Frame().PointInside(p)) {
			mouseTrackingWin = win;
			win->DispatchMouseDown(win->ConvertPointFromScreen(p), Button, Mod);
			RefreshCursor(mouseTrackingWin->Cursor);
			return;
		}

		if (win->WindowVisibility() == Window::FRONT) //stop looking further
			break;
	}
}

/** BroadCast Mouse Up Event */
void EventMgr::MouseUp(unsigned short x, unsigned short y, unsigned short Button,
	unsigned short Mod)
{
	MButtons &= ~Button;
	if (mouseTrackingWin == NULL) return;

	Point p = mouseTrackingWin->ConvertPointFromScreen(Point(x, y));
	mouseTrackingWin->DispatchMouseUp(p, Button, Mod);
	mouseTrackingWin = NULL;
}

/** BroadCast Mouse ScrollWheel Event */
void EventMgr::MouseWheelScroll( short x, short y)//these are signed!
{
	if (last_win_over) {
		last_win_over->DispatchMouseWheelScroll(x, y);
	}
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
	if (!last_win_focused)
		return;

	last_win_focused->OnSpecialKeyPress(Key);
}

/** Trigger a fake MouseMove event with current coordinates (typically used to
 *  refresh the cursor without actual user input) */
void EventMgr::FakeMouseMove()
{
	Point p = core->GetVideoDriver()->GetMousePos();
	MouseMove(p.x, p.y);
}

void EventMgr::SetFocused(Window* win, Control* ctrl)
{
	if (!win) return;

	AddWindow(win);
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

Control* EventMgr::GetFocusedControl()
{
	if (mouseTrackingWin) {
		return mouseTrackingWin->GetFocus();
	}
	return NULL;
}

}
