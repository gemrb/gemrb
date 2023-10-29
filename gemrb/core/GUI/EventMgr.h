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

/**
 * @file EventMgr.h
 * Declares EventMgr, class distributing events from input devices to GUI windows
 * @author The GemRB Project
 */


#ifndef EVENTMGR_H
#define EVENTMGR_H

#include "exports.h"
#include "ie_types.h"
#include "globals.h"

#include "Callback.h"
#include "Region.h"
#include "Strings/String.h"

#include <bitset>
#include <climits>
#include <cstdint>
#include <list>
#include <map>
#include <vector>

namespace GemRB {

class Control;
class Window;

#define GEM_LEFT		0x81
#define GEM_RIGHT		0x82
#define GEM_UP			0x83
#define GEM_DOWN		0x84
#define GEM_DELETE		0x85
#define GEM_RETURN		0x86
#define GEM_BACKSP		0x87
#define GEM_TAB			0x88
#define GEM_ALT			0x89
#define GEM_HOME		0x8a
#define GEM_END			0x8b
#define GEM_ESCAPE		0x8c
#define GEM_PGUP		0x8d
#define GEM_PGDOWN		0x8e
#define GEM_GRAB		0x8f

// 0x90 - 0x9f reserved for function keys (1-16)
#define GEM_FUNCTIONX(x) \
	((KeyboardKey)(0x8F + x))

#define GEM_MOD_SHIFT           1
#define GEM_MOD_CTRL            2
#define GEM_MOD_ALT             4

// Mouse buttons
#define GEM_MB_ACTION           1
#define GEM_MB_MIDDLE           2
#define GEM_MB_MENU             4

#define FINGER_MAX 5

enum InputAxis : int8_t {
	AXIS_INVALID = -1,
	AXIS_LEFT_X,
	AXIS_LEFT_Y,
	AXIS_RIGHT_X,
	AXIS_RIGHT_Y
};

enum ControllerButton : int8_t {
	CONTROLLER_INVALID = -1,
	CONTROLLER_BUTTON_A,
	CONTROLLER_BUTTON_B,
	CONTROLLER_BUTTON_X,
	CONTROLLER_BUTTON_Y,
	CONTROLLER_BUTTON_BACK,
	CONTROLLER_BUTTON_GUIDE,
	CONTROLLER_BUTTON_START,
	CONTROLLER_BUTTON_LEFTSTICK,
	CONTROLLER_BUTTON_RIGHTSTICK,
	CONTROLLER_BUTTON_LEFTSHOULDER,
	CONTROLLER_BUTTON_RIGHTSHOULDER,
	CONTROLLER_BUTTON_DPAD_UP,
	CONTROLLER_BUTTON_DPAD_DOWN,
	CONTROLLER_BUTTON_DPAD_LEFT,
	CONTROLLER_BUTTON_DPAD_RIGHT
};

using EventButton = unsigned char;
using KeyboardKey = unsigned short;
using ButtonMask = unsigned short;

struct GEM_EXPORT EventBase {
	unsigned short repeats; // number of times this event has been repeated (within the repeat time interval)
};

struct GEM_EXPORT ScreenEvent : public EventBase {
	// can't use Point due to non-trivial constructor
	int x,y; // mouse position at time of event
	int deltaX, deltaY; // the vector of motion/scroll

	Point Pos() const { return Point(x,y); }
	Point Delta() const { return Point(deltaX, deltaY); }
};

struct GEM_EXPORT MouseEvent : public ScreenEvent {
	ButtonMask buttonStates;
	EventButton button;

	bool ButtonState(unsigned short btn) const {
		return buttonStates & btn;
	}
};

struct GEM_EXPORT KeyboardEvent : public EventBase {
	KeyboardKey keycode; // raw keycode
	KeyboardKey character; // the translated character
};

struct GEM_EXPORT TextEvent : public EventBase {
	String text; // activate the soft keyboard and disable hot keys until next (non TextEvent) event
};

struct GEM_EXPORT ControllerEvent : public EventBase {
	InputAxis axis;
	float axisPct;
	int axisDelta;
	ButtonMask buttonStates;
	EventButton button;
};

struct GEM_EXPORT TouchEvent : public ScreenEvent {
	struct GEM_EXPORT Finger : public ScreenEvent {
		uint64_t id;
	};

	Finger fingers[FINGER_MAX];

	int numFingers;
	float pressure;
};

struct GEM_EXPORT GestureEvent : public TouchEvent {
	float dTheta;
	float dDist;
};

struct GEM_EXPORT ApplicationEvent : public EventBase {
};

struct GEM_EXPORT Event {
	enum EventType {
		MouseMove = 0,
		MouseUp,
		MouseDown,
		MouseScroll,

		KeyUp,
		KeyDown,

		TouchGesture,
		TouchUp,
		TouchDown,

		TextInput, // clipboard or faux event sent to signal the soft keyboard+temp disable hotkeys
		
		ControllerAxis,
		ControllerButtonUp,
		ControllerButtonDown,

		// Something wants the whole screen to be refreshed (SDL backend, OS, ...)
		RedrawRequest
		// leaving off types for unused events
	};

	enum EventTypeMask {
		NoEventsMask = 0U,

