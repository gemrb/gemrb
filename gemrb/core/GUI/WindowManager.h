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

#include "Audio.h"
#include "EventMgr.h"
#include "Resource.h"
#include "Sprite2D.h"
#include "Tooltip.h"
#include "Video/Video.h"

#include "GUI/Window.h"

#include <deque>

namespace GemRB {

class GUIFactory;
class Sprite2D;

using WindowList = std::deque<Window*>;

struct ToolTipData
{
	Tooltip tt;
	tick_t time = 0;
	Holder<SoundHandle> tooltip_sound;
	bool reset = false;
	
	explicit ToolTipData(Tooltip tt)
	: tt(std::move(tt)) {}
};

class GEM_EXPORT WindowManager {
public:
	enum CursorFeedback {
		MOUSE_ALL			= 0,
		MOUSE_NO_CURSOR		= 1,
		MOUSE_NO_TOOLTIPS	= 2,
		MOUSE_NONE			= MOUSE_NO_CURSOR|MOUSE_NO_TOOLTIPS
	} cursorFeedback;

	static Holder<Sprite2D> CursorMouseUp;
	static Holder<Sprite2D> CursorMouseDown;

	Color FadeColor;

	struct HUDLock {
		const WindowManager& wm;

		explicit HUDLock(const WindowManager& wm)
		: wm(wm) {
			wm.video->PushDrawingBuffer(wm.HUDBuf);
		}

		~HUDLock() {
			wm.video->PopDrawingBuffer();
		}
	};

private:
	std::shared_ptr<GUIFactory> guifact;

	WindowList windows;
	WindowList closedWindows; // windows that have been closed. kept around temporarily in case they get reopened

	Region screen; // only a Region for convinience. we dont use x,y
	Window* gameWin;
	Window* hoverWin = nullptr;
	Window* trackingWin = nullptr;

	EventMgr eventMgr;

	PluginHolder<Video> video;
	VideoBufferPtr HUDBuf = nullptr; // heads up display layer. Contains cursors/tooltips/borders and whatever gets drawn via DrawHUD()

	// these are mutable instead of statice because Sprite2Ds must be released before the video driver is unloaded
	mutable ToolTipData tooltip;
	mutable ResRefMap<Holder<Sprite2D>> winframes;

	static tick_t ToolTipDelay;
	static tick_t TooltipTime;

private:
	bool IsOpenWindow(Window* win) const;
	Holder<Sprite2D> WinFrameEdge(int edge) const;

	inline void DrawWindowFrame(BlitFlags flags) const;
	inline void DrawMouse() const;
	// DrawMouse simply calls the following with some position calculations and buffer context changes
	inline void DrawCursor(const Point& pos) const;
	inline void DrawTooltip(Point pos) const;

	Window* NextEventWindow(const Event& event, WindowList::const_iterator& current);
	bool DispatchEvent(const Event&);
	bool HotKey(const Event&) const;

	inline void DestroyClosedWindows();
	void MarkAllDirty() const;

public:
	WindowManager(PluginHolder<Video> vid, std::shared_ptr<GUIFactory>);
	~WindowManager();
	
	WindowManager(const WindowManager&) = delete;
	
	Window* LoadWindow(ScriptingId WindowID, const ScriptingGroup_t& ref, Window::WindowPosition = Window::PosCentered);
	Window* CreateWindow(ScriptingId WindowID, const Region&);
	Window* MakeWindow(const Region& rgn);
	
	void CloseWindow(Window* win);
	void CloseAllWindows() const;

	bool OrderFront(Window* win);
	bool OrderBack(Window* win);
	bool OrderRelativeTo(Window* win, Window* win2, bool front);

	bool FocusWindow(Window* win);
	Window* ModalWindow() const;
	bool PresentModalWindow(Window* win);

	CursorFeedback SetCursorFeedback(CursorFeedback feedback);

	// all drawing will be done directly on the screen until DrawingLock is destoryed
	HUDLock DrawHUD() const;

	/*
	 Drawing is done in layers:
	 1. Game Window is drawn
	 2. Normal Windows are drawn (in order)
	 3. the window frame and modalShield is drawn (if applicable)
	 4. modalWindow is drawn (if applicable)
	 5. cursor and tooltip are drawn (if applicable)
	*/
	void DrawWindows() const;

	Size ScreenSize() const { return screen.size; }

	Holder<Sprite2D> GetScreenshot(Window* win);
	Window* GetGameWindow() const { return gameWin; }
	Window* GetFocusWindow() const;

	static void SetTooltipDelay(int);
};


}

#endif /* defined(__GemRB__WindowManager__) */
