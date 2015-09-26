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

#include "Window.h"

#include "Interface.h"
#include "ScrollBar.h"

namespace GemRB {

Window::Window(const Region& frame, WindowManager& mgr)
	: View(frame), manager(mgr)
{
	isDragging = false;
	focusView = NULL;
	trackingView = NULL;
	hoverView = NULL;
	backBuffer = NULL;
	lastMouseMoveTime = GetTickCount();

	SetFlags(DestroyOnClose, OP_OR);
	SizeChanged(frame.Dimensions());
}

Window::~Window()
{
	Close();
	core->GetVideoDriver()->DestroyBuffer(backBuffer);

	std::map<KeyboardKey, EventMgr::EventCallback*>::iterator it = HotKeys.begin();
	for (; it != HotKeys.end(); ++it) {
		delete it->second;
	}
}

void Window::Close()
{
	if (flags&DestroyOnClose) {
		manager.CloseWindow(this);
	} else {
		// somebody wants to keep a handle to this window around to display it later
		manager.OrderBack(this);
		SetVisible(false);
	}
}

bool Window::DisplayModal(WindowManager::ModalShadow shadow)
{
	return manager.MakeModal(this, shadow);
}

/** Add a Control in the Window */
void Window::SubviewAdded(View* view, View* parent)
{
	Control* ctrl = dynamic_cast<Control*>(view);
	if (ctrl) {
		if (ctrl->Owner == this) return; // already added!
		ctrl->Owner = this;
		if (ctrl->ControlType == IE_GUI_SCROLLBAR && parent == this) {
			scrollbar = static_cast<ScrollBar*>(ctrl);
		}
		Controls.push_back( ctrl );
	}
}

void Window::SubviewRemoved(View* subview, View* /*parent*/)
{
	Control* ctrl = dynamic_cast<Control*>(subview);
	if (ctrl) {
		ctrl->Owner = NULL;
		std::vector< Control*>::iterator it = std::find(Controls.begin(), Controls.end(), ctrl);
		if (it != Controls.end())
			Controls.erase(it);
	}
	if (focusView == ctrl) {
		focusView = NULL;
	}
}

void Window::SizeChanged(const Size& /*oldSize*/)
{
	Video* video = core->GetVideoDriver();
	video->DestroyBuffer(backBuffer);

	backBuffer = video->CreateBuffer(frame.Dimensions());
}

void Window::WillDraw()
{
	core->GetVideoDriver()->SetDrawingBuffer(backBuffer);
	if (flags&Borderless && NeedsDraw()) {
		// FIXME: this shouldnt be needed after the buffers are sized to the window
		// this *really* slows things down (more than 50% frame loss if all windows do it)
		// its needed now because moving a window smears its backbuffer
		// due to the window moving within it instead of the buffer itself being blitted at the new pos
		backBuffer->Clear();
	}
}

void Window::Focus()
{
	manager.FocusWindow(this);
}

void Window::SetFocused(Control* ctrl)
{
	if (ctrl) {
		TrySetFocus(ctrl);
	} else if (Controls.size()) {
		// set a default focus, something should always be focused
		TrySetFocus(Controls[0]);
	}
}

String Window::TooltipText() const
{
	if (hoverView) {
		return hoverView->TooltipText();
	}
	return View::TooltipText();
}

void Window::SetPosition(WindowPosition pos)
{
	// start at top left
	Region newFrame(Point(), frame.Dimensions());
	Size screen = manager.ScreenSize();

	// adjust horizontal
	if ((pos&PosHmid) == PosHmid) {
		newFrame.x = (screen.w / 2) - (newFrame.w) / 2;
	} else if (pos&PosRight) {
		newFrame.x = screen.w - newFrame.w;
	}

	// adjust vertical
	if ((pos&PosVmid) == PosVmid) {
		newFrame.y = (screen.h / 2) - (newFrame.h) / 2;
	} else if (pos&PosBottom) {
		newFrame.y = screen.h - newFrame.h;
	}
	SetFrame(newFrame);
}

void Window::DrawSelf(Region /*drawFrame*/, const Region& /*clip*/)
{
	// window doesnt actually have anything to draw (subviews only)
}

Control* Window::GetFocus() const
{
	return dynamic_cast<Control*>(FocusedView());
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

void Window::DispatchMouseOver(View* target, const Point& p)
{
	if (isDragging) {
		OnMouseOver(ConvertPointFromScreen(p));
		return;
	}

	bool left = false;
	if (target) {
		if (target != hoverView) {
			if (hoverView) {
				hoverView->OnMouseLeave(hoverView->ConvertPointFromScreen(p), drag.get());
				left = true;
			}
			target->OnMouseEnter(target->ConvertPointFromScreen(p), drag.get());
		}
	} else if (hoverView) {
		hoverView->OnMouseLeave(hoverView->ConvertPointFromScreen(p), drag.get());
		left = true;
	}
	if (left) {
		assert(hoverView);
		if (trackingView && !drag) {
			drag = trackingView->DragOperation();
		}
		if (trackingView == hoverView
			&& !trackingView->TracksMouseDown())
		{
			trackingView = NULL;
		}
	}
	if (trackingView) {
		// tracking will eat this event
		trackingView->OnMouseOver(trackingView->ConvertPointFromScreen(p));
	} else if (target) {
		target->OnMouseOver(target->ConvertPointFromScreen(p));
	}
	hoverView = target;
}

void Window::DispatchMouseDown(View* target, const Point& p, unsigned short button, unsigned short mod)
{
	if (button == GEM_MB_ACTION && !(flags&MousePassthrough)) {
		Focus();
	}

	TrySetFocus(target);
	Point subP = target->ConvertPointFromScreen(p);
	target->OnMouseDown(subP, button, mod);
	trackingView = target; // all views track the mouse within their bounds
}

void Window::DispatchMouseUp(View* target, const Point& p, unsigned short button, unsigned short mod)
{
	if (isDragging) {
		isDragging = false;
		return;
	}
	if (trackingView) {
		Point subP = trackingView->ConvertPointFromScreen(p);
		trackingView->OnMouseUp(subP, button, mod);
	} else if (drag) {
		if (target->AcceptsDragOperation(*drag)) {
			target->CompleteDragOperation(*drag);
		}
	}
	drag = NULL;
	trackingView = NULL;
}

bool Window::DispatchEvent(const Event& event)
{
	if (event.isScreen) {
		Point screenPos = event.mouse.Pos();
		View* target = SubviewAt(ConvertPointFromScreen(screenPos), false, true);

		if (target == NULL && flags&MousePassthrough) {
			return false; // do *not* absorb the event, pass it along
		}

		// special event handling
		switch (event.type) {
			case Event::MouseScroll:
				// retarget if NULL or disabled
				if (target == NULL || target->IsDisabled()) {
					OnMouseWheelScroll( event.mouse.deltaX, event.mouse.deltaY );
				} else {
					target->OnMouseWheelScroll( event.mouse.deltaX, event.mouse.deltaY );
				}
				return true;
			case Event::MouseMove:
				// allows NULL and disabled targets
				DispatchMouseOver(target, screenPos);
				return true;
			default:
				if (target == NULL) {
					target = this;
				} else if (target->IsDisabled()) {
					return true; // we still absorb the event
				}
				break;
		}

		// basic event handling
		switch (event.type) {
			case Event::MouseDown:
				DispatchMouseDown(target, screenPos, event.mouse.button, event.mod);
				break;
			case Event::MouseUp:
				DispatchMouseUp(target, screenPos, event.mouse.button, event.mod);
				break;
			default: return false; // should never happen, supressing compiler warning
		}
		// absorb other screen events i guess
		return true;
	} else {
		// key events
		std::map<KeyboardKey, EventMgr::EventCallback*>::iterator it = HotKeys.find(event.keyboard.keycode);
		if (it != HotKeys.end()) {
			(*it->second)(event);
		} else if (event.keyboard.keycode == GEM_ESCAPE) {
			Close();
		}
		return true;
	}
	return false;
}

bool Window::RegisterHotKeyCallback(EventMgr::EventCallback* cb, KeyboardKey key)
{
	// FIXME: check if something already registerd and either return false or delete the old
	HotKeys[key] = cb;
	return true;
}

void Window::OnMouseOver(const Point& p)
{
	if (isDragging) {
		Point delta = dragOrigin - p;
		Point newOrigin = frame.Origin() - delta;
		SetFrameOrigin(newOrigin);
		dragOrigin = dragOrigin + delta;
	}
}

void Window::OnMouseLeave(const Point&, const DragOp*)
{
	// not sure this can happen... maybe in a low fps scenario?
	isDragging = false;
}

void Window::OnMouseDown(const Point& p, unsigned short button, unsigned short mod)
{
	switch (button) {
		case GEM_MB_ACTION:
			if (flags&Draggable) {
				isDragging = true;
				dragOrigin = p;
				break;
			}
			// fallthrough
		default:
			View::OnMouseDown(p, button, mod);
			break;
	}
}

bool Window::OnSpecialKeyPress(unsigned char key)
{
	bool handled = false;
	if (focusView) {
		handled = focusView->OnSpecialKeyPress(key);
	}

	Control* ctrl = NULL;
	if (key >= GEM_FUNCTION1 && key <= GEM_FUNCTION16) {
		// TODO: implement hotkeys
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
