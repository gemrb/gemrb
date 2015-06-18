/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2015 The GemRB Project
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef __GemRB__WindowManager__
#define __GemRB__WindowManager__

#include "EventMgr.h"

#include <deque>

namespace GemRB {

class Sprite2D;
class Video;
class Window;

typedef std::deque<Window*> WindowList;

class WindowManager {
public:
	// Colors of modal window shadow
	// !!! Keep these synchronized with GUIDefines.py !!!
	enum ModalShadow {
		ShadowNone = 0,
		ShadowGray,
		ShadowBlack
	};

	enum WindowPosition {
		PosTop = 1,
		PosBottom = 2,
		PosVmid = 3,
		PosLeft = 4,
		PosRight = 8,
		PosHmid = 12,
		PosCentered = 15
	};

private:
	WindowList windows;
	WindowList closedWindows; // windows that have been closed. kept around temporarily in case they get reopened

	Video* video;
	Size screen;
	ModalShadow modalShadow;

	EventMgr eventMgr;

private:
	bool IsOpenWindow(Window* win) const;

public:
	WindowManager(Video* vid);
	~WindowManager() {};

	Window* MakeWindow(const Region& rgn);
	void CloseWindow(Window* win);
	bool FocusWindow(Window* win);
	bool MakeModal(Window* win, ModalShadow Shadow = ShadowNone);
	bool IsPresentingModalWindow() const;

	void DrawWindows() const;
	void RedrawAll() const;

	void SetVideoDriver(Video* vid);
	Size ScreenSize() const { return screen; }

	Sprite2D* GetScreenshot(Window* win) const;
	
};


}

#endif /* defined(__GemRB__WindowManager__) */
