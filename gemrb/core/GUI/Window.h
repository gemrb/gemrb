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

// Window Flags
#define WF_CHANGED  1     //window changed
#define WF_FRAME    2     //window has frame
#define WF_FLOAT    4     //floating window
#define WF_CHILD    8     //if invalidated, it invalidates all windows on top of it

// Window position anchors (actually flags for WindowSetPos())
// !!! Keep these synchronized with GUIDefines.py !!!
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
private:
	void DrawBackground(const Region* rgn) const;

public: 
	Window(unsigned short WindowID, unsigned short XPos, unsigned short YPos,
		unsigned short Width, unsigned short Height);
	~Window();
	/** Set the Window's BackGround Image. 
	 * If 'img' is NULL, no background will be set. If the 'clean' parameter is true (default is false) the old background image will be deleted. */
	void SetBackGround(Sprite2D* img, bool clean = false);
	/** Add a Control in the Window */
	void AddControl(Control* ctrl);
	/** This function Draws the Window on the Output Screen */
	void DrawWindow();
	/** Set window frame used to fill screen on higher resolutions*/
	void SetFrame();
	/** Returns the Control associated with the function key index, valid indices are 0-11 */
	Control* GetFunctionControl(int x);
	/** Returns the Control at X,Y Coordinates */
	Control* GetControl(unsigned short x, unsigned short y, bool ignore=0);
	/** Returns the Control by Index */
	Control* GetControl(unsigned short i) const;
	/** Returns the number of Controls */
	unsigned int GetControlCount() const;
	/** Returns true if ctrl is valid and ctrl->ControlID is ID */
	bool IsValidControl(unsigned short ID, Control *ctrl) const;
	/** Deletes the xth. Control */
	Control* RemoveControl(unsigned short i);
	/** Returns the Default Control which may be a button/gamecontrol atm */
	Control* GetDefaultControl(unsigned int ctrltype) const;
	/** Returns the Control which should get mouse scroll events */
	Control* GetScrollControl() const;
	/** Sets 'ctrl' as currently under mouse */
	void SetOver(Control* ctrl);
	/** Returns last control under mouse */
	Control* GetOver() const;
	/** Sets 'ctrl' as Focused */
	void SetFocused(Control* ctrl);
	/** Sets 'ctrl' as mouse event Focused */
	void SetMouseFocused(Control* ctrl);
	/** Returns last focused control */
	Control* GetFocus() const;
	/** Returns last mouse event focused control */
	Control* GetMouseFocus() const;
	/** Redraw all the Window */
	void Invalidate();
	/** Redraw controls of the same group */
	void RedrawControls(const char* VarName, unsigned int Sum);
	/** Links a scrollbar to a text area */
	void Link(unsigned short SBID, unsigned short TAID);
	/** Mouse entered a new control's rectangle */
	void OnMouseEnter(unsigned short x, unsigned short y, Control *ctrl);
	/** Mouse left the current control */
	void OnMouseLeave(unsigned short x, unsigned short y);
	/** Mouse is over the current control */
	void OnMouseOver(unsigned short x, unsigned short y);
public: //Public attributes
	/** WinPack */
	char WindowPack[10];
	/** Window ID */
	unsigned short WindowID;
	/** X Position */
	unsigned short XPos;
	/** Y Position */
	unsigned short YPos;
	/** Width */
	unsigned short Width;
	/** Height */
	unsigned short Height;
	/** Visible value: deleted, invisible, visible, grayed */
	signed char Visible;  //-1,0,1,2
	/** Window flags: Changed, Floating, Framed, Child */
	int Flags;
	int Cursor;
	int DefaultControl[2]; //default enter and cancel
	int ScrollControl;
	bool FunctionBar;
private: // Private attributes
	/** BackGround Image. No BackGround if this variable is NULL. */
	Sprite2D* BackGround;
	/** Controls Array */
	std::vector< Control*> Controls;
	/** Last Control returned by GetControl */
	Control* lastC;
	/** Last Focused Control */
	Control* lastFocus;
	/** Last mouse event Focused Control */
	Control* lastMouseFocus;
	/** Last Control under mouse */
	Control* lastOver;

public:
	void release(void);
};

}

#endif
