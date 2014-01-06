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

#include "GUI/Control.h"
#include "GUI/TextArea.h"

#include "exports.h"

#include "Sprite2D.h"

namespace GemRB {

// !!! Keep these synchronized with GUIDefines.py !!!
#define IE_GUI_SCROLLBAR_ON_CHANGE  0x07000000

#define IE_GUI_SCROLLBAR_DEFAULT      0x00000040   // mousewheel triggers it

enum IE_SCROLLBAR_IMAGE_TYPE {
	IE_GUI_SCROLLBAR_UP_UNPRESSED = 0,
	IE_GUI_SCROLLBAR_UP_PRESSED,
	IE_GUI_SCROLLBAR_DOWN_UNPRESSED,
	IE_GUI_SCROLLBAR_DOWN_PRESSED,
	IE_GUI_SCROLLBAR_TROUGH,
	IE_GUI_SCROLLBAR_SLIDER,

	IE_SCROLLBAR_IMAGE_COUNT
};

#define UP_PRESS	 0x0001
#define DOWN_PRESS   0x0010
#define SLIDER_GRAB  0x0100

/**
 * @class ScrollBar
 * Widget displaying scrollbars for paging in long text windows
 */
class GEM_EXPORT ScrollBar : public Control {
protected:
	void DrawInternal(Region& drawFrame);
	bool HasBackground();
public:
	ScrollBar(const Region& frame, Sprite2D*[IE_SCROLLBAR_IMAGE_COUNT]);
	~ScrollBar(void);
	/** safe method to get the height of a frame */
	int GetFrameHeight(int frame) const;
	/**sets position, updates associated stuff */
	void SetPos(ieDword NewPos);
	void SetPosForY(short y);
	void ScrollUp();
	void ScrollDown();
	double GetStep();
	/** refreshes scrollbar if associated with VarName */
	void UpdateState(const char* VarName, unsigned int Sum);
private: //Private attributes
	/** Images for drawing the Scroll Bar */
	Sprite2D* Frames[IE_SCROLLBAR_IMAGE_COUNT];
	/** Range of the slider in pixels. The height - buttons - slider */
	ieDword SliderRange;
	/** a pixel position between 0 and SliderRange*/
	unsigned short SliderYPos;
	/** Item Index */
	unsigned short Pos;
	/** slider y delta between steps */
	double stepPx;
	/** Scroll Bar Status */
	unsigned short State;
public:
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
	/** Mouse Wheel Scroll Event */
	void OnMouseWheelScroll(short x, short y);
	/** Set handler for specified event */
	bool SetEvent(int eventType, EventHandler handler);
	/** OnChange Scripted Event Function Name */
	EventHandler ScrollBarOnChange;
};

}

#endif
