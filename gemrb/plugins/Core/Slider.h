/***************************************************************************
                          Slider.h  -  description
                             -------------------
    begin                : lun ott 13 2003
    copyright            : (C) 2003 by GemRB Developement Team
    email                : Balrog994@yahoo.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef SLIDER_H
#define SLIDER_H

#include "Control.h"
#include "Sprite2D.h"
#include <math.h>

#define IE_GUI_SLIDER_KNOB        0
#define IE_GUI_SLIDER_GRABBEDKNOB 1
#define IE_GUI_SLIDER_BACKGROUND  2

/**Slider Control
  *@author GemRB Developement Team
  */

#ifdef WIN32

#ifdef GEM_BUILD_DLL
#define GEM_EXPORT __declspec(dllexport)
#else
#define GEM_EXPORT __declspec(dllimport)
#endif

#else
#define GEM_EXPORT
#endif

class GEM_EXPORT Slider : public Control  {
public: 
	Slider(short KnobXPos, short KnobYPos, short KnobStep, unsigned short KnobStepsCount, bool Clear = false);
	~Slider();
  /** Draws the Control on the Output Display */
  void Draw(unsigned short x, unsigned short y);
  /** Returns the actual Slider Position */
  unsigned int GetPosition();
  /** Sets the actual Slider Position trimming to the Max and Min Values */
  void SetPosition(unsigned int pos);
  /** Sets the selected image */
  void SetImage(unsigned char type, Sprite2D * img);
  /** Sets the Text of the current control */
  int SetText(const char * string, int pos = 0);
  /** Redraws a slider which is associated with VariableName */
  void RedrawSlider(char *VariableName, int Sum);

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
  void OnMouseDown(unsigned short x, unsigned short y, unsigned char Button, unsigned short Mod);
  /** Mouse Button Up */
  void OnMouseUp(unsigned short x, unsigned short y, unsigned char Button, unsigned short Mod);
  /** Mouse Over Event */
  void OnMouseOver(unsigned short x, unsigned short y);
  /** OnChange Scripted Event Function Name */
  EventHandler SliderOnChange;
};

#endif
