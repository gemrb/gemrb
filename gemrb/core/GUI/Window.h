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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

/**
 * @file Window.h
 * Declares Window, class serving as a container for Control/widget objects
 * and displaying windows in GUI
 * @author The GemRB Project
 */

#ifndef WINDOW_H
#define WINDOW_H

#include "ScrollView.h"
#include "WindowManager.h"

#include <set>

namespace GemRB {

class Control;
class Sprite2D;

/**
 * @class Window
 * Class serving as a container for Control/widget objects
 * and displaying windows in GUI.
 */

class GEM_EXPORT Window : public ScrollView {
public:
	// !!! Keep these synchronized with GUIDefines.py !!!
	enum WindowPosition {
		PosTop = 1,		// top
		PosBottom = 2,	// bottom
		PosVmid = 3,	// top + bottom = vmid
		PosLeft = 4,	// left
		PosRight = 8,	// right
		PosHmid = 12,	// left + right = hmid
		PosCentered = 15// top + bottom + left + right = center
	};
	enum WindowFlags {
		Draggable = 1,			// window can be dragged (by clicking anywhere in its frame) a la PST radial menu
		Borderless = 2,			// window will not draw the ornate borders (see WindowManager::DrawWindows)
		DestroyOnClose = 4,		// window will be deleted on close rather then hidden and sent to the back
		AlphaChannel = 8,		// Create window with RGBA buffer suitable for creating non rectangular windows
		Modal = 16
	};

private:
	void RecreateBuffer();

	void SubviewAdded(View* view, View* parent);
	void SubviewRemoved(View* view, View* parent);

	void FlagsChanged(unsigned int /*oldflags*/);
	void SizeChanged(const Size&);
	void WillDraw();

	// attempt to set focus to view. return the focused view which is view if success or the currently focused view (if any) on failure
	View* TrySetFocus(View* view);

	inline void DispatchMouseMotion(View*, const MouseEvent&);
	inline void DispatchMouseDown(View*, const MouseEvent& me, unsigned short /*Mod*/);
	inline void DispatchMouseUp(View*, const MouseEvent& me, unsigned short /*Mod*/);
	
	inline bool DispatchKey(View*, const Event&);
	
	bool OnKeyPress(const KeyboardEvent& /*Key*/, unsigned short /*Mod*/);
	
	bool OnMouseDrag(const MouseEvent&);
	void OnMouseLeave(const MouseEvent& /*me*/, const DragOp*);
	
	ViewScriptingRef* CreateScriptingRef(ScriptingId id, ResRef group);

public:
	Window(const Region& frame, WindowManager& mgr);
	~Window();

	void Close();
	void Focus();
	bool DisplayModal(WindowManager::ModalShadow = WindowManager::ShadowNone);

	/** Sets 'view' as Focused */
	void SetFocused(View* view);
	View* FocusedView() const { return focusView; }

	void SetPosition(WindowPosition);
	String TooltipText() const;
	Sprite2D* Cursor() const;
	bool IsDisabledCursor() const;

	/** Redraw controls of the same group */
	void RedrawControls(const char* VarName, unsigned int Sum);

	bool DispatchEvent(const Event&);
	bool RegisterHotKeyCallback(Holder<EventMgr::EventCallback>, KeyboardKey key);
	bool UnRegisterHotKeyCallback(Holder<EventMgr::EventCallback>, KeyboardKey key);
	
	bool InHandler() const;
	bool IsOpaque() const { return (Flags()&AlphaChannel) == 0; }
	bool HitTest(const Point& p) const;

private: // Private attributes
	typedef std::map<KeyboardKey, Holder<EventMgr::EventCallback> > KeyMap;

	std::set<Control*> Controls;
	KeyMap HotKeys;

	View* focusView; // keyboard focus
	View* trackingView; // out of bounds mouse tracking
	View* hoverView; // view the mouse was last over
	Holder<DragOp> drag;
	unsigned long lastMouseMoveTime;

	VideoBuffer* backBuffer;
	WindowManager& manager;
};

}

#endif
