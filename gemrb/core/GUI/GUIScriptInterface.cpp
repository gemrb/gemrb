/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2015 The GemRB Project
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
 */

#include "GUIScriptInterface.h"

#include "Resource.h"

namespace GemRB {

static inline ScriptingId ModifiedCtrlIdForWin(ScriptingId id, WindowScriptingRef* winref)
{
	if (winref) {
		id &= 0x00000000ffffffff; // control id is lower 32bits
		id |= (winref->Id << 32); // window id will be storeed in upper 32 bits, but only uses 16
		id |= 0x8000000000000000; // MSB signifies this is a control id (so that win win id 0 and ctrl 0 on that win have distinct ids)
	}
	return id;
}

View* GetView(ScriptingRefBase* base)
{
	ViewScriptingRef* ref = dynamic_cast<ViewScriptingRef*>(base);
	if (ref) {
		return ref->GetObject();
	}
	return NULL;
}

ControlScriptingRef* GetControlRef(ScriptingId id, Window* win)
{
	ResRef group = "Control";
	if (win) {
		WindowScriptingRef* winref = static_cast<WindowScriptingRef*>(win->GetScriptingRef());
		if (winref) {
			group = winref->ScriptingGroup();
			id = ModifiedCtrlIdForWin(id, winref);
		}
	}
	ScriptingRefBase* base = ScriptEngine::GetScripingRef(group, id);
	return static_cast<ControlScriptingRef*>(base);
}

Control* GetControl(ScriptingId id, Window* win)
{
	View* view = GetView( GetControlRef(id, win) );
	return static_cast<Control*>(view);
}

Window* GetWindow(ScriptingId id, ResRef pack)
{
	View* view = GetView( ScriptEngine::GetScripingRef(pack, id) );
	return dynamic_cast<Window*>(view);
}

ControlScriptingRef* RegisterScriptableControl(Control* ctrl, ScriptingId id)
{
	if (!ctrl) return NULL;
	assert(ctrl->GetScriptingRef() == NULL);

	ResRef group = "Control";
	if (ctrl->Owner) {
		WindowScriptingRef* winref = static_cast<WindowScriptingRef*>(ctrl->Owner->GetScriptingRef());
		if (winref) {
			id = ModifiedCtrlIdForWin(id, winref);
			group = winref->ScriptingGroup();
		}
	}

	ctrl->ControlID = (ieDword)id;
	ControlScriptingRef* ref = new ControlScriptingRef(ctrl, id, group);
	if (ScriptEngine::RegisterScriptingRef(ref)) {
		ctrl->AssignScriptingRef(ref);
		return ref;
	}
	// registration with script engine failed
	delete ref;
	return NULL;
}

WindowScriptingRef* RegisterScriptableWindow(Window* win, ResRef pack, ScriptingId id)
{
	if (!win) return NULL;
	assert(win->GetScriptingRef() == NULL);

	WindowScriptingRef* ref = new WindowScriptingRef(win, id, pack);
	if (ScriptEngine::RegisterScriptingRef(ref)) {
		win->AssignScriptingRef(ref);
		return ref;
	}
	// registration with script engine failed
	delete ref;
	return NULL;
}

}
