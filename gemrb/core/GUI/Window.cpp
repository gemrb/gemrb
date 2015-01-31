/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

#include "GUI/Window.h"

#include "GUI/Button.h"
#include "GUI/MapControl.h"
#include "GUI/ScrollBar.h"

#include "win32def.h"

#include "ie_cursors.h"

namespace GemRB {

Window::Window(unsigned short WindowID, const Region& frame)
	: View(frame)
{
	this->WindowID = WindowID;

	Visible = WINDOW_INVISIBLE;
	Cursor = IE_CURSOR_NORMAL;
	DefaultControl[0] = -1;
	DefaultControl[1] = -1;
	FunctionBar = false;
}

Window::~Window()
{

}

/** Add a Control in the Window */
void Window::SubviewAdded(View* view)
{
	Control* ctrl = dynamic_cast<Control*>(view);
	if (ctrl) {
		ctrl->Owner = this;
		for (size_t i = 0; i < Controls.size(); i++) {
			Control* target = Controls[i];
			if (target->ControlID == ctrl->ControlID) {
				Controls[i] = ctrl;
				delete RemoveSubview(target);
				return;
			}
		}
		Controls.push_back( ctrl );
	}
}

void Window::SubviewRemoved(View* subview)
{
	Control* ctrl = dynamic_cast<Control*>(subview);
	if (ctrl) {
		Controls.erase(std::find(Controls.begin(), Controls.end(), ctrl));
	}
}

void Window::SetFocused(Control* ctrl)
{
	TrySetFocus(ctrl);
}

/** This function Draws the Window on the Output Screen */
void Window::DrawSelf(Region /*drawFrame*/, const Region& clip)
{
	if (!Visible) return; // no point in drawing invisible windows
	Video* video = core->GetVideoDriver();

	if ( Flags & WF_FRAME) {
		Region screen( 0, 0, core->Width, core->Height );
		video->SetScreenClip( NULL );

		if (core->WindowFrames[0])
			video->BlitSprite( core->WindowFrames[0], 0, 0, true );
		if (core->WindowFrames[1])
			video->BlitSprite( core->WindowFrames[1], core->Width - core->WindowFrames[1]->Width, 0, true );
		if (core->WindowFrames[2])
			video->BlitSprite( core->WindowFrames[2], (core->Width - core->WindowFrames[2]->Width) / 2, 0, true );
		if (core->WindowFrames[3])
			video->BlitSprite( core->WindowFrames[3], (core->Width - core->WindowFrames[3]->Width) / 2, core->Height - core->WindowFrames[3]->Height, true );
	}

	if ( Visible == WINDOW_GRAYED ) {
		Color black = { 0, 0, 0, 128 };
		video->DrawRect(clip, black);
	}
}

/** Set window frame used to fill screen on higher resolutions*/
void Window::SetFrame()
{
	if ( (frame.w < core->Width) || (frame.h < core->Height) ) {
		Flags|=WF_FRAME;
	}
	Invalidate();
}

bool Window::OnSpecialKeyPress(unsigned char key)
{
	Control* ctrl = NULL;
	//the default control will get only GEM_RETURN
	if (key == GEM_RETURN) {
		ctrl = GetDefaultControl(0);
	}
	//the default cancel control will get only GEM_ESCAPE
	else if (key == GEM_ESCAPE) {
		ctrl = GetDefaultControl(1);
	} else if (key >= GEM_FUNCTION1 && key <= GEM_FUNCTION16) {
		ctrl = GetFunctionControl(key - GEM_FUNCTION1);
	} else {
		ctrl = dynamic_cast<Control*>(FocusedView());
	}

	if (ctrl) {
		switch (ctrl->ControlType) {
				//scrollbars will receive only mousewheel events
			case IE_GUI_SCROLLBAR:
				if (key != GEM_UP && key != GEM_DOWN) {
					return false;
				}
				break;
				//buttons will receive only GEM_RETURN
			case IE_GUI_BUTTON:
				if (key >= GEM_FUNCTION1 && key <= GEM_FUNCTION16) {
					//fake mouse button
					ctrl->OnMouseDown(Point(), GEM_MB_ACTION, 0);
					ctrl->OnMouseUp(Point(), GEM_MB_ACTION, 0);
					return false;
				}
				if (key != GEM_RETURN && key!=GEM_ESCAPE) {
					return false;
				}
				break;
				// shouldnt be any harm in sending these events to any control
		}
		ctrl->OnSpecialKeyPress( key );
		return true;
	}
	return false;
}

Control* Window::GetFunctionControl(int x)
{
	if (!FunctionBar) {
		return NULL;
	}

	std::vector< Control*>::const_iterator m;

	for (m = Controls.begin(); m != Controls.end(); m++) {
		Control *ctrl = *m;
		if ( ctrl->GetFunctionNumber() == x ) return ctrl;
	}
	return NULL;
}

Control* Window::GetFocus() const
{
	return dynamic_cast<Control*>(FocusedView());
}

Control* Window::GetMouseFocus() const
{
	return dynamic_cast<Control*>(FocusedView());
}

size_t Window::GetControlCount() const
{
	return Controls.size();
}

Control* Window::GetControlAtIndex(size_t i) const
{
	if (i < Controls.size()) {
		return Controls[i];
	}
	return NULL;
}

Control* Window::GetControlById(ieDword id) const
{
	size_t i = Controls.size();
	while (i--) {
		if (Controls[i]->ControlID == id) {
			return Controls[i];
		}
	}
	return NULL;
}

/** Returns the Control at X,Y Coordinates */
Control* Window::GetControlAtPoint(const Point& p, bool ignore)
{
	Control* ctrl = dynamic_cast<Control*>(SubviewAt(p));
	if (ctrl) {
		return (ignore && ctrl->ControlID&IGNORE_CONTROL) ? NULL : ctrl;
	}
	return NULL;
}

bool Window::IsValidControl(unsigned short ID, Control *ctrl) const
{
	size_t i = Controls.size();
	while (i--) {
		if (Controls[i]==ctrl) {
			return ctrl->ControlID==ID;
		}
	}
	return false;
}

Control* Window::GetDefaultControl(unsigned int ctrltype) const
{
	if (!Controls.size()) {
		return NULL;
	}
	if (ctrltype>=2) {
		return NULL;
	}
	return GetControlAtIndex( DefaultControl[ctrltype] );
}

Control* Window::GetScrollControl() const
{
	return scrollbar;
}

void Window::release(void)
{
	Visible = WINDOW_INVALID;
}

/** Redraw all the Window */
void Window::Invalidate()
{
	DefaultControl[0] = -1;
	DefaultControl[1] = -1;
	for (unsigned int i = 0; i < Controls.size(); i++) {
		if (!Controls[i]) {
			continue;
		}
		Controls[i]->MarkDirty();
		switch (Controls[i]->ControlType) {
			case IE_GUI_BUTTON:
				if (( Controls[i]->Flags & IE_GUI_BUTTON_DEFAULT )) {
					DefaultControl[0] = i;
				}
				if (( Controls[i]->Flags & IE_GUI_BUTTON_CANCEL )) {
					DefaultControl[1] = i;
				}
				break;
				//falling through
			case IE_GUI_GAMECONTROL:
				DefaultControl[0] = i;
				DefaultControl[1] = i;
				break;
			default: ;
		}
	}
	MarkDirty();
}

void Window::RedrawControls(const char* VarName, unsigned int Sum)
{
	for (std::vector<Control *>::iterator c = Controls.begin(); c != Controls.end(); ++c) {
		(*c)->UpdateState( VarName, Sum);
	}
}

}
