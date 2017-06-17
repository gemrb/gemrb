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

#include "GUI/Control.h"
#include "GUI/Window.h"

#include "win32def.h"

#include "ControlAnimation.h"
#include "Interface.h"
#include "Sprite2D.h"
#include "Variables.h"

#include <cstdio>
#include <cstring>

namespace GemRB {

unsigned int Control::ActionRepeatDelay = 250;

const Control::ValueRange Control::MaxValueRange = std::make_pair(0, std::numeric_limits<ieDword>::max());

Control::Control(const Region& frame)
: View(frame) // dont pass superview to View constructor
{
	InHandler = 0;
	VarName[0] = 0;
	Value = 0;
	SetValueRange(MaxValueRange);

	animation = NULL;
	AnimPicture = NULL;
	ControlType = IE_GUI_INVALID;

	actionTimer = NULL;
	repeatDelay = 0;
}

Control::~Control()
{
	if (actionTimer)
		actionTimer->Invalidate();

	if (InHandler) {
		Log(ERROR, "Control", "Destroying control inside event handler, crash may occur!");
	}
	delete animation;

	Sprite2D::FreeSprite(AnimPicture);
}

void Control::SetText(const String* string)
{
	SetText((string) ? *string : L"");
}

void Control::SetAction(ControlEventHandler handler)
{
	actions[ACTION_DEFAULT] = handler;
}

void Control::SetAction(ControlEventHandler handler, Control::Action type, EventButton button,
						Event::EventMods mod, short count)
{
	actions[ActionKey(type, mod, button, count)] = handler;
}

void Control::SetActionInterval(unsigned int interval)
{
	repeatDelay = interval;
	if (actionTimer) {
		actionTimer->SetInverval(repeatDelay);
	}
}

bool Control::SupportsAction(Action action)
{
	return SupportsAction(ActionKey(action));
}

bool Control::SupportsAction(const ActionKey& key)
{
	return actions.count(key);
}

bool Control::PerformAction()
{
	return PerformAction(ACTION_DEFAULT);
}

bool Control::PerformAction(Action action)
{
	return PerformAction(ActionKey(action));
}

bool Control::PerformAction(const ActionKey& key)
{
	if (IsDisabled()) {
		return false;
	}
	
	ActionIterator it = actions.find(key);
	if (it != actions.end()) {
		if (InHandler) {
			Log(ERROR, "Control", "Executing nested event handler. This is undefined behavior and may blow up.");
		}

		if (!window) {
			Log(WARNING, "Control", "Executing event handler for a control with no window. This most likely indicates a programming or scripting error.");
		}

		++InHandler;
		// TODO: detect caller errors, trap them???
		// TODO: add support for callbacks that return a bool?
		(it->second)(this);
		--InHandler;
		assert(InHandler >= 0);

		return true;
	}
	return false;
}

void Control::UpdateState(const char* varname, unsigned int val)
{
	if (strnicmp(VarName, varname, MAX_VARIABLE_LENGTH-1) == 0) {
		UpdateState(val);
	}
}

void Control::SetFocus()
{
	window->SetFocused(this);
	MarkDirty();
}

bool Control::IsFocused()
{
	return window->FocusedView() == this;
}

void Control::SetValue(ieDword val)
{
	ieDword oldVal = Value;
	Value = Clamp(val, range.first, range.second);

	if (oldVal != Value) {
		if (VarName[0] != 0) {
			core->GetDictionary()->SetAt( VarName, Value );
		}
		PerformAction(ValueChange);
		MarkDirty();
	}
}

void Control::SetValueRange(ValueRange r)
{
	range = r;
	SetValue(Value); // update the value if it falls outside the range
}

void Control::SetValueRange(ieDword min, ieDword max)
{
	SetValueRange(ValueRange(min, max));
}

void Control::SetAnimPicture(Sprite2D* newpic)
{
	Sprite2D::FreeSprite(AnimPicture);
	AnimPicture = newpic;
	MarkDirty();
}

Timer* Control::StartActionTimer(const ControlEventHandler& action)
{
	class RepeatControlEventHandler : public Callback<void, void> {
		const ControlEventHandler action;
		Control* ctrl;

	public:
		RepeatControlEventHandler(const ControlEventHandler& handler, Control* c)
		: action(handler), ctrl(c) {}

		void operator()() const {
			// update the timer to use the actual repeatDelay
			ctrl->SetActionInterval(ctrl->repeatDelay);
			return action(ctrl);
		}
	};

	EventHandler h = new RepeatControlEventHandler(action, this);
	// always start the timer with ActionRepeatDelay
	// this way we have consistent behavior for the initial delay prior to switching to a faster delay
	return &core->SetTimer(h, ActionRepeatDelay);
}

void Control::OnMouseUp(const MouseEvent& me, unsigned short mod)
{
	ActionKey key(Click, mod, me.button, me.repeats);
	if (SupportsAction(key)) {
		PerformAction(key);
		if (actionTimer) {
			actionTimer->Invalidate();
			actionTimer = NULL;
		}
	} else if (me.repeats > 1) {
		// also try a single-click in case there is no doubleclick handler
		// and there is never a triple+ click handler
		MouseEvent me2(me);
		me2.repeats = 1;
		OnMouseUp(me2, mod);
	}
}

void Control::OnMouseDown(const MouseEvent& me, unsigned short mod)
{
	ActionKey key(Click, mod, me.button, me.repeats);
	if (repeatDelay && SupportsAction(key)) {
		actionTimer = StartActionTimer(actions[key]);
	}
}

}
