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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/Control.cpp,v 1.17 2004/02/24 22:20:36 balrog994 Exp $
 *
 */

#include "../../includes/win32def.h"
#include "Control.h"
#ifndef WIN32
#include <stdio.h>
#include <string.h>
#endif

Control::Control()
{
	hasFocus = false;
	Changed = true;
	VarName[0] = 0;
	Value = 0;
}

Control::~Control()
{
}

/** Key Press Event */
void Control::OnKeyPress(unsigned char Key, unsigned short Mod)
{
	//printf("OnKeyPress: CtrlID = 0x%08X, Key = %c (0x%02hX)\n", ControlID, Key, Key);
}
/** Key Release Event */
void Control::OnKeyRelease(unsigned char Key, unsigned short Mod)
{
	printf( "OnKeyRelease: CtrlID = 0x%08X, Key = %c (0x%02hX)\n", ControlID,
		Key, Key );
}
/** Mouse Over Event */
void Control::OnMouseOver(unsigned short x, unsigned short y)
{
	//printf("OnMouseOver: CtrlID = 0x%08X, x = %hd, y = %hd\n", ControlID, x, y);
}
/** Mouse Button Down */
void Control::OnMouseDown(unsigned short x, unsigned short y,
	unsigned char Button, unsigned short Mod)
{
	//printf("OnMouseDown: CtrlID = 0x%08X, x = %hd, y = %hd, Button = %d, Mos = %hd\n", ControlID, x, y, Button, Mod);
}
/** Mouse Button Up */
void Control::OnMouseUp(unsigned short x, unsigned short y,
	unsigned char Button, unsigned short Mod)
{
	//printf("OnMouseUp: CtrlID = 0x%08X, x = %hd, y = %hd, Button = %d, Mos = %hd\n", ControlID, x, y, Button, Mod);
}
/** Special Key Press */
void Control::OnSpecialKeyPress(unsigned char Key)
{
	//printf("OnSpecialKeyPress: CtrlID = 0x%08X, Key = %d\n", ControlID, Key);
}
