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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/EventMgr.h,v 1.14 2004/04/26 15:15:57 edheldil Exp $
 *
 */

#ifndef EVENTMGR_H
#define EVENTMGR_H

#include "Control.h"
#include "WindowMgr.h"
#include <vector>

#define GEM_LEFT		1
#define GEM_RIGHT		2
#define GEM_UP			3
#define GEM_DOWN		4
#define GEM_DELETE		5
#define GEM_RETURN		6
#define GEM_BACKSP		7
#define GEM_TAB			8
#define GEM_ALT			9
#define GEM_MOUSEOUT	128

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
	/** Removes a Window from the Array */
	void DelWindow(unsigned short WindowID);
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
