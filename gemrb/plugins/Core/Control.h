/***************************************************************************
                          Control.h  -  description
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

#ifndef CONTROL_H
#define CONTROL_H


#define IE_GUI_BUTTON    0
#define IE_GUI_SLIDER    2
#define IE_GUI_EDIT      3
#define IE_GUI_TEXTAREA  5
#define IE_GUI_LABEL     6
#define IE_GUI_SCROLLBAR 7



/**This class defines a basic Control Object.
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

class GEM_EXPORT Control {
public: 
	Control();
	virtual ~Control();
	/** Draws the Control on the Output Display */
	virtual void Draw(unsigned short x, unsigned short y) = 0;
	/** Sets the Text of the current control */
	virtual int SetText(const char * string) = 0;
public: // Public attributes
	/** Defines the Control ID Number used for GUI Scripting */
	unsigned long ControlID;
	/** Buffer length of an attached buffer (only significant for some types of text controls) */
	unsigned short BufferLength;
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
	/** Focused Control */
	bool hasFocus;
	/** Changed Flag */
	bool Changed;
	/** Owner Window */
	void * Owner;
  /** Associated Variable for the current Control */
  char VarName[32];
  int  VarValue;
public: //Events
	/** Key Press Event */
	virtual void OnKeyPress(unsigned char Key, unsigned short Mod);
	/** Key Release Event */
	virtual void OnKeyRelease(unsigned char Key, unsigned short Mod);
	/** Mouse Over Event */
	virtual void OnMouseOver(unsigned short x, unsigned short y);
	/** Mouse Button Down */
	virtual void OnMouseDown(unsigned short x, unsigned short y, unsigned char Button, unsigned short Mod);
	/** Mouse Button Up */
	virtual void OnMouseUp(unsigned short x, unsigned short y, unsigned char Button, unsigned short Mod);
	/** Special Key Press */
	virtual void OnSpecialKeyPress(unsigned char Key);
	/** Variable Functions */
	void SetVariableName(const char * varname);
	char& GetVariableName();
	void SetVariableValue(int val);
	int GetVariableValue();
};

#endif
