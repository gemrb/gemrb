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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/EventMgr.h,v 1.20 2005/08/14 19:07:26 avenger_teambg Exp $
 *
 */

#ifndef EVENTMGR_H
#define EVENTMGR_H

#include "Control.h"
#include "WindowMgr.h"
#include <vector>

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

#define GEM_MOUSEOUT	128

// Mouse buttons
#define GEM_MB_ACTION           1
#define GEM_MB_MENU             4

#ifdef WIN32

#ifdef GEM_BUILD_DLL
#define GEM_EXPORT __declspec(dllexport)
#else
#define GEM_EXPORT __declspec(dllimport)
#endif

#else
#define GEM_EXPORT
#endif

class GEM_EXPORT EventMgr {
private:
	std::vector< Window*> windows;
	std::vector< int> topwin;

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
	/** BroadCast Mouse Move Event */
	void MouseMove(unsigned short x, unsigned short y);
	/** BroadCast Mouse Move Event */
	void MouseDown(unsigned short x, unsigned short y, unsigned char Button,
		unsigned short Mod);
	/** BroadCast Mouse Move Event */
	void MouseUp(unsigned short x, unsigned short y, unsigned char Button,
		unsigned short Mod);
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
	/** Mask of which Mouse Buttons are pressed */
	unsigned char MButtons;
private:
	/** Last Window under Mouse Pointer*/
	Window* lastW;
	/** Last Focused Control */
	Control* lastF;
	/** Last Active (entered) Control */
	Control* last_ctrl_over;
	/** Sets a Window on the Top of the Window Queue */
	void SetOnTop(int Index)
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
};

#endif
