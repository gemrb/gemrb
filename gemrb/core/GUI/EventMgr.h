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

/**
 * @file EventMgr.h
 * Declares EventMgr, class distributing events from input devices to GUI windows
 * @author The GemRB Project
 */


#ifndef EVENTMGR_H
#define EVENTMGR_H

#include "exports.h"
#include "Region.h"

#include <vector>

namespace GemRB {

class Control;
class Window;

#define GEM_LEFT		0x81
#define GEM_RIGHT		0x82
#define GEM_UP			0x83
#define GEM_DOWN		0x84
#define GEM_DELETE		0x85
#define GEM_RETURN		0x86
#define GEM_BACKSP		0x87
#define GEM_TAB			0x88
#define GEM_ALT			0x89
#define GEM_HOME		0x8a
#define GEM_END			0x8b
#define GEM_ESCAPE		0x8c
#define GEM_PGUP		0x8d
#define GEM_PGDOWN		0x8e
#define GEM_GRAB		0x8f
#define GEM_FUNCTION1           0x90     //don't use anything between these two and keep a round number
#define GEM_FUNCTION16          0x9f

#define GEM_MOD_SHIFT           1
#define GEM_MOD_CTRL            2
#define GEM_MOD_ALT             4

#define GEM_MOUSEOUT	128

// Mouse buttons
#define GEM_MB_ACTION           1
#define GEM_MB_MENU             4
#define GEM_MB_SCRLUP           8
#define GEM_MB_SCRLDOWN         16

#define GEM_MB_NORMAL           255
#define GEM_MB_DOUBLECLICK      256

#define GEM_RK_DOUBLESPEED      1
#define GEM_RK_DISABLE          2
#define GEM_RK_QUADRUPLESPEED   4

/**
 * @class EventMgr
 * Class distributing events from input devices to GUI windows.
 * The events are pumped into instance of this class from a Video driver plugin
 */

class GEM_EXPORT EventMgr {
private:
	std::vector< Window*> windows;
	std::vector< int> topwin;

	Point dc;
	unsigned long dc_time, dc_delay;
	unsigned long rk_delay, rk_flags;

	/** Function bar window containing function key buttons */
	Window* function_bar;
	/** Last Window focused */
	Window* last_win_focused;
	/** Last Window mouse event focused */
	Window* mouseTrackingWin;
	/** Last Window under Mouse Pointer*/
	Window* last_win_over;
	/** Sets a Window on the Top of the Window Queue */
public:
	EventMgr(void);
	~EventMgr(void);
	/** Adds a Window to the Event Manager */
	void AddWindow(Window* win);
	/** Removes a Window from the Event chain */
	//void DelWindow(unsigned short WindowID, const char *WindowPack);
	void DelWindow(Window* win);
	/** Frees and Removes all the Windows in the Array */
	void Clear();
	/** Call this to change the cursor (moving over windows will change it back) */
	void RefreshCursor(int idx);
	/** Trigger a fake MouseMove event with current coordinates (typically used to refresh cursor) */
	void FakeMouseMove();
	/** BroadCast Mouse Move Event */
	void MouseMove(unsigned short x, unsigned short y);
	/** BroadCast Mouse Move Event */
	void MouseDown(unsigned short x, unsigned short y, unsigned short Button,
		unsigned short Mod);
	/** BroadCast Mouse Move Event */
	void MouseUp(unsigned short x, unsigned short y, unsigned short Button,
		unsigned short Mod);
	/** BroadCast Mouse Scroll Event */
	void MouseWheelScroll( short x, short y);
	/** BroadCast Mouse Idle Event */
	void MouseIdle(unsigned long time);
	/** BroadCast Key Press Event */
	void KeyPress(unsigned char Key, unsigned short Mod);
	/** BroadCast Key Release Event */
	void KeyRelease(unsigned char Key, unsigned short Mod);
	/** Special Ket Press Event */
	void OnSpecialKeyPress(unsigned char Key);
	/** Sets focus to the control of the window */
	void SetFocused(Window *win, Control *ctrl);
	/** Sets mouse event focus to the control of the window */
	void SetMouseFocused(Window *win, Control *ctrl);
	/** Sets the maximum accepted doubleclick delay */
	void SetDCDelay(unsigned long t);
	void SetRKDelay(unsigned long t);
	unsigned long GetRKDelay();
	unsigned long SetRKFlags(unsigned long arg, unsigned int op);
	void inline SetFunctionBar(Window *win) { function_bar = win; }
	Control* GetMouseFocusedControl();
	/** Mask of which Mouse Buttons are pressed */
	unsigned char MButtons;
private:
	void SetDefaultFocus(Window *win);
	void SetOnTop(int Index);
	bool ClickMatch(const Point& p, unsigned long thisTime);
};

}

#endif // ! EVENTMGR_H
