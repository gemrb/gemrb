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
#include "GUI/GUIScriptInterface.h"
#include "GUI/Window.h"

#include "win32def.h"
#include "ie_cursors.h"

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
	VarName[0] = 0;
	Value = 0;
	SetValueRange(MaxValueRange);

	animation = NULL;
	ControlType = IE_GUI_INVALID;

	actionTimer = NULL;
	repeatDelay = 0;
}

Control::~Control()
{
	ClearActionTimer();

	delete animation;
}

bool Control::IsOpaque() const
{
	 return AnimPicture && AnimPicture->HasTransparency() == false;
}

void Control::SetText(const String* string)
{
	SetText((string) ? *string : L"");
}

void Control::SetAction(ControlEventHandler handler, Control::Action type, EventButton button,
						Event::EventMods mod, short count)
{
	ControlActionKey key(type, mod, button, count);
	return SetAction(std::move(handler), key);
}

void Control::SetAction(Responder handler, const ActionKey& key)
{
	if (handler) {
		actions[key] = handler;
	} else {
		// delete the entry if there is one instead of setting it to NULL
		ActionIterator it = actions.find(key);
		if (it != actions.end()) {
			actions.erase(it);
		}
	}
}

void Control::SetActionInterval(unsigned int interval)
{
	repeatDelay = interval;
	if (actionTimer) {
		actionTimer->SetInverval(repeatDelay);
	}
}

bool Control::SupportsAction(const ActionKey& key)
{
	return actions.count(key);
}

bool Control::PerformAction()
{
	return PerformAction(ACTION_DEFAULT);
}

bool Control::PerformAction(const ActionKey& key)
{
	if (IsDisabled()) {
		return false;
	}
	
	ActionIterator it = actions.find(key);
	if (it != actions.end()) {
		if (!window) {
			Log(WARNING, "Control", "Executing event handler for a control with no window. This most likely indicates a programming or scripting error.");
		}

		(it->second)(this);
		return true;
	}
	return false;
}

void Control::FlagsChanged(unsigned int /*oldflags*/)
{
	if (actionTimer && (flags&Disabled)) {
		ClearActionTimer();
	}
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

void Control::SetAnimPicture(Holder<Sprite2D> newpic)
{
	AnimPicture = newpic;
	MarkDirty();
}

void Control::ClearActionTimer()
{
	if (actionTimer) {
		actionTimer->Invalidate();
		actionTimer = NULL;
	}
}

Timer* Control::StartActionTimer(const ControlEventHandler& action, unsigned int delay)
{
	EventHandler h = [this, action] () {
		// update the timer to use the actual repeatDelay
		SetActionInterval(repeatDelay);

		if (VarName[0] != 0) {
			ieDword val = GetValue();
			core->GetDictionary()->SetAt(VarName, val);
			window->RedrawControls(VarName, val);
		}

		return action(this);
	};
	// always start the timer with ActionRepeatDelay
	// this way we have consistent behavior for the initial delay prior to switching to a faster delay
	return &core->SetTimer(h, (delay) ? delay : ActionRepeatDelay);
}

bool Control::HitTest(const Point& p) const
{
	if (!(flags & (IgnoreEvents | Invisible))) {
		return View::HitTest(p);
	}
	return false;
}

View::UniqueDragOp Control::DragOperation()
{
	if (actionTimer) {
		return nullptr;
	}

	ActionKey key(Action::DragDropCreate);
	if (!SupportsAction(key)) {
		return nullptr;
	}

	// we have to use a timer so that the dragop is set before the callback is called
	EventHandler h = [this, key] () {
		actionTimer->Invalidate();
		actionTimer = nullptr;
		return actions[key](this);
	};

	actionTimer = &core->SetTimer(h, 0, 0);
	return std::unique_ptr<ControlDragOp>(new ControlDragOp(this));
}

bool Control::AcceptsDragOperation(const DragOp& dop) const
{
	const ControlDragOp* cdop = dynamic_cast<const ControlDragOp*>(&dop);
	if (cdop) {
		assert(cdop->dragView != this);
		// if 2 controls share the same VarName we assume they are swappable...
		return (strnicmp(VarName, cdop->Source()->VarName, MAX_VARIABLE_LENGTH-1) == 0);
	}

	return View::AcceptsDragOperation(dop);
}

Holder<Sprite2D> Control::DragCursor() const
{
	return core->Cursors[IE_CURSOR_SWAP];
}

bool Control::OnMouseUp(const MouseEvent& me, unsigned short mod)
{
	ControlActionKey key(Click, mod, me.button, me.repeats);
	if (SupportsAction(key)) {
		PerformAction(key);
		ClearActionTimer();
	} else if (me.repeats > 1) {
		// also try a single-click in case there is no doubleclick handler
		// and there is never a triple+ click handler
		MouseEvent me2(me);
		me2.repeats = 1;
		OnMouseUp(me2, mod);
	}
	return true; // always handled
}

bool Control::OnMouseDown(const MouseEvent& me, unsigned short mod)
{
	ControlActionKey key(Click, mod, me.button, me.repeats);
	if (repeatDelay && SupportsAction(key)) {
		actionTimer = StartActionTimer(actions[key]);
	}
	return true; // always handled
}

void Control::OnMouseEnter(const MouseEvent& /*me*/, const DragOp*)
{
	PerformAction(HoverBegin);
}

void Control::OnMouseLeave(const MouseEvent& /*me*/, const DragOp*)
{
	PerformAction(HoverEnd);
}

bool Control::OnTouchDown(const TouchEvent& /*te*/, unsigned short /*mod*/)
{
	ControlEventHandler cb = METHOD_CALLBACK(&Control::HandleTouchActionTimer, this);
	actionTimer = StartActionTimer(cb, 500); // TODO: this time value should be configurable
	return true; // always handled
}

bool Control::OnTouchUp(const TouchEvent& te, unsigned short mod)
{
	if (actionTimer) {
		// touch up before timer triggered
		// send the touch down+up events
		ClearActionTimer();
		View::OnTouchDown(te, mod);
		View::OnTouchUp(te, mod);
		return true;
	}
	return false; // touch was already handled as a long press
}

bool Control::OnKeyPress(const KeyboardEvent& key, unsigned short mod)
{
	if (key.keycode == GEM_RETURN) {
		return PerformAction();
	}
	
	return View::OnKeyPress(key, mod);
}

void Control::HandleTouchActionTimer(Control* ctrl)
{
	assert(ctrl == this);
	assert(actionTimer);

	ClearActionTimer();

	// long press action (GEM_MB_MENU)
	// TODO: we could save the mod value from OnTouchDown to support modifiers to the touch, but we dont have a use ATM
	ControlActionKey key(Click, 0, GEM_MB_MENU, 1);
	PerformAction(key);
}

ViewScriptingRef* Control::CreateScriptingRef(ScriptingId id, ResRef group)
{
	return new ControlScriptingRef(this, id, group);
}

}
