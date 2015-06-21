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
		group = winref->ScriptingGroup();
		if (!(id&0xffffffff00000000)) {
			id |= (winref->Id << 32);
			id |= 0x8000000000000000;
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
	id &= 0x00000000ffffffff;
	if (ctrl->Owner) {
		WindowScriptingRef* winref = static_cast<WindowScriptingRef*>(ctrl->Owner->GetScriptingRef());
		if (winref) {
			id |= (winref->Id << 32);
			id |= 0x8000000000000000;
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
