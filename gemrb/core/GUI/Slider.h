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
 * @file Slider.h
 * Declares Slider widget for displaying scales and sliders for setting
 * numerical values
 * @author The GemRB Project
 */

#ifndef SLIDER_H
#define SLIDER_H

#include "GUI/Control.h"

#include "exports.h"

#include "Sprite2D.h"

namespace GemRB {

// !!! Keep these synchronized with GUIDefines.py !!!
#define IE_GUI_SLIDER_ON_CHANGE    0x02000000


#define IE_GUI_SLIDER_KNOB        0
#define IE_GUI_SLIDER_GRABBEDKNOB 1
#define IE_GUI_SLIDER_BACKGROUND  2

/**
 * @class Slider
 * Widget displaying sliders or scales for inputting numerical values
 * with a limited range
 */

class GEM_EXPORT Slider : public Control  {
protected:
	/** Draws the Control on the Output Display */
	void DrawInternal(Region& drawFrame);
	bool HasBackground() {return BackGround;}
public:
	Slider(const Region& frame, short KnobXPos, short KnobYPos, short KnobStep, unsigned short KnobStepsCount, bool Clear = false);
	~Slider();
	/** Returns the actual Slider Position */
	unsigned int GetPosition();
	/** Sets the actual Slider Position trimming to the Max and Min Values */
	void SetPosition(unsigned int pos);
	/** Sets the selected image */
	void SetImage(unsigned char type, Sprite2D * img);
	/** Sets the State of the Slider */
	void SetState(int arg) { State=(unsigned char) arg; }
	/** Refreshes a slider which is associated with VariableName */
	void UpdateState(const char *VariableName, unsigned int Sum);

private: // Private attributes
	/** BackGround Image. If smaller than the Control Size, the image will be tiled. */
	Sprite2D * BackGround;
	/** Knob Image */
	Sprite2D * Knob;
	/** Grabbed Knob Image */
	Sprite2D * GrabbedKnob;
	/** Knob Starting X Position */
	short KnobXPos;
	/** Knob Starting Y Position */
	short KnobYPos;
	/** Knob Step Size */
	short KnobStep;
	/** Knob Steps Count */
	unsigned short KnobStepsCount;
	/** If true, on deletion the Slider will destroy the associated images */
	bool Clear;
	/** Actual Knob Status */
	unsigned char State;
	/** Slider Position Value */
	unsigned int Pos;
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
	bool SetEvent(int eventType, ControlEventHandler handler);
	/** OnChange Scripted Event Function Name */
	ControlEventHandler SliderOnChange;
};

}

#endif
