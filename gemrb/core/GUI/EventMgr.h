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
#define GEM_FUNCTION1           0x90     //don't use anything between these two and keep a round number
#define GEM_FUNCTION16          0x9f

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

struct ScreenEvent {
	// cant use Point due to non trival constructor
	int x,y; // mouse position at time of event
	int deltaX, deltaY; // the vector of motion/scroll

	Point Pos() const { return Point(x,y); }
};

struct GEM_EXPORT MouseEvent : public ScreenEvent {
	unsigned short buttonStates;
	EventButton button;
};

struct GEM_EXPORT KeyboardEvent {
	KeyboardKey keycode; // raw keycode
	KeyboardKey character; // the translated character
};

// TODO: Unused event type...
struct GEM_EXPORT ControllerEvent {
	unsigned short buttonStates;
	EventButton button;
};

// TODO: Unused event type...
struct GEM_EXPORT TouchEvent : public ScreenEvent {
	ScreenEvent fingers[5];

	unsigned short numFingers;
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

#define IS_KEY_EVENT(e) \
(e.type == Event::KeyUp || e.type == Event::KeyDown)

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

	static bool RegisterHotKeyCallback(EventCallback*, KeyboardKey key, short mod = 0);

private:
	typedef std::multimap<Event::EventType, EventCallback*> EventTaps;
	EventTaps taps;
	// we may register the same tap multiple times. should only delete once
	std::vector<EventCallback*> tapsToDelete;

	// FIXME: this shouldnt really be static... but im not sure we want direct access to the EventMgr instance...
	// currently the delays are static so it makes sense for now that the HotKeys are...
	// map combination of keyboard key and modifier keys to a callback
	static std::map<int, EventCallback*> HotKeys;

	unsigned long dc_time;
	unsigned long rk_flags;

public:
	EventMgr(void);
	~EventMgr(void);

	void AddEventTap(EventCallback* cb, Event::EventType = static_cast<Event::EventType>(-1));
	void DispatchEvent(Event& e);

	unsigned long GetRKDelay();
	unsigned long SetRKFlags(unsigned long arg, unsigned int op);

};

}

#endif // ! EVENTMGR_H
