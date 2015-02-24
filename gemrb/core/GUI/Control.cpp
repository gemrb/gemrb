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
#include "GUI/Window.h"

#include "win32def.h"

#include "ControlAnimation.h"
#include "Interface.h"
#include "ScriptEngine.h"
#include "Sprite2D.h"

#ifdef ANDROID
#include "Variables.h"
#endif

#include <cstdio>
#include <cstring>

namespace GemRB {

Control::Control(const Region& frame)
	: View(frame)
{
	hasFocus = false;
	InHandler = false;
	VarName[0] = 0;
	ControlID = 0;
	Value = 0;
	Flags = 0;
	Owner = NULL;

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
	delete animation;

	Sprite2D::FreeSprite(AnimPicture);
}

void Control::SetText(const String* string)
{
	SetText((string) ? *string : L"");
}

/** Sets the tooltip to be displayed on the screen now */
void Control::DisplayTooltip()
{
	if (tooltip.length() > 0) {
		const Region& winFrame = Owner->Frame();
		core->DisplayTooltip( winFrame.x + frame.x + frame.w / 2, winFrame.y + frame.y + frame.h / 2, this );
	} else
		core->DisplayTooltip( 0, 0, NULL );
}

void Control::ResetEventHandler(ControlEventHandler &handler)
{
	handler = NULL;
}

//return -1 if there is an error
//return 1 if there is no handler (not an error)
//return 0 if the handler ran as intended
int Control::RunEventHandler(ControlEventHandler handler)
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
		handler(this);
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

void Control::SetFocus(bool focus)
{
	hasFocus = focus;
	MarkDirty();
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
	ieDword newFlags = Flags;
	switch (opcode) {
		case BM_SET:
			newFlags = arg_flags;  //set
			break;
		case BM_AND:
			newFlags &= arg_flags;
			break;
		case BM_OR:
			newFlags |= arg_flags; //turn on
			break;
		case BM_XOR:
			newFlags ^= arg_flags;
			break;
		case BM_NAND:
			newFlags &= ~arg_flags;//turn off
			break;
		default:
			return -1;
	}
	Flags = newFlags;
	MarkDirty();
	return 0;
}

void Control::SetAnimPicture(Sprite2D* newpic)
{
	Sprite2D::FreeSprite(AnimPicture);
	AnimPicture = newpic;
	//apparently this is needed too, so the artifacts are not visible
	if (Owner && Owner->Visible==WINDOW_VISIBLE) {
		MarkDirty();
	}
}

}
