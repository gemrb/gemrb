/***************************************************************************
                          Control.cpp  -  description
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

#include "../../includes/win32def.h"
#include "Control.h"
#ifndef WIN32
#include <stdio.h>
#endif

Control::Control(){
	hasFocus = false;
	Changed = true;
}

Control::~Control(){
}

/** Key Press Event */
void Control::OnKeyPress(unsigned char Key, unsigned short Mod)
{
	printf("OnKeyPress: CtrlID = 0x%08X, Key = %c (0x%02hX)\n", ControlID, Key, Key);
}
/** Key Release Event */
void Control::OnKeyRelease(unsigned char Key, unsigned short Mod)
{
	printf("OnKeyRelease: CtrlID = 0x%08X, Key = %c (0x%02hX)\n", ControlID, Key, Key);
}
/** Mouse Over Event */
void Control::OnMouseOver(unsigned short x, unsigned short y)
{
	printf("OnMouseOver: CtrlID = 0x%08X, x = %hd, y = %hd\n", ControlID, x, y);
}
/** Mouse Button Down */
void Control::OnMouseDown(unsigned short x, unsigned short y, unsigned char Button, unsigned short Mod)
{
	printf("OnMouseDown: CtrlID = 0x%08X, x = %hd, y = %hd, Button = %d, Mos = %hd\n", ControlID, x, y, Button, Mod);
}
/** Mouse Button Up */
void Control::OnMouseUp(unsigned short x, unsigned short y, unsigned char Button, unsigned short Mod)
{
	printf("OnMouseUp: CtrlID = 0x%08X, x = %hd, y = %hd, Button = %d, Mos = %hd\n", ControlID, x, y, Button, Mod);
}
/** Special Key Press */
void Control::OnSpecialKeyPress(unsigned char Key)
{
	printf("OnSpecialKeyPress: CtrlID = 0x%08X, Key = %d\n", ControlID, Key);
}
/** Variable Functions */
void SetVariableName(const char * varname)
{
	strncpy(VarName, varname, 32);
}
char& GetVariableName()
{
	return VarName;
}
void SetVariableValue(int val)
{
	VarValue = val;
}
int GetVariableValue()
{
	return VarValue;
}