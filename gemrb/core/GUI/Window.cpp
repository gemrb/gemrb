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
	FunctionBar = false;

	focusView = NULL;
	trackingView = NULL;
	hoverView = NULL;
}

Window::~Window()
{

}

/** Add a Control in the Window */
void Window::SubviewAdded(View* view, View* /*parent*/)
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

void Window::SubviewRemoved(View* subview, View* /*parent*/)
{
	Control* ctrl = dynamic_cast<Control*>(subview);
	if (ctrl) {
		ctrl->Owner = NULL;
		Controls.erase(std::find(Controls.begin(), Controls.end(), ctrl));
	}
	if (focusView == ctrl) {
		focusView = NULL;
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
	MarkDirty();
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

Control* Window::GetScrollControl() const
{
	return scrollbar;
}

void Window::RedrawControls(const char* VarName, unsigned int Sum)
{
	for (std::vector<Control *>::iterator c = Controls.begin(); c != Controls.end(); ++c) {
		Control* ctrl = *c;
		ctrl->UpdateState( VarName, Sum);
	}
}

bool Window::TrySetFocus(View* target)
{
	if (target && !target->CanLockFocus()) {
		// target wont accept focus so dont bother unfocusing current
		return false;
	}
	if (focusView && !focusView->CanUnlockFocus()) {
		// current focus unwilling to reliquish
		return false;
	}
	focusView = target;
	return true;
}

void Window::DispatchMouseOver(const Point& p)
{
	// need screen coordinates because the target may not be a direct subview
	Point screenP = ConvertPointToScreen(p);
	View* target = SubviewAt(p, false, true);
	bool left = false;
	if (target) {
		if (target != hoverView) {
			if (hoverView) {
				hoverView->OnMouseLeave(hoverView->ConvertPointFromScreen(screenP), drag.get());
				left = true;
			}
			target->OnMouseEnter(target->ConvertPointFromScreen(screenP), drag.get());
		}
	} else if (hoverView) {
		hoverView->OnMouseLeave(hoverView->ConvertPointFromScreen(screenP), drag.get());
		left = true;
	}
	if (left) {
		if (trackingView && !drag) {
			drag = trackingView->DragOperation();
		}
		assert(hoverView);
		if (hoverView->TracksMouseDown()) {
			trackingView = hoverView;
		} else {
			trackingView = NULL;
		}
	}
	if (trackingView) {
		// tracking will eat this event
		trackingView->OnMouseOver(trackingView->ConvertPointFromScreen(screenP));
	} else if (target) {
		target->OnMouseOver(target->ConvertPointFromScreen(screenP));
	}
	hoverView = target;
}

void Window::DispatchMouseDown(const Point& p, unsigned short button, unsigned short mod)
{
	View* target = SubviewAt(p, false, true);
	if (target) {
		TrySetFocus(target);
		Point subP = target->ConvertPointFromScreen(ConvertPointToScreen(p));
		target->OnMouseDown(subP, button, mod);
		trackingView = target; // all views track the mouse within their bounds
		return;
	}
	// handle scrollbar events
	View::OnMouseDown(p, button, mod);
}

void Window::DispatchMouseUp(const Point& p, unsigned short button, unsigned short mod)
{
	if (trackingView) {
		Point subP = trackingView->ConvertPointFromScreen(ConvertPointToScreen(p));
		trackingView->OnMouseUp(subP, button, mod);
	} else if (drag) {
		View* target = SubviewAt(p, false, true);
		if (target && target->AcceptsDragOperation(*drag)) {
			target->CompleteDragOperation(*drag);
		}
	}
	drag = NULL;
	trackingView = NULL;
}

void Window::DispatchMouseWheelScroll(short x, short y)
{
	Point mp = core->GetVideoDriver()->GetMousePos();
	View* target = SubviewAt(ConvertPointFromScreen(mp), false, true);
	if (target) {
		target->OnMouseWheelScroll( x, y );
		return;
	}
	// handle scrollbar events
	View::OnMouseWheelScroll(x, y);
}

bool Window::OnSpecialKeyPress(unsigned char key)
{
	bool handled = false;
	if (focusView) {
		handled = focusView->OnSpecialKeyPress(key);
	}

	Control* ctrl = NULL;
	//the default control will get only GEM_RETURN
	if (key == GEM_RETURN) {
		//ctrl = GetDefaultControl(0);
	}
	//the default cancel control will get only GEM_ESCAPE
	else if (key == GEM_ESCAPE) {
		//ctrl = GetDefaultControl(1);
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
	// handle scrollbar events
	return View::OnSpecialKeyPress(key);
}

}
