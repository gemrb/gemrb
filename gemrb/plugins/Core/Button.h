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
#include "Font.h"

#define IE_GUI_BUTTON_UNPRESSED 0
#define IE_GUI_BUTTON_PRESSED   1
#define IE_GUI_BUTTON_SELECTED  2
#define IE_GUI_BUTTON_DISABLED  3

#define OP_SET  0  //set
#define OP_OR   1  //turn on
#define OP_NAND 2  //turn off

/**Button Class. Used also for PixMaps (static images) or for Toggle Buttons.
  *@author GemRB Development Team
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
  /** Sets the Text of the current control */
  int SetText(const char * string, int pos = 0);
  /** Set Event */
  void SetEvent(char * funcName);
  /** Sets the Picture */
  void SetPicture(Sprite2D * Picture);
public: // Public Events
  /** Mouse Button Down */
  void OnMouseDown(unsigned short x, unsigned short y, unsigned char Button, unsigned short Mod);
  /** Mouse Button Up */
  void OnMouseUp(unsigned short x, unsigned short y, unsigned char Button, unsigned short Mod);	
  /** Button Pressed Event Script Function Name */
  char ButtonOnPress[64];
  /** Sets the Display Flags */
  int SetFlags(int Flags, int Operation);
  /** Refreshes the button from a radio group */
  void RedrawButton(char *VariableName, int Sum);
private: // Private attributes
  bool Clear;
  char * Text;
  bool hasText;
  Font * font;
  bool ToggleState;
  Color * palette;
  /** Button Unpressed Image */
  Sprite2D * Unpressed;
  /** Button Pressed Image */
  Sprite2D * Pressed;
  /** Button Selected Image */
  Sprite2D * Selected;
  /** Button Disabled Image */
  Sprite2D * Disabled;
  /** Picture to Apply when the hasPicture flag is set */
  Sprite2D * Picture;
  /** The current state of the Button */
  unsigned char State;
  /** Display Flags, justification */
  unsigned long Flags;
};

#endif
