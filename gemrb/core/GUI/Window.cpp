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
	lastC = NULL;
	lastFocus = NULL;
	lastMouseFocus = NULL;
	lastOver = NULL;
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

Control* Window::GetOver() const
{
	return lastOver;
}

Control* Window::GetFocus() const
{
	return lastFocus;
}

Control* Window::GetMouseFocus() const
{
	return lastMouseFocus;
}

/** Sets 'ctrl' as Focused */
void Window::SetFocused(Control* ctrl)
{
	if (lastFocus != NULL) {
		lastFocus->SetFocus(false);
	}
	lastFocus = ctrl;
	if (ctrl != NULL) {
		lastFocus->SetFocus(true);
	}
}

/** Sets 'ctrl' as Mouse Focused */
void Window::SetMouseFocused(Control* ctrl)
{
	if (lastMouseFocus != NULL) {
		lastMouseFocus->MarkDirty();
	}
	lastMouseFocus = ctrl;
	if (ctrl != NULL) {
		lastMouseFocus->MarkDirty();
	}
}

size_t Window::GetControlCount() const
{
	return Controls.size();
}

Control* Window::GetControl(size_t i) const
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
	return GetControl( (ieWord) DefaultControl[ctrltype] );
}

Control* Window::GetScrollControl() const
{
	return scrollbar;
}

void Window::release(void)
{
	Visible = WINDOW_INVALID;
	lastC = NULL;
	lastFocus = NULL;
	lastMouseFocus = NULL;
	lastOver = NULL;
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

void Window::OnMouseEnter(unsigned short x, unsigned short y, Control *ctrl)
{
	lastOver = ctrl;
	if (!lastOver) {
		return;
	}
	lastOver->OnMouseEnter( x - frame.x - lastOver->Origin().x, y - frame.y - lastOver->Origin().y );
}

void Window::OnMouseLeave(unsigned short x, unsigned short y)
{
	if (!lastOver) {
		return;
	}
	lastOver->OnMouseLeave( x - frame.x - lastOver->Origin().x, y - frame.y - lastOver->Origin().y );
	lastOver = NULL;
}

void Window::OnMouseOver(unsigned short x, unsigned short y)
{
	if (!lastOver) {
		return;
	}
	short cx = x - frame.x - lastOver->Origin().x;
	short cy = y - frame.y - lastOver->Origin().y;
	if (cx < 0) {
		cx = 0;
	}
	if (cy < 0) {
		cy = 0;
	}
	lastOver->OnMouseOver(cx, cy);
}

}
