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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

/**
 * @file ScrollBar.h
 * Declares ScrollBar widget for paging in long text windows.
 * This does not include scales and sliders, which are of Slider class.
 * @author The GemRB Project
 */

#ifndef SCROLLBAR_H
#define SCROLLBAR_H

#include "exports.h"
#include "Control.h"
#include "TextArea.h"
#include "Sprite2D.h"

// !!! Keep these synchronized with GUIDefines.py !!!
#define IE_GUI_SCROLLBAR_ON_CHANGE  0x07000000

#define IE_GUI_SCROLLBAR_DEFAULT      0x00000040   // mousewheel triggers it

#define IE_GUI_SCROLLBAR_UP_UNPRESSED   0
#define IE_GUI_SCROLLBAR_UP_PRESSED 	1
#define IE_GUI_SCROLLBAR_DOWN_UNPRESSED 2
#define IE_GUI_SCROLLBAR_DOWN_PRESSED   3
#define IE_GUI_SCROLLBAR_TROUGH 		4
#define IE_GUI_SCROLLBAR_SLIDER 		5

#define UP_PRESS	 0x0001
#define DOWN_PRESS   0x0010
#define SLIDER_GRAB  0x0100

/**
 * @class ScrollBar
 * Widget displaying scrollbars for paging in long text windows
 */

#define SB_RES_COUNT 6

class GEM_EXPORT ScrollBar : public Control {
public:
	ScrollBar(void);
	~ScrollBar(void);
	/**sets position, updates associated stuff */
	void SetPos(int NewPos);
	void ScrollUp();
	void ScrollDown();
	/**redraws scrollbar if associated with VarName */
	void RedrawScrollBar(const char* VarName, int Sum);
	/**/
	void Draw(unsigned short x, unsigned short y);
private: //Private attributes
	/** Images for drawing the Scroll Bar */
	Sprite2D* Frames[SB_RES_COUNT];
	/** Cursor Position */
	unsigned short Pos;
	/** Scroll Bar Status */
	unsigned short State;
	/** Sets the Text of the current control */
	int SetText(const char* string, int pos = 0);
public:
	void SetImage(unsigned char type, Sprite2D* img);
	/** Sets the Maximum Value of the ScrollBar */
	void SetMax(unsigned short Max);
	/** TextArea Associated Control */
	Control* ta;
public: // Public Events
	/** Mouse Button Down */
	void OnMouseDown(unsigned short x, unsigned short y, unsigned short Button,
		unsigned short Mod);
	/** Mouse Button Up */
	void OnMouseUp(unsigned short x, unsigned short y, unsigned short Button,
		unsigned short Mod);
	/** Mouse Over Event */
	void OnMouseOver(unsigned short x, unsigned short y);
	/** Set handler for specified event */
	bool SetEvent(int eventType, const char *handler);
	/** OnChange Scripted Event Function Name */
	EventHandler ScrollBarOnChange;
};

#endif
