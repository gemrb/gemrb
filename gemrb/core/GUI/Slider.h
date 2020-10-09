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

#define IE_GUI_SLIDER_KNOB        0
#define IE_GUI_SLIDER_GRABBEDKNOB 1
#define IE_GUI_SLIDER_BACKGROUND  2

/**
 * @class Slider
 * Widget displaying sliders or scales for inputting numerical values
 * with a limited range
 */

class GEM_EXPORT Slider : public Control  {
private:
	/** Draws the Control on the Output Display */
	void DrawSelf(Region drawFrame, const Region& clip);
	
	// set postion pased on a point expressed in local (frame) coordinates
	void SetPosition(const Point& p);

public:
	Slider(const Region& frame, Point KnobPos,
		   short KnobStep, unsigned short KnobStepsCount);

	/** Returns the actual Slider Position */
	unsigned int GetPosition();
	/** Sets the actual Slider Position trimming to the Max and Min Values */
	void SetPosition(unsigned int pos);
	/** Sets the selected image */
	void SetImage(unsigned char type, Holder<Sprite2D> img);
	/** Sets the State of the Slider */
	void SetState(int arg) { State=(unsigned char) arg; }
	/** Refreshes a slider which is associated with VariableName */
	void UpdateState(unsigned int Sum);

private: // Private attributes
	/** Knob Image */
	Holder<Sprite2D> Knob;
	/** Grabbed Knob Image */
	Holder<Sprite2D> GrabbedKnob;
	/** Knob Starting Position */
	Point KnobPos;
	/** Knob Step Size */
	short KnobStep;
	/** Knob Steps Count */
	unsigned short KnobStepsCount;

	/** Actual Knob Status */
	unsigned char State;
	/** Slider Position Value */
	unsigned int Pos;

protected:
	/** Mouse Button Down */
	bool OnMouseDown(const MouseEvent& /*me*/, unsigned short Mod);
	/** Mouse Button Up */
	bool OnMouseUp(const MouseEvent& /*me*/, unsigned short Mod);
	/** Mouse Over Event */
	bool OnMouseDrag(const MouseEvent& /*me*/);
};

}

#endif
