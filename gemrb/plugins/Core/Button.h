/***************************************************************************
                          Button.h  -  description
                             -------------------
    begin                : dom ott 12 2003
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

#ifndef BUTTON_H
#define BUTTON_H

#include "Control.h"
#include "Sprite2D.h"

#define IE_GUI_BUTTON_UNPRESSED 0
#define IE_GUI_BUTTON_PRESSED   1
#define IE_GUI_BUTTON_SELECTED  2
#define IE_GUI_BUTTON_DISABLED  3

/**Button Class. Used also for PixMaps (static images) or for Toggle Buttons.
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

class GEM_EXPORT Button : public Control
{
private:
	bool Clear;
public: 
	Button(bool Clear = false);
	~Button();
  /** Sets the 'type' Image of the Button to 'img'.
'type' may assume the following values:
- IE_GUI_BUTTON_UNPRESSED
- IE_GUI_BUTTON_PRESSED
- IE_GUI_BUTTON_SELECTED
- IE_GUI_BUTTON_DISABLED */
  void SetImage(unsigned char type, Sprite2D * img);
  /** Draws the Control on the Output Display */
  void Draw(unsigned short x, unsigned short y);
  /** Sets the Button State */
  void SetState(unsigned char state);
public: // Public Events
  /** Mouse Button Down */
  virtual void OnMouseDown(unsigned short x, unsigned short y, unsigned char Button, unsigned short Mod);
  /** Mouse Button Up */
  virtual void OnMouseUp(unsigned short x, unsigned short y, unsigned char Button, unsigned short Mod);	
private: // Private attributes
	/** Button Unpressed Image */
  Sprite2D * Unpressed;
  /** Button Pressed Image */
  Sprite2D * Pressed;
  /** Button Selected Image */
  Sprite2D * Selected;
  /** Button Disabled Image */
  Sprite2D * Disabled;
	/** The current state of the Button */
  unsigned char State;
};

#endif
