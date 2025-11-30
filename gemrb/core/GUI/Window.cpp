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

#include "Debug.h"
#include "Interface.h"
#include "WindowManager.h"

#include "GUI/GUIScriptInterface.h"
#include "Logging/Logging.h"

#include <utility>

namespace GemRB {

Window::Window(const Region& frame, WindowManager& mgr)
	: ScrollView(frame), manager(mgr)
{
	lastMouseMoveTime = GetMilliseconds();

	SetFlags(DestroyOnClose, BitOp::OR);
	// default ingame windows to frameless
	if (core->HasCurrentArea()) {
		SetFlags(Borderless, BitOp::OR);
	}
	RecreateBuffer();
}

void Window::Close()
{
	// did we already get closed?
	if (GetScriptingRef()) {
		// fire the onclose handler prior to actually invalidating the window
		if (eventHandlers[Closed]) {
			eventHandlers[Closed](this);
		}
	}

	if (flags & DestroyOnClose) {
		ClearScriptingRefs();
		manager.CloseWindow(this);
	} else {
		// somebody wants to keep a handle to this window around to display it later
		manager.OrderBack(this);
		SetVisible(false);
	}

	trackingView = NULL;
	hoverView = NULL;
}

void Window::FocusLost()
{
	if (eventHandlers[LostFocus]) {
		eventHandlers[LostFocus](this);
	}
}

void Window::FocusGained()
{
	if (eventHandlers[GainedFocus]) {
		eventHandlers[GainedFocus](this);
	}
}

bool Window::HasFocus() const
{
	return manager.GetFocusWindow() == this;
}

bool Window::DisplayModal(ModalShadow shadow)
{
	modalShadow = shadow;
	return manager.PresentModalWindow(this);
}

/** Add a Control in the Window */
void Window::SubviewAdded(View* view, View* /*parent*/)
{
	Control* ctrl = dynamic_cast<Control*>(view);
	if (ctrl) {
		Controls.insert(ctrl);
	}

	// MoviePlayer at least relies on this
	if (focusView == NULL) {
		TrySetFocus(view);
	}
}

void Window::SubviewRemoved(View* subview, View* parent)
{
	assert(subview);
	Control* ctrl = dynamic_cast<Control*>(subview);
	if (ctrl) {
		Controls.erase(ctrl);
	}

	if (subview->ContainsView(trackingView)) {
		trackingView = NULL;
		drag = NULL;
	}

	if (subview->ContainsView(hoverView)) {
		hoverView = parent;
	}

	if (subview->ContainsView(focusView)) {
		focusView->DidUnFocus();
		focusView = NULL;
		for (auto control : Controls) {
			if (TrySetFocus(control) == control) {
				break;
			}
		}
	}
}

void Window::SizeChanged(const Size& /*oldSize*/)
{
	RecreateBuffer();
}

void Window::FlagsChanged(unsigned int oldflags)
{
	if ((flags & AlphaChannel) != (oldflags & AlphaChannel)) {
		RecreateBuffer();
	}

	if ((flags & View::Invisible) && focusView) {
		focusView->DidUnFocus();
	} else if ((oldflags & View::Invisible) && focusView) {
		focusView->DidFocus();
	}
}

void Window::RecreateBuffer()
{
	Video::BufferFormat fmt = (flags & AlphaChannel) ? Video::BufferFormat::DISPLAY_ALPHA : Video::BufferFormat::DISPLAY;
	backBuffer = VideoDriver->CreateBuffer(frame, fmt);

	UseScaleBuffer(scale, true);
	// the entire window must be invalidated, because the new buffer is blank
	// TODO: we *could* optimize this to instead blit the old buffer to the new one
	MarkDirty();
}

const VideoBufferPtr& Window::DrawWithoutComposition()
{
	View::Draw();

	VideoDriver->PopDrawingBuffer();
	return backBuffer;
}

void Window::DrawAfterSubviews(const Region& /*drawFrame*/, const Region& /*clip*/)
{
	if (IsDisabled()) {
		Region winrgn(Point(), Dimensions());
		VideoDriver->DrawRect(winrgn, ColorBlack, true, BlitFlags::HALFTRANS | BlitFlags::BLENDED);
	}
}

void Window::WillDraw(const Region& /*drawFrame*/, const Region& /*clip*/)
{
	backBuffer->SetOrigin(frame.origin);
	VideoDriver->PushDrawingBuffer(backBuffer);

	if (scaleBuffer) {
		scaleBuffer->SetOrigin(frame.origin);
		VideoDriver->PushDrawingBuffer(scaleBuffer);
	}
}

void Window::DidDraw(const Region& /*drawFrame*/, const Region& /*clip*/)
{
	if (InDebugMode(DebugMode::WINDOWS)) {
		VideoDriver->SetScreenClip(nullptr);

		auto lock = manager.DrawHUD();

		if (focusView) {
			Region r = focusView->ConvertRegionToScreen(Region(Point(), focusView->Dimensions()));
			VideoDriver->DrawRect(r, ColorWhite, false);
		}

		if (hoverView) {
			Region r = hoverView->ConvertRegionToScreen(Region(Point(), hoverView->Dimensions()));
			r.ExpandAllSides(-5);
			VideoDriver->DrawRect(r, ColorBlue, false);
		}

		if (trackingView) {
			Region r = trackingView->ConvertRegionToScreen(Region(Point(), trackingView->Dimensions()));
			r.ExpandAllSides(-10);
			VideoDriver->DrawRect(r, ColorRed, false);
		}
	}

	if (scaleBuffer) {
		VideoDriver->PopDrawingBuffer();
		VideoDriver->BlitVideoBufferFully(scaleBuffer, BlitFlags::NONE);
	}
}

void Window::Focus()
{
	if (!HasFocus()) {
		manager.FocusWindow(this);
	} else if (responderStack.empty()) {
		FocusGained(); // fire the focus handler anyway
	}
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

Holder<Sprite2D> Window::Cursor() const
{
	if (drag) {
		return drag->cursor;
	}

	Holder<Sprite2D> cursor = ScrollView::Cursor();
	if (cursor == NULL && hoverView) {
		cursor = hoverView->Cursor();
	}
	return cursor;
}

bool Window::IsDisabledCursor() const
{
	bool isDisabled = ScrollView::IsDisabledCursor();
	if (hoverView) {
		// if either the window or view is in a disabled state the cursor will be
		isDisabled = isDisabled || hoverView->IsDisabledCursor();
	}
	return isDisabled;
}

bool Window::IsReceivingEvents() const
{
	return !IsDisabled();
}

void Window::SetPosition(WindowPosition pos)
{
	// start at top left
	Region newFrame(Point(), frame.size);
	Size screen = manager.ScreenSize();

	// adjust horizontal
	if ((pos & PosHmid) == PosHmid) {
		newFrame.x = (screen.w / 2) - (newFrame.w) / 2;
	} else if (pos & PosRight) {
		newFrame.x = screen.w - newFrame.w;
	}

	// adjust vertical
	if ((pos & PosVmid) == PosVmid) {
		newFrame.y = (screen.h / 2) - (newFrame.h) / 2;
	} else if (pos & PosBottom) {
		newFrame.y = screen.h - newFrame.h;
	}
	SetFrame(newFrame);
}

void Window::RedrawControls(const Control::varname_t& VarName) const
{
	Control::value_t val = core->GetDictionary().Get(VarName, Control::INVALID_VALUE);

	for (auto ctrl : Controls) {
		ctrl->UpdateState(VarName, val);
	}
}

View* Window::TrySetFocus(View* target)
{
	View* newFocus = focusView;
	if (target && !target->CanLockFocus()) {
		// target won't accept focus so dont bother unfocusing current
	} else if (focusView && !focusView->CanUnlockFocus()) {
		// current focus unwilling to reliquish
	} else {
		if (focusView)
			focusView->DidUnFocus();

		newFocus = target;

		if (newFocus)
			newFocus->DidFocus();
	}
	focusView = newFocus;

	return newFocus;
}

bool Window::IsDraggable() const
{
	if (trackingView != this) {
		return false;
	}

	return (flags & Draggable) ||
		(EventMgr::ModState(GEM_MOD_CTRL) && EventMgr::MouseButtonState(GEM_MB_ACTION));
}

bool Window::HitTest(const Point& p) const
{
	bool hit = View::HitTest(p);
	if (hit == false) {
		// check the control list. we could make View::HitTest optionally recursive, but this is cheaper
		for (const auto& ctrl : Controls) {
			if (ctrl->IsVisible() && ctrl->View::HitTest(ctrl->ConvertPointFromWindow(p))) {
				hit = true;
				break;
			}
		}
	}
	return hit;
}

void Window::SetAction(Responder handler, const ActionKey& key)
{
	eventHandlers[key.Value()] = std::move(handler);
}

bool Window::PerformAction(const ActionKey& key)
{
	const auto& handler = eventHandlers[key.Value()];
	if (handler) {
		(handler)(this);
		return true;
	}
	return false;
}

bool Window::SupportsAction(const ActionKey& key)
{
	return eventHandlers[key.Value()];
}

void Window::DispatchMouseMotion(View* target, const MouseEvent& me)
{
	if (hoverView && target != hoverView) {
		hoverView->MouseLeave(me, drag.get());
	}

	if (target && target != hoverView) {
		// must create the drag event before calling MouseEnter
		target->MouseEnter(me, drag.get());
	}

	if (trackingView && Distance(dragOrigin, me.Pos()) > EventMgr::mouseDragRadius) {
		// tracking will eat this event
		if (me.buttonStates) {
			trackingView->MouseDrag(me);
			if (trackingView == target && drag == nullptr) {
				drag = trackingView->DragOperation();
			}
		} else {
			trackingView = NULL;
		}
	} else if (target) {
		target->MouseOver(me);
	}
	hoverView = target;
}

void Window::DispatchMouseDown(View* target, const MouseEvent& me, unsigned short mod)
{
	assert(target);

	if (me.button == GEM_MB_ACTION && !(Flags() & View::IgnoreEvents)) {
		Focus();
	}

	TrySetFocus(target);
	target->MouseDown(me, mod);
	trackingView = target; // all views track the mouse within their bounds
	dragOrigin = me.Pos();
	assert(me.buttonStates);
}

void Window::DispatchMouseUp(View* target, const MouseEvent& me, unsigned short mod)
{
	assert(target);

	if (drag && drag->dragView != target && target->AcceptsDragOperation(*drag)) {
		drag->dropView = target;
		target->CompleteDragOperation(*drag);
	} else if (trackingView) {
		if (trackingView == target || trackingView->TracksMouseDown()) {
			trackingView->MouseUp(me, mod);
		}
	} else if (target) {
		target->MouseUp(me, mod);
	}
	drag = NULL;
	trackingView = NULL;
}

void Window::DispatchTouchDown(View* target, const TouchEvent& te, unsigned short mod)
{
	assert(target);

	if (te.numFingers == 1 && !(Flags() & View::IgnoreEvents)) {
		Focus();
	}

	TrySetFocus(target);
	target->TouchDown(te, mod);
	trackingView = target; // all views track the mouse within their bounds
}

void Window::DispatchTouchUp(View* target, const TouchEvent& te, unsigned short mod)
{
	assert(target);

	if (drag && te.numFingers == 1) {
		if (target->AcceptsDragOperation(*drag) && drag->dragView != target) {
			drag->dropView = target;
			target->CompleteDragOperation(*drag);
		}
	} else if (trackingView) {
		if (trackingView == target || trackingView->TracksMouseDown())
			trackingView->TouchUp(te, mod);
	} else if (target) {
		target->TouchUp(te, mod);
	}
	drag = NULL;
	trackingView = NULL;
}

void Window::DispatchTouchGesture(View* target, const GestureEvent& gesture)
{
	// FIXME: this is incomplete
	// this should be a bit closer to DispatchMouseMotion
	// drag and drop for example won't function

	//trackingView = target;
	target->TouchGesture(gesture);
}

bool Window::DispatchKey(View* keyView, const Event& event)
{
	// hotkeys first
	std::map<KeyboardKey, EventMgr::EventCallback>::iterator it = HotKeys.find(event.keyboard.keycode);
	if (it != HotKeys.end()) {
		return (it->second)(event);
	}

	// try the keyView view first, if it fails have the window itself try
	bool handled = false;
	if (keyView) {
		handled = (event.type == Event::KeyDown) ? keyView->KeyPress(event.keyboard, event.mod) : keyView->KeyRelease(event.keyboard, event.mod);
	}

	if (!handled) {
		// FIXME: using OnKeyPress to avoid eventProxy from eating esc key
		// would be better if we could delegate proxies for different event classes (keys, mouse, etc)
		handled = (event.type == Event::KeyDown) ? OnKeyPress(event.keyboard, event.mod) : OnKeyRelease(event.keyboard, event.mod);
	}
	return handled;
}

bool Window::DispatchEvent(const Event& event)
{
	View* target = NULL;

	if (event.type == Event::TextInput) {
		if (focusView) focusView->TextInput(event.text);
		return true;
	}

	if (!event.isScreen) { // key events
		return DispatchKey(focusView, event);
	}

	if (event.type == Event::TouchGesture) {
		if (trackingView) {
			DispatchTouchGesture(trackingView, event.gesture);
		}
		return true;
	}

	Point screenPos = event.mouse.Pos();
	if (!frame.PointInside(screenPos) && trackingView == nullptr) {
		// this can happen if the window is modal since it will absorb all events
		// the window manager maybe shouldnt dispatch the events in this case
		// but this is a public function and its possible to post a phoney event from anywhere anyway
		return true;
	}

	target = SubviewAt(ConvertPointFromScreen(screenPos), false, true);
	assert(target == nullptr || target->IsVisible());

	if (IsDraggable() && target == nullptr) {
		target = this;
	}

	// special event handling
	switch (event.type) {
		case Event::MouseScroll:
			// retarget if NULL or disabled
			{
				Point delta = event.mouse.Delta();
				if (target == nullptr || target->IsDisabled()) {
					target = this;
				}

				target->MouseWheelScroll(delta);
				return true;
			}
		case Event::MouseMove:
			// allows NULL and disabled targets
			if (target == this) {
				// skip the usual dispatch
				// this is so that we can move windows that otherwise ignore events
				OnMouseDrag(event.mouse);
			} else {
				DispatchMouseMotion(target, event.mouse);
			}
			return true;
		default:
			bool reset = false;
			if (event.type == Event::MouseDown && event.mouse.button == GEM_MB_MENU) {
				reset = true;
			}
			if (target == NULL) {
				target = this;
				if (reset) core->ResetActionBar();
			} else if (target->IsDisabled()) {
				if (reset) core->ResetActionBar();
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
		case Event::TouchDown:
			DispatchTouchDown(target, event.touch, event.mod);
			break;
		case Event::TouchUp:
			DispatchTouchUp(target, event.touch, event.mod);
			break;
		default:
#ifdef USE_SDL_CONTROLLER_API
			// If controller api is not used, this maybe reached if a controller is used.
			// others should be handled above
			Log(ERROR, "Window", "Unhandled event type generated: {}", event.type);
#endif
			break;
	}
	// absorb other screen events i guess
	return true;
}

bool Window::InActionHandler() const
{
	for (const auto& ctrl : Controls) {
		if (ctrl->IsExecutingResponseHandler()) {
			return true;
		}
	}

	return !responderStack.empty();
}

bool Window::RegisterHotKeyCallback(EventMgr::EventCallback cb, KeyboardKey key)
{
	if (key < ' ') { // allowing certain non printables (eg 'F' keys)
		return false;
	}

	std::map<KeyboardKey, EventMgr::EventCallback>::iterator it;
	it = HotKeys.find(key);
	if (it != HotKeys.end()) {
		// something already registered
		HotKeys.erase(it);
	}

	HotKeys[key] = std::move(cb);
	return true;
}

bool Window::UnRegisterHotKeyCallback(const EventMgr::EventCallback& cb, KeyboardKey key)
{
	KeyMap::iterator it = HotKeys.find(key);
	if (it != HotKeys.end() && FunctionTargetsEqual(it->second, cb)) {
		HotKeys.erase(it);
		return true;
	}
	return false;
}

bool Window::OnMouseDrag(const MouseEvent& me)
{
	assert(me.buttonStates);
	// dragging the window to a new position. only happens with left mouse.
	if (IsDraggable()) {
		Point newOrigin = frame.origin - me.Delta();
		SetFrameOrigin(newOrigin);
	} else {
		ScrollView::OnMouseDrag(me);
	}
	return true;
}

void Window::OnMouseLeave(const MouseEvent& me, const DragOp*)
{
	DispatchMouseMotion(NULL, me);
}

bool Window::OnKeyPress(const KeyboardEvent& key, unsigned short mod)
{
	if (Flags() & View::IgnoreEvents) {
		return false;
	}

	if (key.keycode == GEM_ESCAPE && mod == 0) {
		Close();
		return true;
	}

	if (ScrollView::OnKeyPress(key, mod)) {
		return true;
	}

	// This way, we can assign GameControl as EP and handle some left-over keys,
	// e.g. Pause from the Container window.
	if (GetEventProxy()) {
		return View::KeyPress(key, mod);
	}

	return false;
}

bool Window::OnControllerButtonDown(const ControllerEvent& ce)
{
	if (ce.button == CONTROLLER_BUTTON_BACK) {
		Close();
		return true;
	}

	return View::OnControllerButtonDown(ce);
}

ViewScriptingRef* Window::CreateScriptingRef(ScriptingId id, ScriptingGroup_t group)
{
	return new WindowScriptingRef(this, id, group);
}

void Window::DropScaleBuffer()
{
	scale = 100;
	scaleBuffer = nullptr;
}

void Window::UseScaleBuffer(unsigned int percent, bool force)
{
	if (!((force && percent != 100) || scale != percent)) {
		return;
	}

	DropScaleBuffer();
	scale = percent;
	if (percent == 100) {
		return;
	}

	auto r = backBuffer->Rect();
	r.Scale(percent);

	Video::BufferFormat fmt = (flags & AlphaChannel) ? Video::BufferFormat::DISPLAY_ALPHA : Video::BufferFormat::DISPLAY;
	scaleBuffer = VideoDriver->CreateBuffer(r, fmt);
}

}