		MouseMoveMask = 1 << MouseMove,
		MouseUpMask = 1 << MouseUp,
		MouseDownMask = 1 << MouseDown,
		MouseScrollMask = 1 << MouseScroll,

		AllMouseMask = MouseMoveMask | MouseUpMask | MouseDownMask | MouseScrollMask,

		KeyUpMask = 1 << KeyUp,
		KeyDownMask = 1 << KeyDown,

		AllKeyMask = KeyUpMask | KeyDownMask,

		TouchGestureMask = 1 << TouchGesture,
		TouchUpMask = 1 << TouchUp,
		TouchDownMask = 1 << TouchDown,

		AllTouchMask = TouchGestureMask | TouchUpMask | TouchDownMask,

		TextInputMask = 1 << TextInput,
		
		ControllerAxisMask = 1 << ControllerAxis,
		ControllerButtonUpMask = 1 << ControllerButtonUp,
		ControllerButtonDownMask = 1 << ControllerButtonDown,
		
		AllControllerMask = ControllerAxisMask | ControllerButtonUpMask | ControllerButtonDownMask,

		RedrawRequestMask = 1 << RedrawRequest,
		AllApplicationMask = RedrawRequestMask,

		AllEventsMask = 0xffffffffU
	};

	static EventTypeMask EventMaskFromType (EventType type) { return static_cast<EventTypeMask>(1U << type); };

	union {
		MouseEvent mouse;
		ControllerEvent controller;
		KeyboardEvent keyboard;
		TouchEvent touch;
		GestureEvent gesture;
		ApplicationEvent application;
	};

	TextEvent text; // text is nontrivial so it stands alone (until C++11 is allowed)

	using EventMods = unsigned short;

	EventType type;
	tick_t time;
	EventMods mod; // modifier keys held during the event
	bool isScreen; // event coresponsds to location on screen
};

MouseEvent MouseEventFromTouch(const TouchEvent& te, bool down);
MouseEvent MouseEventFromController(const ControllerEvent& ce, bool down);
KeyboardEvent KeyEventFromController(const ControllerEvent& ce);

/**
 * @class EventMgr
 * Class distributing events from input devices to GUI windows.
 * The events are pumped into instance of this class from a Video driver plugin
 */

class GEM_EXPORT EventMgr {
public:
	using buttonbits = std::bitset<sizeof(short) * CHAR_BIT>;
	using EventCallback = Callback<bool, const Event&>;
	using TapMonitorId = size_t;
	
	static constexpr int mouseClickRadius = 5; // radius for repeat click events
	static constexpr int mouseDragRadius = 10; // radius for drag events

	static tick_t DCDelay;
	static tick_t DPDelay;
	static bool TouchInputEnabled;

	static Event CreateMouseBtnEvent(const Point& pos, EventButton btn, bool down, int mod = 0);
	static Event CreateMouseMotionEvent(const Point& pos, int mod = 0);
	static Event CreateMouseWheelEvent(const Point& vec, int mod = 0);

	static Event CreateKeyEvent(KeyboardKey key, bool down, int mod = 0);

	static Event CreateTouchEvent(const TouchEvent::Finger fingers[], int numFingers, bool down, float pressure = 0.0);
	static Event CreateTouchGesture(const TouchEvent& touch, float rotation, float pinch);

	static Event CreateTextEvent(const char* text);
	static Event CreateTextEvent(const String& text);
	
	static Event CreateControllerAxisEvent(InputAxis axis, int delta, float pct);
	static Event CreateControllerButtonEvent(EventButton btn, bool down);

	static Event CreateRedrawRequestEvent();

	static bool RegisterHotKeyCallback(const EventCallback&, KeyboardKey key, short mod = 0);
	static void UnRegisterHotKeyCallback(const EventCallback&, KeyboardKey key, short mod = 0);
	static TapMonitorId RegisterEventMonitor(const EventCallback&, Event::EventTypeMask mask = Event::AllEventsMask);
	static void UnRegisterEventMonitor(TapMonitorId monitor);

private:
	// FIXME: this shouldnt really be static... but im not sure we want direct access to the EventMgr instance...
	// currently the delays are static so it makes sense for now that the HotKeys are...
	// map combination of keyboard key and modifier keys to a callback
	using EventTaps = std::map<TapMonitorId, std::pair<Event::EventTypeMask, EventCallback>>;
	using KeyMap = std::map<int, std::list<EventCallback>>;

	static EventTaps Taps;
	static KeyMap HotKeys;

	static buttonbits mouseButtonFlags;
	static buttonbits modKeys;
	static Point mousePos;

	static std::map<uint64_t, TouchEvent::Finger> fingerStates;
	
	static buttonbits controllerButtonStates;

public:
	void DispatchEvent(Event&& e) const;

	static bool ModState(unsigned short mod);

	static bool MouseButtonState(EventButton btn);
	static bool MouseDown();
	static Point MousePos() { return mousePos; }

	static const TouchEvent::Finger* FingerState(uint64_t id) { return (fingerStates.count(id)) ? &fingerStates[id] : NULL; };
	static bool FingerDown();
	static ieByte NumFingersDown() { return fingerStates.size(); };
	
	static bool ControllerButtonState(EventButton btn);
};

}

#endif // ! EVENTMGR_H
