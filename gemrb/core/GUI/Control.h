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
#define IE_GUI_CONSOLE		10 // gemrb extension
#define IE_GUI_VIEW			254 // gemrb extension
#define IE_GUI_INVALID          255

#include "RGBAColor.h"
#include "exports.h"
#include "globals.h"

#include "Callback.h"
#include "GUI/View.h"
#include "Timer.h"

#include <limits>
#include <map>

namespace GemRB {

class Control;
class Sprite2D;

#define ACTION_CAST(a) \
static_cast<Control::Action>(a)

#define ACTION_IS_SCREEN(a) \
(a <= Control::HoverEnd)

#define ACTION_DEFAULT ControlActionKey(Control::Click, 0, GEM_MB_ACTION, 1)
#define ACTION_CUSTOM(x)  ACTION_CAST(Control::CustomAction + int(x))

#define CTL_INVALID_VALUE ieDword(-1)

/**
 * @class Control
 * Basic Control Object, also called widget or GUI element. Parent class for Labels, Buttons, etc.
 */

using ControlActionResponder = View::ActionResponder<Control*>;
using ControlEventHandler = ControlActionResponder::Responder;

class GEM_EXPORT Control : public View, public ControlActionResponder {
private:
	void ClearActionTimer();
	Timer* StartActionTimer(const ControlEventHandler& action, unsigned int delay = 0);
	ViewScriptingRef* CreateScriptingRef(ScriptingId id, ResRef group) override;

	void HandleTouchActionTimer(const Control*);
    
public: // Public attributes
	enum Action : ControlActionResponder::Action {
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

		DragDropCreate,
		DragDropSource, // a DragOp was successfully taken from here
		DragDropDest, // a DragOp was successfully dropped here

		CustomAction // entry value for defining custom actions in subclasses. Must be last in enum.
	};
	
	struct ControlDragOp : public DragOp {
		explicit ControlDragOp(Control* c)
		: DragOp(c, c->DragCursor()){}
		
		Control* Source() const {
			return static_cast<Control*>(dragView);
		}
		
		Control* Destination() const {
			return static_cast<Control*>(dropView);
		}
		
		~ControlDragOp() override {
			Control* src = Source();
			ActionKey srckey(Action::DragDropSource);
			
			Control* dst = Destination();
			ActionKey dstkey(Action::DragDropDest);
			
			if (dst) { // only send actions for successful drags
				if (src->SupportsAction(srckey)) {
					src->PerformAction(srckey);
				}
				
				if (dst->SupportsAction(dstkey)) {
					dst->PerformAction(dstkey);
				}
			}
		}
	};

	/** Variable length is 40-1 (zero terminator) */
	char VarName[MAX_VARIABLE_LENGTH];

	/** Defines the Control ID Number used for GUI Scripting */
	ieDword ControlID;
	/** Type of control */
	ieByte ControlType;

	static unsigned int ActionRepeatDelay;

public:
	explicit Control(const Region& frame);
	~Control() override;

	/** Sets the Text of the current control */
	void SetText(const String*);
	virtual void SetText(const String&) {};

	/** Update the control if it's tied to a GUI variable */
	void UpdateState(const char*, unsigned int);
	virtual void UpdateState(unsigned int) {}

	/** Returns the Owner */
	virtual void SetFocus();
	bool IsFocused();

	bool TracksMouseDown() const override { return bool(actionTimer); }
	
	UniqueDragOp DragOperation() override;
	bool AcceptsDragOperation(const DragOp&) const override;
	virtual Holder<Sprite2D> DragCursor() const;

	//Events
	void SetAction(Responder handler, Action type, EventButton button = 0,
                   Event::EventMods mod = 0, short count = 0);
	void SetAction(Responder handler, const ActionKey& key = ACTION_DEFAULT) override;
	void SetActionInterval(unsigned int interval=ActionRepeatDelay);

	bool PerformAction(); // perform default action (left mouse button up)
	bool PerformAction(const ActionKey&) override;
	bool SupportsAction(const ActionKey&) override;

	virtual String QueryText() const { return String(); }

	typedef std::pair<ieDword, ieDword> ValueRange;
	const static ValueRange MaxValueRange;
	
	ieDword GetValue() const { return Value; }
	ValueRange GetValueRange() const { return range; }
	
	void SetValue(ieDword val);
	void SetValueRange(ValueRange range = MaxValueRange);
	void SetValueRange(ieDword min, ieDword max = std::numeric_limits<ieDword>::max());
	
	bool HitTest(const Point& p) const override;
	
protected:
	using ActionKey = ControlActionResponder::ActionKey;
	struct ControlActionKey : public ActionKey {		
		static uint32_t BuildKeyValue(Control::Action type, Event::EventMods mod = 0, EventButton button = 0, short count = 0) {
			// pack the parameters into the 32 bit key...
			// we will only support the lower 8 bits for each, however. (more than enough for our purposes)
			uint32_t key = 0;
			uint32_t mask = 0x000000FF;
			key |= type & mask;
			key |= (mod & mask) << 8;
			key |= (button & mask) << 16;
			key |= (count & mask) << 24;
			return key;
		}

		ControlActionKey(Control::Action type, Event::EventMods mod = 0, EventButton button = 0, short count = 0)
		: ActionKey(BuildKeyValue(type, mod, button, count) ) {}
	};

	void FlagsChanged(unsigned int /*oldflags*/) override;
	
	bool OnMouseUp(const MouseEvent& /*me*/, unsigned short /*Mod*/) override;
	bool OnMouseDown(const MouseEvent& /*me*/, unsigned short /*Mod*/) override;
	void OnMouseEnter(const MouseEvent& /*me*/, const DragOp*) override;
	void OnMouseLeave(const MouseEvent& /*me*/, const DragOp*) override;

	bool OnTouchDown(const TouchEvent& /*te*/, unsigned short /*Mod*/) override;
	bool OnTouchUp(const TouchEvent& /*te*/, unsigned short /*Mod*/) override;
	
	bool OnKeyPress(const KeyboardEvent& /*Key*/, unsigned short /*Mod*/) override;

private:
	// if the input is held: fires the action at the interval specified by ActionRepeatDelay
	// otherwise action fires on input release up only
	unsigned int repeatDelay;
	std::map<ActionKey, ControlEventHandler> actions;
	Timer* actionTimer;

	/** the value of the control to add to the variable */
	ieDword Value = CTL_INVALID_VALUE;
	ValueRange range;

};


}

#endif
