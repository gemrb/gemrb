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

#include "GUI/Control.h"

#include <vector>

namespace GemRB {

class Sprite2D;

// !!! Keep these synchronized with GUIDefines.py !!!
// Window Flags
#define WF_FLOAT		1 //floating window
#define WF_BORDERLESS	2 //doesnt draw the window frame

// Window position anchors (actually flags for WindowSetPos())
#define WINDOW_TOPLEFT       0x00
#define WINDOW_CENTER        0x01
#define WINDOW_ABSCENTER     0x02
#define WINDOW_RELATIVE      0x04
#define WINDOW_SCALE         0x08
#define WINDOW_BOUNDED       0x10

/**
 * @class Window
 * Class serving as a container for Control/widget objects 
 * and displaying windows in GUI.
 */

class GEM_EXPORT Window : public View {
public:
	enum Visibility {
		INVALID		= -1,
		INVISIBLE	= 0,
		VISIBLE		= 1,
		GRAYED		= 2,
		FRONT		= 3
	};

protected:
	void SubviewAdded(View* view, View* parent);
	void SubviewRemoved(View* view, View* parent);

	void DrawSelf(Region drawFrame, const Region& clip);

	bool TrySetFocus(View*);

	static Sprite2D* WinFrameEdge(int edge);

public: 
	Window(const Region& frame);
	~Window();


	bool OnSpecialKeyPress(unsigned char key);

	/** Returns the Control associated with the function key index, valid indices are 0-11 */
	Control* GetFunctionControl(int x);
	/** Returns the Control by Index */
	Control* GetControlAtIndex(size_t) const;
	Control* GetControlById(ieDword id) const;
	/** Returns the number of Controls */
	size_t GetControlCount() const;
	int GetControlIndex(ieDword id) const;
	/** Returns the Control which should get mouse scroll events */
	Control* GetScrollControl() const;

	/** Sets 'ctrl' as Focused */
	void SetFocused(Control* ctrl);
	void SetVisibility(Visibility);
	Visibility WindowVisibility() { return visibility; }
	/** Returns last focused control */
	Control* GetFocus() const;
	/** Returns last mouse event focused control */
	Control* GetMouseFocus() const;
	View* FocusedView() const { return focusView; }

	/** Redraw controls of the same group */
	void RedrawControls(const char* VarName, unsigned int Sum);

	void DispatchMouseOver(const Point&);
	void DispatchMouseDown(const Point&, unsigned short /*Button*/, unsigned short /*Mod*/);
	void DispatchMouseUp(const Point&, unsigned short /*Button*/, unsigned short /*Mod*/);
	void DispatchMouseWheelScroll(short x, short y);

	ScriptingRef* ScriptingReference(ScriptingId id) { WindowID = id; return new ScriptingObject<Window>(*this, "Window", id); }

public: //Public attributes
	/** WinPack */
	char WindowPack[10];
	/** Window ID */
	unsigned short WindowID;

	int Cursor;
	bool FunctionBar;
private: // Private attributes
	/** Controls Array */
	std::vector< Control*> Controls;
	View* focusView; // keyboard focus
	View* trackingView; // out of bounds mouse tracking
	View* hoverView; // view the mouse was last over
	Holder<DragOp> drag;
	unsigned long lastMouseMoveTime;
	Visibility visibility;
};

}

#endif
