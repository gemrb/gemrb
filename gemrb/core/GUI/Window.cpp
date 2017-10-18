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

#include "GUI/GUIScriptInterface.h"

#include "Interface.h"
#include "ScrollBar.h"

namespace GemRB {

Window::Window(const Region& frame, WindowManager& mgr)
	: ScrollView(frame), manager(mgr)
{
	focusView = NULL;
	trackingView = NULL;
	hoverView = NULL;
	backBuffer = NULL;
	lastMouseMoveTime = GetTickCount();

	SetFlags(DestroyOnClose, OP_OR);
	RecreateBuffer();
}

Window::~Window()
{
	// Dont Close() the window. WindowManager owns the window, so it will be deleting it
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
	return manager.PresentModalWindow(this, shadow);
}

/** Add a Control in the Window */
void Window::SubviewAdded(View* view, View* /*parent*/)
{
	Control* ctrl = dynamic_cast<Control*>(view);
	if (ctrl) {
		Controls.insert(ctrl);
	}
	if (focusView == NULL) {
		TrySetFocus(view);
	}
}

void Window::SubviewRemoved(View* subview, View* /*parent*/)
{
	Control* ctrl = dynamic_cast<Control*>(subview);
	if (ctrl) {
		Controls.erase(ctrl);
	}
	if (focusView == ctrl) {
		focusView = NULL;
		for (std::set<Control *>::iterator c = Controls.begin(); c != Controls.end(); ++c) {
			Control* ctrl = *c;
			if (TrySetFocus(ctrl) == ctrl) {
				break;
			}
		}
	}
}

void Window::SizeChanged(const Size& /*oldSize*/)
{
	RecreateBuffer();
	Update();
}

void Window::FlagsChanged(unsigned int oldflags)
{
	if ((flags&AlphaChannel) != (oldflags&AlphaChannel)) {
		RecreateBuffer();
	}
}

void Window::RecreateBuffer()
{
	Video* video = core->GetVideoDriver();
	video->DestroyBuffer(backBuffer);

	Video::BufferFormat fmt = (flags&AlphaChannel) ? Video::RGBA8888 : Video::DISPLAY;
	backBuffer = video->CreateBuffer(frame, fmt);

	// the entire window must be invalidated, because the new buffer is blank
	// TODO: we *could* optimize this to instead blit the old buffer to the new one
	MarkDirty();
}

void Window::WillDraw()
{
	backBuffer->SetOrigin(frame.Origin());
	core->GetVideoDriver()->PushDrawingBuffer(backBuffer);
}

void Window::Focus()
{
	manager.FocusWindow(this);
}

void Window::SetFocused(View* ctrl)
{
	TrySetFocus(ctrl);
}

String Window::TooltipText() const
{
	if (hoverView) {
		return hoverView->TooltipText();
	}
	return ScrollView::TooltipText();
}

Sprite2D* Window::Cursor() const
{
	if (hoverView) {
		return hoverView->Cursor();
	}
	return ScrollView::Cursor();
}

bool Window::IsDisabledCursor() const
{
	if (hoverView) {
		return hoverView->IsDisabledCursor();
	}
	return ScrollView::IsDisabledCursor();
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

void Window::RedrawControls(const char* VarName, unsigned int Sum)
{
	for (std::set<Control *>::iterator c = Controls.begin(); c != Controls.end(); ++c) {
		Control* ctrl = *c;
		ctrl->UpdateState( VarName, Sum);
	}
}

View* Window::TrySetFocus(View* target)
{
	View* newFocus = focusView;
	if (target && !target->CanLockFocus()) {
		// target wont accept focus so dont bother unfocusing current
	} else if (focusView && !focusView->CanUnlockFocus()) {
		// current focus unwilling to reliquish
	} else {
		newFocus = target;
	}
	focusView = newFocus;
	return newFocus;
}

void Window::DispatchMouseMotion(View* target, const MouseEvent& me)
{
	bool left = false;
	if (target) {
		if (target != hoverView) {
			if (hoverView) {
				hoverView->OnMouseLeave(me, drag.get());
				left = true;
			}
			target->OnMouseEnter(me, drag.get());
		}
	} else if (hoverView) {
		hoverView->OnMouseLeave(me, drag.get());
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
		assert(me.buttonStates);
		trackingView->OnMouseDrag(me);
	} else if (target) {
		target->OnMouseOver(me);
	}
	hoverView = target;
}

void Window::DispatchMouseDown(View* target, const MouseEvent& me, unsigned short mod)
{
	assert(target);
	
	if (me.button == GEM_MB_ACTION
		&& !(Flags() & View::IgnoreEvents)
	) {
		Focus();
	}

	TrySetFocus(target);
	target->OnMouseDown(me, mod);
	trackingView = target; // all views track the mouse within their bounds
	assert(me.buttonStates);
}

void Window::DispatchMouseUp(View* target, const MouseEvent& me, unsigned short mod)
{
	assert(target);
	
	if (trackingView) {
		trackingView->OnMouseUp(me, mod);
	} else if (drag) {
		if (target->AcceptsDragOperation(*drag)) {
			target->CompleteDragOperation(*drag);
		}
	} else if (target) {
		target->OnMouseUp(me, mod);
	}
	drag = NULL;
	trackingView = NULL;
}
	
bool Window::DispatchKey(View* keyView, const Event& event)
{
	// hotkeys first
	std::map<KeyboardKey, EventMgr::EventCallback*>::iterator it = HotKeys.find(event.keyboard.keycode);
	if (it != HotKeys.end()) {
		return (*it->second)(event);
	}

	// try the keyView view first, if it fails have the window itself try
	bool handled = false;
	if (keyView) {
		handled = (event.type == Event::KeyDown)
		? keyView->OnKeyPress(event.keyboard, event.mod)
		: keyView->OnKeyRelease(event.keyboard, event.mod);
	}
	
	if (!handled) {
		handled = (event.type == Event::KeyDown)
		? OnKeyPress(event.keyboard, event.mod)
		: OnKeyRelease(event.keyboard, event.mod);
	}
	return handled;
}

bool Window::DispatchEvent(const Event& event)
{
	View* target = NULL;

	if (event.isScreen) {
		// FIXME: this will be for touch events too. poor name for that!
		// also using a mask will be more suitable for mixed event types
		if (trackingView == NULL && event.type == Event::MouseUp) {
			// window is not the origin of the down event
			// we then ignore this
			return false;
		}

		Point screenPos = event.mouse.Pos();
		if (!frame.PointInside(screenPos)) {
			// this can hapen if the window is modal since it will absorb all events
			// the window manager maybe shouldnt dispatch the events in this case
			// but this is a public function and its possible to post a phoney event from anywhere anyway
			return true;
		}

		target = SubviewAt(ConvertPointFromScreen(screenPos), false, true);
		assert(target == NULL || target->IsVisible());

		// special event handling
		switch (event.type) {
			case Event::MouseScroll:
				// retarget if NULL or disabled
				{
					Point delta = event.mouse.Delta();
					if (target && !target->IsDisabled()) {
						if (target->OnMouseWheelScroll(delta)) {
							return true;
						}
					}
					
					if (focusView && !focusView->IsDisabled()) {
						if (focusView->OnMouseWheelScroll(delta)) {
							return true;
						}
						
					}
					
					return OnMouseWheelScroll(delta);
				}
			case Event::MouseMove:
				// allows NULL and disabled targets
				DispatchMouseMotion(target, event.mouse);
				return true;
			default:
				if (target == NULL) {
					target = this;
				} else if (target->IsDisabled()) {
					return true; // we still absorb the event
				}
				break;
		}

		assert(target);
		// basic event handling
		switch (event.type) {
			case Event::MouseDown:
				DispatchMouseDown(target, event.mouse, event.mod);
				break;
			case Event::MouseUp:
				DispatchMouseUp(target, event.mouse, event.mod);
				break;
			default: return false; // we dont handle touch events yet...
		}
		// absorb other screen events i guess
		return true;
	} else { // key events
		return DispatchKey(focusView, event);
	}
	return false;
}
	
bool Window::InHandler() const
{
	for (std::set<Control *>::iterator c = Controls.begin(); c != Controls.end(); ++c) {
		Control* ctrl = *c;
		if (ctrl->InHandler()) {
			return true;
		}
	}
	return false;
}

bool Window::RegisterHotKeyCallback(EventMgr::EventCallback* cb, KeyboardKey key)
{
	// FIXME: check if something already registerd and either return false or delete the old
	HotKeys[key] = cb;
	return true;
}

void Window::OnMouseDrag(const MouseEvent& me)
{
	assert(me.buttonStates);
	// dragging the window to a new position. only happens with left mouse.
	if ((flags&Draggable) && me.ButtonState(GEM_MB_ACTION) && trackingView == this) {
		Point newOrigin = frame.Origin() - me.Delta();
		SetFrameOrigin(newOrigin);
	} else {
		ScrollView::OnMouseDrag(me);
	}
}
	
void Window::OnMouseLeave(const MouseEvent& me, const DragOp*)
{
	DispatchMouseMotion(NULL, me);
	// FIXME?: should we call trackingView->OnMouseLeave?
	trackingView = NULL;
}

bool Window::OnKeyPress(const KeyboardEvent& key, unsigned short mod)
{
	if (Flags() & View::IgnoreEvents) {
		return false;
	}
	switch (key.keycode) {
		case GEM_ESCAPE:
			Close();
			return true;
	}
	return ScrollView::OnKeyPress(key, mod);
}
	
ViewScriptingRef* Window::CreateScriptingRef(ScriptingId id, ResRef group)
{
	return new WindowScriptingRef(this, id, group);
}

}
