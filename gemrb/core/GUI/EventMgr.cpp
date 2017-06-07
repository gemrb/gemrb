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

#include "globals.h"
#include "win32def.h"

namespace GemRB {

unsigned long EventMgr::DCDelay = 250;
bool EventMgr::TouchInputEnabled = true;

std::bitset<sizeof(short) * CHAR_BIT> EventMgr::mouseButtonFlags;
std::bitset<sizeof(short) * CHAR_BIT> EventMgr::modKeys;
Point EventMgr::mousePos;

std::map<int, EventMgr::EventCallback*> EventMgr::HotKeys = std::map<int, EventMgr::EventCallback*>();
EventMgr::EventTaps EventMgr::Taps = EventTaps();

bool EventMgr::ModState(unsigned short mod)
{
	return modKeys.test(1 >> mod);
}

bool EventMgr::ButtonState(unsigned short btn)
{
	return mouseButtonFlags.test(1 >> btn);
}

bool EventMgr::MouseDown()
{
	return mouseButtonFlags.any();
}

void EventMgr::DispatchEvent(Event& e)
{
	e.time = GetTickCount();

	// first check for hot key listeners
	if (e.EventMaskFromType(e.type) & Event::AllKeyMask) {
		// TODO: for key events: check repeat key delay and set if repeat

		int flags = e.mod << 16;
		flags |= e.keyboard.keycode;
		modKeys = e.mod;

		std::map<int, EventCallback*>::const_iterator hit;
		hit = HotKeys.find(flags);
		if (hit != HotKeys.end()) {
			EventCallback* cb = hit->second;
			if ((*cb)(e)) {
				return;
			}
		}
	} else if (e.EventMaskFromType(e.type) & Event::AllMouseMask) {
		if (e.EventMaskFromType(e.type) & (Event::MouseUpMask | Event::MouseDownMask)) {
			static unsigned long lastMouseDown = 0;
			static unsigned char repeatCount = 0;
			static EventButton repeatButton = 0;
			static Point repeatPos;

			if (e.type == Event::MouseDown) {
				if (e.mouse.button == repeatButton
					&& e.time <= lastMouseDown + DCDelay
					&& repeatPos.isWithinRadius(5, e.mouse.Pos())
				) {
					repeatCount++;
				} else {
					repeatCount = 1;
				}
				repeatPos = e.mouse.Pos();
				repeatButton = e.mouse.button;
				lastMouseDown = GetTickCount();
			}

			e.mouse.repeats = repeatCount;
		}

		// set/clear the appropriate buttons
		mouseButtonFlags = e.mouse.buttonStates;
		mousePos = e.mouse.Pos();
	} else {
		// TODO: implement touch events and controller events
	}

	// no hot keys or their listeners refused the event...
	EventTaps::iterator it = Taps.begin();
	for (; it != Taps.end(); ++it) {
		if (it->first & e.EventMaskFromType(e.type)) {
			// all matching taps get a copy of the event
			EventCallback* cb = it->second.get();
			(*cb)(e);
		}
	}
}

bool EventMgr::RegisterHotKeyCallback(EventCallback* cb, KeyboardKey key, short mod)
{
	// TODO: return false if something already claimed this hot key combo?
	// or maybe should we allow mutliple registrations and iterate them until one accepts the event...
	int flags = mod << 16;
	flags |= key;
	HotKeys.insert(std::make_pair(flags, cb));
	return true;
}

size_t EventMgr::RegisterEventMonitor(EventCallback* cb, Event::EventTypeMask mask)
{
	Taps.push_back(std::make_pair(mask, cb));
	// return the "id" of the inserted tap so it could be unregistered later on
	return Taps.size() - 1;
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
	e.type = (down) ? Event::KeyDown : Event::KeyUp;
	e.keyboard.keycode = key;
	e.isScreen = false;

	KeyboardKey character = 0;
	if (key >= 0x20) {
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

}
