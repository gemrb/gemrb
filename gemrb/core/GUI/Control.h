/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

/**
 * @file Control.h
 * Declares Control, root class for all widgets except of windows
 */

#ifndef CONTROL_H
#define CONTROL_H

#define IE_GUI_BUTTON		0
#define IE_GUI_PROGRESSBAR	1 //gemrb extension
#define IE_GUI_SLIDER		2
#define IE_GUI_EDIT		3
#define IE_GUI_TEXTAREA		5
#define IE_GUI_LABEL		6
#define IE_GUI_SCROLLBAR	7
#define IE_GUI_WORLDMAP         8 // gemrb extension
#define IE_GUI_MAP              9 // gemrb extension
#define IE_GUI_INVALID          255

#define IE_GUI_CONTROL_FOCUSED  0x80

//this is in the control ID
#define IGNORE_CONTROL 0x10000000

#include "RGBAColor.h"
#include "exports.h"
#include "win32def.h"

#include "Callback.h"
#include "GUI/View.h"
#include "Timer.h"

#include <limits>
#include <map>

namespace GemRB {

class ControlAnimation;
class Control;
class Sprite2D;
    
#define ACTION_CAST(a) \
static_cast<Control::Action>(a)
	
#define ACTION_IS_SCREEN(a) \
(a <= Control::Action::MouseLeave)
    
#define ACTION_DEFAULT ActionKey(Control::Action::Click, 0, GEM_MB_ACTION)
#define ACTION_CUSTOM(x)  ACTION_CAST(Control::Action::CustomAction + x)

class GEM_EXPORT ControlEventHandler : public Holder< Callback<Control*, void> > {
public:
	ControlEventHandler(Callback<Control*, void>* ptr = NULL)
	: Holder< Callback<Control*, void> >(ptr) {}

	void operator()(Control* ctrl) const {
		return (*ptr)(ctrl);
	}
};

/**
 * @class Control
 * Basic Control Object, also called widget or GUI element. Parent class for Labels, Buttons, etc.
 */

class GEM_EXPORT Control : public View {
private:
	Timer* StartActionTimer(const ControlEventHandler& action);
	void AddedToView(View* view);
	void RemovedFromView(View*);
    
public: // Public attributes
	enum Action {
		// !!! Keep these synchronized with GUIDefines.py !!!
		// screen events, send coords to callback
		Click,
		// avoid Drag and Hover if you can (will fire frequently). or throttle using SetActionInterval
		Drag,
		// "mouse" over, enter, leave
		Hover,
		HoverBegin,
		HoverEnd,
		
		// other events
		ValueChange, // many times we only care that the value has changed, not about the event that changed it
		
		// TODO: probably others will be needed
		
		CustomAction // entry value for defining custom actions in subclasses. Must be last in enum.
	};

    /** Variable length is 40-1 (zero terminator) */
    char VarName[MAX_VARIABLE_LENGTH];
    
    ControlAnimation* animation;
    Sprite2D* AnimPicture;
    
    /** Defines the Control ID Number used for GUI Scripting */
    ieDword ControlID;
    /** Type of control */
    ieByte ControlType;
    
    static unsigned int ActionRepeatDelay;

public:
	Control(const Region& frame, View* superview = NULL);
	virtual ~Control();

	virtual bool IsAnimated() const { return animation; }
	virtual bool IsOpaque() const { return true; }

	/** Sets the Text of the current control */
	void SetText(const String*);
	virtual void SetText(const String&) {};

	/** Update the control if it's tied to a GUI variable */
	void UpdateState(const char*, unsigned int);
	virtual void UpdateState(unsigned int) {}

	/** Returns the Owner */
	virtual void SetFocus();
	bool IsFocused();
    
	bool TracksMouseDown() const { return bool(actionTimer); }

    //Events
    void SetAction(ControlEventHandler handler); // default action (left mouse button up)
	void SetAction(ControlEventHandler handler, Action type, EventButton button = 0,
                   Event::EventMods mod = 0, short count = 0);
	void SetActionInterval(unsigned int interval=ActionRepeatDelay);
    /** Run specified handler, it may return error code */
    bool PerformAction(); // perform default action (left mouse button up)
	bool PerformAction(Action action);
	bool SupportsAction(Action action);

	virtual String QueryText() const { return String(); }
	/** Sets the animation picture ref */
	virtual void SetAnimPicture(Sprite2D* Picture);

	typedef std::pair<ieDword, ieDword> ValueRange;
	const static ValueRange MaxValueRange;
	ieDword GetValue() const { return Value; }
	ValueRange GetValueRange() const { return range; }
	void SetValue(ieDword val);
	void SetValueRange(ValueRange range = MaxValueRange);
	void SetValueRange(ieDword min, ieDword max = std::numeric_limits<ieDword>::max());

	void OnMouseUp(const MouseEvent& /*me*/, unsigned short /*Mod*/);
	void OnMouseDown(const MouseEvent& /*me*/, unsigned short /*Mod*/);
    // TODO: implement generic handlers for the other types of event actions
	
protected:
	struct ActionKey {
		uint32_t key;
		
		ActionKey(Control::Action type, Event::EventMods mod = 0, EventButton button = 0, short count = 0) {
			// pack the parameters into the 32 bit key...
			// we will only support the lower 8 bits for each, however. (more than enough for our purposes)
			key = 0;
			uint32_t mask = 0xFF;
			key |= type & mask;
			mask <<= 8u;
			key |= (mod << 8) & mask;
			mask <<= 8u;
			key |= (button << 16) & mask;
			mask <<= 8u;
			key |= (count << 24) & mask;
		}
		
		bool operator< (const ActionKey& ak) const {
			return key < ak.key;
		}
	};

	// for convenience because we need to get this so much
	// all it is is a saved pointer returned from View::GetWindow and is updated in an override for AddedToView
	Window* window;
	
	bool SupportsAction(const ActionKey&);
	bool PerformAction(const ActionKey&);

private:
	// if the input is held: fires the action at the interval specified by ActionRepeatDelay
	// otherwise action fires on input release up only
	unsigned int repeatDelay;
	typedef std::map<ActionKey, ControlEventHandler>::const_iterator ActionIterator;
	std::map<ActionKey, ControlEventHandler> actions;
	Timer* actionTimer;
	
	/** True if we are currently in an event handler */
	int InHandler;

	/** the value of the control to add to the variable */
	ieDword Value;
	ValueRange range;

};


}

#endif
