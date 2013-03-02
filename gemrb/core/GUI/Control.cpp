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

#include "GUI/Control.h"

#include "GUI/EventMgr.h"
#include "GUI/Window.h"

#include "win32def.h"

#include "ControlAnimation.h"
#include "Interface.h"
#include "ScriptEngine.h"
#include "Video.h"

#ifdef ANDROID
#include "Variables.h"
#endif

#include <cstdio>
#include <cstring>

namespace GemRB {

Control::Control()
{
	hasFocus = false;
	Changed = true;
	InHandler = false;
	VarName[0] = 0;
	Value = 0;
	Flags = 0;
	Tooltip = NULL;
	Owner = NULL;
	XPos = 0;
	YPos = 0;

	sb = NULL;
	animation = NULL;
	AnimPicture = NULL;
	ControlType = IE_GUI_INVALID;
	FunctionNumber = -1;
}

Control::~Control()
{
	if (InHandler) {
		Log(ERROR, "Control", "Destroying control inside event handler, crash may occur!");
	}
	core->DisplayTooltip( 0, 0, NULL );
	free (Tooltip);

	delete animation;

	core->GetVideoDriver()->FreeSprite(AnimPicture);
}

/** Sets the Tooltip text of the current control */
int Control::SetTooltip(const char* string)
{
	free(Tooltip);

	if ((string == NULL) || (string[0] == 0)) {
		Tooltip = NULL;
	} else {
		Tooltip = strdup (string);
	}
	Changed = true;
	return 0;
}

/** Sets the tooltip to be displayed on the screen now */
void Control::DisplayTooltip()
{
	if (Tooltip)
		core->DisplayTooltip( Owner->XPos + XPos + Width / 2, Owner->YPos + YPos + Height / 2, this );
	else
		core->DisplayTooltip( 0, 0, NULL );
}

void Control::ResetEventHandler(EventHandler &handler)
{
	handler = NULL;
}

void Control::SetText(const char* /*string*/)
{
}

//return -1 if there is an error
//return 1 if there is no handler (not an error)
//return 0 if the handler ran as intended
int Control::RunEventHandler(EventHandler handler)
{
	if (InHandler) {
		Log(WARNING, "Control", "Nested event handlers are not supported!");
		return -1;
	}
	if (handler) {
		Window *wnd = Owner;
		if (!wnd) {
			return -1;
		}
		unsigned short WID = wnd->WindowID;
		unsigned short ID = (unsigned short) ControlID;
		InHandler = true;
		//TODO: detect caller errors, trap them???
		handler->call();
		InHandler = false;
		if (!core->IsValidWindow(WID,wnd) ) {
			Log(ERROR, "Control", "Owner window destructed!");
			return -1;
		}
		if (!wnd->IsValidControl(ID,this) ) {
			Log(ERROR, "Control", "Control destructed!");
			return -1;
		}
		return 0;
	}
	return 1;
}

/** Mouse Button Down */
void Control::OnMouseDown(unsigned short x, unsigned short y,
	unsigned short Button, unsigned short Mod)
{
	if (Button == GEM_MB_SCRLUP || Button == GEM_MB_SCRLDOWN) {
		Control *ctrl = Owner->GetScrollControl();
		if (ctrl && (ctrl!=this)) {
			ctrl->OnMouseDown(x,y,Button,Mod);
		}
	}
}

/** Mouse Button Up */
void Control::OnMouseUp(unsigned short /*x*/, unsigned short /*y*/,
	unsigned short /*Button*/, unsigned short /*Mod*/)
{
	//print("OnMouseUp: CtrlID = 0x%08X, x = %hd, y = %hd, Button = %d, Mos = %hd",(unsigned int) ControlID, x, y, Button, Mod);
}

/** Mouse scroll wheel */
void Control::OnMouseWheelScroll( short x, short y)
{
	Control *ctrl = Owner->GetScrollControl();
	if (ctrl && (ctrl!=this)) {
		ctrl->OnMouseWheelScroll( x, y );
	}	
}

/** Special Key Press */
bool Control::OnSpecialKeyPress(unsigned char Key)
{
	if (Key == GEM_UP || Key == GEM_DOWN) {
		Control *ctrl = Owner->GetScrollControl();
		if (ctrl && (ctrl!=this)) {
			return ctrl->OnSpecialKeyPress(Key);
		}
	}
	return false;
}
void Control::SetFocus(bool focus)
{
	hasFocus = focus;
}

bool Control::isFocused()
{
	return hasFocus;
}
/** Sets the Display Flags */
int Control::SetFlags(int arg_flags, int opcode)
{
	if ((arg_flags >>24) != ControlType) {
		Log(WARNING, "Control", "Trying to modify invalid flag %x on control %d (opcode %d)",
			arg_flags, ControlID, opcode);
		return -2;
	}
	switch (opcode) {
		case BM_SET:
			Flags = arg_flags;  //set
			break;
		case BM_AND:
			Flags &= arg_flags;
			break;
		case BM_OR:
			Flags |= arg_flags; //turn on
			break;
		case BM_XOR:
			Flags ^= arg_flags;
			break;
		case BM_NAND:
			Flags &= ~arg_flags;//turn off
			break;
		default:
			return -1;
	}
	Changed = true;
	Owner->Invalidate();
	return 0;
}

void Control::SetAnimPicture(Sprite2D* newpic)
{
	core->GetVideoDriver()->FreeSprite(AnimPicture);
	AnimPicture = newpic;
	//apparently this is needed too, so the artifacts are not visible
	if (Owner->Visible==WINDOW_VISIBLE) {
		Changed = true;
		Owner->InvalidateForControl(this);
	}
}

/** Sets the Scroll Bar Pointer. If 'ptr' is NULL no Scroll Bar will be linked
	to this Control. */
int Control::SetScrollBar(Control* ptr)
{
	if (ptr && (ptr->ControlType!=IE_GUI_SCROLLBAR)) {
		ptr = NULL;
		Log(WARNING, "Control", "Attached control is not a ScrollBar!");
		return -1;
	}
	sb = ptr;
	Changed = true;
	if (ptr) return 1;
	return 0;
}

}
