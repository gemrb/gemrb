/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

#include "GUI/EventMgr.h"
#include "Interface.h"
#include "Video/Video.h"

#include "globals.h"

namespace GemRB {

tick_t EventMgr::DCDelay = 250;
tick_t EventMgr::DPDelay = 250;
bool EventMgr::TouchInputEnabled = false;

EventMgr::buttonbits EventMgr::mouseButtonFlags;
EventMgr::buttonbits EventMgr::modKeys;
Point EventMgr::mousePos;
std::map<uint64_t, TouchEvent::Finger> EventMgr::fingerStates;
EventMgr::buttonbits EventMgr::controllerButtonStates;

EventMgr::KeyMap EventMgr::HotKeys = KeyMap();
EventMgr::EventTaps EventMgr::Taps = EventTaps();

MouseEvent MouseEventFromTouch(const TouchEvent& te, bool down)
{
	MouseEvent me {};
	me.x = te.x;
	me.y = te.y;
	me.deltaX = te.deltaX;
	me.deltaY = te.deltaY;

	me.buttonStates = down ? GEM_MB_ACTION : 0;
	me.button = GEM_MB_ACTION;
	me.repeats = 1;

	return me;
}

MouseEvent MouseEventFromController(const ControllerEvent& ce, bool down)
{
	Point p = EventMgr::MousePos();

	MouseEvent me {};
	me.x = p.x;
	me.y = p.y;

	if (ce.axis % 2) {
		me.deltaX = ce.axisDelta;
		me.deltaY = 0;
	} else {
		me.deltaX = 0;
		me.deltaY = ce.axisDelta;
	}

	EventButton btn = 0;
	switch (ce.button) {
		case CONTROLLER_BUTTON_A:
			btn = GEM_MB_ACTION;
			break;
		case CONTROLLER_BUTTON_B:
			btn = GEM_MB_MENU;
			break;
		case CONTROLLER_BUTTON_LEFTSTICK:
			btn = GEM_MB_MIDDLE;
			break;
	}

	me.buttonStates = down ? btn : 0;
	me.button = btn;

	return me;
}

KeyboardEvent KeyEventFromController(const ControllerEvent& ce)
{
	KeyboardEvent ke{};

	// TODO: probably want more than the DPad
	switch (ce.button) {
		case CONTROLLER_BUTTON_DPAD_UP:
			ke.keycode = GEM_UP;
			break;
		case CONTROLLER_BUTTON_DPAD_DOWN:
			ke.keycode = GEM_DOWN;
			break;
		case CONTROLLER_BUTTON_DPAD_LEFT:
			ke.keycode = GEM_LEFT;
			break;
		case CONTROLLER_BUTTON_DPAD_RIGHT:
			ke.keycode = GEM_RIGHT;
			break;
		default:
			ke.keycode = 0;
			break;
	}

	return ke;
}

bool EventMgr::ModState(unsigned short mod)
{
	return (modKeys & buttonbits(mod)).any();
}

bool EventMgr::MouseButtonState(EventButton btn)
{
	return (mouseButtonFlags & buttonbits(btn)).any();
}

bool EventMgr::MouseDown()
{
	return mouseButtonFlags.any();
}

bool EventMgr::FingerDown()
{
	return !fingerStates.empty();
}

bool EventMgr::ControllerButtonState(EventButton btn)
{
	return (controllerButtonStates & buttonbits(btn)).any();
}

void EventMgr::DispatchEvent(Event&& e) const
{
	if (TouchInputEnabled == false && e.EventMaskFromType(e.type) & Event::AllTouchMask) {
		return;
	}

	Video* video = core->GetVideoDriver();

	e.time = GetTicks();

	if (e.type == Event::TextInput) {
		if (e.text.text.length() == 0) {
			return;
		}
	} else if (e.EventMaskFromType(e.type) & Event::AllKeyMask) {
		// first check for hot key listeners
		static tick_t lastKeyDown = 0;
		static unsigned char repeatCount = 0;
		static KeyboardKey repeatKey = 0;

		if (e.type == Event::KeyDown) {
			if (e.keyboard.keycode == repeatKey && e.time <= lastKeyDown + DPDelay) {
				repeatCount++;
			} else {
				repeatCount = 1;
			}
			repeatKey = e.keyboard.keycode;
			lastKeyDown = GetTicks();
		}

		e.keyboard.repeats = repeatCount;

		int flags = e.mod << 16;
		flags |= e.keyboard.keycode;
		modKeys = e.mod;

		KeyMap::const_iterator hit = HotKeys.find(flags);
		if (hit != HotKeys.end()) {
			assert(!hit->second.empty());
			KeyMap::value_type::second_type list = hit->second;
			EventCallback cb = hit->second.front();
			if (cb(e)) {
				return;
			}
		}
	} else if (e.EventMaskFromType(e.type) & Event::ControllerAxisMask) {
		// only the left stick moves the cursor
		if (e.controller.axis == AXIS_LEFT_X) {
			mousePos.x += e.controller.axisDelta;
		} else if (e.controller.axis == AXIS_LEFT_Y) {
			mousePos.y += e.controller.axisDelta;
		}
	} else if (e.EventMaskFromType(e.type) & (Event::ControllerButtonUpMask | Event::ControllerButtonDownMask)) {
		controllerButtonStates = e.controller.buttonStates;
	} else if (e.isScreen) {
		if (e.EventMaskFromType(e.type) & (Event::MouseUpMask | Event::MouseDownMask
									    | Event::TouchUpMask | Event::TouchDownMask)
		) {
			// WARNING: these are shared between mouse and touch
			// it is assumed we won't be using both simultaneously
			static tick_t lastMouseDown = 0;
			static unsigned char repeatCount = 0;
			static EventButton repeatButton = 0;
			static Point repeatPos;

			ScreenEvent& se = (e.type == Event::MouseDown) ? static_cast<ScreenEvent&>(e.mouse) : static_cast<ScreenEvent&>(e.touch);
			EventButton btn = (e.type == Event::MouseDown) ? e.mouse.button : 0;

			if (e.type == Event::MouseDown || e.type == Event::TouchDown) {
				core->GetVideoDriver()->CaptureMouse(true);
				if (video->InTextInput())
					video->StopTextInput();

				if (btn == repeatButton
					&& e.time <= lastMouseDown + DCDelay
					&& repeatPos.isWithinRadius(mouseClickRadius, se.Pos())
				) {
					repeatCount++;
				} else {
					repeatCount = 1;
				}
				repeatPos = se.Pos();
				repeatButton = btn;
				lastMouseDown = GetTicks();
			} else if (e.type == Event::MouseUp && e.mouse.buttonStates == 0) {
				core->GetVideoDriver()->CaptureMouse(false);
			}

			se.repeats = repeatCount;
		}

		if (e.EventMaskFromType(e.type) & Event::AllMouseMask) {
			// set/clear the appropriate buttons
			mouseButtonFlags = e.mouse.buttonStates;
			mousePos = e.mouse.Pos();
		} else { // touch
			const TouchEvent& te = e.type == Event::TouchGesture ? static_cast<const TouchEvent&>(e.gesture) : static_cast<const TouchEvent&>(e.touch);
			uint64_t id = te.fingers[0].id;

			switch (e.type) {
				case Event::TouchDown:
					fingerStates[id] = te.fingers[0];
					break;

				case Event::TouchUp:
					fingerStates.erase(id);
					break;

				case Event::TouchGesture:
					fingerStates.clear();
					for (int i = 0; i < te.numFingers; ++i) {
						id = te.fingers[i].id;
						fingerStates[id] = te.fingers[i];
					}
					break;

				default:
					break;
			}

			if (te.numFingers == 1) {
				mousePos = e.touch.Pos();
			}
		}
	} else {
		if (video->InTextInput())
			video->StopTextInput();
	}

	// no hot keys or their listeners refused the event...
	EventTaps tapsCopy = Taps; // copy this because its possible things get unregistered as a result of the event
	EventTaps::iterator it = tapsCopy.begin();
	for (; it != tapsCopy.end(); ++it) {
		const std::pair<Event::EventTypeMask, EventCallback>& pair = it->second;
		if (pair.first & e.EventMaskFromType(e.type)) {
			// all matching taps get a copy of the event
			pair.second(e);
		}
	}
}

bool EventMgr::RegisterHotKeyCallback(const EventCallback& cb, KeyboardKey key, short mod)
{
	if (key < ' ') { // allowing certain non printables (eg 'F' keys)
		return false;
	}

	int flags = mod << 16;
	flags |= key;

	KeyMap::iterator it = HotKeys.find(flags);
	if (it != HotKeys.end()) {
		it->second.push_front(cb);
	} else {
		HotKeys.insert(std::make_pair(flags, std::list<EventCallback>(1, cb)));
	}

	return true;
}

void EventMgr::UnRegisterHotKeyCallback(const EventCallback& cb, KeyboardKey key, short mod)
{
	int flags = mod << 16;
	flags |= key;

	KeyMap::iterator it = HotKeys.find(flags);
	if (it != HotKeys.end()) {
		KeyMap::value_type::second_type::iterator cbit;
		cbit = std::find_if(it->second.begin(), it->second.end(), [&cb](decltype(cb)& item) {
			return FunctionTargetsEqual(cb, item);
		});
		if (cbit != it->second.end()) {
			it->second.erase(cbit);
			if (it->second.empty()) {
				HotKeys.erase(it);
			}
		}
	}
}

EventMgr::TapMonitorId EventMgr::RegisterEventMonitor(const EventCallback& cb, Event::EventTypeMask mask)
{
	static size_t id = 0;
	Taps[id] = std::make_pair(mask, cb);
	// return the "id" of the inserted tap so it could be unregistered later on
	return id++;
}

void EventMgr::UnRegisterEventMonitor(TapMonitorId monitor)
{
	Taps.erase(monitor);
}

Event EventMgr::CreateMouseBtnEvent(const Point& pos, EventButton btn, bool down, int mod)
{
	assert(btn);

	Event e = CreateMouseMotionEvent(pos, mod);

	if (down) {
		e.type = Event::MouseDown;
		e.mouse.buttonStates |= btn;
	} else {
		e.type = Event::MouseUp;
		e.mouse.buttonStates &= ~btn;
	}
	e.mouse.button = btn;

	return e;
}

Event EventMgr::CreateMouseMotionEvent(const Point& pos, int mod)
{
	Event e = {}; // initialize all members to 0
	e.mod = mod;
	e.type = Event::MouseMove;
	e.mouse.buttonStates = mouseButtonFlags.to_ulong();

	e.mouse.x = pos.x;
	e.mouse.y = pos.y;

	Point delta = MousePos() - pos;
	e.mouse.deltaX = delta.x;
	e.mouse.deltaY = delta.y;

	e.isScreen = true;

	return e;
}

Event EventMgr::CreateMouseWheelEvent(const Point& vec, int mod)
{
	Event e = CreateMouseMotionEvent(MousePos(), mod);
	e.type = Event::MouseScroll;
	e.mouse.deltaX = vec.x;
	e.mouse.deltaY = vec.y;
	return e;
}

Event EventMgr::CreateKeyEvent(KeyboardKey key, bool down, int mod)
{
	Event e = {}; // initialize all members to 0
	e.mod = mod;
	e.type = down ? Event::KeyDown : Event::KeyUp;
	e.keyboard.keycode = key;
	e.isScreen = false;

	KeyboardKey character = 0;
	if (key >= ' ' && key < GEM_LEFT) { // if printable
		// FIXME: need to translate the keycode for e.keyboard.character
		// probably need to lookup what encoding we are currently using
		character = key;
		if (mod & GEM_MOD_SHIFT) {
			character = toupper(character);
		}
	}
	e.keyboard.character = character;
	return e;
}

Event EventMgr::CreateTouchEvent(const TouchEvent::Finger fingers[], int numFingers, bool down, float pressure)
{
	if (numFingers > FINGER_MAX) {
		Log(ERROR, "EventManager", "cannot create a touch event with {} fingers; max is {}.", numFingers, FINGER_MAX);
		return Event();
	}

	Event e = {};
	e.isScreen = true;
	e.mod = 0;
	e.type = down ? Event::TouchDown : Event::TouchUp;

	if (numFingers) {
		for (int i = 0; i < numFingers; ++i) {
			e.touch.x += fingers[i].x;
			e.touch.y += fingers[i].y;
			if (abs(fingers[i].deltaX) > abs(e.touch.deltaX)) {
				e.touch.deltaX = fingers[i].deltaX;
			}
			if (abs(fingers[i].deltaY) > abs(e.touch.deltaY)) {
				e.touch.deltaY = fingers[i].deltaY;
			}
			e.touch.fingers[i] = fingers[i];
		}

		e.touch.x /= numFingers;
		e.touch.y /= numFingers;
	}

	e.touch.numFingers = numFingers;
	e.touch.pressure = pressure;

	return e;
}

Event EventMgr::CreateTouchGesture(const TouchEvent& touch, float rotation, float pinch)
{
	Event e = {};
	e.isScreen = true;
	e.mod = 0;
	e.type = Event::TouchGesture;
	e.touch = touch;
	e.gesture.dTheta = rotation;
	e.gesture.dDist = pinch;

	return e;
}

Event EventMgr::CreateTextEvent(const char* text)
{
	Event e = {};
	String* string = StringFromUtf8(text);
	if (string) {
		e = EventMgr::CreateTextEvent(*string);
		delete string;
	}
	return e;
}

Event EventMgr::CreateTextEvent(const String& text)
{
	Event e = {};
	e.type = Event::TextInput;
	e.text.text = text;

	return e;
}

Event EventMgr::CreateControllerAxisEvent(InputAxis axis, int delta, float pct)
{
	Event e = {};
	e.type = Event::ControllerAxis;
	e.controller.axis = axis;
	e.controller.axisPct = pct;
	e.controller.axisDelta = delta;
	e.isScreen = true;
	return e;
}

Event EventMgr::CreateControllerButtonEvent(EventButton btn, bool down)
{
	Event e = {};
	e.controller.buttonStates = controllerButtonStates.to_ulong();

	if (down) {
		e.type = Event::ControllerButtonDown;
		e.controller.buttonStates |= btn;
	} else {
		e.type = Event::ControllerButtonUp;
		e.controller.buttonStates &= ~btn;
	}
	e.controller.button = btn;

	return e;
}

Event EventMgr::CreateRedrawRequestEvent() {
	Event e = {};
	e.type = Event::RedrawRequest;

	return e;
}

}
