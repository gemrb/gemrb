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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/Control.h,v 1.20 2004/08/08 13:21:03 avenger_teambg Exp $
 *
 */

#ifndef CONTROL_H
#define CONTROL_H


#define IE_GUI_BUTTON		0
#define IE_GUI_SLIDER		2
#define IE_GUI_EDIT			3
#define IE_GUI_TEXTAREA		5
#define IE_GUI_LABEL		6
#define IE_GUI_SCROLLBAR	7
#define IE_GUI_GAMECONTROL	128


#include "../../includes/win32def.h"
/**This class defines a basic Control Object.
  *@author GemRB Developement Team
  */
#include "../../includes/RGBAColor.h"

#ifdef WIN32

#ifdef GEM_BUILD_DLL
#define GEM_EXPORT __declspec(dllexport)
#else
#define GEM_EXPORT __declspec(dllimport)
#endif

#else
#define GEM_EXPORT
#endif

typedef char EventHandler[64];


class GEM_EXPORT Control {
public: 
	Control();
	virtual ~Control();
	/** Draws the Control on the Output Display */
	virtual void Draw(unsigned short x, unsigned short y) = 0;
	/** Sets the Text of the current control */
	virtual int SetText(const char* string, int pos = 0) = 0;
	/** Sets the Tooltip text of the current control */
	virtual int SetTooltip(const char* string, int pos = 0);
	void DisplayTooltip();
	/** Variables */
	char VarName[MAX_VARIABLE_LENGTH];
	/** the value of the button to add to the variable */
	unsigned long Value;
public: // Public attributes
	/** Defines the Control ID Number used for GUI Scripting */
	unsigned long ControlID;
	/** X position of control relative to containing window */
	unsigned short XPos;
	/** Y position of control relative to containing window */
	unsigned short YPos;
	/** Width of control */
	unsigned short Width;
	/** Height of control */
	unsigned short Height;
	/** Type of control */
	unsigned char ControlType;
	char* Tooltip;	
	/** Focused Control */
	bool hasFocus;
	/** Changed Flag */
	bool Changed;
	/** Owner Window */
	void* Owner;
public: //Events
	/** Run specified handler */
	void RunEventHandler(EventHandler handler);
	/** Key Press Event */
	virtual void OnKeyPress(unsigned char Key, unsigned short Mod);
	/** Key Release Event */
	virtual void OnKeyRelease(unsigned char Key, unsigned short Mod);
	/** Mouse Enter Event */
	virtual void OnMouseEnter(unsigned short x, unsigned short y);
	/** Mouse Leave Event */
	virtual void OnMouseLeave(unsigned short x, unsigned short y);
	/** Mouse Over Event */
	virtual void OnMouseOver(unsigned short x, unsigned short y);
	/** Mouse Button Down */
	virtual void OnMouseDown(unsigned short x, unsigned short y,
		unsigned char Button, unsigned short Mod);
	/** Mouse Button Up */
	virtual void OnMouseUp(unsigned short x, unsigned short y,
		unsigned char Button, unsigned short Mod);
	/** Special Key Press */
	virtual void OnSpecialKeyPress(unsigned char Key);
	virtual bool IsPixelTransparent(unsigned short x, unsigned short y) {
		return false;
	}
};

#endif
