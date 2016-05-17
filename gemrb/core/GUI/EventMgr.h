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

#include "Callback.h"
#include "Region.h"

#include <bitset>
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
	(0x8F + x)

#define GEM_MOD_SHIFT           1
#define GEM_MOD_CTRL            2
#define GEM_MOD_ALT             4

#define GEM_MOUSEOUT	128

// Mouse buttons
#define GEM_MB_ACTION           1
#define GEM_MB_MENU             4
#define GEM_MB_SCRLUP           8
#define GEM_MB_SCRLDOWN         16

#define GEM_MB_NORMAL           255
#define GEM_MB_DOUBLECLICK      256

#define GEM_RK_DOUBLESPEED      1
#define GEM_RK_DISABLE          2
#define GEM_RK_QUADRUPLESPEED   4

typedef unsigned char EventButton;
typedef unsigned short KeyboardKey;
typedef unsigned short ButtonMask;

struct ScreenEvent {
	// cant use Point due to non trival constructor
	int x,y; // mouse position at time of event
	int deltaX, deltaY; // the vector of motion/scroll

	Point Pos() const { return Point(x,y); }
};

struct GEM_EXPORT MouseEvent : public ScreenEvent {
	ButtonMask buttonStates;
	EventButton button;
};

struct GEM_EXPORT KeyboardEvent {
	KeyboardKey keycode; // raw keycode
	KeyboardKey character; // the translated character
};

// TODO: Unused event type...
struct GEM_EXPORT ControllerEvent {
	ButtonMask buttonStates;
	EventButton button;
};

// TODO: Unused event type...
struct GEM_EXPORT TouchEvent : public ScreenEvent {
	ScreenEvent fingers[5];

	ButtonMask numFingers;
	float pressure;
};

struct GEM_EXPORT Event {
	enum EventType {
		MouseMove = 0,
		MouseUp,
		MouseDown,
		MouseScroll,

		KeyUp,
		KeyDown

		// TODO: need types for touch and controller
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

		AllEventsMask = 0xffffffffU
	};

	static EventTypeMask EventMaskFromType (EventType type ) { return static_cast<EventTypeMask>(1U << type); };

	union {
		MouseEvent mouse;
		//ControllerEvent; unused currently
		KeyboardEvent keyboard;
		//TouchEvent touch; unused currently
	};

	EventType type;
	unsigned long time;
	short mod; // modifier keys held during the event
	unsigned short repeat; // number of times this event has been repeated (within the time interval)
	bool isScreen; // event coresponsds to location on screen
};

/**
 * @class EventMgr
 * Class distributing events from input devices to GUI windows.
 * The events are pumped into instance of this class from a Video driver plugin
 */

class GEM_EXPORT EventMgr {
public:
	typedef Callback<const Event&, bool> EventCallback;

	static unsigned long DCDelay;
	static unsigned long RKDelay;
	static bool TouchInputEnabled;

	static Event CreateMouseBtnEvent(const Point& pos, EventButton btn, bool down, int mod = 0);
	static Event CreateMouseMotionEvent(const Point& pos, int mod = 0);
	static Event CreateKeyEvent(KeyboardKey key, bool down, int mod = 0);

	// TODO/FIXME: need to be able to unregister hotkeys/monitors
	static bool RegisterHotKeyCallback(EventCallback*, KeyboardKey key, short mod = 0);
	static size_t RegisterEventMonitor(EventCallback*, Event::EventTypeMask mask = Event::AllEventsMask);

private:
	// FIXME: this shouldnt really be static... but im not sure we want direct access to the EventMgr instance...
	// currently the delays are static so it makes sense for now that the HotKeys are...
	// map combination of keyboard key and modifier keys to a callback
	typedef std::vector< std::pair<Event::EventTypeMask, Holder<EventCallback> > > EventTaps;

	static EventTaps Taps;
	static std::map<int, EventCallback*> HotKeys;

	static unsigned long dc_time;
	static unsigned long rk_flags;
	static std::bitset<16> mouseButtonFlags;
	static Point mousePos;

public:
	EventMgr(void);

	void DispatchEvent(Event& e);

	static unsigned long GetRKDelay();
	static unsigned long SetRKFlags(unsigned long arg, unsigned int op);

	static bool ButtonState(unsigned short btn);
	static bool MouseDown();
	static Point MousePos() { return mousePos; }
};

}

#endif // ! EVENTMGR_H
