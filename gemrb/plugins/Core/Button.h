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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/Button.h,v 1.26 2004/03/28 14:29:29 avenger_teambg Exp $
 *
 */

#ifndef BUTTON_H
#define BUTTON_H

#include "Control.h"
#include "Sprite2D.h"
#include "Font.h"
#include "Animation.h"

#define IE_GUI_BUTTON_UNPRESSED 0
#define IE_GUI_BUTTON_PRESSED   1
#define IE_GUI_BUTTON_SELECTED  2
#define IE_GUI_BUTTON_DISABLED  3

// NOTE: keep these synchronized with GUIDefines.py!!!
#define IE_GUI_BUTTON_NO_IMAGE     0x00000001   // don't draw image (BAM)
#define IE_GUI_BUTTON_PICTURE      0x00000002   // draw picture (BMP, MOS, ...)
#define IE_GUI_BUTTON_SOUND        0x00000004
#define IE_GUI_BUTTON_ALT_SOUND    0x00000008
#define IE_GUI_BUTTON_CHECKBOX     0x00000010   // or radio button
#define IE_GUI_BUTTON_RADIOBUTTON  0x00000020   // sticks in a state
#define IE_GUI_BUTTON_DEFAULT      0x00000040   // enter button triggers it
#define IE_GUI_BUTTON_DRAGGABLE    0x00000080

//these bits are hardcoded in the .chu structure
#define IE_GUI_BUTTON_ALIGN_LEFT   0x00000100
#define IE_GUI_BUTTON_ALIGN_RIGHT  0x00000200
#define IE_GUI_BUTTON_ALIGN_TOP    0x00000400
//end of hardcoded part

#define IE_GUI_BUTTON_ANIMATED     0x00010000
#define IE_GUI_BUTTON_NO_TEXT      0x00020000   // don't draw button label

#define IE_GUI_BUTTON_NORMAL       0x00000004   // default button, doesn't stick


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

class GEM_EXPORT Button : public Control {
public: 
	Button(bool Clear = false);
	~Button();
	/** Sets the 'type' Image of the Button to 'img'.
	'type' may assume the following values:
	- IE_GUI_BUTTON_UNPRESSED
	- IE_GUI_BUTTON_PRESSED
	- IE_GUI_BUTTON_SELECTED
	- IE_GUI_BUTTON_DISABLED */
	void SetImage(unsigned char type, Sprite2D* img);
	/** Draws the Control on the Output Display */
	void Draw(unsigned short x, unsigned short y);
	/** Sets the Button State */
	void SetState(unsigned char state);
	/** Sets the Text of the current control */
	int SetText(const char* string, int pos = 0);
	/** Set Event */
	void SetEvent(char* funcName, int eventType);
	/** Sets the Picture */
	void SetPicture(Sprite2D* Picture);
public: // Public Events
	/** Mouse Over */
	void OnMouseOver(unsigned short x, unsigned short y);
	/** Mouse Button Down */
	void OnMouseDown(unsigned short x, unsigned short y, unsigned char Button,
		unsigned short Mod);
	/** Mouse Button Up */
	void OnMouseUp(unsigned short x, unsigned short y, unsigned char Button,
		unsigned short Mod);	
	/** A special key has been pressed */
	void OnSpecialKeyPress(unsigned char Key);
	/** Button Pressed Event Script Function Name */
	EventHandler ButtonOnPress;
	EventHandler MouseOverButton;
	/** Sets the Display Flags */
	int SetFlags(int Flags, int Operation);
	/** Returns the Display Flags */
	unsigned int GetFlags()
	{
		return Flags;
	}
	/** Refreshes the button from a radio group */
	void RedrawButton(char* VariableName, unsigned int Sum);
private: // Private attributes
	bool Clear;
	char* Text;
	bool hasText;
	Font* font;
	bool ToggleState;
	Color* palette;
	Color* stdpal;
	/** Button Unpressed Image */
	Sprite2D* Unpressed;
	/** Button Pressed Image */
	Sprite2D* Pressed;
	/** Button Selected Image */
	Sprite2D* Selected;
	/** Button Disabled Image */
	Sprite2D* Disabled;
	/** Picture to Apply when the hasPicture flag is set */
	Sprite2D* Picture;
	/** Animated Button */
	Animation* anim;
	/** The current state of the Button */
	unsigned char State;
	/** Display Flags, justification */
	unsigned long Flags;
	/** MOS Draggable Variables */
	unsigned short ScrollX, ScrollY;
	bool Dragging;
	unsigned short DragX, DragY; //Starting Dragging Positions
};

#endif
